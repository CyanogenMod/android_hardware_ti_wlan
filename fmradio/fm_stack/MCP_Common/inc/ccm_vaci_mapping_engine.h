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
*   FILE NAME:      ccm_vaci_mapping_engine.h
*
*   BRIEF:          This file defines the internal API of the Connectivity Chip
*                   Manager (CCM) Voice and Audio Control (VAC) mapping
*                   engine component.
*                  
*
*   DESCRIPTION:    The mapping engine is a CCM-VAC internal module storing
*                   possible and current mapping between operations and 
*                   audio resources.
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/
#ifndef __CCM_VACI_MAPPING_ENGINE_H__
#define __CCM_VACI_MAPPING_ENGINE_H__

#include "ccm_vac.h"
#include "mcp_hal_defs.h"

#include "mcp_config_parser.h"
#include "ccm_audio_types.h"

/*-------------------------------------------------------------------------------
 * TCCM_VAC_MEResourceList type
 *
 *     List of resources
 */
typedef struct _TCCM_VAC_MEResourceList
{
    ECAL_Resource   eResources[ CCM_VAC_MAX_NUM_OF_RESOURCES_PER_OP ];  /* list of resources */
    McpU32              uNumOfResources;                                    /* number of resources on list */
} TCCM_VAC_MEResourceList;

/* forward declarations */
typedef struct _TCCM_VAC_MappingEngine TCCM_VAC_MappingEngine;

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_Create()
 *
 * Brief:  
 *      Creates a mapping engine object.
 *      
 * Description:
 *      Creates the mapping engine object. Read configuration from ini file
 *      (including default values) and capabilities from CAL to form the initial
 *      operation to resource mapping.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId          [in]    - The chip ID for which this object is created
 *      pConfigParser   [in]    - A config parser object, including the VAC configuration file
 *      ptMappEngine    [out]   - The mapping engine handle
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - creation succeeded
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED  - creation failed
 */
ECCM_VAC_Status _CCM_VAC_MappingEngine_Create (McpHalChipId chipId,
                                               McpConfigParser *pConfigParser,
                                               TCCM_VAC_MappingEngine **ptMappEngine);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_Configure()
 *
 * Brief:  
 *      Configures a mapping engine object.
 *      
 * Description:
 *      Configures the mapping engine object. Read configuration from ini file,
 *      match with chip capabilities and initializes resource to allocation mapping
 *      accordingly.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine        [in]    - the mapping engine handle
 *      ptAvailResources    [in]    - the chip available resources
 *      ptAvailOperations   [in]    - the chip available operations
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - configuration succeeded
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED  - configuration failed
 */
ECCM_VAC_Status _CCM_VAC_MappingEngine_Configure (TCCM_VAC_MappingEngine *ptMappEngine, 
                                                  TCAL_ResourceSupport *ptAvailResources,
                                                  TCAL_OperationSupport *ptAvailOperations);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_Destroy()
 *
 * Brief:  
 *      Destroy a mapping engine object.
 *      
 * Description:
 *      Destroy the mapping engine object. 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine    [in]    - the mapping engine handle
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_MappingEngine_Destroy (TCCM_VAC_MappingEngine **ptMappEngine);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_OperationToResourceList()
 *
 * Brief:  
 *      Returns the current resources mapped to an operation
 *      
 * Description:
 *      Searches current mapping for all resources currently mapped to an operation
 *      (whether it is running or not) and return it 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine    [in]    - the mapping engine handle
 *      eOperation      [in]    - the operation for which resource list is required
 *      ptResourceList  [out]   - a pointer to a resource list to be filled with 
 *                                required resources
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_MappingEngine_OperationToResourceList (TCCM_VAC_MappingEngine *ptMappEngine, 
                                                     ECAL_Operation eOperation,
                                                     TCAL_ResourceList *ptResourceList);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_GetOptionalResourcesList()
 *
 * Brief:  
 *      Returns the current optional resources used by an operation
 *      
 * Description:
 *      Searches current mapping for all optional resources (w/o their derived resources)
 *      mapped to an operation 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine    [in]    - the mapping engine handle
 *      eOperation      [in]    - the operation for which resource list is required
 *      ptResourceList  [out]   - a pointer to a resource list to be filled with 
 *                                optional resources
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_MappingEngine_GetOptionalResourcesList (TCCM_VAC_MappingEngine *ptMappEngine,
                                                      ECAL_Operation eOperation,
                                                      TCAL_ResourceList *ptResourceList);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_SetOptionalResourcesList()
 *
 * Brief:  
 *      Sets optional resources used by an operation
 *      
 * Description:
 *      Sets the optional resources to be used by an operation. Resources that
 *      were in use but are not included in the new list will be removed, whereas
 *      resources that that were not in use but are include in the list will be added 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine    [in]    - the mapping engine handle
 *      eOperation      [in]    - the operation for which resource list is required
 *      ptResourceList  [out]   - a pointer to a resource list including new optional
 *                                resources
 *
 * Returns:
 *      N/A
 */
void _CCM_VAC_MappingEngine_SetOptionalResourcesList (TCCM_VAC_MappingEngine *ptMappEngine,
                                                      ECAL_Operation eOperation,
                                                      TCAL_ResourceList *ptResourceList);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_MappingEngine_GetConfigData()
 *
 * Brief:  
 *      Retrieves the mapping engine configuration object
 *      
 * Description:
 *      Retrieves the mapping engine configuration object, to be updated with
 *      chip capabilities
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptMappEngine    [in]    - the mapping engine handlee
 *
 * Returns:
 *      A pointer to the mapping configuration object
 */
TCAL_MappingConfiguration *_CCM_VAC_MappingEngine_GetConfigData (TCCM_VAC_MappingEngine *ptMappEngine);

#endif /* __CCM_VACI_MAPPING_ENGINE_H__ */

