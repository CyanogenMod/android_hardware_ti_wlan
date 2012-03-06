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
*   BRIEF:          This file defines the API of the Connectivity Chip Manager (CCM)
*                   Voice and Audio Control (VAC) component.
*                  
*
*   DESCRIPTION:    The CCM-VAC is used by different stacks audio and voice use-cases
*                   for synchronization and configuration of baseband resources, such
*                   as PCM and I2S bus and internal resources.
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/
#ifndef __CCM_VAC_H__
#define __CCM_VAC_H__

#include "mcp_hal_types.h"
#include "ccm_audio_types.h"
#include "ccm_config.h"

/*-------------------------------------------------------------------------------
 * ECCM_VAC_Status type
 *
 *     Enumerates the status codes returned by VAC API and in the VAC callback
 */
typedef enum _ECCM_VAC_Status
{
    CCM_VAC_STATUS_SUCCESS = 0,
    CCM_VAC_STATUS_PENDING,
    CCM_VAC_STATUS_FAILURE_UNSPECIFIED,
    CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES,
    CCM_VAC_STATUS_FAILURE_BUSY,
    CCM_VAC_STATUS_FAILURE_INVALID_CONFIGURATION,
    CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED
} ECCM_VAC_Status;

/*-------------------------------------------------------------------------------
 * ECCM_VAC_Event type
 *
 *     Enumerates the different events notified by the VAC callback.
 */
typedef enum _ECCM_VAC_Event
{
    CCM_VAC_EVENT_OPERATION_STARTED = 0,
    CCM_VAC_EVENT_OPERATION_STOPPED,
    CCM_VAC_EVENT_RESOURCE_CHANGED,
    CCM_VAC_EVENT_CONFIGURATION_CHANGED
} ECCM_VAC_Event;

/*-------------------------------------------------------------------------------
 * TCCM_VAC_Callback type
 *
 *     Defines the function prototype of the CCM-VAC callback. A function of 
 *     this type must be registered for each VAC operation before the operation
 *     is started.
 */
typedef void (*TCCM_VAC_Callback)(ECAL_Operation eOperation,
                                  ECCM_VAC_Event eEvent, 
                                  ECCM_VAC_Status eStatus);

#include "ccm_vaci_configuration_engine.h" /* TODO ronen: understand if there's another way */

/*-------------------------------------------------------------------------------
 * TCCM_VAC_Object type
 *
 *     Defines the VAC object type
 */
typedef struct _TCCM_VAC_ConfigurationEngine TCCM_VAC_Object;

/*-------------------------------------------------------------------------------
 * CCM_VAC_RegisterCallback()
 *
 * Brief:  
 *      Registers an operation callback with the VAC
 *      
 * Description:
 *      Registers a callback function to be used for an operation. Should be called
 *      for every operation before it is requested from the VAC.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptVac       [in]    - the VAC object handle
 *      eOperation  [in]    - the operation for which this callback should be used
 *      fCB         [in]    - the callback function
 *
 * Returns:
 *      N/A
 */
void CCM_VAC_RegisterCallback (TCCM_VAC_Object *ptVac,
                               ECAL_Operation eOperation, 
                               TCCM_VAC_Callback fCB);

/*-------------------------------------------------------------------------------
 * CCM_VAC_StartOperation()
 *
 * Brief:  
 *      Request an operation to be started
 *      
 * Description:
 *      Request the VAC to start an operation with a specific configuration.
 *      The VAC will attempt to allocate the required resources for the operation
 *      and config them, according to the supplied configuration structure (if 
 *      applicable). If not all resources are available, the list of
 *      unavailable resources will be returned in ptUnavailResources.
 *
 * Type:
 *      Synchronous / A-Synchronous
 *
 * Parameters:
 *      ptVac               [in]    - the VAC object handle
 *      eOperation          [in]    - the operation to start
 *      ptConfig            [in]    - configuration for this operation. Not
 *                                    required for FM analog
 *      ptUnavailResources  [out]   - list of unavailable resources
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS                         - The operation has started successfully
 *      CCM_VAC_STATUS_PENDING                         - Operation start is pending and will be 
 *                                                       indicated by an event
 *      CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES   - Operation could not be started due to
 *                                                       resource(s) unavailability. Unavailable
 *                                                       resource(s) and their current owner(s) are
 *                                                       indicated by ptUnavailResources.
 *      CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED - The requested operation is not supported
 *                                                       by the chip or the host
 *      CCM_VAC_STATUS_FAILURE_BUSY                    - A call to _CCM_VAC_ConfigurationEngine_StopOperation
 *                                                       to stop the opeartion is still pending
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED             - Operation could not be started due to 
 *                                                       unspecified error (bug)
 */
ECCM_VAC_Status CCM_VAC_StartOperation (TCCM_VAC_Object *ptVac,
                                        ECAL_Operation eOperation,
                                        TCAL_DigitalConfig *ptConfig,
                                        TCCM_VAC_UnavailResourceList *ptUnavailResources);

/*-------------------------------------------------------------------------------
 * CCM_VAC_StopOperation()
 *
 * Brief:  
 *      Request an operation to be stopped
 *      
 * Description:
 *      Request the VAC to stop an operation. The VAC will release all resources
 *      that were allocated to this operation.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      ptVac               [in]    - the VAC object handle
 *      eOperation          [in]    - the operation to stop
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS                         - The operation has stopped successfully
 *      CCM_VAC_STATUS_PENDING                         - Operation stop is pending and will be 
 *                                                       indicated by an event
 *      CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED - The requested operation is not supported
 *                                                       by the chip or the host
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED             - Operation could not be stopped due to 
 *                                                       unspecified error
 */
ECCM_VAC_Status CCM_VAC_StopOperation (TCCM_VAC_Object *ptVac,
                                       ECAL_Operation eOperation);

/*-------------------------------------------------------------------------------
 * CCM_VAC_ChangeResource()
 *
 * Brief:  
 *      Request to change resources used by an operation
 *      
 * Description:
 *      Request the VAC to change current resources used by an operation. If the 
 *      operation is currently running, this will cause the VAC to attempt to allocate
 *      the new resources and config them. If the new resources are available, mapping
 *      will be switched to the new resources and they will be configured. If not all 
 *      resource(s) are available, the list of unavailable resources is returned in 
 *      ptUnavailResources, and the operation continue to run using the old resources.
 *      If the operation is not running, mapping will be changed for next operation runs.
 *
 * Type:
 *      Synchronous / A-Synchronous
 *
 * Parameters:
 *      ptVac               [in]    - the VAC object handle
 *      eOperation          [in]    - the operation to start
 *      eResourceMask       [in]    - new resources to use
 *      ptConfig            [in]    - configuration for this operation
 *      ptUnavailResources  [out]   - list of unavailable resources
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS                      - resources had been changed successfully
 *      CCM_VAC_STATUS_PENDING                      - resource change is pending and will be 
 *                                                    indicated by an event
 *      CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES- resources could not be changed due to
 *                                                    resource(s) unavailability. Unavailable
 *                                                    resource(s) and their current owner(s) are
 *                                                    indicated by ptUnavailResources.
 *      CCM_VAC_STATUS_FAILURE_BUSY -                 The operation is still being configured,
 *                                                    either due to a call to _CCM_VAC_ConfigurationEngine_StartOperation,
 *                                                    _CCM_VAC_ConfigurationEngine_StopOperation or
 *                                                    this function
 *      CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED - 
 *                                                    operation is not supported by chip or host
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED          - resources could not be changed due to 
 *                                                    unspecified error (bug)
 */
ECCM_VAC_Status CCM_VAC_ChangeResource (TCCM_VAC_Object *ptVac,
                                        ECAL_Operation eOperation, 
                                        ECAL_ResourceMask eResourceMask, 
                                        TCAL_DigitalConfig *ptConfig, 
                                        TCCM_VAC_UnavailResourceList *ptUnavailResources);

/*-------------------------------------------------------------------------------
 * CCM_VAC_ChangeConfiguration()
 *
 * Brief:  
 *      Request to change resources configuration for an operation
 *      
 * Description:
 *      Request the VAC to change current resources' configuration for a specific 
 *      operation. 
 *
 * Type:
 *      Synchronous / A-Synchronous
 *
 * Parameters:
 *      ptVac               [in]    - the VAC object handle
 *      eOperation          [in]    - the operation to start
 *      ptConfig            [in]    - new configuration for this operation
 *
 * Returns:
 *      CCM_VAC_STATUS_SUCCESS                      - configuration has been changed successfully
 *      CCM_VAC_STATUS_PENDING                      - configuration change is pending and will be 
 *                                                    indicated by an event
 *      CCM_VAC_STATUS_FAILURE_BUSY -                 The operation is still being configured,
 *                                                    either due to a call to 
 *                                                    _CCM_VAC_ConfigurationEngine_StartOperation,
 *                                                    _CCM_VAC_ConfigurationEngine_StopOperation,
 *                                                    _CCM_VAC_ConfigurationEngine_ChangeResource
 *                                                    or this function
 *      CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED - 
 *                                                    operation is not supported by chip or host
 *      CCM_VAC_STATUS_FAILURE_UNSPECIFIED          - resources configuration could not be 
 *                                                    changed due to unspecified error 
 *                                                    (input values)
 */
ECCM_VAC_Status CCM_VAC_ChangeConfiguration (TCCM_VAC_Object *ptVac,
                                             ECAL_Operation eOperation, 
                                             TCAL_DigitalConfig *ptConfig);

/*-------------------------------------------------------------------------------
 * CCM_VAC_SetResourceProperties()
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
void CCM_VAC_SetResourceProperties (TCCM_VAC_Object *ptVac,
                                    ECAL_Resource eResource,
                                    TCAL_ResourceProperties *pProperties);

#endif /* __CCM_VAC_H__ */

