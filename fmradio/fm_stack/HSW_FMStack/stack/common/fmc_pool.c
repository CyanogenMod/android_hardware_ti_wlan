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
*   FILE NAME:      fmc_pool.c
*
*   DESCRIPTION:
*
*   Represents a pool of elements that may be allocated and released.
*
*   A pool is created with the specified number of elements. The memory for the elements
*   is specified by the caller as well.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_defs.h"
#include "fmc_os.h"
#include "fmc_log.h"
#include "fmc_pool.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMPOOL);

static const FMC_U32 _FMC_POOL_INVALID_ELEMENT_INDEX = FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS + 1;
static const FMC_U8 _FMC_POOL_FREE_ELEMENT_MAP_INDICATION = 0;
static const FMC_U8 _FMC_POOL_ALLOCATED_ELEMENT_MAP_INDICATION = 1;
static const FMC_U8 _FMC_POOL_FREE_MEMORY_VALUE = 0x55;

FMC_STATIC FMC_BOOL _FMC_POOL_IsDestroyed(const FmcPool *pool);
FMC_STATIC FMC_U32 _FMC_POOL_GetFreeElementIndex(FmcPool *pool);
FMC_STATIC FMC_U32 _FMC_POOL_GetElementIndexFromAddress(FmcPool *pool, void *element);
FMC_STATIC void* _FMC_POOL_GetElementAddressFromIndex(FmcPool *pool, FMC_U32 elementIndex);

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

FMC_STATIC void FmcPoolDebugAddPool(FmcPool *pool);

FmcStatus FMC_POOL_Create(  FmcPool     *pool, 
                                const char  *name, 
                                FMC_U32             *elementsMemory, 
                                FMC_U32             numOfElements, 
                                FMC_U32             elementSize)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    FMC_U32 allocatedSize;
    
    FMC_FUNC_START("FMC_POOL_Create");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != name), FMC_STATUS_INTERNAL_ERROR, ("Null name argument"));
    FMC_VERIFY_FATAL((0 != elementsMemory), FMC_STATUS_INTERNAL_ERROR, ("Null elementsMemory argument"));
    FMC_VERIFY_FATAL((FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS > numOfElements), FMC_STATUS_INTERNAL_ERROR, 
                ("Max num of pool elements (%d) exceeded (%d)", numOfElements, FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS));
    FMC_VERIFY_FATAL((0 < elementSize), FMC_STATUS_INTERNAL_ERROR, ("Element size must be >0"));

    allocatedSize = FMC_POOL_ACTUAL_SIZE_TO_ALLOCATED_MEMORY_SIZE(elementSize);

    /* Copy init values to members */
    pool->elementsMemory = elementsMemory;
    pool->elementAllocatedSize = allocatedSize;
    pool->numOfElements = numOfElements;
    
    /* Safely copy the pool name */
    FMC_OS_StrnCpy(( FMC_U8 *)pool->name, (const FMC_U8 *)name, FMC_POOL_MAX_POOL_NAME_LEN);
    pool->name[FMC_POOL_MAX_POOL_NAME_LEN] = '\0';

    /* Init the other auxiliary members */
    
    pool->numOfAllocatedElements = 0;

    /* Mark all entries as free */
    FMC_OS_MemSet(pool->allocationMap, _FMC_POOL_FREE_ELEMENT_MAP_INDICATION,  FMC_POOL_MAX_NUM_OF_POOL_ELEMENTS);

    /* Fill memory in a special value to facilitate identification of dangling pointer usage */
    FMC_OS_MemSet((FMC_U8 *)pool->elementsMemory, _FMC_POOL_FREE_MEMORY_VALUE, numOfElements * allocatedSize);

    FmcPoolDebugAddPool(pool);
    
    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_Destroy(FmcPool   *pool)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    FMC_U32     numOfAllocatedElements;
    
    FMC_FUNC_START("FMC_POOL_Destroy");

    FMC_LOG_INFO(("Destroying Pool %s", pool->name));
        
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));

    /* It is illegal to destroy a pool while there are allocated elements in the pool */
    status = FMC_POOL_GetNumOfAllocatedElements(pool, &numOfAllocatedElements);
    FMC_VERIFY_ERR_NORET(   (0 == numOfAllocatedElements), 
                            ("Pool %s still has %d allocated elements", pool->name, numOfAllocatedElements));

    pool->elementsMemory = 0;
    pool->numOfElements = 0;
    pool->elementAllocatedSize = 0;
    pool->numOfAllocatedElements = 0;
    FMC_OS_StrCpy((FMC_U8*)pool->name, (const FMC_U8 *)"");
    
    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_Allocate(FmcPool *pool, void ** element)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    FMC_U32         allocatedIndex = _FMC_POOL_INVALID_ELEMENT_INDEX;

    FMC_FUNC_START("FMC_POOL_Allocate");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != element), FMC_STATUS_INTERNAL_ERROR, ("Null element argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
        
    allocatedIndex = _FMC_POOL_GetFreeElementIndex(pool);
    
    if (_FMC_POOL_INVALID_ELEMENT_INDEX == allocatedIndex)
    {
        return FMC_STATUS_NO_RESOURCES;
    }

    ++(pool->numOfAllocatedElements);
    pool->allocationMap[allocatedIndex] = _FMC_POOL_ALLOCATED_ELEMENT_MAP_INDICATION;

    *element = _FMC_POOL_GetElementAddressFromIndex(pool, allocatedIndex);
 
    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_Free(FmcPool *pool, void **element)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    /* Map the element's address to a map index */ 
    FMC_U32 freedIndex = _FMC_POOL_INVALID_ELEMENT_INDEX;

    FMC_FUNC_START("FMC_POOL_Free");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != element), FMC_STATUS_INTERNAL_ERROR, ("Null element argument"));
    FMC_VERIFY_FATAL((0 != *element), FMC_STATUS_INTERNAL_ERROR, ("Null *element argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
    
    freedIndex = _FMC_POOL_GetElementIndexFromAddress(pool, *element);

    FMC_VERIFY_FATAL((_FMC_POOL_INVALID_ELEMENT_INDEX != freedIndex), FMC_STATUS_INTERNAL_ERROR,
                        ("Invalid element address"));
    FMC_VERIFY_FATAL(   (_FMC_POOL_ALLOCATED_ELEMENT_MAP_INDICATION == pool->allocationMap[freedIndex]),
                        FMC_STATUS_INTERNAL_ERROR, ("Invalid element address"));
    
    pool->allocationMap[freedIndex] = _FMC_POOL_FREE_ELEMENT_MAP_INDICATION;
    --(pool->numOfAllocatedElements);

    /* Fill memory in a special value to facilitate identification of dangling pointer usage */
    FMC_OS_MemSet(  (FMC_U8 *)_FMC_POOL_GetElementAddressFromIndex(pool, freedIndex), 
            _FMC_POOL_FREE_MEMORY_VALUE, pool->elementAllocatedSize);

    /* Make sure caller's pointer stops pointing to the freed element */
    *element = 0;
    
    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_GetNumOfAllocatedElements(const FmcPool *pool, FMC_U32 *num)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    FMC_FUNC_START("FMC_POOL_GetNumOfAllocatedElements");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
    
    *num = pool->numOfAllocatedElements;

    FMC_FUNC_END();
    
    return status;
}

FmcStatus _FMC_POOL_GetCapacity(const FmcPool *pool, FMC_U32 *capacity)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    FMC_FUNC_START("_FMC_POOL_GetCapacity");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != capacity), FMC_STATUS_INTERNAL_ERROR, ("Null capacity argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));
    
    *capacity = pool->numOfElements;

    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_IsFull(FmcPool *pool, FMC_BOOL *answer)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    FMC_FUNC_START("FMC_POOL_IsFull");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != answer), FMC_STATUS_INTERNAL_ERROR, ("Null answer argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));

    if (pool->numOfAllocatedElements == pool->numOfElements)
    {
        *answer = FMC_TRUE;
    }
    else
    {
        *answer = FMC_FALSE;
    }

    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_POOL_IsElelementAllocated(FmcPool *pool, const void* element, FMC_BOOL *answer)
{
    FmcStatus   status = FMC_STATUS_SUCCESS;
    
    /* Map the element's address to a map index */ 
    FMC_U32 freedIndex = _FMC_POOL_INVALID_ELEMENT_INDEX;
    
    FMC_FUNC_START("FMC_POOL_IsElelementAllocated");
    
    FMC_VERIFY_FATAL((0 != pool), FMC_STATUS_INTERNAL_ERROR, ("Null pool argument"));
    FMC_VERIFY_FATAL((0 != element), FMC_STATUS_INTERNAL_ERROR, ("Null element argument"));
    FMC_VERIFY_FATAL((0 != answer), FMC_STATUS_INTERNAL_ERROR, ("Null answer argument"));
    FMC_VERIFY_FATAL((FMC_FALSE == _FMC_POOL_IsDestroyed(pool)), FMC_STATUS_INTERNAL_ERROR, ("Pool Already Destroyed"));

    freedIndex = _FMC_POOL_GetElementIndexFromAddress(pool, (void*)element);

    FMC_VERIFY_ERR((_FMC_POOL_INVALID_ELEMENT_INDEX != freedIndex), FMC_STATUS_INVALID_PARM,
                    ("Invalid element address"));

    if (_FMC_POOL_ALLOCATED_ELEMENT_MAP_INDICATION == pool->allocationMap[freedIndex])
    {
        *answer = FMC_TRUE;
    }
    else
    {
        *answer = FMC_FALSE;
    }

    FMC_FUNC_END();
    
    return status;
}

FMC_U32 _FMC_POOL_GetFreeElementIndex(FmcPool *pool)
{   
    FMC_U32 freeIndex = _FMC_POOL_INVALID_ELEMENT_INDEX;
    
    for (freeIndex = 0; freeIndex < pool->numOfElements; ++freeIndex)
    {
        if (_FMC_POOL_FREE_ELEMENT_MAP_INDICATION == pool->allocationMap[freeIndex])
        {
            return freeIndex;
        }
    }
        
    return _FMC_POOL_INVALID_ELEMENT_INDEX;
}

FMC_U32 _FMC_POOL_GetElementIndexFromAddress(FmcPool *pool, void *element)
{
    FMC_U32         elementAddress = (FMC_U32)element;
    FMC_U32         elementsMemoryAddress = (FMC_U32)(pool->elementsMemory);
    FMC_U32         index = _FMC_POOL_INVALID_ELEMENT_INDEX;

    FMC_FUNC_START("_FMC_POOL_GetElementIndexFromAddress");
    
    /* Verify that the specified address is valid (in the address range and on an element's boundary) */
    
    FMC_VERIFY_FATAL_SET_RETVAR(    (elementAddress >= elementsMemoryAddress), 
                                    index = _FMC_POOL_INVALID_ELEMENT_INDEX,
                                    ("Invalid element Address"));
    FMC_VERIFY_FATAL_SET_RETVAR(    (((elementAddress - elementsMemoryAddress) % pool->elementAllocatedSize) == 0),
                                    index = _FMC_POOL_INVALID_ELEMENT_INDEX, 
                                    ("Invalid element Address"));

    /* Calculate the index */
    index = (elementAddress - elementsMemoryAddress) / pool->elementAllocatedSize;

    /* Verify that its within index bounds */
    FMC_VERIFY_FATAL_SET_RETVAR((index < pool->numOfElements), index = _FMC_POOL_INVALID_ELEMENT_INDEX, (""));

    FMC_FUNC_END();
    
    return index;
}

void* _FMC_POOL_GetElementAddressFromIndex(FmcPool *pool, FMC_U32 elementIndex)
{
    /* we devide by 4 sinse pool->elementsMemory is FMC_U32 array .*/
    return &(pool->elementsMemory[elementIndex * pool->elementAllocatedSize / 4]);
}

/* DEBUG */

static FmcPool* FmcPoolDebugPools[20];
static FMC_BOOL FmcPoolDebugIsFirstPool = FMC_TRUE;

void FmcPoolDebugInit(void)
{
    FMC_OS_MemSet((FMC_U8*)FmcPoolDebugPools, 0, sizeof(FmcPool*));
}

void FmcPoolDebugAddPool(FmcPool *pool)
{
    FMC_U32 index = 0;
    
    if (FMC_TRUE == FmcPoolDebugIsFirstPool)
    {
        FmcPoolDebugInit();
        FmcPoolDebugIsFirstPool = FMC_FALSE;
    }

    for (index = 0; index < 20; ++index)
    {
        if (0 == FmcPoolDebugPools[index])
        {
            FmcPoolDebugPools[index] = pool;
            return;
        }
    }
    
}

void FMC_POOL_DEBUG_ListPools(void)
{
    FMC_U32 index = 0;

    FMC_FUNC_START("FMC_POOL_DEBUG_ListPools");

    FMC_LOG_DEBUG(("Pool List:\n"));
    
    for (index = 0; index < 20; ++index)
    {
        if (0 != FmcPoolDebugPools[index])
        {
            FMC_LOG_DEBUG(("Pool %s\n", FmcPoolDebugPools[index]->name));
        }
    }

    FMC_LOG_DEBUG(("\n"));

    FMC_FUNC_END();
}

void FMC_POOL_DEBUG_Print(char  *poolName)
{
    FMC_U32 index = 0;
    
    for (index = 0; index < 20; ++index)
    {
        if (0 != FmcPoolDebugPools[index])
        {
            if (0 == FMC_OS_StrCmp((const FMC_U8 *)poolName, (const FMC_U8 *)FmcPoolDebugPools[index]->name))
            {
                FMC_LOG_DEBUG(("%d elements allocated in Pool %s \n", 
                        FmcPoolDebugPools[index]->numOfAllocatedElements, FmcPoolDebugPools[index]->name));
                return;
            }
        }
    }
    
    FMC_LOG_DEBUG(("%s was not found\n", poolName));
}

FMC_BOOL _FMC_POOL_IsDestroyed(const FmcPool *pool)
{
    if (0 != pool->elementsMemory)
    {
        return FMC_FALSE;
    }
    else
    {
        return FMC_TRUE;
    }
}


