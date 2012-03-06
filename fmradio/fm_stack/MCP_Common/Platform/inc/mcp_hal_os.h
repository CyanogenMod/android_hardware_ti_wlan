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
*   FILE NAME:      mcp_hal_os.h
*
*   BRIEF:          This file defines the API of the MCP HAL OS.
*
*   DESCRIPTION:    General
*
*                   MCP HAL OS API layer 
*                   
*   AUTHOR:         Ilan Elias
*
\*******************************************************************************/

#ifndef __MCP_HAL_OS_H
#define __MCP_HAL_OS_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"
#include "mcpf_defs.h"
                        

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 *  McpHal]OsStatus
 */
typedef enum tagMcpHalOsStatus
{
	MCP_HAL_OS_STATUS_SUCCESS 	=  MCP_HAL_STATUS_SUCCESS,
	MCP_HAL_OS_STATUS_FAILED 	=  MCP_HAL_STATUS_FAILED,
	MCP_HAL_OS_STATUS_TIMEOUT	 = MCP_HAL_STATUS_TIMEOUT
} McpHalOsStatus;

/*-------------------------------------------------------------------------------
 * McpHalOsSemaphoreHandle type
 *
 *     Defines semaphore handle.
 */
typedef void *McpHalOsSemaphoreHandle;

#define MCP_HAL_OS_INVALID_SEMAPHORE_HANDLE		((void *)NULL)

/*-------------------------------------------------------------------------------
 * McpHalOsTime type
 *
 *     Defines time type.
 */
typedef McpUint McpHalOsTimeInMs;

#define MCP_HAL_OS_TIME_INFINITE			((McpHalOsTimeInMs)MCP_UINT_MAX)

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_Init()
 *
 * Brief:  
 *	    Initializes the operating system layer.
 *
 * Description:
 *	    Initializes the operating system layer.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void
 *
 * Returns:
 *		MCP HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP HAL_STATUS_FAILED - Indicates that the operation failed.
 */
McpHalOsStatus MCP_HAL_OS_Init(void);


/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_Deinit()
 *
 * Brief:  
 *	    Deinitializes the operating system layer.
 *
 * Description:
 *	    Deinitializes the operating system layer.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void.
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.
 */
McpHalOsStatus MCP_HAL_OS_Deinit(void);


/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_CreateSemaphore()
 *
 * Brief:  
 *	    Create a semaphore, and returns the handle to that semaphore.
 *
 * Description:  
 *	    Create a semaphore, and returns the handle to that semaphore.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		semaphoreName [in] - null-treminated string representing the semaphore name, 
 *			it can be null.
 *
 *		semaphoreHandle [out] - the handle to this semaphore, which is returned.
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.
 */
McpHalOsStatus MCP_HAL_OS_CreateSemaphore(const char *semaphoreName, McpHalOsSemaphoreHandle *semaphoreHandle);


/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_DestroySemaphore()
 *
 * Brief:  
 *	    Destroy a semaphore.
 *
 * Description:  
 *	    Destroy a semaphore.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		semaphoreHandle [in] - the semaphore to be destroyed.
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.
 */
McpHalOsStatus MCP_HAL_OS_DestroySemaphore(McpHalOsSemaphoreHandle *semaphoreHandle);


/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_LockSemaphore()
 *
 * Brief:  
 *	    Lock a semaphore.
 *
 * Description:  
 *	    Lock a semaphore.
 *		A timeout interval can be provided to indicate the amount of time to try
 *		locking this semaphore.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		semaphoreHandle [in] - the semaphore to be locked.
 *
 *		timeout [in] - timeout interval in milliseconds, in which try to lock 
 *					this semaphore. If timeout is MCP_HAL_OS_TIME_INFINITE, then try infinitely to lock it.
 *					If timeout is zero, the function attempts to lock the semaphore and returns immediately.
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *		MCP_HAL_STATUS_TIMEOUT - Indicates that the operation failed, because of 
 *									timeout interval elapsed.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.	
 */
McpHalOsStatus MCP_HAL_OS_LockSemaphore(McpHalOsSemaphoreHandle 	semaphoreHandle, 
													McpHalOsTimeInMs 		timeout);


/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_UnlockSemaphore()
 *
 * Brief:  
 *	    Unlock a semaphore.
 *
 * Description:  
 *	    Unlock a semaphore.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		semaphoreHandle [in] - the semaphore to be unlocked.
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.	
 */
McpHalOsStatus MCP_HAL_OS_UnlockSemaphore(McpHalOsSemaphoreHandle semaphoreHandle);

/*-------------------------------------------------------------------------------
 * MCP_HAL_OS_Sleep()
 *
 * Brief:  
 *	    Suspends the execution of the current task for the specified interval.
 *
 * Description:  
 *	    Suspends the execution of the current task for the specified interval.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		time [in] - sleep time in milliseconds. A value of INFINITE causes an infinite sleep. 
 *
 * Returns:
 *		MCP_HAL_STATUS_SUCCESS - Indicates that the operation was successful.
 *
 *     	MCP_HAL_STATUS_FAILED - Indicates that the operation failed.
 */
McpHalOsStatus MCP_HAL_OS_Sleep(McpHalOsTimeInMs time);

/*-------------------------------------------------------------------------------
 * BTHAL_OS_GetSystemTime()
 *
 * Brief:  
 *	    Returns the system time, in milliseconds.
 *
 * Description:  
 *	    The system time is the time elapsed since the system was started.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void
 *
 * Returns:
 *		Returns the system time, in milliseconds
 */
McpHalOsTimeInMs MCP_HAL_OS_GetSystemTime(void);

/*-------------------------------------------------------------------------------
 * os_memoryAlloc()
 *
 * Brief:  
 *	    OS Memory Allocation
 *
 * Description:  
 *	    This function allocates resident (nonpaged) system-space memory with 
 *		calling specific OS allocation function. It is assumed that this function 
 *		will never be called in an interrupt context since the OS allocation function 
 *		has the potential to put the caller to sleep  while waiting for memory to become available.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		OsContext 	- Handle to the OS object
 *		Size 		- Size (in bytes) to be allocated
 *
 * Returns:
 *		Pointer to the allocated memory on success, NULL on failure (there isn't enough memory available)
 */
void *os_memoryAlloc (handle_t OsContext, McpU32 Size);

/*-------------------------------------------------------------------------------
 * os_memorySet()
 *
 * Brief:  
 *	    OS Memory Set
 *
 * Description:  
 *	    This function fills a block of memory with a given value
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		OsContext 	- Handle to the OS object
 *		pMemPtr 	- Pointer to the base address of a memory block 
 *		Value 		- Value to set to memory block
 *		Length 		- Length (in bytes) of memory block
 *
 * Returns:
 *		void
 */
void os_memorySet (handle_t OsContext, void *pMemPtr, McpS32 Value, McpU32 Length);

/*-------------------------------------------------------------------------------
 * os_memoryZero()
 *
 * Brief:  
 *	    OS Memory Zero
 *
 * Description:  
 *	    This function fills a block of memory with zeros
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		OsContext 	- Handle to the OS object
 *		pMemPtr 	- Pointer to the base address of a memory block 
 *		Length 		- Length (in bytes) of memory block
 *
 * Returns:
 *		void
 */
void os_memoryZero (handle_t OsContext, void *pMemPtr, McpU32 Length);

/*-------------------------------------------------------------------------------
 * os_memoryFree()
 *
 * Brief:  
 *	    OS Memory Zero
 *
 * Description:  
 *	    This function releases a block of memory which was previously allocated by user
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		OsContext 	- Handle to the OS object
 *		pMemPtr 	- Pointer to the base address of a memory block 
 *		Size 		- Size (in bytes) to free
 *
 * Returns:
 *		void
 */
void os_memoryFree (handle_t OsContext, void *pMemPtr, McpU32 Size);

/*-------------------------------------------------------------------------------
 * os_memoryCopy()
 *
 * Brief:  
 *	    OS Memory Copy
 *
 * Description:  
 *	    This function copies a specified number of bytes from one caller-supplied 
 *		location (source buffer) to another (destination buffer)
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		OsContext 		- Handle to the OS object
 *		pDestination	- Pointer to destination buffer 
 *		pSource 		- Pointer to Source buffer
 *		Size 			- Size (in bytes) to copy
 *
 * Returns:
 *		void
 */
void os_memoryCopy (handle_t OsContext, void *pDestination, void *pSource, McpU32 Size);

/*-------------------------------------------------------------------------------
 * os_timerStart()
 *
 * Brief:  
 *	    OS Timer Start
 *
 * Description:  
 *	    This function starts a timer for the requested time
 *
 *
 * Parameters:
 *		expiry_time 	- Elapsed time until timer will expire
 *		cb 				- callback function to call when timer expires 
 *
 * Returns:
 *		timer ID
 */
McpUint os_timerStart(McpUint expiry_time,	mcpf_timer_cb cb);

/*-------------------------------------------------------------------------------
 * os_timerStop()
 *
 * Brief:  
 *	    OS Timer Stop
 *
 * Description:  
 *	    This function stops a specific timer
 *
 *
 * Parameters:
 *		timer_id 	- The timer to stop
 *
 * Returns:
 *		Success > 0,  Fail = 0.
 */
McpBool os_timerStop(McpUint timer_id);

/*-------------------------------------------------------------------------------
 * os_printf()
 *
 * Brief:  
 *	    OS Printf
 *
 * Description:  
 *	    This function prints formatted output using OS available printf method
 *
 *
 * Parameters:
 *		format 	- String to print (with formatted parameters in string if needed) 
 *				  and parameters values if formatted parameters are used in string
 *
 * Returns:
 *		void
 */
void os_printf (const char *format ,...);

#endif /* __MCP_HAL_OS_H */


