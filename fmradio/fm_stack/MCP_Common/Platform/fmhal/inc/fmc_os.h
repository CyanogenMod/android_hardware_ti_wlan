/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
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
*   FILE NAME:      fmc_os.h
*
*   BRIEF:          This file defines the API of the FM OS.
*
*   DESCRIPTION:    General
*
*                   FM OS API layer.
*                   
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __FMC_OS_H
#define __FMC_OS_H

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_defs.h"
#include "fmhal_config.h"

/* Enable it for development . Disable it for customer release

If ASSERTION_DEBUG is set to 1, then assert will print the error message and terminate the program.
If ASSERTION_DEBUG is set to 0, then assert will print the error message.
*/

#define ASSERTION_DEBUG 0

#define FMC_OS_MS_TO_TICKS(ms)                   (ms)


/*-------------------------------------------------------------------------------
 * FmcOsTaskHandle type
 *
 *     Defines task handle.
 */
typedef FMC_U8      FmcOsTaskHandle;


/*-------------------------------------------------------------------------------
 * FmcOsEvent type
 *
 *     	Defines event type. Events can be OR'ed together.
 */
typedef McpU32      FmcOsEvent;


/*-------------------------------------------------------------------------------
 * FmcOsSemaphoreHandle type
 *
 *     Defines semaphore handle.
 */
typedef  FMC_U32 	FmcOsSemaphoreHandle;


#define FMC_OS_TASK_HANDLE_FM                                 (0x01)

/*-------------------------------------------------------------------------------
 * FmcOsTimerHandle type
 *
 *     Defines timer handle.
 */
typedef FMC_U8 FmcOsTimerHandle;

/*-------------------------------------------------------------------------------
 * FmcOsTime type
 *
 *     Defines time type.
 */
typedef FMC_U32 FmcOsTime;

/*---------------------------------------------------------------------------
 * eventCallback type
 *
 *      A eventCallback function is provided when creating a task.
 *      This callback will be called when events are received for the task.
 *      The passed argument is a bitmask of the received events.
 */
typedef void (*FmcOsEventCallback)(FmcOsEvent evtMask);

/* FM Stack event */
#define FMC_OS_EVENT_FMC_STACK_TASK_PROCESS     (FMC_OS_EVENT_GENERAL)
#define FMC_OS_EVENT_FMC_TIMER_EXPIRED          (FMC_OS_EVENT_TIMER_EXPIRED)

/*
FM Events
*/

/*This event is recieved when interrupt is recieved from the FM */
#define FMC_OS_EVENT_INTERRUPT_RECIEVED             (0x00000001)
/* General event usually sent when recieved CC or when entered new command in queue*/   
#define FMC_OS_EVENT_GENERAL                        (0x00000002)
/*This event is recieved when the FM timer expiers */
#define FMC_OS_EVENT_TIMER_EXPIRED              (0x00000004)
/*This event is recieved when Disable API command called*/
#define FMC_OS_EVENT_DISABELING                 (0x00000008)

/*This event is used to terminate the event reading thread*/
/*Define event value high in case other events are added by TI */
#define FMC_OS_INTERNAL_TERMINATE_STACKTHREAD   (0x00010000)

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_OS_Init()
 *
 * Brief:  
 *      Initializes the FM OS abstraction layer.
 *
 * Description:
 *      Initializes the FM OS abstraction layer.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully initialized the FM OS abstraction layer.
 *
 *      FMC_STATUS_FAILED - failed to initialize the FM OS abstraction layer.
 */
FmcStatus FMC_OS_Init(void);

/*-------------------------------------------------------------------------------
 * FMC_OS_Deinit()
 *
 * Brief:  
 *      De-initializes the FM OS abstraction layer.
 *
 * Description:
 *      De-initializes the FM OS abstraction layer.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully de-initialized the FM OS abstraction layer.
 *
 *      FMC_STATUS_FAILED - failed to de-initialize the FM OS abstraction layer.
 */
FmcStatus FMC_OS_Deinit(void);

/*-------------------------------------------------------------------------------
 * FMC_OS_CreateTask()
 *
 * Brief:  
 *      Creates a task.
 *
 * Description:
 *      Creates a task.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      taskHandle - the task specific handle (FMC_OS_TASK_HANDLE_FM)
 *      callback - Callback function to be used for task events
 *      taskName - the task name (ASCII string)
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully cerated the task.
 *
 *      FMC_STATUS_FAILED - failed to create the task.
 */
FmcStatus FMC_OS_CreateTask(FmcOsTaskHandle taskHandle,
                            FmcOsEventCallback callback,
                            const char *taskName);

/*-------------------------------------------------------------------------------
 * FMC_OS_DestroyTask()
 *
 * Brief:  
 *      Destroys a task.
 *
 * Description:
 *      Destroys a task.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      taskHandle - the task specific handle (FMC_OS_TASK_HANDLE_FM)
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully destroyed the task.
 *
 *      FMC_STATUS_FAILED - failed to destroyed the task.
 */
FmcStatus FMC_OS_DestroyTask(FmcOsTaskHandle taskHandle);

/*-------------------------------------------------------------------------------
 * FMC_OS_SendEvent()
 *
 * Brief:  
 *      Sends an FM event to a task.
 *
 * Description:
 *      Sends an FM event to a task.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      taskHandle - the task specific handle (FMC_OS_TASK_HANDLE_FM)
 *      evt - the event to send
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully sent the event.
 *
 *      FMC_STATUS_FAILED - failed to send the event.
 */
FmcStatus FMC_OS_SendEvent(FmcOsTaskHandle taskHandle, FmcOsEvent evt);

/*-------------------------------------------------------------------------------
 * FMC_OS_CreateSemaphore()
 *
 * Brief:  
 *      Creates a semaphore.
 *
 * Description:
 *      Creates a semaphore.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      semaphoreName - the semaphore anme (ASCII string)
 *      semaphoreHandle - the new semaphore handle
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully created a sempahore.
 *
 *      FMC_STATUS_NO_RESOURCES - system rsources exhusted - no more semaphores 
 *                                cen be created
 *
 *      FMC_STATUS_FAILED - failed to create a semaphore.
 */
FmcStatus FMC_OS_CreateSemaphore(const char *semaphoreName,
                                 FmcOsSemaphoreHandle *semaphoreHandle);

/*-------------------------------------------------------------------------------
 * FMC_OS_DestroySemaphore()
 *
 * Brief:  
 *      Destroyes a semaphore.
 *
 * Description:
 *      Destroyes a semaphore.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      semaphoreHandle - semaphore handle to destroy
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully destroyed the sempahore.
 *
 *      FMC_STATUS_FAILED - failed to destroy the semaphore.
 */
FmcStatus FMC_OS_DestroySemaphore(FmcOsSemaphoreHandle semaphoreHandle);

/*-------------------------------------------------------------------------------
 * FMC_OS_LockSemaphore()
 *
 * Brief:  
 *      Locks the FM semaphore.
 *
 * Description:
 *      Locks the FM semaphore.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      semaphore - the FM semaphore handle
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully locked the FM semaphore.
 *
 *      FMC_STATUS_FAILED - Lock the FM semaphore failed.
 */
FmcStatus FMC_OS_LockSemaphore(FmcOsSemaphoreHandle semaphore);

/*-------------------------------------------------------------------------------
 * FMC_OS_UnlockFmStack()
 *
 * Brief:  
 *      Unlock the FM semaphore.
 *
 * Description:
 *      Unlock the FM semaphore.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      semaphore - the FM semaphore handle
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - successfully unlocked the FM semaphore.
 *
 *      FMC_STATUS_FAILED - Unlock the FM semaphore failed.
 */
FmcStatus FMC_OS_UnlockSemaphore(FmcOsSemaphoreHandle semaphore);

/*---------------------------------------------------------------------------
 * FMC_OS_Assert()
 *
 *     Called by the stack to indicate that an assertion failed. FMC_OS_Assert
 *     should display the failed expression and the file and line number
 *     where the expression occurred.
 *
 *     The stack uses Assert calls, which are redefined to call FMC_OS_Assert
 *     if their expression fails. These calls verify the correctness of
 *     function parameters, internal states, and data to detect problems
 *     during debugging.
 *
 * Parameters:
 *     expression - A string containing the failed expression.
 *
 *     file - A string containing the file in which the expression occurred.
 *
 *     line - The line number that tested the expression.
 */
void FMC_OS_Assert(const char *expression, const char *file, FMC_U16 line);

void FMC_OS_APPCB();

/*---------------------------------------------------------------------------
 * FMC_OS_MemCopy()
 *
 *     Called by the stack to copy memory from one buffer to another.
 *     
 *     This function's implementation could use the ANSI memcpy function.
 *
 * Parameters:
 *     dest - Destination buffer for data.
 *
 *     source - Source buffer for data. "dest" and "source" must not
 *         overlap.
 *
 *     numBytes - Number of bytes to copy from "source" to "dest".
 */
void FMC_OS_MemCopy(void *dest, const void *source, FMC_U32 numBytes);

/*---------------------------------------------------------------------------
 * FMC_OS_MemCmp()
 *
 *     Called by the stack to compare the bytes in two different buffers.
 *     If the buffers lengths or contents differ, this function returns FALSE.
 *
 *     This function's implementation could use the ANSI memcmp
 *     routine as shown:
 *
 *     return (len1 != len2) ? FALSE : (0 == memcmp(buffer1, buffer2, len2));
 *     
 *
 * Parameters:
 *     buffer1 - First buffer to compare.
 *
 *     len1 - Length of first buffer to compare.
 *
 *     buffer2 - Second buffer to compare.
 *
 *     len2 - Length of second buffer to compare.
 *
 * Returns:
 *     TRUE - The lengths and contents of both buffers match exactly.
 *
 *     FALSE - Either the lengths or the contents of the buffers do not
 *         match.
 */
FMC_BOOL FMC_OS_MemCmp(const void *buffer1, FMC_U16 len1, const void *buffer2, FMC_U16 len2);

/*---------------------------------------------------------------------------
 * FMC_OS_MemSet()
 *
 *     Fills the destination buffer with the specified byte.
 *
 *     This function's implementation could use the ANSI memset
 *     function.
 *
 * Parameters:
 *     dest - Buffer to fill.
 *
 *     byte - Byte to fill with.
 *
 *     len - Length of the destination buffer.
 */
void FMC_OS_MemSet(void *dest, FMC_U8 byte, FMC_U32 len);

/*---------------------------------------------------------------------------
 * FMC_OS_StrCmp()
 *
 *     Compares two strings for equality.
 *
 * Parameters:
 *     Str1 - String to compare.
 *     Str2 - String to compare.
 *
 * Returns:
 *     Zero - If strings match.
 *     Non-Zero - If strings do not match.
 */
FMC_U8 FMC_OS_StrCmp(const FMC_U8 *Str1, const FMC_U8 *Str2);

/*---------------------------------------------------------------------------
 * FMC_OS_StriCmp()
 *
 *     Compares two strings for equality regardless of case.
 *
 * Parameters:
 *     Str1 - String to compare.
 *     Str2 - String to compare.
 *
 * Returns:
 *     Zero - If strings match.
 *     Non-Zero - If strings do not match.
 */
FMC_U8 FMC_OS_StriCmp(const FMC_U8 *Str1, const FMC_U8 *Str2);

/*---------------------------------------------------------------------------
 * FMC_OS_StrLen()
 *
 *     Calculate the length of the string.
 *
 * Parameters:
 *     Str - String to count length.
 *
 * Returns:
 *     Returns length of string.
 */
FMC_U16 FMC_OS_StrLen(const FMC_U8 *Str);

/*---------------------------------------------------------------------------
 * FMC_OS_StrCpy()
 *
 *    Copy a string (same as ANSI C strcpy)
 *
 *  The FMC_OS_StrCpy function copies StrSource, including the terminating null character, 
 *  to the location specified by StrDest. No overflow checking is performed when strings 
 *  are copied or appended. 
 *
 *  The behavior of FMC_OS_StrCpy is undefined if the source and destination strings overlap 
 *
 * Parameters:
 *     StrDest - Destination string.
 *
 *  StrSource - Source string
 *
 * Returns:
 *     Returns StrDest. No return value is reserved to indicate an error.
 */
FMC_U8 * FMC_OS_StrCpy(FMC_U8 * StrDest, const FMC_U8 *StrSource);

/*---------------------------------------------------------------------------
 * FMC_OS_StrnCpy()
 *
 *    Copy characters of one string to another (same as ANSI C strncpy)
 *
 *  The FMC_OS_StrnCpy function copies the initial Count characters of StrSource to StrDest and 
 *  returns StrDest. If Count is less than or equal to the length of StrSource, a null character 
 *  is not appended automatically to the copied string. If Count is greater than the length of 
 *  StrSource, the destination string is padded with null characters up to length Count. 
 *
 *  The behavior of FMC_OS_StrnCpy is undefined if the source and destination strings overlap.
 *
 * Parameters:
 *     StrDest - Destination string.
 *
 *  StrSource - Source string
 *
 *  Count - Number of characters to be copied
 *
 * Returns:
 *     Returns strDest. No return value is reserved to indicate an error.
 */
FMC_U8 * FMC_OS_StrnCpy(FMC_U8 * StrDest, const FMC_U8 *StrSource, FMC_U32 Count);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_CreateTimer()
 *
 * Brief:  
 *      Create a timer, and returns the handle to that timer.
 *
 * Description:    
 *      Create a timer, and returns the handle to that timer.
 *      Upon creation, the timer is associated with a task, meaning the timer 
 *      expired event is sent to that task event callback.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      taskHandle [in] - the task which will receive the timer expired event 
 *          (via the event callback given in the task creation).
 *      
 *      timerName [in] - null-terminated string representing the timer name, 
 *          it can be null.
 *
 *      timerHandle [out] - the handle to this timer, which is returned.
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *      FMC_STATUS_FAILED - Indicates that the operation failed.
 */
FmcStatus FMC_OS_CreateTimer(FmcOsTaskHandle taskHandle,
                             const char *timerName,
                             FmcOsTimerHandle *timerHandle);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_DestroyTimer()
 *
 * Brief:  
 *      Destroy a timer.
 *
 * Description:    
 *      Destroy a timer.
 * 
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      timerHandle [in] - the timer to be destroyed.
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *      FMC_STATUS_FAILED - Indicates that the operation failed.
 */
FmcStatus FMC_OS_DestroyTimer (FmcOsTimerHandle timerHandle);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_ResetTimer()
 *
 * Brief:  
 *      Reset a timer.
 * 
 * Description:    
 *      Reset a timer.
 *      If the timer is already active, BTHAL_OS_ResetTimer automatically cancels 
 *      the previous timer as if BTHAL_OS_CancelTimer was called.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      timerHandle [in] - the timer to be reset.
 *
 *      time [in] - number of ticks until the timer fires, after which the given event 
 *          will be sent to the associated task (via the event callback given during 
 *          the task creation).
 *
 *      evt [in] - the event to be sent to the task event callback when the timer expires.
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *      FMC_STATUS_FAILED - Indicates that the operation failed.
 */
FmcStatus FMC_OS_ResetTimer(FmcOsTimerHandle timerHandle, 
                            FmcOsTime time, 
                            FmcOsEvent evt);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_CancelTimer()
 *
 * Brief:  
 *      Cancel a timer.
 *
 * Description:    
 *      Cancel a timer.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      timerHandle [in] - the timer to be canceled.
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *      FMC_STATUS_FAILED - Indicates that the operation failed.
 */
FmcStatus FMC_OS_CancelTimer(FmcOsTimerHandle timerHandle);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_Sleep()
 *
 * Brief:  
 *      Suspends the execution of the current task for the specified interval.
 *
 * Description:  
 *      Suspends the execution of the current task for the specified interval.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      time [in] - sleep time in milliseconds.
 *
 * Returns:
 *      BTHAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *      BTHAL_STATUS_FAILED - Indicates that the operation failed.
 */

FMC_U8 FMC_OS_Sleep(FMC_U32 time);


#endif  /* __FMC_OS_H */

