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



#ifndef __CCM_IMI_BT_TRAN_SM_H
#define __CCM_IMI_BT_TRAN_SM_H

#include "mcp_hal_types.h"
#include "mcp_hal_os.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

#include "ccm_imi_common.h"

typedef enum _tagCcmIm_BtTranSm_Event
{
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF,
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT,
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON,
    
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE,
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE,
    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE,

    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED,
    
    _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT,

    _CCM_IM_NUM_OF_BT_TRAN_SM_EVENTS,
    _CCM_IM_INVALID_BT_TRAN_SM_EVENT
} _CcmIm_BtTranSm_Event;

typedef enum _tagCcmIm_BtTranSm_State {
    _CCM_IM_BT_TRAN_SM_STATE_OFF,
    _CCM_IM_BT_TRAN_SM_STATE_ON,
    _CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS,
    _CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS,
    _CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS,
    _CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS,

    _CCM_IM_BT_TRAN_SM_STATE_ON_FAILED,
    
    _CCM_IM_NUM_OF_BT_TRAN_SM_STATES,
    _CCM_IM_INVALID_BT_TRAN_SM_STATE
} _CcmIm_BtTranSm_State;

typedef enum _tagCcmIm_BtTranSm_CompletionEventType
{
    _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_OFF_COMPLETED,
    _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_COMPLETED,
    _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED,
} _CcmIm_BtTranSm_CompletionEventType;

typedef struct _tagCcmIm_BtTranSm_CompletionEvent {
    McpHalChipId                            chipId;
    _CcmIm_BtTranSm_CompletionEventType eventType;
    _CcmImStatus                        completionStatus;
} _CcmIm_BtTranSm_CompletionEvent;


typedef void (*_CcmIm_BtTranSm_CompletionCb)(_CcmIm_BtTranSm_CompletionEvent *event);

typedef struct _tagCcmIm_BtTranSm_Obj _CcmIm_BtTranSm_Obj;

_CcmImStatus _CCM_IM_BtTranSm_StaticInit(void);

_CcmImStatus _CCM_IM_BtTranSm_Create(   McpHalChipId                        chipId,
                                                _CcmIm_BtTranSm_CompletionCb        parentCb,
                                                McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                _CcmIm_BtTranSm_Obj                 **thisObj);
                                            
_CcmImStatus _CCM_IM_BtTranSm_Destroy(_CcmIm_BtTranSm_Obj **thisObj);

_CcmImStatus _CCM_IM_BtTranSm_HandleEvent(  _CcmIm_BtTranSm_Obj         *smData, 
                                                        _CcmIm_BtTranSm_Event   event,
                                                        void                        *eventData);

McpBool _CCM_IM_BtTranSm_IsInProgress(_CcmIm_BtTranSm_Obj *smData);

BtHciIfObj *CCM_IM_BtTranSm_GetBtHciIfObj(_CcmIm_BtTranSm_Obj *smData);

void _CCM_IM_BtTranSm_GetChipVersion(_CcmIm_BtTranSm_Obj *thisObj,
                                     McpU16 *projectType,
                                     McpU16 *versionMajor,
                                     McpU16 *versionMinor);

const char *_CCM_IM_BtTranSm_DebugStatetStr(_CcmIm_BtTranSm_State state);
const char *_CCM_IM_BtTranSm_DebugEventStr(_CcmIm_BtTranSm_Event event);

#endif

