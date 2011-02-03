/*
 * mlmeSm.c
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

/** \file mlmeSM.c
 *  \brief 802.11 MLME SM source
 *
 *  \see mlmeSM.h
 */


/***************************************************************************/
/*                                                                         */
/*      MODULE: mlmeSM.c                                                   */
/*    PURPOSE:  802.11 MLME SM source                                      */
/*                                                                         */
/***************************************************************************/

#define __FILE_ID__  FILE_ID_69
#include "osApi.h"
#include "paramOut.h"
#include "report.h"
#include "mlmeApi.h"
#include "mlmeBuilder.h"
#include "mlmeSm.h"
#include "mlme.h"
#include "connApi.h"
#include "DrvMainModules.h"
#include "timer.h"


#ifdef TI_DBG
#include "siteMgrApi.h"
#endif
/* Constants */

/* Enumerations */

/* Typedefs */

/* Structures */

/* External data definitions */

/* External functions definitions */

/* Global variables */

/* Local function prototypes */

/* functions */
/* state machine functions */
static void mlme_smStartIdle(TI_HANDLE hMlme);

static void mlme_smStop(TI_HANDLE hMlme);
static void mlme_smFail(TI_HANDLE hMlme);

static void mlme_smAuthSuccess(TI_HANDLE hMlme);
static void mlme_smAuthWaitToShared(TI_HANDLE hMlme);
static void mlme_smAsoccSuccess(TI_HANDLE hMlme);

static void mlme_smAuthTimeout(TI_HANDLE hMlme);
static void mlme_smSharedTimeout(TI_HANDLE hMlme);
static void mlme_smAssocTimeout(TI_HANDLE hMlme);

static void mlme_smActionUnexpected(TI_HANDLE hMlme);

TGenSM_actionCell mlmeMatrix[ MLME_SM_NUMBER_OF_STATES ][ MLME_SM_NUMBER_OF_EVENTS ] =
{
	/* MLME_SM_STATE_IDLE         */
	{
		{MLME_SM_STATE_AUTH_WAIT, mlme_smStartIdle},	/* MLME_SM_EVENT_START          */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_STOP           */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_FAIL           */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SUCCESS        */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SHARED_RECV    */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_AUTH_TIMEOUT   */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SHARED_TIMEOUT */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected}	/* MLME_SM_EVENT_ASSOC_TIMEOUT  */
	},

	/* MLME_SM_STATE_AUTH_WAIT    */
	{
		{MLME_SM_STATE_AUTH_WAIT, mlme_smActionUnexpected},		/* MLME_SM_EVENT_START          */
		{MLME_SM_STATE_IDLE, mlme_smStop},						/* MLME_SM_EVENT_STOP           */
		{MLME_SM_STATE_IDLE, mlme_smFail},						/* MLME_SM_EVENT_FAIL           */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smAuthSuccess},			/* MLME_SM_EVENT_SUCCESS        */
		{MLME_SM_STATE_SHARED_WAIT, mlme_smAuthWaitToShared},	/* MLME_SM_EVENT_SHARED_RECV    */
		{MLME_SM_STATE_AUTH_WAIT, mlme_smAuthTimeout},			/* MLME_SM_EVENT_AUTH_TIMEOUT   */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected},			/* MLME_SM_EVENT_SHARED_TIMEOUT */
		{MLME_SM_STATE_IDLE, mlme_smActionUnexpected}			/* MLME_SM_EVENT_ASSOC_TIMEOUT  */
	},

	/* MLME_SM_STATE_SHARED_WAIT  */
	{
		{MLME_SM_STATE_SHARED_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_START          */
		{MLME_SM_STATE_IDLE, mlme_smStop},						/* MLME_SM_EVENT_STOP           */
		{MLME_SM_STATE_IDLE, mlme_smFail},						/* MLME_SM_EVENT_FAIL           */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smAuthSuccess},			/* MLME_SM_EVENT_SUCCESS        */
		{MLME_SM_STATE_SHARED_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SHARED_RECV    */
		{MLME_SM_STATE_SHARED_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_AUTH_TIMEOUT   */
		{MLME_SM_STATE_SHARED_WAIT, mlme_smSharedTimeout},		/* MLME_SM_EVENT_SHARED_TIMEOUT */
		{MLME_SM_STATE_SHARED_WAIT, mlme_smActionUnexpected}	/* MLME_SM_EVENT_ASSOC_TIMEOUT  */
	},

	/* MLME_SM_STATE_ASSOC_WAIT   */
	{
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_START          */
		{MLME_SM_STATE_IDLE, mlme_smStop},						/* MLME_SM_EVENT_STOP           */
		{MLME_SM_STATE_IDLE, mlme_smFail},						/* MLME_SM_EVENT_FAIL           */
		{MLME_SM_STATE_IDLE, mlme_smAsoccSuccess},				/* MLME_SM_EVENT_SUCCESS        */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SHARED_RECV    */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_AUTH_TIMEOUT   */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smActionUnexpected},	/* MLME_SM_EVENT_SHARED_TIMEOUT */
		{MLME_SM_STATE_ASSOC_WAIT, mlme_smAssocTimeout}			/* MLME_SM_EVENT_ASSOC_TIMEOUT  */
	}
};

TI_INT8*  uMlmeStateDescription[] = 
    {
		"IDLE",
		"AUTH_WAIT",
		"SHARED_WAIT",
		"ASSOC_WAIT"
    };

TI_INT8*  uMlmeEventDescription[] = 
    {
		"START",         
		"STOP",       
		"FAIL",      
		"SUCCESS",  
		"SHARED_RECV",
		"AUTH_TIMEOUT",
		"SHARED_TIMEOUT",
		"ASSOC_TIMEOUT",
    };


void mlme_smEvent(TI_HANDLE hGenSm, TI_UINT32 uEvent, void* pData)
{
    mlme_t *pMlme = (mlme_t*)pData;
    TGenSM    *pGenSM = (TGenSM*)hGenSm;

    TRACE2(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_smEvent: Current State = %d, sending event %d\n", (pGenSM->uCurrentState), (uEvent));
    genSM_Event(pGenSM, uEvent, pData);
}

/**************************************************************************************/
/* state machine functions */
/** 
 * \fn     mlme_smStartIdle 
 * \brief  change state from IDLE 
 * 
 * The function is called upon state change from IDLE to AUTH_WAIT
 * sends the auth request and starts the auth rsp timeout
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smStartIdle(TI_HANDLE hMlme)
{
    TI_STATUS       status = TI_OK;
	mlme_t *pMlme = (mlme_t*)hMlme;

    tmr_StopTimer(pMlme->hMlmeTimer);
    status = mlme_sendAuthRequest(pMlme);

    if (TI_OK == status)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_authTmrExpiry, pMlme, pMlme->authInfo.timeout, TI_FALSE);
	}
	else
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smStartIdle: failed to send auth request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
        mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
    }
}

/** 
 * \fn     mlme_smAuthTimeout 
 * \brief  called upon authentication timeout 
 * 
 * The function sends another auth request after timeout and restart the
 * auth request time
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smAuthTimeout(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	TI_STATUS status = mlme_sendAuthRequest(pMlme);
    
	if (TI_OK == status)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_authTmrExpiry, pMlme, pMlme->authInfo.timeout, TI_FALSE);
	}
	else
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smAuthTimeout: failed to send auth request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
	}
}

/** 
 * \fn     mlme_smAuthWaitToShared 
 * \brief  change state function between AUTH_WAIT and SHARED_WAIT
 * 
 * The function is called upon state change from AUTH_WAIT to SHARED_WAIT
 * sends the second auth request and starts the auth resp timer
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smAuthWaitToShared(TI_HANDLE hMlme)
{
	TI_STATUS status;
	mlme_t *pMlme = (mlme_t*)hMlme;

	tmr_StopTimer(pMlme->hMlmeTimer);
	pMlme->authInfo.retryCount = 0;

	status = mlme_sendSharedRequest(pMlme);

	if (status == TI_OK)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_sharedTmrExpiry, pMlme, pMlme->authInfo.timeout, TI_FALSE);
	}
	else
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smAuthWaitToShared: failed to send auth request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
	}
}

/** 
 * \fn     mlme_smSharedTimeout 
 * \brief  called upon authentication timeout of the second auth req in shared key auth
 * 
 * The function sends another auth request after timeout of the second auth req in shared key auth
 * and restart the auth request time
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smSharedTimeout(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	TI_STATUS status = mlme_sendSharedRequest(pMlme);


	if (TI_OK == status)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_sharedTmrExpiry, pMlme, pMlme->authInfo.timeout, TI_FALSE);
	}
	else
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smSharedTimeout: failed to send auth request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
	}
}

/** 
 * \fn     mlme_smAuthSuccess 
 * \brief  change state function between AUTH_WAIT or SHARED_WAIT to ASSOC_WAIT
 * 
 * The function is called upon state change from AUTH_WAIT or SHARED_WAIT to ASSOC_WAIT
 * sends the association request and starts the assoc resp timer
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smAuthSuccess(TI_HANDLE hMlme)
{
	TI_STATUS status;
	mlme_t *pMlme = (mlme_t*)hMlme;

    TRACE0(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_smAuthSuccess\n");

	tmr_StopTimer(pMlme->hMlmeTimer);
	pMlme->extraIesLen = 0; /* reset the extra Ies since it may be used for the association */

	/* TODO ?802.11r - send the information to the wpa_supplicant in case of BSS FT */
	status = mlme_sendAssocRequest(pMlme);

	if (TI_OK == status)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_assocTmrExpiry, pMlme, pMlme->assocInfo.timeout, TI_FALSE);
		pMlme->assocInfo.currentState = MLME_SM_STATE_ASSOC_WAIT;
	}
	else 
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smAuthSuccess: failed to send assoc request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
	}
}

/** 
 * \fn     mlme_smAssocTimeout 
 * \brief  called upon association timeout
 * 
 * The function sends another assoc request after timeout of the assoc resp
 * and restart the assoc request timer
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smAssocTimeout(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	TI_STATUS status = mlme_sendAssocRequest(pMlme);

	if (TI_OK == status)
	{
		tmr_StartTimer(pMlme->hMlmeTimer, mlme_assocTmrExpiry, pMlme, pMlme->authInfo.timeout, TI_FALSE);
	}
	else
	{
		TRACE0(pMlme->hReport, REPORT_SEVERITY_ERROR, "mlme_smAssocTimeout: failed to send assoc request\n");
        pMlme->mlmeData.mgmtStatus = STATUS_UNSPECIFIED;
        pMlme->mlmeData.uStatusCode = TI_NOK;
		mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_FAIL, pMlme);
	}
}

/** 
 * \fn     mlme_smAsoccSuccess 
 * \brief  change state function between ASSOC_WAIT back to IDLE
 * 
 * The function is called upon state change from ASSOC_WAIT back to IDLE
 * sets all parameters back to idle state and report to the conn
 * of a successfull association
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smAsoccSuccess(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	mlme_smToIdle(pMlme);

	pMlme->mlmeData.uStatusCode = TI_OK;
	pMlme->mlmeData.mgmtStatus = STATUS_SUCCESSFUL;

	conn_reportMlmeStatus(pMlme->hConn, pMlme->mlmeData.mgmtStatus, pMlme->mlmeData.uStatusCode); 
}

/** 
 * \fn     mlme_smStop 
 * \brief  called upon stop event
 * 
 * returns the SM back to idle
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smStop(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	mlme_smToIdle(pMlme);
}

/** 
 * \fn     mlme_smFail 
 * \brief  called upon auth/assoc failure
 * 
 * \note   before calling event FAILURE the caller should set mgmtStatus and uStatusCode
 *         accordingly
 * 
 * returns the SM back to idle with failure status code
 *  
 * \param  hMlme - handler to mlme
 */ 
void mlme_smFail(TI_HANDLE hMlme)
{
	mlme_t *pMlme = (mlme_t*)hMlme;

    mlme_smToIdle(pMlme);
    TRACE2(pMlme->hReport, REPORT_SEVERITY_INFORMATION, "mlme_smFail : status = %d Type = %d \n",pMlme->mlmeData.mgmtStatus ,pMlme->legacyAuthType);
    /* if the auth mode is AutoSwitch and the connection in SHARED mode fail then connect with OPEN mode */
	if ((pMlme->legacyAuthType == RSN_AUTH_AUTO_SWITCH) && (pMlme->mlmeData.mgmtStatus == STATUS_AUTH_REJECT ) && ( pMlme->bSharedFailed == TI_FALSE))
    {
        pMlme->bSharedFailed = TI_TRUE;
        pMlme->authInfo.authType = AUTH_LEGACY_OPEN_SYSTEM;
		return mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_START, pMlme);
    }

    conn_reportMlmeStatus(pMlme->hConn, pMlme->mlmeData.mgmtStatus, pMlme->mlmeData.uStatusCode);
}

/** 
 * \fn     mlme_smToIdle 
 * \brief  return all the mlme params to default values
 * 
 * Sets all the mlme params back to the defult values
 * 
 * \param  pMlme - handler to mlme
 */ 
void mlme_smToIdle(mlme_t *pMlme)
{
	tmr_StopTimer(pMlme->hMlmeTimer);
	pMlme->assocInfo.retryCount = 0;
	pMlme->authInfo.retryCount = 0;
	pMlme->assocInfo.currentState = MLME_SM_STATE_IDLE;
	pMlme->extraIesLen = 0; /* reset the extra Ies */
}

/** 
 * \fn     mlme_smActionUnexpected 
 * \brief  called upon unexpected event
 * 
 * \param  hMlme - handler to mlme
 */ 
void mlme_smActionUnexpected(TI_HANDLE hMlme)
{
    return;
}

/* local functions */

TI_STATUS mlme_smReportStatus(mlme_t *pMlme)
{
    TI_STATUS       status;

    if (pMlme == NULL)
    {
        return TI_NOK;
    }

    status = conn_reportMlmeStatus(pMlme->hConn, pMlme->mlmeData.mgmtStatus, pMlme->mlmeData.uStatusCode);
    return status;
}

/*****************************************************************************
**
** MLME messages builder/Parser
**
*****************************************************************************/

/* Timer expiery callbacks */
void mlme_authTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured)
{
	mlme_t *pMlme = (mlme_t*)hMlme;   
	pMlme->authInfo.authTimeoutCount++;
	mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_AUTH_TIMEOUT, pMlme);

}

void mlme_sharedTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured)
{
	mlme_t *pMlme = (mlme_t*)hMlme;
	pMlme->authInfo.authTimeoutCount++;
	mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_SHARED_TIMEOUT, pMlme);
}

void mlme_assocTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured)
{
	mlme_t *pMlme = (mlme_t*)hMlme; 
	pMlme->assocInfo.assocTimeoutCount++;
	mlme_smEvent(pMlme->hMlmeSm, MLME_SM_EVENT_ASSOC_TIMEOUT, pMlme);
}

