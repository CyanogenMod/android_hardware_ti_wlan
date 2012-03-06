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
#include "ccm_hal_pwr_up_dwn.h"
#include "bt_hci_if.h"
#include "ccm_imi_bt_tran_on_sm.h"
#include "mcp_bts_script_processor.h"
#include "mcp_hal_string.h"
#include "mcp_rom_scripts_db.h"
#include "mcp_endian.h"
#include "mcp_unicode.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_IM);

/* Read Local Version Event Parms applicable Fields */
MCP_STATIC const McpUint _CcmIm_BtTranOnSm_HciLocalVersionManufacturerNamePos = 8;
MCP_STATIC const McpUint _CcmIm_BtTranOnSm_HciLocalVersionLmpSubversionPos = 10;
MCP_STATIC const McpU16 _CcmIm_BtTranOnSm_TiManufacturerId = 13;

typedef _CcmImStatus (*_CCM_IM_BtTranOnSm_Action)(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);

struct _tagCcmIm_BtTranOnSm_Obj {
    _CcmIm_BtTranOnSm_State         state;
    McpBool                         asyncCompletion;
    
    McpHalChipId                        chipId;
    BtHciIfObj                      *btHciIfObj;
    _CcmIm_BtTranOnSm_CompletionCb  parentCb;
    McpHalOsSemaphoreHandle         ccmImMutexHandle;

    BtHciIfMngrCb                       btHciIfParentCb;
    McpBtsSpContext                 btsSpContext;

    /* chip version */
    McpU16                          projectType;
    McpU16                          versionMajor;
    McpU16                          versionMinor;
} ;

typedef struct _tagCcmIm_BtTranOnSm_Entry {
    _CCM_IM_BtTranOnSm_Action   action;
    _CcmIm_BtTranOnSm_State     nextState;
    _CcmIm_BtTranOnSm_Event     syncNextEvent;
} _CcmIm_BtTranOnSm_Entry;

typedef struct _tagCcmIm_BtTranOnSm_Data {
    _CcmIm_BtTranOnSm_Obj   smObjs[MCP_HAL_MAX_NUM_OF_CHIPS];

    _CcmIm_BtTranOnSm_Entry sm[_CCM_IM_NUM_OF_BT_TRAN_ON_STATES][_CCM_IM_NUM_OF_BT_TRAN_ON_EVENTS];
} _CcmIm_BtTranOnSm_Data;

MCP_STATIC  _CcmIm_BtTranOnSm_Data  _ccmIm_BtTranOnSm_Data;

MCP_STATIC void _CCM_IM_BtTranOnSm_StaticInitSm(void);
MCP_STATIC void _CCM_IM_BtTranOnSm_StaticInitInstances(void);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_InternalHandleEvent(
                                    _CcmIm_BtTranOnSm_Obj   *thisObj, 
                                    _CcmIm_BtTranOnSm_Event event,
                                    void                        *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerTurnHciOn(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerReadVersion(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortReadVersion(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerExecuteScript(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerScriptCmdCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerScriptSetTranParmsCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerConfigHci(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortTurnHciOn(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortExecuteScript(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortConfigHci(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerTranOnCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerOnFailed(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);
MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_PerformCompletion(   _CcmIm_BtTranOnSm_Obj   *thisObj,
                                                                                _CcmImStatus            completionStatus);

MCP_STATIC void _CCM_IM_BtTranOnSm_BtHciIfMngrCb(BtHciIfMngrEvent *event);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranOnSm_GetInitScriptFileName(
                                            _CcmIm_BtTranOnSm_Obj   *thisObj,
                                            const McpU8             *hciReadVersionEventParms,
                                            char                    *scriptFileName,
                                            McpUtf8                 *scriptFileFullName);

MCP_STATIC McpBtsSpStatus _CCM_IM_BtTranOnSm_SpSendHciCmdCb(
                                            McpBtsSpContext         *context,
                                            McpU16              hciOpcode, 
                                            McpU8               *hciCmdParms, 
                                            McpUint                 hciCmdParmsLen);

MCP_STATIC McpBtsSpStatus _CCM_IM_BtTranOnSm_SpSetTranParmsCmdCb(
                                            McpBtsSpContext             *context,
                                            McpUint                     speed, 
                                            McpBtSpFlowControlType  flowCtrlType);

MCP_STATIC void _CCM_IM_BtTranOnSm_SpExecCompleteCmdCb(
                                            McpBtsSpContext *context, 
                                            McpBtsSpStatus status);

_CcmImStatus _CCM_IM_BtTranOnSm_StaticInit(void)
{   
    _CCM_IM_BtTranOnSm_StaticInitSm();
    _CCM_IM_BtTranOnSm_StaticInitInstances();
    
    return _CCM_IM_STATUS_SUCCESS;
}

_CcmImStatus _CCM_IM_BtTranOnSm_Create( McpHalChipId                        chipId,
                                                    BtHciIfObj                      *btHciIfObj,
                                                    _CcmIm_BtTranOnSm_CompletionCb  parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranOnSm_Obj           **thisObj)
{
    *thisObj = &_ccmIm_BtTranOnSm_Data.smObjs[chipId];

    (*thisObj)->chipId = chipId;
    (*thisObj)->btHciIfObj = btHciIfObj;
    (*thisObj)->parentCb = parentCb;
    (*thisObj)->ccmImMutexHandle= ccmImMutexHandle;

    return _CCM_IM_STATUS_SUCCESS;
}
                                            
_CcmImStatus _CCM_IM_BtTranOnSm_Destroy(_CcmIm_BtTranOnSm_Obj **thisObj)
{
    *thisObj = NULL;
    return _CCM_IM_STATUS_SUCCESS;
}

_CcmImStatus _CCM_IM_BtTranOnSm_HandleEvent(    _CcmIm_BtTranOnSm_Obj   *thisObj, 
                                                            _CcmIm_BtTranOnSm_Event event,
                                                            void                        *eventData)
{
    /* 
        This function is called by the parent (BtTranSm) => we should initialize the flag (thisObj is the beginning of the
        process
    */
    thisObj->asyncCompletion = MCP_FALSE;

    /* Call the actual SM engine */
    return _CCM_IM_BtTranOnSm_InternalHandleEvent(thisObj, event, eventData);
}

_CcmImStatus _CCM_IM_BtTranOnSm_InternalHandleEvent(
                    _CcmIm_BtTranOnSm_Obj   *thisObj, 
                    _CcmIm_BtTranOnSm_Event event,
                    void                        *eventData)
{
    _CcmImStatus                status;
    _CcmIm_BtTranOnSm_Entry     *smEntry;
    
    MCP_FUNC_START("_CCM_IM_BtTranOnSm_InternalHandleEvent");

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

        In our case, the parent sends 2 events to this SM:
        1. Start - to start transport turn on
        2. Abort - to abort transport turn on.

        The start event starts the whole process => the scenario above is not possible 
        The on-abort handling will not access BT_HCI_If. If the SM is waiting for a BT_HCI_If event, it just waits for 
            the event to arrive and then aborts.
    */
    MCP_HAL_OS_LockSemaphore(thisObj->ccmImMutexHandle, MCP_HAL_OS_TIME_INFINITE); 
    
    MCP_VERIFY_FATAL((thisObj != NULL), _CCM_IM_STATUS_INVALID_PARM, ("_CCM_IM_BtTranOnSm_InternalHandleEvent: Null thisObj"));
    MCP_VERIFY_FATAL((event < _CCM_IM_NUM_OF_BT_TRAN_ON_EVENTS), _CCM_IM_STATUS_INVALID_PARM,
                        ("_CCM_IM_BtTranOnSm_InternalHandleEvent: Invalid event (%d)", event));

    MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_InternalHandleEvent: Handling SM{%s, %s}", 
                    _CCM_IM_BtTranOnSm_DebugStatetStr(thisObj->state),
                    _CCM_IM_BtTranOnSm_DebugEventStr(event)));
    
    /* Extract the SM entry matching the current {state, event} */
    smEntry = &_ccmIm_BtTranOnSm_Data.sm[thisObj->state][event];
    
    MCP_VERIFY_FATAL((smEntry->action != NULL), _CCM_IM_STATUS_NULL_SM_ENTRY, 
                        ("_CCM_IM_BtTranOnSm_InternalHandleEvent: No matching SM Entry"));

    /* Transit to the next SM state */
    thisObj->state = smEntry->nextState;

    /* The entry is valid, invoke the action */
    status = (smEntry->action)(thisObj, eventData);

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_InternalHandleEvent: Action Completed Synchronously"));
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
        
        MCP_VERIFY_FATAL((smEntry->syncNextEvent != _CCM_IM_INVALID_BT_TRAN_ON_EVENT), 
                            _CCM_IM_STATUS_INTERNAL_ERROR, 
                            ("_CCM_IM_BtTranSm_InternalHandleEvent: Missing syncNextEvent"));
        if (smEntry->syncNextEvent != _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT)
        {
            status = _CCM_IM_BtTranOnSm_InternalHandleEvent(thisObj, smEntry->syncNextEvent, NULL);
        }
    }

    MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_InternalHandleEvent: On Exit, State: %s", _CCM_IM_BtTranOnSm_DebugStatetStr(thisObj->state)));
    
    MCP_FUNC_END();

    /* Unlocking AFTER MCP_FUNC_END() to make sure all paths (including erroneous) will unlock before exiting */
    MCP_HAL_OS_UnlockSemaphore(thisObj->ccmImMutexHandle);
    
    return status;
}

void _CCM_IM_BtTranOnSm_StaticInitSm(void)
{
    McpUint btTranOnStateIdx;
    McpUint btTranOnEventIdx;

    for (btTranOnStateIdx = 0; btTranOnStateIdx < _CCM_IM_NUM_OF_BT_TRAN_ON_STATES; ++btTranOnStateIdx)
    {   
        for (btTranOnEventIdx = 0; btTranOnEventIdx < _CCM_IM_NUM_OF_BT_TRAN_ON_EVENTS; ++btTranOnEventIdx)
        {
            _ccmIm_BtTranOnSm_Data.sm[btTranOnStateIdx][btTranOnEventIdx].action = NULL;
            _ccmIm_BtTranOnSm_Data.sm[btTranOnStateIdx][btTranOnEventIdx].nextState = _CCM_IM_INVALID_BT_TRAN_ON_STATE;
            _ccmIm_BtTranOnSm_Data.sm[btTranOnStateIdx][btTranOnEventIdx].syncNextEvent = _CCM_IM_INVALID_BT_TRAN_ON_EVENT;
        }
    }

    /*
        {State None, Event Start} -> Turn HCI On
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_NONE]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_START].action = _CCM_IM_BtTranOnSm_HandlerTurnHciOn;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_NONE]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_START].nextState = _CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON;

    /*
        {State Turn HCI On, Event HCI On Completed} -> Read Version
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE].action = _CCM_IM_BtTranOnSm_HandlerReadVersion;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION;

    /*
        {State Turn HCI On, Event HCI On Failed} -> None
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED].action = 
                                                                _CCM_IM_BtTranOnSm_HandlerOnFailed;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED].syncNextEvent= 
                                                                            _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;

    /*
        {State Read Version, Event HCI Command Complete} -> Executing Script
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].action =
                                                                            _CCM_IM_BtTranOnSm_HandlerExecuteScript;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].syncNextEvent= 
                                                                            _CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE;
    /*
        {State Executing Script, Event HCI Command Completed} -> Executing Script (no change, script still executing)
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerScriptCmdCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT;

    /*
        {State Executing Script, Event HCI Command Completed} -> Executing Script (no change, script still executing)
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerScriptSetTranParmsCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT;

    /*
        {State Executing Script, Event Execute Script Completed} -> Config HCI
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerConfigHci;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI;
    /*
        {State Executing Script, Event HCI Config Completed} -> None (Completed)
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerTranOnCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].syncNextEvent = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;
    
    /*
        {State Turn HCI On, Event Abort} -> Abort Turn HCI On
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].action =
                                                                            _CCM_IM_BtTranOnSm_HandlerAbortTurnHciOn;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON;

    /*
        {State Abort Turn HCI On, Event HCI On Completed} -> None (Completed)
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE].nextState = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE].syncNextEvent = 
                                                                            _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;

    /*
        {State Read Version, Event Abort} -> Abort Read Version
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].action = 
                                                                            _CCM_IM_BtTranOnSm_HandlerAbortReadVersion;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].nextState =
                                                                            _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].syncNextEvent =
                                                                            _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE;

    /*
        {State Abort Read Version, Event HCI Cmd Complete} -> Abort Read Version
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].action = 
                                                                                _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].nextState = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].syncNextEvent = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;

    /*
        {State Executing Script, Event Abort} -> Abort Execute Script
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].action = 
                                                                    _CCM_IM_BtTranOnSm_HandlerAbortExecuteScript;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].nextState = 
                                                                    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT;
    

    /*
        {State Executing Script, Event HCI Cmd Complete} -> Abort Execute Script
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].action = 
                                                                    _CCM_IM_BtTranOnSm_HandlerScriptCmdCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE].nextState = 
                                                                    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT;

    /*
        {State Executing Script, Event HCI Cmd Complete} -> Abort Execute Script
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE].action = 
                                                                    _CCM_IM_BtTranOnSm_HandlerScriptSetTranParmsCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE].nextState = 
                                                                    _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT;
    
    /*
        {State Abort Execute Script, Event Abort Execute Script Complete} -> None (Completed) (Aborted)
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE].action = 
                                                                                _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE].nextState = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE].syncNextEvent = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;

    /*
        {State Config HCI, Event Abort} -> Abort HCI Config
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].action = 
                                                                                _CCM_IM_BtTranOnSm_HandlerAbortConfigHci;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].nextState = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI;
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT].syncNextEvent = 
                                                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE;

    /*
        {State Abort Config HCI, Event HCI Config Complete} -> Abort HCI Config
    */
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].action = 
                                                                        _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted;
    
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].nextState = 
                                                                        _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
    
    _ccmIm_BtTranOnSm_Data.sm[_CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI]
                                    [_CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE].syncNextEvent = 
                                                                        _CCM_IM_BT_TRAN_ON_SM_EVENT_NULL_EVENT;
}


void    _CCM_IM_BtTranOnSm_StaticInitInstances(void)
{
    McpUint chipIdx;
    
    for (chipIdx = 0; chipIdx < MCP_HAL_MAX_NUM_OF_CHIPS; ++chipIdx)
    {
        _ccmIm_BtTranOnSm_Data.smObjs[chipIdx].state = _CCM_IM_BT_TRAN_ON_SM_STATE_NONE;
        
        _ccmIm_BtTranOnSm_Data.smObjs[chipIdx].chipId = MCP_HAL_INVALID_CHIP_ID;
        _ccmIm_BtTranOnSm_Data.smObjs[chipIdx].btHciIfObj = NULL;
        _ccmIm_BtTranOnSm_Data.smObjs[chipIdx].parentCb = NULL;
        _ccmIm_BtTranOnSm_Data.smObjs[chipIdx].btHciIfParentCb = NULL;
    }
}

/*
    This function is called to start the BT transport turn on process
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerTurnHciOn(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerTurnHciOn");

    MCP_UNUSED_PARAMETER(eventData);

    /*
        Replace the current BT HCI If manager callback with mine, so that I receive the events that 
        I need for my state machine.

        The current callback is saved. It will be restored when we Complete / Abort the Transport-On process
    */
    btHciIfStatus = BT_HCI_IF_MngrChangeCb(thisObj->btHciIfObj, _CCM_IM_BtTranOnSm_BtHciIfMngrCb,  &thisObj->btHciIfParentCb);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrChangeCb Failed"));

    /* Initiate the basic initialization of the BT transport. */
    btHciIfStatus = BT_HCI_IF_MngrTransportOn(thisObj->btHciIfObj);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_PENDING), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrTransportOn Failed"));

    status = _CCM_IM_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when the transport completed the basic initializatio. HCI reset was already sent to the chip.

    What must be done now is start sending the correct BT init script. To that end, we must know the file's name.
    The file name is generated from information stored in the LMP Seubversion fields. These fields are retrieved via
    the Read Local Version HCI command.
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerReadVersion(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerTurnHciOn");
    
    MCP_UNUSED_PARAMETER(eventData);
    
    btHciIfStatus = BT_HCI_IF_MngrSendRadioCommand( thisObj->btHciIfObj,                       /* Bt If Obj */
                                                        BT_HCI_IF_HCI_CMD_READ_LOCAL_VERSION,   /* HCI Opcode */
                                                        NULL,                                   /* Parms (none) */
                                                        0);                                     /* Parms Len (0 - No Parms) */
    MCP_VERIFY_ERR((btHciIfStatus == BT_HCI_IF_STATUS_PENDING), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_TransportOn Failed"));

    status = _CCM_IM_STATUS_PENDING;
                                                
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when the read local version command completes

    The LMP Subversion fields will be extracted from the HCI event and init script execution will start
    using the Script Processor.
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerExecuteScript(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus                status;
    McpBtsSpStatus              btsSpStatus;
    McpBtsSpScriptLocation      scriptLocation;
    McpBtsSpExecuteScriptCbData scriptCbData;
    BtHciIfMngrEvent            *btHciIfMngrEvent;
    BtHciIfHciEventData         *hciEventData;
    char                        scriptFileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS];
    McpUtf8                     scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                                                   MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerExecuteScript");

    /* Event data holds the HCI event data */
    btHciIfMngrEvent = (BtHciIfMngrEvent*)eventData;
    hciEventData = &btHciIfMngrEvent->data.hciEventData;

    /* Get Init Script file name */
    status = _CCM_IM_BtTranOnSm_GetInitScriptFileName(thisObj, hciEventData->parms, scriptFileName, scriptFullFileName);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), status,
                        ("_CCM_IM_BtTranOnSm_HandlerExecuteScript: _CCM_IM_BtTranOnSm_GetInitScriptFullFileName Failed"));

    /* Prepare data needed from the Script Processor */
    
    scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    scriptLocation.locationData.fullFileName = scriptFullFileName;

    scriptCbData.sendHciCmdCb = _CCM_IM_BtTranOnSm_SpSendHciCmdCb;
    scriptCbData.setTranParmsCb = _CCM_IM_BtTranOnSm_SpSetTranParmsCmdCb;
    scriptCbData.execCompleteCb = _CCM_IM_BtTranOnSm_SpExecCompleteCmdCb;

    /* Start script execution */
    btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &thisObj->btsSpContext);

    if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_HandlerExecuteScript: Script Execution Completed Synchronously"));

        
        status = _CCM_IM_STATUS_SUCCESS;
    }
    else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
    {
        status = _CCM_IM_STATUS_PENDING;
    }

    /* Script was not found on FFS. Try to find it in memory */
    
    else if (btsSpStatus == MCP_BTS_SP_STATUS_FILE_NOT_FOUND)
    {       
        McpBool memInitScriptFound;

        MCP_LOG_INFO(("%s Not Found on FFS - Checking Memory", scriptFullFileName));

        /* Check if memory has a script for this version and load its parameters if found */
        memInitScriptFound =  MCP_RomScriptsGetMemInitScriptData(
                                    scriptFileName, 
                                    &scriptLocation.locationData.memoryData.size,
                                    (const McpU8**)&scriptLocation.locationData.memoryData.address);

        /* BT Init script can't be loaded => FATAL ERROR */
        MCP_VERIFY_FATAL((memInitScriptFound == MCP_TRUE), _CCM_IM_STATUS_INTERNAL_ERROR,
                            ("%s Not Found on Memory as well", scriptFileName));

        /* updating location, rest of fields were already set above and are correct as is */            
        scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_MEMORY;

        /* Retry script execution - this time from memory */
        btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &thisObj->btsSpContext);

        if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
        {
            MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_HandlerExecuteScript: Memory Script Execution Completed Synchronously"));

            
            status = _CCM_IM_STATUS_SUCCESS;
        }
        else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
        {
            status = _CCM_IM_STATUS_PENDING;
        }
        else
        {
            /* BT Init script can't be loaded => FATAL ERROR */
            MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOnSm_HandlerExecuteScript: Memory MCP_BTS_SP_ExecuteScript Failed"));
        }
    }
    else
    {
        /* BT FS Init script exists, but we failed loading it => FATAL ERROR */
        MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, 
                    ("_CCM_IM_BtTranOnSm_HandlerExecuteScript: MCP_BTS_SP_ExecuteScript Failed"));
    }

    MCP_FUNC_END();

    return status;
}

/*
    This function is called by the SM engine when BT_HCI_If completes sending a radio command (initiated via the call
    to BT_HCI_IF_MngrSendRadioCommand). Radio commands are sent to BT_HCI_If when the script processor
    sends an HCI command
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerScriptCmdCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus        status;
    BtHciIfMngrEvent        *btHciIfMngrEvent;
    BtHciIfHciEventData *hciEventData;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerScriptCmdCompleted");
    
    /* Event data holds the HCI event data */
    btHciIfMngrEvent = (BtHciIfMngrEvent*)eventData;
    hciEventData = &btHciIfMngrEvent->data.hciEventData;

    /* Notify the script processor that the HCI command completed to let it continue script processing */
    MCP_BTS_SP_HciCmdCompleted(&thisObj->btsSpContext, hciEventData->parms, hciEventData->parmLen);
    
    status = _CCM_IM_STATUS_PENDING;
                                                
    MCP_FUNC_END();

    return status;
}

/*
    This function is called the SM engine when BT_HCI_If completes setting the transport parameters (initiated via the call
    to BT_HCI_IF_MngrSetTranParms). Transport parameters are sent to BT_HCI_If when the script processor
    sends a set transport parameters command
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerScriptSetTranParmsCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus        status;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerScriptSetTranParmsCompleted");

    MCP_UNUSED_PARAMETER(eventData);
        
    MCP_BTS_SP_SetTranParmsCompleted(&thisObj->btsSpContext);
        
    status = _CCM_IM_STATUS_PENDING;
                                                
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when the init script successfully completed execution. What is left to do is to
    complete HCI Layer configuration (HCI Flow Control).
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerConfigHci(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;
    BtHciIfStatus       btHciIfStatus;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerConfigHci");

    MCP_UNUSED_PARAMETER(eventData);
    
    btHciIfStatus = BT_HCI_IF_MngrConfigHci(thisObj->btHciIfObj);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_PENDING), _CCM_IM_STATUS_INTERNAL_ERROR, 
                    ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrConfigHci Failed"));

    status = _CCM_IM_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when an abort event is sent to the SM from the parent while the SM is waiting for 
    BT_HCI_IF_MngrTransportOn to complete.
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortTurnHciOn(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(thisObj);
    MCP_UNUSED_PARAMETER(eventData);

    /* The process will be aborted when the HCI On Completed event will arrive, nothing to do till then */
    
    MCP_LOG_INFO(("BT Tran On Abort While Turning HCI On, Doing nothing, waiting for Turn On Complete Event"));

    return _CCM_IM_STATUS_PENDING;  
}

/*
    This function is called when an abort event is sent to the SM from the parent while the SM is waiting for 
    the HCI Read Local Version command to complete
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortReadVersion(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(thisObj);
    MCP_UNUSED_PARAMETER(eventData);

    /* 
        Do Nothing, Wait for version to be read and continue handling in the corresponding handler 
        (the state indicates that the process should be aborted)
    */
    
    MCP_LOG_INFO(("BT Tran On Abort While Reading Version, Doing nothing, waiting for Read Version Complete Event"));

    return _CCM_IM_STATUS_PENDING;
}

/*
    This function is called when an abort event is sent to the SM from the parent while the script processor
    is executing the init script. 
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortExecuteScript(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    _CcmImStatus    status;
    McpBtsSpStatus  btsSpStatus;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerAbortExecuteScript");

    MCP_UNUSED_PARAMETER(thisObj);
    MCP_UNUSED_PARAMETER(eventData);

    MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_HandlerAbortExecuteScript: Calling MCP_BTS_SP_AbortScriptExecution"));
    
    /* Requesting Script Processor to abort script execution. Waiting for script execution completion event to abort then */
    btsSpStatus = MCP_BTS_SP_AbortScriptExecution(&thisObj->btsSpContext);
    MCP_VERIFY_FATAL((btsSpStatus == MCP_BTS_SP_STATUS_PENDING), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranOnSm_HandlerAbortExecuteScript: MCP_BTS_SP_AbortScriptExecution Failed (%d)", btsSpStatus));
    
    status = _CCM_IM_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;  
}

/*
    This function is called when an abort event is sent to the SM from the parent while the SM is waiting for 
    BT_HCI_IF_MngrConfigHci to complete.
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerAbortConfigHci(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(thisObj);
    MCP_UNUSED_PARAMETER(eventData);
        
    /* 
        Do Nothing, Wait for version to be read and continue handling in the corresponding handler 
        (the state indicates that the process should be aborted)
    */
    
    MCP_LOG_INFO(("BT Tran On Abort While Configuring HVI, Doing nothing, waiting for HCI Config Complete Event"));

    return _CCM_IM_STATUS_PENDING;
}

/*
    This function is called when the process terminated successfully 
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerTranOnCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranOnSm_PerformCompletion(thisObj, _CCM_IM_STATUS_SUCCESS);
}

/*
    This function is called when the process failed for some reason 
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerOnFailed(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);

    MCP_LOG_FATAL(("_CCM_IM_BtTranOnSm_HandlerOnFailed: Tran On Failed"));
    
    return _CCM_IM_BtTranOnSm_PerformCompletion(thisObj, _CCM_IM_STATUS_FAILED);
}

/*
    This function is called when the process was aborted as a result of an abort request from the parent 
*/
_CcmImStatus _CCM_IM_BtTranOnSm_HandlerTranOnAbortCompleted(_CcmIm_BtTranOnSm_Obj *thisObj, void *eventData)
{
    MCP_UNUSED_PARAMETER(eventData);
    
    return _CCM_IM_BtTranOnSm_PerformCompletion(thisObj, _CCM_IM_STATUS_TRAN_ON_ABORTED);
}

_CcmImStatus _CCM_IM_BtTranOnSm_PerformCompletion(  _CcmIm_BtTranOnSm_Obj   *thisObj,
                                                                    _CcmImStatus            completionStatus)
{
    _CcmImStatus                        status;
    BtHciIfStatus                           btHciIfStatus;
    _CcmIm_BtTranOnSm_CompletionEvent   completionEvent;
        
    MCP_FUNC_START("_CCM_IM_BtTranOnSm_PerformCompletion");

    /*
        Restore the parent's BT HCI If manager callback 
    */
    btHciIfStatus = BT_HCI_IF_MngrChangeCb(thisObj->btHciIfObj, thisObj->btHciIfParentCb, NULL);
    MCP_VERIFY_FATAL((btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrChangeCb Failed"));
    
    MCP_LOG_INFO(("_CCM_IM_BtTranOnSm_PerformCompletion: BT Tran On Completed (status: %d), Calling Parent", completionStatus));
    
    /* 
        Notify parent SM (BtTranSm) of completion and its status. BtTranOnSm is not aware
        of the id of the stack that initiated Bt Tran On. It is up to the parent SM to record
        this information.
    */
    completionEvent.chipId = thisObj->chipId;
    completionEvent.completionStatus = completionStatus;

    /* Invoking the callback */
    (thisObj->parentCb)(&completionEvent);

    status =    _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called by BT_HCI_If to receive all the BT_HCI_If events

    The SM engine is called to process the event. This function maps the BT_HCI_If event to
    an SM engine event before forwarding it to the SM engine
*/
void _CCM_IM_BtTranOnSm_BtHciIfMngrCb(BtHciIfMngrEvent *event)
{
    _CcmIm_BtTranOnSm_Obj   *smObj = &_ccmIm_BtTranOnSm_Data.smObjs[MCP_HAL_CHIP_ID_0];

    _CcmIm_BtTranOnSm_Event tranOnSmEvent;
    
    MCP_FUNC_START("_CCM_IM_BtTranOnSm_BtHciIfMngrCb");

    MCP_VERIFY_FATAL_NO_RETVAR((smObj->btHciIfObj == event->reportingObj), ("Mismatching BT HCI If Objs"));
    
    switch (event->type)
    {
        case BT_HCI_IF_MNGR_EVENT_TRANPORT_ON_COMPLETED:

            if (event->status == BT_HCI_IF_STATUS_SUCCESS)
            {
                tranOnSmEvent = _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE;
            }
            else
            {
                tranOnSmEvent = _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED;
            }
            
        break;

        case BT_HCI_IF_MNGR_EVENT_SEND_RADIO_CMD_COMPLETED:

            tranOnSmEvent = _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE;

        break;

        case BT_HCI_IF_MNGR_EVENT_SET_TRANS_PARMS_COMPLETED:

            tranOnSmEvent = _CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE;

        break;

        case BT_HCI_IF_MNGR_EVENT_HCI_CONFIG_COMPLETED:

            tranOnSmEvent = _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE;

        break;

        default:
            MCP_FATAL_NO_RETVAR(("_CCM_IM_BtTranOnSm_BtHciIfMngrCb: Unexpected Event (%d)", event->type));
    }

    _CCM_IM_BtTranOnSm_InternalHandleEvent(smObj, tranOnSmEvent, event);
    
    MCP_FUNC_END();
}

/*
    This function generates an init script file name from the local version information returned in the HCI event
*/
_CcmImStatus _CCM_IM_BtTranOnSm_GetInitScriptFileName(_CcmIm_BtTranOnSm_Obj *thisObj,
                                                      const McpU8 *hciReadVersionEventParms,
                                                      char *scriptFileName,
                                                      McpUtf8 *scriptFileFullName)
{
    _CcmImStatus    status;
    McpU16          manufacturerId;
    McpU16          lmpSubversion;
    McpU16          versionMinor;
    McpU16          versionMajor;
    McpU16          projectType;

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_GetInitScriptFileName");
    
    /* Get Manufacturer name field value (stored in Little-Endian format) */
    manufacturerId = MCP_ENDIAN_LEtoHost16(&hciReadVersionEventParms[
                                                _CcmIm_BtTranOnSm_HciLocalVersionManufacturerNamePos]);

    MCP_VERIFY_FATAL(   (manufacturerId == _CcmIm_BtTranOnSm_TiManufacturerId), 
                        _CCM_IM_STATUS_INVALID_BT_HCI_VERSION_INFO,
                        ("_CCM_IM_BtTranOnSm_GetInitScriptFileName: Invalid Version Info (%d intstead of %d",
                            manufacturerId, _CcmIm_BtTranOnSm_TiManufacturerId));
    
    /* Get LMP Subversion field value (stored in Little-Endian format) */
    lmpSubversion = MCP_ENDIAN_LEtoHost16(&hciReadVersionEventParms[_CcmIm_BtTranOnSm_HciLocalVersionLmpSubversionPos]);

    versionMinor = (McpU16)(lmpSubversion & 0x007F);
    
    versionMajor = (McpU16)((lmpSubversion & 0x0380) >> 7);

    if ((lmpSubversion & 0x8000) != 0)
    {
            versionMajor |= 0x0008;
    }
        
    projectType = (McpU16)((lmpSubversion & 0x7C00) >> 10);

    /* store chip version for later use */
    thisObj->projectType = projectType;
    thisObj->versionMajor = versionMajor;
    thisObj->versionMinor = versionMinor;

    /* Format the name of the script file */
    MCP_HAL_STRING_Sprintf(scriptFileName,
                           "tiinit_%d.%d.%d.bts", 
                           projectType, 
                           versionMajor, 
                           versionMinor);

    /* Format the full path of the script file */
    MCP_StrCpyUtf8(scriptFileFullName,
                   (const McpUtf8 *)MCP_HAL_CONFIG_FS_SCRIPT_FOLDER);
    MCP_StrCatUtf8(scriptFileFullName, (const McpUtf8 *)scriptFileName);

    MCP_LOG_INFO(("Init Script File Name:|%s|", (char *)scriptFileFullName));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

void _CCM_IM_BtTranOnSm_GetChipVersion(_CcmIm_BtTranOnSm_Obj *thisObj, 
                                       McpU16 *projectType,
                                       McpU16 *versionMajor,
                                       McpU16 *versionMinor)
{
    MCP_FUNC_START("_CCM_IM_BtTranOnSm_GetChipVersion");

    *projectType = thisObj->projectType;
    *versionMajor = thisObj->versionMajor;
    *versionMinor = thisObj->versionMinor;

    MCP_FUNC_END ();
}

/*
    This module sends the HCI command via BT_HCI_If. This function should be called only
    for init script loading as it is using BT_HCI_IF_MngrSendRadioCommand
*/
McpBtsSpStatus _CCM_IM_BtTranOnSm_SpSendHciCmdCb(   McpBtsSpContext         *context,
                                                                    McpU16              hciOpcode, 
                                                                    McpU8               *hciCmdParms, 
                                                                    McpUint                 hciCmdParmsLen)
{
    McpBtsSpStatus          status;
    BtHciIfStatus               btHciIfStatus;
    _CcmIm_BtTranOnSm_Obj   *smObj = &_ccmIm_BtTranOnSm_Data.smObjs[MCP_HAL_CHIP_ID_0];

    MCP_FUNC_START("_CCM_IM_BtTranOnSm_HandlerTurnHciOn");
    
    MCP_UNUSED_PARAMETER(context);

    btHciIfStatus = BT_HCI_IF_MngrSendRadioCommand( smObj->btHciIfObj,
                                                        hciOpcode,
                                                        hciCmdParms,
                                                        (McpU8)hciCmdParmsLen);
    MCP_VERIFY_ERR((btHciIfStatus == BT_HCI_IF_STATUS_PENDING), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrSendRadioCommand Failed"));

    status = MCP_BTS_SP_STATUS_PENDING;
                                                
    MCP_FUNC_END();

    return status;
}

/*
    This function is called by the script processor when it needs to configure the transport parameters during
    execution of the BT init script
*/
McpBtsSpStatus _CCM_IM_BtTranOnSm_SpSetTranParmsCmdCb(  McpBtsSpContext             *context,
                                                                            McpUint                     speed, 
                                                                            McpBtSpFlowControlType  flowCtrlType)
{
    McpBtsSpStatus          status;
    BtHciIfStatus               btHciIfStatus;
    BtHciIfTranParms            btHciIfTanParms;
    _CcmIm_BtTranOnSm_Obj   *smObj = &_ccmIm_BtTranOnSm_Data.smObjs[MCP_HAL_CHIP_ID_0];
    
    MCP_FUNC_START("_CCM_IM_BtTranOnSm_SpSetTranParmsCmdCb");
    
    MCP_UNUSED_PARAMETER(context);
    MCP_UNUSED_PARAMETER(flowCtrlType);

    btHciIfTanParms.parms.uartParms.speed = speed;
    
    btHciIfStatus = BT_HCI_IF_MngrSetTranParms(smObj->btHciIfObj, &btHciIfTanParms);

    /* Transport parameters setting may complete synchronously or asynchronously */
    if (btHciIfStatus == BT_HCI_IF_STATUS_SUCCESS)
    {
        status = MCP_BTS_SP_STATUS_SUCCESS;
    }
    else if (btHciIfStatus == BT_HCI_IF_STATUS_PENDING)
    {
        status = MCP_BTS_SP_STATUS_PENDING;
    }
    else
    {
        MCP_FATAL(MCP_BTS_SP_STATUS_INTERNAL_ERROR,
                            ("_CCM_IM_BtTranOnSm_HandlerTurnHciOn: BT_HCI_IF_MngrSetTranParms Failed"));
    }
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called by the script processor when it completed executing the script. 

    Completion may be due to various reasons:
    1. Successful completion of all script commands
    2. Error during script execution
    3. Execution was aborted

    The SM engine is notified and handles the event
*/
void _CCM_IM_BtTranOnSm_SpExecCompleteCmdCb(McpBtsSpContext *context, McpBtsSpStatus status)
{
    _CcmIm_BtTranOnSm_Obj   *smObj = &_ccmIm_BtTranOnSm_Data.smObjs[MCP_HAL_CHIP_ID_0];
    
    MCP_UNUSED_PARAMETER(context);
    MCP_UNUSED_PARAMETER(status);

    /* [ToDo][Udi] - Handle execution failure */
    _CCM_IM_BtTranOnSm_InternalHandleEvent( smObj, 
                                                _CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE,
                                                NULL);
}

const char *_CCM_IM_BtTranOnSm_DebugStatetStr(_CcmIm_BtTranOnSm_State state)
{
    switch (state)
    {
        case _CCM_IM_BT_TRAN_ON_SM_STATE_NONE:      return "TRAN_ON_SM_STATE_NONE";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_TURN_HCI_ON:       return "TRAN_ON_SM_STATE_TURN_HCI_ON";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON:     return "TRAN_ON_SM_STATE_ABORT_TURN_HCI_ON";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_READ_VERSION:      return "TRAN_ON_SM_STATE_READ_VERSION";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_READ_VERSION:        return "TRAN_ON_SM_STATE_ABORT_READ_VERSION";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_EXECUTING_SCRIPT:      return "TRAN_ON_SM_STATE_EXECUTING_SCRIPT";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT:        return "TRAN_ON_SM_STATE_ABORT_EXECUTING_SCRIPT";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_CONFIG_HCI:        return "TRAN_ON_SM_STATE_CONFIG_HCI";
        case _CCM_IM_BT_TRAN_ON_SM_STATE_ABORT_CONFIG_HCI:      return "TRAN_ON_SM_STATE_ABORT_CONFIG_HCI";
        default:                                                return "INVALID BT TRAN ON SM STATE";
    }
}

const char *_CCM_IM_BtTranOnSm_DebugEventStr(_CcmIm_BtTranOnSm_Event event)
{
    switch (event)
    {
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_START:     return "TRAN_ON_SM_EVENT_START";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE:        return "TRAN_ON_SM_EVENT_HCI_IF_ON_COMPLETE";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED:      return "TRAN_ON_SM_EVENT_HCI_IF_ON_FAILED";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE:       return "TRAN_ON_SM_EVENT_SET_TRAN_PARMS_COMPLETE";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE:     return "TRAN_ON_SM_EVENT_HCI_COMMMAND_COMPLETE";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE:      return "TRAN_ON_SM_EVENT_SCRIPT_EXEC_COMPLETE";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE:       return "TRAN_ON_SM_EVENT_HCI_CONFIG_COMPLETE";
        case _CCM_IM_BT_TRAN_ON_SM_EVENT_ABORT:     return "TRAN_ON_SM_EVENT_ABORT";
        default:                                                return "INVALID BT TRAN ON SM EVENT";
    }
}

