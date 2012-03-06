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
#include "mcp_hal_string.h"
#include "ccm_hal_pwr_up_dwn.h"
#include "ccm_imi.h"
#include "ccm_imi_common.h"
#include "ccm_imi_bt_tran_mngr.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_IM);

typedef enum _tagCcmIm_StackEvent {
    _CCM_IM_STACK_EVENT_OFF,
    _CCM_IM_STACK_EVENT_ON_ABORT,
    _CCM_IM_STACK_EVENT_ON,

    _CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE,
    _CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED,
    _CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE,
    _CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED,
    _CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE,

    _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT,
    
    _CCM_IM_NUM_OF_STACK_EVENTS,
    _CCM_IM_INVALID_STACK_EVENT
} _CcmIm_StackEvent;

typedef struct _tagCcmImStackData _CcmImStackData;

typedef _CcmImStatus (*_CCM_IM_StackSmAction)(_CcmImStackData *stackData);

typedef struct _tagCcmIm_StackSmEntry {
    _CCM_IM_StackSmAction   action;
    CcmImStackState         nextState;
    _CcmIm_StackEvent       syncNextEvent;
} _CcmIm_StackSmEntry;

struct _tagCcmImStackData {
    CcmImObj            *containingImObj;
    CcmImStackId            stackId;
    CcmImEventCb        callback;
    CcmImStackState     state;
    McpBool             asyncCompletion;
};

struct tagCcmImObj {
    McpHalChipId                    chipId;
    McpHalOsSemaphoreHandle     mutexHandle;
    _CcmIm_BtTranMngr_Obj       *tranMngrObj;
    
    _CcmImStackData             stacksData[CCM_IM_MAX_NUM_OF_STACKS];
    
    CcmImEvent                  stackEvent;
    CcmChipOnNotificationCb     chipOnNotificationCB;
    void                        *userData;
    McpBool                     chipOnNotificationCalled;
};

typedef struct _tagCcmStaticData {
    CcmImObj                imObjs[MCP_HAL_MAX_NUM_OF_CHIPS];
    _CcmIm_StackSmEntry         stackSm[CCM_IM_NUM_OF_STACK_STATES][_CCM_IM_NUM_OF_STACK_EVENTS];
} _CcmImStaticData;

/*
    A single instance that holds the MCP_STATIC ("class") CCM data 
*/
MCP_STATIC  _CcmImStaticData _ccmIm_StaticData;

MCP_STATIC void _CCM_IM_StaticInitStackSm(void);

MCP_STATIC void _CCM_IM_InitObjData(CcmImObj *thisObj, McpHalChipId chipId);
MCP_STATIC void _CCM_IM_InitStackData(_CcmImStackData *stackData);

MCP_STATIC CcmImStatus _CCM_IM_HandleStackClientCall(_CcmImStackData *stackData, _CcmIm_StackEvent stackEvent);

MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandleEvent(_CcmImStackData *stackData, _CcmIm_StackEvent stackEvent);

MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_On(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OnFailed(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OnAbort(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_Off(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OffFailed(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OnComplete(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OnAbortComplete(_CcmImStackData *stackData);
MCP_STATIC _CcmImStatus _CCM_IM_StackSmHandler_OffComplete(_CcmImStackData *stackData);

MCP_STATIC _CcmImStatus _CCM_IM_NotifyStack(    _CcmImStackData *stackData, 
                                                    CcmImEventType  eventType,
                                                    CcmImStatus         completionStatus);


MCP_STATIC void _CCM_IM_BtTranMngrCb(_CcmIm_BtTranMngr_CompletionEvent *event);

MCP_STATIC void _CCM_IM_GenerateMutexName(McpHalChipId chipId, char *mutexName);

MCP_STATIC McpHalCoreId _CCM_IM_StackId2CoreId(CcmImStackId stackId);
MCP_STATIC McpBool _CCM_IM_IsStatusExternal(_CcmImStatus status);

MCP_STATIC const char *_CCM_IM_DebugStackIdStr(CcmImStackState stackId);
MCP_STATIC const char *_CCM_IM_DebugStackStateStr(CcmImStackState stackState);
MCP_STATIC const char *_CCM_IM_DebugStackEventStr(_CcmIm_StackEvent stackEvent);

CcmImStatus CCM_IM_StaticInit(void)
{
    _CcmImStatus    status;
    
    MCP_FUNC_START("CCM_IM_StaticInit");
    
    /* Init my own static data */
    _CCM_IM_StaticInitStackSm();

    /* Init the contained Tran SM module static data */
    status = _CCM_IM_BtTranMngr_StaticInit();
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), CCM_STATUS_INTERNAL_ERROR, ("CCM_IM_StaticInit"));
    
    MCP_FUNC_END();
    
    return status;
}

CcmImStatus CCM_IM_Create(McpHalChipId chipId, CcmImObj **thisObj,  CcmChipOnNotificationCb notificationCB, void *userData)
{
    _CcmImStatus    status;
    McpHalOsStatus  halOsStatus;
    char                mutexName[MCP_HAL_OS_MAX_ENTITY_NAME_LEN + 1];

    MCP_FUNC_START("CCM_IM_Create");
    
    *thisObj = &_ccmIm_StaticData.imObjs[chipId];

    _CCM_IM_InitObjData(*thisObj, chipId);

    /* Generate a name for the mutex of this instance (one per chip) */
    _CCM_IM_GenerateMutexName(chipId, mutexName);
    
    halOsStatus = MCP_HAL_OS_CreateSemaphore(mutexName, &((*thisObj)->mutexHandle));
    MCP_VERIFY_FATAL((halOsStatus == MCP_HAL_OS_STATUS_SUCCESS), CCM_IM_STATUS_INTERNAL_ERROR,
                        ("CCM_IM_Create: MCP_HAL_OS_CreateSemaphore Failed (%d)", halOsStatus));

    status = _CCM_IM_BtTranMngr_Create((*thisObj)->chipId, _CCM_IM_BtTranMngrCb, (*thisObj)->mutexHandle, &((*thisObj)->tranMngrObj));
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("CCM_IM_Create: _CCM_IM_BtTranMngr_Create Failed"));

    /* store the chip on notification BC and user data */
    (*thisObj)->chipOnNotificationCB = notificationCB;
    (*thisObj)->userData = userData;

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}

CcmImStatus CCM_IM_Destroy(CcmImObj **thisObj)
{
    CcmImStatus     status;
    McpHalOsStatus  halOsStatus;

    MCP_FUNC_START("CCM_IM_Destroy");

    status = _CCM_IM_BtTranMngr_Destroy(&((*thisObj)->tranMngrObj));
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("CCM_IM_Create: _CCM_IM_BtTranMngr_Destroy Failed"));
    
    halOsStatus = MCP_HAL_OS_DestroySemaphore(&((*thisObj)->mutexHandle));
    MCP_VERIFY_FATAL((halOsStatus == MCP_HAL_OS_STATUS_SUCCESS), CCM_IM_STATUS_INTERNAL_ERROR,
                        ("CCM_IM_Destroy: MCP_HAL_OS_CreateSemaphore Failed (%d)", halOsStatus));

    *thisObj = NULL;
    
    status = CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}


BtHciIfObj *CCM_IMI_GetBtHciIfObj(CcmImObj *thisObj)
{
    _CcmIm_BtTranMngr_Obj   *tranMngrObj;

    tranMngrObj = thisObj->tranMngrObj;

    return CCM_IM_BtTranMngr_GetBtHciIfObj(tranMngrObj);
}

CcmImStatus CCM_IM_RegisterStack(CcmImObj           *thisObj, 
                                        CcmImStackId        stackId,
                                        CcmImEventCb        callback,
                                        CCM_IM_StackHandle  *stackHandle)
{
    CcmImStatus     status;
    _CcmImStackData     *stackData;

    MCP_FUNC_START("CCM_IM_RegisterClient");

    MCP_VERIFY_FATAL((thisObj != NULL), CCM_IM_STATUS_INVALID_PARM, ("CCM_IM_RegisterClient: Null this"));
    MCP_VERIFY_FATAL((stackId < CCM_IM_MAX_NUM_OF_STACKS), CCM_IM_STATUS_INVALID_PARM, 
                        ("CCM_IM_RegisterClient: Invalid StackId", stackId));
    MCP_VERIFY_FATAL((callback != NULL), CCM_IM_STATUS_INVALID_PARM, ("CCM_IM_RegisterClient: Null callback"));

    stackData = &thisObj->stacksData[stackId];

    stackData->containingImObj = thisObj;
    stackData->stackId = stackId;
    stackData->callback = callback;

    *stackHandle = stackData;
    
    status = CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

CcmImStatus CCM_IM_DeregisterStack(CCM_IM_StackHandle *stackHandle)
{
    CcmImStatus     status;
    _CcmImStackData     *stackData;
    
    MCP_FUNC_START("CCM_IM_DeregisterStack");

    MCP_VERIFY_ERR((stackHandle != NULL), CCM_IM_STATUS_INVALID_PARM, ("CCM_IM_DeregisterClient: Null this"));
    
    stackData = (_CcmImStackData*)(*stackHandle);    
    
    MCP_VERIFY_ERR((stackData->stackId != CCM_IM_INVALID_STACK_ID), CCM_IM_STATUS_INVALID_PARM, 
                        ("CCM_IM_DeregisterStack: Stack (%s) Is not registered", 
                        _CCM_IM_DebugStackIdStr(stackData->stackId)));
    
    _CCM_IM_InitStackData(stackData);
    
    *stackHandle = NULL;
    
    status = CCM_IM_STATUS_SUCCESS;

    MCP_FUNC_END();

    return status;
}

CcmImStatus CCM_IM_StackOn(CCM_IM_StackHandle stackHandle)
{
    CcmImStatus     status;
    _CcmImStackData     *stackData;
    
    MCP_FUNC_START("CCM_IM_StackOn");

    stackData = (_CcmImStackData*)stackHandle;
    
    status = _CCM_IM_HandleStackClientCall(stackData, _CCM_IM_STACK_EVENT_ON);

    MCP_FUNC_END();

    return status;
}

CcmImStatus CCM_IM_StackOnAbort(CCM_IM_StackHandle stackHandle)
{
    CcmImStatus     status;
    _CcmImStackData     *stackData;
    
    MCP_FUNC_START("CCM_IM_StackOnAbort");

    stackData = (_CcmImStackData*)stackHandle;
    
    status = _CCM_IM_HandleStackClientCall(stackData, _CCM_IM_STACK_EVENT_ON_ABORT);

    MCP_FUNC_END();

    return status;
}

CcmImStatus CCM_IM_StackOff(CCM_IM_StackHandle stackHandle)
{
    CcmImStatus     status;
    _CcmImStackData     *stackData;
    
    MCP_FUNC_START("CCM_IM_StackOff");

    stackData = (_CcmImStackData*)stackHandle;
    
    status = _CCM_IM_HandleStackClientCall(stackData, _CCM_IM_STACK_EVENT_OFF);

    MCP_FUNC_END();

    return status;
}

CcmImObj *CCM_IM_GetImObj(CCM_IM_StackHandle stackHandle)
{
    _CcmImStackData     *stackData;
    
    stackData = (_CcmImStackData*)stackHandle;
    
    return stackData->containingImObj;
}

CcmImStackState CCM_IM_GetStackState(CCM_IM_StackHandle *stackHandle)
{
    CcmImStackState state;
    _CcmImStackData     *stackData;
    
    MCP_FUNC_START("CCM_IM_GetStackState");
    
    MCP_VERIFY_ERR_SET_RETVAR((stackHandle != NULL), (state = CCM_IM_INVALID_STACK_STATE), 
                                    ("CCM_IM_GetStackState: Null stackHandle"));

    stackData = (_CcmImStackData*)stackHandle;

    state = stackData->state;

    MCP_FUNC_END();

    return state;
}

void CCM_IM_GetChipVersion(CCM_IM_StackHandle *stackHandle,
                           McpU16 *projectType,
                           McpU16 *versionMajor,
                           McpU16 *versionMinor)
{
    _CcmImStackData     *stackData;

    MCP_FUNC_START("CCM_IM_GetChipVersion");

    stackData = (_CcmImStackData*)stackHandle;

    _CCM_IM_BtTranMngr_GetChipVersion(stackData ->containingImObj->tranMngrObj, projectType,
                                      versionMajor, versionMinor);
    
    MCP_FUNC_END();
}

void _CCM_IM_StaticInitStackSm(void)
{
    McpUint stackStateIdx;
    McpUint stackEventIdx;

    for (stackStateIdx = 0; stackStateIdx < CCM_IM_NUM_OF_STACK_STATES; ++stackStateIdx)
    {   
        for (stackEventIdx = 0; stackEventIdx < _CCM_IM_NUM_OF_STACK_EVENTS; ++stackEventIdx)
        {
            _ccmIm_StaticData.stackSm[stackStateIdx][stackEventIdx].action = NULL;
            _ccmIm_StaticData.stackSm[stackStateIdx][stackEventIdx].nextState = CCM_IM_INVALID_STACK_STATE;
            _ccmIm_StaticData.stackSm[stackStateIdx][stackEventIdx].syncNextEvent = _CCM_IM_INVALID_STACK_EVENT;
        }
    }

    /*
        {State Off, Event On} -> On In Progress (Async) / On (Sync)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF]
                                [_CCM_IM_STACK_EVENT_ON].action = _CCM_IM_StackSmHandler_On;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF]
                                [_CCM_IM_STACK_EVENT_ON].nextState = CCM_IM_STACK_STATE_ON_IN_PROGRESS;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF]
                                [_CCM_IM_STACK_EVENT_ON].syncNextEvent = _CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE;
    /*
        {State On, Event Off} -> Off In Progress (Async) / Off (Sync)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON]
                                [_CCM_IM_STACK_EVENT_OFF].action = _CCM_IM_StackSmHandler_Off;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON]
                                [_CCM_IM_STACK_EVENT_OFF].nextState = CCM_IM_STACK_STATE_OFF_IN_PROGRESS;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON]
                                [_CCM_IM_STACK_EVENT_OFF].syncNextEvent = _CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE;

    /*
        {State On In Progress, Event On Abort} -> On Abort In Progress (Async) / Off (Sync)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_ON_ABORT].action = 
                                                                _CCM_IM_StackSmHandler_OnAbort;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_ON_ABORT].nextState = 
                                                                CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_ON_ABORT].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE;
    
    /*
        {State On In Progress, Event Tran On Complete} -> On (must complete synchronously)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE].action = _CCM_IM_StackSmHandler_OnComplete;
    
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE].nextState = CCM_IM_STACK_STATE_ON;
    
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT;

    /*
        {State On In Progress, Event On Failed} -> On Abort In Progress (Async) / Off (Sync)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED].action = 
                                                                _CCM_IM_StackSmHandler_OnFailed;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED].nextState = 
                                                                CCM_IM_STACK_FATAL_ERROR;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT;

    /*
        {State Off In Progress, Event Tran Off Complete} -> Off (must complete synchronously)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE].action = _CCM_IM_StackSmHandler_OffComplete;
    
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE].nextState = CCM_IM_STACK_STATE_OFF;
    
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_OFF_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT;
    
    /*
        {State On In Progress, Event On Failed} -> On Abort In Progress (Async) / Off (Sync)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED].action = 
                                                                _CCM_IM_StackSmHandler_OffFailed;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED].nextState = 
                                                                CCM_IM_STACK_FATAL_ERROR;
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_IN_PROGRESS]
                                    [_CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT;
    
    /*
        {State On Abort In Progress, Event Tran On Abort Complete} -> Off (must complete synchronously)
    */
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE].action = 
                                                                _CCM_IM_StackSmHandler_OnAbortComplete; 
    
    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE].nextState = 
                                                                CCM_IM_STACK_STATE_OFF; 

    _ccmIm_StaticData.stackSm[CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS]
                                [_CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE].syncNextEvent = 
                                                                _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT;  

}

void _CCM_IM_InitObjData(CcmImObj *thisObj, McpHalChipId chipId)
{
    _CcmIm_SmStackIdx   stackIdx;
    
    thisObj->chipId = chipId;
    thisObj->mutexHandle = MCP_HAL_OS_INVALID_SEMAPHORE_HANDLE;
    thisObj->tranMngrObj = NULL;
    thisObj->chipOnNotificationCalled = MCP_FALSE;

    for (stackIdx  = 0; stackIdx < CCM_IM_MAX_NUM_OF_STACKS; ++stackIdx)
    {
        _CCM_IM_InitStackData(&thisObj->stacksData[stackIdx]);
    }
}

void _CCM_IM_InitStackData(_CcmImStackData *stackData)
{   
    stackData->containingImObj = NULL;
    stackData->stackId = CCM_IM_INVALID_STACK_ID;
    stackData->callback = NULL;
    
    stackData->state = CCM_IM_STACK_STATE_OFF;
    stackData->asyncCompletion = MCP_FALSE;
}

CcmImStatus _CCM_IM_HandleStackClientCall(_CcmImStackData *stackData, _CcmIm_StackEvent stackEvent)
{
    CcmImStatus status;
    CcmImStatus internalStatus;
    
    MCP_FUNC_START("_CCM_IM_HandleStackClientCall");

    stackData->asyncCompletion = MCP_FALSE;
        
    internalStatus = _CCM_IM_StackSmHandleEvent(stackData, stackEvent);

    /*
        3 cases:
        1. There wasn't a matching stack SM entry => BT stack is in a state that prevents calling BtOn. This could
            be when it is in one of multiple states, such as , 'on-in-progress', 'on', 'on-abort', among others.
        2. Some status code (e.g., pending) that is understood by the caller (external status) => return that status
            to the caller and let it act accordingly.
        3. Some internal code (caller can't understand it). In that case it indicates that a bug occurred => FATAL
    */
    if (internalStatus == _CCM_IM_STATUS_NULL_SM_ENTRY)
    {
        MCP_LOG_ERROR(("_CCM_IM_HandleStackClientCall: %s State Prevents Calling %s %s At this point", 
                            _CCM_IM_DebugStackIdStr(stackData->stackId), 
                            _CCM_IM_DebugStackIdStr(stackData->stackId),
                            _CCM_IM_DebugStackEventStr(stackEvent)));
        
        MCP_RET(CCM_IM_STATUS_IMPROPER_STATE);
    }
    else if (_CCM_IM_IsStatusExternal(internalStatus) == MCP_TRUE)
    {
        MCP_RET((CcmImStatus)internalStatus);
    }
    else
    {
        MCP_FATAL(CCM_IM_STATUS_INTERNAL_ERROR, ("_CCM_IM_HandleStackClientCall"));
    }
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_StackSmHandleEvent(_CcmImStackData *stackData, _CcmIm_StackEvent stackEvent)
{
    _CcmImStatus            status;
    _CcmIm_StackSmEntry     *smEntry = NULL;
    CcmImObj                *imObj;
    
    MCP_FUNC_START("_CCM_IM_StackSmHandleEvent");

    imObj = stackData->containingImObj;
    
    MCP_HAL_OS_LockSemaphore(imObj->mutexHandle, MCP_HAL_OS_TIME_INFINITE);     

    MCP_VERIFY_FATAL((stackEvent < _CCM_IM_NUM_OF_STACK_EVENTS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_StackSmHandleEvent: Invalid stack event (%d)", stackEvent));

    MCP_LOG_INFO(("_CCM_IM_StackSmHandleEvent: Handling SM{%s, %s", 
                    _CCM_IM_DebugStackStateStr(stackData->state),
                    _CCM_IM_DebugStackEventStr(stackEvent)));

    /* Extract the SM entry matching the current {state, event} */
    smEntry = &_ccmIm_StaticData.stackSm[stackData->state][stackEvent];
    MCP_VERIFY_FATAL((smEntry->action != NULL), _CCM_IM_STATUS_NULL_SM_ENTRY, ("No matching Stack SM Entry"));

    MCP_VERIFY_FATAL((smEntry->nextState != CCM_IM_INVALID_STACK_STATE), 
                        _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_StackSmHandleEvent: Invalid Next SM State"));

    /* Transit to the next SM state */
    stackData->state = smEntry->nextState;
            
    /* Invoke the action */
    status = (smEntry->action)(stackData);

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_CCM_IM_StackSmHandleEvent: Action Completed Synchronously"));
    }
    else if (status == _CCM_IM_STATUS_PENDING)
    {
        stackData->asyncCompletion = MCP_TRUE;  
    }
    else
    {
        /* Action failed */
        MCP_ERR(status, ("_CCM_IM_StackSmHandleEvent"));
    }
    
    if (status == _CCM_IM_STATUS_SUCCESS)
    {   
        MCP_VERIFY_FATAL((smEntry->syncNextEvent != _CCM_IM_INVALID_STACK_EVENT), 
                            _CCM_IM_STATUS_INTERNAL_ERROR, 
                            ("_CCM_IM_StackSmHandleEvent: Missing syncNextEvent"));

        if (smEntry->syncNextEvent != _CCM_IM_STACK_EVENT_BT_TRAN_ON_NULL_EVENT)
        {
            status = _CCM_IM_StackSmHandleEvent(stackData, smEntry->syncNextEvent);
        }
    }

    MCP_LOG_INFO(("_CCM_IM_StackSmHandleEvent: On Exit, State: %s",
                        _CCM_IM_DebugStackStateStr(stackData->state)));
    
    MCP_FUNC_END();

    /* Unlocking AFTER MCP_FUNC_END() to make sure all paths (including erroneous) will unlock before exiting */
    MCP_HAL_OS_UnlockSemaphore(imObj->mutexHandle);
    
    return status;
}

/*
    Pseudo Code:

    Power up the nShutdown line that corresponds to this stack

    If this stack is configured NOT to use the shared BT transport
    
        Exit successfully (nothing else to do)
        
    Else
    
        Send BtTran SM a Tran On event

        If the return code is SUCCESS (Tran On Completed)
            Change Stack state to "On"
            Return with SUCCESS

        Else
            Change stack state to "On Pending"
            Return with PENDING
        End If
        
    End If
        
*/
_CcmImStatus _CCM_IM_StackSmHandler_On(_CcmImStackData *stackData)
{
    _CcmImStatus            status;
    CcmHalPwrUpDwnStatus    pwrUpDwnStatus;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_On");
    
    pwrUpDwnStatus = CCM_HAL_PWR_UP_DWN_Reset(stackData->containingImObj->chipId, _CCM_IM_StackId2CoreId(stackData->stackId));
    MCP_VERIFY_FATAL((pwrUpDwnStatus == CCM_HAL_PWR_UP_DWN_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_StackSmHandler_On: CCM_HAL_PWR_UP_DWN_Reset (Chip %d) Failed (%d)", 
                        stackData->containingImObj->chipId, pwrUpDwnStatus));
    /*
        [ToDo] - Check if this stack is using the shared BT transport. Currently assume it is
    */

    status  = _CCM_IM_BtTranMngr_HandleEvent(   stackData->containingImObj->tranMngrObj, 
                                                stackData->stackId, 
                                                _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS) || (status == _CCM_IM_STATUS_PENDING),
                        _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_StackSmHandler_On: _CCM_IM_BtTranMngrHandleEvent Failed"));
    
    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_OnAbort(_CcmImStackData *stackData)
{
    _CcmImStatus                status;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OnAbort");

    /*
        [ToDo] - Check if this stack is using the shared BT transport. Currently assume it is
    */

    status  = _CCM_IM_BtTranMngr_HandleEvent(   stackData->containingImObj->tranMngrObj, 
                                                stackData->stackId, 
                                                _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS) || (status == _CCM_IM_STATUS_PENDING),
                        _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_StackSmHandler_OnAbort: _CCM_IM_BtTranMngrHandleEvent Failed"));
    
    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_Off(_CcmImStackData *stackData)
{
    _CcmImStatus            status;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_Off");
    
    /*
        [ToDo] - Check if this stack is using the shared BT transport. Currently assume it is
    */

    status  = _CCM_IM_BtTranMngr_HandleEvent(   stackData->containingImObj->tranMngrObj, 
                                                stackData->stackId, 
                                                _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS) || (status == _CCM_IM_STATUS_PENDING),
                        _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_StackSmHandler_Off: _CCM_IM_BtTranMngrHandleEvent Failed"));
    
    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_OnComplete(_CcmImStackData *stackData)
{
    _CcmImStatus            status;
    CcmImObj                *thisObj;
    McpU16                  projectType, versionMajor, versionMinor;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OnComplete");

    thisObj = stackData->containingImObj;

    /*if this is the first ON notification */
    if (MCP_FALSE == thisObj->chipOnNotificationCalled)
    {
        thisObj->chipOnNotificationCalled = MCP_TRUE;
        /* notify other CCM modules, with the chip version */
        _CCM_IM_BtTranMngr_GetChipVersion (thisObj->tranMngrObj, &projectType, &versionMajor, &versionMinor);
        thisObj->chipOnNotificationCB (thisObj->userData, projectType, versionMajor, versionMinor);
    }

    _CCM_IM_NotifyStack(stackData, CCM_IM_EVENT_TYPE_ON_COMPLETE, CCM_IM_STATUS_SUCCESS);
    
    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_OnFailed(_CcmImStackData *stackData)
{
    _CcmImStatus            status;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OnFailed");

    _CCM_IM_NotifyStack(stackData, CCM_IM_EVENT_TYPE_ON_COMPLETE, CCM_IM_STATUS_FAILED);
    
    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_OnAbortComplete(_CcmImStackData *stackData)
{
    _CcmImStatus            status;
    CcmHalPwrUpDwnStatus    pwrUpDwnStatus;
    CcmImObj                *imObj;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OnAbortComplete");

    imObj = stackData->containingImObj;
    
    /* Stack reverts to Off state => core must be shiut down */
    pwrUpDwnStatus = CCM_HAL_PWR_UP_DWN_Shutdown(   imObj->chipId, 
                                                        _CCM_IM_StackId2CoreId(stackData->stackId));
    MCP_VERIFY_FATAL((pwrUpDwnStatus == CCM_HAL_PWR_UP_DWN_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_StackSmHandler_OnAbortComplete: CCM_HAL_PWR_UP_DWN_Shutdown (Chip %d) Failed (%d)", 
                        imObj->chipId, pwrUpDwnStatus));

    _CCM_IM_NotifyStack(stackData, CCM_IM_EVENT_TYPE_ON_ABORT_COMPLETE, CCM_IM_STATUS_SUCCESS);
    
    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_StackSmHandler_OffComplete(_CcmImStackData *stackData)
{
    _CcmImStatus            status;
    CcmHalPwrUpDwnStatus    pwrUpDwnStatus;
    CcmImObj                *imObj;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OffComplete");

    imObj = stackData->containingImObj;
    
    /* Stack is goinf off => core must be shiut down */
    pwrUpDwnStatus = CCM_HAL_PWR_UP_DWN_Shutdown(imObj->chipId, _CCM_IM_StackId2CoreId(stackData->stackId));
    MCP_VERIFY_FATAL((pwrUpDwnStatus == CCM_HAL_PWR_UP_DWN_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_StackSmHandler_OffComplete: CCM_HAL_PWR_UP_DWN_Shutdown (Chip %d) Failed (%d)", 
                        imObj->chipId, pwrUpDwnStatus));
    
    _CCM_IM_NotifyStack(stackData, CCM_IM_EVENT_TYPE_OFF_COMPLETE, CCM_IM_STATUS_SUCCESS);
    
    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}


_CcmImStatus _CCM_IM_StackSmHandler_OffFailed(_CcmImStackData *stackData)
{
    _CcmImStatus            status;

    MCP_FUNC_START("_CCM_IM_StackSmHandler_OffFailed");

    _CCM_IM_NotifyStack(stackData, CCM_IM_EVENT_TYPE_OFF_COMPLETE, CCM_IM_STATUS_FAILED);
    
    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_NotifyStack(   _CcmImStackData *stackData, 
                                        CcmImEventType  eventType, 
                                        CcmImStatus         completionStatus)
{
    _CcmImStatus    status;
    CcmImObj        *imObj;

    MCP_FUNC_START("_CCM_IM_NotifyStack");

    MCP_VERIFY_FATAL((stackData->callback != NULL), CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_NotifyStack: Stack (%s) Is not registered", _CCM_IM_DebugStackIdStr(stackData->stackId)));

    imObj = stackData->containingImObj;
    
    if (stackData->asyncCompletion == MCP_TRUE)
    {
        imObj->stackEvent.type = eventType;
        imObj->stackEvent.status = completionStatus;
        
        /* Set the stack id in the event. All other fields should have been filled already */
        imObj->stackEvent.stackId = stackData->stackId;

        /* Call the callback with the event */
        (stackData->callback)(&imObj->stackEvent);
    }

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

void _CCM_IM_BtTranMngrCb(_CcmIm_BtTranMngr_CompletionEvent *event)
{
    CcmImObj            *ccmImObj;
    _CcmImStackData     *stackData;
    _CcmIm_StackEvent   stackEvent;

    MCP_FUNC_START("_CCM_IM_BtTranMngrCb");

    MCP_LOG_INFO(("_CCM_IM_BtTranMngrCb: Received Event (%d), Chip: %d, StackId: %d, Status: %d", 
                        event->eventType, event->chipId, event->stackId, event->completionStatus));
        
    switch (event->eventType)
    {
        case _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_COMPLETED:

            if (event->completionStatus == _CCM_IM_STATUS_SUCCESS)
            {
                stackEvent = _CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE;
            }
            else
            {
                stackEvent = _CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED;
            }
        
        break;

        case _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_OFF_COMPLETED:

            if (event->completionStatus == _CCM_IM_STATUS_SUCCESS)
            {
                stackEvent = _CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE;
            }
            else
            {
                stackEvent = _CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED;
            }

        break;

        case _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED:

            stackEvent = _CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE;

        break;
        
        default:
            MCP_FATAL_NO_RETVAR(("_CCM_IM_BtTranMngrCb: Unexected Event (%d)", event->eventType));
    }

    ccmImObj = &_ccmIm_StaticData.imObjs[event->chipId];
    stackData = &ccmImObj->stacksData[event->stackId];

    _CCM_IM_StackSmHandleEvent(stackData, stackEvent);

    MCP_FUNC_END();
}

void _CCM_IM_GenerateMutexName(McpHalChipId chipId, char *mutexName)
{
    MCP_HAL_STRING_Sprintf(mutexName, "%s%d", CCM_IM_CONFIG_MUTEX_NAME, chipId);
}

McpHalCoreId _CCM_IM_StackId2CoreId(CcmImStackId stackId)
{
    McpHalCoreId    coreId;
    
    MCP_FUNC_START("_CCM_IM_StackId2CoreId");
    
    switch (stackId)
    {
        case CCM_IM_STACK_ID_BT:                
            coreId = MCP_HAL_CORE_ID_BT;
        break;
        
        case CCM_IM_STACK_ID_FM:    
            coreId = MCP_HAL_CORE_ID_FM;
        break;
        
        case CCM_IM_STACK_ID_GPS:
            coreId = MCP_HAL_CORE_ID_GPS;
        break;
        
        default:
            MCP_FATAL_SET_RETVAR((coreId =MCP_HAL_INVALID_CORE_ID), ("_CCM_IM_StackId2CoreId: Invalid stackId (%d)", stackId));
    }

    MCP_FUNC_END();

    return coreId;
}

McpBool _CCM_IM_IsStatusExternal(_CcmImStatus status)
{
    if (status < _CCM_IM_FIRST_INTERNAL_STATUS)
    {
        return MCP_TRUE;
    }
    else
    {
        return MCP_FALSE;
    }
}

const char *_CCM_IM_DebugStackIdStr(CcmImStackState stackId)
{
    switch (stackId)
    {
        case CCM_IM_STACK_ID_BT:    return "BT";
        case CCM_IM_STACK_ID_FM:    return "FM";
        case CCM_IM_STACK_ID_GPS:   return "GPS";
        
        default:                        return "INVALID STACK ID";
    }
}

const char *_CCM_IM_DebugStackStateStr(CcmImStackState stackState)
{
    switch (stackState)
    {
        case CCM_IM_STACK_STATE_OFF:                        return "STACK_STATE_OFF";
        case CCM_IM_STACK_STATE_OFF_IN_PROGRESS:            return "STACK_STATE_OFF_IN_PROGRESS";
        case CCM_IM_STACK_STATE_ON_IN_PROGRESS:             return "STACK_STATE_ON_IN_PROGRESS";
        case CCM_IM_STACK_STATE_ON_ABORT_IN_PROGRESS:   return "STACK_STATE_ON_ABORT_IN_PROGRESS";
        case CCM_IM_STACK_STATE_ON:                         return "STACK_STATE_ON";
        case CCM_IM_STACK_FATAL_ERROR:                      return "STACK_FATAL_ERROR";
        default:                                                return "INVALID STACK STATE";
    }
}

const char *_CCM_IM_DebugStackEventStr(_CcmIm_StackEvent stackEvent)
{
    switch (stackEvent)
    {
        case _CCM_IM_STACK_EVENT_OFF:                           return "STACK_EVENT_OFF";
        case _CCM_IM_STACK_EVENT_ON_ABORT:                  return "STACK_EVENT_ON_ABORT";
        case _CCM_IM_STACK_EVENT_ON:                            return "STACK_EVENT_ON";
        case _CCM_IM_STACK_EVENT_BT_TRAN_OFF_COMPLETE:      return "STACK_EVENT_BT_TRAN_OFF_COMPLETE";
        case _CCM_IM_STACK_EVENT_BT_TRAN_OFF_FAILED:            return "STACK_EVENT_BT_TRAN_OFF_FAILED";
        case _CCM_IM_STACK_EVENT_BT_TRAN_ON_COMPLETE:       return "STACK_EVENT_BT_TRAN_ON_COMPLETE";
        case _CCM_IM_STACK_EVENT_BT_TRAN_ON_FAILED:             return "STACK_EVENT_BT_TRAN_ON_FAILED";
        case _CCM_IM_STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE:     return "STACK_EVENT_BT_TRAN_ON_ABORT_COMPLETE";
        default:                                                    return "INVALID STACK EVENT";
    }
}

