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
*   FILE NAME:      mcp_pool.h
*
*   DESCRIPTION:
*
*	Represents a pool of elements that may be allocated and released.
*
*	A pool is created with the specified number of elements. The memory for the elements
*	is specified by the caller as well.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/


#ifndef __MCP_POOL_H
#define __MCP_POOL_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_defs.h"
#include "mcp_config.h"

/********************************************************************************
 *
 * Definitions
 *
 *******************************************************************************/
/* Allocated memory is allways dividable by 4.*/
#define MCP_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(_actual_size)		((_actual_size) + 4 - ((_actual_size) % 4))

#define MCP_POOL_DECLARE_POOL(_pool_name, _memory_name, _num_of_elements, _element_size)	\
					McpPool _pool_name;		\
					McpU32 _memory_name[(_num_of_elements) * (MCP_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(_element_size)) / 4]

typedef enum tagMcpPoolStatus {
	MCP_POOL_STATUS_SUCCESS			= MCP_HAL_STATUS_SUCCESS,
	MCP_POOL_STATUS_FAILED		 	= MCP_HAL_STATUS_FAILED,
	MCP_POOL_STATUS_PENDING		 	= MCP_HAL_STATUS_PENDING,
	MCP_POOL_STATUS_IN_PROGRESS		= MCP_HAL_STATUS_IN_PROGRESS,
	MCP_POOL_STATUS_NO_RESOURCES	= MCP_HAL_STATUS_NO_RESOURCES,
	MCP_POOL_STATUS_INVALID_PARM	= MCP_HAL_STATUS_INVALID_PARM,
	MCP_POOL_STATUS_NOT_SUPPORTED	= MCP_HAL_STATUS_NOT_SUPPORTED,
	MCP_POOL_STATUS_TIMEOUT		 	= MCP_HAL_STATUS_TIMEOUT,
	MCP_POOL_STATUS_INTERNAL_ERROR	= MCP_HAL_STATUS_INTERNAL_ERROR,
	MCP_POOL_STATUS_IMPROPER_STATE	= MCP_HAL_STATUS_IMPROPER_STATE,
} McpPoolStatus;

/********************************************************************************
 *
 * Constants
 *
 *******************************************************************************/
/*-------------------------------------------------------------------------------
 * _McpPoolConstants constants enumerator
 *
 *     Contains constant values definitions
 */
enum _McpPoolConstants
{
	MCP_POOL_MAX_POOL_NAME_LEN = 20,
	MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS = 120
};

/*-------------------------------------------------------------------------------
 * McpPool structure
 *
 *     Contains a pool "class" members
 */
typedef struct _McpPool
{
	/* external memory region from which elements are allocated */
	McpU32		*elementsMemory;

	/* number of elements in the pool */ 
	McpU32		numOfElements;

	/* size of a single element in bytes */
	McpU32		elementAllocatedSize;

	/* number of currently allocated elements */ 
	McpU32		numOfAllocatedElements;

	/* pool name (for debugging */
	char	name[MCP_POOL_MAX_POOL_NAME_LEN + 1];

	/* Per-element allocation flag (TRUE = allocated) */
	McpU8		allocationMap[MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS];
} McpPool;

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_POOL_Create()
 *
 *		Create a new Pool instance.
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 *		name [in] - pool name (for debugging)
 *
 *		elementsMemory [in] - external memory for allocation
 *
 *		numOfElements [in] - num of elements in the pool
 *
 *		elementSize [in] - size (in bytes) of a single element
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_Create(	McpPool				*pool,
								const char 		*name,
								McpU32 			*elementsMemory,
								McpU32 			numOfElements,
								McpU32 			elementSize);

/*-------------------------------------------------------------------------------
 * MCP_POOL_Destroy()
 *
 *		Destroys an existing pool instance
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_Destroy(McpPool	 *pool);

/*-------------------------------------------------------------------------------
 * MCP_POOL_Allocate()
 *
 *		Allocate a free pool element.
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 *		element [out] - allocated element
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_NO_RESOURCES - All elements are allocated
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_Allocate(McpPool *pool, void ** element);

/*-------------------------------------------------------------------------------
 * MCP_POOL_Free()
 *
 *		Frees an allocated pool element.
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 *		element [in / out] - freed element. Set to 0 upon exit to prevent caller from using it 
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_Free(McpPool *pool, void **element);

/*-------------------------------------------------------------------------------
 * MCP_POOL_GetNumOfAllocatedElements()
 *
 *		Gets the number of allocated elements.
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		num [out] - number of allocated elements 
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_GetNumOfAllocatedElements(const McpPool *pool, McpU32 *num);

/*-------------------------------------------------------------------------------
 * MCP_POOL_GetCapacity()
 *
 *		Gets the capacity (max number of elements) of the pool.
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		capacity [out] - max number of elements
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_GetCapacity(const McpPool *pool, McpU32 *capacity);

/*-------------------------------------------------------------------------------
 * MCP_POOL_IsFull()
 *
 *		Checks if the pool is full (all elements are allocated)l.
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		answer [out] - The answer:
 *						TRUE - pool is full (all elements are allocated)
 *						FALSE - pool is not full (there are unallocated elements)
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_IsFull(McpPool *pool, McpBool *answer);

/*-------------------------------------------------------------------------------
 * MCP_POOL_IsElelementAllocated()
 *
 *		Checks if the specified element is allocated in the pool
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		element [in] - Checked element
 *
 *		answer [out] - The answer:
 *						TRUE - element is allocated
 *						FALSE - element is NOT allocated
 *		
 * Returns:
 *		MCP_POOL_STATUS_SUCCESS - The operation was successful
 *
 *		MCP_POOL_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
McpPoolStatus MCP_POOL_IsElelementAllocated(McpPool *pool, const void* element, McpBool *answer);


/********************************************************************************
					DEBUG	UTILITIES
********************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_POOL_DEBUG_ListPools()
 *
 *		Debug utility that lists the names of all existing pools.
 *
 */
void MCP_POOL_DEBUG_ListPools(void);

/* Debug Utility - Print allocation state for a specific pool (identified by name) */
/*-------------------------------------------------------------------------------
 * MCP_POOL_DEBUG_Print()
 *
 *		Debug utility that prints information regarding the specified pool (by name)
 *
 * Parameters:
 *		poolName [in] - Pool name (as given during creation)
 *		
 */
void MCP_POOL_DEBUG_Print(char  *poolName);

#endif /* __MCP_POOL_H */



