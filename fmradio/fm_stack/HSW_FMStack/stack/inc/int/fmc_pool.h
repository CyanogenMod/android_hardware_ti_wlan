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
*   FILE NAME:      fmc_pool.h
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


#ifndef __FMC_POOL_H
#define __FMC_POOL_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"

/********************************************************************************
 *
 * Definitions
 *
 *******************************************************************************/
/* Allocated memory is allways dividable by 4.*/
#define FMC_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(_actual_size)		((_actual_size) + 4*(((_actual_size) % 4)!=0) - ((_actual_size) % 4))

#define FMC_POOL_DECLARE_POOL(_pool_name, _memory_name, _num_of_elements, _element_size)	\
					FmcPool _pool_name;		\
					FMC_U32 _memory_name[(_num_of_elements) * \
						(FMC_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(_element_size)) / 4]

/********************************************************************************
 *
 * Constants
 *
 *******************************************************************************/
/*-------------------------------------------------------------------------------
 * FmcPoolConstants constants enumerator
 *
 *     Contains constant values definitions
 */
enum FmcPoolConstants
{
	FMC_POOL_MAX_POOL_NAME_LEN = 20,
	FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS = 120
};

/*-------------------------------------------------------------------------------
 * FmcPool structure
 *
 *     Contains a pool "class" members
 */
typedef struct
{
	/* external memory region from which elements are allocated */
	FMC_U32		*elementsMemory;

	/* number of elements in the pool */ 
	FMC_U32		numOfElements;

	/* size of a single element in bytes */
	FMC_U32		elementAllocatedSize;

	/* number of currently allocated elements */ 
	FMC_U32		numOfAllocatedElements;

	/* pool name (for debugging */
	char	name[FMC_POOL_MAX_POOL_NAME_LEN + 1];

	/* Per-element allocation flag (TRUE = allocated) */
	FMC_U8		allocationMap[FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS];
} FmcPool;

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_POOL_Create()
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
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_Create(	FmcPool		*pool,
									const char 	*name,
									FMC_U32 	*elementsMemory,
									FMC_U32 	numOfElements,
									FMC_U32 	elementSize);
	
/*-------------------------------------------------------------------------------
 * FMC_POOL_Destroy()
 *
 *		Destroys an existing pool instance
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 * Returns:
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_Destroy(FmcPool	 *pool);

/*-------------------------------------------------------------------------------
 * FMC_POOL_Allocate()
 *
 *		Allocate a free pool element.
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 *		element [out] - allocated element
 *		
 * Returns:
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_NO_RESOURCES - All elements are allocated
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_Allocate(FmcPool *pool, void ** element);

/*-------------------------------------------------------------------------------
 * FMC_POOL_Free()
 *
 *		Frees an allocated pool element.
 *
 * Parameters:
 *		pool [in / out] - Pool data ("this")
 *
 *		element [in / out] - freed element. Set to 0 upon exit to prevent caller from using it 
 *		
 * Returns:
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_Free(FmcPool *pool, void **element);

/*-------------------------------------------------------------------------------
 * FMC_POOL_GetNumOfAllocatedElements()
 *
 *		Gets the number of allocated elements.
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		num [out] - number of allocated elements 
 *		
 * Returns:
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_GetNumOfAllocatedElements(const FmcPool *pool, FMC_U32 *num);

/*-------------------------------------------------------------------------------
 * FMC_POOL_GetCapacity()
 *
 *		Gets the capacity (max number of elements) of the pool.
 *
 * Parameters:
 *		pool [in] - Pool data ("this")
 *
 *		capacity [out] - max number of elements
 *		
 * Returns:
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_GetCapacity(const FmcPool *pool, FMC_U32 *capacity);

/*-------------------------------------------------------------------------------
 * FMC_POOL_IsFull()
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
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_IsFull(FmcPool *pool, FMC_BOOL *answer);

/*-------------------------------------------------------------------------------
 * FMC_POOL_IsElelementAllocated()
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
 *		FMC_STATUS_SUCCESS - The operation was successful
 *
 *		FMC_STATUS_INVALID_PARM - An invalid argument was passed (e.g., null pointer)
 */
FmcStatus FMC_POOL_IsElelementAllocated(FmcPool *pool, const void* element, FMC_BOOL *answer);


/********************************************************************************
					DEBUG	UTILITIES
********************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_POOL_DEBUG_ListPools()
 *
 *		Debug utility that lists the names of all existing pools.
 *
 */
void FMC_POOL_DEBUG_ListPools(void);

/* Debug Utility - Print allocation state for a specific pool (identified by name) */
/*-------------------------------------------------------------------------------
 * FMC_POOL_DEBUG_Print()
 *
 *		Debug utility that prints information regarding the specified pool (by name)
 *
 * Parameters:
 *		poolName [in] - Pool name (as given during creation)
 *		
 */
void FMC_POOL_DEBUG_Print(char  *poolName);

#endif /* _FMC_POOL_H */

