#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "uart.h"

/***************************************************************************************************
 Local Variables
 **************************************************************************************************/



static int32_t serialHandle = -1;

/***************************************************************************************************
 Static Function Declarations
 **************************************************************************************************/

static void softReset(int8_t *port, uint32_t baudRate);
static int32_t uartOpenSerial(int8_t* device, uint32_t bps, uint32_t dataBits, uint32_t parity,
                              uint32_t stopBits, uint32_t rtsCts, uint32_t xOnXOff,
                              int32_t timeout);
static int32_t uartCloseSerial(int32_t handle);

/***************************************************************************************************
 Public Function Definitions
 **************************************************************************************************/
int32_t uartOpen(int8_t* port, uint32_t baudRate, uint32_t rtsCts, uint32_t xonXoff, int32_t timeout)
{
  return 0;
}

int32_t uartClose(void)
{
  return 0;
}

/***************************************************************************************************
 Send the BGAPI reset command to the NCP manually instead of with gecko_cmd_system_reset.
 Flush buffers and close.  This is used to as a workaround to help with resetting the device. 
 **************************************************************************************************/
void softReset(int8_t *port, uint32_t baudRate)
{

}

int32_t uartRx(uint32_t dataLength, uint8_t* data)
{

}

int32_t uartRxNonBlocking(uint32_t dataLength, uint8_t* data)
{
  return 0;
}

int32_t uartRxPeek(void)
{
  return 0;
}

int32_t uartTx(uint32_t dataLength, uint8_t* data)
{
  return 0;
}

/***************************************************************************************************
 Static Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Open a serial port.
 *  \param[in] device Serial Port number.
 *  \param[in] bps Baud Rate.
 *  \param[in] dataBits Number of databits.
 *  \param[in] parity Parity bit used.
 *  \param[in] stopBits Stop bits used.
 *  \param[in] rtsCts Hardware handshaking used.
 *  \param[in] xOnXOff Software Handshaking used.
 *  \param[in] timeout Block until a character is received or for timeout milliseconds. If
 *             timeout < 0, block until character is received, there is no timeout.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
static int32_t uartOpenSerial(int8_t* device, uint32_t bps, uint32_t dataBits, uint32_t parity,
                              uint32_t stopBits, uint32_t rtsCts, uint32_t xOnXOff, int32_t timeout)
{
  return 0;
}

/* Close a serial port. Return 0 on success, -1 on failure. */
static int32_t uartCloseSerial(int32_t handle)
{
  return 0;
}


