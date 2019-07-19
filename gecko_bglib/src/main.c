#include "main.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "app.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "uart.h"
#include "azure_functions.h"
#include "log.h"

//TODO Before Release - Set advertisement timeout time much higher
// orientation 0-180
// sound
// air pressure
extern SensorValues _sensor_values;

static FILE *_log_file = NULL;

static int get_parameters(int argc, char **argv, G300Args *args);
static void upload_sensor_values();

BGLIB_DEFINE();

int main(int argc, char **argv) {
    uint32_t last_reading_id = 0;
    G300Args arguments = {0};
    struct gecko_cmd_packet *event = NULL;

    CURLcode res;

    if (get_parameters(argc, argv, &arguments)) {
        exit(-1);
    }

    set_led_color(LED_RED);
    usleep(300000);
    set_led_color(LED_GREEN);
    usleep(300000);
    set_led_color(LED_YELLOW);
    usleep(300000);
    set_led_color(LED_GREEN);
    usleep(300000);
    set_led_color(LED_YELLOW);

    _log_file = fopen("/data/g300.log", "w");
    if (_log_file) {
        log_set_fp(_log_file);
        log_set_level(arguments.log_level);
    }

    res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
      log_fatal("curl_global_init failed: %s", curl_easy_strerror(res));
      flash_led();
    }

    if (wait_for_network_connection(30)) {
        log_fatal("no network available");
        flash_led();
    }

    if (azure_init()) {
        log_fatal("Azure Init Failed.");
        flash_led();
    } else {
        log_trace("Azure Initialized.");
    }

    BGLIB_INITIALIZE_NONBLOCK(serial_write, uartRx, uartRxPeek);

    if (uartOpen((int8_t *)arguments.serial_port, arguments.baudrate, 0, 100)
        < 0) {
        log_fatal("Serial Port Initialization Failed");
        flash_led();
    } else {
        log_trace("Serial Port Initialized.");
    }

    set_led_color(LED_YELLOW);

    gecko_cmd_system_reset(0);

    while (1) {
        event = gecko_peek_event();

        if (event) {
            handle_event(event);
        }

        if (_sensor_values.id > last_reading_id) {
            set_led_color(LED_YELLOW);
            usleep(700000);
            last_reading_id = _sensor_values.id;
            upload_sensor_values();
            set_led_color(LED_GREEN);

            if ((last_reading_id % 10) == 0) {
                log_info("Azure Upload (%d)", last_reading_id);
            }
        }
    }

    return 0;
}

void set_led_color(G300LedColor color) {
    switch (color) {
        case LED_RED:
            system("usockc /var/gpio_ctrl WLAN_LED_RED");
        break;
        case LED_GREEN:
            system("usockc /var/gpio_ctrl WLAN_LED_GREEN");
        break;
        case LED_YELLOW:
            system("usockc /var/gpio_ctrl WLAN_LED_YELLOW");
        break;
        case LED_OFF:
            system("usockc /var/gpio_ctrl WLAN_LED_OFF");
        break;
    }
}

void flash_led() {
    log_debug("FLASH LED");

    for(int i = 0; i < 15; i++) {
        set_led_color(LED_RED);
        sleep(1);
        set_led_color(LED_OFF);
        sleep(1);
    }

    if (_log_file) {
        fclose(_log_file);
    }

    exit(-1);
}

static int get_parameters(int argc, char **argv, G300Args *args) {
    args->baudrate = 115200;
    snprintf(args->serial_port, sizeof(args->serial_port), "/dev/ttyS1");
    args->log_level = LOG_DEBUG;

    if (argc == 1) {
        return 0;
    }

    bool expect_baud = FALSE;
    bool got_baud = FALSE;
    bool expect_serial = FALSE;
    bool got_serial = FALSE;
    bool expect_log = FALSE;
    bool got_log = FALSE;

    for (uint32_t arg_index = 1; arg_index < argc; arg_index++) {
        if (expect_baud) {
            args->baudrate = atoi(argv[arg_index]);
            expect_baud = FALSE;
            got_baud = TRUE;
        } else if (expect_serial) {
            snprintf(args->serial_port, sizeof(args->serial_port), "%s",
                argv[arg_index]);
            expect_serial = FALSE;
            got_serial = TRUE;
        } else if (expect_log) {
            args->log_level = atoi(argv[arg_index]);
            expect_log = FALSE;
            got_log = TRUE;
        } else {
            if (strcmp(argv[arg_index], "-b") == 0) {
                if (got_baud) {
                    printf(USAGE, argv[0]);
                    return -1;
                } else {
                    expect_baud = TRUE;
                }
            } else if (strcmp(argv[arg_index], "-s") == 0) {
                if (got_serial) {
                    printf(USAGE, argv[0]);
                    return -1;
                } else {
                    expect_serial = TRUE;
                }
            } else if (strcmp(argv[arg_index], "-l") == 0) {
                if (got_log) {
                    printf(USAGE, argv[0]);
                    return -1;
                } else {
                    expect_log = TRUE;
                }
            } else {
                printf(USAGE, argv[0]);
                return -1;
            }
        }
    }

    if (expect_baud || expect_serial || expect_log) {
        printf(USAGE, argv[0]);
        return -1;
    }

    log_info("Baud Rate: %d", args->baudrate);
    log_info("Serial Port: %s", args->serial_port);
    log_info("Log Level: %d", args->log_level);

    return 0;
}

void serial_write(uint32_t length, uint8_t *data) {
    int32_t result = uartTx(length, data);
    if (result < 0) {
        log_error("ERROR: Failed to write to serial port\n  Error: %d\nErrno: %d", result, errno);
    }
}

static void upload_sensor_values() {
    log_trace("Sending Sensor Values:");
    log_trace("  Temperature: %f", _sensor_values.temperature);
    log_trace("  Pressure: %f", _sensor_values.pressure);
    log_trace("  Humidity: %f", _sensor_values.humidity);
    log_trace("  CO2: %f", _sensor_values.co2);
    log_trace("  VOC: %f", _sensor_values.voc);
    log_trace("  Light: %f", _sensor_values.light);
    log_trace("  Sound: %f", _sensor_values.sound);
    log_trace("  Acceleration: (%f, %f, %f)", _sensor_values.acceleration[0], _sensor_values.acceleration[1],
           _sensor_values.acceleration[2]);
    log_trace("  Orientation: (%f, %f, %f)", _sensor_values.orientation[0], _sensor_values.orientation[1],
           _sensor_values.orientation[2]);

    char json_buffer[1024];
    snprintf(json_buffer, 1024, "{\"temp\":%f,"
    "\"press\":%f,"
    "\"hum\":%f,"
    "\"co2\":%f,"
    "\"voc\":%f,"
    "\"ambientlight\":%f,"
    "\"sound\":%f,"
    "\"accx\":%f,"
    "\"accy\":%f,"
    "\"accz\":%f,"
    "\"orientationx\":%f,"
    "\"orientationy\":%f,"
    "\"orientationz\":%f}",
    _sensor_values.temperature, _sensor_values.pressure, _sensor_values.humidity, _sensor_values.co2, _sensor_values.voc, _sensor_values.light,
    _sensor_values.sound, _sensor_values.acceleration[0],_sensor_values.acceleration[1],_sensor_values.acceleration[2],
    _sensor_values.orientation[0],_sensor_values.orientation[1],_sensor_values.orientation[2]);

    azure_post_telemetry(json_buffer);
}
