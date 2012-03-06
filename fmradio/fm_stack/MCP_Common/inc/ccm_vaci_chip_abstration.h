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
*   FILE NAME:      ccm_vaci_chip_abstracion.h
*
*   BRIEF:          This file defines the API of the Connectivity Chip
*                   Manager (CCM) chip abstraction layer (CAL).
*                  
*
*   DESCRIPTION:    The chip abstraction layer is used by the VAC to identify
*                   chip capabilities and configure resouurces for audio
*                   operations
*
*   AUTHOR:         Ram Malovany
*
\*******************************************************************************/
#ifndef __CCM_VACI_CHIP_ABTRACTION_H__
#define __CCM_VACI_CHIP_ABTRACTION_H__

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "ccm_audio_types.h"
#include "bt_hci_if.h"
#include "mcp_config_parser.h"

#define CAL_VAC_MAX_NUM_OF_RESOURCES_PER_OP         5
#define CAL_VAC_MAX_NUM_OF_OPT_RESOURCES_PER_OP     10
#define CAL_VAC_MAX_NUM_OF_OP_PAIRS_PER_RES         2 /* TODO ronen: whether this is the right place */

typedef enum _ECAL_RetValue
{
    CAL_STATUS_SUCCESS = 0,
    CAL_STATUS_PENDING,
    CAL_STATUS_FAILURE
} ECAL_RetValue;


typedef enum _ECAL_ChipVersion
{
    CAL_CHIP_6350 = 0,
    CAL_CHIP_6450_1_0,
    CAL_CHIP_6450_2,
    CAL_CHIP_1273,
    CAL_CHIP_1273_2,
    CAL_CHIP_1283,
    CAL_CHIP_5500
} ECAL_ChipVersion;

typedef struct _Cal_Config_ID Cal_Config_ID;

/* CAL CB - events to the VAC regarding the operation will be sent via this callback*/
typedef void (*TCAL_CB)(void *pUserData, ECAL_RetValue eRetValue);

typedef struct _TCAL_ResourceList
{
    ECAL_Resource   eResources[ CAL_VAC_MAX_NUM_OF_RESOURCES_PER_OP ];  /* list of resources */
    McpU32              uNumOfResources;                                    /* number of resources on list */
} TCAL_ResourceList;

typedef struct _TCAL_OptionalResource
{
    ECAL_Resource           eOptionalResource;      /* optional resource */
    TCAL_ResourceList     tDerivedResourcesList;  /* list of derived resources */
    McpBool                     bIsUsed;                /* indicates whetehr the resources are currently in use */
} TCAL_OptionalResource;

typedef struct _TCAL_OptionalResourcesLists
{
    TCAL_OptionalResource     tOptionalResourceLists[ CAL_VAC_MAX_NUM_OF_OPT_RESOURCES_PER_OP ];
                                        /* all possible optional resources */
    McpU32                          uNumOfResourceLists; /* number of possible optional resources */
} TCAL_OptionalResourcesLists;

typedef struct _TCAL_OperationToResourceMap
{
    ECAL_Operation                eOperation;             /* operation */
    TCAL_ResourceList             tMandatoryResources;    /* mandatory resources for the operation */
    TCAL_OptionalResourcesLists   tOptionalResources;     /* all possible optional resources for the operation */
} TCAL_OperationToResourceMap;

typedef struct _TCAL_Configuration
{
    TCAL_OperationToResourceMap   tOpToResMap[ CAL_OPERATION_MAX_NUM ]; 
                                            /* array of resource mapping for each operation */
} TCAL_MappingConfiguration;

typedef struct _TCAL_OperationPair
{
    ECAL_Operation                  eOperations[ 2 ]; /* two operations that can mutually own a resource */
} TCAL_OperationPair;


typedef struct _TCAL_OperationPairsList
{
    TCAL_OperationPair              tOpPairs[ CAL_VAC_MAX_NUM_OF_OP_PAIRS_PER_RES ]; 
                                                                /* list of allowed operation pairs */
    McpU32                          uNumOfAllowedPairs;         /* number of pairs */
} TCAL_OperationPairsList;


typedef struct _TCAL_OpPairConfig
{
    TCAL_OperationPairsList     tAllowedPairs[ CAL_RESOURCE_MAX_NUM ];
                                                    /* all allowed operations pairs, per resource */
} TCAL_OpPairConfig;

/*-------------------------------------------------------------------------------
 * TCAL_OperationSupport
 *
 * holds information regarding opeartion support acording to eChip_Version
 */
typedef struct _TCAL_OperationSupport
{
        McpBool bOperationSupport[ CAL_OPERATION_MAX_NUM ];
} TCAL_OperationSupport;


/*-------------------------------------------------------------------------------
 * TCAL_ResourceSupport
 *
 * holds information regarding resource support acording to eChip_Version
 */
typedef struct _TCAL_ResourceSupport
{
        McpBool bResourceSupport[ CAL_RESOURCE_MAX_NUM ];

} TCAL_ResourceSupport;



/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * CCM_CAL_StaticInit()
 *
 * Brief:  
 *      Initialize global CAL data structures
 *
 * Description:
 *      Initialize global CAL data structures
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      N/A
 */
void CCM_CAL_StaticInit(void);

/*-------------------------------------------------------------------------------
 * CAL_Create()
 *
 * Brief:  
 *      Initialized CAL structures according to chip id and return a pointer
 *      to a structure configuration according to the chip.
 *
 * Description:
 *      Initialized CAL structures according to chip id and return a pointer
 *      to a structure configuration according to the chip.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId      [in]        - chip logical identifier
 *      hciIfObj    [in]        - BT transport object
 *	  pConfigParser [in]	- A pointer to config parser object, including the VAC configuration file
 *      ppConfigid  [out]       - a pointer to the configuration structure 
 *	 
 * Returns:
 *      N/A
 */
void CAL_Create(McpHalChipId chipId, BtHciIfObj *hciIfObj, Cal_Config_ID **ppConfigid,McpConfigParser 			*pConfigParser);


/*-------------------------------------------------------------------------------
 * CAL_Destroy()
 *
 * Brief:  
 *      Destroys a CAL object
 *
 * Description:
 *      Destroys a CAL object
 *
 * Type:
 *      Synchronous
 *
  * Parameters:
 *      ppConfigid  [out]       - a pointer to the configuration structure 
 *
 * Returns:
 *      N/A
 */
void CAL_Destroy(Cal_Config_ID **ppConfigid);

/*-------------------------------------------------------------------------------
 * CCM_CAL_Configure()
 *
 * Brief:  
 *      Setting the chip version that is currently running on;
 *
 * Description:
 *      Setting the chip version that is running according to eChip_Version enum
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pConfigid       [in] - a pointer to structure configuration.
 *      projectType     [in] - the chip project type
 *      versionMajor    [in] - the chip major version (PG number)
 *      versionMinor    [in] - the chip minor version (FW version)
 *
 * Returns:
 *      N/A
 */

void CCM_CAL_Configure(Cal_Config_ID *pConfigid,
                              McpU16 projectType,
                              McpU16 versionMajor,
                              McpU16 versionMinor);

/*-------------------------------------------------------------------------------
 * CAL_GetAudioCapabilities()
 *
 * Brief:  
 *      gets the chip capabilities in terms of: supported operations, available 
 *      resources, and operations to resource mapping
 *
 * Description:
 *      gets the chip capabilities in terms of: supported operations, available 
 *      resources, and operations to resource mapping
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *          pConfigid [in] - idicate a pointer to stracture configuration.
 *      tOperationToResource [out] - pointer to a struct that holds information
 *          regarding mapping operation to resource acorrding to eChip_Version.
 *      tOpPairConfig [out] - pointer to a struct holding allowed operation 
 *          pairs
 *      tResource [out] - pointer to a struct that holds information regarding 
 *          resource support acording to eChip_Version
 *      tOperation [out] - pointer to a struct that holds information regarding
 *          opeartion support acording to eChip_Version
 *
 * Returns:
 *      N/A
 */
void CAL_GetAudioCapabilities (Cal_Config_ID *pConfigid,
                               TCAL_MappingConfiguration  *tOperationToResource,
                               TCAL_OpPairConfig *tOpPairConfig,
                               TCAL_ResourceSupport *tResource,
                               TCAL_OperationSupport *tOperation);


/*-------------------------------------------------------------------------------
 * CAL_ConfigResource()
 *
 * Brief:  
 *      Configure the chip resource acorrding to the operation and 
 *      and configuration sent by the client.
 *
 * Description:
 *      Configure the chip resource acorrding to the operation and 
 *      and configuration sent by the client.
 *
 * Type:
 *      Synchronous\Asynchronous
 *
 * Parameters:
 *      pConfigid [in] - idicate a pointer to stracture configuration.
 *      eOperation[in] - Operation at the current configuration.
 *      eResource [in] - Resource to configure.
 *      ptConfig [in] - Configuration for the current operation.
 *      ptProperties [in] - Resource specific properties (e.g. SCO handle, I2S or PCM for FM-IF)
 *      fCB [in] - Client callbackfor the current operation.
 *      pUserData [in] - Pointer for a data specific for the current 
 *              operation.
 *
 * Generated Events:
 *      CAL_STATUS_SUCCESS 
 *      CAL_STATUS_FAILURE
 *
 * Returns:
 *
 *      CAL_STATUS_PENDING - The config resource operation was started 
 *          successfully.A CAL_STATUS_SUCCESS event will be received 
 *          when the configuration has been succesfully done.
 *          If the configuration failed the CAL_STATUS_FAILURE event
 *          will be received.
 *
 *      CAL_STATUS_SUCCESS - The configuration was successfuly
 *
 *      CAL_STATUS_FAILURE - The configuration failed.
 *
 *
 */

ECAL_RetValue CAL_ConfigResource(Cal_Config_ID *pConfigid,
                                 ECAL_Operation eOperation,
                                 ECAL_Resource eResource,
                                 TCAL_DigitalConfig *ptConfig,
                                 TCAL_ResourceProperties *ptProperties,
                                 TCAL_CB fCB,
                                 void *pUserData);

/*-------------------------------------------------------------------------------
 * CAL_StopResourceConfiguration()
 *
 * Brief:  
 *      Stop and undo configuration for a given resource and operation.
 *
 * Description:
 *      Stop and undo configuration for a given resource and operation. For most 
 *      resources and operations, this functio will simply cancel a configuration 
 *      sequence initiated by CAL_ConfigResource. Only for PCM-IF and FM RX over 
 *      SCO, this function will both cancel the current configuration sequence 
 *      and start a new de-configuration sequence.
 *      For future-proofing the CAL user must call this function for every 
 *      resource that was previously configured using CAL_ConfigResource when
 *      an operation is stopped, and act according to the return value.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pConfigid [in] - idicate a pointer to stracture configuration.
 *      eOperation[in] - Operation at the current configuration.
 *      eResource [in] - Resource to configure.
 *      ptProperties [in] - Resource specific properties (e.g. SCO handle, I2S or PCM for FM-IF)
 *      fCB [in] - Client callbackfor the current operation.
 *      pUserData [in] - Pointer for a data specific for the current 
 *              operation.
 *
 * Generated Events:
 *      CAL_STATUS_SUCCESS 
 *      CAL_STATUS_FAILURE
 *
 * Returns:
 *      CAL_STATUS_PENDING - The config resource operation was started 
 *          successfully.A CAL_STATUS_SUCCESS event will be received 
 *          when the configuration has been succesfully done.
 *          If the configuration failed the CAL_STATUS_FAILURE event
 *          will be received.
 *
 *      CAL_STATUS_SUCCESS - The configuration was successfuly
 *
 *      CAL_STATUS_FAILURE - The configuration failed.
 *
 *
 */
ECAL_RetValue CAL_StopResourceConfiguration (Cal_Config_ID *pConfigid,
                                             ECAL_Operation eOperation,
                                             ECAL_Resource eResource,
                                             TCAL_ResourceProperties *ptProperties,
                                             TCAL_CB fCB,
                                             void *pUserData);

/*-------------------------------------------------------------------------------
 *
 * Brief:  
 *      Return a resource according to string;
 *
 * Description:
 *      Return a resource according to string;
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      str1 [in] - a string that indiate a resource
 *
 *
  * Returns:
 *      CAL_RESOURCE_INVALID - When the resource cannot be found
 *      CAL_VAC_RESOURCE_* - When the resource is found;
 *
 *
 */
ECAL_Resource StringtoResource(const McpU8 *str1);

/*-------------------------------------------------------------------------------
 *
 * Brief:  
 *      Return an operation according to string;
 *
 * Description:
 *      Return an operation according to string;
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      str1 [in] - a string that indiate an operation
 *
 *
  * Returns:
 *      CAL_OPERATION_INVALID - When the resource cannot be found
 *      CAL_VAC_OPERATION_* - When the resource is found;
 *
 *
 */
ECAL_Operation StringtoOperation(const McpU8 *str1);


#endif /* __CCM_VACI_CHIP_ABTRACTION_H__ */

