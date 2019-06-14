#include "apitypes.h"

#define DEBUG
#define MAX_DEVICES 64
#define UART_PORT "COM5"
#define UART_TIMEOUT 1000
#define THUNDERBOARD_DEVICE_NAME_PREFIX "Thunder Sense #"

#ifdef DEBUG
#define dbg_printf(x) printf("(DEBUG) " x)
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
    state_last
} states;

typedef enum {
    type_float,
    type_int
} value_type;

typedef struct tb_sensor_val {
    value_type type;
    union {
        float float_val;
        uint32 int_val;
    } value;
} tb_sensor_val;

typedef struct tb_sensor {
    uint8 *uuid_bytes;
    uint32 uuid_length;
    uint32 handle;
    tb_sensor_val value;
} tb_sensor;

#define NUM_TB2_SENSORS 2
typedef struct tb_sense_2 {
    union {
        struct {
            tb_sensor temperature;
            tb_sensor pressure;
        };
        tb_sensor all[NUM_TB2_SENSORS];
    };
} tb_sense_2;


void change_state(states new_state);
void print_bdaddr(bd_addr bdaddr);
int cmp_bdaddr(bd_addr first, bd_addr second);