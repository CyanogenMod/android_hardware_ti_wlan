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

#include "fmc_defs.h"
#include "fmc_utils.h"
#include "fmc_log.h"
#include "fmc_pool.h"
#include "fmc_debug.h"
#include "fmc_config.h"
#include "fmc_commoni.h"
#include "fmc_core.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMCOMMON);

/* Initialization states */
typedef enum _tagFmcState {
    _FMC_INIT_STATE_NOT_INITIALIZED,
    _FMC_INIT_STATE_INIT_FAILED,
    _FMC_INIT_STATE_INITIALIZED,
    _FMC_INIT_STATE_DEINIT_STARTED
} _FmcInitState;

typedef struct {
    /* Counts number of clients (Tx / Rx) */
    FMC_UINT                    clientRefCount;

    /* Shared Mutex for locking FM stack */
    FmcOsSemaphoreHandle          mutexHandle; 

    /* Shared commands Queue */
    FMC_ListNode                    cmdsQueue;

    FmciFmTaskClientCallbackFunc    clientTaskCbFunc;

    /* Allocation of commands memory pool */
    FMC_POOL_DECLARE_POOL(cmdsPool,         /* pool name */
                                        cmdsMemory,     /* pool's memory */
                                        FMC_CONFIG_MAX_NUM_OF_PENDING_CMDS, 
                                        FMC_MAX_CMD_LEN);
} FmcData;

static FmcData _fmcData;

/* Defined outside of FmcData to allow explicit static initialization */
static _FmcInitState _fmcInitState = _FMC_INIT_STATE_NOT_INITIALIZED;

static const char _fmcCmdsPoolName[] = "FmcCmdsPool";

static FMC_U8 _fmTimerHandel;

/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/
FMC_STATIC void _FMC_TaskEventCallback(FmcOsEvent evtMask);
FMC_STATIC FmcStatus _FMC_OsInit(void);
FMC_STATIC FmcStatus _FMC_OsDeinit(void);

FmcStatus FMCI_Init(void)
{
    FmcStatus status = FMC_STATUS_SUCCESS;
    
    FMC_FUNC_START_AND_LOCK("FMCI_Init");

    /* Handle all possible initialization states */
    switch (_fmcInitState)
    {
        case _FMC_INIT_STATE_NOT_INITIALIZED:
            
            /* First Initialization (by RX or TX) - Continue & Initialize*/
            
        break;
            
        case _FMC_INIT_STATE_INIT_FAILED:       

            /* Init already failed and yet another attempt was made */
            FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, ("FMCI_Init: Init already failed, please deinit and retry"));

        case _FMC_INIT_STATE_INITIALIZED:

            /* Double initialization - Nothing left to do, just record the added client, and skip the rest */
            
            ++_fmcData.clientRefCount;
            
            FMC_LOG_INFO(("FMCI_Init: Cleint #%d Initialized - Exiting Successfully",_fmcData.clientRefCount));
            FMC_RET(FMC_STATUS_SUCCESS);

        default:

            /* Invalid state => BUG */
            FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, ("FMCI_Init: Unexpected State (%d)", _fmcInitState));
            
    }

    /* We got here to actually initialize for the first time */
    
    _fmcData.clientRefCount = 1;

    /* Assume failure. If we fail before reaching the end, we will stay in this state */
    _fmcInitState = _FMC_INIT_STATE_INIT_FAILED;
    
    /* Init Common FM RX & TX OS Issues (task, mutex, etc) */
    status = _FMC_OsInit();
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("FMCI_Init: _FMC_OsInit Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));


    /* Create common commands pool */
    status = FMC_POOL_Create(   &_fmcData.cmdsPool,
                                _fmcCmdsPoolName,
                                _fmcData.cmdsMemory, 
                                FMC_CONFIG_MAX_NUM_OF_PENDING_CMDS,
                                FMC_MAX_CMD_LEN);
    FMC_VERIFY_FATAL((FMC_STATUS_SUCCESS == status), FMC_STATUS_INTERNAL_ERROR, 
                        ("FMCI_Init: Cmds pool creation failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

    FMC_InitializeListHead(&_fmcData.cmdsQueue);

    /* Initialize common transport layer */
    status = FMC_CORE_Init();
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), status, ("FMCI_Init"));
    
    /* Initialization completed successfully */
    _fmcInitState = _FMC_INIT_STATE_INITIALIZED;

    FMC_LOG_INFO(("FMCI_Init: FMC Successfully Initialized"));
    
    FMC_FUNC_END_AND_UNLOCK();
    
    return status;
}

FmcStatus FMCI_Deinit(void)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    FMC_U32     numOfAllocatedCmds = 0;

    FMC_FUNC_START_AND_LOCK("FMCI_Deinit");

    /* Update number of remaining clients */
    --_fmcData.clientRefCount;

    /* We actually de-initialize only when the last client calls the function */
    if (_fmcData.clientRefCount > 0)
    {
        FMC_LOG_INFO(("FMCI_Deinit: %d Cleints left to de-initialize, exiting without actually de-initializing",_fmcData.clientRefCount));
        FMC_RET(FMC_STATUS_SUCCESS);
    }

    _fmcInitState =_FMC_INIT_STATE_DEINIT_STARTED;

    /* Last client de-initializing  - Actually de-initialize */
    FMC_LOG_INFO(("FMCI_Deinit: Last Client Deinitializing"));

    /* De-Initialize common transport layer */
    status = FMC_CORE_Deinit();
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
                        ("FMCI_Deinit: FMC_CORE_Deinit Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

    /* Verify that there are no commands pending in the queue at this stage (should have already been handled) */
    status = FMC_POOL_GetNumOfAllocatedElements(&_fmcData.cmdsPool, &numOfAllocatedCmds);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
                        ("FMCI_Deinit: FMC_POOL_GetNumOfAllocatedElements Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));
    FMC_VERIFY_FATAL((numOfAllocatedCmds == 0), FMC_STATUS_INTERNAL_ERROR, 
                        ("FMCI_Deinit: Commands pool has %d allocated commands, Expected to be emmpty", numOfAllocatedCmds));

    /* Free the commands pool */
    status = FMC_POOL_Destroy(&_fmcData.cmdsPool);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
                        ("FMCI_Deinit: FMC_POOL_Destroy Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

    /* Free OS resources */ 
    status = _FMC_OsDeinit();
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
                        ("FMCI_Deinit: _FMC_OsDeinit Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

    _fmcInitState = _FMC_INIT_STATE_NOT_INITIALIZED;
    
    FMC_LOG_INFO(("FMCI_Deinit: FMC Successfully Deinitialized"));
    
    FMC_FUNC_END_AND_UNLOCK();

    return status;
}

FmcStatus FMCI_SetEventCallback(FmciFmTaskClientCallbackFunc eventCbFunc)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    FMC_FUNC_START("FMCI_SetEventCallback");

    FMC_LOG_INFO(("FMCI_SetEventCallback: %s Event Callback Func", ((eventCbFunc != NULL)? ("Setting") : ("Clearing"))));

    /* It is a bug if a client requests to receive task events while another client is already receiving them */
    if (eventCbFunc != NULL)
    {       
        FMC_VERIFY_FATAL((_fmcData.clientTaskCbFunc == NULL), FMC_STATUS_INTERNAL_ERROR,
                            ("FMCI_SetEventCallback: _fmcData.clientTaskCbFunc already set"));
    }

    /* Record the callback to which task events will be directed from now */
    _fmcData.clientTaskCbFunc = eventCbFunc;

    FMC_FUNC_END();
    
    return status;
}

FmcStatus _FMC_OsInit(void)
{
    FmcStatus   status;


    FMC_FUNC_START("_FMC_OsInit");

    /* initialize FMC_OS module */
    status = FMC_OS_Init();
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Initializing FMC OS (%d)", status));

    /* Create FM Mutex */
    status = FMC_OS_CreateSemaphore(FMC_CONFIG_FM_MUTEX_NAME, &_fmcData.mutexHandle);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Creating FM Mutex (%d)", status));

    /* Create FM Task */
    status = FMC_OS_CreateTask(FMC_OS_TASK_HANDLE_FM, 
                               _FMC_TaskEventCallback,
                               FMC_CONFIG_FM_TASK_NAME);
    
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Creating FM Task (%d)", status));

    status = FMC_OS_CreateTimer(FMC_OS_TASK_HANDLE_FM, (const char *)"FM TIMER", &_fmTimerHandel);

    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Creating FM Timer (%d)", status));

    FMC_FUNC_END();

    return status;
}

FmcStatus _FMC_OsDeinit(void)
{
    FmcStatus       status;

    FMC_FUNC_START("_FMC_OsDeinit");
    
    /* Get rid of resources allocated during init */
    status = FMC_OS_DestroySemaphore(_fmcData.mutexHandle);   
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Destroying FM Mutex (%d)", status));

    status = FMC_OS_DestroyTask(FMC_OS_TASK_HANDLE_FM);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR,
                     ("Failed Destroying FM Task (%d)", status));

    status = FMC_OS_DestroyTimer(_fmTimerHandel);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Destroying FM Timer (%d)", status));

    status = FMC_OS_Deinit();
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), FMC_STATUS_INTERNAL_ERROR, 
                     ("Failed Destroying FMC OS module (%d)", status));
    
    FMC_FUNC_END();
    
    return status;
}

void FMCI_NotifyFmTask(FmcOsEvent event)
{
    FmcStatus status;

    FMC_FUNC_START("FMCI_NotifyFmTask");
    
    /* Send a event to the FM task */
    status = FMC_OS_SendEvent(FMC_OS_TASK_HANDLE_FM, event);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FMC_STATUS_SUCCESS), 
                                    ("Failed Sending Event (%d) to FM Task (%d)", 
                                     FMC_OS_TASK_HANDLE_FM, status));

    FMC_FUNC_END();
}

FmcStatus FMCI_OS_ResetTimer(FMC_U32 time)
{
    /* we should convert ms to ticks*/
    return FMC_OS_ResetTimer(_fmTimerHandel, time, FMC_OS_EVENT_TIMER_EXPIRED);
}
FmcStatus FMCI_OS_CancelTimer(void)
{
    return FMC_OS_CancelTimer(_fmTimerHandel);

}

FmcOsSemaphoreHandle FMCI_GetMutex(void)
{
    return _fmcData.mutexHandle;
}

FmcPool *FMCI_GetCmdsPool(void)
{
    return &_fmcData.cmdsPool;
}

FMC_ListNode *FMCI_GetCmdsQueue(void)
{
    return &_fmcData.cmdsQueue;
}

void _FMC_TaskEventCallback(FmcOsEvent evtMask)
{
    /* Lock the Mutex on entry to FM state machine */
    FMC_FUNC_START_AND_LOCK("Fm_Stack_EventCallback");
    if(_fmcInitState !=_FMC_INIT_STATE_DEINIT_STARTED)
    {
        /* There must be a registered callback for the FM task events */
        FMC_VERIFY_FATAL_NO_RETVAR((_fmcData.clientTaskCbFunc != NULL), ("Null clientTaskCbFunc"));

        /* Foraward the event the currently configured event handler (RX or TX) */
        (_fmcData.clientTaskCbFunc)(evtMask);
    }
    FMC_FUNC_END_AND_UNLOCK();
}


