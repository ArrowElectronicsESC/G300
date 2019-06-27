#ifndef __INCLUDE_APP_H
#define __INCLUDE_APP_H

#include "bg_types.h"
#include "gecko_bglib.h"
#include "main.h"

#define MAX_UUID_LENGTH 16
#define MAX_NAME_LENGTH 32
#define MAX_NUM_SERVICES 32
#define MAX_NUM_CHARACTERISTICS 64
#define VALUE_PAYLOAD_LENGTH 8
#define NUM_THUNDERBOARD_SENSORS 10

typedef void (*state_handler)(uint32_t, struct gecko_cmd_packet *, bool);

typedef enum AppState {
    STATE_INIT = 0,
    STATE_DISCOVERY,
    STATE_CONNECT,
    STATE_DISCOVER_SERVICES,
    STATE_DISCOVER_CHARACTERISTICS,
    STATE_READ_CHARACTERISTIC_VALUES,

    NUM_STATES
} AppState;

typedef struct GattServiceList {
    uint32_t length;
    uint32_t list[MAX_NUM_SERVICES];
} GattServiceList;

typedef struct UUID {
    uint32_t length;
    uint8_t bytes[MAX_UUID_LENGTH];
} UUID;

typedef union CharacteristicProperties {
    struct {
        uint8_t broadcast : 1;
        uint8_t read : 1;
        uint8_t write_no_response : 1;
        uint8_t write : 1;
        uint8_t notify : 1;
        uint8_t indicate : 1;
        uint8_t write_sign : 1;
        uint8_t extended : 1;
    };
    uint8_t all;
} CharacteristicProperties;

typedef struct Characteristic {
    UUID uuid;
    uint8_t value[VALUE_PAYLOAD_LENGTH];
    uint8_t value_length;
    uint32_t characteristic;
    CharacteristicProperties properties;
} Characteristic;

typedef struct CharacteristicList {
    uint32_t length;
    Characteristic list[MAX_NUM_CHARACTERISTICS];
} CharacteristicList;

typedef struct ThunderBoardDevice {
    bool initialized;
    char name[MAX_NAME_LENGTH];
    bd_addr address;
    int8_t rssi;
    uint8_t connection;
    GattServiceList services;
    CharacteristicList characteristics;

    union {
        struct {
            Characteristic *temperature_sensor;
            Characteristic *pressure_sensor;
            Characteristic *humidity_sensor;
            Characteristic *co2_sensor;
            Characteristic *voc_sensor;
            Characteristic *light_sensor;
            Characteristic *uv_sensor;
            Characteristic *sound_sensor;
            Characteristic *acceleration_sensor;
            Characteristic *orientation_sensor;
        };
        Characteristic *all_sensors[NUM_THUNDERBOARD_SENSORS];
    };

} ThunderBoardDevice;

void handle_event(struct gecko_cmd_packet *event);

#endif  // __INCLUDE_APP_H