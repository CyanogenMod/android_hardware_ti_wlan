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
*   FILE NAME:      ccm_audio_types.h
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
#ifndef __CCM_AUDIO_TYPES_H__
#define __CCM_AUDIO_TYPES_H__

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_defs.h"
#include "ccm_config.h"

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
#define CCM_VAC_MAX_NUM_OF_RESOURCES_PER_OP 5 /* TODO ronen: understand if the number is correct */

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * ECAL_Resource type
 *
 *     Enumerates the different audio resources that are controlled by the CCM-VAC
 */
typedef enum _ECAL_VAC_Resource
{
    CAL_RESOURCE_I2SH = 0,
    CAL_RESOURCE_PCMH,
    CAL_RESOURCE_PCMT_1,
    CAL_RESOURCE_PCMT_2,
    CAL_RESOURCE_PCMT_3,
    CAL_RESOURCE_PCMT_4,
    CAL_RESOURCE_PCMT_5,
    CAL_RESOURCE_PCMT_6,
    CAL_RESOURCE_FM_ANALOG,
    CAL_RESOURCE_LAST_EX_RESOURCE = CAL_RESOURCE_FM_ANALOG,
    CAL_RESOURCE_PCMIF,
    CAL_RESOURCE_FMIF,
    CAL_RESOURCE_CORTEX,    
    CAL_RESOURCE_FM_CORE,
    CAL_RESOURCE_MAX_NUM,
    CAL_RESOURCE_INVALID
} ECAL_Resource;

/*-------------------------------------------------------------------------------
 * ECAL_ResourceMask type
 *
 *     Bitmask for the different resources that can be configured for the 
 *     different operations
 */
typedef enum _ECAL_ResourceMask
{
    CAL_RESOURCE_MASK_NONE          = 0,
    CAL_RESOURCE_MASK_I2SH          = 1 << CAL_RESOURCE_I2SH,
    CAL_RESOURCE_MASK_PCMH          = 1 << CAL_RESOURCE_PCMH,
    CAL_RESOURCE_MASK_PCMT_1        = 1 << CAL_RESOURCE_PCMT_1,
    CAL_RESOURCE_MASK_PCMT_2        = 1 << CAL_RESOURCE_PCMT_2,
    CAL_RESOURCE_MASK_PCMT_3        = 1 << CAL_RESOURCE_PCMT_3,
    CAL_RESOURCE_MASK_PCMT_4        = 1 << CAL_RESOURCE_PCMT_4,
    CAL_RESOURCE_MASK_PCMT_5        = 1 << CAL_RESOURCE_PCMT_5,
    CAL_RESOURCE_MASK_PCMT_6        = 1 << CAL_RESOURCE_PCMT_6,
    CAL_RESOURCE_MASK_FM_ANALOG     = 1 << CAL_RESOURCE_FM_ANALOG,
} ECAL_ResourceMask;

/*-------------------------------------------------------------------------------
 * ECAL_Operation type
 *
 *     Enumerates the different audio operations which are synchronized by 
 *     the VAC
 */
typedef enum _ECAL_Operation
{
    CAL_OPERATION_FM_TX = 0,
    CAL_OPERATION_FM_RX,
    CAL_OPERATION_A3DP,
    CAL_OPERATION_BT_VOICE,
    CAL_OPERATION_WBS,
    CAL_OPERATION_AWBS,
    CAL_OPERATION_FM_RX_OVER_SCO,
    CAL_OPERATION_FM_RX_OVER_A3DP,
    CAL_OPERATION_MAX_NUM,
    CAL_OPERATION_INVALID
} ECAL_Operation;

/*-------------------------------------------------------------------------------
 * ECAL_SampleFrequency type
 *
 *     Enumerates the different sample frequencies supported by the physical 
 *     audio channels
 */
typedef enum _ECAL_SampleFrequency
{
    CAL_SAMPLE_FREQ_8000 = 0,
    CAL_SAMPLE_FREQ_11025,
    CAL_SAMPLE_FREQ_12000,
    CAL_SAMPLE_FREQ_16000,
    CAL_SAMPLE_FREQ_22050,
    CAL_SAMPLE_FREQ_24000,
    CAL_SAMPLE_FREQ_32000,
    CAL_SAMPLE_FREQ_44100,
    CAL_SAMPLE_FREQ_48000
} ECAL_SampleFrequency;


/*-------------------------------------------------------------------------------
 * TCAL_DigitalConfig type
 *
 *     Defines the configuration information for a digital audio bus - 
 *     sample frequency and number of channels
 */
typedef struct _TCAL_DigitalConfig
{
    ECAL_SampleFrequency        eSampleFreq;
    McpU32                      uChannelNumber;
} TCAL_DigitalConfig;

/*-------------------------------------------------------------------------------
 * TCCM_VAC_ResourceProperty type
 *
 *     defines a type for resource property data
 */
typedef McpU32 TCAL_ResourceProperty;

/*-------------------------------------------------------------------------------
 * TCCM_VAC_ResourceProperties type
 *
 *     structure holding all properties of a resource (currently only one)
 */
typedef struct _TCCM_VAC_ResourceProperties
{
    TCAL_ResourceProperty   tResourcePorpeties[ CCM_CAL_MAX_NUMBER_OF_PROPERTIES_PER_RESOURCE ];
    McpU32                  uPropertiesNumber;
} TCAL_ResourceProperties;

/*-------------------------------------------------------------------------------
 * TCCM_VAC_ResourceOwner type
 *
 *     Defines the owner of a certain resource (by means of a VAC operation).
 */
typedef struct _TCCM_VAC_ResourceOwner
{
    ECAL_Resource       eResource;
    ECAL_Operation      eOperation;
} TCCM_VAC_ResourceOwner;

/*-------------------------------------------------------------------------------
 * TCCM_VAC_UnavailResourceList type
 *
 *     Defines a list of resources which are currently unavailable. For each 
 *     resource, the operation currently owning it is indicated.
 */
typedef struct _TCCM_VAC_UnavailResourceList
{
    TCCM_VAC_ResourceOwner  tUnavailResources[ CCM_VAC_MAX_NUM_OF_RESOURCES_PER_OP ];
    McpU32                  uNumOfUnavailResources;
} TCCM_VAC_UnavailResourceList;

#endif /* __CCM_AUDIO_TYPES_H__ */

