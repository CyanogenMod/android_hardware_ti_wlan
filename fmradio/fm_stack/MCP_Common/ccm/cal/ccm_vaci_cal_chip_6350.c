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
*   FILE NAME:      ccm_vaci_cal_chip_6350.c
*
*   BRIEF:          This file define the audio capabilities and 
*                   configuration Implamantation Dolphin 6350 PG 2.11
*                  
*
*   DESCRIPTION:    This is an internal configuration file for Dolphin 6350 PG 2.11
                    For the CCM_VACI_CAL moudule.
*
*   AUTHOR:         Malovany Ram
*
\*******************************************************************************/



/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "ccm_vaci_cal_chip_6350.h"
#include "mcp_endian.h"

/********************************************************************************
 *
 * Internal Functions
 *
 *******************************************************************************/
static McpU8 Set_params_Fm_Over_Bt_6350(Cal_Resource_Data *pResourceData, McpU8 *pBuffer);

/********************************************************************************
 *
 * Function definitions
 *
 *******************************************************************************/

void CAL_GetAudioCapabilities_6350 (TCAL_MappingConfiguration *tOperationtoresource,
                                    TCAL_OpPairConfig *tOpPairConfig,
                                    TCAL_ResourceSupport *tResource,
                                    TCAL_OperationSupport *tOperation)
{
    McpU32      uIndex;

    /* Update chip supported operations struct for 6350 */
    OPERATION(tOperation)[CAL_OPERATION_FM_TX]=MCP_FALSE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_A3DP]=MCP_FALSE;
    OPERATION(tOperation)[CAL_OPERATION_BT_VOICE]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_WBS]=MCP_FALSE;
    OPERATION(tOperation)[CAL_OPERATION_AWBS]=MCP_FALSE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX_OVER_SCO]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX_OVER_A3DP]=MCP_FALSE;

    /* Update chip available resources struct */
    RESOURCE(tResource)[CAL_RESOURCE_I2SH]=MCP_TRUE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMH]=MCP_TRUE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_1]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_2]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_3]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_4]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_5]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_6]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_PCMIF]=MCP_TRUE;
    RESOURCE(tResource)[CAL_RESOURCE_FMIF]=MCP_TRUE;
    RESOURCE(tResource)[CAL_RESOURCE_FM_ANALOG]=MCP_TRUE;
    RESOURCE(tResource)[CAL_RESOURCE_CORTEX]=MCP_FALSE;
    RESOURCE(tResource)[CAL_RESOURCE_FM_CORE]=MCP_TRUE;

    /* Update operations to resource mapping struct */
    /* FM RX operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].eOperation=CAL_OPERATION_FM_RX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tMandatoryResources.eResources[0]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].eOptionalResource=CAL_RESOURCE_PCMH; /* TODO ronen: is this valid? */
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].eOptionalResource=CAL_RESOURCE_I2SH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].tDerivedResourcesList.uNumOfResources=1;                                          
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].eOptionalResource=CAL_RESOURCE_FM_ANALOG;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].tDerivedResourcesList.uNumOfResources=0;                                          
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tOptionalResources.uNumOfResourceLists=3;

    /* BT_VOICE operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].eOperation=CAL_OPERATION_BT_VOICE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[0].tDerivedResourcesList.uNumOfResources=0;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tOptionalResources.uNumOfResourceLists=1;

    /* FM_RX_OVER_SCO operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].eOperation=CAL_OPERATION_FM_RX_OVER_SCO;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[1]=CAL_RESOURCE_FMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[2]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.uNumOfResources=3;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tOptionalResources.uNumOfResourceLists=0;

    /* update allowed operation pairs */
    /* first nullify pairs for all resources */
    for (uIndex = 0; uIndex < CAL_RESOURCE_MAX_NUM; uIndex++)
    {
        tOpPairConfig->tAllowedPairs[ uIndex ].uNumOfAllowedPairs = 0;

    }
    /* set allowed pairs */
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].uNumOfAllowedPairs = 1;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 0 ].eOperations[ 0 ] = CAL_OPERATION_FM_RX;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 0 ].eOperations[ 1 ] = CAL_OPERATION_FM_RX_OVER_SCO;
}


void Cal_Prep_BtOverFm_Config_6350(McpHciSeqCmdToken *pToken, void *pUserData)
{
        Cal_Resource_Data  *pResourceData = (Cal_Resource_Data*)pUserData;      

        pToken->callback = CAL_Config_CB_Complete;
        pToken->eHciOpcode = BT_HCI_IF_HCI_CMD_FM_OVER_BT_6350;
        pToken->uCompletionEvent = BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
        pToken->pHciCmdParms = &(pResourceData->hciseqCmdPrms[0]);
        pToken->uhciCmdParmsLen = Set_params_Fm_Over_Bt_6350(pResourceData,&(pResourceData->hciseqCmdPrms[0]));              
        pToken->pUserData = (void*)pResourceData;
}

static McpU8 Set_params_Fm_Over_Bt_6350(Cal_Resource_Data *pResourceData, McpU8 *pBuffer)
{
    McpU16      uScoHandle = (McpU16)pResourceData->tProperties.tResourcePorpeties[ 0 ];

    MCP_ENDIAN_HostToLE16 (uScoHandle, &(pBuffer[0])); /*Set the SCO Handle */

    /* whether this is a strat or stop FM over SCO command */
    if (MCP_TRUE == pResourceData->bIsStart)
    {
        pBuffer[2] = 1;
    }
    else
    {
        pBuffer[2] = 0;
    }
    pBuffer[3]=0;
    pBuffer[4]=0;   
    return 5; /* number of bytes written */

}



