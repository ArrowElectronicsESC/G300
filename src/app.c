/***********************************************************************************************//**
 * \file   app.c
 * \brief  Event handling and application code for Empty NCP Host application example
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef __linux__
#include <unistd.h>
#include "ipc.h"
#endif
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

#include "app.h"
#include "discovery.h"

#ifdef _WIN32
typedef int ssize_t;
#endif

/*
 * Variables
 */

bool appBooted = false;
static char line_buf[256];
static char discovery_list_buf[8092];


/**
 * Read the command into a buffer.  Called after ipcWait() indicates a client has sent a command.
 */
char *appPeekCommand(void)
{
  int sc;
  char *cmd;
#ifdef __linux__
  sc = ipcReadLine(line_buf, sizeof line_buf);
#endif
  if (sc != -1) {
    cmd = line_buf;
  } else {
    printf("empty command, closing connection\n");
#ifdef __linux__
    ipcClose();
#endif
    cmd = NULL;
  }

  return cmd;
}


/**
 * Parse the received command from the client and invoke appropriate handlers.
 */

void appHandleCommands(char *line)
{
  char *cmd;
  char *response;
  ssize_t sz;

  printf("appHandleCommands: %s\n", line);

  cmd = strtok(line, " ");
  if (cmd == NULL) {
    printf ("Tokenizing command failed, closing connection\n");
#ifdef __linux__
    ipcClose();
#endif
    return;
  }

  if (strcmp(cmd, "startscan") == 0) {
    gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 1);
    gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_general_discoverable);
    response = "ok\n";
#ifdef __linux__
    ipcWriteAll(response, strlen(response));
#endif
    

  } else if (strcmp(cmd, "stopscan") == 0) {
    gecko_cmd_le_gap_end_procedure();
    response = "ok\n";
#ifdef __linux__
    ipcWriteAll(response, strlen(response));
#endif
  } else if (strcmp(cmd, "get_discovered_devices") == 0) {
    sz = getDiscoveryList(discovery_list_buf, sizeof discovery_list_buf);
#ifdef __linux__
    ipcWriteAll(response, strlen(response));
#endif
    response = "\n";
#ifdef __linux__
    ipcWriteAll(response, strlen(response));
#endif  } else {
     printf("UNKNOWN\n");
  }
}

/**
 * Handle events received back from the NCP
 */
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id) && !appBooted) {
    
#ifdef __linux__
    usleep(50000);
#endif
    return;
  }

  switch(BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:
      printf("System booted. Starting scanning... \n");
      appBooted = true;
      break;

      case gecko_evt_le_gap_scan_response_id:
        extractAdvertising(evt);
        break;

    default:
      break;
  }
}


