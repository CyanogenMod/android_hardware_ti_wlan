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
*   FILE NAME:      ccm_vac.h
*
*   BRIEF:          This file defines the internal API of the Connectivity Chip 
*                   Manager (CCM) Voice and Audio Control (VAC) component.
*                  
*
*   DESCRIPTION:    The CCM-VAC is used by different stacks audio and voice use-cases
*                   for synchronization and configuration of baseband resources, such
*                   as PCM and I2S bus and internal resources.
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/
#ifndef __CCM_VACI_H__
#define __CCM_VACI_H__

#include "ccm_vac.h"
#include "ccm_vaci_configuration_engine.h"
#include "mcp_hal_defs.h"
#include "ccm_vaci_chip_abstration.h"

/*-------------------------------------------------------------------------------
 * CCM_VAC_StaticInit()
 *
 * Brief:  
 *      Initialize static data (related to all chips)
 *      
 * Description:
 *      Initialize data that is not unique pre chip
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - Initialization succeeded
 */
ECCM_VAC_Status CCM_VAC_StaticInit (void);

/*-------------------------------------------------------------------------------
 * CCM_VAC_Create()
 *
 * Brief:  
 *      Creates a VAC instance for a specific chip
 *      
 * Description:
 *      Initializes all VAC data relating to a specific chip
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId      [in]     - the chip id for which the VAC object is created
 *      pCAL        [in]     - a pointer to the CAL object
 *	  pConfigParser	[in]	- A pointer to config parser object, including the VAC configuration file
 *      this        [out]    - the VAC object
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS              - Creation succeeded
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED  - Creation failed
 */
ECCM_VAC_Status CCM_VAC_Create (McpHalChipId chipId,
                                Cal_Config_ID *pCal,
                                TCCM_VAC_Object **thisObj,
                                McpConfigParser 			*pConfigParser);

/*-------------------------------------------------------------------------------
 * CCM_VAC_Configure()
 *
 * Brief:  
 *      Configures a VAC instance for a specific chip
 *      
 * Description:
 *      Match ini file information with chip capabilities
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      this        [in]     - the VAC object
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS                          - Configuration succeeded
 *      CCM_VAC_STATUS_FAILURE_INVALID_CONFIGURATION    - Configuration failed
 */
ECCM_VAC_Status CCM_VAC_Configure (TCCM_VAC_Object *thisObj);

/*-------------------------------------------------------------------------------
 * CCM_VAC_Destroy()
 *
 * Brief:  
 *      Destroies a VAC instance for a specific chip
 *      
 * Description:
 *      de-initializes all VAC data relating to a specific chip
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      this        [in]     - the VAC object
 *
 * Returns:
 *      N/A
 */
void CCM_VAC_Destroy (TCCM_VAC_Object **thisObj);

#endif /* __CCM_VACI_H__ */
