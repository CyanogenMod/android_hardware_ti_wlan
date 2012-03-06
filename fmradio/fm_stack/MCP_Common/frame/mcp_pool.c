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
*   FILE NAME:      mcp_pool.c
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

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_memory.h"
#include "mcp_hal_string.h"
#include "mcp_defs.h"
#include "mcp_pool.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

MCP_STATIC const McpU32 MCP_POOL_INVALID_ELEMENT_INDEX = MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS + 1;
MCP_STATIC const McpU8 MCP_POOL_FREE_ELEMENT_MAP_INDICATION = 0;
MCP_STATIC const McpU8 MCP_POOL_ALLOCATED_ELEMENT_MAP_INDICATION = 1;
MCP_STATIC const McpU8 MCP_POOL_FREE_MEMORY_VALUE = 0x55;

MCP_STATIC McpBool MCP_POOL_IsDestroyed(const McpPool *pool);
MCP_STATIC McpU32 MCP_POOL_GetFreeElementIndex(McpPool *pool);
MCP_STATIC McpU32 MCP_POOL_GetElementIndexFromAddress(McpPool *pool, void *element);
MCP_STATIC void* MCP_POOL_GetElementAddressFromIndex(McpPool *pool, McpU32 elementIndex);

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

MCP_STATIC void McpPoolDebugAddPool(McpPool *pool);

McpPoolStatus MCP_POOL_Create(	McpPool		*pool, 
								const char 	*name, 
								McpU32 			*elementsMemory, 
								McpU32 			numOfElements, 
								McpU32 			elementSize)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	McpU32 allocatedSize;
	
	MCP_FUNC_START("MCP_POOL_Create");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != name), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null name argument"));
	MCP_VERIFY_FATAL((0 != elementsMemory), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null elementsMemory argument"));
	MCP_VERIFY_FATAL((MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS > numOfElements), MCP_POOL_STATUS_INTERNAL_ERROR, 
				("Max num of pool elements (%d) exceeded (%d)", numOfElements, MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS));
	MCP_VERIFY_FATAL((0 < elementSize), MCP_POOL_STATUS_INTERNAL_ERROR, ("Element size must be >0"));

	allocatedSize = MCP_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(elementSize);

	/* Copy init values to members */
	pool->elementsMemory = elementsMemory;
	pool->elementAllocatedSize = allocatedSize;
	pool->numOfElements = numOfElements;
	
	/* Safely copy the pool name */
	MCP_HAL_STRING_StrnCpy(pool->name, name, MCP_POOL_MAX_POOL_NAME_LEN);
	pool->name[MCP_POOL_MAX_POOL_NAME_LEN] = '\0';

	/* Init the other auxiliary members */
	
	pool->numOfAllocatedElements = 0;

	/* Mark all entries as free */
	MCP_HAL_MEMORY_MemSet(pool->allocationMap, MCP_POOL_FREE_ELEMENT_MAP_INDICATION,  MCP_POOL_MAX_NUM_OF_POOL_ELEMENTS);

	/* Fill memory in a special value to facilitate identification of dangling pointer usage */
	MCP_HAL_MEMORY_MemSet((McpU8 *)pool->elementsMemory, MCP_POOL_FREE_MEMORY_VALUE, numOfElements * allocatedSize);

	McpPoolDebugAddPool(pool);
	
	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_Destroy(McpPool	 *pool)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	McpU32		numOfAllocatedElements;
	
	MCP_FUNC_START("MCP_POOL_Destroy");

	MCP_LOG_INFO(("Destroying Pool %s", pool->name));
		
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));

	/* It is illegal to destroy a pool while there are allocated elements in the pool */
	status = MCP_POOL_GetNumOfAllocatedElements(pool, &numOfAllocatedElements);
	MCP_VERIFY_ERR_NORET(	(0 == numOfAllocatedElements), 
							("Pool %s still has %d allocated elements", pool->name, numOfAllocatedElements));

	pool->elementsMemory = 0;
	pool->numOfElements = 0;
	pool->elementAllocatedSize = 0;
	pool->numOfAllocatedElements = 0;
	MCP_HAL_STRING_StrCpy(pool->name, "");
	
	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_Allocate(McpPool *pool, void ** element)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	McpU32 		allocatedIndex = MCP_POOL_INVALID_ELEMENT_INDEX;

	MCP_FUNC_START("MCP_POOL_Allocate");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != element), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null element argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
		
	allocatedIndex = MCP_POOL_GetFreeElementIndex(pool);
	
	if (MCP_POOL_INVALID_ELEMENT_INDEX == allocatedIndex)
	{
		return MCP_POOL_STATUS_NO_RESOURCES;
	}

	++(pool->numOfAllocatedElements);
	pool->allocationMap[allocatedIndex] = MCP_POOL_ALLOCATED_ELEMENT_MAP_INDICATION;

	*element = MCP_POOL_GetElementAddressFromIndex(pool, allocatedIndex);
 
	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_Free(McpPool *pool, void **element)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	
	/* Map the element's address to a map index */ 
	McpU32 freedIndex = MCP_POOL_INVALID_ELEMENT_INDEX;

	MCP_FUNC_START("MCP_POOL_Free");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != element), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null element argument"));
	MCP_VERIFY_FATAL((0 != *element), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null *element argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
	
	freedIndex = MCP_POOL_GetElementIndexFromAddress(pool, *element);

	MCP_VERIFY_FATAL((MCP_POOL_INVALID_ELEMENT_INDEX != freedIndex), MCP_POOL_STATUS_INTERNAL_ERROR,
						("Invalid element address"));
	MCP_VERIFY_FATAL(	(MCP_POOL_ALLOCATED_ELEMENT_MAP_INDICATION == pool->allocationMap[freedIndex]),
						MCP_POOL_STATUS_INTERNAL_ERROR, ("Invalid element address"));
	
	pool->allocationMap[freedIndex] = MCP_POOL_FREE_ELEMENT_MAP_INDICATION;
	--(pool->numOfAllocatedElements);

	/* Fill memory in a special value to facilitate identification of dangling pointer usage */
	MCP_HAL_MEMORY_MemSet(	(McpU8 *)MCP_POOL_GetElementAddressFromIndex(pool, freedIndex), 
			MCP_POOL_FREE_MEMORY_VALUE, pool->elementAllocatedSize);

	/* Make sure caller's pointer stops pointing to the freed element */
	*element = 0;
	
	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_GetNumOfAllocatedElements(const McpPool *pool, McpU32 *num)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	
	MCP_FUNC_START("MCP_POOL_GetNumOfAllocatedElements");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
	
	*num = pool->numOfAllocatedElements;

	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_GetCapacity(const McpPool *pool, McpU32 *capacity)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	
	MCP_FUNC_START("MCP_POOL_GetCapacity");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != capacity), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null capacity argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
	
	*capacity = pool->numOfElements;

	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_IsFull(McpPool *pool, McpBool *answer)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	
	MCP_FUNC_START("MCP_POOL_IsFull");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != answer), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null answer argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));

	if (pool->numOfAllocatedElements == pool->numOfElements)
	{
		*answer = MCP_TRUE;
	}
	else
	{
		*answer = MCP_FALSE;
	}

	MCP_FUNC_END();
	
	return status;
}

McpPoolStatus MCP_POOL_IsElelementAllocated(McpPool *pool, const void* element, McpBool *answer)
{
	McpPoolStatus	status = MCP_POOL_STATUS_SUCCESS;
	
	/* Map the element's address to a map index */ 
	McpU32 freedIndex = MCP_POOL_INVALID_ELEMENT_INDEX;
	
	MCP_FUNC_START("MCP_POOL_IsElelementAllocated");
	
	MCP_VERIFY_FATAL((0 != pool), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null pool argument"));
	MCP_VERIFY_FATAL((0 != element), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null element argument"));
	MCP_VERIFY_FATAL((0 != answer), MCP_POOL_STATUS_INTERNAL_ERROR, ("Null answer argument"));
	MCP_VERIFY_FATAL((MCP_FALSE == MCP_POOL_IsDestroyed(pool)), MCP_POOL_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));

	freedIndex = MCP_POOL_GetElementIndexFromAddress(pool, (void*)element);

	MCP_VERIFY_ERR((MCP_POOL_INVALID_ELEMENT_INDEX != freedIndex), MCP_POOL_STATUS_INVALID_PARM,
					("Invalid element address"));

	if (MCP_POOL_ALLOCATED_ELEMENT_MAP_INDICATION == pool->allocationMap[freedIndex])
	{
		*answer = MCP_TRUE;
	}
	else
	{
		*answer = MCP_FALSE;
	}

	MCP_FUNC_END();
	
	return status;
}

McpU32 MCP_POOL_GetFreeElementIndex(McpPool *pool)
{	
	McpU32 freeIndex = MCP_POOL_INVALID_ELEMENT_INDEX;
	
	for (freeIndex = 0; freeIndex < pool->numOfElements; ++freeIndex)
	{
		if (MCP_POOL_FREE_ELEMENT_MAP_INDICATION == pool->allocationMap[freeIndex])
		{
			return freeIndex;
		}
	}
		
	return MCP_POOL_INVALID_ELEMENT_INDEX;
}

McpU32 MCP_POOL_GetElementIndexFromAddress(McpPool *pool, void *element)
{
	McpU32 		elementAddress = (McpU32)element;
	McpU32 		elementsMemoryAddress = (McpU32)(pool->elementsMemory);
	McpU32 		index = MCP_POOL_INVALID_ELEMENT_INDEX;

	MCP_FUNC_START("MCP_POOL_GetElementIndexFromAddress");
	
	/* Verify that the specified address is valid (in the address range and on an element's boundary) */
	
	MCP_VERIFY_FATAL_SET_RETVAR(	(elementAddress >= elementsMemoryAddress), 
									index = MCP_POOL_INVALID_ELEMENT_INDEX,
									("Invalid element Address"));
	MCP_VERIFY_FATAL_SET_RETVAR(	(((elementAddress - elementsMemoryAddress) % pool->elementAllocatedSize) == 0),
									index = MCP_POOL_INVALID_ELEMENT_INDEX, 
									("Invalid element Address"));

	/* Calculate the index */
	index = (elementAddress - elementsMemoryAddress) / pool->elementAllocatedSize;

	/* Verify that its within index bounds */
	MCP_VERIFY_FATAL_SET_RETVAR((index < pool->numOfElements), index = MCP_POOL_INVALID_ELEMENT_INDEX, (""));

	MCP_FUNC_END();
	
	return index;
}

void* MCP_POOL_GetElementAddressFromIndex(McpPool *pool, McpU32 elementIndex)
{
	/* we devide by 4 sinse pool->elementsMemory is McpU32 array .*/
	return &(pool->elementsMemory[elementIndex * pool->elementAllocatedSize / 4]);
}

/* DEBUG */

MCP_STATIC McpPool* McpPoolDebugPools[20];
MCP_STATIC McpBool McpPoolDebugIsFirstPool = MCP_TRUE;

void McpPoolDebugInit(void)
{
	MCP_HAL_MEMORY_MemSet((McpU8*)McpPoolDebugPools, 0, sizeof(McpPool*));
}

void McpPoolDebugAddPool(McpPool *pool)
{
	McpU32	index = 0;
	
	if (MCP_TRUE == McpPoolDebugIsFirstPool)
	{
		McpPoolDebugInit();
		McpPoolDebugIsFirstPool = MCP_FALSE;
	}

	for (index = 0; index < 20; ++index)
	{
		if (0 == McpPoolDebugPools[index])
		{
			McpPoolDebugPools[index] = pool;
			return;
		}
	}
	
}

void MCP_POOL_DEBUG_ListPools(void)
{
	McpU32	index = 0;

	MCP_FUNC_START("MCP_POOL_DEBUG_ListPools");

	MCP_LOG_DEBUG(("Pool List:\n"));
	
	for (index = 0; index < 20; ++index)
	{
		if (0 != McpPoolDebugPools[index])
		{
			MCP_LOG_DEBUG(("Pool %s\n", McpPoolDebugPools[index]->name));
		}
	}

	MCP_LOG_DEBUG(("\n"));

	MCP_FUNC_END();
}

void MCP_POOL_DEBUG_Print(char  *poolName)
{
	McpU32	index = 0;
	
	for (index = 0; index < 20; ++index)
	{
		if (0 != McpPoolDebugPools[index])
		{
			if (0 == MCP_HAL_STRING_StrCmp(poolName, McpPoolDebugPools[index]->name))
			{
				MCP_LOG_DEBUG(("%d elements allocated in Pool %s \n", 
						McpPoolDebugPools[index]->numOfAllocatedElements, McpPoolDebugPools[index]->name));
				return;
			}
		}
	}
	
	MCP_LOG_DEBUG(("%s was not found\n", poolName));
}

McpBool MCP_POOL_IsDestroyed(const McpPool *pool)
{
	if (0 != pool->elementsMemory)
	{
		return MCP_FALSE;
	}
	else
	{
		return MCP_TRUE;
	}
}


