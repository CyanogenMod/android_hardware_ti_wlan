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
#include "ccm_imi_bt_tran_off_sm.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_IM);

typedef _CcmImStatus (*_CCM_IM_BtTranOffSm_Action)(_CcmIm_BtTranOffSm_Obj *thisObj, void *eventData);

struct _tagCcmIm_BtTranOffSm_Obj {
    _CcmIm_BtTranOffSm_State            state;
    McpBool                         asyncCompletion;
    
    McpHalChipId                        chipId;
    BtHciIfObj                      *btHciIfObj;
    _CcmIm_BtTranOffSm_CompletionCb parentCb;
    McpHalOsSemaphoreHandle         ccmImMutexHandle;

    BtHciIfMngrCb                       btHciIfParentCb;
};

typedef struct _tagCcmIm_BtTranOffSm_Entry {
    _CCM_IM_BtTranOffSm_Action  action;
    _CcmIm_BtTranOffSm_State        nextState;
    _CcmIm_BtTranOffSm_Event        syncNextEvent;
} _CcmIm_BtTranOffSm_Entry;

typedef struct _tagCcmIm_BtTranOffSm_Data {
    _CcmIm_BtTranOffSm_Obj  smObjs[MCP_HAL_MAX_NUM_OF_CHIPS];

    _CcmIm_BtTranOffSm_Entry    sm[_CCM_IM_NUM_OF_BT_TRAN_OFF_STATES][_CCM_IM_NUM_OF_BT_TRAN_OFF_EVENTS];
} _CcmIm_BtTranOffSm_Data;

MCP_STATIC  _CcmIm_BtTranOffSm_Data _ccmIm_BtTranOffSm_Data;

MCP_STATIC void _CCM_IM_BtTranOffSm_StaticInitSm(void);
MCP_STATIC void _CCM_IM_BtTranOffSm_StaticInitInstances(void);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOffSm_InternalHandleEvent(
                            _CcmIm_BtTranOffSm_Obj  *thisObj, 
                            _CcmIm_BtTranOffSm_Event    event,
                            void                        *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOffSm_HandlerTurnHciOff(_CcmIm_BtTranOffSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOffSm_HandlerTranOffCompleted(_CcmIm_BtTranOffSm_Obj *thisObj, void *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOffSm_PerformCompletion(  _CcmIm_BtTranOffSm_Obj  *thisObj,
                                                                                _CcmImStatus            completionStatus);

MCP_STATIC void _CCM_IM_BtTranOffSm_BtHciIfMngrCb(BtHciIfMngrEvent *event);


_CcmImStatus _CCM_IM_BtTranOffSm_StaticInit(void)
{
    _CCM_IM_BtTranOffSm_StaticInitSm();
    _CCM_IM_BtTranOffSm_StaticInitInstances();

    return _CCM_IM_STATUS_SUCCESS;
}

_CcmImStatus _CCM_IM_BtTranOffSm_Create(    McpHalChipId                        chipId,
                                                    BtHciIfObj                      *btHciIfObj,
                                                    _CcmIm_BtTranOffSm_CompletionCb parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranOffSm_Obj          **thisObj)
{
    *thisObj = &_ccmIm_BtTranOffSm_Data.smObjs[chipId];

    (*thisObj)->chipId = chipId;
    (*thisObj)->btHciIfObj = btHciIfObj;
    (*thisObj)->parentCb = parentCb;
    (*thisObj)->ccmImMutexHandle= ccmImMutexHandle;

    return _CCM_IM_STATUS_SUCCESS;
}
                                            
_CcmImStatus _CCM_IM_BtTranOffSm_Destroy(_CcmIm_BtTranOffSm_Obj **thisObj)
{
    *thisObj = NULL;
    return _CCM_IM_STATUS_SUCCESS;
}
_CcmImStatus _CCM_IM_BtTranOffSm_HandleEvent(   _CcmIm_BtTranOffSm_Obj  *thisObj, 
                                                            _CcmIm_BtTranOffSm_Event    event,
                                                            void                        *eventData)
{
    /* 
        This function is called by the parent (BtTranSm) => we should initialize the flag (this is the beginning of the
        process
    */
    thisObj->asyncCompletion = MCP_FALSE;

    /* Call the actual SM engine */
    return _CCM_IM_BtTranOffSm_InternalHandleEvent(thisObj, event, eventData);
}

_CcmImStatus _CCM_IM_BtTranOffSm_InternalHandleEvent(
                    _CcmIm_BtTranOffSm_Obj  *thisObj, 
                    _CcmIm_BtTranOffSm_Event    event,
                    void                        *eventData)
{
    _CcmImStatus                status;
    _CcmIm_BtTranOffSm_Entry        *smEntry;
    
    MCP_FUNC_START("_CCM_IM_BtTranOffSm_InternalHandleEvent");

    /*
        Semaphore is locked here since this module receives events fom bt_hci_if. 

        A deadlock may occur in the following scenario:
        1. Parent locks the CCM-IM semaphore.
        2. Parent sends an event to this SM
        3. This SM calls BT_HCI_If, that, in turn, locks some transport semaphore (e.g., ESI stack semaphore).
        4. Parent sends another event to this SM, locking the CCM-IM semaphore, and attempts to access BT_HCI_If at the same time that:
        5. BT_HCI_If returns an event to this SM, in transport context (e.g., BTS).

        Now, 4 and 5 above will cause the parent's context to lock CCM-IM mutex, and try to lock transport semaphore at 
        the same time that transport semaphore is already locked and tries to lock CCM-IM semaphore.

        In our case, the parent sends a single event (start) to this SM => deadlock can't occur
    */
    MCP_HAL_OS_LockSemaphore(thisObj->ccmImMutexHandle, MCP_HAL_OS_TIME_INFINITE); 
    
    MCP_VERIFY_FATAL((thisObj != NULL), _CCM_IM_STATUS_INVALID_PARM, ("_CCM_IM_BtTranOffSm_InternalHandleEvent: Null this"));
    MCP_VERIFY_FATAL((event < _CCM_IM_NUM_OF_BT_TRAN_OFF_EVENTS), _CCM_IM_STATUS_INVALID_PARM,
                        ("_CCM_IM_BtTranOffSm_InternalHandleEvent: Invalid event (%d)", event));

    MCP_LOG_INFO(("_CCM_IM_BtTranOffSm_InternalHandleEvent: Handling SM{%s, %s}", 
                    _CCM_IM_BtTranOffSm_DebugStatetStr(thisObj->state),
                    _CCM_IM_BtTranOffSm_DebugEventStr(event)));
    
    /* Extract the SM entry matching the current {state, event} */
    smEntry = &_ccmIm_BtTranOffSm_Data.sm[thisObj->state][event];
    
    MCP_VERIFY_FATAL((smEntry->action != NULL), _CCM_IM_STATUS_NULL_SM_ENTRY, 
                        ("_CCM_IM_BtTranOffSm_InternalHandleEvent: No matching SM Entry"));

    /* Transit to the next SM state */
    thisObj->state = smEntry->nextState;

    /* The entry is valid, invoke the action */
    status = (smEntry->action)(thisObj, eventData);

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranOffSm_InternalHandleEvent: Action Completed Synchronously"));
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
        /* 
            Action completed synchronously => we transit to the next event by calling the SM engine recursively,
            using the event stored in the SM info.
        */
        
        MCP_VERIFY_FATAL((smEntry->syncNextEvent != _CCM_IM_INVALID_BT_TRAN_OFF_EVENT), 
                            _CCM_IM_STATUS_INTERNAL_ERROR, 
                            ("_CCM_IM_BtTranSm_InternalHandleEvent: Missing syncNextEvent"));

        if (smEntry->syncNextEvent != _CCM_IM_BT_TRAN_OFF_SM_EVENT_NULL_EVENT)
        {
            status = _CCM_IM_BtTranOffSm_InternalHandleEvent(thisObj, smEntry->syncNextEvent, NULL);
        }
    }

    MCP_LOG_INFO(("_CCM_IM_BtTranOffSm_InternalHandleEvent: On Exit, State: %s", _CCM_IM_BtTranOffSm_DebugStatetStr(thisObj->state)));
    
    MCP_FUNC_END();

    /* Unlocking AFTER MCP_FUNC_END() to make sure all paths (including erroneous) will unlock before exiting */
    MCP_HAL_OS_UnlockSemaphore(thisObj->ccmImMutexHandle);
    
    return status;
}

void _CCM_IM_BtTranOffSm_StaticInitSm(void)
{
    McpUint btTranOffStateIdx;
    McpUint btTranOffEventIdx;

    for (btTranOffStateIdx = 0; btTranOffStateIdx < _CCM_IM_NUM_OF_BT_TRAN_OFF_STATES; ++btTranOffStateIdx)
    {   
        for (btTranOffEventIdx = 0; btTranOffEventIdx < _CCM_IM_NUM_OF_BT_TRAN_OFF_EVENTS; ++btTranOffEventIdx)
        {
            _ccmIm_BtTranOffSm_Data.sm[btTranOffStateIdx][btTranOffEventIdx].action = NULL;
            _ccmIm_BtTranOffSm_Data.sm[btTranOffStateIdx][btTranOffEventIdx].nextState = _CCM_IM_INVALID_BT_TRAN_OFF_STATE;
            _ccmIm_BtTranOffSm_Data.sm[btTranOffStateIdx][btTranOffEventIdx].syncNextEvent = _CCM_IM_INVALID_BT_TRAN_OFF_EVENT;
        }
    }

    /*
        {State None, Event Start} -> Turn HCI Off
    */
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_NONE]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_START].action = 
                                                                                _CCM_IM_BtTranOffSm_HandlerTurnHciOff;
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_NONE]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_START].nextState = 
                                                                                _CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF;
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_NONE]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_START].syncNextEvent = 
                                                                                _CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE;

    /*
        {State Executing Script, Event HCI Config Completed} -> None (Completed)
    */
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOffSm_HandlerTranOffCompleted;
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_OFF_SM_STATE_NONE;
    _ccmIm_BtTranOffSm_Data.sm[_CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF]
                                    [_CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE].syncNextEvent = 
                                                                            _CCM_IM_BT_TRAN_OFF_SM_EVENT_NULL_EVENT;
}

MCP_STATIC void _CCM_IM_BtTranOffSm_StaticInitInstances(void)
{
    McpUint chipIdx;
    
    for (chipIdx = 0; chipIdx < MCP_HAL_MAX_NUM_OF_CHIPS; ++chipIdx)
    {
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].state = _CCM_IM_BT_TRAN_OFF_SM_STATE_NONE;
        
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].chipId = MCP_HAL_INVALID_CHIP_ID;
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].btHciIfObj = NULL;
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].parentCb = NULL;
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].btHciIfParentCb = NULL;
        _ccmIm_BtTranOffSm_Data.smObjs[chipIdx].asyncCompletion = MCP_FALSE;
    }
}

/*
    This function is called to start the BT transport turn off process
*/
_CcmImStatus _CCM_IM_BtTranOffSm_HandlerTurnHciOff(_CcmIm_BtTranOffSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranOffSm_HandlerTurnHciOff");

    MCP_UNUSED_PARAMETER(eventData);

    thisObj->asyncCompletion = MCP_FALSE;
    
    /*
        Replace the current BT HCI If manager callback with mine, so that I receive the events that 
        I need for my state machine.

        The current callback is saved. It will be restored when we Complete / Abort the Transport-Off process
    */
    btHciIfStatus = BT_HCI_IF_MngrChangeCb(thisObj->btHciIfObj, _CCM_IM_BtTranOffSm_BtHciIfMngrCb,  &thisObj->btHciIfParentCb);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOffSm_HandlerTurnHciOff: BT_HCI_IF_MngrChangeCb Failed"));
    
    /* Initiate the shutdown of the BT transport. */
    btHciIfStatus = BT_HCI_IF_MngrTransportOff(thisObj->btHciIfObj);

    if (btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_HandlerExecuteScript: Script Execution Completed Synchronously"));

        
        status = _CCM_IM_STATUS_SUCCESS;
    }
    else if (btHciIfStatus == BT_HCI_IF_STATUS_PENDING)
    {
        status = _CCM_IM_STATUS_PENDING;
    }
    else
    {
        MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, 
                    ("_CCM_IM_BtTranOffSm_HandlerTurnHciOff: BT_HCI_IF_MngrTransportOff Failed"));
    }

    MCP_FUNC_END();

    return status;
}

/*
    This function is called when the process terminated successfully 
*/
_CcmImStatus _CCM_IM_BtTranOffSm_HandlerTranOffCompleted(_CcmIm_BtTranOffSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranOffSm_PerformCompletion(thisObj, _CCM_IM_STATUS_SUCCESS);
}

_CcmImStatus _CCM_IM_BtTranOffSm_PerformCompletion( _CcmIm_BtTranOffSm_Obj  *thisObj,
                                                                    _CcmImStatus            completionStatus)
{
    _CcmImStatus                        status;
    BtHciIfStatus                           btHciIfStatus;
    _CcmIm_BtTranOffSm_CompletionEvent  completionEvent;
        
    MCP_FUNC_START("_CCM_IM_BtTranOffSm_PerformCompletion");

    /*
        Restore the parent's BT HCI If manager callback 
    */
    btHciIfStatus = BT_HCI_IF_MngrChangeCb(thisObj->btHciIfObj, thisObj->btHciIfParentCb, NULL);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOffSm_HandlerTurnHciOff: BT_HCI_IF_MngrChangeCb Failed"));
    
    MCP_LOG_INFO(("_CCM_IM_BtTranOffSm_PerformCompletion: BT Tran Off Completed (status: %d), Calling Parent", completionStatus));

    if (thisObj->asyncCompletion == MCP_TRUE)
    {
        /* 
            Notify parent SM (BtTranSm) of completion and its status. BtTranOffSm is not aware
            of the id of the stack that initiated Bt Tran Off. It is up to the parent SM to record
            this information.
        */
        completionEvent.chipId = thisObj->chipId;
        completionEvent.completionStatus = completionStatus;
        (thisObj->parentCb)(&completionEvent);
    }

    status =    _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called by BT_HCI_If to receive all the BT_HCI_If events

    The SM engine is called to process the event. This function maps the BT_HCI_If event to
    an SM engine event before forwarding it to the SM engine
*/
void _CCM_IM_BtTranOffSm_BtHciIfMngrCb(BtHciIfMngrEvent *event)
{
    _CcmIm_BtTranOffSm_Obj  *smObj = &_ccmIm_BtTranOffSm_Data.smObjs[MCP_HAL_CHIP_ID_0];

    MCP_FUNC_START("_CCM_IM_BtTranOffSm_BtHciIfMngrCb");

    MCP_VERIFY_FATAL_NO_RETVAR((smObj->btHciIfObj == event->reportingObj), ("Mismatching BT HCI If Objs"));
    
    switch (event->type)
    {
        case BT_HCI_IF_MNGR_EVENT_TRANPORT_OFF_COMPLETED:
            
            _CCM_IM_BtTranOffSm_InternalHandleEvent(    smObj, 
                                                _CCM_IM_BT_TRAN_OFF_SM_EVENT_HCI_IF_OFF_COMPLETE,
                                                NULL);

        break;

        default:
            MCP_FATAL_NO_RETVAR(("_CCM_IM_BtTranOffSm_BtHciIfMngrCb: Unexpected Event (%d)", event->type));
    }
    
    MCP_FUNC_END();
}

const char *_CCM_IM_BtTranOffSm_DebugStatetStr(_CcmIm_BtTranOffSm_State state)
{
    switch (state)
    {
        case _CCM_IM_BT_TRAN_OFF_SM_STATE_NONE:         return "TRAN_OFF_SM_STATE_NONE";
        case _CCM_IM_BT_TRAN_OFF_SM_STATE_TURN_HCI_OFF: return "TRAN_OFF_SM_STATE_TURN_HCI_OFF";
        default:                                                return "INVALID BT TRAN ON SM STATE";
    }
}

const char *_CCM_IM_BtTranOffSm_DebugEventStr(_CcmIm_BtTranOffSm_Event event)
{
    switch (event)
    {
        case _CCM_IM_BT_TRAN_OFF_SM_EVENT_START:        return "TRAN_OFF_SM_EVENT_START";
        default:                                                return "INVALID BT TRAN ON SM EVENT";
    }
}

