#ifdef PLATFORM_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "cmd_def.h"
#include "uart.h"

const char *state_names[state_last] = {
    "init",
    "find_devices",
    "finding_devices",
    "stop_finding_devices",
    "stopping_discovery",
    "discovery_stopped",
    "intiate_connection",
    "establishing_connection",
    "connection_established",
    "find_information",
    "finding_information",
    "informatin_found",
    "subscribe",
    "subscribing",
    "subscribed",
    "read_attribute",
    "reading_attribute",
    "attribute_read"
};

states  _state;
uint16  _tb_sense_device_index;
uint8   _tb_connection;
uint16  _num_devices_found;
bd_addr _found_devices[MAX_DEVICES];
tb_sense_2 _thunderboard;


void usleep(__int64 usec) {
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

void change_state(states new_state) {
#ifdef DEBUG
    printf("DEBUG: State changed: %s --> %s\n", state_names[_state], state_names[new_state]);
#endif
    _state = new_state;
}

void print_uuid(uint8array *uuid) {
    int uuidIndex;
    for (uuidIndex = 0; uuidIndex < uuid->len; uuidIndex++) {
        printf("%02X", uuid->data[uuidIndex]);
    }
}

void print_bdaddr(bd_addr bdaddr) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
        bdaddr.addr[5],
        bdaddr.addr[4],
        bdaddr.addr[3],
        bdaddr.addr[2],
        bdaddr.addr[1],
        bdaddr.addr[0]);
}

void print_raw_packet(struct ble_header *hdr, unsigned char *data) {
    printf("RX: ");
    int i;
    for (i = 0; i < sizeof(*hdr); i++) {
        printf("%02x ", ((unsigned char *)hdr)[i]);
    }
    for (i = 0; i < hdr->lolen; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

/**
 * Compare Bluetooth addresses
 *
 * @param first First address
 * @param second Second address
 * @return Zero if addresses are equal
 */
int cmp_bdaddr(bd_addr first, bd_addr second) {
    int i;
    for (i = 0; i < sizeof(bd_addr); i++) {
        if (first.addr[i] != second.addr[i]) return 1;
    }
    return 0;
}

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2) {
    if (uart_tx(len1, data1) || uart_tx(len2, data2)) {
        printf("ERROR: Writing to serial port failed\n");
        exit(1);
    }
}

int read_message(int timeout_ms) {
    unsigned char data[256]; // enough for BLE
    struct ble_header hdr;
    int r;

    r = uart_rx(sizeof(hdr), (unsigned char *)&hdr, UART_TIMEOUT);
    if (!r) {
        return -1; // timeout
    } else if (r < 0) {
        printf("ERROR: Reading header failed. Error code:%d\n", r);
        return 1;
    }

    if (hdr.lolen) {
        r = uart_rx(hdr.lolen, data, UART_TIMEOUT);
        if (r <= 0) {
            printf("ERROR: Reading data failed. Error code:%d\n", r);
            return 1;
        }
    }

    const struct ble_msg *msg = ble_get_msg_hdr(hdr);

#ifdef DEBUG
    print_raw_packet(&hdr, data);
#endif

    if (!msg) {
        printf("ERROR: Unknown message received\n");
        exit(1);
    }

    msg->handler(data);

    return 0;
}

void wait_for_state_change() {
    states initial_state = _state;
    while (initial_state == _state) {
        if (read_message(UART_TIMEOUT) > 0) {
            uart_close();
            change_state(state_init);
        }
    }
}

void init_thunderboard() {
    memset(&_thunderboard, 0, sizeof(_thunderboard));
    {
        uint8 temperature_uuid[] = {0x6E, 0x2A};
        _thunderboard.temperature.uuid_length = sizeof(temperature_uuid);
        _thunderboard.temperature.uuid_bytes = malloc(sizeof(temperature_uuid));
        memcpy(_thunderboard.temperature.uuid_bytes, temperature_uuid, sizeof(temperature_uuid));
    }
    {
        uint8 pressure_uuid[] = { 0x6D, 0x2A };
        _thunderboard.pressure.uuid_length = sizeof(pressure_uuid);
        _thunderboard.pressure.uuid_bytes = malloc(sizeof(pressure_uuid));
        memcpy(_thunderboard.pressure.uuid_bytes, pressure_uuid, sizeof(pressure_uuid));
    }
}

int main(int argc, char *argv[]) {
    // Init bgapi
    bglib_output = output;

    init_thunderboard();

    change_state(state_init);

    while (1) {
        switch (_state) {
        case state_init:
        {
            dbg_printf("Opening Serial Port " UART_PORT "... ");
            if (uart_open(UART_PORT)) {
                printf("ERROR: Unable to open serial port\n");
                return 1;
            }
            dbg_printf("DONE\n");

            ble_cmd_system_reset(0);
            uart_close();
            do {
                usleep(500000); // 0.5s
            } while (uart_open(UART_PORT));
            change_state(state_find_devices);
        }
        break;
        
        case state_find_devices:
        {
            _tb_sense_device_index = -1;
            _num_devices_found = 0;
            dbg_printf("Scanning for devices...\n");
            ble_cmd_gap_discover(gap_discover_observation);
            change_state(state_finding_devices);
        }
        break;

        case state_finding_devices:
        {
            wait_for_state_change();
        }
        break;

        case state_stop_finding_devices:
        {
            change_state(state_stopping_discovery);
            ble_cmd_gap_end_procedure();
        }
        break;

        case state_stopping_discovery:
        {
            wait_for_state_change();
        }
        break;

        case state_discovery_stopped:
        {
            if (_tb_sense_device_index != -1) {
                // Thunderboard was found. Connect to it
                change_state(state_intiate_connection);
            } else {
                // Thunderboard not found. Start over.
                change_state(state_find_devices);
            }
        }
        break;

        case state_intiate_connection:
        {
            change_state(state_establishing_connection);
            ble_cmd_gap_connect_direct(&_found_devices[_tb_sense_device_index], gap_address_type_public, 40, 60, 100, 0);
        }
        break;

        case state_establishing_connection:
        {
            wait_for_state_change();
        }
        break;

        case state_connection_established:
            change_state(state_find_information);
            ble_cmd_attclient_find_information(_tb_connection, 0x01, 0xFFFF);
        break;

        case state_finding_information:
        case state_find_information:
        {
            wait_for_state_change();
        }
        break;

        default:
            printf("ERROR: Unhandled state %s\n", state_names[_state]);
            exit(1);
        break;
        }
    }

}