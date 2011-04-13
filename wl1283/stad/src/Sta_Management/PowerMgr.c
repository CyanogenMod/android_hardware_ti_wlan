/*
 * PowerMgr.c
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

/** \file PowerMgr.c
 *  \brief This is the PowerMgr module implementation.
 *  \
 *  \date 24-Oct-2005
 */

/****************************************************************************
 *                                                                          *
 *   MODULE:  PowerMgr                                                      *
 *   PURPOSE: PowerMgr Module implementation.                               *
 *                                                                          *
 ****************************************************************************/

#define __FILE_ID__  FILE_ID_71
#include "tidef.h"
#include "osApi.h"
#include "timer.h"
#include "paramOut.h"
#include "report.h"
#include "PowerMgr.h"
#include "PowerMgr_API.h"
#include "TrafficMonitorAPI.h"
#include "qosMngr_API.h"
#include "siteMgrApi.h"
#include "TWDriver.h"
#include "SoftGeminiApi.h"
#include "DrvMainModules.h"
#include "PowerMgrKeepAlive.h"
#include "CmdBld.h"
#include "healthMonitor.h"
#include "DataCtrl_Api.h"


/*****************************************************************************
 **         Defines                                                         **
 *****************************************************************************/
#define DEFAULT_LISTEN_INTERVAL (1)
#define BET_DISABLE 0
#define BET_ENABLE  1

#define POWER_SAVE_GUARD_TIME_MS            5000       /* The gaurd time used to protect from FW stuck */

#define invokeCallback(fCb, hCb)		\
	do {								\
		if (fCb) {						\
			(fCb)(hCb);					\
			(fCb) = NULL;				\
		}								\
	} while (0)

/*****************************************************************************
 **         Private Function prototypes                                      **
 *****************************************************************************/

static void 		powerSaveReportCB(TI_HANDLE hPowerMgr, char* str , TI_UINT32 strLen);
static void         PowerMgrTMThresholdCrossCB( TI_HANDLE hPowerMgr, TI_UINT32 cookie );
static void         powerMgrDisableThresholdsIndications(TI_HANDLE hPowerMgr);
static void         powerMgrEnableThresholdsIndications(TI_HANDLE hPowerMgr);
static void         powerMgrStartAutoPowerMode(TI_HANDLE hPowerMgr);
static void         powerMgrRetryPsTimeout(TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured);
static TI_STATUS	powerMgrPowerProfileConfiguration(TI_HANDLE hPowerMgr, PowerMgr_PowerMode_e desiredPowerMode);
static void         PowerMgr_setDozeModeInAuto(TI_HANDLE hPowerMgr,PowerMgr_PowerMode_e dozeMode);
static void         PowerMgrConfigBetToFw( TI_HANDLE hPowerMgr, TI_UINT32 cookie );
static void         PowerMgr_PsPollFailureCB( TI_HANDLE hPowerMgr );
static void 		powerMgr_PsPollFailureTimeout( TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured );
static void 		powerMgr_SGSetUserDesiredwakeUpCond( TI_HANDLE hPowerMgr );
static TI_STATUS    powerMgrSendMBXWakeUpConditions(TI_HANDLE hPowerMgr,TI_UINT8 listenInterval, ETnetWakeOn tnetWakeupOn);
static void 		powerMgrGuardTimerExpired (TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured);
static TI_STATUS	powerMgrSetPsMode (TI_HANDLE hPowerMgr, E80211PsMode psMode, TI_HANDLE hPowerSaveCompleteCb,
									   TPowerSaveResponseCb fCb);
static PowerMgr_PowerMode_e powerMgrGetHighestPriority(TI_HANDLE hPowerMgr);
static TI_STATUS	updatePowerAuthority(TI_HANDLE hPowerMgr);

/*****************************************************************************
 **         Public Function prototypes                                      **
 *****************************************************************************/


/****************************************************************************************
 *                        PowerMgr_create                                                           *
 ****************************************************************************************
DESCRIPTION: Creates the object of the power Manager. 
                performs the following:
                -   Allocate the Power Manager handle
                -   Creates the retry timer
                                                                                                                   
INPUT:          - hOs - Handle to OS        
OUTPUT:     
RETURN:     Handle to the Power Manager module on success, NULL otherwise
****************************************************************************************/
TI_HANDLE PowerMgr_create(TI_HANDLE hOs)
{

    PowerMgr_t * pPowerMgr = NULL;
    pPowerMgr = (PowerMgr_t*) os_memoryAlloc (hOs, sizeof(PowerMgr_t));
    if ( pPowerMgr == NULL )
    {
        WLAN_OS_REPORT(("PowerMgr_create - Memory Allocation Error!\n"));
        return NULL;
    }

    os_memoryZero (hOs, pPowerMgr, sizeof(PowerMgr_t));

    pPowerMgr->hOS = hOs;

    /* create the power manager keep-alive sub module */
    pPowerMgr->hPowerMgrKeepAlive = powerMgrKL_create (hOs);

    return pPowerMgr;

}


/****************************************************************************************
*                        powerSrv_destroy                                                          *
****************************************************************************************
DESCRIPTION: Destroy the object of the power Manager.
               -   delete Power Manager alocation
               -   call the destroy function of the timer
                                                                                                                  
INPUT:          - hPowerMgr - Handle to the Power Manager   
OUTPUT:     
RETURN:    TI_STATUS - TI_OK on success else TI_NOK.
****************************************************************************************/
TI_STATUS PowerMgr_destroy(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    /* destroy the power manager keep-alive sub module */
    powerMgrKL_destroy (pPowerMgr->hPowerMgrKeepAlive);

    if (pPowerMgr->hRetryPsTimer)
    {
        tmr_DestroyTimer (pPowerMgr->hRetryPsTimer);
    }

	if (pPowerMgr->hEnterPsGuardTimer)
    {
        tmr_DestroyTimer (pPowerMgr->hEnterPsGuardTimer);
    }

    if ( pPowerMgr->hPsPollFailureTimer != NULL )
    {
        tmr_DestroyTimer(pPowerMgr->hPsPollFailureTimer);
    }
    os_memoryFree(pPowerMgr->hOS, pPowerMgr, sizeof(PowerMgr_t));

    return TI_OK;
}


/****************************************************************************************
*                        PowerMgr_init                                                         *
****************************************************************************************
DESCRIPTION: Power Manager init function, called in init phase.
                                                                                                                 
INPUT:     pStadHandles  - The driver modules handles

OUTPUT:     

RETURN:    void
****************************************************************************************/
void PowerMgr_init (TStadHandlesList *pStadHandles)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)(pStadHandles->hPowerMgr);

    pPowerMgr->hReport          	= pStadHandles->hReport;
    pPowerMgr->hTrafficMonitor  	= pStadHandles->hTrafficMon;
	pPowerMgr->hHealthMonitor		= pStadHandles->hHealthMonitor;
    pPowerMgr->hSiteMgr         	= pStadHandles->hSiteMgr;
    pPowerMgr->hTWD             	= pStadHandles->hTWD;
    pPowerMgr->hSoftGemini      	= pStadHandles->hSoftGemini;
    pPowerMgr->hTimer           	= pStadHandles->hTimer;
	pPowerMgr->hQosMngr			    = pStadHandles->hQosMngr;
	pPowerMgr->hRxData              = pStadHandles->hRxData;
    pPowerMgr->psEnable         	= TI_FALSE;
	pPowerMgr->psCurrentMode		= POWER_SAVE_OFF;
	pPowerMgr->psLastRequest		= POWER_SAVE_OFF;
    pPowerMgr->hRetryPsTimer		= NULL;
	pPowerMgr->hPsPollFailureTimer	= NULL;
	pPowerMgr->hEnterPsGuardTimer	= NULL;
	pPowerMgr->eLastPowerAuth       = POWERAUTHO_POLICY_ELP; /* ELP is the default in FW */
    
    /* initialize the power manager keep-alive sub module */
    powerMgrKL_init (pPowerMgr->hPowerMgrKeepAlive, pStadHandles);

}

/***************************************************************************************
*                        PowerMgr_recover                                              *
****************************************************************************************
DESCRIPTION: Power Manager recovery function, called upon recovery.
                                                                                                                 
INPUT:     hPowerMgr  - The power mgr module handle

OUTPUT:     

RETURN:    void
****************************************************************************************/
void PowerMgr_recover(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    if (pPowerMgr->psEnable && pPowerMgr->hEnterPsGuardTimer)
	{
		tmr_StopTimer(pPowerMgr->hEnterPsGuardTimer);
	}
}


TI_STATUS PowerMgr_SetDefaults (TI_HANDLE hPowerMgr, PowerMgrInitParams_t* pPowerMgrInitParams)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    TI_UINT8 index;
    /* used to initialize the Traffic Monitor for Auto Ps events */
    TrafficAlertRegParm_t tmRegParam;
    TI_STATUS status;

	pPowerMgr->reAuthActivePriority		= pPowerMgrInitParams->reAuthActivePriority;
    
	/* init power management options */
    pPowerMgr->beaconListenInterval = pPowerMgrInitParams->beaconListenInterval;
    pPowerMgr->dtimListenInterval = pPowerMgrInitParams->dtimListenInterval;
    pPowerMgr->defaultPowerLevel =  pPowerMgrInitParams->defaultPowerLevel;
    pPowerMgr->PowerSavePowerLevel =  pPowerMgrInitParams->PowerSavePowerLevel;
    pPowerMgr->powerMngPriority  = POWER_MANAGER_USER_PRIORITY;
    pPowerMgr->maxFullBeaconInterval = pPowerMgrInitParams->MaximalFullBeaconReceptionInterval;
    pPowerMgr->PsPollDeliveryFailureRecoveryPeriod = pPowerMgrInitParams->PsPollDeliveryFailureRecoveryPeriod;

    /*
     set AUTO PS parameters 
     */
    pPowerMgr->autoModeInterval = pPowerMgrInitParams->autoModeInterval;
    pPowerMgr->autoModeActiveTH = pPowerMgrInitParams->autoModeActiveTH;
    pPowerMgr->autoModeDozeTH = pPowerMgrInitParams->autoModeDozeTH;
    pPowerMgr->autoModeDozeMode = pPowerMgrInitParams->autoModeDozeMode;

    /*
     register threshold in the traffic monitor.
     */
  	pPowerMgr->betEnable = pPowerMgrInitParams->BetEnable; /* save BET enable flag for CLI configuration */
	pPowerMgr->betTrafficEnable = TI_FALSE;                   /* starting without BET */

    /* BET thresholds */
    /* general parameters */
    tmRegParam.Context = pPowerMgr;
    tmRegParam.TimeIntervalMs = BET_INTERVAL_VALUE;
    tmRegParam.Trigger = TRAFF_EDGE;
    tmRegParam.MonitorType = TX_RX_ALL_802_11_DATA_FRAMES;
    tmRegParam.CallBack = PowerMgrConfigBetToFw;

    /* BET enable event */
    tmRegParam.Direction = TRAFF_DOWN;
    tmRegParam.Threshold = pPowerMgrInitParams->BetEnableThreshold;
    pPowerMgr->BetEnableThreshold = pPowerMgrInitParams->BetEnableThreshold;
    tmRegParam.Cookie = (TI_UINT32)BET_ENABLE;
    pPowerMgr->betEnableTMEvent = TrafficMonitor_RegEvent (pPowerMgr->hTrafficMonitor,
                                                             &tmRegParam,
                                                             TI_FALSE);
    /* BET disable event */
    tmRegParam.Direction = TRAFF_UP;
    tmRegParam.Threshold = pPowerMgrInitParams->BetDisableThreshold;
    pPowerMgr->BetDisableThreshold = pPowerMgrInitParams->BetDisableThreshold;
    tmRegParam.Cookie = (TI_UINT32)BET_DISABLE;
    pPowerMgr->betDisableTMEvent = TrafficMonitor_RegEvent (pPowerMgr->hTrafficMonitor,
                                                             &tmRegParam,
                                                             TI_FALSE);

    if ( (pPowerMgr->betDisableTMEvent == NULL) ||
         (pPowerMgr->betEnableTMEvent == NULL))
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_init - TM - ERROR registering BET events - ABROTING init!\n");
        return TI_NOK;
    }
    /*
    set the events as resets for one another
    */
    status = TrafficMonitor_SetRstCondition (pPowerMgr->hTrafficMonitor,
                                            pPowerMgr->betDisableTMEvent,
                                            pPowerMgr->betEnableTMEvent,
                                            TI_TRUE);
    if ( status != TI_OK )
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_init - PowerMgr_init - ERROR binding BET events - ABROTING init!\n");
        return TI_NOK;
    }

    /* general parameters */
    tmRegParam.Context = pPowerMgr;

    tmRegParam.Cookie = (TI_UINT32)POWER_MODE_ACTIVE;
    tmRegParam.TimeIntervalMs = pPowerMgr->autoModeInterval;
    tmRegParam.Trigger = TRAFF_EDGE;
    tmRegParam.MonitorType = TX_RX_ALL_802_11_DATA_FRAMES;

    /* Active threshold */
    tmRegParam.CallBack = PowerMgrTMThresholdCrossCB;
    tmRegParam.Direction = TRAFF_UP;
    tmRegParam.Threshold = pPowerMgr->autoModeActiveTH;
    pPowerMgr->passToActiveTMEvent = TrafficMonitor_RegEvent (pPowerMgr->hTrafficMonitor,
                                                             &tmRegParam,
                                                             TI_FALSE);
    /* Doze threshold */
    tmRegParam.Direction = TRAFF_DOWN;
    tmRegParam.Threshold = pPowerMgr->autoModeDozeTH;
    tmRegParam.Cookie = (TI_UINT32)POWER_MODE_SHORT_DOZE; /* diffrentiation between long / short doze is done at the 
                                                          CB, according to configuration at time of CB invokation */
    pPowerMgr->passToDozeTMEvent = TrafficMonitor_RegEvent (pPowerMgr->hTrafficMonitor,
                                                           &tmRegParam,
                                                           TI_FALSE);

    if ( (pPowerMgr->passToActiveTMEvent == NULL) ||
         (pPowerMgr->passToDozeTMEvent == NULL))
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_init - PowerMgr_init - ERROR registering Auto mode events - ABROTING init!\n");
        return TI_NOK;
    }

    /*
    set the events as resets for one another
    */
    status = TrafficMonitor_SetRstCondition (pPowerMgr->hTrafficMonitor,
                                            pPowerMgr->passToActiveTMEvent,
                                            pPowerMgr->passToDozeTMEvent,
                                            TI_TRUE);
    if ( status != TI_OK )
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_init - PowerMgr_init - ERROR binding Auto mode events - ABROTING init!\n");
        return TI_NOK;
    }

    /*
    configure the initialize power mode
    */
    pPowerMgr->desiredPowerModeProfile = pPowerMgrInitParams->powerMode;
    for ( index = 0;index < POWER_MANAGER_MAX_PRIORITY;index++ )
    {
        pPowerMgr->powerMngModePriority[index].powerMode = pPowerMgr->desiredPowerModeProfile;
        pPowerMgr->powerMngModePriority[index].priorityEnable = TI_FALSE;
    }
    pPowerMgr->powerMngModePriority[POWER_MANAGER_USER_PRIORITY].priorityEnable = TI_TRUE;

    if (pPowerMgr->reAuthActivePriority)	
		pPowerMgr->powerMngModePriority[POWER_MANAGER_REAUTH_PRIORITY].powerMode = POWER_MODE_ACTIVE;

    updatePowerAuthority(pPowerMgr);

    /*create the timers */
    pPowerMgr->hRetryPsTimer = tmr_CreateTimer(pPowerMgr->hTimer);
    pPowerMgr->hPsPollFailureTimer = tmr_CreateTimer(pPowerMgr->hTimer);
	pPowerMgr->hEnterPsGuardTimer = tmr_CreateTimer(pPowerMgr->hTimer);

    if ( (pPowerMgr->hPsPollFailureTimer == NULL) ||
		 (pPowerMgr->hRetryPsTimer == NULL) ||
		 (pPowerMgr->hEnterPsGuardTimer == NULL))
    {
		TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_SetDefaults - ERROR creating timers - ABROTING init!\n");
		return TI_NOK;
    }


	/* Register and Enable the PS report event */
    TWD_RegisterEvent (pPowerMgr->hTWD,
        TWD_OWN_EVENT_PS_REPORT,
        (void *)powerSaveReportCB, 
        hPowerMgr);
    TWD_EnableEvent (pPowerMgr->hTWD, TWD_OWN_EVENT_PS_REPORT);


    /* Register and Enable the PsPoll failure */
    TWD_RegisterEvent (pPowerMgr->hTWD,
        TWD_OWN_EVENT_PSPOLL_DELIVERY_FAILURE,
        (void *)PowerMgr_PsPollFailureCB, 
        hPowerMgr);
    TWD_EnableEvent (pPowerMgr->hTWD, TWD_OWN_EVENT_PSPOLL_DELIVERY_FAILURE);

    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INIT, "PowerMgr_init - PowerMgr Initialized\n");

    /* set defaults for the power manager keep-alive sub module */
    powerMgrKL_setDefaults (pPowerMgr->hPowerMgrKeepAlive);

    return TI_OK;
}

/****************************************************************************************
 *                        PowerMgr_startPS                                                          *
 ****************************************************************************************
DESCRIPTION: Start the power save algorithm of the driver and also the 802.11 PS.
                                                                                                                   
INPUT:          - hPowerMgr             - Handle to the Power Manager

OUTPUT:     
RETURN:    TI_STATUS - TI_OK or PENDING on success else TI_NOK.\n
****************************************************************************************/
TI_STATUS PowerMgr_startPS(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
	int frameCount;

    
    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_startPS - called\n");

    if ( pPowerMgr->psEnable == TI_TRUE )
    {
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgr_startPS - PS mechanism is already Enable! Aborting psEnable=%d !\n", pPowerMgr->psEnable);
        /*
        this is a FATAL ERROR of the power manager!
        already enable power-save! thus return TI_OK, but there is an error in the upper
        layer that call tp PowerMgr_startPS() twice - should know that power-save
        is already enable therefor print the Error message.
        or
        the state machine while NOT in PS can be only in ACTIVE state and in some cases in
        PS_PENDING state. therefore the state machine is out of sync from it logic!
        */
        return TI_OK;
    }

    pPowerMgr->psEnable = TI_TRUE;
    
    /*
    if in auto mode then need to refer to the threshold cross indication from the traffic monitor,
    else it need to refer to the configured power mode profile from the user.
    */
    pPowerMgr->desiredPowerModeProfile = powerMgrGetHighestPriority(hPowerMgr);

    if ( pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE)
    {
        powerMgrStartAutoPowerMode(hPowerMgr);
    }
 	if ( pPowerMgr->desiredPowerModeProfile != POWER_MODE_AUTO) /*not auto mode - according to the current profle*/
    {
        powerMgrPowerProfileConfiguration(hPowerMgr, pPowerMgr->desiredPowerModeProfile);
    }

	if ((pPowerMgr->betEnable)&&( pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE ))
	{
		TrafficMonitor_StartEventNotif(pPowerMgr->hTrafficMonitor,
									   pPowerMgr->betEnableTMEvent);

		TrafficMonitor_StartEventNotif(pPowerMgr->hTrafficMonitor,
									   pPowerMgr->betDisableTMEvent);
	

		frameCount = TrafficMonitor_GetFrameBandwidth(pPowerMgr->hTrafficMonitor);
	
		if (frameCount < pPowerMgr->BetEnableThreshold) 
		{
			pPowerMgr->betTrafficEnable = TI_TRUE;

		}
		else if (frameCount > pPowerMgr->BetDisableThreshold) 
		{
			pPowerMgr->betTrafficEnable = TI_FALSE;
		}

		PowerMgrConfigBetToFw(hPowerMgr,pPowerMgr->betTrafficEnable);
	}

	/* also start the power manager keep-alive sub module */
	powerMgrKL_start (pPowerMgr->hPowerMgrKeepAlive);

	return TI_OK;
}


/****************************************************************************************
 *                        PowerMgr_stopPS                                                           *
 ****************************************************************************************
DESCRIPTION: Stop the power save algorithm of the driver and also the 802.11 PS.
                                                                                                                               
INPUT:          - hPowerMgr             - Handle to the Power Manager

OUTPUT:     
RETURN:    TI_STATUS - TI_OK or PENDING on success else TI_NOK.\n
****************************************************************************************/
TI_STATUS PowerMgr_stopPS(TI_HANDLE hPowerMgr, TI_BOOL bDisconnect)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_stopPS - called\n");

    if ( pPowerMgr->psEnable == TI_FALSE )
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_stopPS - PS is already Disable! Aborting!\n");
        /*
        Print Info message incase callng PowerMgr_stopPS() more than once in a row, without
        calling to PowerMgr_startPS() in the middle.
        this will return with TI_OK and not doing the Stop action!
        */
        return TI_OK;
    }

    pPowerMgr->psEnable = TI_FALSE;
    tmr_StopTimer (pPowerMgr->hRetryPsTimer);
	tmr_StopTimer(pPowerMgr->hEnterPsGuardTimer);

    /* Check if PsPoll work-around is currently enabled */
    if ( pPowerMgr->powerMngModePriority[POWER_MANAGER_PS_POLL_FAILURE_PRIORITY].priorityEnable == TI_TRUE)
    {
        tmr_StopTimer(pPowerMgr->hPsPollFailureTimer);
        /* Exit the PsPoll work-around */
        powerMgr_PsPollFailureTimeout( hPowerMgr, TI_FALSE );
    }

    if ( pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE)
    {
        powerMgrDisableThresholdsIndications(hPowerMgr);
    }

    /*stop rx auto streaming*/
	if ( pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE && pPowerMgr->desiredPowerModeProfile != POWER_MODE_AUTO)
	{
		qosMngr_UpdatePsTraffic(pPowerMgr->hQosMngr,TI_FALSE);
	}

    if ((pPowerMgr->betEnable)&&( pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE ))
    {
		TrafficMonitor_StopEventNotif(pPowerMgr->hTrafficMonitor,
									  pPowerMgr->betEnableTMEvent);

		TrafficMonitor_StopEventNotif(pPowerMgr->hTrafficMonitor,
									  pPowerMgr->betDisableTMEvent);
	}

   /* also stop the power manager keep-alive sub module */
    powerMgrKL_stop (pPowerMgr->hPowerMgrKeepAlive, bDisconnect);

	pPowerMgr->psCurrentMode 	= POWER_SAVE_OFF;
	pPowerMgr->psLastRequest 	= POWER_SAVE_OFF;

	updatePowerAuthority(pPowerMgr);

    return TI_OK;
}


/****************************************************************************************
 *                        PowerMgr_setPowerMode                                                         *
 ****************************************************************************************
DESCRIPTION: Configure of the PowerMode profile (auto / active / short doze / long doze).
                                                                                                                               
INPUT:          - hPowerMgr             - Handle to the Power Manager
            - thePowerMode      - the requested power mode (auto / active / short doze / long doze).
OUTPUT:     
RETURN:    TI_STATUS - TI_OK on success else TI_NOK.\n
****************************************************************************************/
TI_STATUS PowerMgr_setPowerMode(TI_HANDLE hPowerMgr)
{
    PowerMgr_t           *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    PowerMgr_PowerMode_e  powerMode;
    TI_STATUS             rc = TI_OK;

    /*in this way we will run with the highest priority that is enabled*/
    powerMode = powerMgrGetHighestPriority(hPowerMgr);

    /* sanity checking */
    if ( powerMode >= POWER_MODE_MAX)
    {
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgr_setPowerMode - unknown parameter: %d\n", powerMode);
        return TI_NOK;
    }

    TRACE1( pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_setPowerMode, power mode = %d\n", powerMode);

    if ( pPowerMgr->desiredPowerModeProfile != powerMode )
    {
        PowerMgr_PowerMode_e previousPowerModeProfile;
        previousPowerModeProfile = pPowerMgr->desiredPowerModeProfile;
        pPowerMgr->desiredPowerModeProfile = powerMode;

		/* Disable the auto rx streaming mechanism in case we move from ps state*/
		if (pPowerMgr->desiredPowerModeProfile == POWER_MODE_ACTIVE || pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO)
		{
			qosMngr_UpdatePsTraffic(pPowerMgr->hQosMngr,TI_FALSE);
		}
		if (previousPowerModeProfile != POWER_MODE_ACTIVE)
		{
			powerMgrDisableThresholdsIndications(hPowerMgr);
		}

        if ( pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO )
        {
            if ( pPowerMgr->psEnable == TI_TRUE )
            {
                powerMgrStartAutoPowerMode(hPowerMgr);
            }

            /*
            the transitions of state will be done according to the events from the
            traffic monitor - therefor abort and wait event from the traffic monitor.
            */
            return TI_OK;
        }
		
        if ( pPowerMgr->psEnable == TI_TRUE )
        {
        	if (pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE)
        	{
        		/*active the performance monitor thresholds indication for auto rx streaming*/
        		powerMgrStartAutoPowerMode(hPowerMgr); 
        	}
            rc = powerMgrPowerProfileConfiguration(hPowerMgr, powerMode);
        }

        updatePowerAuthority(pPowerMgr);
    }
    else
    {
        /*
        the power mode is already configure to the module - don't need to do anything!
        */
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_WARNING, "PowerMgr_setPowerMode - desiredPowerModeProfile == thePowerMode (=%d), ABORTING!\n", powerMode);
    }

    return rc;
}


/****************************************************************************************
 *                        PowerMgr_setDozeModeInAuto                                    *
 ****************************************************************************************
DESCRIPTION: Configure the doze mode (short-doze / long-doze) that auto mode will toggle between doze vs active.                                                                                        
INPUT:      - hPowerMgr             - Handle to the Power Manager
            - dozeMode      - the requested doze mode when Mgr is in Auto mode (short-doze / long-doze)
OUTPUT:     
RETURN:   
****************************************************************************************/
void PowerMgr_setDozeModeInAuto(TI_HANDLE hPowerMgr, PowerMgr_PowerMode_e dozeMode)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    PowerMgr_PowerMode_e powerMode = powerMgrGetHighestPriority(hPowerMgr);

    /* check if we are trying to configure the same Doze mode */
    if ( dozeMode != pPowerMgr->autoModeDozeMode )
    {
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_setDozeModeInAuto - autoModeDozeMode == %d \n", dozeMode);

        pPowerMgr->autoModeDozeMode = dozeMode;

        /* in case we are already in Auto mode, we have to set the wake up condition MIB */
        if ( powerMode == POWER_MODE_AUTO )
        {
            if ( dozeMode == POWER_MODE_SHORT_DOZE )
            {
                if ( pPowerMgr->beaconListenInterval > 1 )
                {
                    powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_N_BEACON);       
                }
                else
                {
                    powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_BEACON);     
                }
            }
            else  /* POWER_MODE_LONG_DOZE */
            {
                if ( pPowerMgr->dtimListenInterval > 1 )
                {
                    powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_N_DTIM);       
                }
                else
                {
                    powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_DTIM);     
                }
            }

            TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_setDozeModeInAuto - already in Auto\n");
        }
    }
    else
    {
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_WARNING, "PowerMgr_setDozeModeInAuto - autoModeDozeMode == %d (same same ...)\n", dozeMode);
    }
}

/****************************************************************************************
 *                        PowerMgr_getPowerMode                                                         *
 ****************************************************************************************
DESCRIPTION: Get the current PowerMode of the PowerMgr module. 
                                                                                                                               
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    PowerMgr_PowerMode_e - (auto / active / short doze / long doze).\n
****************************************************************************************/
PowerMgr_PowerMode_e PowerMgr_getPowerMode(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    return pPowerMgr->desiredPowerModeProfile;
}


TI_STATUS powerMgr_setParam(TI_HANDLE thePowerMgrHandle,
                            paramInfo_t *theParamP)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)thePowerMgrHandle;

    switch ( theParamP->paramType )
    {
	case POWER_MGR_POWER_MODE:
        pPowerMgr->powerMngModePriority[theParamP->content.powerMngPowerMode.PowerMngPriority].powerMode
                        = theParamP->content.powerMngPowerMode.PowerMode;
        PowerMgr_setPowerMode(thePowerMgrHandle);
        if (pPowerMgr->betEnable)
			PowerMgrConfigBetToFw(thePowerMgrHandle, pPowerMgr->betEnable );
        break;

	case POWER_MGR_DISABLE_PRIORITY:
        pPowerMgr->powerMngModePriority[theParamP->content.powerMngPriority].priorityEnable = TI_FALSE;
        PowerMgr_setPowerMode(thePowerMgrHandle);
        break;

	case POWER_MGR_ENABLE_PRIORITY:
        pPowerMgr->powerMngModePriority[theParamP->content.powerMngPriority].priorityEnable = TI_TRUE;
        PowerMgr_setPowerMode(thePowerMgrHandle);
        break;

    case POWER_MGR_POWER_LEVEL_PS:
        pPowerMgr->PowerSavePowerLevel = theParamP->content.PowerSavePowerLevel;

        updatePowerAuthority(pPowerMgr);
        break;

    case POWER_MGR_POWER_LEVEL_DEFAULT:
        pPowerMgr->defaultPowerLevel = theParamP->content.DefaultPowerLevel;

        updatePowerAuthority(pPowerMgr);
        break;

    case POWER_MGR_POWER_LEVEL_DOZE_MODE:
        PowerMgr_setDozeModeInAuto(thePowerMgrHandle,theParamP->content.powerMngDozeMode);
        if (pPowerMgr->betEnable)
        PowerMgrConfigBetToFw(thePowerMgrHandle, pPowerMgr->betEnable );
        break;

    case POWER_MGR_KEEP_ALIVE_ENA_DIS:
    case POWER_MGR_KEEP_ALIVE_ADD_REM:
        return powerMgrKL_setParam (pPowerMgr->hPowerMgrKeepAlive, theParamP);
    

    default:
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgr_setParam - ERROR - Param is not supported, %d\n\n", theParamP->paramType);

        return PARAM_NOT_SUPPORTED;
    }

    return TI_OK;
}



TI_STATUS powerMgr_getParam(TI_HANDLE thePowerMgrHandle,
                            paramInfo_t *theParamP)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)thePowerMgrHandle;

    switch ( theParamP->paramType )
    {
    case POWER_MGR_POWER_MODE:
        theParamP->content.PowerMode = PowerMgr_getPowerMode(thePowerMgrHandle);
        break;

    case POWER_MGR_POWER_LEVEL_PS:
        theParamP->content.PowerSavePowerLevel = pPowerMgr->PowerSavePowerLevel;
        break;

    case POWER_MGR_POWER_LEVEL_DEFAULT:
        theParamP->content.DefaultPowerLevel = pPowerMgr->defaultPowerLevel;
        break;

    case POWER_MGR_POWER_LEVEL_DOZE_MODE:
        theParamP->content.powerMngDozeMode = pPowerMgr->autoModeDozeMode;
        break;

    case POWER_MGR_KEEP_ALIVE_GET_CONFIG:
        return powerMgrKL_getParam (pPowerMgr->hPowerMgrKeepAlive, theParamP);
    

    case POWER_MGR_GET_POWER_CONSUMPTION_STATISTICS:
       
       return cmdBld_ItrPowerConsumptionstat (pPowerMgr->hTWD,
                             theParamP->content.interogateCmdCBParams.fCb, 
                             theParamP->content.interogateCmdCBParams.hCb,
                             (void*)theParamP->content.interogateCmdCBParams.pCb);



            


    default:
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgr_getParam - ERROR - Param is not supported, %d\n\n", theParamP->paramType);
        return PARAM_NOT_SUPPORTED;
    }

    return TI_OK;
}


/*****************************************************************************
 **         Private Function prototypes                                     **
 *****************************************************************************/


/****************************************************************************************
 *                        powerSaveReportCB                                             *
 ****************************************************************************************
DESCRIPTION: Callback for the TWD_OWN_EVENT_PS_REPORT event - gets the result of the request 
              for enter PS. Exit PS request will not result in event.
                                                                                                                               
INPUT:      - hPowerMgr             - Handle to the Power Manager
            - str					- Buffer containing the event result.
            - strLen                - Event result length.
OUTPUT:     
RETURN:    void.\n
****************************************************************************************/
static void powerSaveReportCB(TI_HANDLE hPowerMgr, char* str , TI_UINT32 strLen)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    TI_UINT8           PowerSaveStatus;


    /*Report was received - stop the timer*/
    tmr_StopTimer(pPowerMgr->hEnterPsGuardTimer);

    /*copy the report status*/
    os_memoryCopy(pPowerMgr->hOS, (void *)&PowerSaveStatus, (void *)str, sizeof(TI_UINT8));

    TRACE1( pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerSaveReportCB, status = %d\n", PowerSaveStatus);

    /* Handling the event*/
    switch ( (EventsPowerSave_e)PowerSaveStatus )
    {
        case ENTER_POWER_SAVE_FAIL:
            tmr_StartTimer (pPowerMgr->hRetryPsTimer,
                            powerMgrRetryPsTimeout,
                            (TI_HANDLE)pPowerMgr,
                            RE_ENTER_PS_TIMEOUT,
                            TI_FALSE);
            break;

        case ENTER_POWER_SAVE_SUCCESS:
            /* Update mode to PS_ON only if it wasn't followed by PS_OFF request! */
            if (pPowerMgr->psLastRequest == POWER_SAVE_ON)
            {
                pPowerMgr->psCurrentMode = POWER_SAVE_ON;
                updatePowerAuthority(pPowerMgr);
                invokeCallback(pPowerMgr->fEnteredPsCb, pPowerMgr->hEnteredPsCb);
            }
            break;

        default:
            TRACE1( pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "powerSaveReportCB: invliad status: %d\n", PowerSaveStatus);
            break;
    }
}

/**
 * \\n
 * \date 30-Aug-2006\n
 * \brief Power manager callback fro TM event notification
 *
 * Function Scope \e Public.\n
 * \param hPowerMgr - handle to the power maanger object.\n
 * \param cookie - values supplied during event registration (active / doze).\n
 */
static void PowerMgrTMThresholdCrossCB( TI_HANDLE hPowerMgr, TI_UINT32 cookie )
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgrTMThresholdCrossCB - TM notified threshold crossed, cookie: %d\n", cookie);

    /* sanity cehcking - TM notifications should only be received when PM is enabled and in auto mode */
    if ( (pPowerMgr->psEnable == TI_TRUE) && (pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO))
    {
        switch ((PowerMgr_PowerMode_e)cookie)
        {
        case POWER_MODE_ACTIVE:
            powerMgrPowerProfileConfiguration( hPowerMgr, POWER_MODE_ACTIVE );
            break;

        /* threshold crossed down - need to enter configured doze mode */
        case POWER_MODE_SHORT_DOZE:
            powerMgrPowerProfileConfiguration( hPowerMgr, pPowerMgr->autoModeDozeMode );
            break;

        default:
            TRACE1( pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgrTMThresholdCrossCB: TM notification with invalid cookie: %d!\n", cookie);
            break;
        }
    }
	else if ((pPowerMgr->psEnable == TI_TRUE) && (pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE))
	{
		TI_BOOL bPsTrafficOn = ((PowerMgr_PowerMode_e)cookie == POWER_MODE_ACTIVE) ? TI_TRUE : TI_FALSE;
		qosMngr_UpdatePsTraffic(pPowerMgr->hQosMngr,bPsTrafficOn);
	}
    else
    {
        TRACE2( pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgrTMThresholdCrossCB: TM motification when psEnable is :%d or desired profile is: %d\n", pPowerMgr->psEnable, pPowerMgr->desiredPowerModeProfile);
    }

}

/****************************************************************************************
*                        powerMgrDisableThresholdsIndications                                           *
*****************************************************************************************
DESCRIPTION: This will send a disable message to the traffic monitor,
                 to stop sending indications on threshold pass.

                                                                                                                              
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    void.\n
****************************************************************************************/
static void powerMgrDisableThresholdsIndications(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    /*
    auto is not a static/fix state, else its a dynamic state that flows between
    the 3 static/fix states: active, short-doze and long-doze.
    */
    TrafficMonitor_StopEventNotif(pPowerMgr->hTrafficMonitor,
                                  pPowerMgr->passToActiveTMEvent);

    TrafficMonitor_StopEventNotif(pPowerMgr->hTrafficMonitor,
                                  pPowerMgr->passToDozeTMEvent);

}


/****************************************************************************************
*                        powerMgrEnableThresholdsIndications                                            *
*****************************************************************************************
DESCRIPTION: TThis will send an enable message to the traffic monitor,
                to start sending indications on threshold pass.

                                                                                                                              
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    void.\n
****************************************************************************************/
static void powerMgrEnableThresholdsIndications(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    TRACE0( pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerMgrEnableThresholdsIndications called\n");
    /*
    auto is not a static/fix state, but rather a dynamic state that flows between
    the 3 static/fix states: active, short-doze and long-doze.
    */
    TrafficMonitor_StartEventNotif(pPowerMgr->hTrafficMonitor,
                                   pPowerMgr->passToActiveTMEvent);

    TrafficMonitor_StartEventNotif(pPowerMgr->hTrafficMonitor,
                                   pPowerMgr->passToDozeTMEvent);

}


/****************************************************************************************
*                        powerMgrStartAutoPowerMode                                                 *
*****************************************************************************************
DESCRIPTION: configure the power manager to enter into AUTO power mode. 
             The power manager will deside what power level will be applied 
             acording to the traffic monitor.
                                                                                                                              
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    void.\n
****************************************************************************************/
static void powerMgrStartAutoPowerMode(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    int frameCount;

    frameCount = TrafficMonitor_GetFrameBandwidth(pPowerMgr->hTrafficMonitor);

    TRACE0( pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerMgrStartAutoPowerMode: Starting auto power mode,");

    /*Activates the correct profile*/
    /*Activate the active profile just in case frame count bigger than active TH and bigger than 0*/
    if ( frameCount >= pPowerMgr->autoModeActiveTH && frameCount > 0)
    {
    	if (pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO)
    		{
	        	powerMgrPowerProfileConfiguration(hPowerMgr, POWER_MODE_ACTIVE);
    		}
		else if(pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE)
			{
				/*In power save mode inform the qos manager that auto rx streaming can be activated*/
				qosMngr_UpdatePsTraffic(pPowerMgr->hQosMngr,TI_TRUE);
			}
    }
    else
    {
    	if (pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO)
    		{
	        	powerMgrPowerProfileConfiguration(hPowerMgr, pPowerMgr->autoModeDozeMode);
    		}
		else if(pPowerMgr->desiredPowerModeProfile != POWER_MODE_ACTIVE)
			{
				/*In power save mode inform the qos manager that auto rx streaming should be deactivated*/
				qosMngr_UpdatePsTraffic(pPowerMgr->hQosMngr,TI_FALSE);
			}

    }
    /* Activates the Trafic monitoe Events*/        
    powerMgrEnableThresholdsIndications(hPowerMgr);
}

/****************************************************************************************
*                        powerMgrRetryPsTimeout                                                     *
*****************************************************************************************
DESCRIPTION: Retry function if a PS/exit PS request failed
                                                                                                                              
INPUT:      hPowerMgr       - Handle to the Power Manager
            bTwdInitOccured - Indicates if TWDriver recovery occured since timer started 

OUTPUT:     

RETURN:    void.\n
****************************************************************************************/
static void powerMgrRetryPsTimeout(TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerMgrRetryPsTimeout: timer expired.\n");

    if (pPowerMgr->lastPsTransaction == ENTER_POWER_SAVE_FAIL)
    {
		powerMgrSetPsMode (hPowerMgr, POWER_SAVE_ON, NULL, NULL);
    }
    
	return;
}


/****************************************************************************************
*                        powerMgrPowerProfileConfiguration                                          *
*****************************************************************************************
DESCRIPTION: This function is the " builder " of the Power Save profiles. 
             acording to the desired Power mode.
                                                                                                                              
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    void.\n
****************************************************************************************/
static TI_STATUS powerMgrPowerProfileConfiguration(TI_HANDLE hPowerMgr, PowerMgr_PowerMode_e desiredPowerMode)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    TI_STATUS   rc = TI_NOK;

    tmr_StopTimer (pPowerMgr->hRetryPsTimer);

	pPowerMgr->lastPowerModeProfile = desiredPowerMode;

    switch ( desiredPowerMode )
    {
    case POWER_MODE_AUTO:
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMode==AUTO - This mode should not be sent to the GWSI - we send AUTO instead\n");
        break;

    case POWER_MODE_ACTIVE:
        /* set AWAKE through */
        rc = powerMgrSetPsMode (hPowerMgr, POWER_SAVE_OFF, NULL, NULL);

        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMode==ACTIVE\n");
        break;

    case POWER_MODE_SHORT_DOZE:
        if ( pPowerMgr->beaconListenInterval > 1 )
        {
            powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_N_BEACON);       
        }
        else
        {
            powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_BEACON);     
        }

        rc = powerMgrSetPsMode (hPowerMgr, POWER_SAVE_ON, NULL, NULL);

        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMode==SHORT_DOZE\n");
        break;

    case POWER_MODE_LONG_DOZE:
        if ( pPowerMgr->dtimListenInterval > 1 )
        {
            powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_N_DTIM);       
        }
        else
        {
            powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_DTIM);     
        }

        rc = powerMgrSetPsMode (hPowerMgr, POWER_SAVE_ON, NULL, NULL);

        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMode==LONG_DOZE\n");
        break;

	case POWER_MODE_PS_ONLY:
		/* When in SG PS mode, configure the user desired wake-up condition */
		powerMgr_SGSetUserDesiredwakeUpCond(pPowerMgr);

        rc = powerMgrSetPsMode (hPowerMgr, POWER_SAVE_ON, NULL, NULL);
        
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMode==PS_ONLY\n");
        break;

    default:
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PowerMgr_setWakeUpConfiguration - ERROR - PowerMode - unknown parameter: %d\n", desiredPowerMode);
        return TI_NOK;
    }

    return rc;
}


/****************************************************************************************
*                        powerMgrSendMBXWakeUpConditions                                            *
*****************************************************************************************
DESCRIPTION: Tsend configuration of the power management option that holds in the command
                mailbox inner sturcture.
                                                                                                                              
INPUT:          - hPowerMgr             - Handle to the Power Manager
OUTPUT:     
RETURN:    TI_STATUS - TI_OK on success else TI_NOK.\n
****************************************************************************************/
static TI_STATUS powerMgrSendMBXWakeUpConditions(TI_HANDLE hPowerMgr,
                                                 TI_UINT8 listenInterval,
                                                 ETnetWakeOn tnetWakeupOn)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    TPowerMgmtConfig powerMgmtConfig;
    TI_STATUS status = TI_OK;

    powerMgmtConfig.listenInterval = listenInterval;
    powerMgmtConfig.tnetWakeupOn = tnetWakeupOn;

    TRACE2(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerMgrSendMBXWakeUpConditions: listenInterval = %d, tnetWakeupOn = %d\n", listenInterval,tnetWakeupOn);

    status = TWD_CfgWakeUpCondition (pPowerMgr->hTWD, &powerMgmtConfig);
                                      
    if ( status != TI_OK )
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "powerMgrSendMBXWakeUpConditions - Error in wae up condition IE!\n");
    }
    return status;
}



static PowerMgr_PowerMode_e powerMgrGetHighestPriority(TI_HANDLE hPowerMgr)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    int index;
    for ( index = POWER_MANAGER_MAX_PRIORITY-1;index >= 0;index-- )
    {
        if ( pPowerMgr->powerMngModePriority[index].priorityEnable )
        {

            return pPowerMgr->powerMngModePriority[index].powerMode;
        }

    }

    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "powerMgrGetHighestPriority - error - faild to get highest priority! sefault deseired mode was returned !!!\n");
    return pPowerMgr->desiredPowerModeProfile;
}


 /****************************************************************************************
 *                        PowerMgr_notifyFWReset															*
 ****************************************************************************************
DESCRIPTION: Notify the object of the power Manager about FW reset (recovery).
			 Calls PowerSrv module to Set Ps Mode
				                                                                                                   
INPUT:      - hPowerMgr - Handle to the Power Manager	
OUTPUT:		
RETURN:    TI_STATUS - TI_OK on success else TI_NOK.
****************************************************************************************/
TI_STATUS PowerMgr_notifyFWReset(TI_HANDLE hPowerMgr)
{
	PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    TRACE2(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgr_notifyFWReset(): psEnable = %d, lastPowerModeProfile = %d\n", pPowerMgr->psEnable, pPowerMgr->lastPowerModeProfile);

	pPowerMgr->psCurrentMode = POWER_SAVE_OFF;
	pPowerMgr->psLastRequest = POWER_SAVE_OFF;

	if (pPowerMgr->psEnable)
	{
        powerMgrPowerProfileConfiguration(hPowerMgr, pPowerMgr->lastPowerModeProfile);
	}

	pPowerMgr->eLastPowerAuth = POWERAUTHO_POLICY_NUM; /* Set to invalid value to force update (needed to keep TWD synced) */
	updatePowerAuthority(pPowerMgr);

    return TI_OK;
}


/****************************************************************************************
 *                        PowerMgrConfigBetToFw															*
 ****************************************************************************************
DESCRIPTION: callback from TM event notification.
				-	call PowerSrv module to Set Ps Mode
				                                                                                                   
INPUT:      	- hPowerMgr - Handle to the Power Manager	
                - BetEnable - cookie:values supplied during event registration
OUTPUT:		
RETURN:    None.
****************************************************************************************/
static void PowerMgrConfigBetToFw( TI_HANDLE hPowerMgr, TI_UINT32 BetEnable )
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    TI_UINT8 MaximumConsecutiveET;
    TI_UINT32 listenInterval;
    paramInfo_t param;
    TI_UINT32 beaconInterval;
    TI_UINT32 dtimPeriod;
    PowerMgr_PowerMode_e powerMode;

    param.paramType = SITE_MGR_BEACON_INTERVAL_PARAM;
    siteMgr_getParam(pPowerMgr->hSiteMgr, &param);
    beaconInterval = param.content.beaconInterval;

    param.paramType = SITE_MGR_DTIM_PERIOD_PARAM;
    siteMgr_getParam(pPowerMgr->hSiteMgr, &param);
    dtimPeriod = param.content.siteMgrDtimPeriod;

    /* get actual Power Mode */
    if (pPowerMgr->desiredPowerModeProfile == POWER_MODE_AUTO)
    {
        powerMode = pPowerMgr->autoModeDozeMode;
    }
    else
    {
        powerMode = pPowerMgr->lastPowerModeProfile;
    }

    /* calc ListenInterval */
    if (powerMode == POWER_MODE_SHORT_DOZE)
    {
        listenInterval = beaconInterval * pPowerMgr->beaconListenInterval;
    }
    else if (powerMode == POWER_MODE_LONG_DOZE)
    {
        listenInterval = dtimPeriod * beaconInterval * pPowerMgr->dtimListenInterval;
    }
    else
    {
        listenInterval = beaconInterval;
    }

    if (listenInterval == 0)
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_WARNING, "PowerMgrConfigBetToFw: listenInterval is ZERO\n");
        return;
    }

    /* MaximumConsecutiveET = MaximalFullBeaconReceptionInterval / MAX( BeaconInterval, ListenInterval) */
    MaximumConsecutiveET = pPowerMgr->maxFullBeaconInterval / listenInterval;

    TRACE5(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "PowerMgrConfigBetToFw:\n                           Power Mode = %d\n                           beaconInterval = %d\n                           listenInterval = %d\n                           Bet Enable = %d\n                           MaximumConsecutiveET = %d\n", powerMode, beaconInterval, listenInterval, BetEnable, MaximumConsecutiveET);

    pPowerMgr->betEnable = BetEnable; /* save BET enable flag for CLI configuration */

    TWD_CfgBet(pPowerMgr->hTWD, BetEnable, MaximumConsecutiveET);
}

/**
 * \date 10-April-2007\n
 * \brief Returns to the configured wakeup condition, when SG protective mode is done
 *
 * Function Scope \e Public.\n
 * Parameters:\n
 * 1) TI_HANDLE - handle to the PowerMgr object.\n
 * Return Value: void.\n
 */
static void powerMgr_SGSetUserDesiredwakeUpCond( TI_HANDLE hPowerMgr )
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

   	if (pPowerMgr->psEnable)
	{
		/* set wakeup condition according to user mode power save profile */
		switch ( pPowerMgr->powerMngModePriority[ POWER_MANAGER_USER_PRIORITY ].powerMode )
		{
		case POWER_MODE_AUTO:
			/*set wakeup condition according to doze mode in auto and wakup interval */
			if ( pPowerMgr->autoModeDozeMode == POWER_MODE_SHORT_DOZE )
			{
				/* short doze */
				if ( pPowerMgr->beaconListenInterval > 1 )
				{
					powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_N_BEACON);       
				}
				else
				{
					powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_BEACON);     
				}
			}
			else
			{
				/* long doze */
				if ( pPowerMgr->dtimListenInterval > 1 )
				{
					powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_N_DTIM);       
				}
				else
				{
					powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_DTIM);     
				}
			}
			break;
	
		case POWER_MODE_ACTIVE:
			break;
	
		case POWER_MODE_SHORT_DOZE:
			if ( pPowerMgr->beaconListenInterval > 1 )
			{
				powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_N_BEACON);       
			}
			else
			{
				powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->beaconListenInterval,TNET_WAKE_ON_BEACON);     
			}
			break;
	
		case POWER_MODE_LONG_DOZE:
			if ( pPowerMgr->dtimListenInterval > 1 )
			{
				powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_N_DTIM);       
			}
			else
			{
				powerMgrSendMBXWakeUpConditions(hPowerMgr,pPowerMgr->dtimListenInterval,TNET_WAKE_ON_DTIM);     
			}
			break;
	
		default:
TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, ": ERROR - PowerMode for user prioirty is: %d\n", pPowerMgr->powerMngModePriority[ POWER_MANAGER_USER_PRIORITY ].powerMode);
		}
	}/*end of if (psEnable)*/
}



/****************************************************************************************
*                        PowerMgr_PsPollFailureCB															*
****************************************************************************************
DESCRIPTION: Work around to solve AP bad behavior.
         Some old AP's have trouble with Ps-Poll - The solution will be to exit PS for a 
         period of time
				                                                                                               
INPUT:      	- hPowerMgr - Handle to the Power Manager	
OUTPUT:		
RETURN:    
****************************************************************************************/
static void PowerMgr_PsPollFailureCB( TI_HANDLE hPowerMgr )
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    if ( pPowerMgr->PsPollDeliveryFailureRecoveryPeriod )
    {
        paramInfo_t param;
             
        TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_WARNING, " Oh boy, AP is not answering Ps-Poll's. enter active PS for %d Ms\n", pPowerMgr->PsPollDeliveryFailureRecoveryPeriod);

        /*
         * Set the system to Active power save 
         */
        param.paramType = POWER_MGR_POWER_MODE;
        param.content.powerMngPowerMode.PowerMode = POWER_MODE_ACTIVE;
        param.content.powerMngPowerMode.PowerMngPriority = POWER_MANAGER_PS_POLL_FAILURE_PRIORITY;
        powerMgr_setParam(hPowerMgr,&param);
        
        param.paramType = POWER_MGR_ENABLE_PRIORITY;
        param.content.powerMngPriority = POWER_MANAGER_PS_POLL_FAILURE_PRIORITY;
        powerMgr_setParam(hPowerMgr,&param);

        /*
         * Set timer to exit the active mode
         */
        tmr_StartTimer(pPowerMgr->hPsPollFailureTimer,
					   powerMgr_PsPollFailureTimeout,
					   (TI_HANDLE)pPowerMgr,
					   pPowerMgr->PsPollDeliveryFailureRecoveryPeriod,
					   TI_FALSE);
    } 
    else    /* Work-around is disabled */
    {
        TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_WARNING, " Oh boy, AP is not answering Ps-Poll's !!!\n");
    }
	return;
}

/****************************************************************************************
*                        powerMgr_PsPollFailureTimeout									*
****************************************************************************************
DESCRIPTION: After the timeout of ps-poll failure - return to normal behavior
				                                                                                               
INPUT:      	- hPowerMgr - Handle to the Power Manager	
OUTPUT:		
RETURN:    
****************************************************************************************/
static void powerMgr_PsPollFailureTimeout( TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured )
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;
    paramInfo_t param;

    TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, " \n");

    /* disable Ps-Poll priority */
    param.paramType = POWER_MGR_DISABLE_PRIORITY;
    param.content.powerMngPriority = POWER_MANAGER_PS_POLL_FAILURE_PRIORITY;
    powerMgr_setParam(hPowerMgr,&param);

	return;
}

TI_BOOL PowerMgr_getReAuthActivePriority(TI_HANDLE thePowerMgrHandle)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)thePowerMgrHandle;
	return pPowerMgr->reAuthActivePriority;
}

/***************************************************************************************
*                        powerMgrSetPsMode  										   *
****************************************************************************************
DESCRIPTION: Set the power save mode by calling TWD API.
				                                                                                               
INPUT:      	- hPowerMgr - Handle to the Power Manager	
OUTPUT:		
RETURN:    
****************************************************************************************/
static TI_STATUS powerMgrSetPsMode (TI_HANDLE hPowerMgr, E80211PsMode psMode,
                               TI_HANDLE hPowerSaveCompleteCb, TPowerSaveResponseCb fCb)
{
	PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;


    
    /* if already in required PS mode - don't send the request*/
	if (pPowerMgr->psCurrentMode == pPowerMgr->psLastRequest)
	{
        if (pPowerMgr->psCurrentMode == psMode)
        {
        	TRACE1(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION, "powerMgrSetPsMode: ignoring because psCurrentMode==psLastRequest==psMode==%d\n", psMode);
			return TI_OK;
        }
	}

    /* timer might be running - stop it before sending the new request*/
    tmr_StopTimer(pPowerMgr->hEnterPsGuardTimer);

    
	if (psMode == POWER_SAVE_ON)
	{
		pPowerMgr->psLastRequest = POWER_SAVE_ON;
		/* if enter PS -> start the guard timer, which is used to protect from FW stuck */
        tmr_StartTimer (pPowerMgr->hEnterPsGuardTimer,
						powerMgrGuardTimerExpired,
						hPowerMgr,
						POWER_SAVE_GUARD_TIME_MS,
						TI_FALSE);

        /* Power Authority will be updated when Power Save is effectively ON (when
         * the call-back powerSaveReportCB() is called) */
	}
	else
	{
        pPowerMgr->psLastRequest = pPowerMgr->psCurrentMode = POWER_SAVE_OFF;

        updatePowerAuthority(pPowerMgr);
	}

    /* call the TWD API*/
    TWD_SetPsMode (pPowerMgr->hTWD, psMode, hPowerSaveCompleteCb, fCb);

    return TI_PENDING;
}

/****************************************************************************************
*                               powerMgrGuardTimerExpired                               *
*****************************************************************************************
DESCRIPTION: This function is called upon timer expiry - when the FW has not returned
             a response within the defined timeout
                                                                                                                  
INPUT:      hPowerMgr     - handle to the PowerMgr object.
            bTwdInitOccured - Indicates if TWDriver recovery occured since timer started 

OUTPUT:    None

RETURN:    None
****************************************************************************************/
static void powerMgrGuardTimerExpired (TI_HANDLE hPowerMgr, TI_BOOL bTwdInitOccured)
{
    PowerMgr_t *pPowerMgr = (PowerMgr_t*)hPowerMgr;

    /* Print an error message */
	TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_ERROR, "PS guard timer expired!\n");
    
    /* Trigger PS recovery */
    healthMonitor_sendFailureEvent(pPowerMgr->hHealthMonitor, POWER_SAVE_FAILURE);
}

/****************************************************************************************
*                               updatePowerAuthority                                    *
*****************************************************************************************
DESCRIPTION: Updates the minimum power-level the HW can be in, according to the state
             of the Power Manager module.
             Should be called whenever the Power Manager changes its state.

INPUT:       hPowerMgr - handle to the PowerMgr object
OUTPUT:      None
RETURN:      TI_OK if the Power Authority was changed. TI_NOK otherwise.
****************************************************************************************/
static TI_STATUS updatePowerAuthority(TI_HANDLE hPowerMgr)
{
	PowerMgr_t*  pPowerMgr = (PowerMgr_t*)hPowerMgr;
	EPowerPolicy ePowerAuth;  /* New Power Authority to set */
	TI_STATUS    status;

	if (pPowerMgr->psEnable)
	{
		if (POWER_SAVE_OFF == pPowerMgr->psCurrentMode)
		{
			/* If the Power Manager is enabled, but NOT in Power Save mode,
			 * set Power Authority to AWAKE - don't allow the HW to "fall asleep" */
			ePowerAuth = POWERAUTHO_POLICY_AWAKE;
		}
		else
		{
			/* If the Power Manager is enabled, and in Power Save mode,
			 * set Power Authority to the default level for "connected and in
			 * power-save" (as set in the INI file) */
			ePowerAuth = pPowerMgr->PowerSavePowerLevel;
		}
	}
	else
	{
		/* If the Power Manager is disabled, set Power Authority to the default level (as
		 * configured in the INI file) */

		ePowerAuth = pPowerMgr->defaultPowerLevel;
	}

	if (ePowerAuth == pPowerMgr->eLastPowerAuth)
	{
		/* Don't set - same as last one */

		TRACE3(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION,
			"psEnable=%d, psCurrentMode=%d. Ignoring request to set PowerAuthority to %d (same as last)\n",
			pPowerMgr->psEnable, pPowerMgr->psCurrentMode, ePowerAuth);
		status = TI_NOK;
	}
	else
	{
		/* Set the new Power Authority */

		TRACE3(pPowerMgr->hReport, REPORT_SEVERITY_INFORMATION,
				"psEnable=%d, psCurrentMode=%d. Setting PowerAuthority to %d\n",
				pPowerMgr->psEnable, pPowerMgr->psCurrentMode, ePowerAuth);

		status = TWD_CfgSleepAuth(pPowerMgr->hTWD, ePowerAuth);

		if (TI_OK==status)
		{
			pPowerMgr->eLastPowerAuth = ePowerAuth;
		}
	}

	return status;
}

/*
 * \brief	Attempts to put the Power Manager in long-doze mode
 *
 * 			Sets Long Doze PS mode, NDTIM, and RX Filter as specified in tConfig
 *
 * \param	hPowerMgr	handle to the Power Manager module
 * \param	tConfig		settings for the long-doze mode
 * \param	fCb			call-back to invoke upon successful entrance to Power Save mode
 * \param	hCb			handle for fCb
 *
 * \return	TI_OK if the Power Manager is already suspended; TI_PENDING if suspend process was started (upon successful suspension, fCb will be invoked); TI_NOK if failed to start suspending
 *
 */
TI_STATUS powerMgr_Suspend(TI_HANDLE hPowerMgr, TPwrStateCfg *pConfig, void (*fCb)(TI_HANDLE), TI_HANDLE hCb)
{
	PowerMgr_t*  pPowerMgr = (PowerMgr_t*)hPowerMgr;
	TI_STATUS    rc;

	if (pConfig == NULL)
	{
		TRACE0(pPowerMgr->hReport, REPORT_SEVERITY_ERROR,"powerMgr_Suspend: tConfig is NULL. aborting");
		return TI_NOK;
	}

	/*
	 * setup completion callback
	 */

	pPowerMgr->fEnteredPsCb = fCb;
	pPowerMgr->hEnteredPsCb = hCb;

	/*
	 * setup RX Data filters
	 */
	if (pConfig->eSuspendFilterUsage == PWRSTATE_FILTER_USAGE_RXFILTER)
	{
		/* save current config */
		rxData_GetRxDataFilters(pPowerMgr->hRxData,
					pPowerMgr->tPreSuspendConfig.tRxDataFilters.aValues,
					&pPowerMgr->tPreSuspendConfig.tRxDataFilters.uCount);
		pPowerMgr->tPreSuspendConfig.tRxDataFilters.bEnabled = rxData_IsRxDataFiltersEnabled(pPowerMgr->hRxData);

		/* set new config */
		rxData_SetRxDataFilters(pPowerMgr->hRxData, &pConfig->tSuspendRxFilterValue, 1);
		rxData_enableDisableRxDataFilters(pPowerMgr->hRxData, TI_TRUE);

		pPowerMgr->tPreSuspendConfig.tRxDataFilters.bChanged = TI_TRUE;
	}
	else
	{
		pPowerMgr->tPreSuspendConfig.tRxDataFilters.bChanged = TI_FALSE;
	}

	/*
	 * setup Long Doze mode
	 */

	/* save current config */
	pPowerMgr->tPreSuspendConfig.dtimListenInterval = pPowerMgr->dtimListenInterval; /* save for resuming later */

	/* set new config */
	pPowerMgr->powerMngModePriority[POWER_MANAGER_PWR_STATE_PRIORITY].priorityEnable = TI_TRUE;
	pPowerMgr->powerMngModePriority[POWER_MANAGER_PWR_STATE_PRIORITY].powerMode = POWER_MODE_LONG_DOZE;
	pPowerMgr->dtimListenInterval = pConfig->uSuspendNDTIM;
	pPowerMgr->desiredPowerModeProfile = POWER_MODE_SHORT_DOZE; /* anything other than POWER_MODE_LONG_DOZE to force PowerMgr_setPowerMode() not to ignore the request */
	rc = PowerMgr_setPowerMode(pPowerMgr);

	/* notify if completed successfully (skip if pending/failed) */
	if (rc == TI_OK)
	{
		invokeCallback(pPowerMgr->fEnteredPsCb, pPowerMgr->hEnteredPsCb);
	}

	return rc;
}

/*
 * \brief	Reverts powerMgr_Suspend() actions
 *
 * 			Restores RX Data Filters; restores Power-Save mode
 *
 * \param	hPowerMgr	this module
 *
 * \return	TI_OK
 *
 */
TI_STATUS powerMgr_Resume(TI_HANDLE hPowerMgr)
{
	PowerMgr_t*  pPowerMgr = (PowerMgr_t*)hPowerMgr;

	/*
	 * restore RX Data filters
	 */
	if (pPowerMgr->tPreSuspendConfig.tRxDataFilters.bChanged)
	{
		rxData_SetRxDataFilters(pPowerMgr->hRxData,
				pPowerMgr->tPreSuspendConfig.tRxDataFilters.aValues,
				pPowerMgr->tPreSuspendConfig.tRxDataFilters.uCount);

		rxData_enableDisableRxDataFilters(pPowerMgr->hRxData,
				pPowerMgr->tPreSuspendConfig.tRxDataFilters.bEnabled);
	}

	/*
	 * restore PS mode
	 */
	pPowerMgr->powerMngModePriority[POWER_MANAGER_PWR_STATE_PRIORITY].priorityEnable = TI_FALSE;
	pPowerMgr->dtimListenInterval = pPowerMgr->tPreSuspendConfig.dtimListenInterval;
	PowerMgr_setPowerMode(pPowerMgr);

	return TI_OK;
}



