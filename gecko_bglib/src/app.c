
/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

#include "app.h"



extern ThunderBoardDevice _thunderboard;

/*
 * Local variables
 */

static bool system_ready = false;
static bool _found_thunderboard = FALSE;

static enum states {
  STATE_INIT,
  STATE_DISCOVERY,
  STATE_CONNECT
} state = STATE_INIT;


void changeState(int new_state);
bool handleAdvertisement(struct gecko_cmd_packet *evt);

void appHandleEvents(struct gecko_cmd_packet *event)
{
  if (event == NULL) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(event->header) != gecko_evt_system_boot_id) && !system_ready) {
    usleep(50000);
    return;
  }

  switch(state) {
    case STATE_INIT:
      switch(BGLIB_MSG_ID(event->header)) {
        case gecko_evt_system_boot_id:
          printf("Receieved Boot Message... \n");
          system_ready = true;
          changeState(STATE_DISCOVERY);
          break;

        default:
        	printf("Unhandled message: %X\n", BGLIB_MSG_ID(event->header));
          break;
      }
      break;

    case STATE_DISCOVERY:
      switch(BGLIB_MSG_ID(event->header)) {
        case gecko_evt_le_gap_scan_response_id:
          if (handleAdvertisement(event)) {
          	_found_thunderboard = TRUE;
          	printf("\ngecko_cmd_le_gap_end_procedure\n");
          	struct gecko_msg_le_gap_end_procedure_rsp_t *response = gecko_cmd_le_gap_end_procedure();
          	if (response->result == 0) {
          		changeState(STATE_CONNECT);
          	} else {
          		printf("ERROR: Gap End Procedure Failed: %d\n", response->result);
          	}
          }
          break;

        case gecko_evt_gatt_procedure_completed_id:
        	printf("\ngecko_evt_gatt_procedure_completed_id\n");
        break;

        default:
          printf("Unhandled message: %X\n", BGLIB_MSG_ID(event->header));
          break;
      }
      break;

      case STATE_CONNECT:
      	switch(BGLIB_MSG_ID(event->header)) {
      		case gecko_evt_le_connection_opened_id:
      			printf("\nConnection OPENED\n=================================\n");
      			changeState(STATE_GET_INFO);
      		break;
      	}
      break;

      case STATE_GET_INFO:

      break;

    default:
    	printf("ERROR: Unhandled state %d\n", state);
      break;
  }
}

bool handleAdvertisement(struct gecko_cmd_packet *event) {
	int length = 0;
	int offset = 0;
	int type = 0;
	uint8_t *data;
	bool found_thunderboard = FALSE;
	char *thunderboard_prefix = "Thunder Sense #";

	//memcpy(&device.addr, event->data.evt_le_gap_scan_response.address.addr, 6);
	//device.rssi = event->data.evt_le_gap_scan_response.rssi;

	printf("Got Advertisement:\n");
	printf("Address:\n  %02X:%02X:%02X:%02X:%02X:%02X\n",
		event->data.evt_le_gap_scan_response.address.addr[0], event->data.evt_le_gap_scan_response.address.addr[1], event->data.evt_le_gap_scan_response.address.addr[2],
		event->data.evt_le_gap_scan_response.address.addr[3], event->data.evt_le_gap_scan_response.address.addr[4], event->data.evt_le_gap_scan_response.address.addr[5]);

	while (offset < event->data.evt_le_gap_scan_response.data.len) {
		length = event->data.evt_le_gap_scan_response.data.data[offset];
		type = event->data.evt_le_gap_scan_response.data.data[offset + 1];
		data = event->data.evt_le_gap_scan_response.data.data;

		switch (type) {
			case 0x08: // Short Local Name
			case 0x09: // Complete Local Name
				if (length > strlen(thunderboard_prefix)) {
					if (memcmp(thunderboard_prefix, &data[offset + 2], strlen(thunderboard_prefix)) == 0) {
						found_thunderboard = TRUE;
						memcpy(_thunderboard.name, &data[offset + 2], length - 1);
					}
				}
				//printf("Device Name:\n  %s\n", device.name);
			break;

			case 0xFF:
				//printf("Manufacturer Data\n");
				//printf("  %02X %02X %02X %02X\n", data[offset + 2], data[offset + 3], data[offset + 4], data[offset + 5]);
			break;

			default:
				//printf("Unhandled Type: %02X\n", type);
			break;
		}

		if (found_thunderboard) {
			break;
		}

		offset += length + 1;
	}

	if (found_thunderboard) {
		printf("=====================================\nFound THUNDERBOARD\n\n");
		_thunderboard.rssi = event->data.evt_le_gap_scan_response.rssi;
		memcpy(_thunderboard.address.addr, event->data.evt_le_gap_scan_response.address.addr, 6);
	}

	return found_thunderboard;
}

void changeState(int new_state)
{
  switch (new_state) {
    case STATE_DISCOVERY:
      printf("Start discovery...");
      gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);
      gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_general_discoverable);
      break;

    case STATE_CONNECT:
    	printf("Start connection...");
    	gecko_cmd_le_gap_connect(_thunderboard.address, le_gap_address_type_public, le_gap_phy_1m);
    break;

    case STATE_GET_INFO:
    break;

    default:
      break;
  }

  state = new_state;
}