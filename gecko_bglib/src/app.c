#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bg_types.h"
#include "gecko_bglib.h"

static void initialize_thunderboard();
static void handle_state_transition(AppState new_state);
static void print_message_info(struct gecko_cmd_packet *event);
static bool handle_advertisement(struct gecko_cmd_packet *event);
static void refresh_sensor_values();
static Characteristic *get_characteristic_by_handle(uint16_t handle);

static void state_handler_init(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_connect(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_service_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_characteristic_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_subscribe_characteristics(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_read_characteristics(uint32_t message_id, struct gecko_cmd_packet *event, bool entry);

SensorValues _sensor_values = {0};
static bool _system_ready = FALSE;
static AppState _state = STATE_INIT;
static ThunderBoardDevice _thunderboard = {0};
static state_handler _state_handlers[NUM_STATES] = {&state_handler_init,
                                                    &state_handler_discovery,
                                                    &state_handler_connect,
                                                    &state_handler_service_discovery,
                                                    &state_handler_characteristic_discovery,
                                                    &state_handler_subscribe_characteristics,
                                                    &state_handler_read_characteristics};
static char *_state_names[NUM_STATES] = {"INIT",
                                         "DISCOVERY",
                                         "CONNECT",
                                         "DISCOVER SERVICES",
                                         "DISCOVER CHARACTERISTICS",
                                         "SUBSCRIBE CHARACTERISTICS",
                                         "READ CHARACTERISTIC VALUES"};

void handle_event(struct gecko_cmd_packet *event) {
    if (event == NULL) {
        printf("ERROR: NULL event pointer in state %s.\n", _state_names[_state]);
    }

    print_message_info(event);

    uint32_t message_id = BGLIB_MSG_ID(event->header);

    if (_state < NUM_STATES) {
        _state_handlers[_state](message_id, event, FALSE);
    } else {
        printf("ERROR: Unhandled State: %d\n", _state);
        exit(EXIT_FAILURE);
    }

    return;
}

static void print_message_info(struct gecko_cmd_packet *event) {
    uint32_t message_id = BGLIB_MSG_ID(event->header);

    printf("\n\nMESSAGE\n  Header: 0x%X\n  Message ID: 0x%X\n", event->header, message_id);
    if (event->header & gecko_msg_type_rsp) {
        printf("  Type: RESPONSE\n");
    } else if (event->header & gecko_msg_type_evt) {
        printf("  Type: EVENT\n");
    }

    printf("  Name: ");
    switch (message_id) {
        case gecko_evt_system_boot_id:
            printf("gecko_evt_system_boot_id");
            break;
        case gecko_evt_le_gap_scan_response_id:
            printf("gecko_evt_le_gap_scan_response_id");
            break;
        case gecko_evt_gatt_service_id:
            printf("gecko_evt_gatt_service_id");
            break;
        case gecko_evt_gatt_characteristic_id:
            printf("gecko_evt_gatt_characteristic_id");
            break;
        case gecko_evt_gatt_procedure_completed_id:
            printf("gecko_evt_gatt_procedure_completed_id");
            break;
        case gecko_evt_gatt_characteristic_value_id:
            printf("gecko_evt_gatt_characteristic_value_id");
            break;
        case gecko_evt_le_connection_opened_id:
            printf("gecko_evt_le_connection_opened_id");
            break;
        case gecko_evt_gatt_server_user_write_request_id:
            printf("gecko_evt_gatt_server_user_write_request_id");
            break;
        case gecko_evt_le_connection_parameters_id:
            printf("gecko_evt_le_connection_parameters_id");
            break;
        case gecko_evt_le_connection_phy_status_id:
            printf("gecko_evt_le_connection_phy_status_id");
            break;
        case gecko_evt_le_connection_closed_id:
            printf("gecko_evt_le_connection_closed_id");
            break;
        default:
            printf("UNKNOWN");
            break;
    }

    printf("\n\n");
}

static void print_advertisement(struct gecko_msg_le_gap_scan_response_evt_t *response, char *name) {
    printf("\nAdvertisement:\n");
    printf("  Name:    %s\n", name);
    printf("  Address: %02X:%02X:%02X:%02X:%02X:%02X\n\n", response->address.addr[5], response->address.addr[4],
           response->address.addr[3], response->address.addr[2], response->address.addr[1], response->address.addr[0]);
}

static void print_buffer(uint32_t length, uint8_t *data, bool reversed) {
    uint32_t byte_index;
    if (reversed) {
        for (byte_index = length - 1; byte_index >= 0; byte_index--) {
            printf("%02X ", data[byte_index]);
        }
    } else {
        for (byte_index = 0; byte_index < length; byte_index++) {
            printf("%02X ", data[byte_index]);
        }
    }
}

static void print_characteristic(struct gecko_msg_gatt_characteristic_evt_t *characteristic, uint32_t service) {
    printf("\nCharacteristic Info:\n");
    printf("  Service: 0x%X\n", service);
    printf("  Characteristic: 0x%X\n", characteristic->characteristic);
    printf("  Properties: 0x%X\n", characteristic->properties);
    printf("  UUID: ");
    print_buffer(characteristic->uuid.len, characteristic->uuid.data, FALSE);
    printf("\n\n");
}

static Characteristic *get_characteristic_by_handle(uint16_t handle) {
    Characteristic *requested_characteristic = NULL;
    uint16_t sensor_index = 0;

    for (sensor_index = 0; sensor_index < NUM_THUNDERBOARD_SENSORS; sensor_index++) {
        if (_thunderboard.all_sensors[sensor_index]->characteristic == handle) {
            requested_characteristic = _thunderboard.all_sensors[sensor_index];
            break;
        }
    }

    return requested_characteristic;
}

static void initialize_thunderboard() {
    _thunderboard.initialized = TRUE;

    return;
}

static double translate_value(double value, double input_range_min, double input_range_max, double output_range_min,
                              double output_range_max) {
    double input_span = input_range_max - input_range_min;
    double output_span = output_range_max - output_range_min;

    double scaled_value = (value - input_range_min) / input_span;

    return output_range_min + (scaled_value * output_span);
}

static void refresh_sensor_values() {
    _sensor_values.id++;

    _sensor_values.temperature = ((double)(*((int16_t *)_thunderboard.temperature_sensor->value))) * 0.01;
    _sensor_values.pressure = ((double)(*((uint32_t *)_thunderboard.pressure_sensor->value)) * 0.1);
    _sensor_values.humidity = ((double)(*((uint16_t *)_thunderboard.humidity_sensor->value)) * 0.01);
    _sensor_values.co2 = (double)(*((uint16_t *)_thunderboard.co2_sensor->value));
    _sensor_values.voc = ((double)(*((uint16_t *)_thunderboard.voc_sensor->value)) * 0.01);
    _sensor_values.light = ((double)(*((uint32_t *)_thunderboard.light_sensor->value)) * 0.001);
    _sensor_values.sound = ((double)(*((uint16_t *)_thunderboard.sound_sensor->value)) * 0.01);

    _sensor_values.acceleration[0] = ((double)(*((uint16_t *)(_thunderboard.acceleration_sensor->value + 0)))) * 0.001;
    _sensor_values.acceleration[1] = ((double)(*((uint16_t *)(_thunderboard.acceleration_sensor->value + 2)))) * 0.001;
    _sensor_values.acceleration[2] = ((double)(*((uint16_t *)(_thunderboard.acceleration_sensor->value + 4)))) * 0.001;

    _sensor_values.orientation[0] =
        translate_value(*((uint16_t *)(_thunderboard.orientation_sensor->value + 0)), INT16_MIN, INT16_MAX, -180, 180);
    _sensor_values.orientation[1] =
        translate_value(*((uint16_t *)(_thunderboard.orientation_sensor->value + 2)), INT16_MIN, INT16_MAX, -90, 90);
    _sensor_values.orientation[2] =
        translate_value(*((uint16_t *)(_thunderboard.orientation_sensor->value + 4)), INT16_MIN, INT16_MAX, -180, 180);

    return;
}

static bool handle_advertisement(struct gecko_cmd_packet *event) {
    int length = 0;
    int offset = 0;
    int type = 0;
    uint8_t *data;
    bool found_thunderboard = FALSE;
    char *thunderboard_prefix = "Thunder Sense #";
    char name_buffer[MAX_NAME_LENGTH] = "UNKNOWN";

    while (offset < event->data.evt_le_gap_scan_response.data.len) {
        length = event->data.evt_le_gap_scan_response.data.data[offset];
        type = event->data.evt_le_gap_scan_response.data.data[offset + 1];
        data = event->data.evt_le_gap_scan_response.data.data;

        switch (type) {
            case 0x08:  // Short Local Name
            case 0x09:  // Complete Local Name
                memcpy(name_buffer, &data[offset + 2], length - 1);
                name_buffer[length] = '\0';
                break;

            default:
                break;
        }

        if (found_thunderboard) {
            break;
        }

        offset += length + 1;
    }

    print_advertisement(&event->data.evt_le_gap_scan_response, name_buffer);

    uint32_t name_length = strlen(name_buffer);
    if (name_length > strlen(thunderboard_prefix)) {
        if (memcmp(thunderboard_prefix, name_buffer, strlen(thunderboard_prefix)) == 0) {
            memcpy(_thunderboard.name, name_buffer, name_length);
            _thunderboard.rssi = event->data.evt_le_gap_scan_response.rssi;
            memcpy(_thunderboard.address.addr, event->data.evt_le_gap_scan_response.address.addr, 6);
            found_thunderboard = TRUE;
        }
    }

    return found_thunderboard;
}

static void handle_state_transition(AppState new_state) {
    if (new_state >= NUM_STATES) {
        printf("ERROR: Unhandled State: %d\n", new_state);
        exit(EXIT_FAILURE);
    }

    printf("===================================================\n");
    printf("State Transition: %s --> %s\n", _state_names[_state], _state_names[new_state]);
    printf("===================================================\n");

    _state = new_state;
    _state_handlers[_state](0, NULL, TRUE);

    return;
}

static void state_handler_init(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    if (_thunderboard.initialized) {
        initialize_thunderboard();
    }

    switch (message_id) {
        case gecko_evt_system_boot_id:
            printf("System Booted\n");
            _system_ready = TRUE;
            handle_state_transition(STATE_DISCOVERY);
            break;

        default:
            usleep(5000);
            break;
    }
}

static void state_handler_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    if (entry) {
        struct gecko_msg_le_gap_set_discovery_type_rsp_t *set_discovery_response;
        struct gecko_msg_le_gap_start_discovery_rsp_t *start_discovery_response;

        memset(&_thunderboard, 0, sizeof(_thunderboard));

        set_discovery_response = gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);
        if (set_discovery_response->result == 0) {
            start_discovery_response = gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_general_discoverable);
            if (start_discovery_response->result != 0) {
                printf("ERROR: gecko_cmd_le_gap_start_discovery failure - %d\n", start_discovery_response->result);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("ERROR: gecko_cmd_le_gap_set_discovery_type failure - %d\n", set_discovery_response->result);
            exit(EXIT_FAILURE);
        }

        return;
    }

    switch (message_id) {
        case gecko_evt_le_gap_scan_response_id:
            if (handle_advertisement(event)) {
                struct gecko_msg_le_gap_end_procedure_rsp_t *response = gecko_cmd_le_gap_end_procedure();
                if (response->result == 0) {
                    handle_state_transition(STATE_CONNECT);
                } else {
                    printf("ERROR: gecko_cmd_le_gap_end_procedure failure - %d\n", response->result);
                }
            }
            break;

        case gecko_evt_gatt_procedure_completed_id:
            break;

        default:
            printf("*********************************\n");
            printf("     WARNING: Unhandled Event    \n");
            printf("*********************************\n");
            break;
    }

    return;
}

static void state_handler_connect(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    if (entry) {
        struct gecko_msg_le_gap_connect_rsp_t *response =
            gecko_cmd_le_gap_connect(_thunderboard.address, le_gap_address_type_public, le_gap_phy_1m);
        if (response->result == 0) {
            _thunderboard.connection = response->connection;
        } else {
            printf("ERROR: gecko_cmd_le_gap_connect failed - 0x%X\n", response->result);
            exit(EXIT_FAILURE);
        }
        return;
    }

    switch (message_id) {
        case gecko_evt_le_connection_opened_id:
            handle_state_transition(STATE_DISCOVER_SERVICES);
            break;

        case gecko_evt_le_connection_closed_id:
            handle_state_transition(STATE_DISCOVERY);
            break;

        default:
            printf("*********************************\n");
            printf("     WARNING: Unhandled Event    \n");
            printf("*********************************\n");
            break;
    }

    return;
}

static void state_handler_service_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    if (entry) {
        struct gecko_msg_gatt_discover_primary_services_rsp_t *response =
            gecko_cmd_gatt_discover_primary_services(_thunderboard.connection);
        if (response->result != 0) {
            printf("ERROR: gecko_cmd_gatt_discover_primary_services failed - 0x%X\n", response->result);
            exit(EXIT_FAILURE);
        }
        return;
    }

    switch (message_id) {
        case gecko_evt_gatt_service_id:
            printf("Found Service[%d]: %d\n", _thunderboard.services.length, event->data.evt_gatt_service.service);
            if (_thunderboard.services.length == MAX_NUM_SERVICES) {
                printf("ERROR: Max Services Exceeded");
                exit(1);
            }
            _thunderboard.services.list[_thunderboard.services.length++] = event->data.evt_gatt_service.service;
            break;

        case gecko_evt_gatt_procedure_completed_id:
            handle_state_transition(STATE_DISCOVER_CHARACTERISTICS);
            break;

        case gecko_evt_le_connection_closed_id:
            handle_state_transition(STATE_DISCOVERY);
            break;

        default:
            printf("*********************************\n");
            printf("     WARNING: Unhandled Event    \n");
            printf("*********************************\n");
            break;
    }

    return;
}

static void state_handler_characteristic_discovery(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    static uint32_t service_index = 0;

    if (entry) {
        struct gecko_msg_gatt_discover_characteristics_rsp_t *response = gecko_cmd_gatt_discover_characteristics(
            _thunderboard.connection, _thunderboard.services.list[service_index]);
        if (response->result != 0) {
            printf("ERROR: gecko_cmd_gatt_discover_characteristics failed - 0x%X\n", response->result);
            exit(EXIT_FAILURE);
        }

        return;
    }

    switch (message_id) {
        case gecko_evt_gatt_characteristic_id: {
            print_characteristic(&event->data.evt_gatt_characteristic, _thunderboard.services.list[service_index]);

            Characteristic *new_characteristic =
                &_thunderboard.characteristics.list[_thunderboard.characteristics.length++];
            new_characteristic->characteristic = event->data.evt_gatt_characteristic.characteristic;
            new_characteristic->properties.all = event->data.evt_gatt_characteristic.properties;
            memcpy(&new_characteristic->uuid.bytes, event->data.evt_gatt_characteristic.uuid.data,
                   event->data.evt_gatt_characteristic.uuid.len);
            new_characteristic->uuid.length = event->data.evt_gatt_characteristic.uuid.len;

            do {
                if (_thunderboard.temperature_sensor == NULL) {
                    uint8_t temperature_uuid[] = {0x6E, 0x2A};
                    if (new_characteristic->uuid.length == sizeof(temperature_uuid) &&
                        memcmp(temperature_uuid, new_characteristic->uuid.bytes, sizeof(temperature_uuid)) == 0) {
                        _thunderboard.temperature_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.pressure_sensor == NULL) {
                    uint8_t pressure_uuid[] = {0x6D, 0x2A};
                    if (new_characteristic->uuid.length == sizeof(pressure_uuid) &&
                        memcmp(pressure_uuid, new_characteristic->uuid.bytes, sizeof(pressure_uuid)) == 0) {
                        _thunderboard.pressure_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.humidity_sensor == NULL) {
                    uint8_t humidity_uuid[] = {0x6F, 0x2A};
                    if (new_characteristic->uuid.length == sizeof(humidity_uuid) &&
                        memcmp(humidity_uuid, new_characteristic->uuid.bytes, sizeof(humidity_uuid)) == 0) {
                        _thunderboard.humidity_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.uv_sensor == NULL) {
                    uint8_t uv_uuid[] = {0x76, 0x2A};
                    if (new_characteristic->uuid.length == sizeof(uv_uuid) &&
                        memcmp(uv_uuid, new_characteristic->uuid.bytes, sizeof(uv_uuid)) == 0) {
                        _thunderboard.uv_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.co2_sensor == NULL) {
                    uint8_t co2_uuid[] = {0x3B, 0x10, 0x19, 0x00, 0xB0, 0x91, 0xE7, 0x76,
                                          0x33, 0xEF, 0x01, 0xC4, 0xAE, 0x58, 0xD6, 0xEF};
                    if (new_characteristic->uuid.length == sizeof(co2_uuid) &&
                        memcmp(co2_uuid, new_characteristic->uuid.bytes, sizeof(co2_uuid)) == 0) {
                        _thunderboard.co2_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.voc_sensor == NULL) {
                    uint8_t voc_uuid[] = {0x3B, 0x10, 0x19, 0x0,  0xB0, 0x91, 0xE7, 0x76,
                                          0x33, 0xEF, 0x2,  0xC4, 0xAE, 0x58, 0xD6, 0xEF};
                    if (new_characteristic->uuid.length == sizeof(voc_uuid) &&
                        memcmp(voc_uuid, new_characteristic->uuid.bytes, sizeof(voc_uuid)) == 0) {
                        _thunderboard.voc_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.light_sensor == NULL) {
                    uint8_t light_uuid[] = {0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D,
                                            0xEB, 0x45, 0xD9, 0xBF, 0x13, 0x69, 0x54, 0xC8};
                    if (new_characteristic->uuid.length == sizeof(light_uuid) &&
                        memcmp(light_uuid, new_characteristic->uuid.bytes, sizeof(light_uuid)) == 0) {
                        _thunderboard.light_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.sound_sensor == NULL) {
                    uint8_t sound_uuid[] = {0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D,
                                            0xEB, 0x45, 0x2,  0xBF, 0x13, 0x69, 0x54, 0xC8};
                    if (new_characteristic->uuid.length == sizeof(sound_uuid) &&
                        memcmp(sound_uuid, new_characteristic->uuid.bytes, sizeof(sound_uuid)) == 0) {
                        _thunderboard.sound_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.acceleration_sensor == NULL) {
                    uint8_t acceleration_uuid[] = {0x9F, 0xDC, 0x9C, 0x81, 0xFF, 0xFE, 0x5D, 0x88,
                                                   0xE5, 0x11, 0xE5, 0x4B, 0xE2, 0xF6, 0xC1, 0xC4};
                    if (new_characteristic->uuid.length == sizeof(acceleration_uuid) &&
                        memcmp(acceleration_uuid, new_characteristic->uuid.bytes, sizeof(acceleration_uuid)) == 0) {
                        _thunderboard.acceleration_sensor = new_characteristic;
                        break;
                    }
                }
                if (_thunderboard.orientation_sensor == NULL) {
                    uint8_t orientation_uuid[] = {0x9A, 0xF4, 0x94, 0xE9, 0xB5, 0xF3, 0x9F, 0xBA,
                                                  0xDD, 0x45, 0xE3, 0xBE, 0x94, 0xB6, 0xC4, 0xB7};
                    if (new_characteristic->uuid.length == sizeof(orientation_uuid) &&
                        memcmp(orientation_uuid, new_characteristic->uuid.bytes, sizeof(orientation_uuid)) == 0) {
                        _thunderboard.orientation_sensor = new_characteristic;
                        break;
                    }
                }
            } while (0);
        } break;

        case gecko_evt_gatt_procedure_completed_id: {
            service_index++;
            if (service_index < _thunderboard.services.length) {
                struct gecko_msg_gatt_discover_characteristics_rsp_t *response =
                    gecko_cmd_gatt_discover_characteristics(_thunderboard.connection,
                                                            _thunderboard.services.list[service_index]);
                if (response->result != 0) {
                    printf("ERROR: gecko_cmd_gatt_discover_characteristics failed - 0x%X\n", response->result);
                    exit(EXIT_FAILURE);
                }
            } else {
                handle_state_transition(STATE_SUBSCRIBE_CHARACTERISTICS);
            }
        } break;

        default:
            printf("*********************************\n");
            printf("     WARNING: Unhandled Event    \n");
            printf("*********************************\n");
            break;
    }

    return;
}

static void state_handler_subscribe_characteristics(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    static uint32_t subscribe_sensor_index = 0;

    if (entry) {
        for (subscribe_sensor_index = 0; subscribe_sensor_index < NUM_THUNDERBOARD_SENSORS; subscribe_sensor_index++) {
            if (_thunderboard.all_sensors[subscribe_sensor_index] != NULL &&
                (_thunderboard.all_sensors[subscribe_sensor_index]->properties.notify ||
                 _thunderboard.all_sensors[subscribe_sensor_index]->properties.indicate)) {
                struct gecko_msg_gatt_set_characteristic_notification_rsp_t *response =
                    gecko_cmd_gatt_set_characteristic_notification(
                        _thunderboard.connection, _thunderboard.all_sensors[subscribe_sensor_index]->characteristic, 3);
                if (response->result != 0) {
                    printf("ERROR: gecko_msg_gatt_server_write_attribute_value_rsp_t failed - %d\n", response->result);
                    exit(EXIT_FAILURE);
                }
                break;
            }
        }

        return;
    }

    switch (message_id) {
        case gecko_evt_gatt_procedure_completed_id: {
            subscribe_sensor_index++;
            for (; subscribe_sensor_index < NUM_THUNDERBOARD_SENSORS; subscribe_sensor_index++) {
                if (_thunderboard.all_sensors[subscribe_sensor_index] != NULL &&
                    (_thunderboard.all_sensors[subscribe_sensor_index]->properties.notify ||
                     _thunderboard.all_sensors[subscribe_sensor_index]->properties.indicate)) {
                    struct gecko_msg_gatt_set_characteristic_notification_rsp_t *response =
                        gecko_cmd_gatt_set_characteristic_notification(
                            _thunderboard.connection, _thunderboard.all_sensors[subscribe_sensor_index]->characteristic,
                            3);
                    if (response->result != 0) {
                        printf("ERROR: gecko_msg_gatt_server_write_attribute_value_rsp_t failed - %d\n",
                               response->result);
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
            }
            if (subscribe_sensor_index >= NUM_THUNDERBOARD_SENSORS) {
                handle_state_transition(STATE_READ_CHARACTERISTIC_VALUES);
            }
        } break;

        default:
            printf("*********************************\n");
            printf("     WARNING: Unhandled Event    \n");
            printf("*********************************\n");
            break;
    }
}

static void state_handler_read_characteristics(uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
    static uint32_t sensor_index = 0;

    if (entry) {
        uint32_t i;
        for (i = 0; i < NUM_THUNDERBOARD_SENSORS; i++) {
            printf("all_sensors[%u]: %p\n", i, _thunderboard.all_sensors[i]);
        }

        for (sensor_index = 0; sensor_index < NUM_THUNDERBOARD_SENSORS; sensor_index++) {
            if (_thunderboard.all_sensors[sensor_index] != NULL) {
                struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
                    gecko_cmd_gatt_read_characteristic_value(_thunderboard.connection,
                                                             _thunderboard.all_sensors[sensor_index]->characteristic);
                if (response->result != 0) {
                    printf("ERROR: gecko_cmd_gatt_read_characteristic_value failed - 0x%X\n", response->result);
                    exit(EXIT_FAILURE);
                }
                break;
            } else {
                printf("WARNING: NULL Sensor at index %d\n", sensor_index);
            }
        }
        return;
    }

    switch (message_id) {
        case gecko_evt_gatt_characteristic_value_id: {
            printf("Got Characteristic value\n");
            Characteristic *current_characteristic =
                get_characteristic_by_handle(event->data.evt_gatt_characteristic_value.characteristic);
            if (current_characteristic == NULL) {
                printf("ERROR: get_characteristic_by_handle returned NULL\n");
            } else {
                memcpy(current_characteristic->value, event->data.evt_gatt_characteristic_value.value.data,
                       event->data.evt_gatt_characteristic_value.value.len);
                current_characteristic->value_length = event->data.evt_gatt_characteristic_value.value.len;
                if (current_characteristic->characteristic ==
                    event->data.evt_gatt_characteristic_value.characteristic) {
                    sensor_index++;
                }
            }
        } break;

        case gecko_evt_gatt_procedure_completed_id:
            for (; sensor_index < NUM_THUNDERBOARD_SENSORS; sensor_index++) {
                if (_thunderboard.all_sensors[sensor_index] == NULL) {
                    printf("WARNING: NULL Sensor at index %d\n", sensor_index);
                } else if (_thunderboard.all_sensors[sensor_index]->properties.read) {
                    struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
                        gecko_cmd_gatt_read_characteristic_value(
                            _thunderboard.connection, _thunderboard.all_sensors[sensor_index]->characteristic);
                    if (response->result != 0) {
                        printf("ERROR: gecko_cmd_gatt_read_characteristic_value failed - 0x%X\n", response->result);
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
            }

            if (sensor_index >= NUM_THUNDERBOARD_SENSORS) {
                sensor_index = 0;
                refresh_sensor_values();
                struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
                    gecko_cmd_gatt_read_characteristic_value(_thunderboard.connection,
                                                             _thunderboard.all_sensors[sensor_index]->characteristic);
                if (response->result != 0) {
                    printf("ERROR: gecko_cmd_gatt_read_characteristic_value failed - 0x%X\n", response->result);
                    exit(EXIT_FAILURE);
                }
            }
            break;
    }

    return;
}