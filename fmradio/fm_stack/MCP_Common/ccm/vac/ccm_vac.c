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
*   FILE NAME:      ccm_vac.c
*
*   BRIEF:          This file implements stubs for the VAC API
*                  
*
*   DESCRIPTION:    This file implements stubs for the VAC API
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/

#include "mcp_defs.h"
#include "ccm_vac.h"
#include "ccm_vaci.h"

ECCM_VAC_Status CCM_VAC_StaticInit (void)
{
    return CCM_VAC_STATUS_SUCCESS;
}

ECCM_VAC_Status CCM_VAC_Create (McpHalChipId chipId,
                                Cal_Config_ID *pCal,
                                TCCM_VAC_Object **thisObj,
                                McpConfigParser 			*pConfigParser)
{
    MCP_UNUSED_PARAMETER (chipId);
    MCP_UNUSED_PARAMETER (pCal);

    *thisObj = (TCCM_VAC_Object *)1;

    return CCM_VAC_STATUS_SUCCESS;
}

ECCM_VAC_Status CCM_VAC_Configure (TCCM_VAC_Object *thisObj)
{
    MCP_UNUSED_PARAMETER (thisObj);

    return CCM_VAC_STATUS_SUCCESS;
}


void CCM_VAC_Destroy (TCCM_VAC_Object **thisObj)
{
    *thisObj = NULL;
}

void CCM_VAC_RegisterCallback (TCCM_VAC_Object *ptVac,
                               ECAL_Operation eOperation, 
                               TCCM_VAC_Callback fCB)
{
    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eOperation);
    MCP_UNUSED_PARAMETER (fCB);
}

ECCM_VAC_Status CCM_VAC_StartOperation (TCCM_VAC_Object *ptVac,
                                        ECAL_Operation eOperation,
                                        TCAL_DigitalConfig *ptConfig,
                                        TCCM_VAC_UnavailResourceList *ptUnavailResources)
{
    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eOperation);
    MCP_UNUSED_PARAMETER (ptConfig);

    ptUnavailResources->uNumOfUnavailResources = 0;

    return CCM_VAC_STATUS_SUCCESS;
}

ECCM_VAC_Status CCM_VAC_StopOperation (TCCM_VAC_Object *ptVac,
                                       ECAL_Operation eOperation)
{
    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eOperation);

    return CCM_VAC_STATUS_SUCCESS;
}

ECCM_VAC_Status CCM_VAC_ChangeResource (TCCM_VAC_Object *ptVac,
                                        ECAL_Operation eOperation, 
                                        ECAL_ResourceMask eResourceMask, 
                                        TCAL_DigitalConfig *ptConfig, 
                                        TCCM_VAC_UnavailResourceList *ptUnavailResources)
{
    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eOperation);
    MCP_UNUSED_PARAMETER (eResourceMask);
    MCP_UNUSED_PARAMETER (ptConfig);

    ptUnavailResources->uNumOfUnavailResources = 0;

    return CCM_VAC_STATUS_SUCCESS;
}

ECCM_VAC_Status CCM_VAC_ChangeConfiguration (TCCM_VAC_Object *ptVac,
                                             ECAL_Operation eOperation, 
                                             TCAL_DigitalConfig *ptConfig)
{

    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eOperation);
    MCP_UNUSED_PARAMETER (ptConfig);

    return CCM_VAC_STATUS_SUCCESS;
}

void CCM_VAC_SetResourceProperties (TCCM_VAC_Object *ptVac,
                                    ECAL_Resource eResource,
                                    TCAL_ResourceProperties *pProperties)
{
    MCP_UNUSED_PARAMETER (ptVac);
    MCP_UNUSED_PARAMETER (eResource);
    MCP_UNUSED_PARAMETER (pProperties);
}

