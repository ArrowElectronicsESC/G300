#include "main.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "uart.h"

extern SensorValues _sensor_values;

static void get_parameters(int argc, char **argv, uint32_t *baudrate, char **serial_port);
static void upload_sensor_values();

BGLIB_DEFINE();

int main(int argc, char **argv) {
    uint32_t last_reading_id = 0;
    uint32_t baudrate;
    char *serial_port;
    struct gecko_cmd_packet *event;

    CURL *curl;

    CURLcode res;

    curl = curl_easy_init();

    printf("Hello!\n");
    if (curl) {
        printf("Curl easy init passed!\n");
        curl_easy_setopt(curl, CURLOPT_URL, argv[1]);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            printf("curl_easy_perform failed\n");
            return -1;
        } else {
            printf("Success!!\n");
        }

        curl_easy_cleanup(curl);

    } else {
        printf("curl_easy_init failed\n");
        return -2;
    }

    printf("goodbye!");

#if 0
    BGLIB_INITIALIZE_NONBLOCK(serial_write, uartRx, uartRxPeek);

    get_parameters(argc, argv, &baudrate, &serial_port);
    if (uartOpen((int8_t *)serial_port, baudrate, 0, 100) < 0) {
        printf("ERROR: Serial Port Initialization Failed.\n");
        exit(EXIT_FAILURE);
    }

    gecko_cmd_system_reset(0);

    while (1) {
        event = gecko_peek_event();

        if (event) {
            handle_event(event);
        }

        if (_sensor_values.id > last_reading_id) {
            last_reading_id = _sensor_values.id;
            upload_sensor_values();
        }
    }
#endif
    return 0;
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
        exit(EXIT_FAILURE);
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
}