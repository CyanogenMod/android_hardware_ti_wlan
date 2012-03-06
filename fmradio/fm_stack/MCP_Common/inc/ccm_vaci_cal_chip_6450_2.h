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
*   FILE NAME:      ccm_vaci_cal_chip_6450_2.h
*
*   BRIEF:          This file define the audio capabilities and 
*                   configuration API for Orca 6450 PG 2
*                  
*
*   DESCRIPTION:    This is an internal configuration file for Orca 6450 PG 2
                    For the CCM_VACI_CAL moudule.
*
*   AUTHOR:         Malovany Ram
*
\*******************************************************************************/
#ifndef __CCM_VACI_CAL_CHIP_6450_2_H_
#define __CCM_VACI_CAL_CHIP_6450_2_H_

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "ccm_audio_types.h"
#include "ccm_vaci_chip_abstraction_int.h"

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/
/*-------------------------------------------------------------------------------
 * CAL_GetAudioCapabilities_6450_2()
 *
 * Brief:  
 *     Gets the audio capabilities for 6450 PG 2
 *
 * Description:
 *     Gets the audio capabilities for 6450 PG 2
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      tOperationtoresource [out] - pointer to a struct that holds information
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
void CAL_GetAudioCapabilities_6450_2 (TCAL_MappingConfiguration  *tOperationtoresource,
                                        TCAL_OpPairConfig *tOpPairConfig,
                                        TCAL_ResourceSupport *tResource,
                                        TCAL_OperationSupport *tOperation);



/*-------------------------------------------------------------------------------
 * Cal_Prep_Mux_Config_6450_1_0()
 *
 * Brief:  
 *  Prepre the MUXING command for 6450 PG 2     
 *
 * Description:
 *  Prepre the MUXING command for 6450 PG 2
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *      
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_Mux_Config_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);
/*-------------------------------------------------------------------------------
 * Cal_Prep_Mux_Disable_6450_2()
 *
 * Brief:  
 *  Disable the MUXING 6450 PG 2    
 *
 * Description:
 *  Disable the MUXING 6450 PG 2
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *      
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_Mux_Disable_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);

/*-------------------------------------------------------------------------------
 * Cal_Prep_BtOverFm_Config_6450_1_0()
 *
 * Brief:  
 *  Prepre the BT over FM command for 6450 PG 2     
 *
 * Description:
 *  Prepre the BT over FM command for 6450 PG 2
 *
 * Type:
 *      
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_BtOverFm_Config_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);

/*-------------------------------------------------------------------------------
 * Cal_Prep_FMIF_I2S_Config_6450()
 *
 * Brief:  
 *  bbb   
 *
 * Description:
 *  bbb
 *
 * Type:
 *      
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_FMIF_I2S_Config_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);
/*-------------------------------------------------------------------------------
 * Cal_Prep_FMIF_PCM_Pcm_mode_Config_6450()
 *
 * Brief:  
 *  bbb  
 *
 * Description:
 *  bbb
 *
 * Type:
 *      
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_FMIF_PCM_Pcm_mode_Config_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);
/*-------------------------------------------------------------------------------
 * Cal_Prep_FMIF_PCM_I2s_mode_Config_6450()
 *
 * Brief:  
 *  bbb  
 *
 * Description:
 *  bbb
 *
 * Type:
 *      
 *
 * Parameters:
 *      pToken [out] - pointer to the command token
 *      pUserData [out] - pointer to user data
 *
 * Returns:
 *      N/A
 */
void Cal_Prep_FMIF_PCM_I2s_mode_Config_6450_2(McpHciSeqCmdToken *pToken, void *pUserData);



#endif /* __CCM_VACI_CAL_CHIP_6450_1_0_H_*/

