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
#include "mcp_utils_dl_list.h"
#include "mcp_pool.h"
#include "ccm_imi_bt_tran_mngr.h"
#include "ccm_imi_bt_tran_sm.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_IM);

typedef enum _tagCcmIm_BtTranMngr_PendingStackState {
    _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_ON,
    _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_OFF,

    _CCM_IM_NUM_OF_BT_TRAN_MNGR_PENDING_STACK_STATES,
    _CCM_IM_INVALID_BT_TRAN_MNGR_PENDING_STACK_STATE
} _CcmIm_BtTranMngr_PendingStackState;

typedef struct _tagCcmIm_BtTranMngr_PendingStackData {
    MCP_DL_LIST_Node                    node;
    
    CcmImStackId                            stackId;
    _CcmIm_BtTranMngr_PendingStackState state;
    McpBool                             asyncCompletion;
} _CcmIm_BtTranMngr_PendingStackData;

struct _tagCcmIm_BtTranMngr_Obj {
    McpHalChipId                        chipId;
    _CcmIm_BtTranMngr_CompletionCb  parentCb;

    _CcmIm_BtTranSm_Obj             *tranSmObj;

    CcmImStackId                        activeStackId;

    McpUint                         numOfOnStacks;

    MCP_DL_LIST_Node                pendingListHead;
    McpBool                         iteratingPendingStacks;

        MCP_POOL_DECLARE_POOL(  pendingStacksPool, 
                                            pendingStacksMemory, 
                                            CCM_IM_MAX_NUM_OF_STACKS, 
                                            sizeof(_CcmIm_BtTranMngr_PendingStackData));
};

typedef struct _tagCcmIm_BtTranMngr_Data {
    _CcmIm_BtTranMngr_Obj       tranMngrObjs[MCP_HAL_MAX_NUM_OF_CHIPS];
} _CcmIm_BtTranMngr_Data;

MCP_STATIC  _CcmIm_BtTranMngr_Data  _ccmIm_BtTranMngr_Data;

MCP_STATIC const char _ccmIm_PendingStacksPoolName[] = "PendingStacks";

MCP_STATIC void _CCM_IM_BtTranMngr_StaticInitInstances(void);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleNonActiveStackEventWhileTranInProgress(    
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId,
                                _CcmIm_BtTranMngr_Event event);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPendingOn( 
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPendingOff(    
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPending(   
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                CcmImStackId                            stackId,
                                _CcmIm_BtTranMngr_PendingStackState pendingState);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue( 
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                CcmImStackId                            stackId,
                                _CcmIm_BtTranMngr_PendingStackState pendingState,
                                McpBool                             asyncCompletion);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_RemoveStackFromPendingQueue( 
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandlePendingStackOnAbort(   
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleStackEventWhileTranNotInProgress(  
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId,
                                _CcmIm_BtTranMngr_Event event);

MCP_STATIC void _CCM_IM_BtTranMngr_TranSmCb(_CcmIm_BtTranSm_CompletionEvent *event);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleTranOffCompletedEvent(
                            _CcmIm_BtTranMngr_Obj   *thisObj,
                            CcmImStackId                notifyingStackId,
                            _CcmImStatus            completionStatus);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleTranOnCompletedEvent(
                            _CcmIm_BtTranMngr_Obj   *thisObj,
                            CcmImStackId                notifyingStackId,
                            _CcmImStatus            completionStatus);
                            
MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandleTranOnAbortCompletedEvent(
                            _CcmIm_BtTranMngr_Obj   *thisObj,
                            CcmImStackId                notifyingStackId,
                            _CcmImStatus            completionStatus);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_HandlePendingStacks(_CcmIm_BtTranMngr_Obj *thisObj);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_PendingStackOn(
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_PendingStackOff(
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData);

MCP_STATIC _CcmImStatus _CCM_IM_BtTranMngr_NotifyStack(
                                    _CcmIm_BtTranMngr_Obj                   *thisObj, 
                                    CcmImStackId                                stackId,
                                    _CcmIm_BtTranMngr_CompletionEventType       eventType,
                                    _CcmImStatus                            completionStatus);

MCP_STATIC _CcmIm_BtTranSm_Event _CCM_IM_BtTranMngr_External2TranSmEvent(_CcmIm_BtTranMngr_Event externalEvent);

MCP_STATIC McpBool _CCM_IM_BtTranMngr_PendingQueueMatchByStackId(
                            const MCP_DL_LIST_Node *nodeToMatch, 
                            const MCP_DL_LIST_Node* checkedNode);

MCP_STATIC _CcmIm_BtTranMngr_PendingStackData *_CCM_IM_BtTranMngr_FindPendingStackData(     
                                                    _CcmIm_BtTranMngr_Obj   *thisObj,
                                                    CcmImStackId                stackId);

_CcmImStatus _CCM_IM_BtTranMngr_StaticInit(void)
{
    _CcmImStatus    status;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_StaticInit");

    _CCM_IM_BtTranMngr_StaticInitInstances();
    
    status = _CCM_IM_BtTranSm_StaticInit();
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_StaticInit: _CCM_IM_BtTranOnSm_StaticInit Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}

_CcmImStatus _CCM_IM_BtTranMngr_Create( McpHalChipId                        chipId,
                                                    _CcmIm_BtTranMngr_CompletionCb  parentCb,
                                                    McpHalOsSemaphoreHandle         ccmImMutexHandle,
                                                    _CcmIm_BtTranMngr_Obj           **thisObj)
{
    _CcmImStatus    status;
    McpPoolStatus   mcpPoolStatus;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_Create");

    *thisObj = &_ccmIm_BtTranMngr_Data.tranMngrObjs[chipId];
    
    (*thisObj)->chipId = chipId;
    (*thisObj)->parentCb = parentCb;
    (*thisObj)->activeStackId = CCM_IM_INVALID_STACK_ID;
    (*thisObj)->numOfOnStacks = 0;

    /* Initialize pending stacks list head */
    MCP_DL_LIST_InitializeHead(&(*thisObj)->pendingListHead);
    (*thisObj)->iteratingPendingStacks = MCP_FALSE;

    /* Create the memory pool for pending stack info objects */
    mcpPoolStatus = MCP_POOL_Create(    &(*thisObj)->pendingStacksPool ,
                                                  _ccmIm_PendingStacksPoolName,
                                                    (*thisObj)->pendingStacksMemory, 
                                        CCM_IM_MAX_NUM_OF_STACKS, 
                                        sizeof(_CcmIm_BtTranMngr_PendingStackData));
    MCP_VERIFY_FATAL((MCP_POOL_STATUS_SUCCESS == mcpPoolStatus), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("Pending Stacks pool creation failed"));

    /* Create child instance */
    status  = _CCM_IM_BtTranSm_Create(  (*thisObj)->chipId,
                                        _CCM_IM_BtTranMngr_TranSmCb,
                                        ccmImMutexHandle,
                                        &(*thisObj)->tranSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_Create: _CCM_IM_BtTranSm_Create Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();
    
    return status;
}
                                            
_CcmImStatus _CCM_IM_BtTranMngr_Destroy(_CcmIm_BtTranMngr_Obj **thisObj)
{
    _CcmImStatus    status;
    McpPoolStatus   mcpPoolStatus;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_Create");
    
    status  = _CCM_IM_BtTranSm_Destroy(&(*thisObj)->tranSmObj);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_Destroy: _CCM_IM_BtTranSm_Destroy Failed"));

    mcpPoolStatus = MCP_POOL_Destroy(&(*thisObj)->pendingStacksPool);
    MCP_VERIFY_FATAL((MCP_POOL_STATUS_SUCCESS == mcpPoolStatus), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("Pending Stacks pool Destruction failed"));

    *thisObj = NULL;

    MCP_FUNC_END();
    
    return status;
}

/*
    Pseudo Code:
    
    If the transport state machine is in progress of some action (on / off / on abort)
        If the event is coming from the active stack (the stack that initiated the event
            that the transport sm is in progress for)
            Forward the event to the transport sm (It may be an on abort during on)
        Else
            Forward the event to the pending mngr
        End If
    Else
        Forward the event to the pending mngr       // 1
    End If

    1. The transport is not in progress, but we forward to the pending mngr. This will ensure that
        events are processed in the order of their arrival.         
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandleEvent(    _CcmIm_BtTranMngr_Obj   *thisObj,
                                                            CcmImStackId                stackId,
                                                            _CcmIm_BtTranMngr_Event event)
{
    _CcmImStatus status;

    MCP_LOG_INFO(("_CCM_IM_BtTranMngr_HandleEvent: Received %s for Stack %d", 
                        _CCM_IM_BtTranMngr_DebugEventStr(event), stackId));
    
    if (_CCM_IM_BtTranSm_IsInProgress(thisObj->tranSmObj) == MCP_TRUE)
    {
        /* If the event is from the active stack, forward it to the transport state machine */
        if (thisObj->activeStackId == stackId)
        {
            _CcmIm_BtTranSm_Event   tranSmEvent = _CCM_IM_BtTranMngr_External2TranSmEvent(event);

            MCP_LOG_INFO(("_CCM_IM_BtTranMngr_HandleEvent: Event for active Stack, Forwarding %s To Tran State Machine", 
                            _CCM_IM_BtTranSm_DebugEventStr(tranSmEvent)));
            
            status = _CCM_IM_BtTranSm_HandleEvent(  thisObj->tranSmObj, 
                                                    tranSmEvent,
                                                    NULL);  
        }
        else
        {
            MCP_LOG_INFO(("_CCM_IM_BtTranMngr_HandleEvent: Event for inactive Stack, Forwarding to pending Mngr"));
            
            status = _CCM_IM_BtTranMngr_HandleNonActiveStackEventWhileTranInProgress(thisObj, stackId, event);
        }
    }
    /* Else => Transport is not in progress, forward to the pending mngr to make sure event order is preserved*/
    else
    {
        MCP_LOG_INFO(("_CCM_IM_BtTranMngr_HandleEvent: Tran SM Not in progress, Forwarding to pending Mngr"));
        status = _CCM_IM_BtTranMngr_HandleStackEventWhileTranNotInProgress(thisObj, stackId, event);
    }

    return status;
}


/*
    This function is called when an event is sent by the parent for an inactive stack, and the transport
    is in progress (performing on / on-abort / off for the active stack).

    In this case the following will occur:
    - If the request is for on-abort:
        It means that the same stack must already have a pending-on request on the list. In that case, 
        the pending-on request is removed from the pending list and the abort completes synchronously.
    - Else (it is a on or off request):
        The requesting stack will be added to the end of the list of the pending stacks. 
        The request will be processed once the transport SM completes processing the event of the active 
        stack. In that case the operation completes asynchronously, and PENDING is returned to the parent.
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandleNonActiveStackEventWhileTranInProgress(   
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId,
                                _CcmIm_BtTranMngr_Event event)
{
    _CcmImStatus status;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_HandleNonActiveStackEventWhileTranInProgress");

    switch (event)
    {
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF:
            
            status = _CCM_IM_BtTranMngr_HandleStackBecomesPendingOff(thisObj, stackId);
            
        break;
        
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON:

            status = _CCM_IM_BtTranMngr_HandleStackBecomesPendingOn(thisObj, stackId);
            
        break;
        
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT:
            
            status = _CCM_IM_BtTranMngr_HandlePendingStackOnAbort(thisObj, stackId);
            
        break;

        default:
            MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, ("Unexpected Event: %d", event));
    
    }
    
    MCP_FUNC_END();

    return status;
}

_CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPendingOn(    
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId)
{
    return _CCM_IM_BtTranMngr_HandleStackBecomesPending(thisObj, stackId, _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_ON);
}

_CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPendingOff(   
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId)
{
    return _CCM_IM_BtTranMngr_HandleStackBecomesPending(thisObj, stackId, _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_OFF);
}


/*
    This function is called when an On or Off event is sent to a stack that is not pending.
    The stack must not be in the pending queue (bug otherwise). It should be added to the
    pending queue and its state is either pending-on or pending-off.

    Since the stack will complete the transition to the ON state later (asynchronously), the return status
    is PENDING
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandleStackBecomesPending(  
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                CcmImStackId                            stackId,
                                _CcmIm_BtTranMngr_PendingStackState pendingState)
{
    _CcmImStatus                        status;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_HandleStackBecomesPending");

    status = _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue(thisObj, stackId, pendingState, MCP_TRUE);
    MCP_VERIFY_FATAL((status  == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranMngr_HandleStackBecomesPending: _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue Failed"));

    status = _CCM_IM_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;
}

/*
    The function adds a pending stack to the list. A precondition is that the stack must not
    be on the stack already. The request is added to the list and all applicable request
    information is stored in the pending stack data structure.
*/
_CcmImStatus _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue(    
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                CcmImStackId                            stackId,
                                _CcmIm_BtTranMngr_PendingStackState pendingState,
                                McpBool                             asyncCompletion)
{
    _CcmImStatus                        status;
    McpPoolStatus                       mcpPoolStatus;
    _CcmIm_BtTranMngr_PendingStackData  *pendingStackData;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_InsertStackIntoPendingQueue");

    /* Verify that the stack is not already on the list */
    pendingStackData =  _CCM_IM_BtTranMngr_FindPendingStackData(thisObj, stackId);
    MCP_VERIFY_FATAL((pendingStackData == NULL), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranMngr_InsertStackIntoPendingQueue: Stack Node Already in Queue"));

    /* Allocate memory for the new pending stack data */
    mcpPoolStatus = MCP_POOL_Allocate(&thisObj->pendingStacksPool , (void **)&pendingStackData);
    MCP_VERIFY_FATAL((MCP_POOL_STATUS_SUCCESS == mcpPoolStatus), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("Allocating from Pending Stacks pool Failed"));

    /* Initialize the pending stack data structure */
    
    MCP_DL_LIST_InitializeNode(&pendingStackData->node);
    pendingStackData->stackId = stackId;
    pendingStackData->state = pendingState;
    pendingStackData->asyncCompletion = asyncCompletion;
    
    /* Add it to the tail of the list to preserve the order (FIFO) */
    MCP_DL_LIST_InsertTail(&thisObj->pendingListHead,  &pendingStackData->node);

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    The function removes a pending stack from the list. A precondition is that the stack must
    be on the stack. 
*/
_CcmImStatus _CCM_IM_BtTranMngr_RemoveStackFromPendingQueue(    
                                _CcmIm_BtTranMngr_Obj               *thisObj,
                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData)
{
    _CcmImStatus    status;
    McpPoolStatus   mcpPoolStatus;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_RemoveStackFromPendingQueue");
    
    /* The node is going to become either the active stack or to complete, it will no longer be pending => remove it */
    MCP_DL_LIST_RemoveNode(&pendingStackData->node);

    /* Free memory for the new pending stack data */
    mcpPoolStatus = MCP_POOL_Free(&thisObj->pendingStacksPool , (void **)&pendingStackData);
    MCP_VERIFY_FATAL((MCP_POOL_STATUS_SUCCESS == mcpPoolStatus), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("Freeing from Pending Stacks pool Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when an On-Abort event is sent to a pending stack.
    The stack must be in the pending on state (bug otherwise), and should be removed
    from the pending queue.
    The function completes synchronously so the function returns SUCCESS
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandlePendingStackOnAbort(  
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId)
{
    _CcmImStatus                        status;
    _CcmIm_BtTranMngr_PendingStackData  *pendingStackData;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_HandlePendingStackOnAbort");

    pendingStackData =  _CCM_IM_BtTranMngr_FindPendingStackData(thisObj, stackId);
    MCP_VERIFY_FATAL((pendingStackData != NULL), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranMngr_HandlePendingStackOnAbort: No Matching Stack Node"));

    /* Verify that the stack is in pending on state (only then it is valid to receive an on-abort) */
    MCP_VERIFY_FATAL((pendingStackData->state == _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_ON),
                        _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_HandlePendingStackOnAbort: Invalid Pending Stack State (%d)", pendingStackData->state));

    /* Remove the node  stack returns to the off state */
    status = _CCM_IM_BtTranMngr_RemoveStackFromPendingQueue(thisObj, pendingStackData);
    MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_RemoveStackFromPendingQueue Failed"));

    status = _CCM_IM_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

/*
    This function is called when a stack sends and event while the transport is NOT in progress.

    The pending queue must be empty upon entry. This is because the queue is emptied when 
    the transport completes an on or off process (stops being in progress).

    Thus, as the queue must be empty, the event must be either on or off. On abort is invalid.

    Since the queue is empty, and the transport is NOT in progress
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandleStackEventWhileTranNotInProgress( 
                                _CcmIm_BtTranMngr_Obj   *thisObj,
                                CcmImStackId                stackId,
                                _CcmIm_BtTranMngr_Event event)
{
    _CcmImStatus                        status;
    _CcmIm_BtTranMngr_PendingStackState pendingStackState;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_HandleStackEventWhileTranNotInProgress");

    MCP_VERIFY_FATAL((thisObj->iteratingPendingStacks == MCP_FALSE), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("Iteration over pending stacks is in progress"));
        
    switch (event)
    {
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF:

            pendingStackState = _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_OFF;
            
        break;
        
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON:

            pendingStackState = _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_ON;
            
        break;

        default:
            MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, ("Unexpected Event: %d", event));
    
    }

    status = _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue(thisObj, stackId, pendingStackState, MCP_FALSE);
    MCP_VERIFY_FATAL((status  == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR, 
                        ("_CCM_IM_BtTranMngr_HandleStackBecomesPending: _CCM_IM_BtTranMngr_InsertStackIntoPendingQueue Failed"));

    status = _CCM_IM_BtTranMngr_HandlePendingStacks(thisObj);
    
    MCP_FUNC_END();

    return status;
}

/*
    This function receives notifications from the child transport state machine.

    The notifications indicate that the transport SM completed processing the current
    active stack request. There is no longer an active stack. A new stack will become the
    active stack once the transport SM will receive a new request from this module.
*/
void _CCM_IM_BtTranMngr_TranSmCb(_CcmIm_BtTranSm_CompletionEvent *event)
{
    _CcmIm_BtTranMngr_Obj   *tranMngrObj;
    CcmImStackId                notifyingStackId;
    
    MCP_FUNC_START("_CCM_IM_BtTranMngr_TranOnSmCb");

    tranMngrObj = &_ccmIm_BtTranMngr_Data.tranMngrObjs[event->chipId];

    /* Record the id of the stack that just stopped being the active stack */
    notifyingStackId = tranMngrObj->activeStackId;

    /* Indicate that there is no active stack anymore */
    tranMngrObj->activeStackId = CCM_IM_INVALID_STACK_ID;

    /* Handle the notification according to its type */
    switch (event->eventType)
    {
        case _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_OFF_COMPLETED:
            
            _CCM_IM_BtTranMngr_HandleTranOffCompletedEvent(tranMngrObj, notifyingStackId, event->completionStatus);
            
        break;
        
        case _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_COMPLETED:
            
            _CCM_IM_BtTranMngr_HandleTranOnCompletedEvent(tranMngrObj, notifyingStackId, event->completionStatus);
            
        break;
        
        case _CCM_IM_BT_TRAN_SM_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED:

            _CCM_IM_BtTranMngr_HandleTranOnAbortCompletedEvent(tranMngrObj, notifyingStackId, event->completionStatus);

        break;
        
        default:
            MCP_FATAL_NO_RETVAR(("_CCM_IM_BtTranMngr_TranSmCb: Unexpected Event (%d)", event->eventType));
    }

    _CCM_IM_BtTranMngr_HandlePendingStacks(tranMngrObj);

    MCP_FUNC_END();
}

/*
    This function is called when the transport SM completes handling an Off request
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandleTranOffCompletedEvent(
                    _CcmIm_BtTranMngr_Obj   *thisObj,
                    CcmImStackId                notifyingStackId,
                    _CcmImStatus            completionStatus)
{
    /* The stack must have been in the On state until now => derement the counter */
    --thisObj->numOfOnStacks;
    
    _CCM_IM_BtTranMngr_NotifyStack( thisObj, 
                                        notifyingStackId, 
                                        _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_OFF_COMPLETED,
                                        completionStatus);

    return _CCM_IM_STATUS_SUCCESS;
}


_CcmImStatus _CCM_IM_BtTranMngr_HandleTranOnCompletedEvent(
                    _CcmIm_BtTranMngr_Obj   *thisObj,
                    CcmImStackId                notifyingStackId,
                    _CcmImStatus            completionStatus)
{
    /* The stack must have been in the Off state until now => increment the counter */
    ++thisObj->numOfOnStacks;
    
    _CCM_IM_BtTranMngr_NotifyStack( thisObj, 
                                        notifyingStackId, 
                                        _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_COMPLETED,
                                        completionStatus);

    return _CCM_IM_STATUS_SUCCESS;
}

_CcmImStatus _CCM_IM_BtTranMngr_HandleTranOnAbortCompletedEvent(
                    _CcmIm_BtTranMngr_Obj   *thisObj,
                    CcmImStackId                notifyingStackId,
                    _CcmImStatus            completionStatus)
{
    /* 
        Not decreasing the numOfOnStacks counter since we are increasing it only when the on completes. However,
        the on was aborted => it was not increased
    */
    
    _CCM_IM_BtTranMngr_NotifyStack( thisObj, 
                                        notifyingStackId, 
                                        _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_ABORT_COMPLETED,
                                        completionStatus);

    return _CCM_IM_STATUS_SUCCESS;
}

/*
    This function is called to process requests of pending stacks. It processes the request from the beginning
    of the list to its end. 
*/
_CcmImStatus _CCM_IM_BtTranMngr_HandlePendingStacks(_CcmIm_BtTranMngr_Obj *thisObj)
{
    _CcmImStatus                        status;
    _CcmIm_BtTranMngr_PendingStackData  *currElementPtr;
    _CcmIm_BtTranMngr_PendingStackData  *nextElementPtr;
    _CcmIm_BtTranMngr_PendingStackData  tempStackData;
    
    MCP_FUNC_START("_CCM_IM_BtTranMngr_HandlePendingStacks");

    MCP_VERIFY_FATAL((_CCM_IM_BtTranSm_IsInProgress(thisObj->tranSmObj) == MCP_FALSE), _CCM_IM_STATUS_INTERNAL_ERROR,
                        ("_CCM_IM_BtTranMngr_HandlePendingStacks: Tran SM Is In Progress"));

    /* 
        Protect against a stack event that may cause a new pending stack to be inserted into the queue 
        while we are iterating over existing elements.
    */
    thisObj->iteratingPendingStacks = MCP_TRUE;

    status = _CCM_IM_STATUS_SUCCESS;

    /* Go over all pending stacks, and allow removal of stacks during iteration */
    MCP_DL_LIST_ITERATE_WITH_REMOVAL(   thisObj->pendingListHead, 
                                            currElementPtr, 
                                            nextElementPtr, 
                                            _CcmIm_BtTranMngr_PendingStackData)
    {
        /* 
            Store the stack data in a temp location since the momory is release 
            inside _CCM_IM_BtTranMngr_RemoveStackFromPendingQueue
        */
        tempStackData = *currElementPtr;

        /* Remove the stack from the list and release the occupied memory */
        status = _CCM_IM_BtTranMngr_RemoveStackFromPendingQueue(thisObj, currElementPtr);
        MCP_VERIFY_FATAL((status == _CCM_IM_STATUS_SUCCESS), _CCM_IM_STATUS_INTERNAL_ERROR,
                            ("_CCM_IM_BtTranMngr_RemoveStackFromPendingQueue Failed"));

        /* 
            tempStackData.state indicates the pending request. This may be either On or Off. Abort
            can't be pending since it should have removed an on request from the list and complete
            synchronously (see _CCM_IM_BtTranMngr_HandlePendingStackOnAbort() for details)
        */
        switch (tempStackData.state)
        {
            case _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_ON:

                status = _CCM_IM_BtTranMngr_PendingStackOn(thisObj, &tempStackData);
                    
            break;

            case _CCM_IM_BT_TRAN_MNGR_PENDING_STACK_STATE_OFF:
                
                status = _CCM_IM_BtTranMngr_PendingStackOff(thisObj, &tempStackData);

            break;

            default:
                MCP_FATAL(_CCM_IM_STATUS_INTERNAL_ERROR, ("Invalid Pending State (%d)", tempStackData.state));
        }

        /* 
            If the request of the stack that was just handled is in progress (it actually became the active stack), 
            then we must first wait for the request to complete and only then move to process the next
            stack on the list. Otherwies (request completed synchronously), we can go on and process the next
            pending request
        */
        if (status == _CCM_IM_STATUS_PENDING)
        {
            thisObj->activeStackId = tempStackData.stackId;
            break;
        }
    }

    /* 
        If the first stack is pending, all other pendign stacks didn't complete synchronously =>
        Mark all of them as completing asynchronously
    */
    if (status == _CCM_IM_STATUS_PENDING)
    {       
        MCP_DL_LIST_ITERATE_NO_REMOVAL(thisObj->pendingListHead, currElementPtr, _CcmIm_BtTranMngr_PendingStackData)
        {
            currElementPtr->asyncCompletion = MCP_TRUE;
        }   
    }

    thisObj->iteratingPendingStacks = MCP_FALSE;

    MCP_FUNC_END();

    return status;
}

/*
    This function is called to handle a request for transport on from a stack that was just
    removed from the pending list.

    If this is the first stack to request transport on (the transport is currently off), then the transport 
    state machine should be triggered to process the request. Otherwise, the numOfOnStacks
    is incremented and the request completes.
*/
_CcmImStatus _CCM_IM_BtTranMngr_PendingStackOn( _CcmIm_BtTranMngr_Obj               *thisObj,
                                                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData)
{
    _CcmImStatus    status;


    if (thisObj->numOfOnStacks == 0)
    {
        /* Transport is off - forward the event to the transport SM */
        status = _CCM_IM_BtTranSm_HandleEvent(thisObj->tranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON, NULL);
    }
    else
    {
        status = _CCM_IM_STATUS_SUCCESS;
    }

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        ++thisObj->numOfOnStacks;      
        
        if (pendingStackData->asyncCompletion == MCP_TRUE)
        {
            _CCM_IM_BtTranMngr_NotifyStack( thisObj, 
                                                pendingStackData->stackId, 
                                                _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_ON_COMPLETED,
                                                _CCM_IM_STATUS_SUCCESS);
    
        }
    }

    return status;
}

/*
    This function is called to handle a request for transport off from a stack that was just
    removed from the pending list.

    If this is the last stack to request transport off (the transport is currently on), then the transport 
    state machine should be triggered to process the request. Otherwise, the numOfOnStacks
    is decremented and the request completes.
*/
_CcmImStatus _CCM_IM_BtTranMngr_PendingStackOff(    _CcmIm_BtTranMngr_Obj               *thisObj,
                                                                _CcmIm_BtTranMngr_PendingStackData  *pendingStackData)
{
    _CcmImStatus    status;
    
    if (thisObj->numOfOnStacks == 1)
    {
        status = _CCM_IM_BtTranSm_HandleEvent(thisObj->tranSmObj, _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF, NULL);
    }
    else
    {       
        status = _CCM_IM_STATUS_SUCCESS;
    }

    if (status == _CCM_IM_STATUS_SUCCESS)
    {
        --thisObj->numOfOnStacks;
        
        if (pendingStackData->asyncCompletion == MCP_TRUE)
        {
            _CCM_IM_BtTranMngr_NotifyStack( thisObj, 
                                                pendingStackData->stackId, 
                                                _CCM_IM_BT_TRAN_MNGR_COMPLETED_EVENT_TRAN_OFF_COMPLETED,
                                                _CCM_IM_STATUS_SUCCESS);
    
        }
    }

    return status;
}

_CcmImStatus _CCM_IM_BtTranMngr_NotifyStack(    _CcmIm_BtTranMngr_Obj                   *thisObj, 
                                                        CcmImStackId                                stackId,
                                                        _CcmIm_BtTranMngr_CompletionEventType       eventType,
                                                        _CcmImStatus                            completionStatus)
{
    _CcmIm_BtTranMngr_CompletionEvent       completionEvent;

    completionEvent.chipId = thisObj->chipId;
    completionEvent.stackId = stackId;
    completionEvent.eventType = eventType;
    completionEvent.completionStatus = completionStatus;
    
    (thisObj->parentCb)(&completionEvent);

    return _CCM_IM_STATUS_SUCCESS;
}

_CcmIm_BtTranSm_Event _CCM_IM_BtTranMngr_External2TranSmEvent(_CcmIm_BtTranMngr_Event externalEvent)
{
    _CcmIm_BtTranSm_Event   tranSmEvent;

    MCP_FUNC_START("_CCM_IM_BtTranMngr_External2TranSmEvent");
    
    switch (externalEvent)
    {
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF:
            
            tranSmEvent = _CCM_IM_BT_TRAN_SM_EVENT_TRAN_OFF;
            
        break;
        
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT:

            tranSmEvent = _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON_ABORT;
            
        break;

        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON:

            tranSmEvent = _CCM_IM_BT_TRAN_SM_EVENT_TRAN_ON;
            
        break;

        default:
            MCP_FATAL_SET_RETVAR((tranSmEvent = _CCM_IM_INVALID_BT_TRAN_SM_EVENT), 
                                    ("_CCM_IM_BtTranMngr_External2TranSmEvent: Unexpected Event (%d)", externalEvent));

    }

    MCP_FUNC_END();
    
    return tranSmEvent;
}

/*
    An auxiliary function that checks whether 2 stack data element have the same stack id
*/
McpBool _CCM_IM_BtTranMngr_PendingQueueMatchByStackId(
                const MCP_DL_LIST_Node *nodeToMatch, 
                const MCP_DL_LIST_Node* checkedNode)
{
    const _CcmIm_BtTranMngr_PendingStackData    *stackDataToMatch = (const _CcmIm_BtTranMngr_PendingStackData*)nodeToMatch; 
    const _CcmIm_BtTranMngr_PendingStackData    *stackDataToCheck = (const _CcmIm_BtTranMngr_PendingStackData*)checkedNode; 

    McpBool answer;
    
    if (stackDataToMatch->stackId == stackDataToCheck->stackId)
    {
        answer = MCP_TRUE;
    }
    else
    {
        answer = MCP_FALSE;
    }

    return answer;
}

/*
    A function that searches the pending list for a stack with a matching stack id

    The function  returns a pointer to the matching stack data structure if found and 
    NULL otherwise
*/
_CcmIm_BtTranMngr_PendingStackData *_CCM_IM_BtTranMngr_FindPendingStackData(    
                                        _CcmIm_BtTranMngr_Obj   *thisObj,
                                        CcmImStackId                stackId)
{
    _CcmIm_BtTranMngr_PendingStackData  *pendingStackData;
    _CcmIm_BtTranMngr_PendingStackData  templateStackData;
    
    MCP_FUNC_START("_CCM_IM_BtTranMngr_FindPendingStackData");

    /* Prepare template node that is used to find the match */
    MCP_DL_LIST_InitializeNode(&templateStackData.node);
    templateStackData.stackId = stackId;
    
    pendingStackData =  (_CcmIm_BtTranMngr_PendingStackData*) 
                            MCP_DL_LIST_FindMatchingNode(
                                &thisObj->pendingListHead,
                                NULL,
                                MCP_DL_LIST_SEARCH_DIR_FIRST_TO_LAST,
                                &templateStackData.node, 
                                _CCM_IM_BtTranMngr_PendingQueueMatchByStackId);
        
    MCP_FUNC_END();

    return pendingStackData;
}

void _CCM_IM_BtTranMngr_StaticInitInstances(void)
{
    McpUint chipIdx;
    
    for (chipIdx = 0; chipIdx < MCP_HAL_MAX_NUM_OF_CHIPS; ++chipIdx)
    {
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].chipId = MCP_HAL_INVALID_CHIP_ID;
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].parentCb = NULL;
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].tranSmObj = NULL;
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].activeStackId = CCM_IM_INVALID_STACK_ID;
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].numOfOnStacks = 0;
        _ccmIm_BtTranMngr_Data.tranMngrObjs[chipIdx].iteratingPendingStacks = MCP_FALSE;
    }
}

void _CCM_IM_BtTranMngr_GetChipVersion(_CcmIm_BtTranMngr_Obj   *thisObj,
                                       McpU16 *projectType,
                                       McpU16 *versionMajor,
                                       McpU16 *versionMinor)
{
    MCP_FUNC_START("_CCM_IM_BtTranSm_GetChipVersion");

    _CCM_IM_BtTranSm_GetChipVersion (thisObj->tranSmObj, projectType, versionMajor, versionMinor);

    MCP_FUNC_END();
}


BtHciIfObj *CCM_IM_BtTranMngr_GetBtHciIfObj(_CcmIm_BtTranMngr_Obj   *smData)
{
    return CCM_IM_BtTranSm_GetBtHciIfObj(smData->tranSmObj);
}

const char *_CCM_IM_BtTranMngr_DebugEventStr(_CcmIm_BtTranMngr_Event event)
{
    switch (event)
    {
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_OFF:       return "BT_TRAN_MNGR_EVENT_TRAN_OFF";
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT:      return "BT_TRAN_MNGR_EVENT_TRAN_ON_ABORT";
        case _CCM_IM_BT_TRAN_MNGR_EVENT_TRAN_ON:        return "BT_TRAN_MNGR_EVENT_TRAN_ON";            
        default:                                                return "INVALID BT TRAN SM EVENT";
    }
}

