#include "main.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "app.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "uart.h"
#include "azure_functions.h"

extern SensorValues _sensor_values;

static void get_parameters(int argc, char **argv, uint32_t *baudrate, char **serial_port);
static void upload_sensor_values();

BGLIB_DEFINE();

int main(int argc, char **argv) {
    uint32_t last_reading_id = 0;
    uint32_t baudrate;
    char *serial_port;
    struct gecko_cmd_packet *event;

    CURLcode res;

    set_led_color(LED_RED);
    usleep(300000);
    set_led_color(LED_GREEN);
    usleep(300000);
    set_led_color(LED_YELLOW);
    usleep(300000);
    set_led_color(LED_GREEN);
    usleep(300000);
    set_led_color(LED_YELLOW);

    res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
      printf("curl_global_init failed\n");
      flash_led();
    }

    if (azure_init() != 0) {
        printf("ERROR: Azure Init Failed.\n");
        flash_led();
    }

    BGLIB_INITIALIZE_NONBLOCK(serial_write, uartRx, uartRxPeek);

    get_parameters(argc, argv, &baudrate, &serial_port);
    if (uartOpen((int8_t *)serial_port, baudrate, 0, 100) < 0) {
        printf("ERROR: Serial Port Initialization Failed.\n");
        flash_led();
    }

    gecko_cmd_system_reset(0);

    while (1) {
        event = gecko_peek_event();

        if (event) {
            handle_event(event);
        }

        if (_sensor_values.id > last_reading_id) {
            set_led_color(LED_YELLOW);
            last_reading_id = _sensor_values.id;
            upload_sensor_values();
            set_led_color(LED_GREEN);
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
    while (1) {
        set_led_color(LED_RED);
        sleep(1);
        set_led_color(LED_OFF);
        sleep(1);
    }
}

static void get_parameters(int argc, char **argv, uint32_t *baudrate, char **serial_port) {
    if (argc == 1) {
        *baudrate = 115200;
        *serial_port = "/dev/ttyS1";
        printf("\nUsing Default Parameters:\n");
    } else if (argc == 3) {
        *baudrate = atoi(argv[2]);
        *serial_port = argv[1];
    } else {
        printf(USAGE, argv[0]);
        flash_led();
    }

    printf("Baud Rate: %d\nSerial Port: %s\n", *baudrate, *serial_port);

    return;
}

void serial_write(uint32_t length, uint8_t *data) {
    int32_t result = uartTx(length, data);
    if (result < 0) {
        printf("ERROR: Failed to write to serial port\n  Error: %d\nErrno: %d", result, errno);
    }
}

static void upload_sensor_values() {
    printf("\nSending Sensor Values:\n");
    printf("  Temperature: %f\n", _sensor_values.temperature);
    printf("  Pressure: %f\n", _sensor_values.pressure);
    printf("  Humidity: %f\n", _sensor_values.humidity);
    printf("  CO2: %f\n", _sensor_values.co2);
    printf("  VOC: %f\n", _sensor_values.voc);
    printf("  Light: %f\n", _sensor_values.light);
    printf("  Sound: %f\n", _sensor_values.sound);
    printf("  Acceleration: (%f, %f, %f)\n", _sensor_values.acceleration[0], _sensor_values.acceleration[1],
           _sensor_values.acceleration[2]);
    printf("  Orientation: (%f, %f, %f)\n", _sensor_values.orientation[0], _sensor_values.orientation[1],
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