#ifndef DISCOVERY_H
#define DISCOVERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#ifdef __linux__
#include <sys/time.h>
#endif

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/**
 * The maximum amount of time since the last beacon before a device is removed
 * from the discovery list.
 */
#define DISCOVERY_EXPIRE_SECS 5
#define URL_SIZE 64

#ifdef _WIN32
struct timeval {
  int tv_sec;
};
#endif

/**
 * An entry in the list of currently discovered devices.
 */
struct DiscoveredDevice
{
  struct DiscoveredDevice *next;

  struct timeval timestamp;

  bool in_use;
  
 	uint8_t flags;
  uint8_t addr[6];
  int8_t rssi;
  char name[32];

  bool is_ibeacon;
  uint8_t ibeacon_proximity_uuid[16];
  uint16_t ibeacon_major;
  uint16_t ibeacon_minor;
  int8_t ibeacon_power;

  bool has_eddystone_uid;
  uint8_t eddystone_namespace[10];
  uint8_t eddystone_instance[6];

  bool has_eddystone_url;
  char eddystone_url[URL_SIZE];

	bool has_eddystone_eid;
	uint8_t eddystone_eid[8];

  int8_t eddystone_tx_power;
};


/**
 * Prototypes
 */
void extractAdvertising(struct gecko_cmd_packet *evt);
void updateDiscoveryList(struct DiscoveredDevice *device);
#ifdef __linux__
ssize_t getDiscoveryList(char *buf, size_t sz);
#else
int getDiscoveryList(char *buf, size_t sz);
#endif 

#ifdef __cplusplus
}
#endif
#endif

