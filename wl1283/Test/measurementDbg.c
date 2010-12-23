/*
 * measurementDbg.c
 *
 * Copyright(c) 1998 - 2010 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file measurementDbg.c
 *  \brief measurement module Debug interface
 *
 *  \see measurementDbg.h
 */

/***************************************************************************/
/*																	       */
/*	  MODULE:	measurementDbg.c										   */
/*    PURPOSE:	measurement module Debug interface						   */
/*																		   */
/***************************************************************************/
#include "tidef.h"
#include "osApi.h"
#include "report.h"
#include "paramOut.h"
#include "measurementDbg.h"
#include "siteMgrApi.h"
#include "measurementMgr.h"
#include "MeasurementSrv.h"
#include "MacServices_api.h"
#include "sme.h"
#ifdef XCC_MODULE_INCLUDED  
#include "XCCRMMngr.h"
#endif
#include "SwitchChannelApi.h"
#include "TWDriver.h"
#include "rrmMgr.h"

#include "TWDriverMsr.h"


TI_UINT8     actionFrameBuffer[2000];


static void measurement_buildBeaconRequestDbg(TI_HANDLE hMeasurementMgr, TI_UINT8 mode,
                                              TI_UINT8 channel,
                                              TI_UINT8 regClass,
                                              TI_UINT8* pBeaconReqActionFrame, 
                                              TI_UINT32 *outLen);

static void measurement_buildTSMRequestDbg(TI_HANDLE    hMeasurementMgr, 
                                           TI_BOOL      isTriggeredReport,
                                           TI_UINT16    duration,
                                           TI_UINT8     tid,
                                           TI_UINT32    *outLen);

void printMeasurementDbgFunctions(void);   

void regDomainPrintValidTables(TI_HANDLE hRegulatoryDomain);

TI_UINT32			channelNum;
TI_BOOL            flag;

/*************************************************************************
 *					measurementDebugFunction							 *
 *************************************************************************
DESCRIPTION:                  
                                                      
INPUT:       

OUTPUT:      

RETURN:     
                                                   
************************************************************************/
void measurementDebugFunction(TI_HANDLE hMeasurementMgr, TI_HANDLE hSwitchChannel, TI_HANDLE hRegulatoryDomain, TI_UINT32 funcType, void *pParam)
{
	paramInfo_t		param;
#ifdef XCC_MODULE_INCLUDED
    TTwdParamInfo   tTwdParam;
#endif
	TI_STATUS	    status = TI_OK;
    TI_UINT8           rangeUpperBound;
    TI_UINT8           rangeIndex;
    TI_UINT16          trafficThreshold;
    TNoiseHistogram pNoiseHistParams;
	measurementMgr_t * pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
	TI_UINT8			SwitchChannelParam = *(TI_UINT8*)pParam;
	siteMgr_t		*pSiteMgr = (siteMgr_t *) pMeasurementMgr->hSiteMgr;
    TI_UINT32       bufferLen = 0;
    TI_UINT8        channel = 0;
    TI_UINT8        mode = 0; /* 0 = passive, 1=active, 2=table */
    TI_UINT8        regClass = 12; /* 12 = B/G ,  1 = A band */
    TI_UINT8        tid = 0; 
    TI_UINT16       duration = 0;
    TI_BOOL         isTriggeredReport = TI_FALSE;
    


    
#ifdef XCC_MODULE_INCLUDED
    TI_UINT8           iappPacket[90] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00, 
                                      0x00, 0x20, 0x32, 0x01, 
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x20, 0x00, 0x01, 0x02, 
                                      0x26, 0x00, 0x08, 0x00, 0xA1, 0x00, 0x00, 0x01, 
                                      0x0B, 0x00, 0x26, 0x26};
    
    TI_UINT8           iappPacket1[76] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00, 
                                       0x00, 0x20, 0x32, 0x01, 
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                       0x30, 0x00, 0x01, 0x02, 
                                       0x26, 0x00, 0x08, 0x00, 0xF1, 0x00, 0x00, 0x01, 
                                       0x06, 0x00, 0x64, 0x00};

    TI_UINT8           iappPacket2[76] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00, 
                                       0x00, 0x20, 0x32, 0x01, 
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                       0x13, 0x00, 0x01, 0x02, 
                                       0x26, 0x00, 0x08, 0x00, 0xA3, 0x00, 0x00, 0x03, 
                                       0x0B, 0x02, 0xD1, 0x03};

    TI_UINT8           iappPacket3[76] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00,
                                       0x00, 0x38, 0x32, 0x01,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x20, 0x00, 0x01, 0x02,
                                       0x26, 0x00, 0x08, 0x00, 0xA1, 0x00, 0x00, 0x03,
                                       0x03,
									   0X00, 0XFF, 0X00, 0X26, 0X00, 0X08,
									   0X00, 0XC1, 0X00, 0X00, 0X02, 0X03,
									   0X00, 0XFF, 0X00, 0X26, 0X00, 0X08,
									   0X00, 0XB1, 0X00, 0X00, 0X01, 0X03,
				                       0X00, 0XFF, 0X00};

    TI_UINT8           iappPacket4[76] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00,
                                       0x00, 0x38, 0x32, 0x01,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x20, 0x00, 0x01, 0x02,
                                       0x26, 0x00, 0x08, 0x00, 0xA1, 0x00, 0x00, 0x03,
                                       0x06,
									   0X00, 0X32, 0X00, 0X26, 0X00, 0X08,
									   0X00, 0XC1, 0X00, 0X00, 0X02, 0X01,
									   0X00, 0XFF, 0X00, 0X26, 0X00, 0X08,
									   0X00, 0XB1, 0X00, 0X01, 0X01, 0X01,
				                       0X00, 0X96, 0X00};
#endif

	switch (funcType)
	{
	case DBG_MEASUREMENT_PRINT_HELP:
		printMeasurementDbgFunctions();
		break;
	case DBG_MEASUREMENT_PRINT_STATUS:
		param.paramType = MEASUREMENT_GET_STATUS_PARAM;
		measurementMgr_getParam(hMeasurementMgr, &param);
		break;

#if 0

	case DBG_MEASUREMENT_CHANNEL_LOAD_START:
        
        /* Clearing the Medium Occupancy Register */
        tTwdParam.paramType = HAL_CTRL_MEDIUM_OCCUPANCY_PARAM;
        tTwdParam.content.interogateCmdCBParams.CB_Func = measurement_mediumUsageCB;
        tTwdParam.content.interogateCmdCBParams.CB_handle = hMeasurementMgr;
        tTwdParam.content.interogateCmdCBParams.CB_buf = (TI_UINT8*)(&(pMeasurementMgr->mediumOccupancyStart));

        if ((status = TWD_GetParam (pMeasurementMgr->hTWD, &tTwdParam)) == TI_OK)
        {
            WLAN_OS_REPORT(("%s: MEASUREMENT - Channel Load Started!\n", __FUNCTION__));            
		}
		
		break;

    case DBG_MEASUREMENT_CHANNEL_LOAD_STOP:
        
        /* Getting the Medium Occupancy Register */
        tTwdParam.paramType = HAL_CTRL_MEDIUM_OCCUPANCY_PARAM;
        tTwdParam.content.interogateCmdCBParams.CB_Func = measurement_channelLoadCallBackDbg;
        tTwdParam.content.interogateCmdCBParams.CB_handle = hMeasurementMgr;
        tTwdParam.content.interogateCmdCBParams.CB_buf = (TI_UINT8*)(&(pMeasurementMgr->mediumOccupancyEnd));
        
        if ((status = TWD_GetParam (pMeasurementMgr->hTWD, &tTwdParam)) == TI_OK)
        {
            WLAN_OS_REPORT(("%s: MEASUREMENT - Channel Load Stoped!", __FUNCTION__));            
        }

		break;

#endif /* 0 */

    case DBG_MEASUREMENT_SEND_FRAME_REQUEST:

	    WLAN_OS_REPORT(("-----------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: Sending Frame Request \n"));
	    WLAN_OS_REPORT(("-----------------------------------------------------\n"));

        WLAN_OS_REPORT(("beaconInterval = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->beaconInterval));
		WLAN_OS_REPORT(("dtimPeriod = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->dtimPeriod));

#ifdef XCC_MODULE_INCLUDED        
        measurementMgr_XCCParse(hMeasurementMgr, iappPacket);
#endif

	    WLAN_OS_REPORT(("-----------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: END                   \n"));
	    WLAN_OS_REPORT(("-----------------------------------------------------\n"));

        break;

    case DBG_MEASUREMENT_START_NOISE_HIST:
        /* Set Noise Histogram Cmd Params */
        pNoiseHistParams.cmd = START_NOISE_HIST;
        pNoiseHistParams.sampleInterval = 100;
        os_memoryZero(pMeasurementMgr->hOs, &(pNoiseHistParams.ranges[0]), MEASUREMENT_NOISE_HISTOGRAM_NUM_OF_RANGES);
        
        /* Set Ranges */
        rangeUpperBound = (TI_UINT8)-87; /* TWD_convertRSSIToRxLevel(pMeasurementMgr->hTWD, -87);*/
        for(rangeIndex = 0; rangeIndex < MEASUREMENT_NOISE_HISTOGRAM_NUM_OF_RANGES - 1; rangeIndex++)
        {
            pNoiseHistParams.ranges[rangeIndex] = rangeUpperBound;
            rangeUpperBound += 5; 
        }
        pNoiseHistParams.ranges[rangeIndex] = 0xFE;
        
        /* Send a Start command to the FW */
        status = TWD_CmdNoiseHistogram (pMeasurementMgr->hTWD, &pNoiseHistParams);		

        WLAN_OS_REPORT(("Measurement Debug Functions - Start Noise Hist Succeded"));
        
        if (status != TI_OK)
            WLAN_OS_REPORT(("Measurement Debug Functions - Start Noise Hist FAILED"));
        
        break;
        
    case DBG_MEASUREMENT_STOP_NOISE_HIST:
        /* Set Noise Histogram Cmd Params */
        pNoiseHistParams.cmd = STOP_NOISE_HIST;
        pNoiseHistParams.sampleInterval = 0;
        os_memoryZero(pMeasurementMgr->hOs, &(pNoiseHistParams.ranges[0]), MEASUREMENT_NOISE_HISTOGRAM_NUM_OF_RANGES);
        
        /* Send a Stop command to the FW */
        status = TWD_CmdNoiseHistogram (pMeasurementMgr->hTWD, &pNoiseHistParams);
			
        WLAN_OS_REPORT(("Measurement Debug Functions - Stop Noise Hist Succeded"));
        
        if (status != TI_OK)
            WLAN_OS_REPORT(("Measurement Debug Functions - Stop Noise Hist FAILED"));
        
        break;

    case DBG_MEASUREMENT_GET_NOISE_HIST_RESULTS:
		{
	#ifdef XCC_MODULE_INCLUDED
			TNoiseHistogramResults results;

			/* Get measurement results */
			tTwdParam.paramType = TWD_NOISE_HISTOGRAM_PARAM_ID;
			tTwdParam.content.interogateCmdCBParams.fCb = (void *)measurement_noiseHistCallBackDbg;
			tTwdParam.content.interogateCmdCBParams.hCb = ((TTwd *)pMeasurementMgr->hTWD)->hMeasurementSRV;
			tTwdParam.content.interogateCmdCBParams.pCb = (TI_UINT8 *)&results;

			TWD_GetParam (pMeasurementMgr->hTWD, &tTwdParam);
	#endif               

			break;
		}

    case DBG_MEASUREMENT_SEND_CHANNEL_LOAD_FRAME:
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: Sending another Frame Request   \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        WLAN_OS_REPORT(("beaconInterval = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->beaconInterval));
		WLAN_OS_REPORT(("dtimPeriod = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->dtimPeriod));

#ifdef XCC_MODULE_INCLUDED        
        measurementMgr_XCCParse(hMeasurementMgr, iappPacket1);
#endif

	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: END                             \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        break;




        
    case DBG_MEASUREMENT_SEND_BEACON_TABLE_FRAME:
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: Sending Beacon Table Request    \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        WLAN_OS_REPORT(("beaconInterval = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->beaconInterval));
		WLAN_OS_REPORT(("dtimPeriod = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->dtimPeriod));

#ifdef XCC_MODULE_INCLUDED        
        measurementMgr_XCCParse(hMeasurementMgr, iappPacket2);
#endif

	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: END                             \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        break;

		

        
    case DBG_MEASUREMENT_SEND_NOISE_HIST_1_FRAME:
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: Sending Unknown Request #1      \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        WLAN_OS_REPORT(("beaconInterval = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->beaconInterval));
		WLAN_OS_REPORT(("dtimPeriod = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->dtimPeriod));

#ifdef XCC_MODULE_INCLUDED        
        measurementMgr_XCCParse(hMeasurementMgr, iappPacket3);
#endif

	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: END                             \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        break;





    case DBG_MEASUREMENT_SEND_NOISE_HIST_2_FRAME:
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: Sending Unknown Request #1      \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        WLAN_OS_REPORT(("beaconInterval = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->beaconInterval));
		WLAN_OS_REPORT(("dtimPeriod = %d\n", pSiteMgr->pSitesMgmtParams->pPrimarySite->dtimPeriod));

#ifdef XCC_MODULE_INCLUDED        
        measurementMgr_XCCParse(hMeasurementMgr, iappPacket4);
#endif

	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));
        WLAN_OS_REPORT(("   Measurement Debug Function: END                             \n"));
	    WLAN_OS_REPORT(("---------------------------------------------------------------\n"));

        break;





    case DBG_MEASUREMENT_SET_TRAFFIC_THRSLD:

        trafficThreshold = *(TI_UINT16*)pParam;
        param.paramType = MEASUREMENT_TRAFFIC_THRESHOLD_PARAM;
        param.content.measurementTrafficThreshold = trafficThreshold;
        measurementMgr_setParam(hMeasurementMgr, &param);
        break;
		

	case DBG_SC_PRINT_STATUS:
		switchChannelDebug_printStatus(hSwitchChannel);
		break;
	case DBG_SC_SET_SWITCH_CHANNEL_NUM:
		switchChannelDebug_setCmdParams(hSwitchChannel, SC_SWITCH_CHANNEL_NUM , SwitchChannelParam);
		break;
	case DBG_SC_SET_SWITCH_CHANNEL_TBTT:
		switchChannelDebug_setCmdParams(hSwitchChannel, SC_SWITCH_CHANNEL_TBTT , SwitchChannelParam);
		break;
	case DBG_SC_SET_SWITCH_CHANNEL_MODE:
		switchChannelDebug_setCmdParams(hSwitchChannel, SC_SWITCH_CHANNEL_MODE , SwitchChannelParam);
		break;
	case DBG_SC_SET_CHANNEL_AS_VALID:
		switchChannelDebug_setChannelValidity(hSwitchChannel, SwitchChannelParam, TI_TRUE);
		break;
	case DBG_SC_SET_CHANNEL_AS_INVALID:
		switchChannelDebug_setChannelValidity(hSwitchChannel, SwitchChannelParam, TI_FALSE);
		break;
	case DBG_SC_SWITCH_CHANNEL_CMD:
		{
			switchChannelDebug_SwitchChannelCmdTest(hSwitchChannel, SwitchChannelParam);
		}
		break;
	case DBG_SC_CANCEL_SWITCH_CHANNEL_CMD:
		if ((SwitchChannelParam!=TI_TRUE) && (SwitchChannelParam!=TI_FALSE))
		{	/* default is TI_TRUE */
			SwitchChannelParam = TI_TRUE;
		}
		switchChannelDebug_CancelSwitchChannelCmdTest(hSwitchChannel, SwitchChannelParam);
		break;

	case DBG_REG_DOMAIN_PRINT_VALID_CHANNELS:
		regDomainPrintValidTables(hRegulatoryDomain);
		break;
        
    case DBG_MEASUREMENT_SIMULATE_NEIGHBOR_REPORT_REQUEST:
        {
            param.paramType = SITE_MGR_CURRENT_SSID_PARAM;
            siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);
            rrmMgr_buildAndSendNeighborAPsRequest(hMeasurementMgr, &param.content.siteMgrCurrentSSID);
            break;
        }

    case DBG_MEASUREMENT_SIMULATE_BEACON_REQUEST_SCAN_PASSIVE_BAND_BG: /* 39 */
    {
        bufferLen = 0;
        channel = 6;
        mode = 0; /* 0 = passive, 1=active, 2=table */
        regClass = 12; /* 12 = B/G ,  1 = A band */
        

        measurement_buildBeaconRequestDbg(hMeasurementMgr, mode,channel, regClass, actionFrameBuffer, &bufferLen);

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);
 
        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
    }

    case DBG_MEASUREMENT_SIMULATE_BEACON_REQUEST_SCAN_PASSIVE_BAND_A:
    {
         bufferLen = 0;
         channel = 36;
         mode = 0; /* 0 = passive, 1=active, 2=table */
         regClass = 1; /* 12 = B/G ,  1 = A band */
             
        measurement_buildBeaconRequestDbg(hMeasurementMgr, mode,channel , regClass,  actionFrameBuffer, &bufferLen);

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);
        
        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
            
    }

    case DBG_MEASUREMENT_SIMULATE_BEACON_REQUEST_SCAN_ACTIVE_BAND_BG:
    {
        bufferLen = 0;
        channel = 1;
        mode = 1;       /* 0 = passive, 1=active, 2=table */
        regClass = 12;  /* 12 = B/G ,  1 = A band */

        measurement_buildBeaconRequestDbg(hMeasurementMgr, mode, channel, regClass, actionFrameBuffer, &bufferLen);

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);

        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                               MSR_FRAME_TYPE_UNICAST,
                               &param.content.siteMgrDesiredBSSID,
                               bufferLen,
                               actionFrameBuffer);
        break;
        
    }
    case DBG_MEASUREMENT_SIMULATE_BEACON_REQUEST_SCAN_ACTIVE_BAND_A:
    {
        bufferLen = 0;
        channel = 60;
        mode = 1;       /* 0 = passive, 1=active, 2=table */
        regClass = 1;   /* 12 = B/G ,  1 = A band */

        measurement_buildBeaconRequestDbg(hMeasurementMgr, mode, channel, regClass, actionFrameBuffer, &bufferLen);

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);

        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
    }

    case DBG_MEASUREMENT_SIMULATE_BEACON_REQUEST_SCAN_TABLE_BAND_BG: 
    {
        bufferLen = 0;
        channel = 0;
        mode = 2; /* 0 = passive, 1=active, 2=table */
        regClass = 12; /* 12 = B/G ,  1 = A band */


        measurement_buildBeaconRequestDbg(hMeasurementMgr, mode,channel, regClass, actionFrameBuffer, &bufferLen);

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);

   
        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
    }

    
    /* TSM REPORT */
    case DBG_MEASUREMENT_SIMULATE_TSM_REQUEST_REPORT_NORMAL: 
    {
        bufferLen = 0;
        tid = 0;
        isTriggeredReport = TI_FALSE; 
        duration  = 10000; 


        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);

        
        measurement_buildTSMRequestDbg(hMeasurementMgr, isTriggeredReport, duration , tid, &bufferLen);
       

        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
    }


    case DBG_MEASUREMENT_SIMULATE_TSM_REQUEST_REPORT_TRIGGERED: 
    {
        bufferLen = 0;
        tid = 0;
        isTriggeredReport = TI_TRUE; 
        duration  = 0; 

        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr, &param);

        measurement_buildTSMRequestDbg(hMeasurementMgr, isTriggeredReport, duration , tid, &bufferLen);

        rrmMgr_receiveFrameRequest(hMeasurementMgr, 
                                   MSR_FRAME_TYPE_UNICAST,
                                   &param.content.siteMgrDesiredBSSID,
                                   bufferLen,
                                   actionFrameBuffer);
        break;
    }
        
	default:
		WLAN_OS_REPORT(("Invalid function type in MEASUREMENT Function Command: %d\n", funcType));
		break;
	}
} 


/* 0-passive,  1-active scan , 2-table */
static void measurement_buildBeaconRequestDbg(TI_HANDLE hMeasurementMgr, TI_UINT8 mode, TI_UINT8 channel,TI_UINT8 regClass,
                                              TI_UINT8* pBeaconReqActionFrame, TI_UINT32 *outLen)
{
    TI_UINT8* ssidBuff = "arik";
    TI_UINT8  ssidLen = 4;
    TI_UINT8  reqIELength = 0;
    TI_UINT8 *pFrame = pBeaconReqActionFrame;
    TI_UINT8 *pReqestLen = NULL;


    measurementMgr_t * pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;


    WLAN_OS_REPORT(("\n measurement_buildBeaconRequestDbg. mode=%d. regClass=%d. channel=%d \n\n",
                    mode,regClass, channel));
       

    *pFrame++ = 5;      /* category */
    *pFrame++ = 0;      /* action */
    *pFrame++ = 0x99;   /* dialog frame token */
    *pFrame++ = 0;
    *pFrame++ = 0;      /* number of repetitiions */
  

    *pFrame++ = 38;    /* IE ID */
    pReqestLen = pFrame; /* save the Measurement Request IE length position to be filled later */
     
    pFrame++;
     
    *pFrame++ = 0x88;   /* measurement Token */
    *pFrame++ = 0;     /* ReqMode */
    *pFrame++ = 5;     /* ReqType */

    reqIELength += 3;

    *pFrame++ = regClass;  /* regulatory class */
    *pFrame++ = channel;   /* channelNumber */

    reqIELength += 2;
    
    /* randomInterval */
    *pFrame++ = 0x44; 
    *pFrame++ = 0x44;
    reqIELength += 2;
    
     /* duration */ 
    *pFrame++ = 0x33;      
    *pFrame++ = 0x00;
    reqIELength += 2;

    /* mode */
    *pFrame++ = mode; /* 0-passive,  1-active scan , 2-table */
    reqIELength++;

    /* bssid */
    *pFrame++ = 0xFF; 
    *pFrame++ = 0xFF;
    *pFrame++ = 0xFF;
    *pFrame++ = 0xFF;
    *pFrame++ = 0xFF;   
    *pFrame++ = 0xFF;

    reqIELength += 6;

    if (ssidLen > 0) 

    {
        /* Sub-elements section ID(1 byte)-LENGTH(1-byte)-DATA(variable)*/
        *pFrame++ = 0; /* SSID sub elemenet */
        *pFrame++ = ssidLen;
        os_memoryCopy(pMeasurementMgr->hOs, pFrame, ssidBuff, ssidLen);
        pFrame += ssidLen;
    
        reqIELength += ssidLen + 2;
    }
  

    
    *pFrame++ = 2; /* Reporting detail element ID */
    *pFrame++ = 1; /* Length */
    *pFrame++ = 1; /* 0 = fixed values only, 1 = All fixed values & requested IEs, 2= All fixed values & IEs */
    reqIELength += 3;

    
    *pFrame++ = 10;     /* Request sub element */
    *pFrame++ = 3;      /* Length */
    /* IEs ID to report  */
    *pFrame++ = 0;      /* ssid IE*/ 
    *pFrame++ = 48;     /* rsn IE */
    *pFrame++ = 221;     /* vendor specific */
    reqIELength += 5;

    
#if 0    
    *pFrame++ = 51;     /* AP channel report sub element */
    *pFrame++ = 3;      /* Length */
    *pFrame++ = 12;     /* regulatory class */
    *pFrame++ = 1;     /* channel 1 */
    *pFrame++ = 9;     /* channel 6 */
    reqIELength += 5;


    *pFrame++ = 51;     /* AP channel report sub element */
    *pFrame++ = 3;      /* Length */
    *pFrame++ = 12;     /* regulatory class */
    *pFrame++ = 4;     /* channel 36 */
    *pFrame++ = 10;     /* channel 60 */
    reqIELength += 5;
#endif    
    
    *pReqestLen = reqIELength;
    *outLen = reqIELength + 5 + 2;
    
   
        
    WLAN_OS_REPORT(("\nmeasurement_buildBeaconRequestDbg: total frame Length = %d ,beacon request IE len=%d  *****\n",
                    *outLen, reqIELength));

    WLAN_OS_REPORT(("\n measurement_buildBeaconRequestDbg: print hex dump: \n"));
    report_PrintDump(pBeaconReqActionFrame, *outLen);

    
}



static void measurement_buildTSMRequestDbg(TI_HANDLE    hMeasurementMgr, 
                                           TI_BOOL      isTriggeredReport,
                                           TI_UINT16    duration,
                                           TI_UINT8     tid,
                                           TI_UINT32    *outLen)
{

    TI_UINT8  reqIELength = 0;
    TI_UINT8 *pFrame = actionFrameBuffer;
    TI_UINT8 *pReqestLen = NULL;
    TI_UINT8  macAddress[6];


    macAddress[0] = 0x11;
    macAddress[1] = 0x22;
    macAddress[2] = 0x33;
    macAddress[3] = 0x44;
    macAddress[4] = 0x55;
    macAddress[5] = 0x66;
    

    WLAN_OS_REPORT(("\nmeasurement_buildTSMRequestDbg: isTriggeredReport=%d. duration=%d. tid=%d \n", 
                    isTriggeredReport ,duration, tid));


    *pFrame++ = 5;  /* category */
    *pFrame++ = 0; /* action */
    *pFrame++ = 0x99; /* dialog frame token */
    *pFrame++ = 0;
    *pFrame++ = 0;/* number of repetitiions */


    *pFrame++ = 38;    /* IE ID */
    pReqestLen = pFrame; /* save the Measurement Request IE length position to be filled later */

    pFrame++;

    *pFrame++ = 0x77;   /* measurement Token */
    *pFrame++ = 0;     /* ReqMode */
    *pFrame++ = 9;     /* ReqType */

    reqIELength = 3;

    
    /* The request itself starts from here */

    /* randomInterval */
    *pFrame++ = 0x44; 
    *pFrame++ = 0x44;
    reqIELength += 2;
    
    /* duration */ 
    COPY_WLAN_WORD(pFrame, &duration);
    pFrame += 2;
    reqIELength += 2;

    /* peer sta address */
    MAC_COPY(pFrame, macAddress);
    pFrame += 6;
    reqIELength += 6;
    

    /* traffic identifier bit4-7 */
    *pFrame++ = tid << 4; 
    reqIELength++;


    /* bin0 Range field */
    *pFrame++ = 5; 
    reqIELength++;
    
  
    if (isTriggeredReport == TI_TRUE) 
    {

         *pFrame++ = 0x01;  /* reoprting deatil info element ID */
         *pFrame++ = 0x06;  /* reoprting deatil info element len */
         reqIELength += 2;
        
         *pFrame++ = 0x04;  /* trigger condition */
         *pFrame++ = 0;     /* average error threshold  */
         *pFrame++ = 0;     /* consecutive error threshold */
         *pFrame++ = 13;    /* delay threshold (count =3, bin range = 1) */
         *pFrame++ = 100;   /* measurement count */
         *pFrame++ = 0x05;  /* timeout */

         pFrame += 6;
         reqIELength += 6;
    }


    *pReqestLen = reqIELength;
    *outLen = reqIELength + 5 + 2;


    WLAN_OS_REPORT(("\n measurement_buildTSMRequestDbg: total frame Length = %d ,beacon request IE len=%d  *****\n",
                    *outLen, reqIELength));

}

void measurement_channelLoadCallBackDbg(TI_HANDLE hMeasurementMgr, TI_STATUS status, 
                                        TI_UINT8* CB_buf)
{
#ifdef REPORT_LOG
    TMediumOccupancy *pMediumOccupancy = (TMediumOccupancy*)(CB_buf+4);
    
    WLAN_OS_REPORT(("MediumUsage = %d\nPeriod = %d\n", 
					pMediumOccupancy->MediumUsage/1000, pMediumOccupancy->Period/1000));            
#endif
}


void measurement_noiseHistCallBackDbg(TI_HANDLE hMeasurementSRV, TI_STATUS status, 
                                      TI_UINT8* CB_buf)
{
    TI_UINT8		            index;
    TNoiseHistogramResults *pNoiseHistogramResults = (TNoiseHistogramResults*)CB_buf;

    if(status == TI_OK)
    {
		report_PrintDump ((TI_UINT8 *)pNoiseHistogramResults, sizeof(TNoiseHistogramResults));
 
        WLAN_OS_REPORT(("Noise Histogram Measurement Results:\nNum of Lost Cycles = %u\nNum Of Tx Hw Gen Lost Cycles = %u\n Num Of Rx Lost Cycles = %u\n Num Of 'Exceed Last Threshold' Lost Cycles = %u\n", 
                         pNoiseHistogramResults->numOfLostCycles, 
                         pNoiseHistogramResults->numOfTxHwGenLostCycles,
                         pNoiseHistogramResults->numOfRxLostCycles,
                         pNoiseHistogramResults->numOfLostCycles - (pNoiseHistogramResults->numOfRxLostCycles)));
        
        for(index = 0;index < NUM_OF_NOISE_HISTOGRAM_COUNTERS; index++)
            WLAN_OS_REPORT(("Counter # %u = %u\n", index,
            pNoiseHistogramResults->counters[index]));
    }
    else
    {
        WLAN_OS_REPORT(("Measurement Debug Functions - Interogate Noise Hist FAILED"));
    }
}
             
           
     
      
     
       
        
 
     
     
                   
         
        
        
           
         
             
      
									   
    


void printMeasurementDbgFunctions(void)
{
	WLAN_OS_REPORT(("   Measurement Debug Functions   \n"));
	WLAN_OS_REPORT(("-----------------------------\n"));
	
	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_PRINT_HELP\n", DBG_MEASUREMENT_PRINT_HELP));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_PRINT_STATUS\n", DBG_MEASUREMENT_PRINT_STATUS));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_CHANNEL_LOAD_START\n", DBG_MEASUREMENT_CHANNEL_LOAD_START));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_CHANNEL_LOAD_STOP\n", DBG_MEASUREMENT_CHANNEL_LOAD_STOP));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SEND_FRAME_REQUEST\n", DBG_MEASUREMENT_SEND_FRAME_REQUEST));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_START_NOISE_HIST\n", DBG_MEASUREMENT_START_NOISE_HIST));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_STOP_NOISE_HIST\n", DBG_MEASUREMENT_STOP_NOISE_HIST));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_GET_NOISE_HIST_RESULTS\n", DBG_MEASUREMENT_GET_NOISE_HIST_RESULTS));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SEND_CHANNEL_LOAD_FRAME\n", DBG_MEASUREMENT_SEND_CHANNEL_LOAD_FRAME));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SEND_BEACON_TABLE_FRAME\n", DBG_MEASUREMENT_SEND_BEACON_TABLE_FRAME));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SEND_NOISE_HIST_1_FRAME\n", DBG_MEASUREMENT_SEND_NOISE_HIST_1_FRAME));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SEND_NOISE_HIST_2_FRAME\n", DBG_MEASUREMENT_SEND_NOISE_HIST_2_FRAME));

	WLAN_OS_REPORT(("%d - DBG_MEASUREMENT_SET_TRAFFIC_THRSLD\n", DBG_MEASUREMENT_SET_TRAFFIC_THRSLD));

	WLAN_OS_REPORT(("%d - DBG_SC_PRINT_STATUS\n", DBG_SC_PRINT_STATUS));

	WLAN_OS_REPORT(("%d - DBG_SC_SET_SWITCH_CHANNEL_NUM\n", DBG_SC_SET_SWITCH_CHANNEL_NUM));

	WLAN_OS_REPORT(("%d - DBG_SC_SET_SWITCH_CHANNEL_TBTT\n", DBG_SC_SET_SWITCH_CHANNEL_TBTT));

	WLAN_OS_REPORT(("%d - DBG_SC_SET_SWITCH_CHANNEL_MODE\n", DBG_SC_SET_SWITCH_CHANNEL_MODE));

	WLAN_OS_REPORT(("%d - DBG_SC_SET_CHANNEL_AS_VALID\n", DBG_SC_SET_CHANNEL_AS_VALID));

	WLAN_OS_REPORT(("%d - DBG_SC_SET_CHANNEL_AS_INVALID\n", DBG_SC_SET_CHANNEL_AS_INVALID));

	WLAN_OS_REPORT(("%d - DBG_SC_SWITCH_CHANNEL_CMD\n", DBG_SC_SWITCH_CHANNEL_CMD));

	WLAN_OS_REPORT(("%d - DBG_SC_CANCEL_SWITCH_CHANNEL_CMD\n", DBG_SC_CANCEL_SWITCH_CHANNEL_CMD));

	WLAN_OS_REPORT(("%d - DBG_REG_DOMAIN_PRINT_VALID_CHANNELS\n", DBG_REG_DOMAIN_PRINT_VALID_CHANNELS));


}

