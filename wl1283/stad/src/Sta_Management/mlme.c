/*
 * mlme.c
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

/** \file mlme.c
 *  \brief 802.11 MLME source
 *
 *  \see mlme.h
 */


/***************************************************************************/
/*                                                                         */
/*      MODULE: mlme.c                                                     */
/*    PURPOSE:  802.11 MLME source                                         */
/*                                                                         */
/***************************************************************************/

#define __FILE_ID__  FILE_ID_64
#include "mlme.h"
#include "report.h"
#include "GenSM.h"
#include "mlmeSm.h"
#include "mlmeBuilder.h"
#include "timer.h"
#include "siteMgrApi.h"
#include "regulatoryDomainApi.h"
#include "qosMngr_API.h"
#include "StaCap.h"
#include "apConn.h"
#include "smeApi.h"

#ifdef XCC_MODULE_INCLUDED
#include "XCCMngr.h"
#include "XCCRMMngr.h"
#endif


/**
*
* mlme_Create - allocate memory for MLME SM
*
* \b Description:
* Allocate memory for MLME SM. \n
*       Allocates memory for MLME context. \n
*       Allocates memory for MLME timer. \n
*       Allocates memory for MLME SM matrix. \n
*
* \b ARGS:
*  I   - pOs - OS context  \n
*
* \b RETURNS:
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_mainSecSmKeysOnlyStop()
*/
TI_HANDLE mlme_create(TI_HANDLE hOs)
{
    mlme_t  *pHandle;
    /* allocate MLME context memory */
    pHandle = (mlme_t*)os_memoryAlloc(hOs, sizeof(mlme_t));
    if (pHandle == NULL)
    {
        return NULL;
    }

	/* nullify the MLME object */
    os_memoryZero(hOs, (void*)pHandle, sizeof(mlme_t));

	pHandle->hMlmeSm = genSM_Create (hOs);
	if (NULL == pHandle->hMlmeSm)
	{
		WLAN_OS_REPORT (("mlme_Create: unable to create MLME generic SM. MLME creation failed\n"));
		mlme_destroy ((TI_HANDLE)pHandle);
		return NULL;
	}

    pHandle->hOs = hOs;
    return pHandle;
}

/**
*
* mlme_destroy - destrit MLME
*
* \b Description:
* Unload MLME SM from memory
*
* \b ARGS:
*  I   - hMlme - MLME SM context  \n
*
* \b RETURNS:
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_mainSecSmKeysOnlyStop()
*/
TI_STATUS mlme_destroy(TI_HANDLE hMlme)
{
    mlme_t      *pMlme;

    pMlme = (mlme_t*)hMlme;
	if (NULL != pMlme->hMlmeTimer)
	{
		tmr_DestroyTimer (pMlme->hMlmeTimer);
	}

	if (NULL != pMlme->hMlmeSm)
    {
        genSM_Unload (pMlme->hMlmeSm);
    }
    os_memoryFree(pMlme->hOs, hMlme, sizeof(mlme_t));

    return TI_OK;
}

void mlme_init (TStadHandlesList *pStadHandles)
{
    mlme_t *pHandle = (mlme_t *)(pStadHandles->hMlme);

    pHandle->currentState = MLME_SM_STATE_IDLE;
    pHandle->legacyAuthType = AUTH_LEGACY_NONE;
    pHandle->reAssoc = TI_FALSE;
    pHandle->disConnType = DISCONNECT_IMMEDIATE;
    pHandle->disConnReason = STATUS_UNSPECIFIED;
    pHandle->bSharedFailed = TI_FALSE;

    pHandle->hSiteMgr          = pStadHandles->hSiteMgr;
    pHandle->hCtrlData         = pStadHandles->hCtrlData;
    pHandle->hTxMgmtQ          = pStadHandles->hTxMgmtQ;
    pHandle->hMeasurementMgr   = pStadHandles->hMeasurementMgr;
    pHandle->hSwitchChannel    = pStadHandles->hSwitchChannel;
    pHandle->hRegulatoryDomain = pStadHandles->hRegulatoryDomain;
    pHandle->hReport           = pStadHandles->hReport;
    pHandle->hOs               = pStadHandles->hOs;
    pHandle->hConn             = pStadHandles->hConn;
    pHandle->hCurrBss          = pStadHandles->hCurrBss;
    pHandle->hApConn           = pStadHandles->hAPConnection;
    pHandle->hScanCncn         = pStadHandles->hScanCncn;
    pHandle->hQosMngr          = pStadHandles->hQosMngr;
    pHandle->hTWD              = pStadHandles->hTWD;
    pHandle->hTxCtrl           = pStadHandles->hTxCtrl;
	pHandle->hRsn              = pStadHandles->hRsn;
	pHandle->hStaCap           = pStadHandles->hStaCap;
	pHandle->hSme			   = pStadHandles->hSme;
	pHandle->hTimer            = pStadHandles->hTimer;
#ifdef XCC_MODULE_INCLUDED
    pHandle->hXCCMngr          = pStadHandles->hXCCMngr;
#endif

    /*
    debug info
    */
    pHandle->debug_lastProbeRspTSFTime = 0;
    pHandle->debug_lastDtimBcnTSFTime = 0;
    pHandle->debug_lastBeaconTSFTime = 0;
    pHandle->debug_isFunctionFirstTime = TI_TRUE;
    pHandle->BeaconsCounterPS = 0;

	genSM_Init(pHandle->hMlmeSm, pHandle->hReport);

	os_memoryZero(pHandle->hOs, &(pHandle->authInfo), sizeof(auth_t));
	os_memoryZero(pHandle->hOs, &(pHandle->assocInfo), sizeof(assoc_t));
	os_memoryZero(pHandle->hOs, (TI_UINT8*)pHandle->extraIes, MAX_EXTRA_IES_LEN);

	pHandle->extraIesLen = 0;
}

void mlme_SetDefaults (TI_HANDLE hMlme, TMlmeInitParams *pMlmeInitParams)
{
    mlme_t *pMlme = (mlme_t *)(hMlme);

	/* set default values */
    pMlme->bParseBeaconWSC = pMlmeInitParams->parseWSCInBeacons;
	pMlme->authInfo.timeout = pMlmeInitParams->authResponseTimeout;
	pMlme->authInfo.maxCount = pMlmeInitParams->authMaxRetryCount;
	pMlme->assocInfo.timeout = pMlmeInitParams->assocResponseTimeout;
    pMlme->assocInfo.maxCount = pMlmeInitParams->assocMaxRetryCount;

	pMlme->hMlmeTimer = tmr_CreateTimer (pMlme->hTimer);
	if (NULL == pMlme->hMlmeTimer)
	{
		WLAN_OS_REPORT (("mlme_Create: unable to create MLME timer. MLME creation failed\n"));
		return;
	}

	/* Initialize the MLME state-machine */
	genSM_SetDefaults (pMlme->hMlmeSm, MLME_SM_NUMBER_OF_STATES, MLME_SM_NUMBER_OF_EVENTS, (TGenSM_matrix)mlmeMatrix,
                       MLME_SM_STATE_IDLE, "MLME SM", uMlmeStateDescription, uMlmeEventDescription, __FILE_ID__);

}


TI_STATUS mlme_setParam(TI_HANDLE           hMlme,
                        paramInfo_t         *pParam)
{
    mlme_t *pMlme = (mlme_t *)hMlme;

    switch(pParam->paramType)
    {
	case MLME_LEGACY_TYPE_PARAM:

		if (pParam->content.mlmeLegacyAuthType == AUTH_LEGACY_OPEN_SYSTEM ||
			pParam->content.mlmeLegacyAuthType == AUTH_LEGACY_RESERVED1)
		{
			pMlme->authInfo.authType = AUTH_LEGACY_OPEN_SYSTEM;
		}
		else
		{
			pMlme->authInfo.authType = AUTH_LEGACY_SHARED_KEY;
		}
		return TI_OK;

    case MLME_RE_ASSOC_PARAM:
        pMlme->reAssoc = pParam->content.mlmeReAssoc;
        break;

    default:
        TRACE1(pMlme->hReport, REPORT_SEVERITY_ERROR, "Set param, Params is not supported, 0x%x\n\n", pParam->paramType);
        return PARAM_NOT_SUPPORTED;
    }

    return TI_OK;
}

TI_STATUS mlme_getParam(TI_HANDLE           hMlme,
                        paramInfo_t         *pParam)
{
    mlme_t *pMlme = (mlme_t *)hMlme;

    switch(pParam->paramType)
    {
    case MLME_LEGACY_TYPE_PARAM:
        pParam->content.mlmeLegacyAuthType = pMlme->authInfo.authType;
        break;

    case MLME_CAPABILITY_PARAM:
        pParam->content.mlmeLegacyAuthType = pMlme->legacyAuthType;
        mlme_assocCapBuild(pMlme, &(pParam->content.siteMgrSiteCapability));
        break;

    case MLME_BEACON_RECV:
        pParam->content.siteMgrTiWlanCounters.BeaconsRecv = pMlme->BeaconsCounterPS;
        break;

	case MLME_AUTH_RESPONSE_TIMEOUT_PARAM:
		pParam->content.authResponseTimeout = pMlme->authInfo.timeout;
		break;

	case MLME_AUTH_COUNTERS_PARAM:
		pParam->content.siteMgrTiWlanCounters.AuthRejects = pMlme->authInfo.authRejectCount;
		pParam->content.siteMgrTiWlanCounters.AuthTimeouts = pMlme->authInfo.authTimeoutCount;
		break;

	case MLME_ASSOC_RESPONSE_TIMEOUT_PARAM:
		pParam->content.assocResponseTimeout = pMlme->assocInfo.timeout;
		break;

	case MLME_ASSOCIATION_REQ_PARAM:
        pParam->content.assocReqBuffer.buffer = pMlme->assocInfo.assocReqBuffer;
        pParam->content.assocReqBuffer.bufferSize = pMlme->assocInfo.assocReqLen;
		pParam->content.assocReqBuffer.reAssoc = pMlme->reAssoc;
        break;

	case MLME_ASSOCIATION_RESP_PARAM:
        pParam->content.assocReqBuffer.buffer = pMlme->assocInfo.assocRespBuffer;
        pParam->content.assocReqBuffer.bufferSize = pMlme->assocInfo.assocRespLen;
		pParam->content.assocReqBuffer.reAssoc = pMlme->assocInfo.reAssocResp;
        break;

	case MLME_ASSOC_COUNTERS_PARAM:
        pParam->content.siteMgrTiWlanCounters.AssocRejects = pMlme->assocInfo.assocRejectCount;
        pParam->content.siteMgrTiWlanCounters.AssocTimeouts = pMlme->assocInfo.assocTimeoutCount;
        break;

	case MLME_ASSOCIATION_INFORMATION_PARAM:
       {
           TI_UINT8  reqBuffIEOffset, respBuffIEOffset;
           TI_UINT32 RequestIELength = 0;
           TI_UINT32 ResponseIELength = 0;
           paramInfo_t  *lParam;
           ScanBssType_enum bssType;

           TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "MLME: DEBUG - Association Information Get:  \n");
           lParam = (paramInfo_t *)os_memoryAlloc(pMlme->hOs, sizeof(paramInfo_t));
           if (!lParam)
           {
               return TI_NOK;
           }

           /* Assoc exists only in Infrastructure */
           lParam->paramType = CTRL_DATA_CURRENT_BSS_TYPE_PARAM;
           ctrlData_getParam(pMlme->hCtrlData, lParam);
           bssType = lParam->content.ctrlDataCurrentBssType;
           os_memoryFree(pMlme->hOs, lParam, sizeof(paramInfo_t));
           if (bssType != BSS_INFRASTRUCTURE)
           {
               TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "Not in Infrastructure BSS, No ASSOC Info for GET ASSOC_ASSOCIATION_INFORMATION_PARAM\n");
               return TI_NOK;
           }

           /* Init the result buffer to 0 */
           os_memoryZero(pMlme->hOs ,&pParam->content, sizeof(OS_802_11_ASSOCIATION_INFORMATION));

           reqBuffIEOffset  = 4;  /* In Assoc request frame IEs are located from byte 4 */
           respBuffIEOffset = 6;  /* In Assoc response frame the IEs are located from byte 6 */

            /* If the last associate was re-associciation, the current AP MAC address */
            /* is placed before the IEs. Copy it to the result parameters.            */
            if (pMlme->reAssoc)
            {
                MAC_COPY (pParam->content.assocAssociationInformation.RequestFixedIEs.CurrentAPAddress,
                          &pMlme->assocInfo.assocReqBuffer[reqBuffIEOffset]);
                reqBuffIEOffset += MAC_ADDR_LEN;
            }

            /* Calculate length of Info elements in assoc request and response frames */
            if(pMlme->assocInfo.assocReqLen > reqBuffIEOffset)
                RequestIELength = pMlme->assocInfo.assocReqLen - reqBuffIEOffset;

            if(pMlme->assocInfo.assocRespLen > respBuffIEOffset)
                ResponseIELength = pMlme->assocInfo.assocRespLen - respBuffIEOffset;

            /* Copy the association request information */
            pParam->content.assocAssociationInformation.Length = sizeof(OS_802_11_ASSOCIATION_INFORMATION);
            pParam->content.assocAssociationInformation.AvailableRequestFixedIEs = OS_802_11_AI_REQFI_CAPABILITIES | OS_802_11_AI_REQFI_LISTENINTERVAL;
            pParam->content.assocAssociationInformation.RequestFixedIEs.Capabilities = *(TI_UINT16*)&(pMlme->assocInfo.assocReqBuffer[0]);
            pParam->content.assocAssociationInformation.RequestFixedIEs.ListenInterval = *(TI_UINT16*)(&pMlme->assocInfo.assocReqBuffer[2]);

            pParam->content.assocAssociationInformation.RequestIELength = RequestIELength;
            pParam->content.assocAssociationInformation.OffsetRequestIEs = 0;
            if (RequestIELength > 0)
            {
                pParam->content.assocAssociationInformation.OffsetRequestIEs = (TI_UINT32)&pMlme->assocInfo.assocReqBuffer[reqBuffIEOffset];
            }
            /* Copy the association response information */
            pParam->content.assocAssociationInformation.AvailableResponseFixedIEs =
                OS_802_11_AI_RESFI_CAPABILITIES | OS_802_11_AI_RESFI_STATUSCODE | OS_802_11_AI_RESFI_ASSOCIATIONID;
            pParam->content.assocAssociationInformation.ResponseFixedIEs.Capabilities = *(TI_UINT16*)&(pMlme->assocInfo.assocRespBuffer[0]);
            pParam->content.assocAssociationInformation.ResponseFixedIEs.StatusCode = *(TI_UINT16*)&(pMlme->assocInfo.assocRespBuffer[2]);
            pParam->content.assocAssociationInformation.ResponseFixedIEs.AssociationId = *(TI_UINT16*)&(pMlme->assocInfo.assocRespBuffer[4]);
            pParam->content.assocAssociationInformation.ResponseIELength = ResponseIELength;
            pParam->content.assocAssociationInformation.OffsetResponseIEs = 0;
            if (ResponseIELength > 0)
            {
                pParam->content.assocAssociationInformation.OffsetResponseIEs = (TI_UINT32)&pMlme->assocInfo.assocRespBuffer[respBuffIEOffset];
            }
	   }
	   break;
    default:
        TRACE1(pMlme->hReport, REPORT_SEVERITY_ERROR, "Get param, Params is not supported, %d\n\n", pParam->content.mlmeLegacyAuthType);
        return PARAM_NOT_SUPPORTED;
    }

    return TI_OK;
}

/**
* mlme_Start - Start event for the MLME SM
*
* \b Description:
* Start event for the MLME SM
*
* \b ARGS:
*  I   - hMlme - MLME SM context  \n
*  II  - connectionType - roaming or initial? with FT (802.11r) or not?
* \b RETURNS:
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa mlme_Stop, mlme_Recv
*/
TI_STATUS mlme_start(TI_HANDLE hMlme, TI_UINT8 connectionType)
{
	EConnType econnectionType = (EConnType)connectionType;
	mlme_t		*pMlme = (mlme_t*)hMlme;
    paramInfo_t param;

	if (pMlme == NULL)
    {
        return TI_NOK;
    }

	pMlme->assocInfo.disAssoc = TI_FALSE;

	param.paramType = RSN_EXT_AUTHENTICATION_MODE;
	rsn_getParam(pMlme->hRsn, &param);
	pMlme->legacyAuthType = AUTH_LEGACY_NONE;
    switch (econnectionType)
	{
	case CONN_TYPE_FIRST_CONN:
	case CONN_TYPE_ROAM:
        if (RSN_AUTH_SHARED_KEY == param.content.rsnExtAuthneticationMode)
        {
            pMlme->authInfo.authType = AUTH_LEGACY_SHARED_KEY;
        }
        else if (RSN_AUTH_AUTO_SWITCH == param.content.rsnExtAuthneticationMode)
        {
            pMlme->authInfo.authType = AUTH_LEGACY_SHARED_KEY;  /* the default of AutoSwitch mode is SHARED mode */
			pMlme->legacyAuthType = RSN_AUTH_AUTO_SWITCH;   /* legacyAuthType indecate that the auth mode is AutoSwitch */
			pMlme->bSharedFailed = TI_FALSE;
        }
        else
		{
			pMlme->authInfo.authType = AUTH_LEGACY_OPEN_SYSTEM;
		}
		break;
	default:
		pMlme->authInfo.authType = AUTH_LEGACY_OPEN_SYSTEM;
		break;
	}
	mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_START, pMlme);
	return TI_OK;
}

/** 
 * \fn     mlme_sendAuthRequest 
 * \brief  sends authentication request
 * 
 * The function builds and sends the authentication request according to the 
 * mlme parames
 * 
 * \param  pMlme - pointer to mlme_t
 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_authMsgBuild 
 */ 
TI_STATUS mlme_sendAuthRequest(mlme_t *pMlme)
{
	TI_STATUS status = TI_OK;
	TI_UINT8  authMsg[MAX_AUTH_MSG_LEN] = {0};
	TI_UINT16 authMsgLen = 0;

	legacyAuthType_e authType = pMlme->authInfo.authType;
	TI_UINT16 seq = 1, statusCode = 0;

	TI_UINT8* pExtraIes = NULL;
	TI_UINT8  uExtraIesLen = pMlme->extraIesLen;

	TRACE2(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_sendAuthRequest: retryCount = %d, maxCount = %d\n", pMlme->authInfo.retryCount, pMlme->authInfo.maxCount);
	if (pMlme->authInfo.retryCount >= pMlme->authInfo.maxCount)
	{
		return TI_NOK;
	}
	pMlme->authInfo.retryCount++;

	/* TODO - YD - 802.11r add the extra Ies in case of BSS FT authentication and RSN in the build auth and assoc*/
	if (pMlme->extraIesLen > 0)
	{
		pExtraIes = pMlme->extraIes;
	}

	mlme_authMsgBuild(pMlme, authType, seq, statusCode, pExtraIes, uExtraIesLen, (TI_UINT8*)authMsg, &authMsgLen);

	status =  mlmeBuilder_sendFrame(pMlme, AUTH, authMsg, authMsgLen, 0);

	if (status != TI_OK)
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_sendAuthRequest: failed to send auth request\n");
    }
    return status;
}

/** 
 * \fn     mlme_sendSharedRequest 
 * \brief  sends authentication request
 * 
 * The function builds and sends the second authentication request in shared key authentication
 * according to the * mlme parames
 * 
 * \param  pMlme - pointer to mlme_t
 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_authMsgBuild 
 */ 
TI_STATUS mlme_sendSharedRequest(mlme_t *pMlme)
{
	TI_UINT8  authMsg[MAX_AUTH_MSG_LEN];
	TI_UINT16 authMsgLen = 0;

	legacyAuthType_e authType = AUTH_LEGACY_SHARED_KEY;
	TI_UINT16 seq = 3, statusCode = 0;

	TI_UINT8* pChallenge = pMlme->authInfo.authData.pChalange;
	TI_UINT8  challengeLen = pMlme->authInfo.authData.challangeLen;

	dot11_CHALLENGE_t	dot11Challenge;

	TRACE2(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_sendAuthRequest: retryCount = %d, maxCount = %d\n", pMlme->authInfo.retryCount, pMlme->authInfo.maxCount);
	if (pMlme->authInfo.retryCount >= pMlme->authInfo.maxCount)
	{
		return TI_NOK;
	}
	pMlme->authInfo.retryCount++;

	/*If the shared key auth was received stop the timout timer */
	tmr_StopTimer(pMlme->hMlmeTimer);

	if (pChallenge != NULL)
	{
/*		pDot11Challenge = (dot11_CHALLENGE_t*)&authMsg[len]; */

		dot11Challenge.hdr[0] = CHALLANGE_TEXT_IE_ID;
		dot11Challenge.hdr[1] = challengeLen;
		os_memoryCopy(pMlme->hOs, (void *)dot11Challenge.text, pChallenge, challengeLen);
	}

	mlme_authMsgBuild(pMlme, authType, seq, statusCode, (TI_UINT8*)&dot11Challenge, challengeLen + 2, (TI_UINT8*)authMsg, &authMsgLen);

	report_PrintDump((TI_UINT8*)authMsg, authMsgLen);
	return  mlmeBuilder_sendFrame(pMlme, AUTH, authMsg, authMsgLen, 1);
}

/** 
 * \fn     mlme_authMsgBuild 
 * \brief  builds authentication request
 * 
 * The function builds the authentication request according to the given params
 * 
 * \param  pMlme - pointer to mlme_t
 * \param  authType - auth type (OPEN or SHARED)
 * \param  seq - message sequence number
 * \param  statusCode - 0 in case of success
 * \param  extraIes - pointer extra IEs buffer
 * \param  extraIesLen - extra IEs buffer length
 * \param  authMsg - <output> pointer of the built auth message
 * \param  authMsgLen - <output> length of the built auth message
 * 
 * \return TI_OK if auth built successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_sendAuthRequest, mlme_sendSharedRequest
 */ 
TI_STATUS mlme_authMsgBuild(mlme_t *pMlme, legacyAuthType_e authType, TI_UINT16 seq, TI_UINT16 statusCode,
							TI_UINT8* pExtraIes, TI_UINT8 uExtraIesLen,
							TI_UINT8 *authMsg, TI_UINT16 *authMsgLen)
{
	TI_UINT8				len;
	authMsg_t				*pAuthMsg;

	pAuthMsg = (authMsg_t*)authMsg;

	/* insert algorithm */
	pAuthMsg->authAlgo = ENDIAN_HANDLE_WORD((TI_UINT16)authType);

	/* insert sequense */
	pAuthMsg->seqNum = ENDIAN_HANDLE_WORD(seq);

	/* insert status code */
	pAuthMsg->status = ENDIAN_HANDLE_WORD(statusCode);

	len = sizeof(pAuthMsg->authAlgo) + sizeof(pAuthMsg->seqNum) + sizeof(pAuthMsg->status);

	if (pExtraIes != NULL)
	{
        os_memoryCopy(pMlme->hOs, &authMsg[len], pExtraIes, uExtraIesLen);
		len += uExtraIesLen;
	}

	*authMsgLen = len;
	return TI_OK;
}

/** 
 * \fn     mlme_sendAssocRequest 
 * \brief  sends association request
 * 
 * The function builds and sends the association request according to the 
 * mlme parames
 * 
 * \param  pMlme - pointer to mlme_t
 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_assocRequestMsgBuild 
 */ 
TI_STATUS mlme_sendAssocRequest(mlme_t *pMlme)
{
	TI_STATUS status = TI_OK;
	TI_UINT32           msgLen;
	dot11MgmtSubType_e  assocType=ASSOC_REQUEST;

	if ( pMlme->assocInfo.retryCount >= pMlme->assocInfo.maxCount)
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_sendAssocRequest: retry count - failed to send assoc request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
		return TI_NOK;
	}
	pMlme->assocInfo.retryCount++;

	if (pMlme->reAssoc)
	{
		assocType = RE_ASSOC_REQUEST;
	}

	status = mlme_assocRequestMsgBuild(pMlme, pMlme->assocMsg, &msgLen);
	if (status == TI_OK) {

        TRACE_INFO_HEX(pMlme->hReport,(TI_UINT8 *)(pMlme->assocMsg), msgLen);
		/* Save the association request message */
		mlme_saveAssocReqMessage(pMlme, pMlme->assocMsg, msgLen);
		status = mlmeBuilder_sendFrame(pMlme, assocType, pMlme->assocMsg, msgLen, 0);
	}

    return status;
}

/** 
 * \fn     mlme_assocRequestMsgBuild 
 * \brief  buils association request
 * 
 * The function builds the association request according to the given parames
 * 
 * \param  pCtx - pointer to mlme_t
 * \param  reqBuf - <output> pointer to built assoc request buffer
 * \param  reqLen - <output> length of built assoc request buffer
 * 
 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_sendAssocRequest 
 */ 
TI_STATUS mlme_assocRequestMsgBuild(mlme_t *pCtx, TI_UINT8* reqBuf, TI_UINT32* reqLen)
{
    TI_STATUS       status;
    TI_UINT8        *pRequest;
    TI_UINT32       len;
    paramInfo_t     param;
    TTwdParamInfo   tTwdParam;
    TI_UINT16       capabilities;
	TI_BOOL spectrumManagementEnabled;
	ECipherSuite    eCipherSuite = TWD_CIPHER_NONE; /* To be used for checking whether

                                                       AP supports HT rates and TKIP */
    pRequest = reqBuf;
    *reqLen = 0;


    /* insert capabilities */
    status = mlme_assocCapBuild(pCtx, &capabilities);
    if (status == TI_OK)
    {
         *(TI_UINT16*)pRequest = ENDIAN_HANDLE_WORD(capabilities);
    }
    else
	{
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build assoc Capa\n");
        return TI_NOK;
	}

    pRequest += sizeof(TI_UINT16);
    *reqLen += sizeof(TI_UINT16);

    /* insert listen interval */
    tTwdParam.paramType = TWD_LISTEN_INTERVAL_PARAM_ID;
    status =  TWD_GetParam (pCtx->hTWD, &tTwdParam);
    if (status == TI_OK)
    {
        *(TI_UINT16*)pRequest = ENDIAN_HANDLE_WORD((TI_UINT16)tTwdParam.content.halCtrlListenInterval);
    } else {
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to get listen interval\n");
        return TI_NOK;
    }

    pRequest += sizeof(TI_UINT16);
    *reqLen += sizeof(TI_UINT16);

	if (pCtx->reAssoc)
    {   /* Insert currentAPAddress element only in reassoc request*/
        param.paramType = SITE_MGR_PREV_SITE_BSSID_PARAM;
        status = siteMgr_getParam(pCtx->hSiteMgr, &param);
        if (status == TI_OK)
        {
            MAC_COPY (pRequest, param.content.siteMgrDesiredBSSID);
            TRACE6(pCtx->hReport, REPORT_SEVERITY_INFORMATION, "ASSOC_SM: ASSOC_REQ - prev AP = %x-%x-%x-%x-%x-%x\n", param.content.siteMgrDesiredBSSID[0], param.content.siteMgrDesiredBSSID[1], param.content.siteMgrDesiredBSSID[2], param.content.siteMgrDesiredBSSID[3], param.content.siteMgrDesiredBSSID[4], param.content.siteMgrDesiredBSSID[5]);


            pRequest += MAC_ADDR_LEN;
            *reqLen += MAC_ADDR_LEN;
        }
        else
        {
            TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: ASSOC_REQ - No prev AP \n");
            return status;

        }
    }

    /* insert SSID element */
    status = mlme_assocSSIDBuild(pCtx, pRequest, &len);
    if (status != TI_OK)
    {
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build SSID IE\n");
        return TI_NOK;
    }

    pRequest += len;
    *reqLen += len;

    /* insert Rates element */
    status = mlme_assocRatesBuild(pCtx, pRequest, &len);
    if (status != TI_OK)
    {
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build rates IE\n");
        return TI_NOK;
    }
    pRequest += len;
    *reqLen += len;

	/* Checking if the station supports Spectrum Management (802.11h) */
    param.paramType = REGULATORY_DOMAIN_MANAGEMENT_CAPABILITY_ENABLED_PARAM;
    status = regulatoryDomain_getParam(pCtx->hRegulatoryDomain,&param);
	spectrumManagementEnabled = param.content.spectrumManagementEnabled;

	/* Checking the selected AP capablities */
    param.paramType = SITE_MGR_SITE_CAPABILITY_PARAM;
    status =  siteMgr_getParam(pCtx->hSiteMgr,&param);
    if (status == TI_OK &&
    		spectrumManagementEnabled &&
    		param.content.siteMgrSiteCapability & (DOT11_SPECTRUM_MANAGEMENT != 0))
    {
         /* insert Power capability element */
         status = mlme_assocPowerCapabilityBuild(pCtx, pRequest, &len);
         if (status != TI_OK)
         {
			 TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build Power IE\n");
             return TI_NOK;
         }
         pRequest += len;
         *reqLen += len;
    }


#ifdef XCC_MODULE_INCLUDED
    status = rsn_getXCCExtendedInfoElement(pCtx->hRsn, pRequest, (TI_UINT8*)&len);
    if (status != TI_OK)
    {
        return TI_NOK;
    }
    pRequest += len;
    *reqLen += len;

    if (pCtx->reAssoc)
    {   /* insert CCKM information element only in reassoc */
        status = XCCMngr_getCckmInfoElement(pCtx->hXCCMngr, pRequest, (TI_UINT8*)&len);

        if (status != TI_OK)
        {
            return TI_NOK;
        }
        pRequest += len;
        *reqLen += len;
    }
    status = XCCMngr_getXCCVersionInfoElement(pCtx->hXCCMngr, pRequest, (TI_UINT8*)&len);
    if (status != TI_OK)
    {
        return TI_NOK;
    }
    pRequest += len;
    *reqLen += len;

    /* Insert Radio Mngt Capability IE */
    status = measurementMgr_radioMngtCapabilityBuild(pCtx->hMeasurementMgr, pRequest, (TI_UINT8*)&len);
    if (status != TI_OK)
    {
        return TI_NOK;
    }
    pRequest += len;
    *reqLen += len;
#endif

     /* Get Simple-Config state */
    param.paramType = SITE_MGR_SIMPLE_CONFIG_MODE;
    status = siteMgr_getParam(pCtx->hSiteMgr, &param);

   if (param.content.siteMgrWSCMode.WSCMode == TIWLN_SIMPLE_CONFIG_OFF)
   {
   /* insert RSN information elements */
    status = rsn_getInfoElement(pCtx->hRsn, pRequest, &len);

	if (status != TI_OK)
	{
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build RSN IE\n");
		return TI_NOK;
	}
	pRequest += len;
	*reqLen += len;
  }

  /* Privacy - Used later on HT */

    param.paramType = RSN_ENCRYPTION_STATUS_PARAM;

    status          = rsn_getParam(pCtx->hRsn, &param);



    if(status == TI_OK)

    {

        eCipherSuite = param.content.rsnEncryptionStatus;

    }

    /* Primary Site support HT ? */
    param.paramType = SITE_MGR_PRIMARY_SITE_HT_SUPPORT;
    siteMgr_getParam(pCtx->hSiteMgr, &param);


    /* Disallow TKIP with HT Rates: If this is the case - discard HT rates from Association Request */
    if((TI_TRUE == param.content.bPrimarySiteHtSupport) && 
              (eCipherSuite != TWD_CIPHER_TKIP) && 
              (eCipherSuite != TWD_CIPHER_WEP) && 
           (eCipherSuite != TWD_CIPHER_WEP104)      )
    {

        status = StaCap_GetHtCapabilitiesIe (pCtx->hStaCap, pRequest, &len);
    	if (status != TI_OK)
    	{
    		return TI_NOK;
    	}
    	pRequest += len;
    	*reqLen += len;
    }

	status = qosMngr_assocReqBuild(pCtx->hQosMngr,pRequest,&len);
	if (status != TI_OK)
	{
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build QoS IE\n");
		return TI_NOK;
	}
	pRequest += len;
	*reqLen += len;

	status = apConn_getVendorSpecificIE(pCtx->hApConn, pRequest, &len);
	if (status != TI_OK)
	{
		TRACE0(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build vendor IE\n");
		return TI_NOK;
	}
	pRequest += len;
	*reqLen += len;

    if (*reqLen>=MAX_ASSOC_MSG_LENGTH)
    {
		TRACE1(pCtx->hReport, REPORT_SEVERITY_ERROR, "mlme_assocRequestMsgBuild: failed to build, reqLen = %u\n", *reqLen);
        return TI_NOK;
    }



    return TI_OK;
}

/** 
 * \fn     mlme_assocCapBuild 
 * \brief  builds capabilties of assoc request 
 * 
 * builds capabilties of assoc request  according to the mlme params 
 * 
 * \param  pCtx - pointer to mlme_t
 * \param  cap - <output> pointer to the built capablities

 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_assocRequestMsgBuild 
 */ 
TI_STATUS mlme_assocCapBuild(mlme_t *pCtx, TI_UINT16 *cap)
{
    paramInfo_t         param;
    TI_STATUS           status;
    EDot11Mode          mode;
    TI_UINT32           rateSuppMask, rateBasicMask;
    TI_UINT8            ratesBuf[DOT11_MAX_SUPPORTED_RATES];
    TI_UINT32           len = 0, ofdmIndex = 0;
    TI_BOOL             b11nEnable, bWmeEnable;
    ECipherSuite        rsnEncryption;

    *cap = 0;

    /* Bss type */
    param.paramType = CTRL_DATA_CURRENT_BSS_TYPE_PARAM;
    status =  ctrlData_getParam(pCtx->hCtrlData, &param);
    if (status == TI_OK)
    {
        if (param.content.ctrlDataCurrentBssType == BSS_INFRASTRUCTURE)
        {
            *cap |= DOT11_CAPS_ESS;
        } else {
            *cap |= DOT11_CAPS_IBSS;
        }
    } else {
        return TI_NOK;
    }

    /* Privacy */
    param.paramType = RSN_ENCRYPTION_STATUS_PARAM;
    status =  rsn_getParam(pCtx->hRsn, &param);
    rsnEncryption = param.content.rsnEncryptionStatus;
    if (status == TI_OK)
    {
        if (param.content.rsnEncryptionStatus != TWD_CIPHER_NONE)
        {
            *cap |= DOT11_CAPS_PRIVACY;
        }
    } else {
        return TI_NOK;
    }

    /* Preamble */
    param.paramType = SITE_MGR_DESIRED_PREAMBLE_TYPE_PARAM;
    status =  siteMgr_getParam(pCtx->hSiteMgr, &param);
    if (status == TI_OK)
    {
        if (param.content.siteMgrCurrentPreambleType == PREAMBLE_SHORT)
            *cap |= DOT11_CAPS_SHORT_PREAMBLE;
    } else {
        return TI_NOK;
    }

    /* Pbcc */
    param.paramType = SITE_MGR_CURRENT_RATE_PAIR_PARAM;
    status =  siteMgr_getParam(pCtx->hSiteMgr, &param);
    if (status == TI_OK)
    {
        if(param.content.siteMgrCurrentRateMask.supportedRateMask & DRV_RATE_MASK_22_PBCC)
            *cap |= DOT11_CAPS_PBCC;
    } else {
        return TI_NOK;
    }

    /* Checking if the station supports Spectrum Management (802.11h) */
    param.paramType = REGULATORY_DOMAIN_MANAGEMENT_CAPABILITY_ENABLED_PARAM;
    status =  regulatoryDomain_getParam(pCtx->hRegulatoryDomain, &param);
    if (status == TI_OK )
    {
        if( param.content.spectrumManagementEnabled)
            *cap |= DOT11_SPECTRUM_MANAGEMENT;
    }
    else
    {
        return TI_NOK;
    }

    /* slot time */
    param.paramType = SITE_MGR_OPERATIONAL_MODE_PARAM;
    status = siteMgr_getParam(pCtx->hSiteMgr, &param);
    if(status == TI_OK)
    {
        mode = param.content.siteMgrDot11OperationalMode;
    }
    else
        return TI_NOK;

    if(mode == DOT11_G_MODE)
    {
        /* new requirement: the short slot time should be set only
           if the AP's modulation is OFDM (highest rate) */

        /* get Rates */
        param.paramType = SITE_MGR_CURRENT_RATE_PAIR_PARAM;
        status =  siteMgr_getParam(pCtx->hSiteMgr, &param);
        if (status == TI_OK)
        {
            rateBasicMask = param.content.siteMgrCurrentRateMask.basicRateMask;
            rateSuppMask  = param.content.siteMgrCurrentRateMask.supportedRateMask;
        } else {
            return TI_NOK;
        }

        /* convert the bit map to the rates array */
        rate_DrvBitmapToNetStr (rateSuppMask, rateBasicMask, ratesBuf, &len, &ofdmIndex);

        if(ofdmIndex < len)
            *cap |= DOT11_CAPS_SHORT_SLOT_TIME;

/*
        param.paramType = SITE_MGR_CURRENT_MODULATION_TYPE_PARAM;
        status = siteMgr_getParam(pCtx->hSiteMgr, &param);
        if(param.content.siteMgrCurrentModulationType == DRV_MODULATION_OFDM)
            *cap |= DOT11_CAPS_SHORT_SLOT_TIME;
*/
    }

    /* if chiper = TKIP/WEP then 11n is not used */
    if (rsnEncryption != TWD_CIPHER_TKIP &&
        rsnEncryption != TWD_CIPHER_WEP &&
        rsnEncryption != TWD_CIPHER_WEP104)
    {
		/* Primary Site support HT ? */
		param.paramType = SITE_MGR_PRIMARY_SITE_HT_SUPPORT;
		siteMgr_getParam(pCtx->hSiteMgr, &param);

		if (param.content.bPrimarySiteHtSupport == TI_TRUE)
		{
			/* Immediate Block Ack subfield - (is WME on?) AND (is HT Enable?) */
			/* verify 11n_Enable and Chip type */
			StaCap_IsHtEnable (pCtx->hStaCap, &b11nEnable);
			/* verify that WME flag enable */
			qosMngr_GetWmeEnableFlag (pCtx->hQosMngr, &bWmeEnable);

			if ((b11nEnable != TI_FALSE) && (bWmeEnable != TI_FALSE))
			{
				*cap |= DOT11_CAPS_IMMEDIATE_BA;
			}
		}
    }

    return TI_OK;
}

/** 
 * \fn     mlme_assocSSIDBuild 
 * \brief  builds the SSID IE of assoc request 
 * 
 * builds the SSID IE of assoc request according to the mlme params 
 * 
 * \param  pCtx - pointer to mlme_t
 * \param  pSSID - <output> pointer to the built SSID buffer
 * \param  ssidLen - <output> length of the built SSID buffer

 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_assocRequestMsgBuild 
 */ 
TI_STATUS mlme_assocSSIDBuild(mlme_t *pCtx, TI_UINT8 *pSSID, TI_UINT32 *ssidLen)
{
    paramInfo_t         param;
    TI_STATUS               status;
    dot11_SSID_t        *pDot11Ssid;

    pDot11Ssid = (dot11_SSID_t*)pSSID;
    /* set SSID element id */
    pDot11Ssid->hdr[0] = SSID_IE_ID;

    /* get SSID */
    param.paramType = SME_DESIRED_SSID_ACT_PARAM;
    status =  sme_GetParam(pCtx->hSme, &param);
    if (status != TI_OK)
    {
        return status;
    }

    /* check for ANY ssid */
    if (param.content.smeDesiredSSID.len != 0)
    {
        pDot11Ssid->hdr[1] = param.content.smeDesiredSSID.len;
        os_memoryCopy(pCtx->hOs,
                      (void *)pDot11Ssid->serviceSetId,
                      (void *)param.content.smeDesiredSSID.str,
                      param.content.smeDesiredSSID.len);

    } else {
        /* if ANY ssid is configured, use the current SSID */
        param.paramType = SITE_MGR_CURRENT_SSID_PARAM;
        status =  siteMgr_getParam(pCtx->hSiteMgr, &param);
        if (status != TI_OK)
        {
            return status;
        }
        pDot11Ssid->hdr[1] = param.content.siteMgrCurrentSSID.len;
        os_memoryCopy(pCtx->hOs,
                      (void *)pDot11Ssid->serviceSetId,
                      (void *)param.content.siteMgrCurrentSSID.str,
                      param.content.siteMgrCurrentSSID.len);

    }

    *ssidLen = pDot11Ssid->hdr[1] + sizeof(dot11_eleHdr_t);

    return TI_OK;
}

/** 
 * \fn     mlme_assocRatesBuild 
 * \brief  builds the rates IE of assoc request 
 * 
 * builds the rates IE of assoc request according to the mlme params 
 * 
 * \param  pCtx - pointer to mlme_t
 * \param  pRates - <output> pointer to the built rates buffer
 * \param  ratesLen - <output> length of the built rates buffer

 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_assocRequestMsgBuild 
 */ 
TI_STATUS mlme_assocRatesBuild(mlme_t *pCtx, TI_UINT8 *pRates, TI_UINT32 *ratesLen)
{
    paramInfo_t         param;
    TI_STATUS           status;
    TI_UINT32           rateSuppMask, rateBasicMask;
    dot11_RATES_t       *pDot11Rates;
    TI_UINT32           len = 0, ofdmIndex = 0;
    TI_UINT8            ratesBuf[DOT11_MAX_SUPPORTED_RATES];
    EDot11Mode          mode;
    TI_UINT32           suppRatesLen, extSuppRatesLen, i;
    pDot11Rates = (dot11_RATES_t*)pRates;


    /* get Rates */
    param.paramType = SITE_MGR_CURRENT_RATE_PAIR_PARAM;
    status =  siteMgr_getParam(pCtx->hSiteMgr, &param);
    if (status == TI_OK)
    {
        rateBasicMask = param.content.siteMgrCurrentRateMask.basicRateMask;
        rateSuppMask  = param.content.siteMgrCurrentRateMask.supportedRateMask;
    }
    else
    {
        return TI_NOK;
    }

    /* get operational mode */
    param.paramType = SITE_MGR_OPERATIONAL_MODE_PARAM;
    status = siteMgr_getParam(pCtx->hSiteMgr, &param);
    if(status == TI_OK)
        mode = param.content.siteMgrDot11OperationalMode;
    else
        return TI_NOK;

    /* convert the bit map to the rates array */
    /* remove MCS rates from Extended Supported Rates IE */
    rateSuppMask &= ~(DRV_RATE_MASK_MCS_0_OFDM |
                      DRV_RATE_MASK_MCS_1_OFDM |
                      DRV_RATE_MASK_MCS_2_OFDM |
                      DRV_RATE_MASK_MCS_3_OFDM |
                      DRV_RATE_MASK_MCS_4_OFDM |
                      DRV_RATE_MASK_MCS_5_OFDM |
                      DRV_RATE_MASK_MCS_6_OFDM |
                      DRV_RATE_MASK_MCS_7_OFDM );

    rate_DrvBitmapToNetStr (rateSuppMask, rateBasicMask, ratesBuf, &len, &ofdmIndex);

    if(mode != DOT11_G_MODE || ofdmIndex == len )
    {
        pDot11Rates->hdr[0] = SUPPORTED_RATES_IE_ID;
        pDot11Rates->hdr[1] = len;
        os_memoryCopy(NULL, (void *)pDot11Rates->rates, ratesBuf, len);
        *ratesLen = pDot11Rates->hdr[1] + sizeof(dot11_eleHdr_t);
    }
    else
    {
        /* fill in the supported rates */
        pDot11Rates->hdr[0] = SUPPORTED_RATES_IE_ID;
        pDot11Rates->hdr[1] = ofdmIndex;
        os_memoryCopy(NULL, (void *)pDot11Rates->rates, ratesBuf, pDot11Rates->hdr[1]);
        suppRatesLen = pDot11Rates->hdr[1] + sizeof(dot11_eleHdr_t);
        /* fill in the extended supported rates */
        pDot11Rates = (dot11_RATES_t*)(pRates + suppRatesLen);
        pDot11Rates->hdr[0] = EXT_SUPPORTED_RATES_IE_ID;
        pDot11Rates->hdr[1] = len - ofdmIndex;
        os_memoryCopy(NULL, (void *)pDot11Rates->rates, &ratesBuf[ofdmIndex], pDot11Rates->hdr[1]);
        extSuppRatesLen = pDot11Rates->hdr[1] + sizeof(dot11_eleHdr_t);
        *ratesLen = suppRatesLen + extSuppRatesLen;
    }

    TRACE3(pCtx->hReport, REPORT_SEVERITY_INFORMATION, "ASSOC_SM: ASSOC_REQ - bitmapSupp= 0x%X,bitMapBasic = 0x%X, len = %d\n", rateSuppMask,rateBasicMask,len);
    for(i=0; i<len; i++)
    {
        TRACE2(pCtx->hReport, REPORT_SEVERITY_INFORMATION, "ASSOC_SM: ASSOC_REQ - ratesBuf[%d] = 0x%X\n", i, ratesBuf[i]);
    }

    return TI_OK;
}

/** 
 * \fn     mlme_assocPowerCapabilityBuild 
 * \brief  builds the Power Capability IE of assoc request 
 * 
 * builds the Power Capability IE of assoc request according to the mlme params 
 * 
 * \param  pCtx - pointer to mlme_t
 * \param  pPowerCapability - <output> pointer to the built Power Capability IE buffer
 * \param  powerCapabilityLen - <output> length of the built Power Capability IE buffer

 * \return TI_OK if auth send successfully
 *         TI_NOK otherwise
 * 
 * \sa     mlme_assocRequestMsgBuild 
 */ 
TI_STATUS mlme_assocPowerCapabilityBuild(mlme_t *pCtx, TI_UINT8 *pPowerCapability, TI_UINT32 *powerCapabilityLen)
{
    paramInfo_t         param;
    TI_STATUS               status;
    dot11_CAPABILITY_t      *pDot11PowerCapability;

    pDot11PowerCapability = (dot11_CAPABILITY_t*)pPowerCapability;

    /* set Power Capability element id */
    pDot11PowerCapability->hdr[0] = DOT11_CAPABILITY_ELE_ID;
    pDot11PowerCapability->hdr[1] = DOT11_CAPABILITY_ELE_LEN;

    /* get power capability */
    param.paramType = REGULATORY_DOMAIN_POWER_CAPABILITY_PARAM;
    status =  regulatoryDomain_getParam(pCtx->hRegulatoryDomain, &param);

    if (status == TI_OK)
    {
        pDot11PowerCapability->minTxPower = param.content.powerCapability.minTxPower;
        pDot11PowerCapability->maxTxPower = param.content.powerCapability.maxTxPower;
        *powerCapabilityLen = pDot11PowerCapability->hdr[1] + sizeof(dot11_eleHdr_t);
    }
    else
        *powerCapabilityLen = 0;

    return TI_OK;
}



TI_STATUS mlme_saveAssocReqMessage(TI_HANDLE hMlme, TI_UINT8 *pAssocBuffer, TI_UINT32 length)
{
	mlme_t *pMlme = (mlme_t*)hMlme;

    if ((pMlme==NULL) || (pAssocBuffer==NULL) || (length>=MAX_ASSOC_MSG_LENGTH))
    {
        return TI_NOK;
    }

	os_memoryCopy(pMlme->hOs, pMlme->assocInfo.assocReqBuffer, pAssocBuffer, length);
    pMlme->assocInfo.assocReqLen = length;

    TRACE1(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "assoc_saveAssocReqMessage: length=%ld \n",length);
    return TI_OK;
}

TI_STATUS mlme_saveAssocRespMessage(TI_HANDLE hMlme, TI_UINT8 *pAssocBuffer, TI_UINT32 length)
{
	mlme_t *pMlme = (mlme_t*)hMlme;

    if ((pMlme==NULL) || (pAssocBuffer==NULL) || (length>=MAX_ASSOC_MSG_LENGTH))
    {
        return TI_NOK;
    }

    os_memoryCopy(pMlme->hOs, pMlme->assocInfo.assocRespBuffer, pAssocBuffer, length);
    pMlme->assocInfo.assocRespLen = length;

    TRACE1(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "assoc_saveAssocRespMessage: length=%ld \n",length);
    return TI_OK;
}





/**
*
* mlme_Stop - Stop event for the MLME SM
*
* \b Description:
* Stop event for the MLME SM
*
* \b ARGS:
*  I   - hMlme - MLME SM context  \n
*
* \b RETURNS:
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa mlme_Start, mlme_Recv
*/
TI_STATUS mlme_stop(TI_HANDLE hMlme, DisconnectType_e disConnType, mgmtStatus_e reason)
{
    mlme_t      *pMlme;

    pMlme = (mlme_t*)hMlme;

	mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_STOP, pMlme);

    if (pMlme->authInfo.authType == AUTH_LEGACY_NONE)
	{
        return TI_NOK;
	}

    pMlme->disConnType = disConnType;
    pMlme->disConnReason = reason;

    return TI_OK;
}

/**
*
* mlme_authRecv - Recive an authentication message from the AP
*
* \b Description:
* Parse a message form the AP and perform the appropriate event.
*
* \b ARGS:
*  I   - hMlme - mlme handler  \n
*
* \b RETURNS:
*  TI_OK if successful, TI_NOK otherwise.
*
*/
TI_STATUS mlme_authRecv(TI_HANDLE hMlme, mlmeFrameInfo_t *pFrame)
{
	mlme_t	*pMlme = (mlme_t*)hMlme;
	if ((pMlme == NULL) || (pFrame->subType != AUTH) || (pMlme->authInfo.authType == AUTH_LEGACY_NONE))
		return TI_NOK;

	if (pFrame->content.auth.authAlgo != pMlme->authInfo.authType)
	{
		return TI_NOK;
	}

	if (pFrame->content.auth.status != STATUS_SUCCESSFUL)
	{
		pMlme->authInfo.authRejectCount++;

		pMlme->mlmeData.mgmtStatus = STATUS_AUTH_REJECT;
		pMlme->mlmeData.uStatusCode = TI_OK;

		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
		return TI_OK;
	}

	TRACE1(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_authRecv authAlgo = %d\n", pFrame->content.auth.authAlgo);
	if (pFrame->content.auth.authAlgo == AUTH_LEGACY_SHARED_KEY)
	{
		pMlme->authInfo.authData.challangeLen = pFrame->content.auth.pChallenge->hdr[1];
		pMlme->authInfo.authData.pChalange = pFrame->content.auth.pChallenge->text;
		switch (pFrame->content.auth.seqNum)
		{
		case 2:
			mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_SHARED_RECV, pMlme);
			break;
		case 4:
			mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_SUCCESS, pMlme);
			break;
		default:
			TRACE1(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_authRecv: authAlgo = AUTH_LEGACY_SHARED_KEY, unexpected seqNum = %d\n",
				   pFrame->content.auth.seqNum);

			return TI_NOK;
		}
	}
	else
	{
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_SUCCESS, pMlme);
	}
	return TI_OK;
}


TI_STATUS mlme_assocRecv(TI_HANDLE hMlme, mlmeFrameInfo_t *pFrame)
{
    TI_STATUS       status = TI_OK;
	mlme_t			*pMlme = (mlme_t*)hMlme;
    assoc_t         *pAssoc;
    TTwdParamInfo   tTwdParam;
    TI_UINT16           rspStatus;

    if (pMlme == NULL)
    {
        return TI_NOK;
    }

	pAssoc = &(pMlme->assocInfo);

    /* ensure that the SM is waiting for assoc response */
    if(pAssoc->currentState != MLME_SM_STATE_ASSOC_WAIT)
        return TI_OK;


    if ((pFrame->subType != ASSOC_RESPONSE) && (pFrame->subType != RE_ASSOC_RESPONSE))
    {
        return TI_NOK;
    }

    /* check response status */
    rspStatus  = pFrame->content.assocRsp.status;

    if (rspStatus == 0)
    {
        dot11_RSN_t    *pRsnIe;
        TI_UINT8       curRsnData[255];
        TI_UINT8       length = 0;


        TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "MLME_SM: DEBUG Success associating to AP \n");

        /* set AID to HAL */
        tTwdParam.paramType = TWD_AID_PARAM_ID;
        tTwdParam.content.halCtrlAid  = pFrame->content.assocRsp.aid;
        TWD_SetParam (pMlme->hTWD, &tTwdParam);


        /* Get the RSN IE data */
        pRsnIe = pFrame->content.assocRsp.pRsnIe;
        while (length < pFrame->content.assocRsp.rsnIeLen && (pFrame->content.assocRsp.rsnIeLen < 255))
        {
            curRsnData[0+length] = pRsnIe->hdr[0];
            curRsnData[1+length] = pRsnIe->hdr[1];
            os_memoryCopy(pMlme->hOs, &curRsnData[2+length], (void *)pRsnIe->rsnIeData, pRsnIe->hdr[1]);
            length += pRsnIe->hdr[1] + 2;
            pRsnIe += 1;
        }


        /* update siteMgr with capabilities and whether we are connected to Cisco AP */
        siteMgr_assocReport(pMlme->hSiteMgr,
                            pFrame->content.assocRsp.capabilities, pFrame->content.assocRsp.ciscoIEPresent);

        /* update QoS Manager - it the QOS active protocol is NONE, or no WME IE present, it will return TI_OK */
        /* if configured by AP, update MSDU lifetime */
        status = qosMngr_setSite(pMlme->hQosMngr, &pFrame->content.assocRsp);

        if(status != TI_OK)
        {
            TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "MLME_SM: DEBUG - Association failed : qosMngr_setSite error \n");
            /* in case we wanted to work with qosAP and failed to connect to qos AP we want to reassociated again
               to another one */
        	pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        	pMlme->mlmeData.uStatusCode = status;
            mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
        }
        else
        {
            mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_SUCCESS, pMlme);
        }
    }
    else
    {
        pAssoc->assocRejectCount++;

        /* If there was attempt to renegotiate voice settings, update QoS Manager */
        qosMngr_checkTspecRenegResults(pMlme->hQosMngr, &pFrame->content.assocRsp);

        /* check failure reason */
        switch (rspStatus)
        {
        case 0:
            break;
        case 1:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Unspecified error \n");
            break;
        case 10:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Cannot support all requested capabilities in the Capability Information field \n");
            break;
        case 11:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Reassociation denied due to inability to confirm that association exists \n");
            break;
        case 12:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Association denied due to reason outside the scope of this standard \n");
            rsn_reportAuthFailure(pMlme->hRsn, RSN_AUTH_STATUS_INVALID_TYPE);
            break;
        case 13:
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Association denied due to wrong authentication algorithm \n");
            rsn_reportAuthFailure(pMlme->hRsn, RSN_AUTH_STATUS_INVALID_TYPE);
            break;
        case 17:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Association denied because AP is unable to handle additional associated stations \n");
            break;
        case 18:
            /* print debug message */
            TRACE0(pMlme->hReport, REPORT_SEVERITY_SM, "ASSOC_SM: DEBUG - Association denied: Association denied due to requesting station not supporting all of the data rates in the BSSBasicRateSet parameter \n");
            break;
        default:
            /* print error message on wrong error code for association response */
            TRACE1(pMlme->hReport, REPORT_SEVERITY_ERROR, "ASSOC_SM: ERROR - Association denied: error code (%d) irrelevant \n", rspStatus);
            break;
        }

       pMlme->mlmeData.mgmtStatus = STATUS_ASSOC_REJECT;
       pMlme->mlmeData.uStatusCode = rspStatus;
       mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
    }

    return status;
}
