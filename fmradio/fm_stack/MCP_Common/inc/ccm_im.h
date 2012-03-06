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
#ifndef __CCM_IM_H
#define __CCM_IM_H

#include "mcp_hal_types.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

typedef enum
{
    CCM_IM_STATUS_SUCCESS           = CCM_STATUS_SUCCESS,
    CCM_IM_STATUS_FAILED                = CCM_STATUS_FAILED,
    CCM_IM_STATUS_PENDING           = CCM_STATUS_PENDING,
    CCM_IM_STATUS_NO_RESOURCES      = CCM_STATUS_NO_RESOURCES,
    CCM_IM_STATUS_IN_PROGRESS       = CCM_STATUS_IN_PROGRESS,
    CCM_IM_STATUS_INVALID_PARM      = CCM_STATUS_INVALID_PARM,
    CCM_IM_STATUS_INTERNAL_ERROR    = CCM_STATUS_INTERNAL_ERROR,
    CCM_IM_STATUS_IMPROPER_STATE    = CCM_STATUS_IMPROPER_STATE
} CcmImStatus;

/*
 *  CcmImStackId type
 *
 *  A logical identifier of a stack in the CCM IM
 */
typedef enum tagCcmImStackId {
    CCM_IM_STACK_ID_BT,
    CCM_IM_STACK_ID_FM,
    CCM_IM_STACK_ID_GPS,

    CCM_IM_MAX_NUM_OF_STACKS,
    CCM_IM_INVALID_STACK_ID
} CcmImStackId;

typedef enum tagCcmImStackState {
    CCM_IM_STACK_STATE_OFF,
    CCM_IM_STACK_STATE_OFF_IN_PROGRESS,
    CCM_IM_STACK_STATE_ON_IN_PROGRESS,
    CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS,
    CCM_IM_STACK_STATE_ON,
    CCM_IM_STACK_FATAL_ERROR,

    CCM_IM_NUM_OF_STACK_STATES,
    CCM_IM_INVALID_STACK_STATE
} CcmImStackState;

typedef enum
{   
    CCM_IM_EVENT_TYPE_ON_COMPLETE,
    CCM_IM_EVENT_TYPE_ON_ABORT_COMPLETE,
    CCM_IM_EVENT_TYPE_OFF_COMPLETE
} CcmImEventType;

typedef struct tagCcmEvent
{
    CcmImStackId        stackId;
    CcmImEventType  type;
    CcmImStatus     status;
} CcmImEvent;

typedef void (*CcmImEventCb)(CcmImEvent *event);

/*
    Forward Decleration
*/
typedef struct tagCcmImObj CcmImObj;

typedef void* CCM_IM_StackHandle;

CcmImStatus CCM_IM_RegisterStack(CcmImObj           *thisObj, 
                                        CcmImStackId        stackId, 
                                        CcmImEventCb        callback, 
                                        CCM_IM_StackHandle  *stackHandle);

CcmImStatus CCM_IM_DeregisterStack(CCM_IM_StackHandle *stackHandle);

CcmImStatus CCM_IM_StackOn(CCM_IM_StackHandle stackHandle);
CcmImStatus CCM_IM_StackOnAbort(CCM_IM_StackHandle stackHandle);
CcmImStatus CCM_IM_StackOff(CCM_IM_StackHandle stackHandle);

CcmImObj *CCM_IM_GetImObj(CCM_IM_StackHandle stackHandle);

CcmImStackState CCM_IM_GetStackState(CCM_IM_StackHandle     *stackHandle);

void CCM_IM_GetChipVersion(CCM_IM_StackHandle *stackHandle,
                           McpU16 *projectType,
                           McpU16 *versionMajor,
                           McpU16 *versionMinor);

#endif  /* __CCM_IM_H */

