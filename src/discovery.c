/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <math.h>
#include "discovery.h"

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/**
 * Constants
 */
#define MAX_DISCOVERED_DEVICES  300

#define EDDYSTONE_TYPE_UID			0x00
#define EDDYSTONE_TYPE_URL			0x10
#define EDDYSTONE_TYPE_TLM			0x20
#define EDDYSTONE_TYPE_EID			0x30
#define EDDYSTONE_TYPE_MASK			0xF0

#define MAX_URL_PREFIX          4
#define MAX_URL_EXPANSION       14

/**
 * Private types
 */
struct IBeacon{
  uint8_t compId[2];      /* Company ID field. */
  uint8_t type[2];        /* Beacon Type field. */
  uint8_t uuid[16];       /* 128-bit Universally Unique Identifier (UUID). The UUID is an identifier for the company using the beacon*/
  uint8_t major[2];       /* Beacon major number. Used to group related beacons. */
  uint8_t minor[2];       /* Beacon minor number. Used to specify individual beacons within a group.*/
  uint8_t txPower;        /* The Beacon's measured RSSI at 1 meter distance in dBm. See the iBeacon specification for measurement guidelines. */
};

struct EddystoneUid {
  int8_t txPower;         /* The Beacon's measured RSSI at 0 meter distance in dBm. */
  uint8_t namespace[10];  /* namespace MSB first. */
  uint8_t instance[6];
};

struct EddystoneUrl {
  int8_t txPower;         /* The Beacon's measured RSSI at 0 meter distance in dBm. */
  uint8_t urlPrefix;      /* URL prefix type. */
  char url[];             /* encoded URL. */
};

struct EddystoneEid {
  int8_t txPower;         /* The Beacon's measured RSSI at 0 meter distance in dBm. */
  uint8_t eid[8];         /* Encrypted Ephemeral Identifier. */
};


/**
 * Variables
 */
static struct DiscoveredDevice *discovered_device_list;
static struct DiscoveredDevice discovered_device_pool[MAX_DISCOVERED_DEVICES];
static char *urlPrefix[MAX_URL_PREFIX] = {
  "http://www.", "https://www.", "http://", "https://"
};
static char *urlExpansion[MAX_URL_EXPANSION] = {
  ".com/", ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/",
  ".com", ".org", ".edu", ".net", ".info", ".biz", ".gov"
};


/**
 * Private prototypes
 */

static int parseEddystoneUrl(char *dst, size_t dst_sz, int prefix, char *encoded_url, size_t url_length);
static struct DiscoveredDevice *AllocDiscoveredDevice(void);
static void FreeDiscoveredDevice (struct DiscoveredDevice *device);
static void updateFields(struct DiscoveredDevice *device, struct DiscoveredDevice *new_device);


/**
 * Extracts the fields from an advertising packet and updates the discovered device list.
 */
void extractAdvertising(struct gecko_cmd_packet *evt)
{
  int len = 0;
  int data_len = 0;
  int offset = 0;
  int type = 0;
  size_t sz;
  uint8_t *data;
  struct DiscoveredDevice device;
  bool eddystone_uuid_present = false;
  int num_service_uuids;

  memset(&device, 0, sizeof device);
  memcpy(&device.addr, evt->data.evt_le_gap_scan_response.address.addr, 6);
  device.rssi = evt->data.evt_le_gap_scan_response.rssi;

  while (offset < evt->data.evt_le_gap_scan_response.data.len) {
    len = evt->data.evt_le_gap_scan_response.data.data[offset];

    if (len <= 2) {
      offset += len + 1;
      continue;
    }

    type = evt->data.evt_le_gap_scan_response.data.data[offset+1];
    data = &evt->data.evt_le_gap_scan_response.data.data[offset+2];
    data_len = len - 1;

    switch (type) {
      case 0x01:  //flags
        break;

      case 0x03:	// List of 16-bit Service Class UUIDs
        num_service_uuids = data_len / 2;
        {int t;
        for (t = 0; t< num_service_uuids; t++) {
          if (data[2*t] == 0xAA && data[2*t+1] == 0xFE) {
            eddystone_uuid_present = true;
          }
        }
      }
        break;

      case 0x08:  // Short local name
      case 0x09:  // Complete local name
        sz = (sizeof device.name < data_len) ? sizeof device.name : data_len;
        memcpy(device.name, data, sz);
        device.name[sz] = '\0';
        break;

      case 0x16:	// Service Data
        if (eddystone_uuid_present == true && data_len >= 3 && data[0] == 0xAA && data[1] == 0xFE) {
          if ((data[2] & EDDYSTONE_TYPE_MASK) == EDDYSTONE_TYPE_UID
                      && (data_len - 3) >= sizeof (struct EddystoneUid)) {

            struct EddystoneUid *uid = (struct EddystoneUid *)&data[3];
             device.has_eddystone_uid = true;
            device.eddystone_tx_power = uid->txPower;
            memcpy(device.eddystone_namespace, &uid->namespace, sizeof device.eddystone_namespace);
            memcpy(device.eddystone_instance, &uid->instance, sizeof device.eddystone_instance);

          } else if ((data[2] & EDDYSTONE_TYPE_MASK) == EDDYSTONE_TYPE_URL
                      && (data_len - 3) >= sizeof (struct EddystoneUrl)) {

             struct EddystoneUrl *url = (struct EddystoneUrl *)&data[3];
            device.has_eddystone_url = true;
            device.eddystone_tx_power = url->txPower;
            parseEddystoneUrl(device.eddystone_url, sizeof (device.eddystone_url),
                              url->urlPrefix, url->url,
                              data_len - 3 - offsetof(struct EddystoneUrl, url));

          } else if ((data[2] & EDDYSTONE_TYPE_MASK) == EDDYSTONE_TYPE_EID
                      && (data_len - 3) >= sizeof (struct EddystoneEid)) {

            struct EddystoneEid *eid = (struct EddystoneEid *)&data[3];
              device.has_eddystone_eid = true;
            device.eddystone_tx_power = eid->txPower;
            memcpy (device.eddystone_eid, eid->eid, sizeof (device.eddystone_eid));
          }
        }
        break;

      case 0xFF:  // Manufacturer-Specific Data
        if (data_len >= sizeof (struct IBeacon))
        {
          struct IBeacon *ibeacon = (struct IBeacon *)data;

          if (ibeacon->compId[0] == 0x4C && ibeacon->compId[1] == 0x00
              && ibeacon->type[0] == 0x02 && ibeacon->type[1] == 0x15) {
            device.is_ibeacon = true;
            memcpy(&device.ibeacon_proximity_uuid, &ibeacon->uuid, sizeof ibeacon->uuid);
            device.ibeacon_major = ibeacon->major[1] | (ibeacon->major[0]<<8);
            device.ibeacon_minor = ibeacon->minor[1] | (ibeacon->minor[0]<<8);
            device.ibeacon_power = ibeacon->txPower;
         }
        }
        break;

      default:
        break;
    }

    offset += len + 1;
  }

  updateDiscoveryList(&device);
}


/**
 * Converts the URL prefix and encoded URL into a standard URL and stores it in the device struct.
 */
static int parseEddystoneUrl(char *dst, size_t dst_sz, int prefix, char *encoded_url, size_t url_length)
{
  int ch;
  size_t offset;
  size_t len;
  int t;

  if (prefix >= MAX_URL_PREFIX || strlen(urlPrefix[prefix]) >= dst_sz) {
    dst[0] = '\0';
    printf ("eddystone prefix too long.\n");
    return -1;
  }

  strcpy(dst, urlPrefix[prefix]);
  offset = strlen(dst);

  for (t=0; t < url_length && offset < dst_sz; t++) {
    ch = encoded_url[t];
    if (ch > 0x20 && ch < 0x7F) {
      dst[offset] = ch;
      offset++;
    } else if (ch < MAX_URL_EXPANSION) {
      len = strlen(urlExpansion[ch]);

      if (len < dst_sz - offset) {
        strcpy(&dst[offset], urlExpansion[ch]);
        offset += len;
      }
      else {
        dst[0] = '\0';
        printf ("not enough space for eddystone url expansion\n");
        return -1;
      }
    } else {
      printf("invalid characters in eddystone encoded url\n");
      dst[0] = '\0';
      return -1;
    }
  }

  if (offset >= dst_sz) {
    dst[0] = '\0';
    printf("eddystone url too long.\n");
    return -1;
  }

  dst[offset] = '\0';
  return 0;
}


/**
 * Updates the discovery list with the new state.  If there is an existing device with the address
 * only the appropriate fields are updated.
 */
void updateDiscoveryList(struct DiscoveredDevice *new_device)
{
  struct timeval current_time;
  struct DiscoveredDevice *device;
  struct DiscoveredDevice *prev;
  struct DiscoveredDevice *next;
  struct DiscoveredDevice *tail;

  // Update fields if device exists or insert new device

  device = discovered_device_list;

  while (device != NULL) {
    if (!memcmp(device->addr, new_device->addr, sizeof device->addr)) {
      updateFields (device, new_device);
      break;
    }

    device = device->next;
  }

  if (device == NULL) {
    if ((device = AllocDiscoveredDevice()) != NULL) {
      updateFields (device, new_device);

      // Append device to bottom of list
      tail = discovered_device_list;
      if (tail == NULL) {
        discovered_device_list = device;
        device->next = NULL;
      } else {
        while (tail->next != NULL) {
          tail = tail->next;
        }

        tail->next = device;
        device->next = NULL;
      }
    }
    else {
      return;
    }
  }


  // Expire devices not present for a few seconds
#ifdef __linux__
  gettimeofday (&current_time, NULL);
  #endif
  device = discovered_device_list;
  prev = NULL;
  next = NULL;

  while (device != NULL) {
    next = device->next;

    if (current_time.tv_sec - device->timestamp.tv_sec > DISCOVERY_EXPIRE_SECS) {
      if (prev == NULL) {
        discovered_device_list = device->next;
      } else {
        prev->next = device->next;
      }
      FreeDiscoveredDevice(device);
    } else {
      prev = device;
    }

    device = next;
  }
}

/**
 * Allocate a DiscoveredDevice
 */
static struct DiscoveredDevice *AllocDiscoveredDevice(void)
{
  struct DiscoveredDevice *device = NULL;
  int t;

  for (t=0; t < MAX_DISCOVERED_DEVICES; t++) {
    if (discovered_device_pool[t].in_use == false) {
      device = &discovered_device_pool[t];
      memset(device, 0, sizeof *device);
      device->in_use = true;
      break;
    }
  }

  return device;
}


/**
 * Free a DiscoveredDevice
 */
static void FreeDiscoveredDevice(struct DiscoveredDevice *device)
{
  device->in_use = false;
}

typedef int ssize_t;
/**
 * Retrieves the list of discovered devices and stores them in the supplied output buffer.
 * This is a newline delimited string of text containing one device per row.
 */
ssize_t getDiscoveryList(char *output, size_t sz)
{
  struct DiscoveredDevice *device = discovered_device_list;
  char tmp[64];
  int t;

  strncpy(output, "", sz);

  while (device != NULL) {
    strncat(output, "addr:", sz);
    for (t = sizeof device->addr - 1; t >= 0; t--) {
      sprintf(tmp, "%02X", device->addr[t]);
      strncat(output, tmp, sz);

      if (t != 0) {
        strncat(output, ":", sz);
      }
    }

    sprintf(tmp, ", rssi:%d", device->rssi);
    strncat(output, tmp, sz);

    if (device->name[0] != '\0') {
      sprintf(tmp, ", name:%s", device->name);
      strncat(output, tmp, sz);
    }

    if (device->is_ibeacon) {
      sprintf(tmp, ", ibeacon: ib-pwr:%d, ib-id:", device->ibeacon_power);
      strncat(output, tmp, sz);

      for (t=0; t < sizeof device->ibeacon_proximity_uuid; t++) {
        sprintf(tmp, "%02X", device->ibeacon_proximity_uuid[t]);
        strncat(output, tmp, sz);
      }
      sprintf(tmp, ", ib-maj:%d, ib-min:%d", device->ibeacon_major, device->ibeacon_minor);
      strncat(output, tmp, sz);
    }

    if (device->has_eddystone_url || device->has_eddystone_uid || device->has_eddystone_eid) {
      sprintf(tmp, ", eddystone: es-pwr:%d", device->eddystone_tx_power);
      strncat(output, tmp, sz);
    }

    if (device->has_eddystone_url) {
      sprintf(tmp, ", es-url:%s", device->eddystone_url);
      strncat(output, tmp, sz);
    }

    if (device->has_eddystone_uid) {
      strncat(output, ", es-uid-ns:", sz);
      for (t=0; t < sizeof device->eddystone_namespace; t++) {
        sprintf(tmp, "%02X", device->eddystone_namespace[t]);
        strncat(output, tmp, sz);
      }

      strncat(output, ", es-uid-ins:", sz);
      for (t=0; t < sizeof device->eddystone_instance; t++) {
        sprintf(tmp, "%02X", device->eddystone_instance[t]);
        strncat(output, tmp, sz);
      }
    }

    if (device->has_eddystone_eid) {
      strncat(output, ", uid-eid:", sz);
      for (t=0; t < sizeof device->eddystone_instance; t++) {
        sprintf(tmp, "%02X", device->eddystone_eid[t]);
        strncat(output, tmp, sz);
      }
    }

    sprintf(tmp, ",\n");
    strncat(output, tmp, sz);
    device = device->next;
  }

  return strlen(output);
}

/**
 * Updates the fields of a discovered device with new values.
 */
static void updateFields(struct DiscoveredDevice *device, struct DiscoveredDevice *new_device)
{
  memcpy(device->addr, new_device->addr, sizeof device->addr);

  if (device->name[0] == '\0' && new_device->name[0] != '\0') {
    strncpy(device->name, new_device->name, sizeof device->name);
  }

  device->rssi = new_device->rssi;

  if (new_device->is_ibeacon == true) {
    device->is_ibeacon = true;
    memcpy(device->ibeacon_proximity_uuid, new_device->ibeacon_proximity_uuid, sizeof device->ibeacon_proximity_uuid);
    device->ibeacon_major = new_device->ibeacon_major;
    device->ibeacon_minor = new_device->ibeacon_minor;
    device->ibeacon_power = new_device->ibeacon_power;
  }

  if (new_device->has_eddystone_uid == true) {
    device->has_eddystone_uid = true;
    memcpy(device->eddystone_namespace, new_device->eddystone_namespace, sizeof device->eddystone_namespace);
    memcpy(device->eddystone_instance, new_device->eddystone_instance, sizeof device->eddystone_instance);
    device->eddystone_tx_power = new_device->eddystone_tx_power;
  }

  if (new_device->has_eddystone_url == true) {
    device->has_eddystone_url = true;
    memcpy(device->eddystone_url, new_device->eddystone_url, sizeof device->eddystone_url);
    device->eddystone_tx_power = new_device->eddystone_tx_power;
  }
#ifdef __linux__
  gettimeofday(&device->timestamp, NULL);
  #endif
  
}


