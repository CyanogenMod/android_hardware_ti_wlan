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

#ifndef __CCM_IMI_BT_TRAN_OFF_SM_H
#define __CCM_IMI_BT_TRAN_OFF_SM_H

#include "mcp_hal_types.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

#include "ccm_imi_common.h"
#include "mcp_bts_script_processor.h"
#include "bt_hci_if.h"

typedef enum _tagCcmIm_BtTranOffSm_Event
{
    _CCM_IM_BT_TRAN_OFF_SM_EVENT_START,
    _CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE,

    _CCM_IM_BT_TRAN_OFF_SM_EVENT_NULL_EVENT,

    _CCM_IM_NUM_OF_BT_TRAN_OFF_EVENTS,
    _CCM_IM_INVALID_BT_TRAN_OFF_EVENT
} _CcmIm_BtTranOffSm_Event;

typedef enum _tagCcmIm_BtTranOffState {
    _CCM_IM_BT_TRAN_OFF_SM_STATE_NONE,
    _CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF,

    _CCM_IM_NUM_OF_BT_TRAN_OFF_STATES,
    _CCM_IM_INVALID_BT_TRAN_OFF_STATE
} _CcmIm_BtTranOffSm_State;

typedef struct _tagCcmIm_BtTranOffSm_CompletionEvent {
    McpHalChipId        chipId;
    _CcmImStatus    completionStatus;
} _CcmIm_BtTranOffSm_CompletionEvent;


typedef void (*_CcmIm_BtTranOffSm_CompletionCb)(_CcmIm_BtTranOffSm_CompletionEvent *event);

typedef struct _tagCcmIm_BtTranOffSm_Obj _CcmIm_BtTranOffSm_Obj;

_CcmImStatus _CCM_IM_BtTranOffSm_StaticInit(void);

_CcmImStatus _CCM_IM_BtTranOffSm_Create(    McpHalChipId                        chipId,
                                                    BtHciIfObj                      *btHciIfObj,
                                                    _CcmIm_BtTranOffSm_CompletionCb parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranOffSm_Obj          **thisObj);
                                            
_CcmImStatus _CCM_IM_BtTranOffSm_Destroy(_CcmIm_BtTranOffSm_Obj **thisObj);

_CcmImStatus _CCM_IM_BtTranOffSm_HandleEvent(   _CcmIm_BtTranOffSm_Obj      *smData, 
                                                            _CcmIm_BtTranOffSm_Event        event,
                                                            void                            *eventData);

const char *_CCM_IM_BtTranOffSm_DebugStatetStr(_CcmIm_BtTranOffSm_State state);
const char *_CCM_IM_BtTranOffSm_DebugEventStr(_CcmIm_BtTranOffSm_Event event);

#endif

