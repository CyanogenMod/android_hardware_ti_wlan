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
*   FILE NAME:      ccm_vaci_allocation_engine.h
*
*   BRIEF:          This file defines the internal API of the Connectivity Chip
*                   Manager (CCM) Voice and Audio Control (VAC) allocation 
*                   engine component.
*                  
*
*   DESCRIPTION:    The Allocation engine is a CCM-VAC internal module storing
*                   audio resources allocations to operations.
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/
#ifndef __CCM_VACI_ALLOC_ENGINE_H__
#define __CCM_VACI_ALLOC_ENGINE_H__

#include "ccm_vac.h"
#include "mcp_config_parser.h"
#include "ccm_audio_types.h"

/* forward declarations */
typedef struct _TCCM_VAC_AllocationEngine TCCM_VAC_AllocationEngine;

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_Create()
 *
 * Brief:  
 *      Creates an allocation engine object.
 *      
 * Description:
 *      Creates the allocation engine object.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId          [in]    - the chip ID for which this object is created
 *      pConfigParser   [in]    - a config parser object, including the VAC configuration file
 *      ptAllocEngine   [out]   - the allocation engine handle
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - creation succeeded
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED  - creation failed
 */
ECCM_VAC_Status _CCM_VAC_AllocationEngine_Create (McpHalChipId chipId,
                                                  McpConfigParser *pConfigParser,
                                                  TCCM_VAC_AllocationEngine **ptAllocEngine);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_Configure()
 *
 * Brief:  
 *      Configures an allocation engine object.
 *      
 * Description:
 *      Configures the allocation engine object. Read configuration from ini file,
 *      match with chip capabilities and initializes all resources accordingly.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine       [in]    - the allocation engine handle
 *      ptAvailResources    [in]    - the chip available resources
 *      ptAvailOperations   [in]    - the chip available operations
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - configuration succeeded
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED  - configuration failed
 */
ECCM_VAC_Status _CCM_VAC_AllocationEngine_Configure (TCCM_VAC_AllocationEngine *ptAllocEngine, 
                                                     TCAL_ResourceSupport *ptAvailResources,
                                                     TCAL_OperationSupport *ptAvailOperations);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_Destroy()
 *
 * Brief:  
 *      Destroy an allocation engine object.
 *      
 * Description:
 *      Destroy an allocation engine object. 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine    [in]    - the allocation engine handle
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_AllocationEngine_Destroy (TCCM_VAC_AllocationEngine **ptAllocEngine);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_TryAllocate()
 *
 * Brief:  
 *      Attempts to allocate a resource.
 *      
 * Description:
 *      Attempts to allocate a resource for the specified operation. If the 
 *      resource is already allocated for an operation that cannot mutually allocate
 *      this resource, a negative reply is returned 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine   [in]     - the allocation object handle
 *      eResource       [in]     - the resource to allocate
 *      peOperation     [in/out] - the operation requesting the resource / 
 *                                 the operation currently owning the resource (if any)
 *
 * Returns:
 *      MCP_TRUE    - the resource was allocated
 *      MCP_FALSE   - the resource is already allocated. Current owner is indicated 
 *                    by peOperation
 */
McpBool _CCM_VAC_AllocationEngine_TryAllocate (TCCM_VAC_AllocationEngine *ptAllocEngine,
                                               ECAL_Resource eResource, 
                                               ECAL_Operation *peOperation);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_Release()
 *
 * Brief:  
 *      Release a resource.
 *      
 * Description:
 *      Release a resource that was previously allocated.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine   [in]    - the allocation object handle
 *      eResource       [in]    - the resource to release
 *      eOperation      [in]    - the operation releasing the resource
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_AllocationEngine_Release (TCCM_VAC_AllocationEngine *ptAllocEngine,
                                        ECAL_Resource eResource,
                                        ECAL_Operation eOperation);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_SetResourceProperties()
 *
 * Brief:  
 *      Set properties for a resource
 *      
 * Description:
 *      Set unique properties per resource, e.g. SCO handle, that may be used
 *      later for resource configuration
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine   [in]    - the allocation object handle
 *      eResource       [in]    - the resource to release
 *      pProperties     [in]    - new properties to keep
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_AllocationEngine_SetResourceProperties (TCCM_VAC_AllocationEngine *ptAllocEngine,
                                                      ECAL_Resource eResource,
                                                      TCAL_ResourceProperties *pProperties);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_GetResourceProperties()
 *
 * Brief:  
 *      Get properties for a resource
 *      
 * Description:
 *      Get unique properties per resource, e.g. SCO handle, that may be used
 *      later for resource configuration
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine   [in]    - the allocation object handle
 *      eResource       [in]    - the resource to release
 *
 * Returns:
 *      The resource properties structure
 */
TCAL_ResourceProperties *_CCM_VAC_AllocationEngine_GetResourceProperties (TCCM_VAC_AllocationEngine *ptAllocEngine,
                                                                          ECAL_Resource eResource);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_AllocationEngine_GetConfigData()
 *
 * Brief:  
 *      Retrieves the allocation engine configuration object
 *      
 * Description:
 *      Retrieves the allocation engine configuration object, to be updated with
 *      allowed opeartion pairs
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptAllocEngine   [in]    - the allocation object handle
 *
 * Returns:
 *      The allocation engine configuration object
 */
TCAL_OpPairConfig *_CCM_VAC_AllocationEngine_GetConfigData (TCCM_VAC_AllocationEngine *ptAllocEngine);

#endif /* __CCM_VACI_ALLOC_ENGINE_H__ */
