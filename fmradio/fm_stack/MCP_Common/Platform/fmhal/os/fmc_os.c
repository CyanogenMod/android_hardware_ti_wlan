/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010 Sony Ericsson Mobile Communications AB.
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


#define _GNU_SOURCE /* needed for PTHREAD_MUTEX_RECURSIVE */

#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "fm_trace.h"
#include "fmc_os.h"
#include "mcp_hal_string.h"
#include "mcp_hal_memory.h"
#include "mcp_hal_misc.h"
#include "mcp_hal_log.h"
#include "mcp_hal_os.h"
#include "fmc_types.h"
#include "fmc_defs.h"
#include "fmc_log.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMOS);

#if FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED


/*
    The function does the following:

    Calls the App callback to report error to the application. 
    */
extern void send_fm_error_statusToApp();
/* Stack task */
#define Fmc_OS_TASK_HANDLE_STACK								(0x00)
/* macros and constants */
#define FMC_OS_ASSERT(condition)                       \
        (((condition) != 0) ? (void)0 : FMC_OS_Assert(#condition, __FILE__, (FMC_U16)__LINE__))

/* data type definitions */
typedef struct _FMC_OS_TASK_PARAMS
{
	/* Handle to the thread */
	pthread_t threadHandle;
	/* Unix domain socket for the listener */
    /* Event Mask used as condition for synchronization */
    FmcOsEvent evtMask;

    /* Mutex used for the protecting the condition 'evtMask' */
    pthread_mutex_t  evtMaskMutex;

    /* Condition variable used for the signaling when the condition 'evtMask' changed */
    pthread_cond_t evtMaskCond;

	/* handle to the task */
	FmcOsTaskHandle taskHandle;       
    
	/* Tracks whether the task is running */
	FMC_BOOL taskRunning;                

	/* Callbacks provided by the task */
	FmcOsEventCallback taskCallback;             

} FMC_OS_TASK_PARAMS;

typedef struct _FMC_OS_TIMER_PARAMS
{
	/* Timer event sent to the task */
	FmcOsEvent timerEvent;

	/* Identifier for the timer events */ 
	timer_t timerId;
	int timerUsed;

	/* associate timerHandle to taskHandle (used in FmcTimerFired()) */
	FmcOsTaskHandle taskHandle;

} FMC_OS_TIMER_PARAMS;

typedef struct _FMC_OS_HCI_FILTER {

        uint32_t type_mask;

        uint32_t event_mask[2];

        uint16_t opcode;

}FMC_OS_HCI_FILTER;

/* Global variables */
FMC_OS_TASK_PARAMS          fmParams;
FMC_OS_TASK_PARAMS          fmParams2;
uint16_t g_current_request_opcode;
pthread_mutex_t g_current_request_opcode_guard = PTHREAD_MUTEX_INITIALIZER;
/* FM task parameters */
FMC_OS_TIMER_PARAMS         timerParams[ FMHAL_OS_MAX_NUM_OF_TIMERS ]; /* timers storage */
/* Handles to the semaphores */
pthread_mutex_t* semaphores_ptr[FMHAL_OS_MAX_NUM_OF_SEMAPHORES];
pthread_mutex_t  semaphores[FMHAL_OS_MAX_NUM_OF_SEMAPHORES];

#define FMC_OS_LARGEST_TASK_HANDLE FMC_OS_TASK_HANDLE_FM

static void* FmStackThread(void* param);
static void *fm_wait_for_interrupt_thread(void *dev);
static void TimerHandlerFunc(union sigval val);

static void initTaskParms(FMC_OS_TASK_PARAMS* params)
{
    int rc;

    params->evtMask = 0;
    params->taskHandle = 0;
    params->taskRunning = FMC_FALSE;
    params->threadHandle = 0;
    params->taskCallback = 0;

    rc = pthread_mutex_init(&params->evtMaskMutex, NULL);
    FM_ASSERT(rc == 0);

    rc = pthread_cond_init(&params->evtMaskCond, NULL);
    FM_ASSERT(rc == 0);
}

static void destroyTaskParms(FMC_OS_TASK_PARAMS* params)
{
    int rc;

    rc = pthread_mutex_destroy(&params->evtMaskMutex);
    FM_ASSERT(rc == 0);

    rc = pthread_cond_destroy(&params->evtMaskCond);
    FM_ASSERT(rc == 0);
}

static inline void hci_set_bit(int nr, void *addr)
{
        *((uint32_t *) addr + (nr >> 5)) |= (1 << (nr & 31));
}

static inline void hci_filter_clear(FMC_OS_HCI_FILTER *f)
{
        memset(f, 0, sizeof(*f));
}

static inline void hci_filter_set_ptype(int t, FMC_OS_HCI_FILTER *f)
{
        hci_set_bit((t == HCI_VENDOR_PKT) ? 0 : (t & HCI_FLT_TYPE_BITS), &f->type_mask);
}

static inline void hci_filter_set_event(int e, FMC_OS_HCI_FILTER *f)
{
        hci_set_bit((e & HCI_FLT_EVENT_BITS), &f->event_mask);
}

/*-------------------------------------------------------------------------------
 * FMC_OS_Init()
 *
 */
FmcStatus FMC_OS_Init(void)
{
	FMC_U32     idx;
	FmcStatus ret = FMC_STATUS_SUCCESS;

	FM_BEGIN();

	/* init FM task parameters */
    initTaskParms(&fmParams);

	/* init timer handles array */
	for (idx = 0; idx < FMHAL_OS_MAX_NUM_OF_TIMERS; idx++)
	{
		timerParams[idx].timerUsed = 0;
		timerParams[ idx ].timerId = 0;
		timerParams[ idx ].taskHandle = 0;
		timerParams[ idx ].timerEvent = 0;
	}

	/* init semaphore handles array */
	for (idx=0; idx < FMHAL_OS_MAX_NUM_OF_SEMAPHORES; idx++)
	{
		semaphores_ptr[idx] = NULL;
	}

	goto out;

out:
	FM_END();
	return ret;    
}


/*-------------------------------------------------------------------------------
 * FMC_OS_Deinit()
 *
 */
FmcStatus FMC_OS_Deinit(void)
{
	FM_BEGIN();

	FM_TRACE("FMHAL_OS_Deinit executing\n");
	
    destroyTaskParms(&fmParams);

	FM_END();
	return FMC_STATUS_SUCCESS;
}


/*-------------------------------------------------------------------------------
 * FMC_OS_CreateTask()
 *
 */
FmcStatus FMC_OS_CreateTask(FmcOsTaskHandle taskHandle, 
                            FmcOsEventCallback callback, 
                            const char *taskName)
{
	pthread_t threadHandle = 0;	/* thread Id */
	int rc=-1;
	FmcStatus ret;
	pthread_attr_t StackThreadAttr,IntThreadAttr;
	void *status;
	FM_BEGIN();

	FMC_UNUSED_PARAMETER(taskName);

	FM_ASSERT (taskHandle == FMC_OS_TASK_HANDLE_FM);

	/* verify the desired task is not already running */
	if (fmParams.taskRunning == FMC_TRUE ||
			fmParams2.taskRunning == FMC_TRUE)
	{
		ret = FMC_STATUS_FAILED;
		goto out;
	}    
    
	/* 
	* set task callback and handle - must be done before task is started to have this 
	* info ready for it
	*/
	fmParams.taskHandle = taskHandle;
	fmParams.taskCallback = callback;

	/* mark that the desired task is now running */
	fmParams.taskRunning = FMC_TRUE;
	fmParams2.taskRunning = FMC_TRUE;

	/*
	 * Create thread joinable, so we can wait for its completion.
	 */
	if(pthread_attr_init(&StackThreadAttr) != 0)
	{
		goto err_stackattr;
	}

	if(pthread_attr_setdetachstate(&StackThreadAttr,
					PTHREAD_CREATE_JOINABLE) != 0)
	{
		goto err_stackdetach;
	}

	/* and finally create the task */
	rc = pthread_create(&threadHandle, // thread structure
				&StackThreadAttr,
				FmStackThread, // main function
				NULL);
	if(0 != rc)
	{
		FM_TRACE("FMHAL_OS_CreateTask | pthread_create() failed to create 1st FM thread: %s", strerror(errno));
		goto err_stackcreate;
	}

	FM_TRACE("Created 1st FM thread, id = %u", threadHandle);

	fmParams.threadHandle = threadHandle;

	fmParams2.taskHandle = taskHandle;
	fmParams2.taskCallback = NULL;

	/*
	 * Create thread joinable, so we can wait for its completion.
	 */
	if(pthread_attr_init(&IntThreadAttr) != 0)
	{
		goto err_intattr;
	}

	if(pthread_attr_setdetachstate(&IntThreadAttr,
					PTHREAD_CREATE_JOINABLE) != 0)
	{
		goto err_intdetach;
	}

	rc = pthread_create(&threadHandle, // thread structure
				&IntThreadAttr,
				fm_wait_for_interrupt_thread,
				NULL);
	if(0 != rc)
	{
		FM_TRACE("FMHAL_OS_CreateTask | pthread_create() failed to create 2nd FM thread: %s", strerror(errno));
		goto err_intcreate;
	}

	FM_TRACE("Created 2nd FM thread, id = %u", threadHandle);

	fmParams2.threadHandle = threadHandle;

	pthread_attr_destroy(&StackThreadAttr);
	pthread_attr_destroy(&IntThreadAttr);

	ret = FMC_STATUS_SUCCESS;
	goto out;

err_intcreate:
err_intdetach:
	pthread_attr_destroy(&IntThreadAttr);

err_intattr:
	fmParams.taskRunning = FMC_FALSE;
	FMC_OS_SendEvent(fmParams.taskHandle,
			 FMC_OS_INTERNAL_TERMINATE_STACKTHREAD);
	pthread_join(fmParams.threadHandle,&status);

err_stackcreate:
err_stackdetach:
	pthread_attr_destroy(&StackThreadAttr);

err_stackattr:
	/* failed to create the task */
	fmParams.taskRunning = FMC_FALSE;
	fmParams2.taskRunning = FMC_FALSE;
	ret = FMC_STATUS_FAILED;
out:
	FM_END();
	return ret;
}


/*-------------------------------------------------------------------------------
 * FMC_OS_DestroyTask()
 *
 */
FmcStatus FMC_OS_DestroyTask(FmcOsTaskHandle taskHandle)
{
	void *status;

	FM_BEGIN();

	FM_TRACE("FMHAL_OS_DestroyTask: killing task %d", taskHandle);
    
	FM_ASSERT (taskHandle == FMC_OS_TASK_HANDLE_FM);

	fmParams.taskRunning = FMC_FALSE;
	fmParams2.taskRunning = FMC_FALSE;

	/*
	 * Wait for work threads to terminate.
	 */
	FMC_OS_SendEvent(fmParams.taskHandle,
			 FMC_OS_INTERNAL_TERMINATE_STACKTHREAD);

	if(pthread_join(fmParams.threadHandle,&status) != 0)
	{
		FM_TRACE("FMHAL_OS_DestroyTask: join() failed on stack \
			  thread. Err: %s", strerror(errno));
	}

	if(pthread_join(fmParams2.threadHandle,&status) != 0)
	{
		FM_TRACE("FMHAL_OS_DestroyTask: join() failed on interrupt \
			  thread. Err: %s", strerror(errno));
	}

	FM_END();
	return FMC_STATUS_SUCCESS;
}

/*-------------------------------------------------------------------------------
 * FMC_OS_CreateSemaphore()
 *
 */
FmcStatus FMC_OS_CreateSemaphore(const char *semaphoreName, 
                                 FmcOsSemaphoreHandle *semaphoreHandle)
{
	int rc;
	FMC_U32 idx;
	pthread_mutexattr_t mutex_attr;
	FM_BEGIN();
	
	FMC_UNUSED_PARAMETER(semaphoreName);
		
	/* we use recursive mutex - a mutex owner can relock the mutex. to release it,
	     the mutex shall be unlocked as many times as it was locked
	  */
	for (idx=0; idx < FMHAL_OS_MAX_NUM_OF_SEMAPHORES; idx++)
	{
		if (semaphores_ptr[idx] == NULL)
		{
			break;
		}
	}
	
        if (idx == FMHAL_OS_MAX_NUM_OF_SEMAPHORES)
	{
		/* exceeds maximum of available handles */
		return FMC_STATUS_FAILED;
	}

	semaphores_ptr[idx] = &semaphores[idx];	

	rc = pthread_mutexattr_init(&mutex_attr);
	if(rc != 0)
	{
		FM_TRACE("FMC_OS_CreateSemaphore | pthread_mutexattr_init() failed: %s", strerror(rc));
		return FMC_STATUS_FAILED;
	}
	
	rc = pthread_mutexattr_settype( &mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	if(rc != 0)
	{
		FM_TRACE("FMC_OS_CreateSemaphore | pthread_mutexattr_settype() failed: %s", strerror(rc));
		pthread_mutexattr_destroy(&mutex_attr);
		return FMC_STATUS_FAILED;
	}
		
	rc = pthread_mutex_init(&semaphores[idx], &mutex_attr);
	if(rc != 0)
	{
		FM_TRACE("FMC_OS_CreateSemaphore | pthread_mutex_init() failed: %s", strerror(rc));
		pthread_mutexattr_destroy(&mutex_attr);
		return FMC_STATUS_FAILED;
	}
	
	rc = pthread_mutexattr_destroy(&mutex_attr);
	if(rc != 0)
	{
		FM_TRACE("FMC_OS_CreateSemaphore | pthread_mutexattr_destroy() failed: %s", strerror(rc));
		pthread_mutex_destroy(&semaphores[idx]);
		return FMC_STATUS_FAILED;
	}
	*semaphoreHandle = (FmcOsSemaphoreHandle)idx;
			
	FM_END();	
	return FMC_STATUS_SUCCESS;
}

/*-------------------------------------------------------------------------------
 * FMC_OS_DestroySemaphore()
 *
 */
FmcStatus FMC_OS_DestroySemaphore(FmcOsSemaphoreHandle semaphoreHandle)
{
	int rc;
	FM_BEGIN();

	FM_TRACE("FMC_OS_DestroySemaphore: killing semaphore");
	
	FM_ASSERT(semaphoreHandle < FMHAL_OS_MAX_NUM_OF_SEMAPHORES);

	if (semaphores_ptr[semaphoreHandle] == NULL)
	{
		/* mutex does not exists */
		return FMC_STATUS_FAILED;
	}

	rc = pthread_mutex_destroy(semaphores_ptr[semaphoreHandle]);
	semaphores_ptr[semaphoreHandle] = NULL;
	if (rc != 0)
	{
		FM_TRACE("FMC_OS_DestroySemaphore | pthread_mutex_destroy() failed: %s", strerror(rc));
		return FMC_STATUS_FAILED;
	}

	FM_END();
	return FMC_STATUS_SUCCESS;
}


FmcStatus FMC_OS_LockSemaphore(FmcOsSemaphoreHandle semaphore)
{
	int rc;
	pthread_mutex_t * mutex;
	
	FM_ASSERT(semaphore < FMHAL_OS_MAX_NUM_OF_SEMAPHORES);
	mutex = semaphores_ptr[semaphore];
	
	if (mutex == NULL)
	{
		/* mutex does not exists */
		FM_TRACE(("FMHAL_OS_LockSemaphore: failed: mutex is NULL!"));
		return FMC_STATUS_FAILED;
	}


	rc = pthread_mutex_lock(mutex);
	if (rc != 0)
	{
		FM_TRACE("FMC_OS_LockSemaphore: pthread_mutex_lock() failed: %s", strerror(rc));
		return FMC_STATUS_FAILED;

	}

	return FMC_STATUS_SUCCESS;
}

FmcStatus FMC_OS_UnlockSemaphore(FmcOsSemaphoreHandle semaphore)
{
	int rc;
	pthread_mutex_t* mutex; 

	FM_ASSERT(semaphore < FMHAL_OS_MAX_NUM_OF_SEMAPHORES);
	mutex = semaphores_ptr[semaphore];

	if (mutex == NULL)
	{
		FM_TRACE(("FMHAL_OS_UnLockSemaphore: failed: mutex is NULL!"));
		/* mutex does not exist */
		return FMC_STATUS_FAILED;
	}
	rc = pthread_mutex_unlock(mutex);
	if(rc != 0)
	{
		FM_TRACE("FMC_OS_UnlockSemaphore: pthread_mutex_unlock() failed: %s", strerror(rc));
		return FMC_STATUS_FAILED;
	}

	return FMC_STATUS_SUCCESS;

}

/*-------------------------------------------------------------------------------
 * FMC_OS_CreateTimer()
 * note: timer event is created in FMC_OS_Init(). 
 */
FmcStatus FMC_OS_CreateTimer(FmcOsTaskHandle taskHandle, 
                             const char *timerName, 
                             FmcOsTimerHandle *timerHandle)
{
    FMC_U32 idx;
	struct sigevent evp;
	int rc;
	
    FMC_UNUSED_PARAMETER(timerName);

	bzero((void*)&evp, sizeof(evp));

    for (idx = 0; idx < FMHAL_OS_MAX_NUM_OF_TIMERS; idx++)
    {
        if (timerParams[ idx ].timerUsed == 0)
        {
            break;
        }
    }
    
    if (idx == FMHAL_OS_MAX_NUM_OF_TIMERS)
    {
        /* exceeds maximum of available timers */
        return FMC_STATUS_FAILED;
    }

	*timerHandle = (FmcOsTimerHandle)idx;

	/* mark as used (when timer started, will hold timerId) */
	timerParams[idx].timerUsed = 1;
	timerParams[idx].timerId = 0; /* nonexistent timer id */
	
	/* associate timerHandle to taskHandle (used in TimerHandler()) */
	timerParams[idx].taskHandle = taskHandle;
 	timerParams[idx].timerEvent = 7; /* nonexistent event */

    // Create the timer
    evp.sigev_value.sival_int = *timerHandle; // pass the handle as argument
    evp.sigev_notify_function = TimerHandlerFunc;
    evp.sigev_notify_attributes = NULL;
    evp.sigev_notify = SIGEV_THREAD;

    rc = timer_create(CLOCK_REALTIME, &evp, &timerParams[idx].timerId);
	//printf("Create Timer: %s, handle=%d, timerId=%d\n", timerName, idx, timerParams[idx].timerId);
	if (rc != 0)
	{
		timerParams[idx].timerUsed = 0;
		FM_TRACE("FMC_OS_CreateTimer: timer_create() failed: %s", strerror(errno));
		return FMC_STATUS_FAILED;
	}

    return FMC_STATUS_SUCCESS;
}


/*-------------------------------------------------------------------------------
 * FMC_OS_DestroyTimer()
 *
 */
FmcStatus FMC_OS_DestroyTimer (FmcOsTimerHandle timerHandle)
{
	int rc;

	//printf("FMCOS | FMC_OS_DestroyTimer | timerHandle = %d\n", timerHandle);
	FM_ASSERT(timerHandle < FMHAL_OS_MAX_NUM_OF_TIMERS);
	FM_ASSERT(timerParams[timerHandle].timerUsed == 1);
		
	rc = timer_delete(timerParams[timerHandle].timerId);
	//printf("FMCOS | FMC_OS_DestroyTimer | rc = %d\n", rc);
	if (rc == 0)
	{
		timerParams[timerHandle].timerId = 0;
		timerParams[timerHandle].timerUsed = 0;
		return FMC_STATUS_SUCCESS;
	}

	FM_TRACE("FMC_OS_DestroyTimer | timer_delete() failed: %s", strerror(errno));
    return FMC_STATUS_FAILED;
}


/*-------------------------------------------------------------------------------
 * FMC_OS_ResetTimer()
 *
 */
FmcStatus FMC_OS_ResetTimer(FmcOsTimerHandle timerHandle, 
                            FmcOsTime time, 
                            FmcOsEvent evt)
{
	timer_t timerId;
	struct itimerspec ts;
	long rc;
	
	//printf("Reset Timer: %d, t=%d, eidx=%d\n", timerHandle, time, eventIdx);
	
	FM_ASSERT(timerHandle < FMHAL_OS_MAX_NUM_OF_TIMERS);
	FM_ASSERT(timerParams[timerHandle].timerUsed == 1);
	if (timerParams[timerHandle].timerId == 0){
		return FMC_STATUS_FAILED;
	}

	timerId = timerParams[timerHandle].timerId;
	//printf("FMC_OS_ResetTimer | timerHandle = %d , timerId = %d\n", timerHandle, timerId);
	
	timerParams[timerHandle].timerEvent = evt;

	bzero(&ts, sizeof(struct itimerspec));
	ts.it_value.tv_sec = time ? time / 1000 : 0;
	ts.it_value.tv_nsec = time ? (time % 1000) * 1000 * 1000 : 0;  //from mili to nano
	ts.it_interval.tv_sec = 0; // non-periodic
	ts.it_interval.tv_nsec = 0;	
	rc = timer_settime (timerId, 0, &ts, NULL);

	if (rc == 0)
	{
		return FMC_STATUS_SUCCESS;
	}
	else
	{
		FM_TRACE("FMC_OS_ResetTimer: timer_settime() failed: %s", strerror(errno));
		return FMC_STATUS_FAILED;
	}
}


/*-------------------------------------------------------------------------------
 * FMC_OS_CancelTimer()
 *
 */
FmcStatus FMC_OS_CancelTimer(FmcOsTimerHandle timerHandle)
{
	//disarm timer using 0 as expiration value
	return FMC_OS_ResetTimer(timerHandle, 0, 0xFF);
}

/*---------------------------------------------------------------------------
 * Called by the POSIX timer to indicate that the timer fired.
 * value is the index of the timer in the timers array
 */

static void TimerHandlerFunc(union sigval val)
{
	int idx = val.sival_int;

	FM_ASSERT(idx < FMHAL_OS_MAX_NUM_OF_TIMERS);

    FMC_OS_SendEvent(timerParams[idx].taskHandle, timerParams[idx].timerEvent);

}

/*-------------------------------------------------------------------------------
 * FMC_OS_SendEvent()
 *
 */
FmcStatus FMC_OS_SendEvent(FmcOsTaskHandle taskHandle, FmcOsEvent evt)
{
    int rc;
    FMC_BOOL evtMaskChanged = FMC_FALSE;

	/* sanity check */
	FM_ASSERT(taskHandle <= FMC_OS_LARGEST_TASK_HANDLE);

    rc = pthread_mutex_lock(&fmParams.evtMaskMutex);
    FM_ASSERT(rc == 0);

    if ((fmParams.evtMask & evt) == 0)
    {
        fmParams.evtMask |= evt;
        evtMaskChanged = FMC_TRUE;
    }

    rc = pthread_mutex_unlock(&fmParams.evtMaskMutex);
    FM_ASSERT(rc == 0);

    if (evtMaskChanged == FMC_TRUE)
    {
        rc = pthread_cond_signal(&fmParams.evtMaskCond);
        FM_ASSERT(rc == 0);
    }
    
    return FMC_STATUS_SUCCESS;
}
#if ASSERTION_DEBUG /* Debug enabled, will crash on assertion */
void FMC_OS_Assert(const char*expression, const char*file, FMC_U16 line)
{
    MCP_HAL_MISC_Assert(expression, file, line);
}
#else  /* No Debug, will just print error on assertion */
void FMC_OS_Assert(const char*expression, const char*file, FMC_U16 line)
{
    FM_TRACE("FMC_OS_Assert");
    MCP_HAL_LOG_ERROR( file, line,MCP_HAL_LOG_MODULE_TYPE_ASSERT,(expression));
}


void FMC_OS_APPCB()
{
	FM_TRACE("FMC_OS_APPCB");
	/*Post the error to the upper layer*/
	send_fm_error_statusToApp();
}
#endif
void FMC_OS_MemCopy(void *dest, const void *source, FMC_U32 numBytes)
{
    MCP_HAL_MEMORY_MemCopy(dest, source, numBytes);
}

FMC_BOOL FMC_OS_MemCmp(const void *buffer1, FMC_U16 len1, const void *buffer2, FMC_U16 len2)
{
    return MCP_HAL_MEMORY_MemCmp(buffer1, len1,buffer2, len2);
}

void FMC_OS_MemSet(void *dest, FMC_U8 byte, FMC_U32 len)
{
    MCP_HAL_MEMORY_MemSet(dest, byte, len);
}

FMC_U8 FMC_OS_StrCmp(const FMC_U8*Str1, const FMC_U8*Str2)
{
    return MCP_HAL_STRING_StrCmp((const char *)Str1, (const char *)Str2);
}

FMC_U8 FMC_OS_StriCmp(const FMC_U8*Str1, const FMC_U8*Str2)
{
    return MCP_HAL_STRING_StriCmp((const char *)Str1, (const char *)Str2);
}

FMC_U16 FMC_OS_StrLen(const FMC_U8 *Str)
{
    return (FMC_U16)MCP_HAL_STRING_StrLen((const char *)Str);
}

FMC_U8* FMC_OS_StrCpy(FMC_U8* StrDest, const FMC_U8*StrSource)
{
    return (FMC_U8*)MCP_HAL_STRING_StrCpy((char*)StrDest, (const char *)StrSource);
 }

FMC_U8* FMC_OS_StrnCpy(FMC_U8*  StrDest, const FMC_U8*StrSource, FMC_U32 Count)
{
    return (FMC_U8*)MCP_HAL_STRING_StrnCpy((char *)StrDest, (const char *)StrSource, Count);
}

FMC_U8 FMC_OS_Sleep(FMC_U32 time)
{
    return (FMC_U8)MCP_HAL_OS_Sleep(time);
}

/*
FM Main thread.
 listens for events, and calls the callback
*/
static void * FmStackThread(void* param)
{
	int rc;
	FmcOsEventCallback callback;
    FmcOsEvent evtMask = 0;
	FMC_UNUSED_PARAMETER(param);
    
	FM_BEGIN_L(HIGHEST_TRACE_LVL);
FM_TRACE("FmStackThread");
	
	/* set self priority to 0.
	   The UART threads are set to -1, stack threads are set to 0, and the
	   application thread will be set to 1 */
	if (-1 == setpriority(PRIO_PROCESS, 0, 0))
	{
		FM_TRACE("FMHAL GenericThread: failed to set priority: %s", strerror(errno));
		FM_ASSERT(0);
	}

	callback = fmParams.taskCallback;

	/*sock was already created */
	FM_ASSERT(callback != NULL);

	/* Stay in this thread while still connected */
	while (fmParams.taskRunning) {

        rc = pthread_mutex_lock(&fmParams.evtMaskMutex);
        FM_ASSERT(rc == 0);

		/* First check if we already received any signal */
        if (fmParams.evtMask == 0) {
        	/* No signal yet => block until we get a signal */
        	rc = pthread_cond_wait(&fmParams.evtMaskCond, &fmParams.evtMaskMutex);
			FM_ASSERT(rc == 0);
        }

        /* Got a signal, copy evtMask to local variable, and clear evtMask */
        evtMask = fmParams.evtMask;
        fmParams.evtMask = 0;

        rc = pthread_mutex_unlock(&fmParams.evtMaskMutex);
        FM_ASSERT(rc == 0);

        FM_ASSERT(callback != NULL);
        if(evtMask != FMC_OS_INTERNAL_TERMINATE_STACKTHREAD)
        {
            callback(evtMask);
        }
	}

	FM_TRACE("FMHALOS | GenericThread | thread id %u is now terminating", pthread_self());
	FM_END();
	return 0;
}

/*
 * FM Stack's second thread.
 * This thread waits for fm interrupts.
 * upon Invokation, it should receive the hci device
 * number on which to attach its socket.
 */
static void *fm_wait_for_interrupt_thread(void *dev)
{
#define HCI_FM_EVENT 0xF0

	int dd, ret = FMC_STATUS_SUCCESS, len;
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	FMC_U16 opcode;
	FMC_OS_HCI_FILTER nf;
	struct sockaddr_hci addr;
	FMC_U8 pktType, evtType;

	FM_BEGIN_L(HIGHEST_TRACE_LVL);


	FM_TRACE("@@@@@@@@@@fm_wait_for_interrupt_thread @@@@@@@@" ); 

	/* Create HCI socket */
	dd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);

	if (dd < 0) {
		FM_ERROR_SYS("Can't create raw socket" );
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	/* Bind socket to the HCI device */
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = 0;
	if (bind(dd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		FMC_LOG_ERROR(("Can't bind to device hci%d", 0));
		ret = FMC_STATUS_FAILED;
		goto close;
	}

	/*
	 * Our interrupt event is 0xF0.
	 * BlueZ filters out two most-significant bits of the
	 * event type, which makes us wait for event 0x48.
	 * It is still ok since 0x48 does not exists as an event, too.
	 * to be 100% we verify the event type upon arrival
	 */

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT,  &nf);
	hci_filter_set_event(EVT_CMD_COMPLETE, &nf);
	hci_filter_set_event(HCI_FM_EVENT, &nf);
	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		FM_TRACE("failed to set socket options" ); 
		FM_ERROR_SYS("failed to set socket options");
		ret = FMC_STATUS_FAILED;
		goto close;
	}

	/* continue running as long as not terminated */
	while (fmParams2.taskRunning) {
		struct pollfd p;
		int n;

		p.fd = dd; p.events = POLLIN;

		/* poll socket, wait for fm interrupts */
		while ((n = poll(&p, 1, 500)) == -1) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			FM_ERROR_SYS("failed to poll socket");
			FM_TRACE("failed to poll socket" ); 
			ret = FMC_STATUS_FAILED;
			goto close;
		}

		/* we timeout once in a while */
		/* this let's us the chance to check if we were terminated */
		if (0 == n)
			continue;

		/* read newly arrived data */
		/* TBD: rethink assumption about single arrival */
		memset(buf, 0 , sizeof(buf));
		while ((len = read(dd, buf, sizeof(buf))) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			FM_ERROR_SYS("failed to read socket");
			FM_TRACE("failed to read socket" ); 
			ret = FMC_STATUS_FAILED;
			goto close;
		}

		if (len == 0)
			continue;

		pktType = *buf;
		if (pktType != HCI_EVENT_PKT)
			continue;

		evtType = *(buf + 1);
		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		switch (evtType) {
			case HCI_FM_EVENT:
				/* notify FM stack that an interrupt has arrived */
				if (fmParams2.taskCallback)
				{

					FM_TRACE("@@@@@@@@@@fm_wait_for_interrupt_thread call the callback@@@@@@@@" ); 
					fmParams2.taskCallback(0);/* the param is ignored */
				}
				break;
			case EVT_CMD_COMPLETE:
				memcpy(&opcode, ptr + 1, sizeof(opcode));

				/* Protect against races on g_current_request_opcode */
				FMC_ASSERT(0 == pthread_mutex_lock(&g_current_request_opcode_guard));
				if (btohs(opcode) != g_current_request_opcode) {
					FMC_ASSERT(0 == pthread_mutex_unlock(&g_current_request_opcode_guard));
					break;
				}
				g_current_request_opcode = 0;
				FMC_ASSERT(0 == pthread_mutex_unlock(&g_current_request_opcode_guard));

				ptr += EVT_CMD_COMPLETE_SIZE;
				len -= EVT_CMD_COMPLETE_SIZE;

				_FMC_CORE_HCI_CmdCompleteCb(ptr, len, 0); /* _FMC_CORE_TRANSPORT_CLIENT_FM == 0 */
				break;
			default:
				break;
		}
	}

close:
	close(dd);
out:
	FM_END();
	pthread_exit((void *)ret);
#if defined(ANDROID)
	return 0;
#endif
}

void FMC_OS_RegisterFMInterruptCallback(FmcOsEventCallback func)
{
	fmParams2.taskCallback = func;
}

#endif  /* FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED */

