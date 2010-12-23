/*
 * rrmMgr.h
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

/** \file spectrumMngmntMgr.h
 *  \brief dot11h spectrum Management Meneger module interface header file
 *
 *  \see spectrumMngmntMgr.c
 */

/***************************************************************************/
/*																			*/
/*	  MODULE:	spectrumMngmntMgr.h											*/
/*    PURPOSE:	dot11h spectrum Management Meneger module interface         */
/*              header file                                        			*/
/*																			*/
/***************************************************************************/
#ifndef __RRMMGR_H__
#define __RRMMGR_H__

#include "paramOut.h"
#include "measurementMgr.h"
#include "requestHandler.h"
#include "StaCap.h"
#include "txCtrl_Api.h"



#define RRM_ENABLED_CAPABILITY                      BIT_12
#define RRM_LINK_MEASURE_CAPABILITY				    BIT_0
#define RRM_NEIGHBOR_MEASURE_CAPABILITY 			BIT_1
#define RRM_BEACON_PASSIVE_MEASURE_CAPABILITY		BIT_4
#define RRM_BEACON_ACTIVE_MEASURE_CAPABILITY		BIT_5
#define RRM_BEACON_TABLE_MEASURE_CAPABILITY		    BIT_6
#define RRM_TS_TC_NORMAL_MEASURE_CAPABILITY			BIT_14
#define RRM_TS_TC_TRIGGERED_MEASURE_CAPABILITY 		BIT_15
#define RRM_AP_CHANNEL_REPORT_CAPABILITY	        BIT_16


/********************************************************************************/
/*						Internal Structures.        							*/
/********************************************************************************/

#define BEACON_REQUEST_MAX_SUB_ELEMENTS_LEN                 (100)
#define BEACON_REQUEST_SUB_ELE_SSID                         (0)
#define BEACON_REQUEST_SUB_ELE_REPORTING_INFO               (1)
#define BEACON_REQUEST_SUB_ELE_REPORTING_DETAIL             (2)
#define BEACON_REQUEST_SUB_ELE_REQUEST_INFO                 (10)
#define BEACON_REQUEST_SUB_ELE_AP_CHANNEL_REPORT            (51)

#define BEACON_REPORT_SUB_ELE_ID_REPORTED_FRAME             (1)
#define BEACON_REPORT_MAX_SUB_ELEMENTS_LEN                  (224)
#define BEACON_REPORT_MAX_APS_NUM_TO_REPORT                 (8)

#define BEACON_REPORTS_MAX_LEN                              (2000)
#define BEACON_REQUEST_FIXED_FIELDS_LEN                     (17)


#define TSM_REQUEST_SUB_ELE_AP_TRIGGERED_REPORTING_ELE_ID   (1)
#define TSM_REQUEST_SUB_ELE_AP_TRIGGERED_REPORTING_ELE_LEN  (6)


typedef enum
{
    REPORT_DETAIL_NO_FIXED_LENGTH_FIELDS_OR_ELES,
    REPORT_DETAIL_ALL_FIXED_LENGTH_FIELDS_AND_REQ_ELES,
    REPORT_DETAIL_ALL_FIXED_LENGTH_AND_ELES
} reportingDetail_e;

typedef struct 
{
    TI_UINT8  IeId;
    TI_UINT8  Length;
    TI_UINT8  Token;
    TI_UINT8  ReqMode; /* Bit Field Parallel/ Duration Mandatory */
    TI_UINT8  ReqType; /* Beacon, TSM */
    
} rrmReqReportIEHdr_t;


/* RRM Beacon Request structure (802.11k) */
typedef struct 
{

    rrmReqReportIEHdr_t     hdr;
    TI_UINT8                regulatoryClass;
    TI_UINT8                channelNumber;
    TI_UINT16               randomInterval;
    TI_UINT16               duration;
    TI_UINT8                mode;        /*  passive, TSM */
    TMacAddr                bssid;       /* Fixed size = 13 bytes */
    TI_UINT8                subElements[BEACON_REQUEST_MAX_SUB_ELEMENTS_LEN];
} beaconReq_t;



/* RRM Beacon Report structure (802.11k) */
typedef struct
{
    RadioMeasurementHdr_t    hdrFrame;
    TI_UINT16                numOfRepetitions;
    beaconReq_t              beaconRequest;
}  beaconRequestFrame_t;



/* The APIs (Beacon report) below are the CBs to be used in measurments infra structure - registered in measurementMgrSM_acConnected() */
TI_STATUS rrmMgr_ParseFrameReq(TI_HANDLE hMeasurementMgr, 
                                           TI_UINT8 *pData, TI_INT32 dataLen,
                                           TMeasurementFrameRequest	*frameReq);

TI_STATUS rrmMgr_ParseRequestElement(TI_HANDLE hMeasurementMgr,
                                     TI_UINT8 *pData, TI_UINT16 *measurementToken, 
                                     TI_UINT8* singleReqLen, MeasurementRequest_t *pCurrRequest);

TI_BOOL   rrmMgr_IsTypeValid(TI_HANDLE hMeasurementMgr, 
                                              EMeasurementType type, 
                                              EMeasurementScanMode scanMode);

TI_STATUS rrmMgr_BuildReport(TI_HANDLE hMeasurementMgr, MeasurementRequest_t request, TMeasurementTypeReply * reply);

TI_STATUS rrmMgr_SendReportAndCleanObject(TI_HANDLE hMeasurementMgr);

TI_STATUS rrmMgr_BuildRejectReport(TI_HANDLE hMeasurementMgr,
											 MeasurementRequest_t *pRequestArr[],
											 TI_UINT8		numOfRequestsInParallel,
											 EMeasurementRejectReason	rejectReason);


/* API for the received Radio Measurement Request (Type: Beacon/TSM)*/ 
TI_STATUS rrmMgr_receiveFrameRequest(TI_HANDLE hMeasurementMgr, EMeasurementFrameType frameType,
                                     TMacAddr *pBssidSender, TI_INT32 dataLen, TI_UINT8 * pData);



/* API for the TSM report */
TI_STATUS rrmMgr_parseTSMRequest(TI_HANDLE hMeasurementMgr, EMeasurementFrameType frameType, 
                                 TI_UINT8 *pData, TI_UINT32 dataLen);

    void rrmMgr_buildAndSendTSMReport(TI_HANDLE hMeasurementMgr, TSMReportData_t  *pReportData);



/* API for the neighbor report */
TI_STATUS rrmMgr_RecvNeighborApsReportResponse(TI_HANDLE hMeasurementMgr,TI_UINT8 *pFrame, TI_UINT32 len);


TI_STATUS rrmMgr_buildAndSendNeighborAPsRequest(TI_HANDLE hMeasurementMgr,TSsid *pSsid);



#endif /* __RRMMGR_H__ */
