#include <stdio.h>
#include <stdint.h>

#include "main.h"
#include "cmd_def.h"

extern states  _state;
extern uint16  _tb_sense_device_index;
extern uint8   _tb_connection;
extern uint16  _num_devices_found;
extern bd_addr _found_devices[MAX_DEVICES];
extern tb_sense_2 _thunderboard;
extern uint16 _last_read_sensor_index;

void ble_rsp_gap_discover(const struct ble_msg_gap_discover_rsp_t *msg) {
    dbg_printf("ble_rsp_gap_discover\n");
}

void ble_rsp_system_get_info(const struct ble_msg_system_get_info_rsp_t *msg) {
    printf("Build: %u, protocol_version: %u, hardware: ", msg->build, msg->protocol_version);
    switch (msg->hw) {
    case 0x01: printf("BLE112"); break;
    case 0x02: printf("BLED112"); break;
    default: printf("Unknown");
    }
    printf("\n");

}


void ble_rsp_gap_end_procedure(const struct ble_msg_gap_end_procedure_rsp_t *msg) {
    dbg_printf("ble_rsp_gap_end_procedure\n");
    change_state(state_discovery_stopped);
}

void ble_rsp_attclient_find_information(const struct ble_msg_attclient_find_information_rsp_t *msg) {
    dbg_printf("ble_rsp_attclient_find_information");
    change_state(state_finding_information);
}


double translate_value(double value, double input_range_min,
    double input_range_max, double output_range_min, double output_range_max) {
    double input_span = input_range_max - input_range_min;
    double output_span = output_range_max - output_range_min;

    double scaled_value = (value - input_range_min) / input_span;

    return output_range_min + (scaled_value * output_span);
}

/*EVENTS*/
void ble_evt_attclient_attribute_value(const struct ble_msg_attclient_attribute_value_evt_t *msg) {
    uint16 byte_index;
    uint16 sensor_index;
    uint8 sensor_was_subscription = 0;

    dbg_printf("ble_evt_attclient_attribute_value\n");
#ifdef DEBUG
    printf("Got Value for Handle: %u\n  [", msg->atthandle);
    for (byte_index = 0; byte_index < msg->value.len; byte_index++) {
        printf("%02X ", msg->value.data[byte_index]);
    }
    printf("]\n");
#endif

    for (sensor_index = 0; sensor_index < NUM_TB2_SENSORS; sensor_index++) {
        if (_thunderboard.all[sensor_index].handle == msg->atthandle) {
            switch (_thunderboard.all[sensor_index].id) {
                case sensor_temperature:
                {
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((int16*)msg->value.data)) * 0.01;
                    dbg_printf("Temperature: %f\n", _thunderboard.all[sensor_index].value.value.double_val);
                }
                break;
                case sensor_pressure:
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((uint32*)msg->value.data)) * 0.1;
                    dbg_printf("Pressure: %f\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_humidity:
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((uint16*)msg->value.data)) * 0.01;
                    dbg_printf("Pressure: %f\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_co2:
                    _thunderboard.all[sensor_index].value.type = type_int;
                    _thunderboard.all[sensor_index].value.value.int_val = *((uint16*)msg->value.data);
                    dbg_printf("CO2: %d\n", _thunderboard.all[sensor_index].value.value.int_val);
                break;
                case sensor_voc:
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((uint16*)msg->value.data)) * 0.01;
                    dbg_printf("VOC: %d\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_light:
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((uint32*)msg->value.data)) * 0.001;
                    dbg_printf("Light: %d\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_uv:
                    _thunderboard.all[sensor_index].value.type = type_int;
                    _thunderboard.all[sensor_index].value.value.double_val = (*((uint8*)msg->value.data));
                    dbg_printf("UV: %d\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_sound:
                    _thunderboard.all[sensor_index].value.type = type_double;
                    _thunderboard.all[sensor_index].value.value.double_val = (double)(*((uint16*)msg->value.data)) * 0.01;
                    dbg_printf("Sound: %f\n", _thunderboard.all[sensor_index].value.value.double_val);
                break;
                case sensor_acceleration:
                    sensor_was_subscription = 1;
                    _thunderboard.all[sensor_index].value.type = type_double_triplet;
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[0] = (double)(*((uint16*)(msg->value.data + 0))) * 0.001;
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[1] = (double)(*((uint16*)(msg->value.data + 2))) * 0.001;
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[2] = (double)(*((uint16*)(msg->value.data + 4))) * 0.001;
                    dbg_printf("Accelerometer: X:%f | Y:%f | Z:%f\n", _thunderboard.all[sensor_index].value.value.double_triplet_val[0],
                        _thunderboard.all[sensor_index].value.value.double_triplet_val[1],
                        _thunderboard.all[sensor_index].value.value.double_triplet_val[2]);
                break;
                case sensor_orientation:
                    sensor_was_subscription = 1;
                    _thunderboard.all[sensor_index].value.type = type_double_triplet;
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[0] = (double)(*((uint16*)(msg->value.data + 0)));
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[1] = (double)(*((uint16*)(msg->value.data + 2)));
                    _thunderboard.all[sensor_index].value.value.double_triplet_val[2] = (double)(*((uint16*)(msg->value.data + 4)));
                    dbg_printf("Orientation: X:%f | Y:%f | Z:%f\n", _thunderboard.all[sensor_index].value.value.double_triplet_val[0],
                        _thunderboard.all[sensor_index].value.value.double_triplet_val[1],
                        _thunderboard.all[sensor_index].value.value.double_triplet_val[2]);
                break;
            }
        }
        
    }

    if (_state == state_reading_attribute && sensor_was_subscription != 1) {
        change_state(state_read_attribute);
    }
}

void ble_evt_system_protocol_error(const struct ble_msg_system_protocol_error_evt_t*msg) {
    dbg_printf("ble_evt_system_protocol_error\n");

}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg) {
    printf("Connection terminated, trying to reconnect\n");
    //ble_cmd_gap_connect_direct(&connect_addr, gap_address_type_public, 40, 60, 100, 0);
}

void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg) {
    dbg_printf("ble_evt_gap_scan_response\n");

    // If the max number of devices has been found. Stop Scanning
    if (_num_devices_found >= MAX_DEVICES) {
        change_state(state_stop_finding_devices);
        return;
    }

    // Check if the device has already been discovered
    uint32 device_index = 0;
    for (device_index = 0; device_index < _num_devices_found; device_index++) {
        if (!cmp_bdaddr(msg->sender, _found_devices[device_index])) {
            return;
        }
    }

    // A new device has been discovered
    _num_devices_found++;
    memcpy(_found_devices[device_index].addr, msg->sender.addr, sizeof(bd_addr));

    // Parse Message
    uint32 data_index = 0;
    char *device_name = NULL;
    for (data_index = 0; data_index < msg->data.len;) {
        int8 name_length = msg->data.data[data_index++];
        if (!name_length) {
            // Leading Zero
            continue;
        }
        if ((data_index + name_length) > msg->data.len) {
            break; // Not enough data
        }
        uint8 type = msg->data.data[data_index++];
        switch (type) {
        case 0x09:
            device_name = malloc(name_length);
            memcpy(device_name, msg->data.data + data_index, name_length - 1);
            device_name[name_length - 1] = '\0';
        }

        data_index += name_length - 1;
    }

#ifdef DEBUG
    printf("SCAN RESPONSE:\n");
    printf("  Address: ");
    print_bdaddr(msg->sender);
    printf("\n  RSSI: %u", msg->rssi);
    printf("\n  Name: ");
#endif
    if (device_name) {
#ifdef DEBUG
        printf("%s\n", device_name);
#endif
        if (strcmp(device_name, THUNDERBOARD_DEVICE_NAME_PREFIX) > 0) {
            _tb_sense_device_index = _num_devices_found - 1;
            change_state(state_stop_finding_devices);
        }
        free(device_name);
    } else {
#ifdef DEBUG
        printf("Unknown\n");
#endif
    }
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg) {
    dbg_printf("ble_evt_connection_status\n");
    if (msg->flags & connection_connected) {
        printf("Connection established.");
        _tb_connection = msg->connection;
        change_state(state_connection_established);
    }
}



void ble_evt_attclient_group_found(const struct ble_msg_attclient_group_found_evt_t *msg) {
    /*if (msg->uuid.len == 0) return;
    uint16 uuid = (msg->uuid.data[1] << 8) | msg->uuid.data[0];

    printf("UUID:\n  length: %d\n", msg->uuid.len);
    print_uuid(msg->uuid);
    // First thermometer service found
    if (state == state_finding_services && uuid == THERMOMETER_SERVICE_UUID && thermometer_handle_start == 0) {
        thermometer_handle_start = msg->start;
        thermometer_handle_end = msg->end;
    }*/
}

void ble_evt_attclient_procedure_completed(const struct ble_msg_attclient_procedure_completed_evt_t *msg) {
    dbg_printf("ble_evt_attclient_procedure_completed\n");

    if (_state == state_finding_information) {
        change_state(state_information_found);
    } else if (_state == state_subscribing) {

        change_state(state_subscribe);
    }
}

void ble_evt_attclient_find_information_found(const struct ble_msg_attclient_find_information_found_evt_t *msg) {
#ifdef DEBUG
    printf("Information Found:\n");
    printf("  UUID: ");
    print_uuid(&msg->uuid);
    printf("\n  Handle: %u\n", msg->chrhandle);
#endif

    // Find uuid in thunderboard uuid list
    uint32 sensor_index = 0;
    for (sensor_index = 0; sensor_index < NUM_TB2_SENSORS; sensor_index++) {
        if (_thunderboard.all[sensor_index].uuid_length == msg->uuid.len) {

            if (memcmp(_thunderboard.all[sensor_index].uuid_bytes, msg->uuid.data, msg->uuid.len) == 0) {
                _thunderboard.all[sensor_index].handle = msg->chrhandle;
                break;
            }
        }
    }
}



void ble_rsp_attclient_write_command(const struct ble_msg_attclient_write_command_rsp_t *msg) {
    dbg_printf("ble_rsp_attclient_write_command\n");
}

void ble_rsp_attclient_indicate_confirm(const struct ble_msg_attclient_indicate_confirm_rsp_t *msg) {
    dbg_printf("ble_rsp_attclient_indicate_confirm\n");

}

void ble_rsp_attclient_attribute_write(const struct ble_msg_attclient_attribute_write_rsp_t *msg) {
    dbg_printf("ble_rsp_attclient_attribute_write\n");
}


