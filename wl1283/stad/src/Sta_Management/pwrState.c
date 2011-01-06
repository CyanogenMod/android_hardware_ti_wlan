/*
 * pwrState.c
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


/** \file pwrState.c
 *
 *  \brief Power State Module implementation.

 *  \date 02-Nov-2010
 */

#define __FILE_ID__ FILE_ID_140

#include "tidef.h"
#include "osApi.h"
#include "timer.h"
#include "paramOut.h"
#include "report.h"
#include "TWDriver.h"
#include "DrvMainModules.h"

#include "ScanCncn.h"
#include "sme.h"
#include "measurementMgr.h"
#include "PowerMgr_API.h"
#include "WlanDrvIf.h"

#include "pwrState.h"

/*****************************************************************************
 **         Functions (prototypes)                                          **
 *****************************************************************************/

static TI_STATUS pwrState_ConfigSuspend (TI_HANDLE hPwrState, TPwrStateCfg tConfig);
static TI_STATUS pwrState_SetPwr(TI_HANDLE hPwrState, EPwrStateSmState eState, void *fCb, TI_HANDLE hCb);
static TI_STATUS validateConfig(TI_HANDLE hPwrState, TPwrStateCfg tConfig);
static void      notifyTransitionComplete(TI_HANDLE hPwrState, TI_BOOL bTWDInitOccured);
static TI_STATUS state_Initial(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_Off(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_Sleep(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_LowOn(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_Standby(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_FullOn(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitFwInit(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitDrvStart(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitDrvStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitScanStopDueToDoze(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitScanStopDueToSleepOff(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitMeasureStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitDoze(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);
static TI_STATUS state_WaitSmeStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);

/*****************************************************************************
 **         Macro Definitions                                               **
 *****************************************************************************/

#define handleUnexpectedEvent(pPwrState, eEvent)				\
	do { 														\
		TRACE2((pPwrState)->hReport, REPORT_SEVERITY_WARNING,	\
				"%pF: unexpected event %d\n",					\
				(pPwrState)->fCurrentState, (eEvent));			\
	} while (0)

#define reportEvent(pPwrState, eEvent)			\
	do {							\
		TRACE2(this->hReport, REPORT_SEVERITY_INFORMATION,		\
				"%pF: got event %d\n",							\
				(pPwrState)->fCurrentState, (eEvent));			\
	} while (0)

/*****************************************************************************
 **         Functions (implementation)                                      **
 *****************************************************************************/

TI_HANDLE pwrState_Create (TI_HANDLE hOs)
{
	TPwrState *pPwrState = NULL;

	pPwrState = (TPwrState*) os_memoryAlloc (hOs, sizeof(TPwrState));

	if ( NULL == pPwrState )
	{
		WLAN_OS_REPORT(("%s: Memory Allocation Error!\n", __func__));
		return NULL;
	}

	os_memoryZero (hOs, pPwrState, sizeof(TPwrState));

	pPwrState->hOs = hOs;

	return pPwrState;
}

void pwrState_Init (TStadHandlesList *pStadHandles)
{
	TPwrState *pPwrState = (TPwrState*) pStadHandles->hPwrState;

	pPwrState->hReport			= pStadHandles->hReport;
	pPwrState->hDrvMain			= pStadHandles->hDrvMain;
	pPwrState->hScanCncn		= pStadHandles->hScanCncn;
	pPwrState->hSme				= pStadHandles->hSme;
	pPwrState->hMeasurementMgr	= pStadHandles->hMeasurementMgr;
	pPwrState->hPowerMgr		= pStadHandles->hPowerMgr;
	pPwrState->hTimer			= pStadHandles->hTimer;
	pPwrState->hTWD				= pStadHandles->hTWD;

	pPwrState->tCurrentTransition.hCompleteTimer = tmr_CreateTimer(pPwrState->hTimer);
	pPwrState->hDozeTimer = tmr_CreateTimer(pPwrState->hTimer);

	pPwrState->fCurrentState = state_Initial;

	pPwrState->fCurrentState(pPwrState, PWRSTATE_EVNT_CREATE);
}

TI_STATUS pwrState_SetDefaults (TI_HANDLE hPwrState, TPwrStateInitParams *pInitParams)
{
	TPwrStateCfg tCfg;

	tCfg.eSuspendType = pInitParams->eSuspendType;
	tCfg.uSuspendNDTIM = pInitParams->uSuspendNDTIM;
	tCfg.eStandbyNextState = pInitParams->eStandbyNextState;
	tCfg.eSuspendFilterUsage = pInitParams->eSuspendFilterUsage;
	tCfg.tSuspendRxFilterValue = pInitParams->tSuspendRxFilterValue;
	tCfg.uDozeTimeout = pInitParams->uDozeTimeout;
	tCfg.tTwdSuspendConfig.uCmdMboxTimeout = pInitParams->uCmdTimeout;

	return pwrState_ConfigSuspend(hPwrState, tCfg);
}

TI_STATUS pwrState_Destroy (TI_HANDLE hPwrState)
{
	TPwrState *pPwrState = (TPwrState*) hPwrState;

	if (pPwrState->tCurrentTransition.hCompleteTimer)
	{
		tmr_DestroyTimer(pPwrState->tCurrentTransition.hCompleteTimer);
	}

	if (pPwrState->hDozeTimer)
	{
		tmr_DestroyTimer(pPwrState->hDozeTimer);
	}

	os_memoryFree(pPwrState->hOs, pPwrState, sizeof(TPwrState));

	return TI_OK;
}

TI_STATUS pwrState_SetParam (TI_HANDLE hPwrState, paramInfo_t *pParam)
{
	TPwrState *pPwrState      = (TPwrState*) hPwrState;
	TI_STATUS  rc             = TI_OK;

	switch (pParam->paramType)
	{

	case PWR_STATE_SUSPEND_TYPE_PARAM:
	{
		/* prepare new configuration */
		TPwrStateCfg tCfg = pPwrState->tConfig;
		tCfg.eSuspendType = pParam->content.pwrStateSuspendType;

		rc = pwrState_ConfigSuspend(pPwrState, tCfg);
		break;
	}

	case PWR_STATE_SUSPEND_NDTIM_PARAM:
	{
		/* prepare new configuration */
		TPwrStateCfg tCfg = pPwrState->tConfig;
		tCfg.uSuspendNDTIM = pParam->content.pwrStateSuspendNDTIM;

		rc = pwrState_ConfigSuspend(pPwrState, tCfg);
		break;
	}

	case PWR_STATE_STNDBY_DOZE_ACTN_PARAM:
	{
		/* prepare new configuration */
		TPwrStateCfg tCfg = pPwrState->tConfig;
		tCfg.eStandbyNextState = pParam->content.pwrStateStandbyNextState;

		rc = pwrState_ConfigSuspend(pPwrState, tCfg);
		break;
	}

	case PWR_STATE_FILTER_USAGE_PARAM:
	{
		/* prepare new configuration */
		TPwrStateCfg tCfg = pPwrState->tConfig;
		tCfg.eSuspendFilterUsage = pParam->content.pwrStateSuspendFilterUsage;

		rc = pwrState_ConfigSuspend(pPwrState, tCfg);
		break;
	}

	case PWR_STATE_RX_FILTER_PARAM:
	{
		/* prepare new configuration */
		TPwrStateCfg tCfg = pPwrState->tConfig;
		tCfg.tSuspendRxFilterValue = pParam->content.pwrStateSuspendRxFilterValue;

		rc = pwrState_ConfigSuspend(pPwrState, tCfg);
		break;
	}

	default:
		TRACE1(pPwrState->hReport, REPORT_SEVERITY_WARNING, "pwrState_SetParam: unknown param-id (0x%x)\n", pParam->paramType);
		rc = TI_NOK;
	}

	return (rc == TI_PENDING) ? TI_OK : rc;
}

TI_STATUS pwrState_GetParam (TI_HANDLE hPwrState, paramInfo_t *pParam)
{
	TPwrState *pPwrState      = (TPwrState*) hPwrState;
	TI_STATUS  rc             = TI_OK;

	switch (pParam->paramType)
	{
	case PWR_STATE_PWR_ON_PARAM:
	{
		rc = pwrState_SetPwr(pPwrState, PWRSTATE_STATE_FULLY_ON,
				pParam->content.interogateCmdCBParams.fCb, pParam->content.interogateCmdCBParams.hCb);
		break;
	}

	case PWR_STATE_DOZE_PARAM:
	{
		rc = pwrState_SetPwr(pPwrState, PWRSTATE_STATE_LOW_ON,
				pParam->content.interogateCmdCBParams.fCb, pParam->content.interogateCmdCBParams.hCb);
		break;
	}

	case PWR_STATE_SLEEP_PARAM:
	{
		rc = pwrState_SetPwr(pPwrState, PWRSTATE_STATE_SLEEP,
				pParam->content.interogateCmdCBParams.fCb, pParam->content.interogateCmdCBParams.hCb);
		break;
	}

	case PWR_STATE_PWR_OFF_PARAM:
	{
		rc = pwrState_SetPwr(pPwrState, PWRSTATE_STATE_OFF,
				pParam->content.interogateCmdCBParams.fCb, pParam->content.interogateCmdCBParams.hCb);
		break;
	}

	default:
		TRACE1(pPwrState->hReport, REPORT_SEVERITY_WARNING, "pwrState_GetParam: unknown param-id (0x%x)\n", pParam->paramType);
		rc = TI_NOK;
	}

	return (rc == TI_PENDING) ? TI_OK : rc;
}

static TI_STATUS validateConfig(TI_HANDLE hPwrState, TPwrStateCfg tConfig)
{
	TPwrState *this = (TPwrState*) hPwrState;

	if ( (tConfig.eSuspendType >= PWRSTATE_SUSPEND_LAST) ||
		 (tConfig.eSuspendType == PWRSTATE_SUSPEND_RESERVED) )
	{
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "invalid eSuspendType (%d)", tConfig.eSuspendType);
		return TI_NOK;
	}

	if ( (tConfig.uSuspendNDTIM < PWRSTATE_SUSPEND_NDTIM_MIN) ||
		 (tConfig.uSuspendNDTIM > PWRSTATE_SUSPEND_NDTIM_MAX) )
	{
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "invalid uSuspendNDTIM (%d)", tConfig.uSuspendNDTIM);
		return TI_NOK;
	}

	if ( (tConfig.uDozeTimeout < PWRSTATE_DOZE_TIMEOUT_MIN) ||
		 (tConfig.uDozeTimeout > PWRSTATE_DOZE_TIMEOUT_MAX) )
	{
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "invalid uDozeTimeout (%d)", tConfig.uDozeTimeout);
		return TI_NOK;
	}


	if ( (tConfig.eStandbyNextState >= PWRSTATE_STNDBY_NEXT_STATE_LAST) ||
		 (tConfig.eStandbyNextState == PWRSTATE_STNDBY_NEXT_STATE_RESERVED1) ||
		 (tConfig.eStandbyNextState == PWRSTATE_STNDBY_NEXT_STATE_RESERVED2) )
	{
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "invalid eStandbyNextState (%d)", tConfig.eStandbyNextState);
		return TI_NOK;
	}

	if (tConfig.eSuspendFilterUsage >= PWRSTATE_FILTER_USAGE_LAST)
	{
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "invalid eSuspendFilterUsage (%d)", tConfig.eSuspendFilterUsage);
		return TI_NOK;
	}

	return TI_OK;
}

/*
 * \brief	Configures the suspend parameters
 *
 * \param	hPwrState		handle to the pwrState module
 * \param	tConfig			configuration
 *
 * \return	TI_NOK if tConfig is invalid; TI_OK otherwise
 */
TI_STATUS pwrState_ConfigSuspend(TI_HANDLE hPwrState, TPwrStateCfg tConfig)
{
	TPwrState *this = (TPwrState*) hPwrState;

	if (validateConfig(this, tConfig) != TI_OK)
	{
		return TI_NOK;
	}

	this->tConfig = tConfig;
	return TI_OK;
}

/*
 * \brief	Entry point for steady state to steady state transitions
 *
 * 			A Steady State is one of the target states: FullyOn. LowOn, Sleep or Off.
 *
 * \param	hPwrState	handle to the pwrState module
 * \param	eState		steady state to transition to
 * \param	fCb			call-back to invoke when state eState is reached
 * \param	hCb			handle for fCb
 *
 * \return	TI_OK if event was issued and the state-machine transitioned to state eState.
 * 			TI_PENDING if event was issued and the state-machine is not yet in state eState. When the state-machine reaches state eState, fCb(hCb) will be invoked.
 * 			TI_NOK if event was not issued.
 *
 */
TI_STATUS pwrState_SetPwr(TI_HANDLE hPwrState, EPwrStateSmState eState, void *fCb, TI_HANDLE hCb)
{
	TPwrState       *this = (TPwrState*) hPwrState;
	EPwrStateSmEvent eEvent;
	TI_STATUS        rc;

	/* sanity check.. */
	if (NULL == this->fCurrentState) {
		TRACE0(this->hReport, REPORT_SEVERITY_ERROR, "pwrState_SetPwr: fCurrentState not initialized!\n");
		return TI_NOK;
	}

	/*
	 * convert requested state to event
	 */
	switch (eState)
	{
	case PWRSTATE_STATE_FULLY_ON:
		eEvent = PWRSTATE_EVNT_ON;
		break;
	case PWRSTATE_STATE_LOW_ON:
		eEvent = PWRSTATE_EVNT_DOZE;
		break;
	case PWRSTATE_STATE_SLEEP:
		eEvent = PWRSTATE_EVNT_SLEEP;
		break;
	case PWRSTATE_STATE_OFF:
		eEvent = PWRSTATE_EVNT_OFF;
		break;
	default:
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "pwrState_SetPwr: invalid requested-state (%d)\n", eState);
		return TI_NOK;
	}

	/*
	 * setup transition-completion callback
	 */
	this->tCurrentTransition.fCompleteCb     = fCb;
	this->tCurrentTransition.hCompleteCb     = hCb;
	this->tCurrentTransition.iCompleteStatus = TI_OK;

	/*
	 * Steady state to steady state transition (e.g. FullOn to Sleep) can be
	 * carried out by several transitions through intermediate states.
	 *
	 * Initially, the (steady state to steady state) transition is
	 * marked as synchronous - starts and ends in the same task.
	 *
	 * If the event eventually calls notifyTransitionComplete() in this
	 * driver-task - a timer will be used to complete the command in
	 * another driver-task (see notifyTransitionComplete()).
	 *
	 * If the event does not call notifyTransitionComplete() in this
	 * task (it probably returns TI_PENDING), the transition is marked
	 * as asynchronous (started and completed in different tasks) and
	 * no timer will be used (see notifyTransitionComplete()).
	 *
	 */
	this->tCurrentTransition.bAsyncTransition = TI_FALSE;

	/*
	 * invoke state transition
	 */
	rc = this->fCurrentState(this, eEvent);

	if (rc == TI_PENDING)
	{
		this->tCurrentTransition.bAsyncTransition = TI_TRUE;
	}

	return rc;
}

/*
 * \brief	Invokes the transition-complete callback
 *
 * 			If the transition was completed in the same driver-task it
 * 			was started in, uses a timer to complete it in another task.
 *
 * 			Otherwise, directly invokes the callback.
 *
 * \param	bTWDInitOccured	ignored
 */
static void notifyTransitionComplete(TI_HANDLE hPwrState, TI_BOOL bTWDInitOccured)
{
	TPwrState *this = (TPwrState*) hPwrState;

	/*
	 * If the transition was started and completed in different
	 * tasks - it is safe to invoke the command-complete callback.
	 */
	if (this->tCurrentTransition.bAsyncTransition)
	{
		TRACE1(this->hReport, REPORT_SEVERITY_INFORMATION, "transition to %pF completed\n", this->fCurrentState);

		if (this->tCurrentTransition.fCompleteCb)
		{
			void(*fCb)(TI_HANDLE,int,void*);

			/* prevent re-entring / re-invoking the callback */
			fCb = this->tCurrentTransition.fCompleteCb;
			this->tCurrentTransition.fCompleteCb = NULL;

			/* invoke the callback */
			(fCb)(
					(this->tCurrentTransition.hCompleteCb),
					(this->tCurrentTransition.iCompleteStatus),
					NULL );
		}
	}
	/*
	 * Otherwise, the transition was started in this task (where
	 * this function was called).
	 * Since we cannot invoke the command-complete callback from the
	 * same driver's task the (async) command was started in - we need
	 * to use a timer to do it in another task.
	 */
	else
	{
		TRACE0(this->hReport, REPORT_SEVERITY_INFORMATION, "transition completed synchronously. Setting timer to complete it asynchronously\n");

		/*
		 * Mark as an asynchronous transition (we will finish it in another task)
		 */
		this->tCurrentTransition.bAsyncTransition = TI_TRUE;

		/*
		 * Start a minimal-time timer (it will still let this task finish first)
		 */
		tmr_StartTimer(this->tCurrentTransition.hCompleteTimer, notifyTransitionComplete, this, 1, TI_FALSE);
	}
}

/************************** State-machine state functions *****************************/

static TI_STATUS state_Initial(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_CREATE:
		this->fCurrentState = state_Off;

		wlanDrvIf_UpdateDriverState(this->hOs, DRV_STATE_IDLE);

		rc = TI_OK;
		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_Off(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_ON:
		this->fCurrentState = state_WaitFwInit;

		rc = drvMain_Start(this->hDrvMain, pwrState_FwInitDone, this, pwrState_DrvMainStarted, this);

		/* if failed to start the driver, revert back to this state */
		if (TI_NOK == rc)
		{
			this->fCurrentState = state_Off;
		}

		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitFwInit(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_FW_INIT_DONE:
		this->fCurrentState = state_WaitDrvStart;

		wlanDrvIf_UpdateDriverState(this->hOs, DRV_STATE_RUNNING);

		/* transition is now complete */
		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;

		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitDrvStart(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_DRVMAIN_STARTED:
		this->fCurrentState = state_Sleep;

		rc = this->fCurrentState(this, PWRSTATE_EVNT_ON);
		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitDrvStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_DRVMAIN_STOPPED:
		this->fCurrentState = state_Off;

		wlanDrvIf_UpdateDriverState(this->hOs, DRV_STATE_STOPPED);

		/* transition is now complete */
		TWD_CompleteSuspend(this->hTWD);
		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;

		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_Sleep(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_OFF:
		this->fCurrentState = state_WaitDrvStop;

		wlanDrvIf_UpdateDriverState(this->hOs, DRV_STATE_STOPING);

		rc = drvMain_Stop(this->hDrvMain, pwrState_DrvMainStopped, this);

		/* if failed to stop driver, revert back to this state */
		if (TI_NOK == rc)
		{
			this->fCurrentState = state_Sleep;
		}

		break;

	case PWRSTATE_EVNT_ON:
		this->fCurrentState = state_Standby;

		scanCncn_Resume(this->hScanCncn);
		measurementMgr_Resume(this->hMeasurementMgr);
		sme_Start(this->hSme);

		/* transition is now complete */
		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;

		break;
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_Standby(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState       *this = (TPwrState*) hPwrState;
	TI_STATUS        rc;
	EPwrStateSmEvent eTempEvent;

	reportEvent(this, eEvent);

	/* if we got a Doze event, convert it to either Off, Sleep or None, according
	 * to the configuration
	 */
	if (eEvent == PWRSTATE_EVNT_DOZE) {
		switch (this->tConfig.eStandbyNextState)
		{
		case PWRSTATE_STNDBY_NEXT_STATE_STANDBY:
			eTempEvent = PWRSTATE_EVNT_NONE;
			break;
		case PWRSTATE_STNDBY_NEXT_STATE_SLEEP:
			eTempEvent = PWRSTATE_EVNT_SLEEP;
			break;
		case PWRSTATE_STNDBY_NEXT_STATE_OFF:
			eTempEvent = PWRSTATE_EVNT_OFF;
			break;
		default:
			TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "state_Standby: invalid eStandbyNextState (%d)\n", this->tConfig.eStandbyNextState);
			return TI_NOK;
		}

		TRACE1(this->hReport, REPORT_SEVERITY_INFORMATION, "state_Standby: processing Doze event as %d\n", eTempEvent);
	}
	else
	{
		eTempEvent = eEvent;
	}

	switch (eTempEvent)
	{
	case PWRSTATE_EVNT_NONE:
	case PWRSTATE_EVNT_ON:
		/* do nothing. stay in Standby state */

		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;
		break;

	case PWRSTATE_EVNT_OFF:
		/* stop scan and move to Sleep, and then Off state */

		TWD_PrepareSuspend(this->hTWD, &this->tConfig.tTwdSuspendConfig);

		this->fCurrentState = state_WaitScanStopDueToSleepOff;
		this->tCurrentTransition.bContinueToOff = TI_TRUE;
		rc = scanCncn_Suspend(this->hScanCncn);

		break;

	case PWRSTATE_EVNT_SLEEP:
		/* stop scan and move to Sleep state */

		TWD_PrepareSuspend(this->hTWD, &this->tConfig.tTwdSuspendConfig);

		this->fCurrentState = state_WaitScanStopDueToSleepOff;
		rc = scanCncn_Suspend(this->hScanCncn);

		break;

	case PWRSTATE_EVNT_SME_CONNECTED:
		this->fCurrentState = state_FullOn;

		rc = TI_OK;
		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_FullOn(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_OFF:
		/* stop scan and move to Sleep, and then Off state */

		TWD_PrepareSuspend(this->hTWD, &this->tConfig.tTwdSuspendConfig);

		this->fCurrentState = state_WaitScanStopDueToSleepOff;
		this->tCurrentTransition.bContinueToOff = TI_TRUE;
		rc = scanCncn_Suspend(this->hScanCncn);

		break;

	case PWRSTATE_EVNT_SLEEP:
		/* stop scan and move to Sleep state */

		TWD_PrepareSuspend(this->hTWD, &this->tConfig.tTwdSuspendConfig);

		this->fCurrentState = state_WaitScanStopDueToSleepOff;
		rc = scanCncn_Suspend(this->hScanCncn);

		break;

	case PWRSTATE_EVNT_DOZE:
		TWD_PrepareSuspend(this->hTWD, &this->tConfig.tTwdSuspendConfig);

		this->fCurrentState = state_WaitMeasureStop;

		rc = measurementMgr_Suspend(this->hMeasurementMgr, pwrState_MeasurementStopped, this);

		break;

	case PWRSTATE_EVNT_SME_DISCONNECTED:
		this->fCurrentState = state_Standby;

		rc = TI_OK;
		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitMeasureStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_MEASUR_STOPPED:
		this->fCurrentState = state_WaitScanStopDueToDoze;

		rc = scanCncn_Suspend(this->hScanCncn);

		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitScanStopDueToDoze(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_SCANCNCN_STOPPED:
	{
		this->fCurrentState = state_WaitDoze;

		tmr_StartTimer(this->hDozeTimer, pwrState_DozeTimeout, this, this->tConfig.uDozeTimeout, TI_FALSE);
		rc = powerMgr_Suspend(this->hPowerMgr, &this->tConfig, pwrState_DozeDone, this);

		break;
	}
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitScanStopDueToSleepOff(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_SCANCNCN_STOPPED:
	{
		this->fCurrentState = state_WaitSmeStop;

		sme_Stop(this->hSme);

		rc = TI_PENDING;

		break;
	}
	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_LowOn(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_ON:
		this->fCurrentState = state_FullOn;

		powerMgr_Resume(this->hPowerMgr);
		scanCncn_Resume(this->hScanCncn);
		measurementMgr_Resume(this->hMeasurementMgr);

		/* transition is now complete */
		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;

		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitSmeStop(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_SME_STOPPED:
		this->fCurrentState = state_Sleep;

		if (this->tCurrentTransition.bContinueToOff)
		{
			this->tCurrentTransition.bContinueToOff = TI_FALSE;
			rc = this->fCurrentState(this, PWRSTATE_EVNT_OFF);
		}
		else
		{
			/* transition is now complete */
			TWD_CompleteSuspend(this->hTWD);
			notifyTransitionComplete(this, TI_FALSE);
			rc = TI_OK;
		}

		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

static TI_STATUS state_WaitDoze(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent)
{
	TPwrState *this = (TPwrState*) hPwrState;
	TI_STATUS rc;

	reportEvent(this, eEvent);

	switch (eEvent)
	{
	case PWRSTATE_EVNT_DOZE_DONE:
		this->fCurrentState = state_LowOn;

		tmr_StopTimer(this->hDozeTimer);

		/* transition is now complete */
		TWD_CompleteSuspend(this->hTWD);
		notifyTransitionComplete(this, TI_FALSE);
		rc = TI_OK;

		break;

	case PWRSTATE_EVNT_DOZE_TIMEOUT:
		TRACE1(this->hReport, REPORT_SEVERITY_WARNING, "state_WaitDoze: doze timer (%d ms) expired!\n", this->tConfig.uDozeTimeout);

		this->fCurrentState = state_WaitSmeStop;

		this->tCurrentTransition.bContinueToOff = (this->tConfig.eStandbyNextState == PWRSTATE_STNDBY_NEXT_STATE_OFF);
		sme_Stop(this->hSme);

		rc = TI_OK;

		if (this->tConfig.eStandbyNextState == PWRSTATE_STNDBY_NEXT_STATE_STANDBY)
		{
			this->tCurrentTransition.iCompleteStatus = TI_NOK;
		}

		break;

	default:
		handleUnexpectedEvent(this, eEvent);
		rc = TI_NOK;
	}

	return rc;
}

/************************** State-machine notification functions *****************************/

void pwrState_FwInitDone(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_FW_INIT_DONE);
}

void pwrState_DrvMainStarted(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_DRVMAIN_STARTED);
}

void pwrState_DrvMainStopped(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_DRVMAIN_STOPPED);
}

void pwrState_DozeDone(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_DOZE_DONE);
}

void pwrState_DozeTimeout(TI_HANDLE hPwrState, TI_BOOL bTWDInitOccured)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_DOZE_TIMEOUT);
}

void pwrState_SmeStopped(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_SME_STOPPED);
}

void pwrState_SmeConnected(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_SME_CONNECTED);
}

void pwrState_SmeDisconnected(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_SME_DISCONNECTED);
}

void pwrState_ScanCncnStopped(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_SCANCNCN_STOPPED);
}

void pwrState_MeasurementStopped(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*) hPwrState;

	this->fCurrentState(this, PWRSTATE_EVNT_MEASUR_STOPPED);
}
