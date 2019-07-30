/*******************************************************************************
 * Copyright Arrow Electronics, Inc., 2019
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/*******************************************************************************
 *  Author: Andrew Reisdorph
 *  Date:   2019/07/21
 ******************************************************************************/

#ifndef __INCLUDE_MAIN
#define __INCLUDE_MAIN

#include <stdint.h>
#include <stdbool.h>

#define USAGE "Usage: %s [-n] [-b baud rate] [-s serial port] [-l log level]\n\n"
#define HELP_MESSAGE \
  "Run G300 Bluetooth to Azure Demo\n" \
  " -b <baud rate>    Set baud rate for uart to mighty gecko (default: 115200)\n" \
  " -s <serial port>  Specify serial port to mighty gecko (default: /dev/ttyS1)\n" \
  " -l <log level>    Set logging level\n" \
  " -n                Disable log file creation\n" \
  " -h  or  --help    Print Help (this message) and exit\n"

#define LOG_FILE_PATH "/data/g300.log"

typedef struct G300Args {
  uint32_t baudrate;
  char serial_port[32];
  uint8_t log_level;
  bool disable_log_file;
} G300Args;

void serial_write(uint32_t length, uint8_t* data);
void flash_led();

#endif  // __INCLUDE_MAIN
