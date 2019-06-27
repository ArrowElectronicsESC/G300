#include "main.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "uart.h"

static void get_parameters(int argc, char **argv, uint32_t *baudrate, char **serial_port);

BGLIB_DEFINE();

int main(int argc, char **argv) {
    uint32_t baudrate;
    char *serial_port;
    struct gecko_cmd_packet *event;

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
    }

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