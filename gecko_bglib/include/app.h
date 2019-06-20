/***********************************************************************************************//**
 * \file   app.h
 * \brief  Application header file
 ***************************************************************************************************
 * <b> (C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

#ifndef APP_H
#define APP_H

#define FALSE 0
#define TRUE  1

#define MAX_NAME_LENGTH 32

typedef struct ThunderBoardDevice {
	char name[MAX_NAME_LENGTH];
 	bd_addr address;
 	int8_t rssi;
} ThunderBoardDevice;

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Handle application events.
 *  \param[in]  evt  incoming event ID
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt);



#endif /* APP_H */
