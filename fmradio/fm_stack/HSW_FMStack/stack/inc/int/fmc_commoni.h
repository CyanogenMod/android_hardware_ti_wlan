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
/*******************************************************************************\
*
*   FILE NAME:      fm_commoni.h
*
*   BRIEF:          This file defines Internal common FM types, definitions, and functions.
*
*   DESCRIPTION:    
*
*   FM RX and FM TX each have their own specific files with specific definitions. This file contains
*   definitions that are common to both FM RX & TX. However, these definitions are used internally by
*   the FM stack. They are not to be used outside the stack
*           
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/

#ifndef __FMC_COMMONI_H
#define __FMC_COMMONI_H

#include "fmc_utils.h"
#include "fmc_pool.h"
#include "fmc_common.h"
#include "ccm_imi_common.h"
#include "mcp_hal_misc.h"
#include "fmc_os.h"

/*
[ToDo] - decide the maximum size of a single Tx or RX command. Currently fmc_commoni.h includes Tx-specific definitions
*/
typedef struct  {
    FMC_ListNode        node;
    void                *context;
    FMC_UINT        cmdType;
} FmcBaseCmd;

#define FMC_CMD_TYPE_LEN                sizeof(FMC_UINT)
#define FMC_BASE_CMD_LEN                sizeof(FmcBaseCmd)
#define FMC_MAX_CMD_PARMS_LEN       (8*sizeof(FMC_U8))/*FmRxSetAudioTargetCmd is the largest command*/

#define FMC_MAX_CMD_LEN             (FMC_UINT)(FMC_BASE_CMD_LEN + FMC_MAX_CMD_PARMS_LEN)

#define FMC_RDS_FIFO_SIZE               (FM_RDS_RAW_MAX_MSG_LEN)

typedef void (*FmciFmTaskClientCallbackFunc)(FmcOsEvent osEvent);

/* 
    Initialize the Common FMC module.

    The function may be called more than once to allow TX & RX to be independent. However,
    FMCI_Deinit must be called the same number of times.
*/
FmcStatus FMCI_Init(void);

/*
    Deinitializes the common FM module. 

    The function should be called once per call of FMCI_Init(). Once the 
    last client de-initializes it actually de-initializes the module.
*/
FmcStatus FMCI_Deinit(void);

/*
    Only single client at a time may be active (RX or TX). That client should cal this function 
    and provide a callback function. The FM task main routine that is implemented by the common
    module forwards all FM task events to the active client via the callback.
*/
FmcStatus FMCI_SetEventCallback(FmciFmTaskClientCallbackFunc eventCbFunc);

/*
    Sends a FMC_OS_TASK_HANDLE_FM notification to the FM task
*/
void FMCI_NotifyFmTask(FmcOsEvent event);

/*
    Returns the handle of the shared FM mutex.

    The function is used by FMC_OS_LockSemaphore() and FMC_OS_UnlockSemaphore
*/
FmcOsSemaphoreHandle FMCI_GetMutex(void);

/*
    Returns the shared commands pool.

    The function is used when FM Tx or Rx need to allocate and free space for their commands 
*/
FmcPool *FMCI_GetCmdsPool(void);

/*
    Returns the shared commands queue.

    The function is used when FM Tx or Rx need to access the queue (e.g., to insert commands)
*/
FMC_ListNode *FMCI_GetCmdsQueue(void);

FmcStatus FMCI_OS_ResetTimer(FMC_U32 time);

FmcStatus FMCI_OS_CancelTimer(void);



#endif  /* __FMC_COMMONI_H */

