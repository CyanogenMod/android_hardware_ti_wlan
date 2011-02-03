/*
 * rrmMgr.c
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

/** \file rrmMgr.c
 *  
 *
 *  \see rrmMgr.h
 */

/****************************************************************************************************/
/*																									*/
/*		MODULE:		rrmMgr.c																        */
/*		PURPOSE:	                                            									*/
/*																						 			*/
/****************************************************************************************************/

#define __FILE_ID__  FILE_ID_139
#include "report.h"
#include "osApi.h"
#include "siteMgrApi.h"
#include "regulatoryDomainApi.h"
#include "mlmeBuilder.h"
#include "Ctrl.h"
#include "rrmMgr.h"
#include "bssTypes.h"
#include "apConn.h"
#include "sme.h"
#include "measurementMgrApi.h"


#define RRM_REQUEST_IE_HDR_LEN           (5)
#define DOT11H_REQUEST_IE_LEN            (7)
#define DOT11_MEASUREMENT_REQUEST_ELE_ID (38)
#define DOT11_MEASUREMENT_REPORT_ELE_ID  (39)
#define IE_HDR_LEN                       (2)


#define RRM_TSM_REPORT_IE_FIXED_LENGTH   (74)

#define REPORT_REJECT_REASON_LATE        (BIT_0)
#define REPORT_REJECT_REASON_INCAPABLE   (BIT_1)
#define REPORT_REJECT_REASON_REFUSED     (BIT_2)


#define MEAS_REQ_MODE_ENABLE_MASK               (0x01)
#define MEAS_REQ_MODE_PARALLEL_MASK             (0x02)
#define MEAS_REQ_MODE_DURATION_MANDATORY_MASK   (0x10)

#define MEASUREMENT_TYPE_FIXED_OFFSET           (9)

/********************************************************************************/
/*						Internal functions prototypes.							*/
/********************************************************************************/

static TI_BOOL iSChannelIncludedInTheRequest(measurementMgr_t    *pMeasurementMgr,
                                             MeasurementRequest_t *pRequest, 
                                             ERadioBand band, 
                                             TI_UINT8 channel);

static TI_STATUS rrmMgr_buildBeaconReport(TI_HANDLE hMeasurementMgr, 
									      MeasurementRequest_t request,
										  TMeasurementTypeReply * reply);




/* This structure holds all the data related to single AP found */
typedef struct 
{
    TI_UINT8                regulatoryClass;
    TI_UINT8                channel;
    TI_UINT64               actualMeasuStartTimeTSF;
    TI_UINT16               duration;
    TI_UINT8                reportedInfo;
    TI_UINT8                RCPI;
    TI_UINT8                RSNI;
    TMacAddr                bssid;
    TI_UINT8                antennaID;
    TI_UINT32               parentTSF; /* The lower 4 bytes of the beacon/probeResp TSF */
    TI_UINT8                subElements[BEACON_REPORT_MAX_SUB_ELEMENTS_LEN];
} beaconReportBody_t;


typedef struct 
{
    rrmReqReportIEHdr_t     hdr;
    beaconReportBody_t      body;

} beaconReport_t;



/* RRM Beacon Report structure (802.11k) */
typedef struct
{
    RadioMeasurementHdr_t    hdrFrame;
    rrmReqReportIEHdr_t      hdrMeasReport;
    TI_UINT8                 reportsBody[BEACON_REPORTS_MAX_LEN];
}  BeaconReportFrame_t;

typedef struct 
{
    TI_UINT8       triggerCondition;
    TI_UINT8       averageErrorThreshold;
    TI_UINT8       consecutiveErrorThreshold;
    TI_UINT8       delayThreshold;
    TI_UINT8       measureCount;
    TI_UINT8       triggerTimeout;
} TSMTriggerReportField_t;


typedef struct 
{
    rrmReqReportIEHdr_t         hdr;
    TI_UINT16                   randomInterval;
    TI_UINT16                   duration;
    TMacAddr                    peerStaAddress;
    TI_UINT8                    trafficID;        /*  bits: B4-B7 */
    TI_UINT8                    bin0Range;
    TSMTriggerReportField_t     triggerReportField;
} TSMReq_t;





TI_STATUS rrmMgr_receiveFrameRequest(TI_HANDLE hMeasurementMgr, EMeasurementFrameType frameType,
                                     TMacAddr  *pBssidSender, TI_INT32 dataLen, TI_UINT8 * pData)
{
    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
    paramInfo_t         param;
    EMeasurementType    measType = pData[MEASUREMENT_TYPE_FIXED_OFFSET];

    TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, "rrmMgr_receiveFrameRequest: EMeasurementType = %d \n", measType);

    /* If the measurement manager is in RRM mode */
    if (pMeasurementMgr->Mode == MSR_MODE_RRM) 
    {
        param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
        siteMgr_getParam(pMeasurementMgr->hSiteMgr,&param);
        
        /* if the frame receievd does not belong to the associated AP - ignore it, currently only 
        requests from the STA's associated AP are supported */
        if (MAC_EQUAL (param.content.siteMgrDesiredBSSID, *pBssidSender))
        {          
            if (MSR_TYPE_RRM_BEACON_MEASUREMENT == measType) 
            {
                return measurementMgr_receiveFrameRequest(hMeasurementMgr, frameType, dataLen, pData);
            }
            else if (MSR_TYPE_RRM_TS_TC_MEASUREMENT == measType)
            {
                return rrmMgr_parseTSMRequest(hMeasurementMgr, frameType, pData, dataLen);
            }
            else
            {
                TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "rrmMgr_receiveFrameRequest: "
                                                                        "Unrecognized measurement type = %d \n", measType);

            }
        }
        else
        {
            TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, "rrmMgr_receiveFrameRequest: "
                                                                    "Measurement Request received from non- associated AP. ignore it. \n");

        }
    }
    else
    {
        TRACE1(pMeasurementMgr->hReport,
               REPORT_SEVERITY_WARNING, 
               "rrmMgr_receiveFrameRequest: Measurement manager is not in RRM mode (current mode=%d) so ignore this request . \n",
               pMeasurementMgr->Mode);
    }

    return TI_NOK;
}



/***********************************************************************
 * NOTE: The next 4 functions (dot11h...) should be corrected according 
 *       to the 802.11k standard.
 ***********************************************************************/

/************************************************************************
*					rrmMgr_dot11hParseFrameReq					        *
************************************************************************
DESCRIPTION: Frame Request Parser function, called by the Measurement 
             object when a measurement request frame is received. 
				performs the following:
				-	Parsers the received frame request.
					
INPUT:      hMeasurementMgr - MeasurementMgr Handle
			pData			- The frame request
            dataLen         - The frame'sa length

OUTPUT:		fraemReq        - The Parsered Frame Request

RETURN:     TI_OK on success, TI_NOK otherwise

************************************************************************/
TI_STATUS rrmMgr_ParseFrameReq(TI_HANDLE hMeasurementMgr, TI_UINT8 *pData,
                               TI_INT32 dataLen, TMeasurementFrameRequest *frameReq)
{   
    TI_UINT8    hdrLen = 0;

    
    /* Skip the default values for the 802.11k RRM */
    hdrLen += 2;
    pData += 2;
    
    frameReq->dialogToken = (TI_UINT16)*pData++;
    hdrLen++;


    /* LiorC: currently number of repetitions field is expected to be 0 - meaning do the measurement once. 
       TBD: support numberOfRepetitions > 0 */
    COPY_WLAN_WORD(&frameReq->numberOfRepetitions, pData);
    pData += 2;
    hdrLen += 2;

    frameReq->requests = pData;
    frameReq->requestsLen = dataLen - hdrLen;
    
    return TI_OK;
}


/********************************************************************************/
/*						Internal functions Implementation.						*/
/********************************************************************************/

TI_STATUS rrmMgr_ParseRequestElement(TI_HANDLE hMeasurementMgr, TI_UINT8 *pData, 
                                     TI_UINT16 *measurementToken, TI_UINT8* singleReqLen, 
                                     MeasurementRequest_t *pCurrRequest)
{
    TI_UINT8		    measurementMode;
	TI_UINT8		    parallelBit;
	TI_UINT8		    enableBit;
    paramInfo_t         param;
    TI_UINT8            i=0;
    TI_UINT8            j=0;
    TI_UINT32           reqLenTmp = 0;
    TI_UINT32           recvReqLen = 0;
    TI_UINT8            currSubEleLen = 0;
    TI_UINT8            currSubEleID = 0;
    TI_UINT8           *pBeaconMeasReq = pData;
    
    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;

     
    /* checking if received the correct information element ID */
    if (*pBeaconMeasReq != DOT11_MEASUREMENT_REQUEST_ELE_ID)
    {
        TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "rrmMgr_ParseRequestElement: Error, unrecognized IE ID = %d ****\n", *pBeaconMeasReq);
        return TI_NOK;
    }

    pBeaconMeasReq++;

    recvReqLen = *pBeaconMeasReq++;
    *measurementToken = (TI_UINT16)*pBeaconMeasReq++;


    /*** Getting the Measurement Mode ***/
	measurementMode = *pBeaconMeasReq++;

  
	/* getting parallel bit */
	parallelBit = measurementMode & MEAS_REQ_MODE_ENABLE_MASK;
	
    /* getting Enable bit */
	enableBit = (measurementMode & MEAS_REQ_MODE_PARALLEL_MASK) >> 1;

    pCurrRequest->bDurationMandatory = (TI_BOOL)((measurementMode & MEAS_REQ_MODE_DURATION_MANDATORY_MASK) >> 4);
    
#if 0  	
    /* checking enable bit, the current implementation does not support 
		enable bit which set to one, so there is no need to check request/report bits	*/
	if(enableBit == 1)
		return TI_OK;


    pCurrRequest->bIsParallel = parallelBit;
#endif


    /* LiorC: parallel bit meas for rrm beacon req is not expected to be set so ignore it. */
    pCurrRequest->bIsParallel = TI_FALSE; 
    
    /* Getting the Measurement Type */
   	pCurrRequest->Type = (EMeasurementType)*pBeaconMeasReq++;

    TRACE2(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, "rrmMgr_ParseRequestElement: Is Parallel=%d .Request Type is %d. (5=beacon req. 9=TSM req) \n",
           pCurrRequest->bIsParallel, pCurrRequest->Type);

    
    /*  Start Parsing the Measurement Request Field - Type TSM */    

    if (MSR_TYPE_RRM_TS_TC_MEASUREMENT == pCurrRequest->Type) 
    {
        *singleReqLen = 2 + recvReqLen;  /* 2 = IE's ID + Length bytes */

        COPY_WLAN_WORD(&pCurrRequest->randomInterval, pBeaconMeasReq);
        pBeaconMeasReq += 2;

        COPY_WLAN_WORD(&pCurrRequest->DurationTime, pBeaconMeasReq);
        pBeaconMeasReq += 2;

        /*  The PEER STA Address (Associated AP BSSID ) */
        MAC_COPY(pCurrRequest->bssid, pBeaconMeasReq);
        pBeaconMeasReq += 6;


        pCurrRequest->tid = *pBeaconMeasReq++;
        pCurrRequest->bin0Range = *pBeaconMeasReq++;
        MAC_COPY(pCurrRequest->triggeredReporting, pBeaconMeasReq);
    
        return TI_OK;
    }


    /* Start Parsing the Measurement Request Field -Type Beacon */   
    
    pCurrRequest->regulatoryClass = *pBeaconMeasReq++;
    pCurrRequest->channelSingle = *pBeaconMeasReq++;
    
    /********************************************************************************/
    /* Handle the channels Issue according to the Regulatory Class + Channel fields */
    /********************************************************************************/


    pCurrRequest->uActualNumOfChannelsBandBG =0;
    pCurrRequest->uActualNumOfChannelsBandA = 0;


    TRACE2(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION,"rrmMgr_ParseRequestElement: channel=%d, regClass=%d\n",
           pCurrRequest->channelSingle,
           pCurrRequest->regulatoryClass);
    
    param.paramType = REGULATORY_DOMAIN_GET_CHANNELS_BY_REG_CLASS_PARAM;
    param.content.regClass = pCurrRequest->regulatoryClass;
    regulatoryDomain_getParam(pMeasurementMgr->hRegulatoryDomain, &param);
        
    
    if (0 == pCurrRequest->channelSingle) 
    {
        /* get all the channels supported in this regulatory class */

        TRACE0(pMeasurementMgr->hReport,
               REPORT_SEVERITY_ERROR,
               "rrmMgr_ParseRequestElement: channel "
               "is equal to 0, so get all the channels supported in this regulatory class \n");
       
        
        if (BAND_TYPE_2_4GHZ == param.content.regClassChannelList.band) 
        {
             for (i =0 ; i< REG_DOMAIN_MAX_CHAN_NUM ; i++) 
             {
                 if (param.content.regClassChannelList.Channel[i] > 0 ) 
                 {
                     pCurrRequest->channelListBandBG[pCurrRequest->uActualNumOfChannelsBandBG++] = param.content.regClassChannelList.Channel[i];
                 }   
             }             
        }
        else if (BAND_TYPE_5_GHZ == param.content.regClassChannelList.band) 
        {
            for (i =0 ; i< REG_DOMAIN_MAX_CHAN_NUM ; i++) 
            {

                 if (param.content.regClassChannelList.Channel[i] > 0 ) 
                 {
                     pCurrRequest->channelListBandA[pCurrRequest->uActualNumOfChannelsBandA++] = param.content.regClassChannelList.Channel[i];
                 } 
            }
        }
        else
        {
            TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, "rrmMgr_ParseRequestElement: The specified band is not supported\n");
        }
    }
    else if (255 == pCurrRequest->channelSingle) 
    {/* set channels according to the AP Channel report element(s) in the request - Will be handled later  */
        TRACE0(pMeasurementMgr->hReport, 
               REPORT_SEVERITY_INFORMATION,
               "rrmMgr_ParseRequestElement: channelNumber= 255. AP Channel report sub elements are expected to be found later\n");
        
    }
    else  /* set only one channel  */
    {
        if (BAND_TYPE_2_4GHZ == param.content.regClassChannelList.band) 
        {
            pCurrRequest->channelListBandBG[0] = pCurrRequest->channelSingle;
            pCurrRequest->uActualNumOfChannelsBandBG = 1;
        }
        else if (BAND_TYPE_5_GHZ == param.content.regClassChannelList.band) 
        {
            pCurrRequest->channelListBandA[0] = pCurrRequest->channelSingle;
            pCurrRequest->uActualNumOfChannelsBandA  = 1;
        }
        else
        {
            TRACE0(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_ERROR,
                   "rrmMgr_ParseRequestElement: Error, The specified band is not supported!\n");
        }
       
    }

    COPY_WLAN_WORD(&pCurrRequest->randomInterval, pBeaconMeasReq);
    pBeaconMeasReq += 2;
	

    COPY_WLAN_WORD(&pCurrRequest->DurationTime, pBeaconMeasReq);
    pBeaconMeasReq += 2;
    
	pCurrRequest->ScanMode = (EMeasurementScanMode)(*pBeaconMeasReq++);
    
    /*  The desired BSSID to scan for */
    MAC_COPY(pCurrRequest->bssid, pBeaconMeasReq);
    pBeaconMeasReq += 6;
    
    reqLenTmp = BEACON_REQUEST_FIXED_FIELDS_LEN + 1;


    report_PrintDump(pData, recvReqLen + 2);
    
    /*  Start parsing the sub-elements fields */
    if (reqLenTmp != recvReqLen) 
    {
        TRACE0(pMeasurementMgr->hReport, 
               REPORT_SEVERITY_INFORMATION,
               "rrmMgr_ParseRequestElement: Sub-Element detected, keep parsing the beacon request!\n");
            
        pData += reqLenTmp; /* the sub-elements start position  */
      
        while (reqLenTmp < recvReqLen) 
        {
            currSubEleID = pData[0];
            currSubEleLen = pData[1];

            TRACE4(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_INFORMATION,
                   "rrmMgr_ParseRequestElement: reqLenTmp=%d, recvReqLen=%d .currSubEleID =%d, currSubEleLen=%d!\n",
                   reqLenTmp, recvReqLen, currSubEleID, currSubEleLen);
            
            switch (*pData++)
            {
            case BEACON_REQUEST_SUB_ELE_SSID:
                 pCurrRequest->tSSID.len = currSubEleLen;
                 os_memoryCopy(pMeasurementMgr->hOs, (void*)pCurrRequest->tSSID.str, (void*)(pData+1), pCurrRequest->tSSID.len); 
                 break;
            case BEACON_REQUEST_SUB_ELE_REPORTING_INFO:
                TRACE0(pMeasurementMgr->hReport,
                       REPORT_SEVERITY_ERROR,
                       "rrmMgr_ParseRequestElement: SUB_ELE_REPORTING_INFO is not supported for beacon request!\n");
                break;
            case BEACON_REQUEST_SUB_ELE_REPORTING_DETAIL:
                pCurrRequest->reportingDetailLevel = pData[1];  
                break;
            case BEACON_REQUEST_SUB_ELE_REQUEST_INFO:

                pCurrRequest->uActualNumOfIEs = 0;

                /* iterate and save the IEs to be reported in th beacon report */
                for (i=0; i< currSubEleLen ; i++) 
                {
                    pCurrRequest->IEsIdNeededForDeatailedReport[i] = pData[i+1];
                    pCurrRequest->uActualNumOfIEs++;
                    TRACE1(pMeasurementMgr->hReport,
                           REPORT_SEVERITY_INFORMATION,
                           "rrmMgr_ParseRequestElement: IE ID=%d is requested to be reported!\n",
                           pData[i+1]);
                }
                break;
            case BEACON_REQUEST_SUB_ELE_AP_CHANNEL_REPORT:
                {
                    TI_UINT8 numOfChannels = currSubEleLen - 1; /*  list of channel length - one regClass field */

                    if (numOfChannels > SCAN_MAX_NUM_OF_CHANNELS)
                    {
                        TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR,
                               "rrmMgr_ParseRequestElement: num of channels=%d > SCAN_MAX_NUM_OF_CHANNELS\n",
                               numOfChannels);
                        return TI_NOK;
                    }
                    TRACE2(pMeasurementMgr->hReport,
                           REPORT_SEVERITY_INFORMATION,
                           "rrmMgr_ParseRequestElement: SUB_ELE_AP_CHANNEL_REPORT detected. regClass=%d , num of channels=%d, \n",
                           pData[1], numOfChannels);   
                    
                    /* get all the channels supported in this regulatory class */
                    param.paramType = REGULATORY_DOMAIN_GET_CHANNELS_BY_REG_CLASS_PARAM;
                    param.content.regClass = pData[1];
                    regulatoryDomain_getParam(pMeasurementMgr->hRegulatoryDomain, &param);

                    if (BAND_TYPE_2_4GHZ == param.content.regClassChannelList.band) 
                    {
                        for (i=0; i< numOfChannels ; i++)
                        {
				/* if pCurrRequest->uActualNumOfChannelsBandBG overflowed, break (won't be able to add any channel) */
				if (pCurrRequest->uActualNumOfChannelsBandBG >= SCAN_MAX_NUM_OF_CHANNELS) {
					TRACE2(pMeasurementMgr->hReport,
									   REPORT_SEVERITY_WARNING,
									   "rrmMgr_ParseRequestElement: pCurrRequest->uActualNumOfChannelsBandBG (%d) >= SCAN_MAX_NUM_OF_CHANNELS (%d). breaking\n",
									   pCurrRequest->uActualNumOfChannelsBandBG,
									   SCAN_MAX_NUM_OF_CHANNELS);
					break;
				}

                            for (j=0 ; j< REG_DOMAIN_MAX_CHAN_NUM ;j++) 
                            {/* Add only channels that supported by the STA for the specified regulatory class */
                                if (param.content.regClassChannelList.Channel[j] == pData[i+2]) 
                                {
                                    TRACE1(pMeasurementMgr->hReport,
                                           REPORT_SEVERITY_INFORMATION,
                                           "rrmMgr_ParseRequestElement: Adding channel %d to Band B/G list\n",
                                           pData[i+2]);
                                
                                    pCurrRequest->channelListBandBG[pCurrRequest->uActualNumOfChannelsBandBG++] = pData[i+2];
                                    break;
                                }
                            }
                        }
                    }
                    else if (BAND_TYPE_5_GHZ == param.content.regClassChannelList.band) 
                    {
                        for (i=0; i< numOfChannels ; i++) 
                        {
                            for (j=0 ; j< REG_DOMAIN_MAX_CHAN_NUM ;j++) 
                            {/* Add only channels that supported by the STA for the specified regulatory class */
                                if (param.content.regClassChannelList.Channel[j] == pData[i+2]) 
                                {
                                     TRACE1(pMeasurementMgr->hReport,
                                           REPORT_SEVERITY_INFORMATION,
                                           "rrmMgr_ParseRequestElement: Adding channel %d to Band A list",
                                           pData[i+2]);
                               
                                    pCurrRequest->channelListBandA[pCurrRequest->uActualNumOfChannelsBandA++] = pData[i+2];
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, "rrmMgr_ParseRequestElement:  Unknown Band found - ignore this channel\n");
                    }              
                }

                 break;
            default:
                TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, "rrmMgr_ParseRequestElement:  Sub-Elements  (%d) is not supported\n",
                       currSubEleID);
     
            }/* End of switch */


            pData += currSubEleLen + 1; /* point to the next sub-element position*/
            reqLenTmp += currSubEleLen + 2; /* Increase the req length we have alreay handled. In order to know when we finish
                                           with the sub-elements parsing*/
           
        }/* End of while */
    }/*  End of If statement */


    
    /* set the requested band 2.4/5/dual according to the channels/regClass found */
    if ((pCurrRequest->uActualNumOfChannelsBandBG > 0) && (pCurrRequest->uActualNumOfChannelsBandA == 0)) 
    {
        pCurrRequest->band = RADIO_BAND_2_4_GHZ;
    }
    else if ((pCurrRequest->uActualNumOfChannelsBandA > 0) && (pCurrRequest->uActualNumOfChannelsBandBG == 0)) 
    {
        pCurrRequest->band = RADIO_BAND_5_0_GHZ;
    }
    else
    {
        pCurrRequest->band = RADIO_BAND_DUAL;
    }

    
    *singleReqLen = 2 + recvReqLen;  /* 2 = IE's ID + Length bytes */

    TRACE1(pMeasurementMgr->hReport, 
           REPORT_SEVERITY_INFORMATION, 
           "rrmMgr_ParseRequestElement: Finish Parsing the body request. singleReqLen=%d\n",
           *singleReqLen);
    
    return TI_OK;
}




/************************************************************************
 *					rrmMgr_IsTypeValid         				*
 ************************************************************************
DESCRIPTION: RRM function that checks if the given 
             measurement type is valid. 
					
INPUT:      hMeasurementMgr -	MeasurementMgr Handle
            type			-	The measurement type.
            scanMode        -   The Measurement scan Mode.

OUTPUT:		

RETURN:     TI_TRUE if type is valid, TI_FALSE otherwise

************************************************************************/
TI_BOOL rrmMgr_IsTypeValid(TI_HANDLE hMeasurementMgr, 
                                         EMeasurementType type, 
                                         EMeasurementScanMode scanMode)
{
    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t *)hMeasurementMgr;
    
    if (type != MSR_TYPE_RRM_BEACON_MEASUREMENT)
    {
        TRACE1(pMeasurementMgr->hReport, 
               REPORT_SEVERITY_ERROR, 
               "rrmMgr_IsTypeValid: Measurement type =%d is not valid!! " , type);
        return TI_FALSE;   
    }

    return TI_TRUE;
}

/***********************************************************************
 *                  rrmMgr_BuildRejectReport							
 ***********************************************************************
DESCRIPTION:	Send reject measurement report frame Function.
                The function does the following:
				-	checking the reason for reject.
				-	builds measurement report Frame.
				-	Calls the mlmeBuolder to allocate a mangement frame
					and to transmit it.
				
INPUT:      hMeasurementMgr -	MeasurementMgr Handle
			pRequestArr		-	Array of pointer to all measurement 
								request that should be rejected.
			numOfRequestsInParallel - indicates the number of 
								request that should be rejected.
			rejectReason	-	Indicated the rejection reason.

OUTPUT:		None

RETURN:     TI_OK on success, TI_NOK otherwise
************************************************************************/
TI_STATUS rrmMgr_BuildRejectReport(TI_HANDLE hMeasurementMgr,
								   MeasurementRequest_t* pRequestArr[],
								   TI_UINT8	numOfRequestsInParallel,
								   EMeasurementRejectReason	rejectReason)
{

    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t*)hMeasurementMgr;
    TI_UINT8            *pReportFrame = pMeasurementMgr->rrmFrameReportBuff;

    TRACE1(pMeasurementMgr->hReport, 
           REPORT_SEVERITY_INFORMATION, 
           "rrmMgr_BuildRejectReport: building reject report. rejectReason=%d \n",
           rejectReason);

    /* Filling the frame header (3 bytes) */
    *pReportFrame++ = (TI_UINT8)CATEGORY_RRM;
    *pReportFrame++ = (TI_UINT8)RRM_MEASUREMENT_REPORT;
    *pReportFrame++ = (TI_UINT8)pRequestArr[0]->frameToken;
   
    pMeasurementMgr->frameLength = 3; /* this field saves the total action frame length to be sent later */

    /* Filling the measurement report element header (4 bytes) */
    *pReportFrame++ = (TI_UINT8)DOT11_MEASUREMENT_REPORT_ELE_ID;
    *pReportFrame++ = 3; /* measurment report length is equal to 3 in reject report */
    *pReportFrame++ = (TI_UINT8)pRequestArr[0]->measurementToken;

    switch(rejectReason)
    {
        case MSR_REJECT_DTIM_OVERLAP:
        case MSR_REJECT_DURATION_EXCEED_MAX_DURATION:
        case MSR_REJECT_TRAFFIC_INTENSITY_TOO_HIGH:
        case MSR_REJECT_SCR_UNAVAILABLE:
        case MSR_REJECT_INVALID_CHANNEL:
        case MSR_REJECT_EMPTY_REPORT:
        case MSR_REJECT_MAX_DELAY_PASSED:
        {
            /* Setting the Refused bit */
            *pReportFrame++ = REPORT_REJECT_REASON_REFUSED;
            break;
        }
        case MSR_REJECT_INVALID_MEASUREMENT_TYPE:
        {
             /* Setting the Incapable bit */
            *pReportFrame++ = REPORT_REJECT_REASON_INCAPABLE;
            break;
        }
        default:
        {
            /* Setting the Refused bit */
            *pReportFrame++ = REPORT_REJECT_REASON_INCAPABLE | REPORT_REJECT_REASON_REFUSED;
TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "Reject reason is unspecified, %d\n\n", rejectReason);

            break;
        }
    }
 
    *pReportFrame++ = (TI_UINT8)pRequestArr[0]->Type; /* Measurement Type */
    pMeasurementMgr->frameLength += 5;

    return TI_OK;
}


/***********************************************************************
 *                        rrmMgr_BuildReport							
 ***********************************************************************
DESCRIPTION:	build measurement report Function.
                The function does the following:
				-	builds measurement report IE.
				
INPUT:      hMeasurementMgr -	MeasurementMgr Handle
			pRequestArr		-	Array of pointer to all measurement 
								request that should be reported.
			numOfRequestsInParallel - indicates the number of 
								request that should be reported.
			
OUTPUT:		None

RETURN:     TI_OK on success, TI_NOK otherwise
************************************************************************/
TI_STATUS rrmMgr_BuildReport(TI_HANDLE hMeasurementMgr, MeasurementRequest_t request, TMeasurementTypeReply * reply)
{
    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t *)hMeasurementMgr;
    TI_STATUS           status = TI_OK;
    
   
    /* Getting measurement results */
    switch (request.Type)
    {
        case MSR_TYPE_RRM_BEACON_MEASUREMENT:
        {
           TRACE0(pMeasurementMgr->hReport, 
                  REPORT_SEVERITY_INFORMATION, 
                  "rrmMgr_BuildReport: Building Beacon report... \n");
             
            status = rrmMgr_buildBeaconReport(hMeasurementMgr, request, reply);
            break;
        }
   
    default:
        {
            TRACE0(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_ERROR, 
                   "rrmMgr_BuildReport: Unrecognized report type - do not build the report! \n");

            break;
        }
        
    }


    if (reply != NULL) 
    {
         reply->msrType = request.Type;
         reply->status = status;
    }
   

    return status;
}




static TI_BOOL iSChannelIncludedInTheRequest(measurementMgr_t   *pMeasurementMgr, MeasurementRequest_t *pRequest, ERadioBand band, TI_UINT8 channel)
{
    TI_UINT8    i = 0;
    
    switch (band) 
    {
        case RADIO_BAND_2_4_GHZ:
        {
            for (i=0; i< pRequest->uActualNumOfChannelsBandBG ; i++) 
            {
                if (pRequest->channelListBandBG[i] == channel) 
                {
                    return TI_TRUE;
                }
            }
            break;
        }

        case RADIO_BAND_5_0_GHZ:
        {
            for (i=0; i< pRequest->uActualNumOfChannelsBandA ; i++) 
            {
                if (pRequest->channelListBandA[i] == channel) 
                {
                    return TI_TRUE;
                }
            }
            break;
        }
        
        default:
        {
             TRACE0(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_ERROR, 
                   "rrmMgr_BuildReport: The beacon/probeResp channel has not requested in the request! \n");
        }
    }

    return TI_FALSE;
}



static TI_STATUS rrmMgr_buildBeaconReport(TI_HANDLE hMeasurementMgr, MeasurementRequest_t request,
                                          TMeasurementTypeReply * reply)
{
    TI_UINT32                   addSiteToReport;
    TSiteEntry                  *pCurrentSite;
    TI_HANDLE                   hScanResultTable;
    measurementMgr_t            *pMeasurementMgr = (measurementMgr_t*)hMeasurementMgr;
    TI_UINT8                    *pReportFrame = pMeasurementMgr->rrmFrameReportBuff;
    paramInfo_t                 param;
    TI_UINT8                    i = 0;
    TI_UINT32                   beaconReportsTotalLen = 0;
    TI_UINT32                   subElementsLen = 0;
    TI_UINT8                    ieLen = 0, reportLen = 0;  
    TI_UINT8                    numOfSitesAdded = 0; 
    TI_UINT8                    *pSubEleLen = NULL, *pReportLen = NULL;
    siteEntry_t                 *pSite;
  
    
    if ((request.ScanMode != MSR_SCAN_MODE_BEACON_TABLE) && (reply->status != TI_OK))
	{
	    MeasurementRequest_t * reqArr[1];
        reqArr[0] = &request;

        
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, 
               "rrmMgr_buildBeaconReport: Beacon measurement request failed. sending reject report!\n");
        
        rrmMgr_BuildRejectReport(hMeasurementMgr, reqArr, 1, MSR_REJECT_OTHER_REASON);

		return TI_OK;
	}

    TRACE2(pMeasurementMgr->hReport,
           REPORT_SEVERITY_INFORMATION,
           "rrmMgr_buildBeaconReport: bMeasurementScanExecuted = %d. MODE=%d\n",
           pMeasurementMgr->bMeasurementScanExecuted, request.ScanMode);

    
    /* if measurement scan was executed get the SME scan result table (passive/active measurement), otherwise,
     * get from SME the scan result table used to connect to the AP (beacon table).   */
    if (pMeasurementMgr->bMeasurementScanExecuted)
    {
        sme_MeasureScanReport(pMeasurementMgr->hSme, &hScanResultTable); 
        /* Table holding results received under measurement procedure */
    }
    else
    {
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, 
               "rrmMgr_buildBeaconReport: No measurememnt has been executed. It's A BEACON TABLE!\n");
        sme_ConnectScanReport(pMeasurementMgr->hSme, &hScanResultTable);
    }

  
    /* Filling the frame header (3 bytes) */
    
    *pReportFrame++ = (TI_UINT8)CATEGORY_RRM;
    *pReportFrame++ = (TI_UINT8)RRM_MEASUREMENT_REPORT;
    *pReportFrame++ = (TI_UINT8)request.frameToken;
    
    pMeasurementMgr->frameLength = 3; /* this field saves the total action frame length to be sent later */

    /* Filling the measurement report element header (4 bytes) */
    *pReportFrame++ = (TI_UINT8)DOT11_MEASUREMENT_REPORT_ELE_ID;
    
    pReportLen = pReportFrame; /* will be calculated later for the entire report element */
    reportLen = 3;

    pReportFrame++;
    *pReportFrame++ = (TI_UINT8)request.measurementToken;
   
    *pReportFrame++ =  0; /* (Report mode ) Should be 0 if the report is ok and not a rejected one */
    *pReportFrame++ = (TI_UINT8)MSR_TYPE_RRM_BEACON_MEASUREMENT; /* Measurement Type */
    
    pMeasurementMgr->frameLength += 5;
    
    /* Till here we handled the frame fields corresponding to the entire request. from now on 
       We enter all the APs results to the report in its reported frame body sub-element  */
    
    beaconReportsTotalLen = 0;
    subElementsLen = 0;
    numOfSitesAdded = 0;

    TRACE3(pMeasurementMgr->hReport, 
           REPORT_SEVERITY_INFORMATION, 
           "rrmMgr_buildBeaconReport: regulatoryClass = %d, Scan mode =%d, reportingLevel=%d\n",
           request.regulatoryClass, request.ScanMode, request.reportingDetailLevel);
    
    /* get the first site from the scan result table */
    pCurrentSite = scanResultTable_GetFirst (hScanResultTable);

    
    /* Get the relevant site entries from the current site table */
    /* check all sites */
    while (NULL != pCurrentSite)
    {
        pSite = (siteEntry_t *)pCurrentSite; /* we can do the casting as it is the same structure */
        
        if (pSite->siteType != SITE_NULL)
        {
            /* 
             * If the site is on the requested channel(s) when the measurement operation was in action, or the request is
			 * of Beacon Table Type --> build the report
             */

            TRACE7(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_INFORMATION, 
                   "rrmMgr_buildBeaconReport: checking AP  %02x:%02x:%02x:%02x:%02x:%02x. channel=%d\n",
                   pSite->bssid[ 0 ], 
                   pSite->bssid[ 1 ], 
                   pSite->bssid[ 2 ], 
                   pSite->bssid[ 3 ], 
                   pSite->bssid[ 4 ], 
                   pSite->bssid[ 5 ],
                   pSite->channel);

            param.paramType = SITE_MGR_CURRENT_BSSID_PARAM;
            siteMgr_getParam(pMeasurementMgr->hSiteMgr,&param);

            
            if ((TI_TRUE == iSChannelIncludedInTheRequest( pMeasurementMgr,  &request, pSite->eBand, pSite->channel)) 
                 && (!MAC_EQUAL (param.content.siteMgrDesiredBSSID, pSite->bssid)) 
                 && (!os_memoryCompare(pMeasurementMgr->hOs, (TI_UINT8*)pSite->ssid.str, (TI_UINT8*)request.tSSID.str, request.tSSID.len)))
            {
                TI_UINT16 durationTime = 0;
                addSiteToReport = TI_TRUE;
               
                numOfSitesAdded++;

                TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, "rrmMgr_buildBeaconReport: AP IS VALID\n");
                 
                /* Setting the regulatory class field */
                param.paramType = REGULATORY_DOMAIN_GET_REG_CLASS_BY_BAND_AND_CHANNEL;
                param.content.regClassChannelList.Channel[0] = pSite->channel;
                param.content.regClassChannelList.band = (BandType_e)pSite->eBand;
                regulatoryDomain_getParam(pMeasurementMgr->hRegulatoryDomain, &param);

                *pReportFrame++ = param.content.regClass;
                beaconReportsTotalLen ++;

                
                /* setting the channel field */
                *pReportFrame++ = pSite->channel;
                beaconReportsTotalLen++;    
                
                /* setting the actual measurement TSF */
                os_memoryCopy(pMeasurementMgr->hOs,pReportFrame, &request.startTimeInTSF, 8);
                pReportFrame += 8;
                beaconReportsTotalLen += 8;                

                
                if (request.ScanMode != MSR_SCAN_MODE_BEACON_TABLE)
                {
                    durationTime = request.DurationTime; /* set duration to 0 */ 
                }

                /* setting the duration field */
                COPY_WLAN_WORD(pReportFrame, &durationTime);
                pReportFrame += 2;
                beaconReportsTotalLen += 2;  
                

                /* setting the reported frame Info field (0= beacon/probeResp)  */
                *pReportFrame++ = 0;
                beaconReportsTotalLen ++;

                /* setting the RSSI RCPI field  */
                *pReportFrame++ = (TI_UINT8)pSite->rssi;
                beaconReportsTotalLen++;

                /* setting the RSNI field  */
                *pReportFrame++ = (TI_UINT8)pSite->snr;
                beaconReportsTotalLen++;

                /* setting the BSSID field  */
                MAC_COPY(pReportFrame, pSite->bssid);
                pReportFrame += 6;
                beaconReportsTotalLen += 6;

                /* setting the antenna ID field  */
                *pReportFrame++ = 0; /* default value of our antenna ID */
                beaconReportsTotalLen++;

                
                /* setting the parent TSF field */
                COPY_WLAN_LONG(pReportFrame, pSite->tsfTimeStamp);
                pReportFrame += 4;
                beaconReportsTotalLen += 4;   

                subElementsLen = 0;

                
                if ((((reportingDetail_e)request.reportingDetailLevel) > REPORT_DETAIL_NO_FIXED_LENGTH_FIELDS_OR_ELES))
                {
                    TI_UINT8        *desiredIe;
                    TI_BOOL         foundIe = TI_FALSE;

                    /* setting the reported frame body sub-element ID field */
                    *pReportFrame++ = BEACON_REPORT_SUB_ELE_ID_REPORTED_FRAME;

                    pSubEleLen = pReportFrame;/* will be filled later */
                    *pReportFrame++ = 0;

                    subElementsLen = 2;
                    
                    /* copy the fixed length fields: Timestamp, beaconInterval & capabilities (12 bytes long) */
                    os_memoryCopy(pMeasurementMgr->hReport, (void*)pReportFrame,
                                  &pSite->tsfTimeStamp, TIME_STAMP_LEN);

                    pReportFrame += TIME_STAMP_LEN;
                    subElementsLen += TIME_STAMP_LEN;
                    
                    COPY_WLAN_WORD(pReportFrame, &pSite->beaconInterval);
                    pReportFrame += 2;
                    subElementsLen += 2;
                    
                    COPY_WLAN_WORD(pReportFrame, &pSite->capabilities);
                    pReportFrame += 2;
                    subElementsLen += 2;

                    if (((reportingDetail_e)request.reportingDetailLevel) == REPORT_DETAIL_ALL_FIXED_LENGTH_FIELDS_AND_REQ_ELES) 
                    {
                         TRACE1(pMeasurementMgr->hReport, 
                                REPORT_SEVERITY_INFORMATION, 
                                "rrmMgr_buildBeaconReport: reportingDetailLevel == 1. Copying IEs requested (%d IEs)\n",
                                request.uActualNumOfIEs);
                        
                        /* Iterate over all the IEs needed for the detailed report */
                        for (i=0; i< request.uActualNumOfIEs ; i++)
                        {

                            TRACE1(pMeasurementMgr->hReport, 
                                   REPORT_SEVERITY_INFORMATION, 
                                   "rrmMgr_buildBeaconReport: reportingDetailLevel == 1. Searching specific IE ID=%d\n",
                                   request.IEsIdNeededForDeatailedReport[i]);
                             
                            /* getting the IE ID */
                            if (pSite->beaconRecv)
                            {
                                foundIe = mlmeParser_ParseIeBuffer (pMeasurementMgr->hMlme, 
                                                                    pSite->beaconBuffer, 
                                                                    pSite->beaconLength, 
                                                                    request.IEsIdNeededForDeatailedReport[i], 
                                                                    &desiredIe, 
                                                                    NULL, 
                                                                    0);
                            }
                            else
                            {
                                foundIe = mlmeParser_ParseIeBuffer (pMeasurementMgr->hMlme, 
                                                                    pSite->probeRespBuffer, 
                                                                    pSite->probeRespLength, 
                                                                    request.IEsIdNeededForDeatailedReport[i], 
                                                                    &desiredIe, 
                                                                    NULL, 
                                                                    0);
                            }
    
                            /* Updating the right fields in the report sub-element section [reported frame body]*/
                            if (foundIe)
                            {
                                ieLen = desiredIe[1] + IE_HDR_LEN;

                                TRACE2(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, ": \t\tIE ID = %d, Len = %d\n", desiredIe[0], desiredIe[1]);
        
                                os_memoryCopy(pMeasurementMgr->hOs, 
                                              pReportFrame, desiredIe, ieLen);

                                pReportFrame += ieLen;
                                subElementsLen += ieLen;
                            }
    
                        }/* End of for loop */
                    }
                    else
                    { /* REPORT_DETAIL_ALL_FIXED_LENGTH_AND_ELES - copy from the ssid IE start position till the end */

                        /* Copy all the info elements (started after the 12 bytes of the Fixed Fields */
                        if (pSite->beaconRecv)
                        {
                            os_memoryCopy(pMeasurementMgr->hReport, 
                                     (void*)pReportFrame,
                                     (void*)&pSite->beaconBuffer[BEACON_PROBRESP_FIXED_LENGTH_FIELDS], 
                                          pSite->beaconLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS);

                            pReportFrame += pSite->beaconLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS;
                            subElementsLen += pSite->beaconLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS;
                        }
                        else /* Probe response */
                        {
                            os_memoryCopy(pMeasurementMgr->hReport, 
                                     (void*)pReportFrame,
                                     (void*)&pSite->probeRespBuffer[BEACON_PROBRESP_FIXED_LENGTH_FIELDS],
                                           pSite->probeRespLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS); 

                            pReportFrame += pSite->probeRespLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS;
                            subElementsLen += pSite->probeRespLength - BEACON_PROBRESP_FIXED_LENGTH_FIELDS;
                        }
                        
                    }

                    beaconReportsTotalLen += subElementsLen;
                    *pSubEleLen = subElementsLen - 2;
                    
                        
                }/* End of reporting deatils are required section*/ 
                else
                {
                    TRACE0(pMeasurementMgr->hReport, 
                           REPORT_SEVERITY_INFORMATION, 
                           "rrmMgr_buildBeaconReport: Reporting details are not required in sub-elements section\n");
                }
            }
            else
            {/* End of if condition: site is on a channel that should be reported */
                TRACE0(pMeasurementMgr->hReport, 
                       REPORT_SEVERITY_INFORMATION, 
                       "rrmMgr_buildBeaconReport: AP IGNORED!\n");
            }

        }/* End of condition: site is not null */

        
        /* and continue to the next site */
        pCurrentSite = scanResultTable_GetNext (hScanResultTable);
        
    }/* End of while there are scan results in the table */


    if (numOfSitesAdded > 0) 
    {

        if (subElementsLen > 0) 
        {
            *pSubEleLen += subElementsLen;/* The sub-elements total length */     
        }
        
        reportLen += beaconReportsTotalLen;
        *pReportLen = reportLen;
        pMeasurementMgr->frameLength += beaconReportsTotalLen; /* the total action frame to be sent later */

        TRACE3(pMeasurementMgr->hReport, 
               REPORT_SEVERITY_INFORMATION, 
               "rrmMgr_buildBeaconReport: %d sites added to the report. measurement request len =%d. subEle len=%d\n",
               numOfSitesAdded,
               reportLen,
               subElementsLen);
    }
    else
    {
        *pReportLen = 3;
        TRACE0(pMeasurementMgr->hReport, 
               REPORT_SEVERITY_INFORMATION, 
               "rrmMgr_buildBeaconReport: No sites found for the beacon request at all :-( \n");
    }

    TRACE1(pMeasurementMgr->hReport, 
           REPORT_SEVERITY_INFORMATION, 
           "rrmMgr_buildBeaconReport: Beacon report building has finished. action frame len = %d \n",
           pMeasurementMgr->frameLength);
      
    /* Yalla, finish this endless report !!! */
    return TI_OK;
    
}



/***********************************************************************
 *                   measurementMgr_dot11hSendReportAndCleanObject							
 ***********************************************************************
DESCRIPTION:	Send measurement report frame Function.
                The function does the following:
				-	Calls the mlmeBuilder to allocate a mangement frame
					and to transmit it.
				-	Cleans the Measurement Object and reset its Params.
				
INPUT:      hMeasurementMgr -	MeasurementMgr Handle
			
OUTPUT:		None

RETURN:     TI_OK on success, TI_NOK otherwise
************************************************************************/
TI_STATUS rrmMgr_SendReportAndCleanObject(TI_HANDLE hMeasurementMgr)
{ 
    measurementMgr_t    *pMeasurementMgr = (measurementMgr_t*)hMeasurementMgr;
    TI_STATUS       status;

    
    if (pMeasurementMgr->frameLength != 0) 
    {
        TRACE1(pMeasurementMgr->hReport,
               REPORT_SEVERITY_INFORMATION, 
               "rrmMgr_SendReportAndCleanObject: Sending the report(len=%d) \n",
               pMeasurementMgr->frameLength);
        
 
        /* Sending the Rport Frame */
        status =  mlmeBuilder_sendFrame(pMeasurementMgr->hMlme,
                                        ACTION,
                                        (TI_UINT8*)&pMeasurementMgr->rrmFrameReportBuff,
                                        pMeasurementMgr->frameLength, 0);
        if (status != TI_OK)
            return status;

        /* clearing reports data base */
        os_memoryZero(pMeasurementMgr->hOs,&(pMeasurementMgr->rrmFrameReportBuff),
                      RRM_REPORT_MAX_SIZE);
        
        pMeasurementMgr->frameLength = 0;
        pMeasurementMgr->nextEmptySpaceInReport = 0;
    }

    /* Reset the Measurement Object Params */
    pMeasurementMgr->currentFrameType = MSR_FRAME_TYPE_NO_ACTIVE;
    requestHandler_clearRequests(pMeasurementMgr->hRequestH);

	return TI_OK;
}



/********************************************************************************/
/*						Interface functions Implementation.						*/
/********************************************************************************/

TI_STATUS rrmMgr_buildAndSendNeighborAPsRequest(TI_HANDLE hMeasurementMgr,TSsid *pSsid)
{
    measurementMgr_t                *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
    NeighborMeasurementRequest_t    request;
    TI_UINT32                       reqLen = 0;
    
    request.measHdr.category = (TI_UINT8)CATEGORY_RRM;
    request.measHdr.action   = (TI_UINT8)NEIGHBOR_MEASUREMENT_REQUEST;

    pMeasurementMgr->tNeighborReport.dialogToken++; /* Increment the dialog token for each neighbor request */
    request.measHdr.dialogToken = pMeasurementMgr->tNeighborReport.dialogToken;


    reqLen += sizeof(request.measHdr);
    
    request.ssid.hdr[0] = DOT11_SSID_ELE_ID;
    request.ssid.hdr[1] = pSsid->len;
    os_memoryCopy(pMeasurementMgr->hOs, (void*)request.ssid.serviceSetId, (void*)(pSsid->str), pSsid->len); 

    reqLen += sizeof(dot11_eleHdr_t) + pSsid->len;

    TRACE1(pMeasurementMgr->hReport,
           REPORT_SEVERITY_INFORMATION, 
           "rrmMgr_buildAndSendNeighborAPsRequest: reqLen=%d \n", 
           reqLen);
    
    return mlmeBuilder_sendFrame(pMeasurementMgr->hMlme, ACTION, (TI_UINT8*)&request, reqLen, 0);
}



TI_STATUS rrmMgr_parseTSMRequest(TI_HANDLE hMeasurementMgr, EMeasurementFrameType frameType, 
                                 TI_UINT8 *pData, TI_UINT32 dataLen)
{
    TI_UINT32          recvReqLen = 0;
    measurementMgr_t   *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
    TI_UINT8           requestMode = 0;
    TSMParams_t        TSMParams; 
    

    TRACE1(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, "rrmMgr_parseTSMRequest: reqLen=%d \n", dataLen);

  
    /* skip the category and action field */
    pData += 2;
    
    TSMParams.frameToken = *pData++;
 
    /* LiorC: currently, number of repetitions field is expected to be 0 - meaning do the measurement once. 
       TBD: support numberOfRepetitions > 0 */
    COPY_WLAN_WORD(&TSMParams.frameNumOfRepetitions, pData);
    pData += 2;

     /* checking if received the correct information element ID */
    if (*pData != DOT11_MEASUREMENT_REQUEST_ELE_ID)
    {
        TRACE1(pMeasurementMgr->hReport,  REPORT_SEVERITY_INFORMATION, "rrmMgr_parseTSMRequest:  unrecognized IE ID = %d \n", *pData);
        return TI_NOK;
    }

    pData++;
    recvReqLen = *pData++;
    
    TSMParams.measurementToken = *pData++;
    requestMode = *pData++;
    
    if (requestMode != 0) 
    {
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "rrmMgr_parseTSMRequest: Error, parallel/Enable/request/report bit(s) is/are not expected to be set \n");
        return TI_NOK;
    }

    
    pData++; /* skip the request type field - checked already as we are in the TSM request ..*/


    /* start parsing the measuremnt request itslef from here */
    
    COPY_WLAN_WORD(&TSMParams.randomInterval, pData);
    pData += 2;

    
    COPY_WLAN_WORD(&TSMParams.uDuration, pData);
    pData += 2;

    TSMParams.bIsTriggeredReport = (TSMParams.uDuration == 0)? TI_TRUE : TI_FALSE;
    
    MAC_COPY(TSMParams.peerSTA, pData);
    pData += 6;

    TSMParams.uTID = (*pData) >> 4;
    pData++;

    TSMParams.uBin0Range = *pData++;

    /* optional sub-element */
    if (TI_TRUE == TSMParams.bIsTriggeredReport) 
    {
        if (*pData++ == TSM_REQUEST_SUB_ELE_AP_TRIGGERED_REPORTING_ELE_ID) 
        {
            if (*pData++ == TSM_REQUEST_SUB_ELE_AP_TRIGGERED_REPORTING_ELE_LEN) 
            {
                TSMParams.tTriggerReporting.triggerCondition = *pData++;
                TSMParams.tTriggerReporting.averageErrorThreshold = *pData++;
                TSMParams.tTriggerReporting.consecutiveErrorThreshold = *pData++;
                TSMParams.tTriggerReporting.delayThreshold = *pData++;
                TSMParams.tTriggerReporting.measuCount = *pData++;
                TSMParams.tTriggerReporting.triggerTimeout = *pData++;
                TSMParams.uDelayedMsduRange = TSMParams.tTriggerReporting.delayThreshold & 0x03;
                TSMParams.uDelayedMsduCount = TSMParams.tTriggerReporting.delayThreshold >> 2; 
            }
            
        }
        else
        {
            TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_INFORMATION, "rrmMgr_parseTSMRequest: SUB_ELE ID Reporting detail was not found \n");
        }  
    }

   
    if (TI_OK != txCtrl_UpdateTSMParameters(pMeasurementMgr->hTxCtrl, 
                                            &TSMParams,
                                            rrmMgr_buildAndSendTSMReport))

    {
        MeasurementRequest_t    tmpReq;
        MeasurementRequest_t*   pRequestsArr[1];

        tmpReq.frameToken =  TSMParams.frameToken;
        tmpReq.measurementToken = (TI_UINT16)TSMParams.measurementToken;
        tmpReq.Type = MSR_TYPE_RRM_TS_TC_MEASUREMENT;
        
        pRequestsArr[0] = &tmpReq;
        
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_WARNING, "rrmMgr_parseTSMRequest: Request is not available. Reject this request \n");
        rrmMgr_BuildRejectReport(hMeasurementMgr, pRequestsArr, 1, MSR_REJECT_INVALID_MEASUREMENT_TYPE);
 
    }
    return TI_OK;

}


void rrmMgr_buildAndSendTSMReport(TI_HANDLE hMeasurementMgr, TSMReportData_t  *pReportData)
{
    measurementMgr_t   *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
    TI_UINT8           *pReportFrame = pMeasurementMgr->rrmFrameReportBuff;
    TI_UINT32          reportLen = 0;
    TI_UINT32          msduDiscarded = 0;
    TI_UINT8           i=0;
    TI_STATUS          status = TI_OK;

    
    os_memoryZero(pMeasurementMgr->hOs, (void*)pMeasurementMgr->rrmFrameReportBuff, RRM_REPORT_MAX_SIZE);

    
    /* Filling the frame header (3 bytes) */
    *pReportFrame++ = (TI_UINT8)CATEGORY_RRM;
    *pReportFrame++ = (TI_UINT8)RRM_MEASUREMENT_REPORT;
    *pReportFrame++ = pReportData->frameToken;
    reportLen = 3;

    
    /* Filling the measurement report element header (4 bytes) */
    *pReportFrame++ = (TI_UINT8)DOT11_MEASUREMENT_REPORT_ELE_ID;
    
    *pReportFrame++ = RRM_TSM_REPORT_IE_FIXED_LENGTH;
    reportLen += (RRM_TSM_REPORT_IE_FIXED_LENGTH + 2);

    *pReportFrame++ = pReportData->measurementToken;
    
    *pReportFrame++ =  0; /* (Report mode ) Should be 0 if the report is ok and not a rejected one */

    *pReportFrame++ = (TI_UINT8)MSR_TYPE_RRM_TS_TC_MEASUREMENT; /* Measurement Type */
  
    /* The TSM report body starts from here */
    os_memoryCopy(pMeasurementMgr->hOs, pReportFrame, (void*)pReportData->actualMeasurementTSF, 8); 
    pReportFrame += 8;

    COPY_WLAN_WORD(pReportFrame, &pReportData->measurementDuration);
    pReportFrame += 2;

    MAC_COPY(pReportFrame, pReportData->peerSTA);
    pReportFrame += 6;

    
    *pReportFrame++ = pReportData->uTID << 4;
    
    if (TI_TRUE == pReportData->bIsTriggeredReport) 
    {
         *pReportFrame++ = pReportData->reportingReason;
    }
    else
    {
        *pReportFrame++ = 0;
    }

    COPY_WLAN_LONG(pReportFrame, &pReportData->uMsduTransmittedOKCounter);
    pReportFrame += 4; 

    msduDiscarded = pReportData->uMsduRetryExceededCounter + pReportData->uMsduLifeTimeExpiredCounter;
    COPY_WLAN_LONG(pReportFrame, &msduDiscarded);
    pReportFrame += 4; 

    /* msdu failed count field */
    COPY_WLAN_LONG(pReportFrame, &pReportData->uMsduRetryExceededCounter);
    pReportFrame += 4; 

    /* msdu multiple retry count field */
    COPY_WLAN_LONG(pReportFrame, &pReportData->uMsduMultipleRetryCounter);
    pReportFrame += 4;

    /* skip the QoS CF-Polls Lost Count field */
    pReportFrame += 4;

    /* skip the Average Queue Delay field */
    pReportFrame += 4;

    /* skip the Average Transmit Delay field */
    pReportFrame += 4;

    /* Bin0 Threshold field */
    *pReportFrame++ = pReportData->bin0Range;

    
    for ( i=0 ; i< TSM_REPORT_NUM_OF_BINS_MAX ; i++) 
    {
        COPY_WLAN_LONG(pReportFrame, &pReportData->binsDelayCounter[i]);
        pReportFrame += 4;
    }

    pMeasurementMgr->frameLength = reportLen;


    report_PrintDump(pMeasurementMgr->rrmFrameReportBuff, pMeasurementMgr->frameLength);

    
    /* Yalla, send this report */
    status =  mlmeBuilder_sendFrame(pMeasurementMgr->hMlme,
                                    ACTION,
                                    (TI_UINT8*)&pMeasurementMgr->rrmFrameReportBuff,
                                    pMeasurementMgr->frameLength, 0);


    if (TI_OK != status)
    {
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "rrmMgr_parseTSMRequest: Failed to send the TSM Report \n");
    }
}



TI_STATUS rrmMgr_RecvNeighborApsReportResponse(TI_HANDLE hMeasurementMgr,TI_UINT8 *pFrame, TI_UINT32 len)
{
    measurementMgr_t            *pMeasurementMgr = (measurementMgr_t *) hMeasurementMgr;
    TI_UINT8                    dialogToken = 0;
    TI_UINT8                   *pReport = NULL;
    TI_UINT8                    reportIELen = 0;
    TI_UINT8                    numOfApsFound = 0;
    neighborAP_t                *pApEntry;
    paramInfo_t                 tParam;
    TI_STATUS                   status = TI_OK;


    pFrame += 2; /* skip the category & action fields */
    len -= 2;
    
    dialogToken = *pFrame;
   
    
    if (dialogToken != pMeasurementMgr->tNeighborReport.dialogToken) 
    {
        TRACE0(pMeasurementMgr->hReport, REPORT_SEVERITY_ERROR, "rrmMgr_RecvNeighborApsReportResponse: dialog token mismatch - Neighbor Report Response Unrecognized!  \n");
        return TI_NOK;
    }

    pFrame++;
    len--;

    pMeasurementMgr->tNeighborReport.neighborApsList.numOfEntries = 0;

    pReport = pFrame;


    while (len > 0) 
    {       
        if (*pReport++ == NEIGHBOR_REPORT_ELE_ID) 
        {
            pApEntry = &(pMeasurementMgr->tNeighborReport.neighborApsList.APListPtr[numOfApsFound]);

            reportIELen = *pReport++; /* the current neighbor IE length */ 
            
            MAC_COPY (pApEntry->BSSID , pReport);
            pReport += 6;

            pReport += 4; /* skip the bssidInfo field */
            
            /* Set the band according to the regulatory class in the current zone */
            tParam.content.regClass =  *pReport++;
            tParam.paramType = REGULATORY_DOMAIN_GET_BAND_BY_REG_CLASS_PARAM;
            status = regulatoryDomain_getParam (pMeasurementMgr->hRegulatoryDomain, &tParam);
        
            if ((status != TI_OK) || ((ERadioBand)tParam.content.radioBand > BAND_TYPE_5_GHZ)) /* currently the STA support only 2.4/5 Ghz*/
            {
                TRACE1(pMeasurementMgr->hReport, 
                       REPORT_SEVERITY_ERROR, 
                       "rrmMgr_RecvNeighborApsReportResponse: band %d is not supported in discovery scan  \n", 
                       tParam.content.radioBand);
                
                return TI_NOK;
            }
            else
            {
                pApEntry->band = (ERadioBand)tParam.content.radioBand;
            }

            pApEntry->channel = *pReport++;

            len -= (sizeof(dot11_eleHdr_t) + reportIELen);
            
            pFrame +=  sizeof(dot11_eleHdr_t) + reportIELen;/* go to the next report element */
            numOfApsFound++;
            pMeasurementMgr->tNeighborReport.neighborApsList.numOfEntries = numOfApsFound;

            TRACE6(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_INFORMATION, 
                   "rrmMgr_RecvNeighborApsReportResponse: BBSID Found %02x.%02x.%02x.%02x.%02x.%02x.  \n", 
                   pApEntry->BSSID[0],
                   pApEntry->BSSID[1],
                   pApEntry->BSSID[2],
                   pApEntry->BSSID[3],
                   pApEntry->BSSID[4],
                   pApEntry->BSSID[5]);
            
        }
        else
        {
            TRACE0(pMeasurementMgr->hReport, 
                   REPORT_SEVERITY_ERROR, 
                   "rrmMgr_RecvNeighborApsReportResponse: Neighbor Report Element ID was not found!!  \n");
          
            break;
        }
    }

    apConn_updateNeighborAPsList(pMeasurementMgr->hApConn, &(pMeasurementMgr->tNeighborReport.neighborApsList)); 
                                     
    return status;
}





