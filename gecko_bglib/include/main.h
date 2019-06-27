#ifndef __INCLUDE_MAIN
#define __INCLUDE_MAIN

#include <stdint.h>

#define USAGE "Usage: %s <serial port> <baud rate>\n\n"

#define TRUE 1
#define FALSE 0

typedef uint8_t bool;

void serial_write(uint32_t length, uint8_t* data);

#endif  // __INCLUDE_MAIN
