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


#ifndef __CCM_IMI_BT_TRAN_MNGR_H
#define __CCM_IMI_BT_TRAN_MNGR_H

#include "mcp_hal_types.h"
#include "mcp_hal_os.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

#include "ccm_imi_common.h"

typedef enum _tagCcmIm_BtTranMngr_Event
{
    _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF,
    _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT,
    _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON,

    _CCM_IM_NUM_OF_BT_TRAN_MNGR_EVENTS,
    _CCM_IM_INVALID_BT_TRAN_MNGR_EVENT
} _CcmIm_BtTranMngr_Event;

typedef enum _tagCcmIm_BtTranMngr_CompletionEventType
{
    _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_OFF_COMPLETED,
    _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_COMPLETED,
    _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED,
} _CcmIm_BtTranMngr_CompletionEventType;

typedef struct _tagCcmIm_BtTranMngr_CompletionEvent {
    McpHalChipId                                chipId;
    CcmImStackId                                stackId;
    _CcmIm_BtTranMngr_CompletionEventType       eventType;
    _CcmImStatus                            completionStatus;
} _CcmIm_BtTranMngr_CompletionEvent;


typedef void (*_CcmIm_BtTranMngr_CompletionCb)(_CcmIm_BtTranMngr_CompletionEvent *event);

typedef struct _tagCcmIm_BtTranMngr_Obj _CcmIm_BtTranMngr_Obj;

_CcmImStatus _CCM_IM_BtTranMngr_StaticInit(void);

_CcmImStatus _CCM_IM_BtTranMngr_Create( McpHalChipId                        chipId,
                                                    _CcmIm_BtTranMngr_CompletionCb  parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranMngr_Obj           **thisObj);
                                            
_CcmImStatus _CCM_IM_BtTranMngr_Destroy(_CcmIm_BtTranMngr_Obj **thisObj);

_CcmImStatus _CCM_IM_BtTranMngr_HandleEvent(    _CcmIm_BtTranMngr_Obj   *smData,
                                                            CcmImStackId                stackId,
                                                            _CcmIm_BtTranMngr_Event event);

void _CCM_IM_BtTranMngr_GetChipVersion(_CcmIm_BtTranMngr_Obj   *thisObj,
                                       McpU16 *projectType,
                                       McpU16 *versionMajor,
                                       McpU16 *versionMinor);

BtHciIfObj *CCM_IM_BtTranMngr_GetBtHciIfObj(_CcmIm_BtTranMngr_Obj   *smData);

const char *_CCM_IM_BtTranMngr_DebugEventStr(_CcmIm_BtTranMngr_Event event);

#endif

