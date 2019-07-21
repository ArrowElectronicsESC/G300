/*******************************************************************************
 * Copyright Arrow Electronics, Inc., 2019
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *******************************************************************************/

/*******************************************************************************
 *  Author: Andrew Reisdorph
 *  Date:   2019/07/21
 *******************************************************************************/

#include "app.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "led_worker.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static void handle_state_transition(AppState new_state);
static void print_message_info(struct gecko_cmd_packet *event);
static bool handle_advertisement(struct gecko_cmd_packet *event);
static void refresh_sensor_values();
static Characteristic *get_characteristic_by_handle(uint16_t handle);
static void bytes_to_hex_string(uint32_t length, uint8_t *data, bool reversed,
                                char *buffer);

static void state_handler_init(uint32_t message_id,
                               struct gecko_cmd_packet *event, bool entry);
static void state_handler_discovery(uint32_t message_id,
                                    struct gecko_cmd_packet *event, bool entry);
static void state_handler_connect(uint32_t message_id,
                                  struct gecko_cmd_packet *event, bool entry);
static void state_handler_service_discovery(uint32_t message_id,
                                            struct gecko_cmd_packet *event,
                                            bool entry);
static void state_handler_characteristic_discovery(
    uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_subscribe_characteristics(
    uint32_t message_id, struct gecko_cmd_packet *event, bool entry);
static void state_handler_read_characteristics(uint32_t message_id,
                                               struct gecko_cmd_packet *event,
                                               bool entry);

SensorValues _sensor_values = {0};
static AppState _state = STATE_INIT;
static ThunderBoardDevice _thunderboard = {0};
static state_handler _state_handlers[NUM_STATES] = {
    &state_handler_init,
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
  if (_state < NUM_STATES) {
    if (event == NULL) {
      log_error("NULL event pointer in state %s.", _state_names[_state]);
    }

    print_message_info(event);

    uint32_t message_id = BGLIB_MSG_ID(event->header);

    _state_handlers[_state](message_id, event, false);
  } else {
    log_error("Unhandled State: %d", _state);
    flash_led();
  }

  return;
}

static void print_message_info(struct gecko_cmd_packet *event) {
  uint32_t message_id = BGLIB_MSG_ID(event->header);

  log_trace("\nMESSAGE\n  Header: 0x%X\n  Message ID: 0x%X", event->header,
            message_id);
  if (event->header & gecko_msg_type_rsp) {
    log_trace("  Type: RESPONSE");
  } else if (event->header & gecko_msg_type_evt) {
    log_trace("  Type: EVENT");
  }

  log_trace("  Name: ");
  switch (message_id) {
  case gecko_evt_system_boot_id:
    log_trace("gecko_evt_system_boot_id");
    break;
  case gecko_evt_le_gap_scan_response_id:
    log_trace("gecko_evt_le_gap_scan_response_id");
    break;
  case gecko_evt_gatt_service_id:
    log_trace("gecko_evt_gatt_service_id");
    break;
  case gecko_evt_gatt_characteristic_id:
    log_trace("gecko_evt_gatt_characteristic_id");
    break;
  case gecko_evt_gatt_procedure_completed_id:
    log_trace("gecko_evt_gatt_procedure_completed_id");
    break;
  case gecko_evt_gatt_characteristic_value_id:
    log_trace("gecko_evt_gatt_characteristic_value_id");
    break;
  case gecko_evt_le_connection_opened_id:
    log_trace("gecko_evt_le_connection_opened_id");
    break;
  case gecko_evt_gatt_server_user_write_request_id:
    log_trace("gecko_evt_gatt_server_user_write_request_id");
    break;
  case gecko_evt_le_connection_parameters_id:
    log_trace("gecko_evt_le_connection_parameters_id");
    break;
  case gecko_evt_le_connection_phy_status_id:
    log_trace("gecko_evt_le_connection_phy_status_id");
    break;
  case gecko_evt_le_connection_closed_id:
    log_trace("gecko_evt_le_connection_closed_id");
    break;
  case gecko_evt_gatt_mtu_exchanged_id:
    log_trace("gecko_evt_gatt_mtu_exchanged_id");
    break;
  default:
    log_trace("UNKNOWN");
    break;
  }
}

static void
print_advertisement(struct gecko_msg_le_gap_scan_response_evt_t *response,
                    char *name) {
  log_trace("Advertisement:");
  log_trace("  Name:    %s", name);
  log_trace("  Address: %02X:%02X:%02X:%02X:%02X:%02X",
            response->address.addr[5], response->address.addr[4],
            response->address.addr[3], response->address.addr[2],
            response->address.addr[1], response->address.addr[0]);
}

static void bytes_to_hex_string(uint32_t length, uint8_t *data, bool reversed,
                                char *buffer) {
  uint32_t bytes_remaining = length;
  char *buffer_ptr = buffer;
  uint8_t *data_ptr = data;

  while (bytes_remaining) {
    for (uint8_t nibble_counter = 0; nibble_counter < 2; nibble_counter++) {
      uint8_t nibble = (((*data_ptr) >> (nibble_counter * 4)) & 0x0F);
      *buffer_ptr++ = nibble + ((nibble < 10) ? 48 : 55);
    }
    if (reversed) {
      data_ptr--;
    } else {
      data_ptr++;
    }
    bytes_remaining--;
  }
  *buffer_ptr = '\0';
}

static void
print_characteristic(struct gecko_msg_gatt_characteristic_evt_t *characteristic,
                     uint32_t service) {
  log_trace("Characteristic Info:");
  log_trace("  Service: 0x%X", service);
  log_trace("  Characteristic: 0x%X", characteristic->characteristic);
  log_trace("  Properties: 0x%X", characteristic->properties);
  char uuid_buffer[256];
  bytes_to_hex_string(characteristic->uuid.len, characteristic->uuid.data,
                      false, uuid_buffer);
  log_trace("  UUID: %s", uuid_buffer);
}

static Characteristic *get_characteristic_by_handle(uint16_t handle) {
  Characteristic *requested_characteristic = NULL;
  uint16_t sensor_index = 0;

  for (sensor_index = 0; sensor_index < NUM_THUNDERBOARD_SENSORS;
       sensor_index++) {
    if (_thunderboard.all_sensors[sensor_index]->characteristic == handle) {
      requested_characteristic = _thunderboard.all_sensors[sensor_index];
      break;
    }
  }

  return requested_characteristic;
}

static double translate_value(double value, double input_range_min,
                              double input_range_max, double output_range_min,
                              double output_range_max) {
  double input_span = input_range_max - input_range_min;
  double output_span = output_range_max - output_range_min;

  double scaled_value = (value - input_range_min) / input_span;

  return output_range_min + (scaled_value * output_span);
}

static void refresh_sensor_values() {
  _sensor_values.id++;

  _sensor_values.temperature =
      ((double)(*((int16_t *)_thunderboard.temperature_sensor->value))) * 0.01;
  _sensor_values.pressure =
      ((double)(*((uint32_t *)_thunderboard.pressure_sensor->value)) * 0.1);
  _sensor_values.humidity =
      ((double)(*((uint16_t *)_thunderboard.humidity_sensor->value)) * 0.01);
  _sensor_values.co2 = (double)(*((uint16_t *)_thunderboard.co2_sensor->value));
  _sensor_values.voc =
      ((double)(*((uint16_t *)_thunderboard.voc_sensor->value)) * 0.01);
  _sensor_values.light =
      ((double)(*((uint32_t *)_thunderboard.light_sensor->value)) * 0.001);
  _sensor_values.sound =
      ((double)(*((uint16_t *)_thunderboard.sound_sensor->value)) * 0.01);

  _sensor_values.acceleration[0] =
      ((double)(*(
          (uint16_t *)(_thunderboard.acceleration_sensor->value + 0)))) *
      0.001;
  _sensor_values.acceleration[1] =
      ((double)(*(
          (uint16_t *)(_thunderboard.acceleration_sensor->value + 2)))) *
      0.001;
  _sensor_values.acceleration[2] =
      ((double)(*(
          (uint16_t *)(_thunderboard.acceleration_sensor->value + 4)))) *
      0.001;

  _sensor_values.orientation[0] = translate_value(
      *((uint16_t *)(_thunderboard.orientation_sensor->value + 0)), 0,
      UINT16_MAX, -180, 180);
  _sensor_values.orientation[1] = translate_value(
      *((uint16_t *)(_thunderboard.orientation_sensor->value + 2)), 0,
      UINT16_MAX, -90, 90);
  _sensor_values.orientation[2] = translate_value(
      *((uint16_t *)(_thunderboard.orientation_sensor->value + 4)), 0,
      UINT16_MAX, -180, 180);

  return;
}

static bool handle_advertisement(struct gecko_cmd_packet *event) {
  int length = 0;
  int offset = 0;
  int type = 0;
  uint8_t *data;
  bool found_thunderboard = false;
  char *thunderboard_prefix = "Thunder Sense #";
  char name_buffer[MAX_NAME_LENGTH] = "UNKNOWN";

  while (offset < event->data.evt_le_gap_scan_response.data.len) {
    length = event->data.evt_le_gap_scan_response.data.data[offset];
    type = event->data.evt_le_gap_scan_response.data.data[offset + 1];
    data = event->data.evt_le_gap_scan_response.data.data;

    switch (type) {
    case 0x08: // Short Local Name
    case 0x09: // Complete Local Name
      memcpy(name_buffer, &data[offset + 2], length - 1);
      name_buffer[length] = '\0';
      break;

    default:
      break;
    }

    offset += (length + 1);
  }

  print_advertisement(&event->data.evt_le_gap_scan_response, name_buffer);

  uint32_t name_length = strlen(name_buffer);
  if (name_length > strlen(thunderboard_prefix)) {
    if (memcmp(thunderboard_prefix, name_buffer, strlen(thunderboard_prefix)) ==
        0) {
      memcpy(_thunderboard.name, name_buffer, name_length);
      _thunderboard.rssi = event->data.evt_le_gap_scan_response.rssi;
      memcpy(_thunderboard.address.addr,
             event->data.evt_le_gap_scan_response.address.addr, 6);
      found_thunderboard = true;
    }
  }

  return found_thunderboard;
}

static void handle_state_transition(AppState new_state) {
  if (new_state >= NUM_STATES) {
    log_error("ERROR: Unhandled State: %d", new_state);
    flash_led();
  }

  log_debug("State Transition: %s --> %s", _state_names[_state],
            _state_names[new_state]);

  _state = new_state;
  _state_handlers[_state](0, NULL, true);

  return;
}

static void state_handler_init(uint32_t message_id,
                               struct gecko_cmd_packet *event, bool entry) {

  if (entry) {
    sleep(5);
    gecko_cmd_system_reset(0);
    _thunderboard.characteristics.length = 0;
    return;
  }

  switch (message_id) {
  case gecko_evt_system_boot_id:
    log_debug("System Booted");
    handle_state_transition(STATE_DISCOVERY);
    break;

  default:
    usleep(5000);
    break;
  }
}

static void state_handler_discovery(uint32_t message_id,
                                    struct gecko_cmd_packet *event,
                                    bool entry) {
  static time_t discovery_start_time;

  if (entry) {
    discovery_start_time = time(NULL);
    struct gecko_msg_le_gap_set_discovery_type_rsp_t *set_discovery_response;
    struct gecko_msg_le_gap_start_discovery_rsp_t *start_discovery_response;

    //memset(&_thunderboard, 0, sizeof(_thunderboard));
   

    set_discovery_response =
        gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);
    if (set_discovery_response->result == 0) {
      start_discovery_response = gecko_cmd_le_gap_start_discovery(
          le_gap_phy_1m, le_gap_general_discoverable);
      if (start_discovery_response->result != 0) {
        log_error("gecko_cmd_le_gap_start_discovery failure - %d",
                  start_discovery_response->result);
        flash_led();
      }
    } else {
      log_error("gecko_cmd_le_gap_set_discovery_type failure - %d",
                set_discovery_response->result);
      flash_led();
    }

    return;
  }

  // Wait a maximum of ADVERTISEMENT_TIMEOUT_SECONDS seconds for a matching
  // advertisement packet
  if ((time(NULL) - discovery_start_time) > ADVERTISEMENT_TIMEOUT_SECONDS) {
    log_error("Advertisement timeout exceeded.");
    flash_led();
  }

  switch (message_id) {
  case gecko_evt_le_gap_scan_response_id:
    if (handle_advertisement(event)) {
      struct gecko_msg_le_gap_end_procedure_rsp_t *response =
          gecko_cmd_le_gap_end_procedure();
      if (response->result == 0) {
        handle_state_transition(STATE_CONNECT);
      } else {
        log_error("gecko_cmd_le_gap_end_procedure failure - %d",
                  response->result);
      }
    }
    break;

  case gecko_evt_gatt_procedure_completed_id:
    break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }

  return;
}

static void state_handler_connect(uint32_t message_id,
                                  struct gecko_cmd_packet *event, bool entry) {
  if (entry) {
    LedJob flash_green_job = {LED_JOB_ON_OFF, 500, {LED_GREEN, 0, 0}, 1};
    push_led_job(flash_green_job);

    struct gecko_msg_le_gap_connect_rsp_t *response = gecko_cmd_le_gap_connect(
        _thunderboard.address, le_gap_address_type_public, le_gap_phy_1m);
    if (response->result == 0) {
      _thunderboard.connection = response->connection;
    } else {
      log_fatal("gecko_cmd_le_gap_connect failed - 0x%X", response->result);
      flash_led();
    }
    return;
  }

  switch (message_id) {
  case gecko_evt_le_connection_opened_id:
    handle_state_transition(STATE_DISCOVER_SERVICES);
    break;

  case gecko_evt_le_connection_closed_id:
    handle_state_transition(STATE_INIT);
    break;

  case gecko_evt_le_gap_scan_response_id:
    // Ignore scan response message
  break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }

  return;
}

static void state_handler_service_discovery(uint32_t message_id,
                                            struct gecko_cmd_packet *event,
                                            bool entry) {
  if (entry) {
    struct gecko_msg_gatt_discover_primary_services_rsp_t *response =
        gecko_cmd_gatt_discover_primary_services(_thunderboard.connection);
    if (response->result != 0) {
      log_fatal("gecko_cmd_gatt_discover_primary_services failed - 0x%X",
                response->result);
      flash_led();
    }
    return;
  }

  switch (message_id) {
  case gecko_evt_gatt_service_id:
    log_trace("Found Service[%d]: %d", _thunderboard.services.length,
              event->data.evt_gatt_service.service);
    if (_thunderboard.services.length == MAX_NUM_SERVICES) {
      log_fatal("Max Services Exceeded");
      flash_led();
    }
    _thunderboard.services.list[_thunderboard.services.length++] =
        event->data.evt_gatt_service.service;
    break;

  case gecko_evt_gatt_procedure_completed_id:
    handle_state_transition(STATE_DISCOVER_CHARACTERISTICS);
    break;

  case gecko_evt_le_connection_closed_id:
    handle_state_transition(STATE_INIT);
    break;

  case gecko_evt_le_connection_parameters_id:
  break;

  case gecko_evt_le_connection_phy_status_id:
  break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }

  return;
}

static void state_handler_characteristic_discovery(
    uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {
  static uint32_t service_index = 0;

  if (entry) {
     //_thunderboard.characteristics.length = 0;
    struct gecko_msg_gatt_discover_characteristics_rsp_t *response =
        gecko_cmd_gatt_discover_characteristics(
            _thunderboard.connection,
            _thunderboard.services.list[service_index]);
    if (response->result != 0) {
      log_fatal("gecko_cmd_gatt_discover_characteristics failed - 0x%X",
                response->result);
      flash_led();
    }

    return;
  }

  switch (message_id) {
  case gecko_evt_gatt_characteristic_id: {
    print_characteristic(&event->data.evt_gatt_characteristic,
                         _thunderboard.services.list[service_index]);

    Characteristic *new_characteristic =
        &_thunderboard.characteristics
             .list[_thunderboard.characteristics.length++];
    new_characteristic->characteristic =
        event->data.evt_gatt_characteristic.characteristic;
    new_characteristic->properties.all =
        event->data.evt_gatt_characteristic.properties;
    memcpy(&new_characteristic->uuid.bytes,
           event->data.evt_gatt_characteristic.uuid.data,
           event->data.evt_gatt_characteristic.uuid.len);
    new_characteristic->uuid.length =
        event->data.evt_gatt_characteristic.uuid.len;

    do {
      if (_thunderboard.temperature_sensor == NULL) {
        uint8_t temperature_uuid[] = {0x6E, 0x2A};
        if (new_characteristic->uuid.length == sizeof(temperature_uuid) &&
            memcmp(temperature_uuid, new_characteristic->uuid.bytes,
                   sizeof(temperature_uuid)) == 0) {
          _thunderboard.temperature_sensor = new_characteristic;
          log_debug("Registered Temperature Characteristic: %d",
                    _thunderboard.temperature_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.pressure_sensor == NULL) {
        uint8_t pressure_uuid[] = {0x6D, 0x2A};
        if (new_characteristic->uuid.length == sizeof(pressure_uuid) &&
            memcmp(pressure_uuid, new_characteristic->uuid.bytes,
                   sizeof(pressure_uuid)) == 0) {
          _thunderboard.pressure_sensor = new_characteristic;
          break;
        }
      }
      if (_thunderboard.humidity_sensor == NULL) {
        uint8_t humidity_uuid[] = {0x6F, 0x2A};
        if (new_characteristic->uuid.length == sizeof(humidity_uuid) &&
            memcmp(humidity_uuid, new_characteristic->uuid.bytes,
                   sizeof(humidity_uuid)) == 0) {
          _thunderboard.humidity_sensor = new_characteristic;
          log_debug("Registered Humidity Characteristic: %d",
                    _thunderboard.humidity_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.uv_sensor == NULL) {
        uint8_t uv_uuid[] = {0x76, 0x2A};
        if (new_characteristic->uuid.length == sizeof(uv_uuid) &&
            memcmp(uv_uuid, new_characteristic->uuid.bytes, sizeof(uv_uuid)) ==
                0) {
          _thunderboard.uv_sensor = new_characteristic;
          log_debug("Registered UV Characteristic: %d",
                    _thunderboard.uv_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.co2_sensor == NULL) {
        uint8_t co2_uuid[] = {0x3B, 0x10, 0x19, 0x00, 0xB0, 0x91, 0xE7, 0x76,
                              0x33, 0xEF, 0x01, 0xC4, 0xAE, 0x58, 0xD6, 0xEF};
        if (new_characteristic->uuid.length == sizeof(co2_uuid) &&
            memcmp(co2_uuid, new_characteristic->uuid.bytes,
                   sizeof(co2_uuid)) == 0) {
          _thunderboard.co2_sensor = new_characteristic;
          log_debug("Registered CO2 Characteristic: %d",
                    _thunderboard.co2_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.voc_sensor == NULL) {
        uint8_t voc_uuid[] = {0x3B, 0x10, 0x19, 0x0,  0xB0, 0x91, 0xE7, 0x76,
                              0x33, 0xEF, 0x2,  0xC4, 0xAE, 0x58, 0xD6, 0xEF};
        if (new_characteristic->uuid.length == sizeof(voc_uuid) &&
            memcmp(voc_uuid, new_characteristic->uuid.bytes,
                   sizeof(voc_uuid)) == 0) {
          _thunderboard.voc_sensor = new_characteristic;
          log_debug("Registered VOC Characteristic: %d",
                    _thunderboard.voc_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.light_sensor == NULL) {
        uint8_t light_uuid[] = {0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D,
                                0xEB, 0x45, 0xD9, 0xBF, 0x13, 0x69, 0x54, 0xC8};
        if (new_characteristic->uuid.length == sizeof(light_uuid) &&
            memcmp(light_uuid, new_characteristic->uuid.bytes,
                   sizeof(light_uuid)) == 0) {
          _thunderboard.light_sensor = new_characteristic;
          log_debug("Registered Light Characteristic: %d",
                    _thunderboard.light_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.sound_sensor == NULL) {
        uint8_t sound_uuid[] = {0x2E, 0xA3, 0xF4, 0x54, 0x87, 0x9F, 0xDE, 0x8D,
                                0xEB, 0x45, 0x2,  0xBF, 0x13, 0x69, 0x54, 0xC8};
        if (new_characteristic->uuid.length == sizeof(sound_uuid) &&
            memcmp(sound_uuid, new_characteristic->uuid.bytes,
                   sizeof(sound_uuid)) == 0) {
          _thunderboard.sound_sensor = new_characteristic;
          log_debug("Registered Sound Characteristic: %d",
                    _thunderboard.sound_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.acceleration_sensor == NULL) {
        uint8_t acceleration_uuid[] = {0x9F, 0xDC, 0x9C, 0x81, 0xFF, 0xFE,
                                       0x5D, 0x88, 0xE5, 0x11, 0xE5, 0x4B,
                                       0xE2, 0xF6, 0xC1, 0xC4};
        if (new_characteristic->uuid.length == sizeof(acceleration_uuid) &&
            memcmp(acceleration_uuid, new_characteristic->uuid.bytes,
                   sizeof(acceleration_uuid)) == 0) {
          _thunderboard.acceleration_sensor = new_characteristic;
          log_debug("Registered Acceleration Characteristic: %d",
                    _thunderboard.acceleration_sensor->characteristic);
          break;
        }
      }
      if (_thunderboard.orientation_sensor == NULL) {
        uint8_t orientation_uuid[] = {0x9A, 0xF4, 0x94, 0xE9, 0xB5, 0xF3,
                                      0x9F, 0xBA, 0xDD, 0x45, 0xE3, 0xBE,
                                      0x94, 0xB6, 0xC4, 0xB7};
        if (new_characteristic->uuid.length == sizeof(orientation_uuid) &&
            memcmp(orientation_uuid, new_characteristic->uuid.bytes,
                   sizeof(orientation_uuid)) == 0) {
          _thunderboard.orientation_sensor = new_characteristic;
          log_debug("Registered Orientation Characteristic: %d",
                    _thunderboard.orientation_sensor->characteristic);
          break;
        }
      }
    } while (0);
  } break;

  case gecko_evt_gatt_procedure_completed_id: {
    service_index++;
    if (service_index < _thunderboard.services.length) {
      struct gecko_msg_gatt_discover_characteristics_rsp_t *response =
          gecko_cmd_gatt_discover_characteristics(
              _thunderboard.connection,
              _thunderboard.services.list[service_index]);
      if (response->result != 0) {
        log_fatal("gecko_cmd_gatt_discover_characteristics failed - 0x%X",
                  response->result);
        flash_led();
      }
    } else {
      handle_state_transition(STATE_SUBSCRIBE_CHARACTERISTICS);
    }
  } break;

  case gecko_evt_le_connection_closed_id:
    handle_state_transition(STATE_INIT);
    break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }

  return;
}

static bool subscribe_to_next_characteristic(bool from_beginning) {
  static uint32_t sensor_index = 0;
  bool subscription_made = false;
  struct gecko_msg_gatt_set_characteristic_notification_rsp_t *response;

  log_trace("from_beginning: %s", from_beginning ? "true" : "false");

  if (from_beginning) {
    sensor_index = 0;
  }

  for (; sensor_index < NUM_THUNDERBOARD_SENSORS; sensor_index++) {
    Characteristic *current_sensor = _thunderboard.all_sensors[sensor_index];
    log_trace("current_sensor [%d] = %p", sensor_index, current_sensor);
    if (current_sensor && (current_sensor->subscribed == false) &&
        (current_sensor->properties.notify ||
         current_sensor->properties.indicate)) {
      response = gecko_cmd_gatt_set_characteristic_notification(
          _thunderboard.connection, current_sensor->characteristic, 3);
      if (response->result) {
        log_fatal("gecko_cmd_gatt_set_characteristic_notification failed - %d",
                  response->result);
        flash_led();
      } else {
        log_debug("Subscribed to characteristic: %d",
                  current_sensor->characteristic);
        subscription_made = true;
        sensor_index++;
        break;
      }
    }
  }

  return subscription_made;
}

static void state_handler_subscribe_characteristics(
    uint32_t message_id, struct gecko_cmd_packet *event, bool entry) {

  log_trace("Subscribe Characteristics. Entry: %s", entry ? "true" : "false");
  
  if (entry) {
    if (!subscribe_to_next_characteristic(true)) {
      log_debug("Subscriptions already fulfilled");
      handle_state_transition(STATE_READ_CHARACTERISTIC_VALUES);
    }
    return;
  }

  switch (message_id) {
  case gecko_evt_gatt_procedure_completed_id:
    if (!subscribe_to_next_characteristic(false)) {
      handle_state_transition(STATE_READ_CHARACTERISTIC_VALUES);
    }
    break;

  case gecko_evt_le_connection_closed_id:
    handle_state_transition(STATE_INIT);
    break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }
}

static void state_handler_read_characteristics(uint32_t message_id,
                                               struct gecko_cmd_packet *event,
                                               bool entry) {
  static uint32_t sensor_index = 0;

  if (entry) {
    LedJob flash_green_red_job = {
        LED_JOB_ALTERNATE, 500, {LED_GREEN, LED_RED, 0}, 2};
    push_led_job(flash_green_red_job);

    uint32_t i;
    for (i = 0; i < NUM_THUNDERBOARD_SENSORS; i++) {
      log_trace("all_sensors[%u]: %p", i, _thunderboard.all_sensors[i]);
    }

    for (sensor_index = 0; sensor_index < NUM_THUNDERBOARD_SENSORS;
         sensor_index++) {
      if (_thunderboard.all_sensors[sensor_index] != NULL) {
        struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
            gecko_cmd_gatt_read_characteristic_value(
                _thunderboard.connection,
                _thunderboard.all_sensors[sensor_index]->characteristic);
        if (response->result != 0) {
          log_error("gecko_cmd_gatt_read_characteristic_value failed - 0x%X",
                    response->result);
          handle_state_transition(STATE_INIT);
        }
        break;
      } else {
        log_warn("NULL Sensor at index %d", sensor_index);
      }
    }
    return;
  }

  switch (message_id) {
  case gecko_evt_gatt_characteristic_value_id: {
    log_trace("Got Characteristic value");
    Characteristic *current_characteristic = get_characteristic_by_handle(
        event->data.evt_gatt_characteristic_value.characteristic);
    if (current_characteristic == NULL) {
      log_error("get_characteristic_by_handle for %d returned NULL",
                event->data.evt_gatt_characteristic_value.characteristic);
    } else {
      memcpy(current_characteristic->value,
             event->data.evt_gatt_characteristic_value.value.data,
             event->data.evt_gatt_characteristic_value.value.len);
      current_characteristic->value_length =
          event->data.evt_gatt_characteristic_value.value.len;
      if (current_characteristic->characteristic ==
          event->data.evt_gatt_characteristic_value.characteristic) {
        sensor_index++;
      }
    }
  } break;

  case gecko_evt_gatt_procedure_completed_id:
    for (; sensor_index < NUM_THUNDERBOARD_SENSORS; sensor_index++) {
      if (_thunderboard.all_sensors[sensor_index] == NULL) {
        log_warn("WARNING: NULL Sensor at index %d", sensor_index);
      } else if (_thunderboard.all_sensors[sensor_index]->properties.read) {
        struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
            gecko_cmd_gatt_read_characteristic_value(
                _thunderboard.connection,
                _thunderboard.all_sensors[sensor_index]->characteristic);
        if (response->result != 0) {
          log_error("gecko_cmd_gatt_read_characteristic_value failed - 0x%X",
                    response->result);
          if (response->result == bg_err_invalid_conn_handle) {
            log_debug("Invalid connection handle. Revert to STATE_INIT");
            handle_state_transition(STATE_INIT);
          } else {
            log_fatal("Unhandled exception");
            flash_led();
          }
        }
        break;
      }
    }

    if (sensor_index >= NUM_THUNDERBOARD_SENSORS) {
      sensor_index = 0;
      refresh_sensor_values();
      struct gecko_msg_gatt_read_characteristic_value_rsp_t *response =
          gecko_cmd_gatt_read_characteristic_value(
              _thunderboard.connection,
              _thunderboard.all_sensors[sensor_index]->characteristic);
      if (response->result != 0) {
        log_fatal("gecko_cmd_gatt_read_characteristic_value failed - 0x%X",
                  response->result);
        handle_state_transition(STATE_INIT);
      }
    }
    break;

  case gecko_evt_le_connection_closed_id:
    handle_state_transition(STATE_INIT);
    break;

  default:
    log_warn("Unhandled Event: %X", message_id);
    break;
  }

  return;
}