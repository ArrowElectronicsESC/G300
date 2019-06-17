#include "apitypes.h"

#define DEBUG
#define MAX_DEVICES 64
#define UART_PORT "COM5"
#define UART_TIMEOUT 1000
#define THUNDERBOARD_DEVICE_NAME_PREFIX "Thunder Sense #"

#ifdef DEBUG
#define dbg_printf(...) printf("(DEBUG) " __VA_ARGS__)
#else
#define dbg_printf(x)
#endif

typedef enum {
    state_init,
    state_find_devices,
    state_finding_devices,
    state_stop_finding_devices,
    state_stopping_discovery,
    state_discovery_stopped,
    state_intiate_connection,
    state_establishing_connection,
    state_connection_established,
    state_find_information,
    state_finding_information,
    state_information_found,
    state_subscribe,
    state_subscribing,
    state_subscribed,
    state_read_attribute,
    state_reading_attribute,
    state_attribute_read,
    state_idle,
    state_last
} states;

typedef enum {
    sensor_temperature,
    sensor_pressure,
    sensor_humidity,
    sensor_co2,
    sensor_voc,
    sensor_light,
    sensor_uv,
    sensor_sound,
    sensor_acceleration,
    sensor_orientation,
    sensor_rgb
} sensor_id;

typedef enum {
    sub_none,
    sub_needs,
    sub_fulfilled
} subscription_type;

typedef enum {
    type_double,
    type_double_triplet,
    type_int
} value_type;

typedef struct tb_sensor_val {
    value_type type;
    union {
        double double_val;
        double double_triplet_val[3];
        uint32 int_val;
    } value;
} tb_sensor_val;

typedef struct tb_sensor {
    sensor_id id;
    uint8 *uuid_bytes;
    uint32 uuid_length;
    uint32 handle;
    tb_sensor_val value;
    subscription_type subscribe;
} tb_sensor;

#define NUM_TB2_SENSORS 11
typedef struct tb_sense_2 {
    union {
        struct {
            tb_sensor temperature;
            tb_sensor pressure;
            tb_sensor humidity;
            tb_sensor co2;
            tb_sensor voc;
            tb_sensor light;
            tb_sensor uv;
            tb_sensor sound;
            tb_sensor acceleration;
            tb_sensor orientation;
            tb_sensor rgb;
        };
        tb_sensor all[NUM_TB2_SENSORS];
    };
} tb_sense_2;


void change_state(states new_state);
void print_bdaddr(bd_addr bdaddr);
int cmp_bdaddr(bd_addr first, bd_addr second);