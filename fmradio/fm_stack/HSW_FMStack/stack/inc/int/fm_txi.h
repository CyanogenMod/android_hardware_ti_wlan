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
*   FILE NAME:      fm_txi.h
*
*   BRIEF:          This file defines the API of the FM TX stack.
*
*   DESCRIPTION:    General
*
*	TBD: 
*	1. Add some overview of the API
*	2. List of funtions, divided into groups, with their short descriptions.
*	3. Add API sequences for Common use-cases:
*		1.1. Enable + Tune + Power On
*		1.2. RDS Transmission
*		1.3 TBD - Others
*                   
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/

#ifndef __FM_TXI_H
#define __FM_TXI_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fm_tx.h"
#include "fmc_commoni.h"


/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
typedef enum 
{
	FM_TX_AUDIO_SOURCE_MASK_I2SH			= 1 << FM_TX_AUDIO_SOURCE_I2SH,
	FM_TX_AUDIO_SOURCE_MASK_PCMH			= 1 << FM_TX_AUDIO_SOURCE_PCMH,
	FM_TX_AUDIO_SOURCE_MASK_FM_ANALOG 	= 1 << FM_TX_AUDIO_SOURCE_FM_ANALOG,
} FmTxAudioSourceMask;

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FM_TXI_ExecuteScript()
 *
 * Brief:  
 *		Execute a script file
 *
 * Description:
 *		This function allows an application to send a script file for execution. The file
 *		must be a 'bts' file (a TI-Internal format), and contain valid FM TX commands.
 *
 *		This function may be useful for controlling FM TX parameters that are not exposed
 *		via the FM TX API, nor via fm_config.h; Care must be taken with the script commands
 *		since any valid command may be sent in the script, thus potentially causing the 
 *		FM transmitter to malfunction.
 *
 * Caveats:
 *		If, for some reason, the script exeuction fails, FM behavior (RX & TX) becomes undefined. 
 *		The caller is advised to restart FM operation (both RX & TX). In that case, and if the FM stack
 *		was able to detect the failure (the FM stack may not detect all failures), the caller will receive
 *		an event with an appropriate error code (FM_TX_STATUS_SCRIPT_EXECUTION_FAILED). It is The
 *		possible that the script execution may have failed and the caller would still receive an 
 *		FM_TX_STATUS_SUCCESS code in the event.
 *
 * Generated Events:
 *		1. Event type==FM_TX_EVENT_CMD_DONE, with command type == FM_TX_CMD_EXECUTE_SCRIPT
 *
 * Type:
 *		Synchronous/Asynchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		fullFileName [in] - the file name (full path) with the script commands.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_IN_PROGRESS - This function is already in progress 
 */
FmTxStatus FM_TXI_ExecuteScript(FmTxContext*fmContext, const char* fullFileName);

#endif /* ifndef __FM_TX_H  */

