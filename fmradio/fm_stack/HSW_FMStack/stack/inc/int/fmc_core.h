/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and  
 * limitations under the License.
 */
/*******************************************************************************\
*
*   FILE NAME:      fmc_core.h
*
*   BRIEF:          
*		This file defines the interface to the FM transport layer
*
*   DESCRIPTION: 
*		This file defines funtions that are used by the FM stack to communicate with 
*		the FM firmware.
*
*		There are 2 possible communications means :
*		1. I2C
*		2. HCI
*
*		The transport layer abstracts the specific method used and also the details of the communications.
*
*		The communication with the chip is via an interface that the FM firmware team defines. 
*
*		In addition, this layer is also interacting with the ChipMngr to notify it when FM stack 
*		requires access to the chip.
*		
*					through the relevant transport - UART or I2C.
*                   
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __FMC_CORE_H
#define __FMC_CORE_H

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_defs.h"
#include "fmc_fw_defs.h"
#include "ccm.h"

/********************************************************************************
 *
 * Constants
 *
 *******************************************************************************/
#define FMC_CORE_MAX_PARMS_LEN			255

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
 
/*-------------------------------------------------------------------------------
 * TiTransportFmNotificationType enum
 *
 *     Defines FM notifications from the chip manager.
 */
typedef enum
{	
	FMC_CORE_EVENT_TRANSPORT_ON_COMPLETE,
	FMC_CORE_EVENT_TRANSPORT_OFF_COMPLETE,
	FMC_CORE_EVENT_POWER_MODE_COMMAND_COMPLETE,
	FMC_CORE_EVENT_WRITE_COMPLETE,
	FMC_CORE_EVENT_READ_COMPLETE,
	FMC_CORE_EVENT_SCRIPT_CMD_COMPLETE,
	FMC_CORE_EVENT_FM_INTERRUPT,
	FMC_VAC_EVENT_OPERATION_STARTED,
	FMC_VAC_EVENT_OPERATION_STOPPED,
	FMC_VAC_EVENT_RESOURCE_CHANGED,
	FMC_VAC_EVENT_CONFIGURATION_CHANGED
} FmcCoreEventType;

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/
typedef struct {
	FmcCoreEventType	      type;
	FmcStatus				status;
	FMC_U8					*data;
	FMC_UINT				dataLen;
	FMC_BOOL				fmInter;
} FmcCoreEvent;

typedef void (*FmcCoreEventCb)(const FmcCoreEvent *eventParms);


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_CORE_Init()
 *
 * Brief:  
 *		Returns the first element in a linked list
 *
 * Description:
 *		This function must be called once for every node before inserting it into a list
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - The list head.
 *
 * Returns:
 *		A pointer to the first node on the list (or NULL if the list is empty)	
 */
FmcStatus FMC_CORE_Init(void);

FmcStatus FMC_CORE_Deinit(void);

/*
	Sets the client callback of the transport layer.

	There must be at most a single callback registered at a time (either RX or TX).
*/
FmcStatus FMC_CORE_SetCallback(FmcCoreEventCb eventCb);

/*
	This function should be called when FX TX or RX is enabled.
	It notifies ChipMngr that FM is ON.

	If FMC_STATUS_PENDING is returned, the client will be notified about operation
	completion via its callback with an FMC_CORE_EVENT_TRANSPORT_ON_COMPLETE
	event
*/
FmcStatus FMC_CORE_TransportOn(void);

/*
	This function should be called when FX TX or RX is disabled
	It notifies ChipMngr that FM is OFF

	If FMC_STATUS_PENDING is returned, the client will be notified about operation
	completion via its callback with an FMC_CORE_EVENT_TRANSPORT_OFF_COMPLETE
	event
*/
FmcStatus FMC_CORE_TransportOff(void);

/*
	Sends FM module power on / off command to the chip

	The client will be notified about operation completion via its callback with an 
	FMC_CORE_EVENT_POWER_MODE_COMMAND_COMPLETE event
*/
FmcStatus FMC_CORE_SendPowerModeCommand(FMC_BOOL powerOn);

/*
	Sends an FM Write command to the chip.

	FM Write commands always carry a 2 bytes value (cmdParms)

	The client will be notified about operation completion via its callback with an 
	FMC_CORE_EVENT_WRITE_COMPLETE event
*/
FmcStatus FMC_CORE_SendWriteCommand(FmcFwOpcode fmOpcode, FMC_U16 cmdParms);

/*
	Sends an RDS Data Set command to the chip.

	RDS Data set commands carry the RDS data to set. The data len may be up 
	to FMC_CORE_MAX_PARMS_LEN bytes long.

	The client will be notified about operation completion via its callback with an 
	FMC_CORE_EVENT_WRITE_COMPLETE event
*/
FmcStatus FMC_CORE_SendWriteRdsDataCommand(FmcFwOpcode fmOpcode, FMC_U8 *data, FMC_UINT len);

/*
	Sends an FM Read command to the chip.

	If the read is not for RDS data (fmParameter = 2 bytes) is returned to the caller via the client callback
	In case of RDS read Data fmParameter can be bigger.
	The client will be notified about operation completion via its callback with an 
	FMC_CORE_EVENT_READ_COMPLETE event
*/
FmcStatus FMC_CORE_SendReadCommand(FmcFwOpcode fmOpcode,FMC_U16 fmParameter);

/*
	Sends an HCI script command to the chip.

	Command parameters len may be up to FMC_CORE_MAX_PARMS_LEN bytes long.
	
	The client will be notified about operation completion via its callback with an 
	FMC_CORE_EVENT_SCRIPT_CMD_COMPLETE event
*/
FmcStatus FMC_CORE_SendHciScriptCommand(	FMC_U16 	hciOpcode, 
																FMC_U8 		*hciCmdParms, 
																FMC_UINT 	len);
/*
	Registers/Unregister for FM Interrupts
*/
FmcStatus FMC_CORE_RegisterUnregisterIntCallback(FMC_BOOL reg);

CcmObj* _FMC_CORE_GetCcmObjStackHandle(void);

FmcStatus fm_open_cmd_socket(int hci_dev);

FmcStatus fm_close_cmd_socket(void);
#endif /* ifndef __FMC_CORE_H */

