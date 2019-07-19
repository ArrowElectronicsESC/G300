#ifndef __INCLUDE_MAIN
#define __INCLUDE_MAIN

#include <stdint.h>

#define USAGE "Usage: %s <serial port> <baud rate>\n\n"

#define TRUE 1
#define FALSE 0

typedef enum {
    LED_RED,
    LED_GREEN,
    LED_YELLOW,
    LED_OFF
} G300LedColor;

typedef struct G300Args {
    uint32_t baudrate;
    char serial_port[32];
    uint8_t log_level;
} G300Args;

void serial_write(uint32_t length, uint8_t* data);
void set_led_color(G300LedColor color);
void flash_led();

#endif  // __INCLUDE_MAIN
