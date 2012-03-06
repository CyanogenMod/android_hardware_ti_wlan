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
*   FILE NAME:      ccm_vaci_chip_abstraction_int.h
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
#ifndef __CCM_VACI_CHIP_ABTRACTION_INT_H__
#define __CCM_VACI_CHIP_ABTRACTION_INT_H__

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "ccm_vaci_chip_abstration.h"
#include "mcp_hci_sequencer.h"
#include "mcp_defs.h"
/*******************************************************************************
 *
 * Macro definitions
 *
 ******************************************************************************/
#define CAL_COMMAND_PARAMS_MAX_LEN          34

#define CAL_UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM))

/*Operation support macro */
#define OPERATION(_s)                ((_s)->bOperationSupport)
/*Resource support macro */
#define RESOURCE(_s)             ((_s)->bResourceSupport)

/*Optional Resource List macro */
#define OPTIONALRESOURCELIST(_s, _i)     ((_s)->tOpToResMap[(_i)].tOptionalResources.tOptionalResourceLists)

#define FM_PCMI_I2S_SELECT_OFFSET				(0)
#define FM_PCMI_RIGHT_LEFT_SWAP_OFFSET				(1)
#define FM_PCMI_BIT_OFFSET_VECTOR_OFFSET			(2)
#define FM_PCMI_SLOT_OFSET_VECTOR_OFFSET			(5)
#define FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_OFFSET	(9)

#define FM_I2S_DATA_WIDTH_OFFSET			(0)
#define FM_I2S_DATA_FORMAT_OFFSET			(4)
#define FM_I2S_MASTER_SLAVE_OFFSET			(6)
#define FM_I2S_SDO_TRI_STATE_MODE_OFFSET			(7)
#define FM_I2S_SDO_PHASE_WS_PHASE_SELECT_OFFSET 		(8)
#define FM_I2S_SDO_3ST_ALWZ_OFFSET		(10)
#define FM_I2S_FRAME_SYNC_RATE_OFFSET		(12)

/******************************************************************************/

/*-------------------------------------------------------------------------------
 * Cal_Resource_Data
 *
 * holds information on a specific resource to the config that was sent in CAL_ConfigResource()
 */
typedef struct _Cal_Resource_Data
{
    TCAL_CB                 CB;
    void                    *pUserData;
    ECAL_Operation          eOperation;
    ECAL_Resource           eResource;
    TCAL_DigitalConfig      *ptConfig;
    McpU8                   hciseqCmdPrms[ CAL_COMMAND_PARAMS_MAX_LEN ];
    MCP_HciSeq_Context      hciseq;
    McpHciSeqCmd            hciseqCmd[ HCI_SEQ_MAX_CMDS_PER_SEQUENCE ];
    TCAL_ResourceProperties tProperties;
    McpBool                 bIsStart;  /* for FM RX over SCO command only - indicates if this is start or stop command */
} Cal_Resource_Data;

/*-------------------------------------------------------------------------------
 * Cal_Resource_Config
 *
 * holds information acorrding to resource.
 */

typedef struct _Cal_Resource_Config
{
        Cal_Resource_Data   tResourceConfig[CAL_RESOURCE_MAX_NUM];

} Cal_Resource_Config;

typedef struct
{
	char	*	keyName;
	McpS32 value;
}_Cal_Codec_Config;

typedef enum
{
	FM_PCMI_RIGHT_LEFT_SWAP_LOC = 0,
	FM_PCMI_BIT_OFFSET_VECTOR_LOC,
	FM_PCMI_SLOT_OFSET_VECTOR_LOC ,
	FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_LOC,
	FM_PCM_PARAM_MAX_VALUE_LOC = FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE_LOC,
}_Cal_Fm_Pcm_Param_Loc;
typedef enum
{
	FM_I2S_DATA_WIDTH_LOC = 0,
	FM_I2S_DATA_FORMAT_LOC,
	FM_I2S_MASTER_SLAVE_LOC,
	FM_I2S_SDO_TRI_STATE_MODE_LOC,
	FM_I2S_SDO_PHASE_WS_PHASE_SELECT_LOC,
	FM_I2S_SDO_3ST_ALWZ_LOC,
	FM_I2S_PARAM_MAX_VALUE_LOC = FM_I2S_SDO_3ST_ALWZ_LOC,
}_Cal_Fm_I2s_Param_Loc;

extern _Cal_Codec_Config fmI2sConfigParams[];
extern _Cal_Codec_Config fmPcmConfigParams[];

void CAL_Config_CB_Complete(BtHciIfClientEvent *pEvent);
void CAL_Config_Complete_Null_CB(BtHciIfClientEvent *pEvent);

#endif /* __CCM_VACI_CHIP_ABTRACTION_INT_H__ */

