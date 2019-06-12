//gcc uart_posix.c main.c gecko_bglib.c ipc_local_socket.c app.c discovery.c

/**
 * This an example application that demonstrates Bluetooth connectivity
 * using BGLIB C function definitions. The example enables Bluetooth advertisements
 * and connections.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to a command with a command response indicating
 * it has processed the request and then later sending an event indicating
 * the requested operation has been completed. */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#ifdef _WIN32
#endif

#ifdef __linux__ 
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "ipc.h"
#endif

#include <signal.h>
#include <string.h>
#include <sys/types.h>

#include "infrastructure.h"
/* BG stack headers */
#include "gecko_bglib.h"
#include "bg_types.h"

/* hardware specific headers */
#include "uart.h"

/* application specific files */
#include "app.h"



/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

BGLIB_DEFINE();

/** The default baud rate to use. */
static uint32_t default_baud_rate = 115200;

/** The serial port to use for BGAPI communication. */
static char* uart_port = NULL;

/** The baud rate to use. */
static uint32_t baud_rate = 0;

/** File descriptor of uart to NCP */
static int uart_fd;

/** Daemonize the process (detach from controlling TTY) */
static bool daemonize = false;

/** Uart Hardware flow control 1:enable, 0:disable */
static uint32_t rtsCts = 0;

/** Uart Software flow control, 1:enable, 0:disable */
static uint32_t xonXoff = 1;

/** Usage string */
#define USAGE "Usage: %s <serial port> <baud rate> [daemon]\n" \
              "       Serial port is /dev/ttyACM* on WSTK board, /dev/ttyS1 for D-Link hardware\n\n"

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void getArgs(int argc, char* argv[]);
static void on_message_send(uint32_t msg_len, uint8_t* msg_data);

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

void printEventInfo(uint32_t eventHeader) {
  uint32_t messageId = BGLIB_MSG_ID(eventHeader);
  printf("\nMessage ID %X ", messageId);
  if (messageId & gecko_msg_type_evt) {
    printf("UNKNOWN EVENT\n");
  } else {
    switch(messageId) {
    case gecko_evt_system_boot_id:
      printf("SYSTEM BOOT\n");
    break;

    case gecko_evt_le_gap_scan_response_id:
      printf("LE GAP SCAN RESPONSE\n");
    break;

    default:
      printf("UNKNOWN EVENT\n");
    break;
  }
  }

}

/***********************************************************************************************//**
 *  \brief  The main program.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning Serial Port data.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
int main(int argc, char* argv[])
{
  struct gecko_cmd_packet* evt;
  char*msg;
  int sc;
  int source;

	getArgs(argc, argv);
	
	/* Daemonize the process, detach from controlling terminal */
  #ifdef __linux__
	if (daemonize == true) {
		daemon(0, 0);
	}
  #endif
	
	/* Ignore SIGPIPE (when socket disconnects during reads and writes). */
  //signal (SIGPIPE, SIG_IGN);

  /* Initialize BGLIB with our output function for sending messages. */
  BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

  /* Initialise the serial port */
  uart_fd = uartOpen((int8_t*)uart_port, baud_rate, rtsCts, xonXoff, 100);

	if (uart_fd == -1) {
		perror("serial port: ");
    exit(EXIT_FAILURE);
	}

  // Flush std output
  fflush(stdout);

 #ifdef __linux__
  ipcAddUart(uart_fd);

  sc = ipcHostOpen();

  if (sc == -1) {
    perror("IPCOpen: ");
    exit(EXIT_FAILURE);
  }
#endif
  printf("Starting up...\nResetting NCP target...\n");

  /* Reset NCP to ensure it gets into a defined state.
   * Once the chip successfully boots, gecko_evt_system_boot_id event should be received.
   */
  char p[5] = {0};
    p[1] =1;
  on_message_send(5, p);
  //gecko_cmd_system_reset(0);
  //gecko_cmd_dfu_reset(0);
  //gecko_cmd_system_get_bt_address();
  //usleep(50000);
  //gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 1);
  //gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_general_discoverable);


  // struct gecko_msg_system_get_bt_address_rsp_t* addr = gecko_cmd_system_get_bt_address();
  // printf("Address: %X:%X:%X:%X:%X:%X\n", addr->address.addr[0],
  //   addr->address.addr[1],
  //   addr->address.addr[2],
  //   addr->address.addr[3],
  //   addr->address.addr[4],
  //   addr->address.addr[5]);


  do {
    evt = gecko_peek_event();
    if (evt !=NULL) {
      printEventInfo(evt->header);
    }

    appHandleEvents(evt);
  } while (appBooted == false);

  printf ("...reset complete.\n");



  // while (1) {
  //   source = ipcWait();

  //   if (source == SRC_NCP) {
  //     evt = gecko_peek_event();
  //     appHandleEvents(evt);
  //   } else if (source == SRC_IPC) {
  //     msg = appPeekCommand();
  //     if (msg != NULL) {
  //       appHandleCommands(msg);
  //     }
  //   } else {
  //     printf("unknown source of message or event\n");
  //     exit(EXIT_FAILURE);
  //   }
  // }

  return -1;
}


/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/



/***********************************************************************************************//**
 *  \brief  Function called when a message needs to be written to the serial port.
 *  \param[in] msg_len Length of the message.
 *  \param[in] msg_data Message data, including the header.
 **************************************************************************************************/
static void on_message_send(uint32_t msg_len, uint8_t* msg_data)
{
  /** Variable for storing function return values. */
  int32_t ret;
  int msg_iter=0;

  printf("Sending message:\n");
  for(msg_iter=0; msg_iter<msg_len; msg_iter++) {
    printf("[%d] %X\n", msg_iter, msg_data[msg_iter]);
  }

  ret = uartTx(msg_len, msg_data);
  if (ret < 0) {
    printf("Failed to write to serial port %s, ret: %d, errno: %d\n", uart_port, ret, errno);
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************//**
 *  \brief  Serial Port initialisation routine.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning Serial Port data.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
static void getArgs(int argc, char* argv[])
{
  /**
   * Handle the command-line arguments.
   */
  baud_rate = default_baud_rate;
  switch (argc) {
    case 4:
      if (strcmp(argv[3], "daemon") == 0) {
      	daemonize = true;
      }
    /** Falls through on purpose. */
    case 3:
      baud_rate = atoi(argv[2]);
      uart_port = argv[1];
    /** Falls through on purpose. */
    default:
      break;
  }
  if (!uart_port || !baud_rate) {
    printf(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }
}

