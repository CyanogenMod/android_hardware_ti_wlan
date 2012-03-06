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
*   FILE NAME:      ccm_vaci_cal_chip_1273_2.c
*
*   BRIEF:          This file define the audio capabilities and 
*                   configuration Implamantation for Trio 1273 PG 2
*                  
*
*   DESCRIPTION:    This is an internal configuration file for Trio 1273 PG 2
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
#include "ccm_vaci_cal_chip_1273_2.h"
#include "mcp_endian.h"
#include "mcp_hal_memory.h"
#include "mcp_defs.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_CAL);

/********************************************************************************
 *
 * Internal Functions
 *
 *******************************************************************************/
MCP_STATIC McpU8 Disable_Mux_1273_2(Cal_Resource_Data   *presourcedata,McpU8 *pBuffer);
MCP_STATIC McpU8 Set_params_Mux_1273_2(Cal_Resource_Data    *presourcedata,McpU8 *pBuffer);
MCP_STATIC McpU8 Set_params_Fm_Over_Bt_1273_2(Cal_Resource_Data *pResourceData ,McpU8 *pBuffer);
MCP_STATIC McpU8 Set_params_Fm_Pcm_I2s_mode_1273_2(Cal_Resource_Data *presourcedata,McpU8 *pBuffer);
MCP_STATIC McpU8 Set_params_Fm_Pcm_Pcm_mode_1273_2(Cal_Resource_Data *presourcedata,McpU8 *pBuffer);
MCP_STATIC McpU8 Set_params_Fm_I2s_1273_2(Cal_Resource_Data   *presourcedata,McpU8 *pBuffer);


/********************************************************************************
 *
 * Function definitions
 *
 *******************************************************************************/

void CAL_GetAudioCapabilities_1273_2(TCAL_MappingConfiguration *tOperationtoresource,
                                        TCAL_OpPairConfig *tOpPairConfig,
                                        TCAL_ResourceSupport *tResource,
                                        TCAL_OperationSupport *tOperation)
{   
    McpU32      uIndex;

    /* Update chip supported operations struct for 1273 PG 2 */
    OPERATION(tOperation)[CAL_OPERATION_FM_TX]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_A3DP]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_BT_VOICE]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_WBS]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_AWBS]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX_OVER_SCO]=MCP_TRUE;
    OPERATION(tOperation)[CAL_OPERATION_FM_RX_OVER_A3DP]=MCP_TRUE;

    /* Update chip available resources struct */
    RESOURCE(tResource)[CAL_RESOURCE_I2SH]=MCP_TRUE;         
    RESOURCE(tResource)[CAL_RESOURCE_PCMH]=MCP_TRUE;         
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_1]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_2]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_3]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_4]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_5]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMT_6]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_PCMIF]=MCP_TRUE;        
    RESOURCE(tResource)[CAL_RESOURCE_FMIF]=MCP_TRUE;        
    RESOURCE(tResource)[CAL_RESOURCE_FM_ANALOG]=MCP_TRUE;    
    RESOURCE(tResource)[CAL_RESOURCE_CORTEX]=MCP_TRUE;       
    RESOURCE(tResource)[CAL_RESOURCE_FM_CORE]=MCP_TRUE;      

    /* Update operations to resource mapping struct */
    /* FM_TX operation to resource mapping  */                                       
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_TX].eOperation=CAL_OPERATION_FM_TX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_TX].tMandatoryResources.eResources[0]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_TX].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[0].eOptionalResource=CAL_RESOURCE_FM_ANALOG;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[0].tDerivedResourcesList.uNumOfResources=0;                                          
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[1].eOptionalResource=CAL_RESOURCE_I2SH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[1].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[1].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[2].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[2].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[2].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[3].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[3].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[3].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[4].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[4].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[4].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[5].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[5].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[5].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[6].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[6].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[6].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[7].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[7].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[7].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[7].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[8].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[8].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[8].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_TX)[8].tDerivedResourcesList.uNumOfResources=1;                                  
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_TX].tOptionalResources.uNumOfResourceLists=9;

    /* FM RX operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].eOperation=CAL_OPERATION_FM_RX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tMandatoryResources.eResources[0]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[0].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[1].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[2].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[3].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[3].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[3].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[4].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[4].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[4].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[5].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[5].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[5].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[6].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[6].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[6].tDerivedResourcesList.uNumOfResources=1;          
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[7].eOptionalResource=CAL_RESOURCE_I2SH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[7].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[7].tDerivedResourcesList.eResources[0]=CAL_RESOURCE_FMIF;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[7].tDerivedResourcesList.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[8].eOptionalResource=CAL_RESOURCE_FM_ANALOG;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[8].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_FM_RX)[8].tDerivedResourcesList.uNumOfResources=0;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX].tOptionalResources.uNumOfResourceLists=9;

    /* A3DP operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_A3DP].eOperation=CAL_OPERATION_A3DP;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_A3DP].tMandatoryResources.eResources[0]=CAL_RESOURCE_CORTEX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_A3DP].tMandatoryResources.eResources[1]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_A3DP].tMandatoryResources.uNumOfResources=2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[0].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[1].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[1].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[2].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[2].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[3].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[3].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[4].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[4].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[5].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[5].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[6].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[6].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[7].eOptionalResource=CAL_RESOURCE_I2SH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[7].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_A3DP)[7].tDerivedResourcesList.uNumOfResources=0;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_A3DP].tOptionalResources.uNumOfResourceLists=8;                                            

    /* BT_VOICE operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].eOperation=CAL_OPERATION_BT_VOICE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[0].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[1].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[1].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[2].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[2].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[3].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[3].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[4].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[4].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[5].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[5].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[6].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[6].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[7].eOptionalResource=CAL_RESOURCE_I2SH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[7].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_BT_VOICE)[7].tDerivedResourcesList.uNumOfResources=0;                                          
    tOperationtoresource->tOpToResMap[CAL_OPERATION_BT_VOICE].tOptionalResources.uNumOfResourceLists=8;

    /* WBS operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_WBS].eOperation=CAL_OPERATION_WBS;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_WBS].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_WBS].tMandatoryResources.uNumOfResources=1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[0].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[1].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[1].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[2].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[2].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[3].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[3].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[4].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[4].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[5].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[5].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[6].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_WBS)[6].tDerivedResourcesList.uNumOfResources=0;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_WBS].tOptionalResources.uNumOfResourceLists=7;

    /* AWBS operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_AWBS].eOperation=CAL_OPERATION_AWBS;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_AWBS].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_AWBS].tMandatoryResources.eResources[1]=CAL_RESOURCE_CORTEX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_AWBS].tMandatoryResources.uNumOfResources=2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[0].eOptionalResource=CAL_RESOURCE_PCMH;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[0].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[0].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[1].eOptionalResource=CAL_RESOURCE_PCMT_1;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[1].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[1].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[2].eOptionalResource=CAL_RESOURCE_PCMT_2;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[2].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[2].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[3].eOptionalResource=CAL_RESOURCE_PCMT_3;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[3].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[3].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[4].eOptionalResource=CAL_RESOURCE_PCMT_4;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[4].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[4].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[5].eOptionalResource=CAL_RESOURCE_PCMT_5;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[5].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[5].tDerivedResourcesList.uNumOfResources=0;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[6].eOptionalResource=CAL_RESOURCE_PCMT_6;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[6].bIsUsed=MCP_FALSE;
    OPTIONALRESOURCELIST(tOperationtoresource, CAL_OPERATION_AWBS)[6].tDerivedResourcesList.uNumOfResources=0;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_AWBS].tOptionalResources.uNumOfResourceLists=7;

    /* FM RX over SCO operation to resource mapping */
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].eOperation=CAL_OPERATION_FM_RX_OVER_SCO;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[1]=CAL_RESOURCE_CORTEX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[2]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.eResources[3]=CAL_RESOURCE_FMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tMandatoryResources.uNumOfResources=4;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_SCO].tOptionalResources.uNumOfResourceLists=0;

    /* FM RX over A3DP operation to resource mapping */    
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].eOperation=CAL_OPERATION_FM_RX_OVER_A3DP;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tMandatoryResources.eResources[0]=CAL_RESOURCE_PCMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tMandatoryResources.eResources[1]=CAL_RESOURCE_CORTEX;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tMandatoryResources.eResources[2]=CAL_RESOURCE_FM_CORE;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tMandatoryResources.eResources[3]=CAL_RESOURCE_FMIF;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tMandatoryResources.uNumOfResources=4;
    tOperationtoresource->tOpToResMap[CAL_OPERATION_FM_RX_OVER_A3DP].tOptionalResources.uNumOfResourceLists=0;

    /* update allowed operation pairs */
    /* first nullify pairs for all resources */
    for (uIndex = 0; uIndex < CAL_RESOURCE_MAX_NUM; uIndex++)
    {
        tOpPairConfig->tAllowedPairs[ uIndex ].uNumOfAllowedPairs = 0;

    }
    /* set allowed pairs */
    /* FM core */
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].uNumOfAllowedPairs = 2;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 0 ].eOperations[ 0 ] = CAL_OPERATION_FM_RX;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 0 ].eOperations[ 1 ] = CAL_OPERATION_FM_RX_OVER_SCO;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 1 ].eOperations[ 0 ] = CAL_OPERATION_FM_RX;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FM_CORE ].tOpPairs[ 1 ].eOperations[ 1 ] = CAL_OPERATION_FM_RX_OVER_A3DP;
    /* FM-IF */
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FMIF ].uNumOfAllowedPairs = 2;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FMIF ].tOpPairs[ 0 ].eOperations[ 0 ] = CAL_OPERATION_FM_RX;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FMIF ].tOpPairs[ 0 ].eOperations[ 1 ] = CAL_OPERATION_FM_RX_OVER_SCO;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FMIF ].tOpPairs[ 1 ].eOperations[ 0 ] = CAL_OPERATION_FM_RX;
    tOpPairConfig->tAllowedPairs[ CAL_RESOURCE_FMIF ].tOpPairs[ 1 ].eOperations[ 1 ] = CAL_OPERATION_FM_RX_OVER_A3DP;
}



void Cal_Prep_Mux_Config_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)

{
    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;        

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_VS_BT_IP1_1_SET_FM_AUDIO_PATH;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Set_params_Mux_1273_2(presourcedata,&(presourcedata->hciseqCmdPrms[0]));                           
    pToken->pUserData = (void*)presourcedata;        
}
void Cal_Prep_Mux_Disable_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)

{
    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;        

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_VS_BT_IP1_1_SET_FM_AUDIO_PATH;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Disable_Mux_1273_2(presourcedata,&(presourcedata->hciseqCmdPrms[0]));                          
    pToken->pUserData = (void*)presourcedata;        
}


/*FM-VAC*/
void Cal_Prep_FMIF_I2S_Config_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)

{
    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;        

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_WRITE_TO_FM;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Set_params_Fm_I2s_1273_2(presourcedata,&(presourcedata->hciseqCmdPrms[0]));                          
    pToken->pUserData = (void*)presourcedata;        
}
MCP_STATIC McpU8 Set_params_Fm_I2s_1273_2(Cal_Resource_Data   *presourcedata,McpU8 *pBuffer)
{
    McpU8 fmCmdParms[2];
    McpU16 params = 0;
    McpU16 freqMask = 0;
    McpU16 dataWidth;/*Sets the expected clock frequency with respect to Frame?Sync, as well as the bit length of the word.*/
    McpU16 dataFormat;/*- Data Format Controls the I2S format 00 - I2S Standard format (default)*/
    McpU16 masterSlave;/* 0 = master, 1 = slave     Sets I2S interface in Slave or Master Mode*/
    McpU16 sdoTriStateMode; /*SDO Tri-state mode    0 - SDO is tri-stated after sending (default)        */
    McpU16 sdoPhaseSelect_WsPhase;              /*SDO phase select*/
    McpU16 sdo3stAlwz;
    pBuffer[0] = 31;/*fm opcode I2S_MODE_CONFIG_SET */


    dataWidth = (McpU16)fmI2sConfigParams[FM_I2S_DATA_WIDTH_LOC].value;/*0101 - bit-clock = 50Fs (default)*/


    params |= dataWidth<<FM_I2S_DATA_WIDTH_OFFSET;

    dataFormat = (McpU16)fmI2sConfigParams[FM_I2S_DATA_FORMAT_LOC].value ;/*I2S Standard format (default)*/


    params |= dataFormat<<FM_I2S_DATA_FORMAT_OFFSET;

    masterSlave =  (McpU16)fmI2sConfigParams[FM_I2S_MASTER_SLAVE_LOC].value ;


    params |= masterSlave<<FM_I2S_MASTER_SLAVE_OFFSET;

    sdoTriStateMode = (McpU16)fmI2sConfigParams[FM_I2S_SDO_TRI_STATE_MODE_LOC].value ;    /*0 - SDO is tri-stated after sending (default)      */


    params |= sdoTriStateMode<<FM_I2S_SDO_TRI_STATE_MODE_OFFSET;

    sdoPhaseSelect_WsPhase = (McpU16)fmI2sConfigParams[FM_I2S_SDO_PHASE_WS_PHASE_SELECT_LOC].value ;


    params |= sdoPhaseSelect_WsPhase<<FM_I2S_SDO_PHASE_WS_PHASE_SELECT_OFFSET;

    sdo3stAlwz = (McpU16)fmI2sConfigParams[FM_I2S_SDO_3ST_ALWZ_LOC].value ;


    params |= sdo3stAlwz<<FM_I2S_SDO_3ST_ALWZ_OFFSET;

    switch (presourcedata->ptConfig->eSampleFreq)
    {
    case (CAL_SAMPLE_FREQ_8000):
        freqMask = 10;
    break;  
    case (CAL_SAMPLE_FREQ_11025):
    freqMask = 9;
    break;  
    case (CAL_SAMPLE_FREQ_12000):
    freqMask = 8;
    break;  
    case (CAL_SAMPLE_FREQ_16000):
    freqMask = 6;
    break;  
    case (CAL_SAMPLE_FREQ_22050):
    freqMask = 5;
    break;  
    case (CAL_SAMPLE_FREQ_24000):
    freqMask = 4;
    break;  
    case (CAL_SAMPLE_FREQ_32000):
    freqMask = 2;
    break;  
    case (CAL_SAMPLE_FREQ_44100):
    freqMask = 1;
        break;
   case (CAL_SAMPLE_FREQ_48000):
    freqMask = 0;
        break;
    default:
        break;
    }
    params |= freqMask<<FM_I2S_FRAME_SYNC_RATE_OFFSET;


    MCP_ENDIAN_HostToBE16(params,&fmCmdParms[0]);
    /* Store the FM Parameters Len in LE */

    MCP_ENDIAN_HostToLE16(2,&pBuffer[1]);

    /* Copy the FM cmd parms (should be in the correct endiannity already) */
    MCP_HAL_MEMORY_MemCopy(&pBuffer[3], fmCmdParms, 2);

/* Length of 2nd field of HCI Parameters (described above) */
/*#define FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN           (2)*/

/* Total length of HCI Parameters section */
/*#define FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(fmParmsLen)     \
(sizeof(FmcFwOpcode) + FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN + fmParmsLen)*/

    return(sizeof(McpU8) + 2 + 2);
}
void Cal_Prep_FMIF_PCM_I2s_mode_Config_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)

{
    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;        

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_WRITE_TO_FM;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Set_params_Fm_Pcm_I2s_mode_1273_2(presourcedata,&(presourcedata->hciseqCmdPrms[0]));                         
    pToken->pUserData = (void*)presourcedata;        
}
void Cal_Prep_FMIF_PCM_Pcm_mode_Config_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)

{
    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;        

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_WRITE_TO_FM;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Set_params_Fm_Pcm_Pcm_mode_1273_2(presourcedata,&(presourcedata->hciseqCmdPrms[0]));                         
    pToken->pUserData = (void*)presourcedata;        
}

MCP_STATIC McpU8 Set_params_Fm_Pcm_Pcm_mode_1273_2(Cal_Resource_Data  *presourcedata,McpU8 *pBuffer)
{
    McpU8 fmCmdParms[2];
    McpU16 params = 0;

    McpU16 pcmOrI2s;/*1*/
    McpU16 pcmiLeftRightSwap;/*1- bit PCMI_left_right_swap*/
    McpU16 bitOffsetVector;/*3-bits*/
    McpU16 slotOffsetVector;/*4-bits*/
    McpU16 pcmInterfaceChannelDataSize;/*00 - 16 bits (default);  PCM interface channel data size Number of bits/channel (left/right) in an audio sample*/

    presourcedata = presourcedata;
    pBuffer[0] = 30;/*fm opcode I2S_MODE_CONFIG_SET */


    pcmOrI2s = 0x0001;


    params |= pcmOrI2s<<FM_PCMI_I2S_SELECT_OFFSET;

    pcmiLeftRightSwap = (McpU16)fmPcmConfigParams[FM_PCMI_RIGHT_LEFT_SWAP_LOC].value;


    params |= pcmiLeftRightSwap<<FM_PCMI_RIGHT_LEFT_SWAP_OFFSET;

    bitOffsetVector = (McpU16)fmPcmConfigParams[FM_PCMI_BIT_OFFSET_VECTOR_LOC].value;


    params |= bitOffsetVector<<FM_PCMI_BIT_OFFSET_VECTOR_OFFSET;

    slotOffsetVector = (McpU16)fmPcmConfigParams[FM_PCMI_SLOT_OFSET_VECTOR_LOC].value ;   


    params |= slotOffsetVector<<FM_PCMI_SLOT_OFSET_VECTOR_OFFSET;

    pcmInterfaceChannelDataSize = 0x0000;

    params |= pcmInterfaceChannelDataSize<<FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_OFFSET;


    MCP_ENDIAN_HostToBE16(params,&fmCmdParms[0]);
    /* Store the FM Parameters Len in LE */

    MCP_ENDIAN_HostToLE16(2,&pBuffer[1]);

    /* Copy the FM cmd parms (should be in the correct endiannity already) */
    MCP_HAL_MEMORY_MemCopy(&pBuffer[3], fmCmdParms, 2);

    /* Length of 2nd field of HCI Parameters (described above) */
    /*#define FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN           (2)*/

    /* Total length of HCI Parameters section */
    /*#define FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(fmParmsLen)     \
    (sizeof(FmcFwOpcode) + FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN + fmParmsLen)*/

    return(sizeof(McpU8) + 2 + 2);
}

MCP_STATIC McpU8 Set_params_Fm_Pcm_I2s_mode_1273_2(Cal_Resource_Data  *presourcedata,McpU8 *pBuffer)
{
    McpU8 fmCmdParms[2];
    McpU16 params = 0;

    McpU16 pcmOrI2s;/*1*/
    McpU16 pcmiLeftRightSwap;/*1- bit PCMI_left_right_swap*/
    McpU16 bitOffsetVector;/*3-bits*/
    McpU16 slotOffsetVector;/*4-bits*/
    McpU16 pcmInterfaceChannelDataSize;/*00 - 16 bits (default);  PCM interface channel data size Number of bits/channel (left/right) in an audio sample*/

    presourcedata = presourcedata;
    pBuffer[0] = 30;/*fm opcode I2S_MODE_CONFIG_SET */


    pcmOrI2s = 0x0000;


    params |= pcmOrI2s<<FM_PCMI_I2S_SELECT_OFFSET;

    pcmiLeftRightSwap = (McpU16)fmPcmConfigParams[FM_PCMI_RIGHT_LEFT_SWAP_LOC].value;


    params |= pcmiLeftRightSwap<<FM_PCMI_RIGHT_LEFT_SWAP_OFFSET;

    bitOffsetVector = (McpU16)fmPcmConfigParams[FM_PCMI_BIT_OFFSET_VECTOR_LOC].value ;


    params |= bitOffsetVector<<FM_PCMI_BIT_OFFSET_VECTOR_OFFSET;

    slotOffsetVector = (McpU16)fmPcmConfigParams[FM_PCMI_SLOT_OFSET_VECTOR_LOC].value ;   


    params |= slotOffsetVector<<FM_PCMI_SLOT_OFSET_VECTOR_OFFSET;

    pcmInterfaceChannelDataSize =(McpU16) fmPcmConfigParams[FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_LOC].value ;

    params |= pcmInterfaceChannelDataSize<<FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_OFFSET;

    MCP_ENDIAN_HostToBE16(params,&fmCmdParms[0]);
    /* Store the FM Parameters Len in LE */

    MCP_ENDIAN_HostToLE16(2,&pBuffer[1]);

    /* Copy the FM cmd parms (should be in the correct endiannity already) */
    MCP_HAL_MEMORY_MemCopy(&pBuffer[3], fmCmdParms, 2);

    /* Length of 2nd field of HCI Parameters (described above) */
    /*#define FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN           (2)*/

    /* Total length of HCI Parameters section */
    /*#define FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(fmParmsLen)     \
    (sizeof(FmcFwOpcode) + FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN + fmParmsLen)*/

    return(sizeof(McpU8) + 2 + 2);
}
MCP_STATIC McpU8 Set_params_Mux_1273_2(Cal_Resource_Data    *presourcedata,McpU8 *pBuffer)
{
    MCP_ENDIAN_HostToLE32 (0xffffffff,&(pBuffer[ 0 ]));/*PCMI Override*/
    MCP_ENDIAN_HostToLE32 (0xffffffff,&(pBuffer[ 4 ]));/*I2S Override*/

    switch (presourcedata->eResource)
    {
    case (CAL_RESOURCE_PCMH):
        /*Config PCMH MUX acording to FM-IP or BT-IP */

        switch (presourcedata->eOperation)
        {
        case (CAL_OPERATION_FM_TX):
        case (CAL_OPERATION_FM_RX):
            /*enable FM-IP on external PCM */
            pBuffer[8] = 0;
            pBuffer[9] = 1;
            pBuffer[10] = 0xFF;
            break;      
        case(CAL_OPERATION_A3DP):
        case(CAL_OPERATION_BT_VOICE):
        case(CAL_OPERATION_WBS):
        case(CAL_OPERATION_AWBS):
            /*enable BT-IP on external PCM */
            pBuffer[8] = 1;
            pBuffer[9] = 0xFF;
            pBuffer[10] = 0xFF;
            break;  

        default:
            MCP_LOG_ERROR(("Set_params_Mux_1273_2: invalid operation %d for resource %d",
                           presourcedata->eOperation, presourcedata->eResource));
            break;
        }

        break;

    case (CAL_RESOURCE_I2SH):

        /*Config I2SH MUX acording to FM-IP or BT-IP */
        switch (presourcedata->eOperation)
        {
        case(CAL_OPERATION_FM_TX):
        case(CAL_OPERATION_FM_RX):
            /*enable FM-IP on external I2S*/
            pBuffer[8] = 0xFF;
            pBuffer[9] = 2;
            pBuffer[10] = 0xFF;
            break;      

        case(CAL_OPERATION_A3DP):
        case(CAL_OPERATION_BT_VOICE):
        case(CAL_OPERATION_WBS):
        case(CAL_OPERATION_AWBS):
            /*todo ram -  not supported  in 1.1 update the configuration*/
            pBuffer[8] = 2;
            pBuffer[9] = 0xFF;
            pBuffer[10] = 0xFF;
            break;

        default:
            MCP_LOG_ERROR(("Set_params_Mux_1273_2: invalid operation %d for resource %d",
                           presourcedata->eOperation, presourcedata->eResource));
            break;
        }                       
        break;

    case (CAL_RESOURCE_PCMT_1):
    case (CAL_RESOURCE_PCMT_2):
    case (CAL_RESOURCE_PCMT_3):
    case (CAL_RESOURCE_PCMT_4):
    case (CAL_RESOURCE_PCMT_5):
    case (CAL_RESOURCE_PCMT_6):
        /*todo ram -what about those ones ??*/
        pBuffer[8] = 0xFF;
        pBuffer[9] = 0xFF;
        pBuffer[10] = 0xFF;/* TDM byte*/
        break;

    default:
        MCP_LOG_ERROR(("Set_params_Mux_1273_2: invalid resource %d",
                       presourcedata->eResource));
        break;
    }

    return 15;
}
MCP_STATIC McpU8 Disable_Mux_1273_2(Cal_Resource_Data   *presourcedata,McpU8 *pBuffer)
{

    MCP_UNUSED_PARAMETER(presourcedata);
    
    MCP_ENDIAN_HostToLE32 (0xffffffff,&(pBuffer[ 0 ]));/*PCMI Override*/
    MCP_ENDIAN_HostToLE32 (0xffffffff,&(pBuffer[ 4 ]));/*I2S Override*/

    switch (presourcedata->eResource)
    {
    case (CAL_RESOURCE_PCMH):
        /*Config PCMH MUX acording to FM-IP or BT-IP */
    
        switch (presourcedata->eOperation)
        {
        case (CAL_OPERATION_FM_TX):
        case (CAL_OPERATION_FM_RX):
            /*disable FM-IP on external PCM */
            pBuffer[8] = 0xFF;
            pBuffer[9] = 0;
            pBuffer[10] = 0xFF;
            break;      

        case(CAL_OPERATION_A3DP):
        case(CAL_OPERATION_BT_VOICE):
        case(CAL_OPERATION_WBS):
        case(CAL_OPERATION_AWBS):
            /*disable BT-IP on external PCM */
            pBuffer[8] = 0;
            pBuffer[9] = 0xFF;
            pBuffer[10] = 0xFF;
            break;  

        default:
            MCP_LOG_ERROR(("Disable_Mux_1273_2: invalid operation %d for resource %d",
                           presourcedata->eOperation, presourcedata->eResource));
            break;
        }
    
        break;
    
    case (CAL_RESOURCE_I2SH):
    
        switch (presourcedata->eOperation)
        {
        case(CAL_OPERATION_FM_TX):
        case(CAL_OPERATION_FM_RX):
            /*disable FM-IP on external I2S*/
            pBuffer[8] = 0xFF;
            pBuffer[9] = 0;
            pBuffer[10] = 0xFF;
            break;      

        case(CAL_OPERATION_A3DP):
        case(CAL_OPERATION_BT_VOICE):
        case(CAL_OPERATION_WBS):
        case(CAL_OPERATION_AWBS):
            pBuffer[8] = 0;
            pBuffer[9] = 0xFF;
            pBuffer[10] = 0xFF;
            break;

        default:
            MCP_LOG_ERROR(("Disable_Mux_1273_2: invalid operation %d for resource %d",
                           presourcedata->eOperation, presourcedata->eResource));
            break;
        }                       
        break;

    default:  
        MCP_LOG_ERROR(("Disable_Mux_1273_2: invalid resource %d",
                       presourcedata->eResource));              
        break;
    }
    
    return 15;
}

void Cal_Prep_BtOverFm_Config_1273_2(McpHciSeqCmdToken *pToken, void *pUserData)
{
    Cal_Resource_Data  *pResourceData = (Cal_Resource_Data*)pUserData;      

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode = BT_HCI_IF_HCI_CMD_SET_NARROW_BAND_VOICE_PATH;
    pToken->uCompletionEvent = BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms = &(pResourceData->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen = Set_params_Fm_Over_Bt_1273_2 (pResourceData,
                                                              &(pResourceData->hciseqCmdPrms[0]));              
    pToken->pUserData = (void*)pResourceData;
}

MCP_STATIC McpU8 Set_params_Fm_Over_Bt_1273_2(Cal_Resource_Data *pResourceData ,McpU8 *pBuffer)
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
    pBuffer[3] =0;

    return 4; /* number of bytes written */
}
