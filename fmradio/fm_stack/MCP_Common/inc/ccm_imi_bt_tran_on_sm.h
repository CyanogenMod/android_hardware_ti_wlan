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
*   FILE NAME:      ccm_imi_bt_tran_on_sm.h
*
*   BRIEF:          This file defines the internal interface of the CCM Init Manager BT Tranport On State Machine
*
*   DESCRIPTION:
*   This module is responsible for turning on the shared BT transport.
*   
*   To make it portable, it uses bt_hci_if 
*
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __CCM_IMI_BT_TRAN_ON_SM_H
#define __CCM_IMI_BT_TRAN_ON_SM_H

#include "mcp_hal_types.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

#include "ccm_imi_common.h"
#include "mcp_bts_script_processor.h"
#include "bt_hci_if.h"

typedef enum _tagCcmIm_BtTranOnSm_Event
{
    _CCM_IM_BT_TRAN_ON_SM_EVENT_START,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE,
    _CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT,

    _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT,

    _CCM_IM_NUM_OF_BT_TRAN_ON_EVENTS,
    _CCM_IM_INVALID_BT_TRAN_ON_EVENT
} _CcmIm_BtTranOnSm_Event;

typedef enum _tagCcmIm_BtTranOnState {
    _CCM_IM_BT_TRAN_ON_SM_STATE_NONE,
    _CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON,
    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON,
    _CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION,
    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION,
    _CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT,
    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT,
    _CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI,
    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI,

    _CCM_IM_NUM_OF_BT_TRAN_ON_STATES,
    _CCM_IM_INVALID_BT_TRAN_ON_STATE
} _CcmIm_BtTranOnSm_State;

typedef struct _tagCcmIm_BtTranOnSm_CompletionEvent {
    McpHalChipId        chipId;
    _CcmImStatus    completionStatus;
} _CcmIm_BtTranOnSm_CompletionEvent;


typedef void (*_CcmIm_BtTranOnSm_CompletionCb)(_CcmIm_BtTranOnSm_CompletionEvent *event);

typedef struct _tagCcmIm_BtTranOnSm_Obj _CcmIm_BtTranOnSm_Obj;

/*-------------------------------------------------------------------------------
 * _CCM_IM_BtTranOnSm_StaticInit()
 *
 * Brief:       
 *  Static initializations that are common to all CCM-IM instances
 *
 * Description:
 *  This function must be called once before creating CCM-IM instances
 *
 *  The function is called by the CCM when it is statically initialized
 *
 * Type:
 *      Synchronous
 *
 * Parameters:  void
 *
 * Returns:
 *      CCM_STATUS_SUCCESS - Operation is successful.
 *
 *      CCM_STATUS_INTERNAL_ERROR - A fatal error occurred
 */
_CcmImStatus _CCM_IM_BtTranOnSm_StaticInit(void);

/*-------------------------------------------------------------------------------
 * _CCM_IM_BtTranOnSm_Create()
 *
 * Brief:       
 *  Creates a CCM-IM BT Tran On SM instance
 *
 * Description:
 *  Creates a new instance of the CCM-IM for a chip.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId [in] - Identifies the chip that this instance is managing
 *      btHciIfObj [in] - pointer to BT HCI I/F instance
 *      parentCb [in] - Parent's callback function for event notification
 *      ccmImMutexHandle [in] - CCM IM Mutex Handle
 *      thisObj [out] - pointer to the instance data
 *
 * Returns:
 *      CCM_STATUS_SUCCESS - Operation is successful.
 *
 *      CCM_STATUS_INTERNAL_ERROR - A fatal error occurred
 */
_CcmImStatus _CCM_IM_BtTranOnSm_Create( McpHalChipId                        chipId,
                                                    BtHciIfObj                      *btHciIfObj,
                                                    _CcmIm_BtTranOnSm_CompletionCb  parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranOnSm_Obj           **thisObj);
                                            
/*-------------------------------------------------------------------------------
 * CCM_IMI_Destroy()
 *
 * Brief:       
 *  Destroys a CCM-IM BT Tran On SM instance
 *
 * Description:
 *  Destroys an existing CCM-IM BT Tran On SM instance.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      thisObj [in / out] - pointer to the instance data. Set to null on exit
 *
 * Returns:
 *      CCM_STATUS_SUCCESS - Operation is successful.
 *
 *      CCM_STATUS_INTERNAL_ERROR - A fatal error occurred
 */
_CcmImStatus _CCM_IM_BtTranOnSm_Destroy(_CcmIm_BtTranOnSm_Obj **thisObj);

/*-------------------------------------------------------------------------------
 * _CCM_IM_BtTranOnSm_HandleEvent()
 *
 * Brief:       
 *  Sends an event to the state machine engine
 *
 * Description:
 *  This function is called to send a new event to the state machine. The event is handled according
 *  to the current state and the sent event.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      smData [in] - instance pointer
 *      event [in] - Sent Event
 *      eventData [in] - Data that accompanies the event. NULL if not applicable
 *
 * Returns:
 *      CCM_STATUS_SUCCESS - Operation is successful.
 *
 *      CCM_STATUS_INTERNAL_ERROR - A fatal error occurred
 */
_CcmImStatus _CCM_IM_BtTranOnSm_HandleEvent(    _CcmIm_BtTranOnSm_Obj       *smData, 
                                                            _CcmIm_BtTranOnSm_Event     event,
                                                            void                            *eventData);

void _CCM_IM_BtTranOnSm_GetChipVersion(_CcmIm_BtTranOnSm_Obj *thisObj, 
                                       McpU16 *projectType,
                                       McpU16 *versionMajor,
                                       McpU16 *versionMinor);

const char *_CCM_IM_BtTranOnSm_DebugStatetStr(_CcmIm_BtTranOnSm_State state);
const char *_CCM_IM_BtTranOnSm_DebugEventStr(_CcmIm_BtTranOnSm_Event event);

#endif

