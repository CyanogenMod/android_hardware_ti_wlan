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
*   FILE NAME:      fm_transport_if.h
*
*   BRIEF:          This file defines the interface of the FM HCI abstraction layer
*
*   DESCRIPTION:    
*
*
*   AUTHOR:   Zvi Schneider
*
\*******************************************************************************/
#ifndef __FM_TRANSPORT_IF_H
#define __FM_TRANSPORT_IF_H

#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"


/*-------------------------------------------------------------------------------
 * FM_TRANSPORT_IF_SendFmVacCommand()
 *
 * Brief:  
 *      Sends an Fm command
 *
 * Description:
 *      Send an FM command via the fm transport(HCI/I2C).
 *
 * Type:
 *      ASynchronous
 *
 * Parameters:
 *      hciCmdParms [in] - HCI Command parameters. NULL if command has none.
 *
 *      hciCmdParmsLen [in] - Length of hciCmdParms. 0 if hciCmdParms is NULL
 *
 *      sequencerCb [in] - the sequencer callback that will be called when the command completes.
 *
 *      pUserData[in] - void * will hold the data for the cb.
 *
 * 
 *
 * Returns:
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 *
 * Returns:
 *      TBD.
 */
BtHciIfStatus FM_TRANSPORT_IF_SendFmVacCommand( 
                                                McpU8               *hciCmdParms, 
                                                McpU8               hciCmdParmsLen,
                                                BtHciIfClientCb sequencerCb,
                                                void* pUserData);

#endif

