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
#include <time.h>


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
    "attribute_read",
    "idle"
};

states  _state;
uint16  _tb_sense_device_index;
uint8   _tb_connection;
uint16  _num_devices_found;
bd_addr _found_devices[MAX_DEVICES];
tb_sense_2 _thunderboard;
tb_sensor *_subscribe_characteristic;
uint16 _last_read_sensor_index;

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

void create_sensor(uint8 uuid[], int uuid_length, subscription_type subscription, sensor_id id, tb_sensor *sensor) {
    sensor->uuid_length = uuid_length;
    sensor->uuid_bytes = malloc(uuid_length);
    sensor->subscribe = subscription;
    sensor->id = id;
    memcpy(sensor->uuid_bytes, uuid, uuid_length);
}

void init_thunderboard() {
    memset(&_thunderboard, 0, sizeof(_thunderboard));
    {
        uint8 temperature_uuid[] = {0x6E, 0x2A};
        create_sensor(temperature_uuid, sizeof(temperature_uuid), sub_none, sensor_temperature, &_thunderboard.temperature);
    }
    {
        uint8 pressure_uuid[] = { 0x6D, 0x2A };
        create_sensor(pressure_uuid, sizeof(pressure_uuid), sub_none, sensor_pressure, &_thunderboard.pressure);
    }
    {
        uint8 humidity_uuid[] = { 0x6F, 0x2A };
        create_sensor(humidity_uuid, sizeof(humidity_uuid), sub_none, sensor_humidity, &_thunderboard.humidity);
    }
    {
        uint8 co2_uuid[] = { 0x3B, 0x10, 0x19, 0x00, 0xB0, 0x91, 0xE7, 0x76, 0x33, 0xEF, 0x01,
            0xC4, 0xAE, 0x58, 0xD6, 0xEF};
        create_sensor(co2_uuid, sizeof(co2_uuid), sub_none, sensor_co2, &_thunderboard.co2);
    }
    {
        uint8 voc_uuid[] = { 0x3B, 0x10, 0x19, 0x0, 0xB0, 0x91, 0xE7, 0x76, 0x33, 0xEF, 0x2, 0xC4, 0xAE, 0x58, 0xD6, 0xEF};
        create_sensor(voc_uuid, sizeof(voc_uuid), sub_none, sensor_voc, &_thunderboard.voc);
    }
    {
        uint8 light_uuid[] = { 0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D, 0xEB, 0x45, 0xD9, 0xBF, 0x13, 0x69, 0x54, 0xC8};
        create_sensor(light_uuid, sizeof(light_uuid), sub_none, sensor_light, &_thunderboard.light);
    }
    {
        uint8 uv_uuid[] = {0x76, 0x2A};
        create_sensor(uv_uuid, sizeof(uv_uuid), sub_none, sensor_uv, &_thunderboard.uv);
    }
    {
        uint8 sound_uuid[] = { 0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D, 0xEB, 0x45, 0x2, 0xBF, 0x13, 0x69, 0x54, 0xC8 };
        create_sensor(sound_uuid, sizeof(sound_uuid), sub_none, sensor_sound, &_thunderboard.sound);
    }
    {
        uint8 acceleration_uuid[] = { 0x9F, 0xDC, 0x9C, 0x81, 0xFF, 0xFE, 0x5D, 0x88, 0xE5, 0x11, 0xE5, 0x4B, 0xE2, 0xF6, 0xC1, 0xC4 };
        create_sensor(acceleration_uuid, sizeof(acceleration_uuid), sub_needs, sensor_acceleration, &_thunderboard.acceleration);
    }
    {
        uint8 orientation_uuid[] = { 0x9A, 0xF4, 0x94, 0xE9, 0xB5, 0xF3, 0x9F, 0xBA, 0xDD, 0x45, 0xE3, 0xBE, 0x94, 0xB6, 0xC4, 0xB7 };
        create_sensor(orientation_uuid, sizeof(orientation_uuid), sub_needs, sensor_orientation, &_thunderboard.orientation);
    }
    {
        uint8 rgb_uuid[] = { 0x1B, 0x40, 0x4A, 0x44, 0xCE, 0x5E, 0xC3, 0x7D, 0xF3, 0x59, 0x3, 0xC6, 0x40, 0x9C, 0xB8, 0xFC };
        create_sensor(rgb_uuid, sizeof(rgb_uuid), sub_none, sensor_rgb, &_thunderboard.rgb);
    }
}

int main(int argc, char *argv[]) {
    time_t last_sensor_read_time;
    last_sensor_read_time = time(NULL);

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

        case state_reading_attribute:
        case state_subscribing:
        case state_finding_information:
        case state_find_information:
        {
            wait_for_state_change();
        }
        break;

        case state_information_found:
            change_state(state_subscribe);
        break;

        case state_subscribe:
        {
            int sensor_index = 0;
            for (sensor_index = 0; sensor_index < NUM_TB2_SENSORS; sensor_index++) {
                if (_thunderboard.all[sensor_index].subscribe == sub_needs) {
                    if (_thunderboard.all[sensor_index].handle == 0) {
                        printf("ERROR Missing handle");
                        return;
                    }
                    _subscribe_characteristic = &_thunderboard.all[sensor_index];
                    uint8 subscribe_data[] = { 0x03, 0x00 };
                    ble_cmd_attclient_attribute_write(_tb_connection, _thunderboard.all[sensor_index].handle + 1, 2, subscribe_data);
                    _thunderboard.all[sensor_index].subscribe = sub_fulfilled;
                    change_state(state_subscribing);
                    break;
                }
            }
            // All subscriptions have been made
            if (_state == state_subscribe) {
                change_state(state_subscribed);
            }
        }
        break;

        case state_subscribed:
            change_state(state_idle);
        break;

        case state_idle:
        {
            while ((time(NULL) - last_sensor_read_time) < 2) {
                if (read_message(UART_TIMEOUT) > 0) {
                    uart_close();
                    change_state(state_init);
                }
            }
            _last_read_sensor_index = -1;
            change_state(state_read_attribute);
        }
        break;

        case state_read_attribute:
        {
            for (_last_read_sensor_index++; _last_read_sensor_index < NUM_TB2_SENSORS;) {
                if (_thunderboard.all[_last_read_sensor_index].subscribe == sub_none) {
                    ble_cmd_attclient_read_by_handle(_tb_connection, _thunderboard.all[_last_read_sensor_index].handle);
                    change_state(state_reading_attribute);
                    break;
                } else {
                    _last_read_sensor_index++;
                }
            }
            if (_state == state_read_attribute) {
                change_state(state_idle);
            }
        }
        break;

        default:
            printf("ERROR: Unhandled state %s\n", state_names[_state]);
            exit(1);
        break;
        }
    }

}