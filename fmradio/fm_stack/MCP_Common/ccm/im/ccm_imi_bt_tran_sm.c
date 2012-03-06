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

#include "mcp_hal_os.h"

#include "mcp_ver_defs.h"
#include "bt_hci_if.h"
#include "ccm_imi_bt_tran_sm.h"
#include "ccm_imi_bt_tran_on_sm.h"
#include "ccm_imi_bt_tran_off_sm.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_IM);

typedef _CcmImStatus (*_CCM_IM_BtTranSm_Action)(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);

struct _tagCcmIm_BtTranSm_Obj {
    _CcmIm_BtTranSm_State           state;
    McpBool                         asyncCompletion;

    McpHalChipId                        chipId;
    BtHciIfObj                      *btHciIfObj;
    _CcmIm_BtTranSm_CompletionCb        parentCb;

    _CcmIm_BtTranOnSm_Obj           *tranOnSmObj;
    _CcmIm_BtTranOffSm_Obj          *tranOffSmObj;
};

typedef struct _tagCcmIm_BtTranSm_Entry {
    _CCM_IM_BtTranSm_Action action;
    _CcmIm_BtTranSm_State   nextState;
    _CcmIm_BtTranSm_Event   syncNextEvent;
} _CcmIm_BtTranSm_Entry;

typedef struct _tagCcmIm_BtTranSm_Data {
    _CcmIm_BtTranSm_Obj smObjs[MCP_HAL_MAX_NUM_OF_CHIPS];

    _CcmIm_BtTranSm_Entry   sm[_CCM_IM_NUM_OF_BT_TRAN_SM_STATES][_CCM_IM_NUM_OF_BT_TRAN_SM_EVENTS];
} _CcmIm_BtTranSm_Data;

MCP_STATIC  _CcmIm_BtTranSm_Data    _ccmIm_BtTranSm_Data;

MCP_STATIC void _CCM_IM_BtTranSm_StaticInitSm(void);
MCP_STATIC void _CCM_IM_BtTranSm_StaticInitInstances(void);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_InternalHandleEvent(   
                                                        _CcmIm_BtTranSm_Obj         *smData, 
                                                        _CcmIm_BtTranSm_Event   event,
                                                        void                        *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerTurnTranOn(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerTranOnCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerTranOnFailed(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerTranOff(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerTranOffCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerAbortOn(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerOnAbortedStartOff(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_HandlerOnAbortedOffCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranSm_PerformCompletion( 
                                _CcmIm_BtTranSm_Obj                     *thisObj,
                                _CcmIm_BtTranSm_CompletionEventType eventType,
                                _CcmImStatus                        completionStatus);
                                                                
MCP_STATIC void _CCM_IM_BtTranSm_TranOnSmCb(_CcmIm_BtTranOnSm_CompletionEvent *event);
MCP_STATIC void _CCM_IM_BtTranSm_TranOffSmCb(_CcmIm_BtTranOffSm_CompletionEvent *event);

_CcmImStatus _CCM_IM_BtTranSm_StaticInit(void)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranSm_StaticInit");
            
    _CCM_IM_BtTranSm_StaticInitSm();
    _CCM_IM_BtTranSm_StaticInitInstances();

    btHciIfStatus = BT_HCI_IF_StaticInit();
    MCP_VERIFY_FATAL((btHciIfStatus == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_StaticInit: BT_HCI_IF_StaticInit Failed"));

    status = _CCM_IM_BtTranOnSm_StaticInit();
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_StaticInit: _CCM_IM_BtTranOnSm_StaticInit Failed"));

    status = _CCM_IM_BtTranOffSm_StaticInit();
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_StaticInit: _CCM_IM_BtTranOffSm_StaticInit Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_BtTranSm_Create(   McpHalChipId                        chipId,
                                                    _CcmIm_BtTranSm_CompletionCb    parentCb,
                                                McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranSm_Obj             **thisObj)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranSm_Create");

    *thisObj = &_ccmIm_BtTranSm_Data.smObjs[chipId];

    (*thisObj)->chipId = chipId;
    (*thisObj)->parentCb = parentCb;

    /* Create the BT HCI If obj that is used by both BtTranSmOn and BtTranSmOff state instances */
    btHciIfStatus = BT_HCI_IF_Create((*thisObj)->chipId, NULL, &((*thisObj)->btHciIfObj));
    MCP_VERIFY_FATAL((btHciIfStatus == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Create: BT_HCI_IF_Create Failed"));

    status  = _CCM_IM_BtTranOnSm_Create(    (*thisObj)->chipId,
                                            (*thisObj)->btHciIfObj,
                                            _CCM_IM_BtTranSm_TranOnSmCb,
                                            ccmImMutexHandle,
                                            &(*thisObj)->tranOnSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Create: _CCM_IM_BtTranOnSm_Create Failed"));

    status  = _CCM_IM_BtTranOffSm_Create(   (*thisObj)->chipId,
                                            (*thisObj)->btHciIfObj,
                                            _CCM_IM_BtTranSm_TranOffSmCb,
                                            ccmImMutexHandle,
                                            &(*thisObj)->tranOffSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Create: _CCM_IM_BtTranOffSm_Create Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}
                                            
_CcmImStatus _CCM_IM_BtTranSm_Destroy(_CcmIm_BtTranSm_Obj **thisObj)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranSm_Create");
    
    status  = _CCM_IM_BtTranOffSm_Destroy(&(*thisObj)->tranOffSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Destroy: _CCM_IM_BtTranOffSm_Destroy Failed"));

    status  = _CCM_IM_BtTranOnSm_Destroy(&(*thisObj)->tranOnSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Destroy: _CCM_IM_BtTranOnSm_Destroy Failed"));

    btHciIfStatus = BT_HCI_IF_Destroy(&((*thisObj)->btHciIfObj));
    MCP_VERIFY_FATAL((btHciIfStatus == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranSm_Destroy: BT_HCI_IF_Destroy Failed"));

    *thisObj = NULL;

    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_BtTranSm_HandleEvent(  _CcmIm_BtTranSm_Obj         *thisObj, 
                                                        _CcmIm_BtTranSm_Event   event,
                                                        void                        *eventData)
{
    /* 
        This function is called by the parent (BtTranMngr) => we should initialize the flag (this is the beginning of the
        process
    */
    thisObj->asyncCompletion = MCP_FALSE;

    /* Call the actual SM engine */
    return _CCM_IM_BtTranSm_InternalHandleEvent(thisObj, event, eventData);
}

_CcmImStatus _CCM_IM_BtTranSm_InternalHandleEvent(  _CcmIm_BtTranSm_Obj         *thisObj, 
                                                                _CcmIm_BtTranSm_Event   event,
                                                                void                        *eventData)
{
    _CcmImStatus                status;
    _CcmIm_BtTranSm_Entry       *smEntry;
    
    MCP_FUNC_START("_CCM_IM_BtTranSm_InternalHandleEvent");
    
    MCP_VERIFY_FATAL((thisObj != NULL), _CCM_IM_STATUS_INVALID_PARM, ("_CCM_IM_BtTranSm_InternalHandleEvent: Null this"));
    MCP_VERIFY_FATAL((event < _CCM_IM_NUM_OF_BT_TRAN_SM_EVENTS), _CCM_IM_STATUS_INVALID_PARM,
                        ("_CCM_IM_BtTranSm_InternalHandleEvent: Invalid event (%d)", event));

    MCP_LOG_INFO(("_CCM_IM_BtTranSm_InternalHandleEvent: Handling SM{%s, %s}", 
                    _CCM_IM_BtTranSm_DebugStatetStr(thisObj->state),
                    _CCM_IM_BtTranSm_DebugEventStr(event)));
    
    /* Extract the SM entry matching the current {state, event} */
    smEntry = &_ccmIm_BtTranSm_Data.sm[thisObj->state][event];
    
    MCP_VERIFY_FATAL((smEntry->action != NULL), _CCM_IM_STATUS_NULL_SM_ENTRY, 
                        ("_CCM_IM_BtTranSm_InternalHandleEvent: No matching SM Entry"));
    
    /* Transit to the next SM state */
    thisObj->state = smEntry->nextState;

    /* The entry is valid, invoke the action */
    status = (smEntry->action)(thisObj, eventData);

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranSm_InternalHandleEvent: Action Completed Synchronously"));
    }
    else if (status == _CCM_IM_STATUS_PENDING)
    {
        thisObj->asyncCompletion = MCP_TRUE;   
    }
    else
    {
        /* Action failed */
        MCP_FATAL(status, ("_CCM_IM_BtTranSm_InternalHandleEvent"));
    }
    
    if (status == _CCM_IM_STATUS_SUCCESS)
    {   
        MCP_VERIFY_FATAL((smEntry->syncNextEvent != _CCM_IM_INVALID_BT_TRAN_SM_EVENT), 
                            _CCM_IM_STATUS_INTERNAL_ERROR, 
                            ("_CCM_IM_BtTranSm_InternalHandleEvent: Missing syncNextEvent"));

        if (smEntry->syncNextEvent != _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT)
        {
            status = _CCM_IM_BtTranSm_InternalHandleEvent(thisObj, smEntry->syncNextEvent, NULL);
        }
    }

    MCP_LOG_INFO(("_CCM_IM_BtTranSm_InternalHandleEvent: On Exit, State: %s", _CCM_IM_BtTranSm_DebugStatetStr(thisObj->state)));
    
    MCP_FUNC_END();
    
    return status;
}

/*
    This function checks whether the state machine is in the process of turning the transport
    on or off (including on-abort)
*/
McpBool _CCM_IM_BtTranSm_IsInProgress(_CcmIm_BtTranSm_Obj *smData)
{
    /* 
        Check if the state is in one of the "static" states, indicating that no process 
        is in progress (on, off , on-abort)
    */
    if (    (smData->state == _CCM_IM_BT_TRAN_SM_STATE_OFF) || 
        (smData->state == _CCM_IM_BT_TRAN_SM_STATE_ON) ||
        (smData->state == _CCM_IM_BT_TRAN_SM_STATE_ON_FAILED))
    {
        return MCP_FALSE;
    }
    else
    {
        return MCP_TRUE;
    }
}

BtHciIfObj *CCM_IM_BtTranSm_GetBtHciIfObj(_CcmIm_BtTranSm_Obj *smData)
{
    return smData->btHciIfObj;
}

void _CCM_IM_BtTranSm_GetChipVersion(_CcmIm_BtTranSm_Obj *thisObj,
                                     McpU16 *projectType,
                                     McpU16 *versionMajor,
                                     McpU16 *versionMinor)
{
    _CCM_IM_BtTranOnSm_GetChipVersion(thisObj->tranOnSmObj, projectType, versionMajor, versionMinor);
}

void _CCM_IM_BtTranSm_StaticInitSm(void)
{
    McpUint btTranStateIdx;
    McpUint btTranEventIdx;

    for (btTranStateIdx = 0; btTranStateIdx < _CCM_IM_NUM_OF_BT_TRAN_SM_STATES; ++btTranStateIdx)
    {   
        for (btTranEventIdx = 0; btTranEventIdx < _CCM_IM_NUM_OF_BT_TRAN_SM_EVENTS; ++btTranEventIdx)
        {
            _ccmIm_BtTranSm_Data.sm[btTranStateIdx][btTranEventIdx].action = NULL;
            _ccmIm_BtTranSm_Data.sm[btTranStateIdx][btTranEventIdx].nextState = _CCM_IM_INVALID_BT_TRAN_SM_STATE;
            _ccmIm_BtTranSm_Data.sm[btTranStateIdx][btTranEventIdx].syncNextEvent = _CCM_IM_INVALID_BT_TRAN_SM_EVENT;
        }
    }

    /*
        {State Off, Event On} -> {On In progress, Turn Tran On}
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_OFF]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON].action =                                     
                                                                    _CCM_IM_BtTranSm_HandlerTurnTranOn;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_OFF]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON].nextState =                                  
                                                                    _CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS;

    /*
        {State On In Progress, Event On Complete} -> {On, Completed}
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE].action = 
                                                                    _CCM_IM_BtTranSm_HandlerTranOnCompleted;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE].nextState = 
                                                                    _CCM_IM_BT_TRAN_SM_STATE_ON;

    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE].syncNextEvent = 
                                                                    _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT;

    /*
        {State On In Progress, Event On Complete} -> {On, Completed}
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED].action = 
                                                                    _CCM_IM_BtTranSm_HandlerTranOnFailed;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED].nextState = 
                                                                    _CCM_IM_BT_TRAN_SM_STATE_ON_FAILED;

    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED].syncNextEvent = 
                                                                    _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT;

    /*
        {State On, Event Off} -> {Off In progress, Completed}
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF].action = 
                                                                    _CCM_IM_BtTranSm_HandlerTranOff;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF].nextState = 
                                                                    _CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF].syncNextEvent= 
                                                                    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE;

    /*
        {State Off In Progress, Event Off Completed} -> {Off, Completed}
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].action = 
                                                                    _CCM_IM_BtTranSm_HandlerTranOffCompleted;
    
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].nextState = 
                                                                    _CCM_IM_BT_TRAN_SM_STATE_OFF;

    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].syncNextEvent = 
                                                                    _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT;

    /*
        {State On In Progress, Event On Abort} -> Abort On
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT].action = 
                                                                            _CCM_IM_BtTranSm_HandlerAbortOn;
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT].nextState = 
                                                                            _CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS;

    /*
        {State On Abort In Progress, Event On Abort Complete} -> Turn Off
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE].action = 
                                                                        _CCM_IM_BtTranSm_HandlerOnAbortedStartOff;
    
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE].nextState = 
                                                                        _CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS;
    
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE].syncNextEvent= 
                                                                    _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE;

    /*
        {State On Abort In Progress, Event Off Complete} -> Complete Abort
    */
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].action = 
                                                                            _CCM_IM_BtTranSm_HandlerOnAbortedOffCompleted;
    
    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_SM_STATE_OFF;

    _ccmIm_BtTranSm_Data.sm[_CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS]
                                    [_CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE].syncNextEvent = 
                                                                            _CCM_IM_BT_TRAN_SM_EVENT_NULL_EVENT;
}

void    _CCM_IM_BtTranSm_StaticInitInstances(void)
{
    McpUint chipIdx;
    
    for (chipIdx = 0; chipIdx < MCP_HAL_MAX_NUM_OF_CHIPS; ++chipIdx)
    {
        _ccmIm_BtTranSm_Data.smObjs[chipIdx].state = _CCM_IM_BT_TRAN_SM_STATE_OFF;
        
        _ccmIm_BtTranSm_Data.smObjs[chipIdx].chipId = MCP_HAL_INVALID_CHIP_ID;
        _ccmIm_BtTranSm_Data.smObjs[chipIdx].btHciIfObj = NULL;
        _ccmIm_BtTranSm_Data.smObjs[chipIdx].parentCb = NULL;
    }
}

/*
    This function is called to start the BT transport turn on process

    The tran_on_sm will be called to handle that
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerTurnTranOn(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;

    MCP_FUNC_START("_CCM_IM_BtTranSm_HandlerTurnTranOn");
    
    MCP_UNUSED_PARAMETER(eventData);

    status  = _CCM_IM_BtTranOnSm_HandleEvent(   thisObj->tranOnSmObj,
                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_START,
                                                NULL);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_PENDING), status, ("_CCM_IM_BtTranSm_HandlerTurnTranOn"));
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when tran_on_sm completes turning the transport on
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerTranOnCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranSm_PerformCompletion(thisObj, 
                                                _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_COMPLETED,
                                                _CCM_IM_STATUS_SUCCESS);
}

/*
    This function is called when transport on failed
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerTranOnFailed(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);

    MCP_LOG_FATAL(("_CCM_IM_BtTranSm_HandlerTranOnFailed: Tran On Failed"));
    
    return _CCM_IM_BtTranSm_PerformCompletion(thisObj, 
                                                _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_COMPLETED,
                                                _CCM_IM_STATUS_FAILED);
}

/*
    This function is called to shutdown the BT transport

    The tran_off_sm will be called to handle that
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerTranOff(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;

    MCP_FUNC_START("_CCM_IM_BtTranSm_HandlerTranOff");
    
    MCP_UNUSED_PARAMETER(eventData);
    
    status  = _CCM_IM_BtTranOffSm_HandleEvent(  thisObj->tranOffSmObj,
                                                _CCM_IM_BT_TRAN_OFF_SM_EVENT_START,
                                                NULL);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS) || (status == _CCM_IM_STATUS_PENDING), status,
                        ("_CCM_IM_BtTranSm_HandlerTurnTranOff"));
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when tran_off_sm completes shutting down the transport
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerTranOffCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranSm_PerformCompletion(thisObj, 
                                                _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_OFF_COMPLETED,
                                                _CCM_IM_STATUS_SUCCESS);
}

/*
    This function is called to abort the transport on process

    The event is forwarded to tran_on_sm that should handle it according to its state
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerAbortOn(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;

    MCP_FUNC_START("_CCM_IM_BtTranSm_HandlerAbortOn");
    
    MCP_UNUSED_PARAMETER(eventData);

    status  = _CCM_IM_BtTranOnSm_HandleEvent(   thisObj->tranOnSmObj,
                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT,
                                                NULL);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_PENDING), status, ("_CCM_IM_BtTranOnSm_HandleEvent"));
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called during the On-Abort process. It is called when the transport on process was aborted in the
    BtTranSmOn instance. Now, to complete the process, we should activate the BtTranSmOff instance to turn the
    transport off. This is required in order to bring the transport back to the state it was before starting to turn it
    on (which is the off state).
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerOnAbortedStartOff(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;

    MCP_FUNC_START("_CCM_IM_BtTranSm_HandlerOnAbortedStartOff");
    
    MCP_UNUSED_PARAMETER(eventData);

    /* Call the BtTranOffSm to turn the transport off */
    status  = _CCM_IM_BtTranOffSm_HandleEvent(  thisObj->tranOffSmObj,
                                                _CCM_IM_BT_TRAN_OFF_SM_EVENT_START,
                                                NULL);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS) || (status == _CCM_IM_STATUS_PENDING), status,
                        ("_CCM_IM_BtTranSm_HandlerOnAbortedStartOff"));
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called during the On-Abort process. It is called when the transport off process completed.
    Now the transport is back in the off state.
*/
_CcmImStatus _CCM_IM_BtTranSm_HandlerOnAbortedOffCompleted(_CcmIm_BtTranSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranSm_PerformCompletion(thisObj, 
                                                _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED,
                                                _CCM_IM_STATUS_SUCCESS);
}

_CcmImStatus _CCM_IM_BtTranSm_PerformCompletion(    
                                _CcmIm_BtTranSm_Obj                     *thisObj,
                                _CcmIm_BtTranSm_CompletionEventType eventType,
                                _CcmImStatus                        completionStatus)
{
    _CcmImStatus                        status;
    _CcmIm_BtTranSm_CompletionEvent completionEvent;
        
    MCP_FUNC_START("_CCM_IM_BtTranSm_PerformCompletion");

    MCP_LOG_INFO(("_CCM_IM_BtTranSm_PerformCompletion: BT Tran Off Completed (status: %d), Calling Parent", completionStatus));

    if (thisObj->asyncCompletion == MCP_TRUE)
    {
        /* 
            Notify parent SM (BtTranSm) of completion and its status. BtTranSm is not aware
            of the id of the stack that initiated Bt Tran Off. It is up to the parent SM to record
            this information.
        */
        completionEvent.chipId = thisObj->chipId;
        completionEvent.completionStatus = completionStatus;
        completionEvent.eventType = eventType;
        
        (thisObj->parentCb)(&completionEvent);
    }
        
    status =    _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function receives all the notification from the child BtTranOnSm instance. It maps the notification
    to an SM engine event and calls the engine to process the event
*/
void _CCM_IM_BtTranSm_TranOnSmCb(_CcmIm_BtTranOnSm_CompletionEvent *event)
{
    _CcmIm_BtTranSm_Obj *btTranSmObj = &_ccmIm_BtTranSm_Data.smObjs[event->chipId];

    MCP_FUNC_START("_CCM_IM_BtTranSm_TranOnSmCb");

    MCP_UNUSED_PARAMETER(event);

    if (event->completionStatus == _CCM_IM_STATUS_SUCCESS)
    {
        _CCM_IM_BtTranSm_InternalHandleEvent(btTranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE, NULL);
    }
    else if (event->completionStatus == _CCM_IM_STATUS_TRAN_ON_ABORTED)
    {
        _CCM_IM_BtTranSm_InternalHandleEvent(btTranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE, NULL);
    }
    else if (event->completionStatus == _CCM_IM_STATUS_FAILED)
    {
        _CCM_IM_BtTranSm_InternalHandleEvent(btTranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED, NULL);
    }
    else
    {
        MCP_FATAL_NO_RETVAR(("_CCM_IM_BtTranSm_TranOnSmCb: Unexpected Completion Status (%d)", event->completionStatus));
    }

    MCP_FUNC_END();
}

/*
    This function receives all the notification from the child BtTranOffSm instance. It maps the notification
    to an SM engine event and calls the engine to process the event
*/
void _CCM_IM_BtTranSm_TranOffSmCb(_CcmIm_BtTranOffSm_CompletionEvent *event)
{
    _CcmIm_BtTranSm_Obj *btTranSmObj = &_ccmIm_BtTranSm_Data.smObjs[event->chipId];

    MCP_UNUSED_PARAMETER(event);
    
    _CCM_IM_BtTranSm_InternalHandleEvent(btTranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE, NULL);
}

const char *_CCM_IM_BtTranSm_DebugStatetStr(_CcmIm_BtTranSm_State state)
{
    switch (state)
    {
        case _CCM_IM_BT_TRAN_SM_STATE_OFF:      return "BT_TRAN_SM_STATE_OFF";
        case _CCM_IM_BT_TRAN_SM_STATE_ON:       return "BT_TRAN_SM_STATE_ON";
        case _CCM_IM_BT_TRAN_SM_STATE_OFF_IN_PROGRESS:      return "BT_TRAN_SM_STATE_OFF_IN_PROGRESS";
        case _CCM_IM_BT_TRAN_SM_STATE_ON_IN_PROGRESS:       return "BT_TRAN_SM_STATE_ON_IN_PROGRESS";
        case _CCM_IM_BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS:     return "BT_TRAN_SM_STATE_ON_ABORT_IN_PROGRESS";
        case _CCM_IM_BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS:       return "BT_TRAN_SM_STATE_ON_ABORTED_OFF_IN_PROGRESS";
        case _CCM_IM_BT_TRAN_SM_STATE_ON_FAILED:        return "BT_TRAN_SM_STATE_ON_FAILED";
        default:                                                return "INVALID BT TRAN SM STATE";
    }
}

const char *_CCM_IM_BtTranSm_DebugEventStr(_CcmIm_BtTranSm_Event event)
{
    switch (event)
    {
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF:     return "BT_TRAN_SM_EVENT_TRAN_OFF";
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT:        return "BT_TRAN_SM_EVENT_TRAN_ON_ABORT";
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON:      return "BT_TRAN_SM_EVENT_TRAN_ON";          
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE:        return "BT_TRAN_SM_EVENT_TRAN_OFF_COMPLETE";
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE:     return "BT_TRAN_SM_EVENT_TRAN_ON_COMPLETE";
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE:       return "BT_TRAN_SM_EVENT_TRAN_ON_ABORT_COMPLETE";
        case _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_FAILED:       return "BT_TRAN_SM_EVENT_TRAN_ON_FAILED";
        default:                                                return "INVALID BT TRAN SM EVENT";
    }
}

