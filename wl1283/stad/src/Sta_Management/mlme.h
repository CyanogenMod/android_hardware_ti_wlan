/*
 * mlme.h
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

/** \file mlme.h
 *  \brief MLME
 *
 *  \see mlme.c
 */

/***************************************************************************/
/*                                                                          */
/*    MODULE:   mlmeApi.h                                                   */
/*    PURPOSE:  MLME API                                                    */
/*                                                                          */
/***************************************************************************/
#ifndef __MLME_H__
#define __MLME_H__

#include "tidef.h"
#include "paramOut.h"
#include "mlmeApi.h"
#include "connApi.h"

#define AUTH_MSG_HEADER_LEN		6
#define MAX_CHALLANGE_LEN		256
#define MD_IE_LEN               5
#define MAX_RSN_IE_LEN          256
#define FT_IE_LEN               186
#define TSPEC_IE_LEN            55
#define RIC_REQUEST_LEN         (6 + TSPEC_IE_LEN*2) /* support upto 2 tspec renegotiation in reassociation */
/* since an auth request can not contain simultaneously a RSNIE and a chanllenge IE max size is headerLen + 256*/
#define MAX_AUTH_MSG_LEN		(MAX_CHALLANGE_LEN + AUTH_MSG_HEADER_LEN)
#define MAX_EXTRA_IES_LEN       (MAX_RSN_IE_LEN + MD_IE_LEN + FT_IE_LEN + RIC_REQUEST_LEN)
#define MAX_ASSOC_MSG_LENGTH	(512 + MAX_EXTRA_IES_LEN)

/* Data structures */
typedef struct
{
	TI_UINT16			status;
	char				*pChalange;
	TI_UINT8			challangeLen;
} authData_t;

typedef struct /* Authentication info */
{
	legacyAuthType_e	authType;
	TI_UINT32			maxCount;
	TI_UINT8			retryCount;
	TI_UINT32			authRejectCount;
	TI_UINT32			authTimeoutCount;
	TI_UINT32			timeout;
    authData_t          authData;
} auth_t;


typedef struct /* Association info */
{
	TI_UINT32				    timeout;
	TI_UINT8				    currentState;
	TI_UINT32				    maxCount;
	TI_UINT8				    retryCount;
	TI_UINT32				    assocRejectCount;
	TI_UINT32				    assocTimeoutCount;
	char				        *pChalange;
    TI_UINT8                    assocRespBuffer[MAX_ASSOC_MSG_LENGTH];
    TI_UINT32                   assocRespLen;
    TI_UINT8                    assocReqBuffer[MAX_ASSOC_MSG_LENGTH];
    TI_UINT32                   assocReqLen;
  	TI_BOOL					    reAssocResp;

	TI_BOOL 				    disAssoc; /* When set dissasociation frame will be sent. */

} assoc_t;


typedef struct
{
    mgmtStatus_e mgmtStatus;
    TI_UINT16        uStatusCode;
} mlmeData_t;


typedef struct
{
    TI_UINT8            currentState;
    mlmeData_t          mlmeData;
    TI_BOOL             reAssoc;
    DisconnectType_e    disConnType;
    mgmtStatus_e        disConnReason;
    TI_BOOL             bParseBeaconWSC;
    TI_BOOL             bSharedFailed;

	legacyAuthType_e	legacyAuthType;
	auth_t				authInfo;
	assoc_t				assocInfo;

    TI_UINT8			extraIes[MAX_EXTRA_IES_LEN];
	TI_UINT16			extraIesLen;

	TI_HANDLE			hMlmeTimer;

    /* temporary frame info */
    mlmeIEParsingParams_t tempFrameInfo;

    TI_UINT8            assocMsg[MAX_ASSOC_MSG_LENGTH];

    /* debug info - start */
    TI_UINT32           debug_lastProbeRspTSFTime;
    TI_UINT32           debug_lastDtimBcnTSFTime;
    TI_UINT32           debug_lastBeaconTSFTime;
    TI_BOOL             debug_isFunctionFirstTime;
    TI_UINT32           totalMissingBeaconsCounter;
    TI_UINT32           totalRcvdBeaconsCounter;
    TI_UINT32           maxMissingBeaconSequence;
    TI_UINT32           BeaconsCounterPS;
    /* debug info - end */

	TI_HANDLE           hMlmeSm;

	TI_HANDLE           hSiteMgr;
    TI_HANDLE           hCtrlData;
    TI_HANDLE           hConn;
    TI_HANDLE           hTxMgmtQ;
    TI_HANDLE           hMeasurementMgr;
    TI_HANDLE           hSwitchChannel;
    TI_HANDLE           hRegulatoryDomain;
    TI_HANDLE           hReport;
    TI_HANDLE           hOs;
    TI_HANDLE           hCurrBss;
    TI_HANDLE           hApConn;
    TI_HANDLE           hScanCncn;
    TI_HANDLE           hQosMngr;
    TI_HANDLE           hTWD;
    TI_HANDLE           hTxCtrl;
	TI_HANDLE           hRsn;
	TI_HANDLE			hStaCap;
	TI_HANDLE			hSme;
	TI_HANDLE           hTimer;
    TI_HANDLE           hXCCMngr;
} mlme_t;



/* internal functions */
TI_STATUS mlme_sendAuthRequest(mlme_t *pMlme);

TI_STATUS mlme_sendSharedRequest(mlme_t *pMlme);

TI_STATUS mlme_authMsgBuild(mlme_t *pMlme, legacyAuthType_e authType, TI_UINT16 seq, TI_UINT16 statusCode,
							TI_UINT8* extraIes, TI_UINT8 extraIesLen,
							TI_UINT8 *authMsg, TI_UINT16 *authMsgLen);

TI_STATUS mlme_sendAssocRequest(mlme_t *pMlme);

TI_STATUS mlme_assocRequestMsgBuild(mlme_t *pCtx, TI_UINT8* reqBuf, TI_UINT32* reqLen);

TI_STATUS mlme_assocCapBuild(mlme_t *pCtx, TI_UINT16 *cap);

TI_STATUS mlme_assocSSIDBuild(mlme_t *pCtx, TI_UINT8 *pSSID, TI_UINT32 *ssidLen);

TI_STATUS mlme_assocRatesBuild(mlme_t *pCtx, TI_UINT8 *pRates, TI_UINT32 *ratesLen);

TI_STATUS mlme_assocPowerCapabilityBuild(mlme_t *pCtx, TI_UINT8 *pPowerCapability, TI_UINT32 *powerCapabilityLen);


#endif

