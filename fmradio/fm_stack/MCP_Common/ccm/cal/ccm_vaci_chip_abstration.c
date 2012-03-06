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
*   FILE NAME:      ccm_vaci_configuration_engine.c
*
*   BRIEF:          This file defines the implementation of the Connectivity Chip
*                   Manager (CCM) Voice and Audio Control (VAC) configuration 
*                   engine component.
*                  
*
*   DESCRIPTION:    The configuration engine is a CCM-VAC internal module Performing
*                   VAC operations (starting and stopping an operation, changing
*                   routing and configuration)
*
*   AUTHOR:        Malovany Ram
*
\*******************************************************************************/



/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "ccm_vaci_chip_abstration.h"
#include "mcp_endian.h"
#include "mcp_hal_string.h"
#include "ccm_vaci_cal_chip_6350.h"
#include "ccm_vaci_cal_chip_6450_1_0.h"
#include "ccm_vaci_cal_chip_6450_2.h"
#include "ccm_vaci_cal_chip_1273.h"
#include "ccm_vaci_cal_chip_1273_2.h"
#include "ccm_vaci_cal_chip_1283.h"
#include "mcp_hal_memory.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_CCM_CAL);

/*******************************************************************************
 *
 * Macro definitions
 *
 ******************************************************************************/

/*Convert Sample frequency to Frame Sync Frequency*/
#define CAL_SAMPLE_FREQ_TO_CODEC(_sampleFreq)              (((_sampleFreq) == 0) ? 8000 : \
                                                                 (((_sampleFreq) == 1) ? 11025 : \
                                                                  (((_sampleFreq) == 2) ? 12000 : \
                                                                   (((_sampleFreq) == 3) ? 16000 : \
                                                                    (((_sampleFreq) == 4) ? 22050 : \
                                                                     (((_sampleFreq) == 5) ? 24000 : \
                                                                      (((_sampleFreq) == 6) ? 32000 : \
                                                                       (((_sampleFreq) == 7) ? 44100 : 48000))))))))

/********************************************************************************
 *
 * Globals
 *
 *******************************************************************************/

struct _Cal_Config_ID
{
    /*Chip version parameter */
    ECAL_ChipVersion chip_version;

    /*Operation Data Struct*/
    Cal_Resource_Config resourcedata;
	
	McpConfigParser 			*pConfigParser;		  /* configuration file storage and parser */
} ;


typedef struct 
{
    Cal_Config_ID       calconfigid[MCP_HAL_MAX_NUM_OF_CHIPS];
} _Cal_Resource_Chipid_Config;

MCP_STATIC  _Cal_Resource_Chipid_Config calresource_Chipidconfig;


MCP_STATIC _Cal_Codec_Config codecConfigParams[] =  { 
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_NUMBER_OF_PCM_CLK_CH1,	0},			   
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_NUMBER_OF_PCM_CLK_CH2,	0},			
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_PCM_CLOCK_RATE,			0},				
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_PCM_DIRECTION_ROLE,		0},   
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_FRAME_SYNC_DUTY_CYCLE,	0},   
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_FRAME_SYNC_EDGE,		0},			
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_FRAME_SYNC_POLARITY, 	0}, 			 
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_DATA_OUT_SIZE,		0},				
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_DATA_OUT_OFFSET, 	0}, 				
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_OUT_EDGE,			0},		 
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_DATA_IN_SIZE,		0},
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_DATA_IN_OFFSET, 		0},
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH1_IN_EDGE, 			0}, 			
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_DATA_OUT_SIZE,		0},	
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_DATA_OUT_OFFSET,	0},
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_OUT_EDGE,			0},		 
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_DATA_IN_SIZE,		0},
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_DATA_IN_OFFSET,		0},				
				{CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_CH2_IN_EDGE,				0},					
			 };
typedef enum
{
	NUMBER_OF_PCM_CLK_CH1_LOC = 0,
	NUMBER_OF_PCM_CLK_CH2_LOC ,
	PCM_CLOCK_RATE_LOC,
	PCM_DIRECTION_ROLE_LOC,
	FRAME_SYNC_DUTY_CYCLE_LOC,
	FRAME_SYNC_EDGE_LOC,
	FRAME_SYNC_POLARITY_LOC,
	CH1_DATA_OUT_SIZE_LOC,
	CH1_DATA_OUT_OFFSET_LOC,
	CH1_OUT_EDGE_LOC,
	CH1_DATA_IN_SIZE_LOC,
	CH1_DATA_IN_OFFSET_LOC,
	CH1_IN_EDGE_LOC,
	CH2_DATA_OUT_SIZE_LOC,
	CH2_DATA_OUT_OFFSET_LOC,
	CH2_OUT_EDGE_LOC,
	CH2_DATA_IN_SIZE_LOC,
	CH2_DATA_IN_OFFSET_LOC,
	CH2_IN_EDGE_LOC,
	CONFIG_PARAM_MAX_VALUE = CH2_IN_EDGE_LOC,
}_Cal_Param_Loc;



_Cal_Codec_Config fmPcmConfigParams[] = { 
				{CCM_VAC_CONFIG_INI_FM_PCMI_RIGHT_LEFT_SWAP,	0}, 		   
				{CCM_VAC_CONFIG_INI_FM_PCMI_BIT_OFFSET_VECTOR,	0}, 		
				{CCM_VAC_CONFIG_INI_FM_PCMI_SLOT_OFSET_VECTOR,			0}, 			
				{CCM_VAC_CONFIG_INI_FM_PCMI_PCM_INTERFACE_CHANNEL_DATA_SIZE,		0},   
			 };

_Cal_Codec_Config fmI2sConfigParams[] =	{ 
				{CCM_VAC_CONFIG_INI_FM_I2S_DATA_WIDTH,	0}, 		   
				{CCM_VAC_CONFIG_INI_FM_I2S_DATA_FORMAT, 0}, 		
				{CCM_VAC_CONFIG_INI_FM_I2S_MASTER_SLAVE,			0}, 			
				{CCM_VAC_CONFIG_INI_FM_I2S_SDO_TRI_STATE_MODE,		0},   
				{CCM_VAC_CONFIG_INI_FM_I2S_SDO_PHASE_WS_PHASE_SELECT,			0}, 			
				{CCM_VAC_CONFIG_INI_FM_I2S_SDO_3ST_ALWZ,		0},   
			 };

/********************************************************************************
 *
 * Internal functions prototypes
 *
 *******************************************************************************/
MCP_STATIC const char *ChipToString(ECAL_ChipVersion chip);
MCP_STATIC const char *OperationToString(ECAL_Operation eResource);
MCP_STATIC const char *ResourceToString(ECAL_Resource eResource);
MCP_STATIC const char *IFStatusToString(BtHciIfStatus status);
MCP_STATIC void Cal_Prep_PCMIF_Config(McpHciSeqCmdToken *pToken, void *pUserData);
MCP_STATIC McpU8 Set_params_PCMIF(Cal_Resource_Data  *presourcedata,McpU8 *pBuffer);
MCP_STATIC void CAL_Init_id_data (Cal_Config_ID *pConfigid, BtHciIfObj *hciIfObj);

/********************************************************************************
 *
 * Function definitions
 *
 *******************************************************************************/
void CCM_CAL_StaticInit(void)
{
    MCP_FUNC_START ("CCM_CAL_StaticInit");

    /* no need to do anything - everything is per object */

    MCP_FUNC_END ();
}

void CAL_Create(McpHalChipId chipId, BtHciIfObj *hciIfObj, Cal_Config_ID **ppConfigid,McpConfigParser 			*pConfigParser)
{    
    MCP_FUNC_START("CAL_Create");

    *ppConfigid= &calresource_Chipidconfig.calconfigid[chipId];

	(*ppConfigid)->pConfigParser = pConfigParser;
	
    CAL_Init_id_data(*ppConfigid, hciIfObj);

    MCP_FUNC_END();

}

void CAL_Destroy(Cal_Config_ID **ppConfigid)
{       
    ECAL_Resource resourceIndex;

    MCP_FUNC_START ("CAL_Destroy");

    /* destroy resources sequences */
    for (resourceIndex = 0; resourceIndex < CAL_RESOURCE_MAX_NUM; resourceIndex++)
    {
         /* NULL check*/
        if(((*ppConfigid)->resourcedata.tResourceConfig[resourceIndex].hciseq.handle) != NULL)
        MCP_HciSeq_DestroySequence (&((*ppConfigid)->resourcedata.tResourceConfig[resourceIndex].hciseq));
    }
    /* Nullify the CAL object */
    *ppConfigid = NULL;

    MCP_FUNC_END ();
}

/*---------------------------------------------------------------------------
 *            CAL_Init_id_data()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Initialize Resource and FM data acording to the chip
 *
 */

MCP_STATIC void CAL_Init_id_data (Cal_Config_ID *pConfigid, BtHciIfObj *hciIfObj)
{       
    ECAL_Resource resourceindex;

    /*Init  resourcedata struct */
    for (resourceindex=0;resourceindex<CAL_RESOURCE_MAX_NUM;resourceindex++)
    {
        pConfigid->resourcedata.tResourceConfig[resourceindex].CB=NULL;
        pConfigid->resourcedata.tResourceConfig[resourceindex].eOperation=CAL_OPERATION_INVALID;
        pConfigid->resourcedata.tResourceConfig[resourceindex].eResource=CAL_RESOURCE_INVALID;
        pConfigid->resourcedata.tResourceConfig[resourceindex].pUserData=NULL;

        /*Register every HCI seq per resource as client */
        if ((resourceindex == CAL_RESOURCE_FMIF)||
            (resourceindex == CAL_RESOURCE_FM_ANALOG)||
            (resourceindex == CAL_RESOURCE_CORTEX)||
            (resourceindex == CAL_RESOURCE_FM_CORE))
        {
            MCP_HciSeq_CreateSequence(&(pConfigid->resourcedata.tResourceConfig[ resourceindex ].hciseq), 
                                      hciIfObj, MCP_HAL_CORE_ID_FM);           
        }
        else
        {
            MCP_HciSeq_CreateSequence(&(pConfigid->resourcedata.tResourceConfig[ resourceindex ].hciseq), 
                                      hciIfObj, MCP_HAL_CORE_ID_BT);           
        }
    }
}



void CCM_CAL_Configure(Cal_Config_ID *pConfigid,
                              McpU16 projectType,
                              McpU16 versionMajor,
                              McpU16 versionMinor)
{
	McpConfigParserStatus eCPStatus;
	McpU8 index;
	McpBool fmDigitalAudioConfSupported =MCP_FALSE;
    McpS32 ovr_projectType;
    McpS32 ovr_versionMajor;
    McpS32 ovr_versionMinor;

    /* Read configuration file, for overriding the version number provided in the 
     * function arguments
     */
     
	if (MCP_CONFIG_PARSER_STATUS_SUCCESS == MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
												 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_SECTION_NAME,
												 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_PROJECT_TYPE,
												 (McpS32*)&ovr_projectType))

        projectType = (McpU16)ovr_projectType;

	if (MCP_CONFIG_PARSER_STATUS_SUCCESS == MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
            									 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_SECTION_NAME,
            									 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_VERSION_MAJOR,
            									 (McpS32*)&ovr_versionMajor))
        versionMajor = (McpU16)ovr_versionMajor;

	if (MCP_CONFIG_PARSER_STATUS_SUCCESS == MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
												 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_SECTION_NAME,
												 (McpU8 *)CCM_VAC_CONFIG_INI_OVERRIDE_VERSION_MINOR,
												 (McpS32*)&ovr_versionMinor))
        versionMinor = (McpU16)ovr_versionMinor;

    
    /* Update the Chip Version that we are working with */
    switch (projectType)
    {
    case 5:
        pConfigid->chip_version = CAL_CHIP_6350;
	 fmDigitalAudioConfSupported = MCP_FALSE;
        break;

    case 6:
       /* pConfigid->chip_version = CAL_CHIP_6450_1_0;*/
	fmDigitalAudioConfSupported = MCP_TRUE;
        /* TODO ronen: understand how to distinguish between 1.0 and 1.1 */
	 if (1 == versionMajor)
        {
            pConfigid->chip_version = CAL_CHIP_6450_1_0;
        }
        else if (2 == versionMajor)
        {
            pConfigid->chip_version = CAL_CHIP_6450_2;
        }
        else
        {
            MCP_LOG_FATAL (("CCM_CAL_Configure: unrecognized chip version %d.%d.%d",
                            projectType, versionMajor, versionMinor));
        }
        break;

    case 7:
	fmDigitalAudioConfSupported = MCP_TRUE;
	  if (1 == versionMajor)
	{
		pConfigid->chip_version = CAL_CHIP_1273;
	}
	else if (2 == versionMajor)
	{
		pConfigid->chip_version = CAL_CHIP_1273_2;
	}
	else
	{
		MCP_LOG_FATAL (("CCM_CAL_Configure: unrecognized chip version %d.%d.%d",
						projectType, versionMajor, versionMinor));
	}
        break;

    case 8:
        pConfigid->chip_version = CAL_CHIP_5500;
	fmDigitalAudioConfSupported = MCP_TRUE;
        break;
    case 10:
        pConfigid->chip_version = CAL_CHIP_1283;
        fmDigitalAudioConfSupported = MCP_TRUE;
        break;

    default:
        MCP_LOG_FATAL (("CCM_CAL_Configure: unrecognized chip version %d.%d.%d",
                        projectType, versionMajor, versionMinor));
    }

    MCP_LOG_INFO(("Chip version is : %s", ChipToString(pConfigid->chip_version)));

	/* Get the Codec configuration params from the vac_conf.ini*/
    for (index = 0; index <=CONFIG_PARAM_MAX_VALUE;index++)
	{
		eCPStatus = MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
													 (McpU8 *)CCM_VAC_CONFIG_INI_CODEC_CONFIG_PARAM_SECTION_NAME,
													 (McpU8 *)codecConfigParams[index].keyName,
													 (McpS32*)&codecConfigParams[index].value);

        if (eCPStatus != MCP_CONFIG_PARSER_STATUS_SUCCESS)
		{
			MCP_LOG_FATAL (("CCM_CAL_Configure: while geting codec configuration, error reading %s's value",
						codecConfigParams[index].keyName));
		}
	}
    if (fmDigitalAudioConfSupported)
	{
        for (index = 0; index <=FM_PCM_PARAM_MAX_VALUE_LOC;index++)
		{
			eCPStatus = MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
														 (McpU8 *)CCM_VAC_CONFIG_INI_FM_PCMI_PARAM_SECTION_NAME,
														 (McpU8 *)fmPcmConfigParams[index].keyName,
														 (McpS32*)&fmPcmConfigParams[index].value);
		
            if (eCPStatus != MCP_CONFIG_PARSER_STATUS_SUCCESS)
			{
				MCP_LOG_FATAL (("CCM_CAL_Configure: while geting codec configuration, error reading %s's value",
							fmPcmConfigParams[index].keyName));
			}
		}
        for (index = 0; index <=FM_I2S_PARAM_MAX_VALUE_LOC;index++)
		{
			eCPStatus = MCP_CONFIG_PARSER_GetIntegerKey (pConfigid->pConfigParser, 
														 (McpU8 *)CCM_VAC_CONFIG_INI_FM_I2S_PARAM_SECTION_NAME,
														 (McpU8 *)fmI2sConfigParams[index].keyName,
														 (McpS32*)&fmI2sConfigParams[index].value);
		
            if (eCPStatus != MCP_CONFIG_PARSER_STATUS_SUCCESS)
			{
				MCP_LOG_FATAL (("CCM_CAL_Configure: while geting codec configuration, error reading %s's value",
							fmI2sConfigParams[index].keyName));
			}
		}
	}
}


void CAL_GetAudioCapabilities (Cal_Config_ID *pConfigid,
                               TCAL_MappingConfiguration *tOperationToResource,
                               TCAL_OpPairConfig *tOpPairConfig,
                               TCAL_ResourceSupport *tResource,
                               TCAL_OperationSupport *tOperation)

{   

    MCP_FUNC_START("CAL_GetAudioCapabilities");

    switch (pConfigid->chip_version)
    {
    case (CAL_CHIP_6350):                                           
        CAL_GetAudioCapabilities_6350 (tOperationToResource, tOpPairConfig,
                                       tResource, tOperation);
        break;                      
    case (CAL_CHIP_6450_1_0):
        CAL_GetAudioCapabilities_6450_1_0 (tOperationToResource, tOpPairConfig, 
                                           tResource, tOperation);
        break;                      
    case (CAL_CHIP_6450_2):
		CAL_GetAudioCapabilities_6450_2 (tOperationToResource, tOpPairConfig, 
										   tResource, tOperation);
        break;
    case (CAL_CHIP_1273):
		CAL_GetAudioCapabilities_1273 (tOperationToResource, tOpPairConfig, 
										   tResource, tOperation);
        break;
	case (CAL_CHIP_1273_2):
		CAL_GetAudioCapabilities_1273_2 (tOperationToResource, tOpPairConfig, 
										   tResource, tOperation);
        break;
	case (CAL_CHIP_1283):
		CAL_GetAudioCapabilities_1283 (tOperationToResource, tOpPairConfig, 
										   tResource, tOperation);
        break;
    case (CAL_CHIP_5500):
        /*todo ram - update when 5500 will be supported  */
        break;                              
        
    }

    MCP_FUNC_END();

}



/*FM-VAC*/

ECAL_RetValue CAL_ConfigResource(Cal_Config_ID *pConfigid,
                                 ECAL_Operation eOperation, 
                                 ECAL_Resource eResource,
                                 TCAL_DigitalConfig *ptConfig,
                                 TCAL_ResourceProperties *ptProperties,
                                 TCAL_CB fCB,
                                 void *pUserData)
{
    ECAL_RetValue status=CAL_STATUS_PENDING;
    BtHciIfStatus HciSeqstatus = BT_HCI_IF_STATUS_FAILED;   
    McpU32 uCommandcount = 0;
    McpBool callCbOnlyAfterLastCmd = MCP_FALSE;

    MCP_FUNC_START("CAL_ConfigResource");

    /*Update the Resource data struct per resource*/
    pConfigid->resourcedata.tResourceConfig[eResource].CB=fCB;
    pConfigid->resourcedata.tResourceConfig[eResource].pUserData=(void*)pUserData;
    pConfigid->resourcedata.tResourceConfig[eResource].eOperation=eOperation;
    pConfigid->resourcedata.tResourceConfig[eResource].ptConfig=ptConfig;
    pConfigid->resourcedata.tResourceConfig[eResource].eResource=eResource;
    MCP_HAL_MEMORY_MemCopy (&(pConfigid->resourcedata.tResourceConfig[ eResource ].tProperties),
                            ptProperties, sizeof (TCAL_ResourceProperties));

    /* Configure every resource according to the operation */
    switch (eResource)
    {
    case (CAL_RESOURCE_PCMH):                                               
    case (CAL_RESOURCE_I2SH):                                               
    case (CAL_RESOURCE_PCMT_1):
    case (CAL_RESOURCE_PCMT_2):
    case (CAL_RESOURCE_PCMT_3):
    case (CAL_RESOURCE_PCMT_4):
    case (CAL_RESOURCE_PCMT_5):
    case (CAL_RESOURCE_PCMT_6):
        /*Create Sequence command to configure the MUX for 6450 , 6350 dosent need MUX configuration*/
        switch (pConfigid->chip_version)
        {
        case (CAL_CHIP_6350):   
            status= CAL_STATUS_SUCCESS;
            MCP_LOG_INFO(("CAL_ConfigResource -CAL_CHIP_6350 MUX configuration is not needed"));
            break;

        case (CAL_CHIP_6450_1_0):   
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_Mux_Config_6450_1_0;
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
            uCommandcount=1;
            break;

        case (CAL_CHIP_6450_2):   
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_Mux_Config_6450_2;
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
    		uCommandcount=1;
                break;

    	case (CAL_CHIP_1273):	
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_Mux_Config_1273;
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
    		uCommandcount=1;
    		break;

    	case (CAL_CHIP_1273_2):	
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_Mux_Config_1273_2;
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
    		uCommandcount=1;
    		break;
    	case (CAL_CHIP_1283):	
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_Mux_Config_1283;
    		pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
    		uCommandcount=1;
    		break;

        case (CAL_CHIP_5500):
                MCP_LOG_ERROR(("5500 is not supported in this version!!!"));
                break;
        }                                                                                                                                                           
        break;  
    case (CAL_RESOURCE_PCMIF):
        if (CAL_OPERATION_FM_RX_OVER_A3DP == eOperation)
        {
            /* don't configure nothing for FM RX over A3DP */
            status= CAL_STATUS_SUCCESS;
        }
        else if (CAL_OPERATION_FM_RX_OVER_SCO == eOperation)
        {
            /* indicate this is a START FM RX over SCO command */
            pConfigid->resourcedata.tResourceConfig[eResource].bIsStart = MCP_TRUE;

            /* send FM over SCO command */
            switch (pConfigid->chip_version)
            {
            case CAL_CHIP_6350:
                pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
                    Cal_Prep_BtOverFm_Config_6350;
                pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData = 
                (void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
                uCommandcount=1;
                break;

            case CAL_CHIP_6450_1_0:
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
				Cal_Prep_AVPR_Enable_6450_1_0;
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData = 
			(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB =
				Cal_Prep_BtOverFm_Config_6450_1_0;
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData = 
			(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
			uCommandcount = 2;
			break;

		case CAL_CHIP_6450_2:		
                pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB =
                    Cal_Prep_BtOverFm_Config_6450_2;
                pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData = 
                (void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
                uCommandcount = 1;
                break;
				
		case CAL_CHIP_1273:
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
				Cal_Prep_AVPR_Enable_1273;
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData = 
			(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB =
				Cal_Prep_BtOverFm_Config_1273;
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData = 
			(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
			uCommandcount = 2;
			break;

		case CAL_CHIP_1273_2:	
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB =
                Cal_Prep_BtOverFm_Config_1273_2;
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData = 
            (void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
            uCommandcount = 1;
			break;
		case CAL_CHIP_1283:	
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB =
                Cal_Prep_BtOverFm_Config_1283;
            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData = 
            (void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
            uCommandcount = 1;
			break;

            case CAL_CHIP_5500:
                MCP_LOG_ERROR(("5500 is not supported in this version!!!"));
                break;
            }
        }
        else /* for all other operations, simply configure the resource */
        {
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_PCMIF_Config;
			pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
			uCommandcount=1;
        }
        break;                          
    case (CAL_RESOURCE_FMIF):
        /* no configuration is needed for FM RX over A3DP - done by the FW (according to A3DP configuration) */
            callCbOnlyAfterLastCmd = MCP_TRUE;
    
            if (ptProperties->uPropertiesNumber == 1)
            {
			switch (pConfigid->chip_version)
			{
			case CAL_CHIP_6450_1_0:
				{
						if (ptProperties->tResourcePorpeties[0] == 1)/* Indicates I2S */
						{
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB= Cal_Prep_FMIF_I2S_Config_6450;
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB= Cal_Prep_FMIF_PCM_I2s_mode_Config_6450;
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
							uCommandcount = 2;
						}
						else
						{
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_6450;
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_Pcm_mode_Config_6450;
							pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
						uCommandcount = 2;
						}
					}
				break;
			case CAL_CHIP_6450_2:
			{
					if (ptProperties->tResourcePorpeties[0] == 1)
					{
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_6450_2;
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_I2s_mode_Config_6450_2;
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
						uCommandcount = 2;
					}
					else
					{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_6450_2;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_Pcm_mode_Config_6450_2;
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
					}
				}
				break;
			case CAL_CHIP_1273:
			{
				if (ptProperties->tResourcePorpeties[0] == 1)/* Indicates I2S */
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1273;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_I2s_mode_Config_1273;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				else
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1273;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_Pcm_mode_Config_1273;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				break;
			}
			case CAL_CHIP_1273_2:
			{
				if (ptProperties->tResourcePorpeties[0] == 1)/* Indicates I2S */
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1273_2;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_I2s_mode_Config_1273_2;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				else
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1273_2;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_Pcm_mode_Config_1273_2;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				break;
			}
			case CAL_CHIP_1283:
			{
				if (ptProperties->tResourcePorpeties[0] == 1)/* Indicates I2S */
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1283;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_I2s_mode_Config_1283;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				else
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB=Cal_Prep_FMIF_I2S_Config_1283;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].fCommandPrepCB=Cal_Prep_FMIF_PCM_Pcm_mode_Config_1283;
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[1].pUserData=(void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));
					uCommandcount = 2;
				}
				break;
			}
			default:
				status= CAL_STATUS_SUCCESS;
				break;
		}
            }
        else
        {
            status= CAL_STATUS_SUCCESS;
        }
        break;
    case (CAL_RESOURCE_FM_ANALOG):
    case (CAL_RESOURCE_FM_CORE):                                                    
    case (CAL_RESOURCE_CORTEX):     
        status= CAL_STATUS_SUCCESS;
        MCP_LOG_INFO(("CAL_ConfigResource - Reasource configuration without need to configure "));
        break;
    case (CAL_RESOURCE_MAX_NUM):
    case (CAL_RESOURCE_INVALID):

        status= CAL_STATUS_FAILURE; 
        MCP_LOG_INFO(("CAL_ConfigResource -Wrong Reasource configuration "));
        break;
    }

    MCP_LOG_INFO(("CAL_ConfigResource -Resource %s  with Operation %s,Number of Commands %d"
                  ,ResourceToString(eResource),OperationToString(eOperation),uCommandcount)); 

    /*Verify that we got a command to send*/
    if (uCommandcount>0)
    {
        HciSeqstatus = MCP_HciSeq_RunSequence (&(pConfigid->resourcedata.tResourceConfig[eResource].hciseq),uCommandcount, 
                                               &(pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0]),callCbOnlyAfterLastCmd);
        if (BT_HCI_IF_STATUS_PENDING != HciSeqstatus)
        {
            MCP_LOG_ERROR (("MCP_HciSeq_RunSequence failed with status %s", IFStatusToString(HciSeqstatus)));;
            status = CAL_STATUS_FAILURE;
        }
    }

    MCP_FUNC_END();

    return status;

}


/*---------------------------------------------------------------------------
 *            Cal_Prep_PCMIF_Config()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Prepare function for thr PCM interface
 *
 */
MCP_STATIC void Cal_Prep_PCMIF_Config(McpHciSeqCmdToken *pToken, void *pUserData)
{

    Cal_Resource_Data  *presourcedata = (Cal_Resource_Data*)pUserData;

    pToken->callback = CAL_Config_CB_Complete;
    pToken->eHciOpcode= BT_HCI_IF_HCI_CMD_VS_WRITE_CODEC_CONFIGURE;
    pToken->uCompletionEvent= BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE;
    pToken->pHciCmdParms=&(presourcedata->hciseqCmdPrms[0]);
    pToken->uhciCmdParmsLen =Set_params_PCMIF(presourcedata,&(presourcedata->hciseqCmdPrms[0]));
    pToken->pUserData = (void*)presourcedata;
}


MCP_STATIC McpU8 Set_params_PCMIF(Cal_Resource_Data  *presourcedata,McpU8 *pBuffer)
{

    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[PCM_CLOCK_RATE_LOC].value),&(pBuffer[ 0 ])); 
    pBuffer[ 2 ] = (McpU8)(codecConfigParams[PCM_DIRECTION_ROLE_LOC].value);	 
    MCP_ENDIAN_HostToLE32 (CAL_SAMPLE_FREQ_TO_CODEC(presourcedata->ptConfig->eSampleFreq),&(pBuffer[ 3 ]));    /*Frame Sync Frequency*/
    MCP_ENDIAN_HostToLE16 ((McpU16)(codecConfigParams[FRAME_SYNC_DUTY_CYCLE_LOC].value),&(pBuffer[ 7 ])); 
    pBuffer[ 9 ] = (McpU8)(codecConfigParams[FRAME_SYNC_EDGE_LOC].value); 
    pBuffer[ 10 ] = (McpU8)(codecConfigParams[FRAME_SYNC_POLARITY_LOC].value);  
    pBuffer[ 11 ] = 0;  /*Reserved */
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH1_DATA_OUT_SIZE_LOC].value),&(pBuffer[ 12 ])); 
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH1_DATA_OUT_OFFSET_LOC].value),&(pBuffer[ 14 ]));
    pBuffer[ 16 ] = (McpU8)(codecConfigParams[CH1_OUT_EDGE_LOC].value);
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH1_DATA_IN_SIZE_LOC].value),&(pBuffer[ 17 ]));
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH1_DATA_IN_OFFSET_LOC].value),&(pBuffer[ 19 ]));
    pBuffer[ 21 ] = (McpU8)(codecConfigParams[CH1_IN_EDGE_LOC].value);
    pBuffer[ 22 ] = 0;/*Reserved */
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH2_DATA_OUT_SIZE_LOC].value),&(pBuffer[ 23 ]));
    MCP_ENDIAN_HostToLE16((McpU16)(codecConfigParams[CH2_DATA_OUT_OFFSET_LOC].value),&(pBuffer[ 25 ]));
    pBuffer[ 27 ] = (McpU8)(codecConfigParams[CH2_OUT_EDGE_LOC].value);
    MCP_ENDIAN_HostToLE16((McpU8)(codecConfigParams[CH2_DATA_IN_SIZE_LOC].value),&(pBuffer[ 28 ]));
    MCP_ENDIAN_HostToLE16((McpU8)(codecConfigParams[CH2_DATA_IN_OFFSET_LOC].value),&(pBuffer[ 30 ]));
    pBuffer[ 32 ] = (McpU8)(codecConfigParams[CH2_IN_EDGE_LOC].value);
    pBuffer[ 33 ] = 0;/*Reserved */
    return 34; /* number of bytes written */

}
ECAL_RetValue CAL_StopResourceConfiguration (Cal_Config_ID *pConfigid,
                                             ECAL_Operation eOperation,
                                             ECAL_Resource eResource,
                                             TCAL_ResourceProperties *ptProperties,
                                             TCAL_CB fCB,
                                             void *pUserData)
{
    BtHciIfStatus       HciSeqstatus;
	McpU32 uCommandcount = 0;
    ECAL_RetValue       status = CAL_STATUS_SUCCESS;

    MCP_FUNC_START("CalGenericHciCB");

        /* copy parameters */
        pConfigid->resourcedata.tResourceConfig[eResource].CB=fCB;
        pConfigid->resourcedata.tResourceConfig[eResource].pUserData=(void*)pUserData;
        pConfigid->resourcedata.tResourceConfig[eResource].eOperation=eOperation;
        pConfigid->resourcedata.tResourceConfig[eResource].eResource=eResource;
        MCP_HAL_MEMORY_MemCopy (&(pConfigid->resourcedata.tResourceConfig[ eResource ].tProperties),
                                ptProperties, sizeof (TCAL_ResourceProperties));        
        /* indicate this is a STOP FM RX over SCO command */
        pConfigid->resourcedata.tResourceConfig[eResource].bIsStart = MCP_FALSE;

    switch (eResource)
	{
		case (CAL_RESOURCE_PCMH):												
		case (CAL_RESOURCE_I2SH):												
		case (CAL_RESOURCE_PCMT_1):
		case (CAL_RESOURCE_PCMT_2):
		case (CAL_RESOURCE_PCMT_3):
		case (CAL_RESOURCE_PCMT_4):
		case (CAL_RESOURCE_PCMT_5):
		case (CAL_RESOURCE_PCMT_6):
            if ((CAL_OPERATION_FM_RX == eOperation)||
        		(CAL_OPERATION_FM_TX == eOperation))
    		{
                if (pConfigid->chip_version == CAL_CHIP_6450_2)
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_Mux_Disable_6450_2;
					uCommandcount =1;
				}
                else if (pConfigid->chip_version == CAL_CHIP_1273_2)
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_Mux_Disable_1273_2;
					uCommandcount =1;
				}
                else if (pConfigid->chip_version == CAL_CHIP_1283)
				{
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_Mux_Disable_1283;
					uCommandcount =1;
				}
    			else 
    			{
    				status= CAL_STATUS_SUCCESS;
    			}
			}
			else
			{
				status= CAL_STATUS_SUCCESS;
			}
			break;
		case CAL_RESOURCE_PCMIF:
			/* only for PCM-IF on FM RX over SCO do we need to send a de-configuration command */
        if (CAL_OPERATION_FM_RX_OVER_SCO == eOperation)
			{
	        /* send FM over SCO command */
		        switch (pConfigid->chip_version)
		        {
		        case CAL_CHIP_6350:
		            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
		                Cal_Prep_BtOverFm_Config_6350;
							uCommandcount =1;
		            break;

		        case CAL_CHIP_6450_1_0:
		            pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
		                Cal_Prep_BtOverFm_Config_6450_1_0;
							uCommandcount =1;
		            break;

				case CAL_CHIP_6450_2:		
						pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
							Cal_Prep_BtOverFm_Config_6450_2;
								uCommandcount =1;
						break;

				case CAL_CHIP_1273:
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_BtOverFm_Config_1273;
								uCommandcount =1;
					break;
				case CAL_CHIP_1273_2:
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_BtOverFm_Config_1273_2;
								uCommandcount =1;
                break;
				case CAL_CHIP_1283:
					pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].fCommandPrepCB =
						Cal_Prep_BtOverFm_Config_1283;
					uCommandcount =1;
                break;

            case (CAL_CHIP_5500):
                MCP_LOG_ERROR(("5500 is not supported in this version!!!"));
					break;
		        }
			}
			else
			{
				status= CAL_STATUS_SUCCESS;
			}
			break;
			case (CAL_RESOURCE_FMIF):
			case (CAL_RESOURCE_FM_ANALOG):
			case (CAL_RESOURCE_FM_CORE):													
			case (CAL_RESOURCE_CORTEX): 	
				status= CAL_STATUS_SUCCESS;
				MCP_LOG_INFO(("CAL_ConfigResource - Reasource configuration stoped without need to configure "));
				break;
			case (CAL_RESOURCE_MAX_NUM):
			case (CAL_RESOURCE_INVALID):
			
				status= CAL_STATUS_FAILURE; 
				MCP_LOG_INFO(("CAL_ConfigResource -Wrong Reasource stop configuration "));
				break;
			default:
				
				status= CAL_STATUS_SUCCESS;

	}
    if (uCommandcount>0)
	{
        pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0].pUserData = 
            (void*)(&(pConfigid->resourcedata.tResourceConfig[eResource]));

        HciSeqstatus = MCP_HciSeq_RunSequence (&(pConfigid->resourcedata.tResourceConfig[eResource].hciseq), 1, 
                                               &(pConfigid->resourcedata.tResourceConfig[eResource].hciseqCmd[0]), 
                                               MCP_FALSE);
        if (BT_HCI_IF_STATUS_PENDING != HciSeqstatus)
        {
            MCP_LOG_ERROR (("MCP_HciSeq_RunSequence failed with status %s", IFStatusToString(HciSeqstatus)));;
            status = CAL_STATUS_FAILURE;
        }
        else
        {
            status = CAL_STATUS_PENDING;
        }
    }
    else
    {
        /* 
         * for all other resources and scenarios just cancel the sequence (no harm even if
         * it's not running)
         */
        MCP_HciSeq_CancelSequence(&(pConfigid->resourcedata.tResourceConfig[eResource].hciseq));
    }

    MCP_FUNC_END();

    return status;
}

/*---------------------------------------------------------------------------
 *            CAL_Config_CB_Complete()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  generic callback function for command complete on HCI commands 
 *            directly followed by other commands
 *
 */

void CAL_Config_CB_Complete(BtHciIfClientEvent *pEvent)
{
    Cal_Resource_Data   *pResourceData = (Cal_Resource_Data*)(pEvent->pUserData);
    ECAL_RetValue       status = CAL_STATUS_SUCCESS;                  

    MCP_FUNC_START("CAL_Config_CB_Complete");

    /*Verify that the status is success */
    if (pEvent->status != BT_HCI_IF_STATUS_SUCCESS)
    {
        status = CAL_STATUS_FAILURE;
    }

    MCP_LOG_INFO (("CAL_Config_CB_Complete: received event->type %d with status %d", 
                  pEvent->type,pEvent->status));

    /*Call the client CB */
    pResourceData->CB(pResourceData->pUserData,status);         

    MCP_FUNC_END();

}

void CAL_Config_Complete_Null_CB(BtHciIfClientEvent *pEvent)
{
    MCP_FUNC_START("CAL_Config_Complete_Null_CB");

    MCP_LOG_INFO (("CAL_Config_Complete_Null_CB: received event type %d with status %d", 
                  pEvent->type, pEvent->status));

    MCP_FUNC_END();
}

ECAL_Resource StringtoResource(const McpU8 *str1)
{

    McpU8 status=0;
    ECAL_Resource idx;
    const char *str2;

    for (idx=0;idx<CAL_RESOURCE_MAX_NUM;idx++)
    {
        str2=ResourceToString(idx);
        status=MCP_HAL_STRING_StrCmp((const char *)str1,str2);
        if (status==NULL)
        {
            return idx;
        }
    }

    return CAL_RESOURCE_INVALID;
}


ECAL_Operation StringtoOperation(const McpU8 *str1)
{

    McpU8 status=0;
    ECAL_Resource idx;
    const char *str2;

    for (idx=0;idx<CAL_OPERATION_MAX_NUM;idx++)
    {
        str2=OperationToString(idx);
        status=MCP_HAL_STRING_StrCmp((const char *)str1,str2);
        if (status==NULL)
        {
            return idx;
        }
    }

    return CAL_OPERATION_INVALID;

}



/*---------------------------------------------------------------------------
 *            ChipToString()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Convert enum eChip_Version to string
 *
 */
MCP_STATIC const char *ChipToString(ECAL_ChipVersion chip)
{
    switch (chip)
    {
    case CAL_CHIP_6350:
        return "6350";
    case CAL_CHIP_6450_1_0:
        return "6450_1_0";
    case CAL_CHIP_6450_2:
        return "6450_2";
    case CAL_CHIP_1273:
        return "1273";
    case CAL_CHIP_1273_2:
    	return "1273_2";
    case CAL_CHIP_1283:
        return "1283";
    case CAL_CHIP_5500:
        return "5500";    
    }
    return "UNKNOWN";
}

/*---------------------------------------------------------------------------
 *            OperationToString()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Convert enum Operation to string
 *
 */
MCP_STATIC const char *OperationToString(ECAL_Operation eOperation)
{
    switch (eOperation)
    {
    case CAL_OPERATION_FM_TX:
        return "CAL_OPERATION_FM_TX";
    case CAL_OPERATION_FM_RX:
        return "CAL_OPERATION_FM_RX";
    case CAL_OPERATION_A3DP:
        return "CAL_OPERATION_A3DP";
    case CAL_OPERATION_BT_VOICE:
        return "CAL_OPERATION_BT_VOICE";
    case CAL_OPERATION_WBS:
        return "CAL_OPERATION_WBS";    
    case CAL_OPERATION_AWBS:
        return "CAL_OPERATION_AWBS";
    case CAL_OPERATION_FM_RX_OVER_SCO:
        return "CAL_OPERATION_FM_RX_OVER_SCO";
    case CAL_OPERATION_FM_RX_OVER_A3DP:
        return "CAL_OPERATION_FM_RX_OVER_A3DP";
    default:
        return "UNKNOWN";
    }
}



/*---------------------------------------------------------------------------
 *            ResourceToString()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Convert enum Resource to string
 *
 */

MCP_STATIC const char *ResourceToString(ECAL_Resource eResource)
{
    switch (eResource)
    {
    case CAL_RESOURCE_I2SH:
        return "CAL_RESOURCE_I2SH";
    case CAL_RESOURCE_PCMH:
        return "CAL_RESOURCE_PCMH";
    case CAL_RESOURCE_PCMT_1:
        return "CAL_RESOURCE_PCMT_1";
    case CAL_RESOURCE_PCMT_2:
        return "CAL_RESOURCE_PCMT_2";
    case CAL_RESOURCE_PCMT_3:
        return "CAL_RESOURCE_PCMT_3";    
    case CAL_RESOURCE_PCMT_4:
        return "CAL_RESOURCE_PCMT_4";    
    case CAL_RESOURCE_PCMT_5:
        return "CAL_RESOURCE_PCMT_5";
    case CAL_RESOURCE_PCMT_6:
        return "CAL_RESOURCE_PCMT_6";    
    case CAL_RESOURCE_PCMIF:
        return "CAL_RESOURCE_PCMIF";    
    case CAL_RESOURCE_FMIF:
        return "CAL_RESOURCE_FMIF";    
    case CAL_RESOURCE_FM_ANALOG:
        return "CAL_RESOURCE_FM_ANALOG";    
    case CAL_RESOURCE_CORTEX:
        return "CAL_RESOURCE_CORTEX";    
    case CAL_RESOURCE_FM_CORE:
        return "CAL_RESOURCE_FM_CORE";   
    default:
        return "UNKNOWN";
    }
}


MCP_STATIC const char *IFStatusToString(BtHciIfStatus status)
{
    switch (status)
    {
    case BT_HCI_IF_STATUS_SUCCESS:
        return "BT_HCI_IF_STATUS_SUCCESS";
    case BT_HCI_IF_STATUS_FAILED:
        return "BT_HCI_IF_STATUS_FAILED";
    case BT_HCI_IF_STATUS_PENDING:
        return "BT_HCI_IF_STATUS_PENDING";
    case BT_HCI_IF_STATUS_INTERNAL_ERROR:
        return "BT_HCI_IF_STATUS_INTERNAL_ERROR";
    case BT_HCI_IF_STATUS_IMPROPER_STATE:
        return "BT_HCI_IF_STATUS_IMPROPER_STATE";
    default:
        return "UNKNOWN";
    }
}

