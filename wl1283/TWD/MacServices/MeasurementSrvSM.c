/*
 * MeasurementSrvSM.c
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

/** \file measurementSrv.c
 *  \brief This file include the measurement SRV state machine implementation.
 *  \
 *  \date 13-November-2005
 */

#define __FILE_ID__  FILE_ID_112
#include "osApi.h"
#include "report.h"
#include "MeasurementSrvSM.h"
#include "MeasurementSrv.h"
#include "timer.h"
#include "fsm.h"
#include "TWDriverInternal.h"
#include "CmdBld.h"



TI_STATUS actionUnexpected( TI_HANDLE hMeasurementSrv );
TI_STATUS actionNop( TI_HANDLE hMeasurementSrv );
static void measurementSRVSM_requestMeasureStartResponseCB(TI_HANDLE hMeasurementSRV, TI_UINT32 uMboxStatus);


/**
 * \\n
 * \date 08-November-2005\n
 * \brief Initialize the measurement SRV SM.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return TI_OK if successful, TI_NOK otherwise.\n
 */
TI_STATUS measurementSRVSM_init( TI_HANDLE hMeasurementSRV )
{
   measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;

    fsm_actionCell_t    smMatrix[ MSR_SRV_NUM_OF_STATES ][ MSR_SRV_NUM_OF_EVENTS ] =
    {
        /* next state and actions for IDLE state */
        {   
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_START, measurementSRVSM_requestMeasureStart}, /*"MESSURE_START_REQUEST"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_SUCCESS"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_FAILURE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"ALL_TYPES_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"STOP_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, measurementSRVSRVSM_dummyStop},                          /*"MEASURE_STOP_REQUEST"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected}                                        /*AP_DISCOVERY*/
        },

        /* next state and actions for WAIT_FOR_MEASURE_START state */
        {    
            {MSR_SRV_STATE_WAIT_FOR_AP_DISCOVERY_START, measurementSRVSM_waitApDiscovery},  /*"MESSURE_START_REQUEST"*/
            {MSR_SRV_STATE_MEASURE_IN_PROGRESS, measurementSRVSM_startMeasureTypesSuccess}, /*"START_SUCCESS"*/
            {MSR_SRV_STATE_IDLE, measurementSRVSM_measureStartFailure},                   /*"START_FAILURE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"ALL_TYPES_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, measurementSRVSM_completeMeasure},                       /*"STOP_COMPLETE"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, measurementSRVSM_stopFromWaitForMeasureStart},
                                                                                          /*"MEASURE_STOP_REQUEST"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_START, actionUnexpected}                      /*AP_DISCOVERY*/
        },


        /* next state and actions for MSR_SRV_STATE_WAIT_FOR_AP_DISCOVERY_START state */
        {
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"MESSURE_START_REQUEST"*/
            {MSR_SRV_STATE_MEASURE_IN_PROGRESS, measurementSRVSM_startMeasureTypesSuccess}, /*"START_SUCCESS"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_FAILURE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"ALL_TYPES_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, measurementSRVSM_completeMeasure},                       /*"STOP_COMPLETE"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, measurementSRVSM_stopFromWaitForMeasureStart}, /*"MEASURE_STOP_REQUEST"*/
            {MSR_SRV_STATE_MEASURE_IN_PROGRESS, measurementSRVSM_startApDiscovery}        /*AP_DISCOVERY*/
        },

        /* next state and actions for MEASURE_IN_PROGRESS state */
        {   
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"MESSURE_START_REQUEST"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_SUCCESS"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_FAILURE"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, measurementSRVSM_requestMeasureStop},   /*"ALL_TYPES_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"STOP_COMPLETE"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, measurementSRVSM_stopFromMeasureInProgress},
                                                                                          /*"MEASURE_STOP_REQUEST"*/
            {MSR_SRV_STATE_MEASURE_IN_PROGRESS, measurementSRVSM_apDiscoveryStopComplete} /*AP_DISCOVERY*/

        },

        /* next state and actions for WAIT_FOR_MEASURE_STOP state */
        {   
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"MESSURE_START_REQUEST"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_SUCCESS"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"START_FAILURE"*/
            {MSR_SRV_STATE_IDLE, actionUnexpected},                                       /*"ALL_TYPES_COMPLETE"*/
            {MSR_SRV_STATE_IDLE, measurementSRVSM_completeMeasure},                       /*"STOP_COMPLETE"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, measurementSRVSRVSM_dummyStop},         /*"MEASURE_STOP_REQUEST"*/
            {MSR_SRV_STATE_WAIT_FOR_MEASURE_STOP, actionUnexpected}                                        /*AP_DISCOVERY_STOP_COMPLETE*/
        }
    };

    /* initialize current state */
    pMeasurementSRV->SMState = MSR_SRV_STATE_IDLE;

    /* configure the state machine */
    return fsm_Config( pMeasurementSRV->SM, (fsm_Matrix_t)smMatrix, 
                       (TI_UINT8)MSR_SRV_NUM_OF_STATES, (TI_UINT8)MSR_SRV_NUM_OF_EVENTS, 
                       (fsm_eventActivation_t)measurementSRVSM_SMEvent, pMeasurementSRV->hOS );
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief Processes an event.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the measurement SRV object.\n
 * \param currentState - the current scan SRV SM state.\n
 * \param event - the event to handle.\n
 * \return TI_OK if successful, TI_NOK otherwise.\n
 */
TI_STATUS measurementSRVSM_SMEvent( TI_HANDLE hMeasurementSrv, measurements_SRVSMStates_e* currentState, 
                                    measurement_SRVSMEvents_e event )
{
    measurementSRV_t *pMeasurementSRV = (measurementSRV_t *)hMeasurementSrv;
    TI_STATUS status = TI_OK;
    TI_UINT8 nextState;

    /* obtain the next state */
    status = fsm_GetNextState( pMeasurementSRV->SM, (TI_UINT8)*currentState, (TI_UINT8)event, &nextState );
    if ( status != TI_OK )
    {
        TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, "measurementSRVSM_SMEvent: State machine error, failed getting next state\n");
        return TI_NOK;
    }

    /* report the move */
	TRACE3( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, "measurementSRVSM_SMEvent: <currentState = %d, event = %d> --> nextState = %d\n", currentState, event, nextState);

    /* move */
    return fsm_Event( pMeasurementSRV->SM, (TI_UINT8*)currentState, (TI_UINT8)event, hMeasurementSrv );
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief Handle a DRIVER_MODE_SUCCESS event by sending start measure command to the FW.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_requestMeasureStart( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t     *pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TMeasurementParams    pMeasurementCmd;
    TI_STATUS             status;
    TI_UINT32                currentTime = os_timeStampMs( pMeasurementSRV->hOS );

    /* check if request time has expired (note: timer wrap-around is also handled)*/
    if ( (pMeasurementSRV->requestRecptionTimeStampMs + pMeasurementSRV->timeToRequestExpiryMs)
                    < currentTime )
    {
        TI_INT32 i;

        TRACE2( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": request time has expired, request expiry time:%d, current time:%d\n", pMeasurementSRV->requestRecptionTimeStampMs + pMeasurementSRV->timeToRequestExpiryMs, currentTime);

        /* mark that all measurement types has failed */
        for ( i = 0; i < pMeasurementSRV->msrRequest.numberOfTypes; i++ )
        {
            pMeasurementSRV->msrReply.msrTypes[ i ].status = TI_NOK;
        }

        /* send a measurement complete event */
        measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
                                  MSR_SRV_EVENT_STOP_COMPLETE );
        
        return TI_OK;
    }

    /* set channel to the first channel in the BG/A band list  */
    if (pMeasurementSRV->msrRequest.msrTypes[0].channelListBandBG.uActualNumOfChannels > 0) 
    {
        pMeasurementCmd.channel = pMeasurementSRV->msrRequest.msrTypes[0].channelListBandBG.channelList[0];
        pMeasurementCmd.band = RADIO_BAND_2_4GHZ;
    }
    else if (pMeasurementSRV->msrRequest.msrTypes[0].channelListBandA.uActualNumOfChannels > 0) 
    {
        pMeasurementCmd.channel = pMeasurementSRV->msrRequest.msrTypes[0].channelListBandA.channelList[0];
        pMeasurementCmd.band = RADIO_BAND_5GHZ;
    }
    else
    {
        TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, "measurementSRVSM_requestMeasureStart: No channel to measure\n");
        return TI_NOK;
    }
    

    pMeasurementCmd.duration = 0; /* Infinite */
    pMeasurementCmd.eTag = pMeasurementSRV->msrRequest.eTag;
	pMeasurementCmd.enterPS = pMeasurementSRV->msrRequest.enterPS;

    if ( measurementSRVIsBeaconMeasureIncluded( hMeasurementSRV ))
    {  /* Beacon Measurement is one of the types */

		/* get the current channel */
		TTwdParamInfo	paramInfo;
        TI_UINT8        numOfChannels = 0; 
        TI_UINT8        bIsRequestOnServingChannelOnly = TI_FALSE;
        

        numOfChannels = pMeasurementSRV->msrRequest.msrTypes[0].channelListBandBG.uActualNumOfChannels + 
            pMeasurementSRV->msrRequest.msrTypes[0].channelListBandA.uActualNumOfChannels;
            
		paramInfo.paramType = TWD_CURRENT_CHANNEL_PARAM_ID;
		cmdBld_GetParam (pMeasurementSRV->hCmdBld, &paramInfo);

		pMeasurementCmd.ConfigOptions = RX_CONFIG_OPTION_FOR_MEASUREMENT; 


        /* If we have more than one channel in total, the scan will be probably be executed on a non-serving channel... */
        if (numOfChannels == 1) 
        {
            if (pMeasurementSRV->msrRequest.msrTypes[0].channelListBandBG.uActualNumOfChannels == 1) 
            {
                if (pMeasurementSRV->msrRequest.msrTypes[0].channelListBandBG.channelList[0] == paramInfo.content.halCtrlCurrentChannel) 
                {
                    bIsRequestOnServingChannelOnly = TI_TRUE;
                }
            }
            else
            {
                 if (pMeasurementSRV->msrRequest.msrTypes[0].channelListBandA.channelList[0] == paramInfo.content.halCtrlCurrentChannel) 
                {
                    bIsRequestOnServingChannelOnly = TI_TRUE;
                }
            }
          
        }
        
        
		/* check if the request is on the serving channel */
		if ( TI_TRUE == bIsRequestOnServingChannelOnly)
		{
			/* Set the RX Filter to the join one, so that any packets will 
            be received on the serving channel - beacons and probe requests for
			the measurmenet, and also data (for normal operation) */
            pMeasurementCmd.FilterOptions = RX_FILTER_OPTION_JOIN;
		}
		else
		{
			/* not on the serving channle - only beacons and rpobe responses are required */
			pMeasurementCmd.FilterOptions = RX_FILTER_OPTION_DEF_PRSP_BCN; 
		}
    }
    else
    {  /* No beacon measurement - use the current RX Filter */
        pMeasurementCmd.ConfigOptions = 0xffffffff;
        pMeasurementCmd.FilterOptions = 0xffffffff;
    }

    /* Send start measurement command */
    status = cmdBld_CmdMeasurement (pMeasurementSRV->hCmdBld,
                                    &pMeasurementCmd,
                                    (void *)measurementSRVSM_requestMeasureStartResponseCB, 
                                    pMeasurementSRV);
    
    if ( TI_OK != status )
    {
        TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": Failed to send measurement start command, statud=%d,\n", status);

        /* keep the faulty return status */
        pMeasurementSRV->returnStatus = status;

        /* send a measurement start fail event */
        return measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
                                         MSR_SRV_EVENT_START_FAILURE );
    }

    TRACE6( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": measure start command sent. Params:\n channel=%d, band=%d, duration=%d, \n configOptions=0x%x, filterOptions=0x%x, status=%d, \n", pMeasurementCmd.channel, pMeasurementCmd.band, pMeasurementCmd.duration, pMeasurementCmd.ConfigOptions, pMeasurementCmd.FilterOptions, status);

    /* start the FW guard timer */
    pMeasurementSRV->bStartStopTimerRunning = TI_TRUE;
    tmr_StartTimer (pMeasurementSRV->hStartStopTimer,
                    MacServices_measurementSRV_startStopTimerExpired,
                    (TI_HANDLE)pMeasurementSRV,
                    MSR_FW_GUARD_TIME_START,
                    TI_FALSE);
  
    return TI_OK;
}

/**
 * \\n
 * \brief Handle measurement start success if all measurements
 *        failed send ALL_TYPE_COMPLETE event to stop measure command to the FW.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_startMeasureTypesSuccess( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    /* if no measurement types are running, send al types complete event.
       This can happen if all types failed to start */
    if ( TI_TRUE == measurementSRVIsMeasurementComplete( hMeasurementSRV ))
    {
        /* send the event */
        measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
                                  MSR_SRV_EVENT_ALL_TYPES_COMPLETE );
    }

    return TI_OK;
}

/**
 * \\n
 * \brief Handle ap discovery measurement start request
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_waitApDiscovery(TI_HANDLE hMeasurementSRV)
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TApDiscoveryParams    pApDiscoveryParams;
    TI_UINT32 i = 0;
    TI_UINT32 requestIndex = pMeasurementSRV->uApDiscoveryRequestIndex;
    TI_STATUS status;

    /* set all parameters in the AP discovery command */
    pApDiscoveryParams.txdRateSetBandBG = HW_BIT_RATE_1MBPS;
    pApDiscoveryParams.txdRateSetBandA = HW_BIT_RATE_6MBPS;

    pApDiscoveryParams.ConfigOptions = RX_CONFIG_OPTION_FOR_MEASUREMENT;
    pApDiscoveryParams.FilterOptions = RX_FILTER_OPTION_DEF_PRSP_BCN;
    pApDiscoveryParams.scanOptions = 0;


    pApDiscoveryParams.ssid.len = pMeasurementSRV->msrRequest.msrTypes[requestIndex].ssid.len;
    os_memoryCopy(pMeasurementSRV->hOS, pApDiscoveryParams.ssid.str,
            pMeasurementSRV->msrRequest.msrTypes[requestIndex].ssid.str,
            pMeasurementSRV->msrRequest.msrTypes[requestIndex].ssid.len);


    pApDiscoveryParams.scanDuration = pMeasurementSRV->msrRequest.msrTypes[ requestIndex ].duration*1000;

    if (MSR_TYPE_RRM_BEACON_MEASUREMENT == pMeasurementSRV->msrRequest.msrTypes[requestIndex].msrType)
    {
        pApDiscoveryParams.numOfProbRqst = 3;
        pApDiscoveryParams.ConfigOptions = RX_CONFIG_OPTION_FOR_SCAN;

    }
    else /* MSR_TYPE_XCC_BEACON_MEASUREMENT */
    {
        pApDiscoveryParams.numOfProbRqst = 1;
    }


    for ( i = 0; i < pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandBG.uActualNumOfChannels; i++ )
    {
        pApDiscoveryParams.channelListBandBG.channelList[i] = pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandBG.channelList[i];
        pApDiscoveryParams.channelListBandBG.txPowerDbm[i] = pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandBG.txPowerDbm[i];
    }

    pApDiscoveryParams.channelListBandBG.uActualNumOfChannels = i;

    for ( i = 0; i < pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandA.uActualNumOfChannels; i++ )
    {
        pApDiscoveryParams.channelListBandA.channelList[i] = pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandA.channelList[i];
        pApDiscoveryParams.channelListBandA.txPowerDbm[i] = pMeasurementSRV->msrRequest.msrTypes[requestIndex].channelListBandA.txPowerDbm[i];
    }

    pApDiscoveryParams.channelListBandA.uActualNumOfChannels = i;


    /* band determined at the initiate measurement command not at that structure */

    /* scan mode go into the scan option field */
    if ( MSR_SCAN_MODE_PASSIVE == pMeasurementSRV->msrRequest.msrTypes[ requestIndex ].scanMode )
    {
        pApDiscoveryParams.scanOptions |= SCAN_PASSIVE;
    }


    /* Send AP Discovery command */
    status = cmdBld_CmdApDiscovery (pMeasurementSRV->hCmdBld, &pApDiscoveryParams, NULL, NULL);

    if ( TI_OK != status )
    {
        tmr_StartTimer(pMeasurementSRV->hApDiscoveryTimer,
                       MacServices_measurementSRV_startStopTimerExpired,
                       hMeasurementSRV,
                       MSR_FW_GUARD_TIME_START,
                       TI_FALSE);
    }
    else
    {
        TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": TWD_ApDiscoveryCmd returned status %d\n", status);
    }

    return TI_OK;
}

/**
 * \\n
 * \brief Handle ap discovery measurement start request
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_startApDiscovery(TI_HANDLE hMeasurementSRV)
{

    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TI_UINT32 requestIndex = pMeasurementSRV->uApDiscoveryRequestIndex;

    /* stop Ap Discovery event timer */
    tmr_StopTimer(pMeasurementSRV->hApDiscoveryTimer);

    /* Start Timer */
    tmr_StartTimer (pMeasurementSRV->hRequestTimer[requestIndex],
                    MacServices_measurementSRV_requestTimerExpired,
                    (TI_HANDLE)pMeasurementSRV,
                    (pMeasurementSRV->msrRequest.msrTypes[requestIndex].duration * 1024)/1000,
                    TI_FALSE);

    pMeasurementSRV->bRequestTimerRunning[ requestIndex ] = TI_TRUE;

    return TI_OK;
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief Handle an ALL_TYPE_COMPLETE event by sending a stop measure command to the FW.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_requestMeasureStop( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TI_STATUS status;

    /* since this function may also be called when stop is requested and start complete event
       has not yet been received from the FW, we may need to stop the FW guard timer */
    if (pMeasurementSRV->bStartStopTimerRunning)
    {
		tmr_StopTimer (pMeasurementSRV->hStartStopTimer);
        pMeasurementSRV->bStartStopTimerRunning = TI_FALSE;
    }
    
    /* Send Measurement Stop command to the FW */
    status = cmdBld_CmdMeasurementStop (pMeasurementSRV->hCmdBld, 
                                        (void *)pMeasurementSRV->commandResponseCBFunc,
                                        pMeasurementSRV->commandResponseCBObj);

    pMeasurementSRV->commandResponseCBFunc = NULL;
    pMeasurementSRV->commandResponseCBObj = NULL;
    
    if ( TI_OK != status )
    {
        TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": Failed to send measurement stop command, statud=%d,\n", status);

        /* send a measurement complete event - since it can't be stopped */
        measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState), MSR_SRV_EVENT_STOP_COMPLETE );
        return TI_OK;
    }

    TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": measure stop command sent.\n");

    /* start the FW guard timer */
    pMeasurementSRV->bStartStopTimerRunning = TI_TRUE;
    tmr_StartTimer (pMeasurementSRV->hStartStopTimer,
                    MacServices_measurementSRV_startStopTimerExpired,
                    (TI_HANDLE)pMeasurementSRV,
                    MSR_FW_GUARD_TIME_STOP,
                    TI_FALSE);

    return TI_OK;
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief Handle a STOP_COMPLETE event by exiting driver mode and calling the complete CB.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_completeMeasure( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t *pMeasurementSRV = (measurementSRV_t *)hMeasurementSRV;


    /* if the response CB is still pending, call it (when requestExpiryTimeStamp was reached) */
    if ( NULL != pMeasurementSRV->commandResponseCBFunc )
    {
        pMeasurementSRV->commandResponseCBFunc( pMeasurementSRV->commandResponseCBObj, TI_OK );
    }

    /* call the complete CB */
    if ( NULL != pMeasurementSRV->measurmentCompleteCBFunc )
    {
        pMeasurementSRV->measurmentCompleteCBFunc( pMeasurementSRV->measurementCompleteCBObj, 
                                                   &(pMeasurementSRV->msrReply));
    }

    return TI_OK;
}

/**
 * \\n
 * \date 27-November-2005\n
 * \brief handle a STOP_REQUEST event when in WAIT_FOR_DRIVER_MODE by marking negative result status
 * \brief and calling the ordinary stop function
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_stopFromWaitForMeasureStart( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TI_INT32 i;

    /* mark that all types has failed */
    for ( i = 0; i < pMeasurementSRV->msrRequest.numberOfTypes; i++ )
    {
        pMeasurementSRV->msrReply.msrTypes[ i ].status = TI_NOK;
    }

    /* call the ordinary stop function (will send a measure stop command to FW) */
    measurementSRVSM_requestMeasureStop( hMeasurementSRV );

    return TI_OK;
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief handle a STOP_REQUEST event when in MEASURE_IN_PROGRESS by stopping all measure types and
 * \brief requesting measure stop from the FW.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_stopFromMeasureInProgress( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TNoiseHistogram   pNoiseHistParams;
    TI_STATUS         status;
    TI_INT32          i;

    /* stop all running measure types */
    for (i = 0; i < pMeasurementSRV->msrRequest.numberOfTypes; i++)
    {
        if (pMeasurementSRV->bRequestTimerRunning[i])
        {
            /* stop timer */
			tmr_StopTimer (pMeasurementSRV->hRequestTimer[i]);
            pMeasurementSRV->bRequestTimerRunning[i] = TI_FALSE;

            /* if necessary, stop measurement type */
            switch ( pMeasurementSRV->msrRequest.msrTypes[ i ].msrType )
            {
            case MSR_TYPE_XCC_BEACON_MEASUREMENT:
            case MSR_TYPE_RRM_BEACON_MEASUREMENT:
                /* send stop AP discovery command */
                status = cmdBld_CmdApDiscoveryStop (pMeasurementSRV->hCmdBld, NULL, NULL);
                if ( TI_OK != status )
                {
                    TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": TWD_ApDiscoveryStop returned status %d\n", status);
                }
                break;

            case MSR_TYPE_XCC_NOISE_HISTOGRAM_MEASUREMENT:
                /* Set Noise Histogram Cmd Params */
                pNoiseHistParams.cmd = STOP_NOISE_HIST;
                pNoiseHistParams.sampleInterval = 0;
                os_memoryZero( pMeasurementSRV->hOS, &(pNoiseHistParams.ranges[ 0 ]), MEASUREMENT_NOISE_HISTOGRAM_NUM_OF_RANGES );

                /* Send a Stop command to the FW */
                status = cmdBld_CmdNoiseHistogram (pMeasurementSRV->hCmdBld, &pNoiseHistParams, NULL, NULL);

                if ( TI_OK != status )
                {
                    TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": TWD_NoiseHistogramCmd returned status %d\n", status);
                }
                break;

            /* These are just to avoid compilation warnings, nothing is actualy done here! */
            case MSR_TYPE_BASIC_MEASUREMENT:
            case MSR_TYPE_XCC_CCA_LOAD_MEASUREMENT:
            case MSR_TYPE_FRAME_MEASUREMENT:
            case MSR_TYPE_MAX_NUM_OF_MEASURE_TYPES:
            default:
                TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": unsupported measurement type: %d\n", pMeasurementSRV->msrRequest.msrTypes[ i ].msrType);
                break;
            }

            /* mark that measurement has failed */
            pMeasurementSRV->msrReply.msrTypes[ i ].status = TI_NOK;
        }
    }

    /* Send Measurement Stop command to the FW */
    status = cmdBld_CmdMeasurementStop (pMeasurementSRV->hCmdBld,
                                        (void *)pMeasurementSRV->commandResponseCBFunc,
                                        pMeasurementSRV->commandResponseCBObj);

    pMeasurementSRV->commandResponseCBFunc = NULL;
    pMeasurementSRV->commandResponseCBObj = NULL;
    
    if ( TI_OK != status )
    {
        TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": Failed to send measurement stop command, statud=%d,\n", status);

        /* send a measurement complete event - since it can't be stopped */
        measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
                                  MSR_SRV_EVENT_STOP_COMPLETE );
        return TI_OK;
    }

    TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": measure stop command sent.\n");

    /* start the FW guard timer */
    pMeasurementSRV->bStartStopTimerRunning = TI_TRUE;
    tmr_StartTimer (pMeasurementSRV->hStartStopTimer,
                    MacServices_measurementSRV_startStopTimerExpired,
                    (TI_HANDLE)pMeasurementSRV,
                    MSR_FW_GUARD_TIME_STOP,
                    TI_FALSE);

    return TI_OK; 
}

/**
 * \\n
 * \date 08-November-2005\n
 * \brief handle AP_DISCOVERY event received from firmware after AP_DISCOVERY_STOP command\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_apDiscoveryStopComplete(TI_HANDLE hMeasurementSRV)
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
    TI_UINT32 requestIndex = pMeasurementSRV->uApDiscoveryRequestIndex;

    /* stop Ap Discovery event timer */
    tmr_StopTimer(pMeasurementSRV->hApDiscoveryTimer);

    pMeasurementSRV->bRequestTimerRunning[ requestIndex ] = TI_FALSE;
    pMeasurementSRV->uApDiscoveryRequestIndex = INVALID_MSR_INDEX;

    /* if no measurement are running and no CBs are pending, send ALL TYPES COMPLETE event */
    if ( TI_TRUE == measurementSRVIsMeasurementComplete( hMeasurementSRV ))
    {
        /* send the event */
        return measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
                MSR_SRV_EVENT_ALL_TYPES_COMPLETE );
    }

    return TI_OK;
}
/**
 * \\n
 * \date 08-November-2005\n
 * \brief handle a START_FAILURE event by exiting driver mode and calling the complete CB.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSrv - handle to the Measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSM_measureStartFailure( TI_HANDLE hMeasurementSRV )
{
    measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;

    /* This function can be called from within a request context (if the driver mode entry process
       was immediate), or from the driver mode CB context. Regardless of teh context in which it runs,
       The error indicates that something is wrong in the HAL. There is no way to solve this (other than debug it).
       The error is either indicating by the measurement start API return status (if still in the request context),
       or by calling the response (if available, only in GWSI) and complete CBs with invalid status */

    
    /* if we are running within a request context, don't call the CB! The startMeasurement function
       will return an invalid status instead */
    if ( TI_FALSE == pMeasurementSRV->bInRequest )
    {
        /* if a response CB is available (GWSI) call it */
        if ( NULL != pMeasurementSRV->commandResponseCBFunc )
        {
            pMeasurementSRV->commandResponseCBFunc( pMeasurementSRV->commandResponseCBObj, TI_NOK );
        }

        /* if a complete CB is available (both GWSI and TI driver), call it */
        if ( NULL != pMeasurementSRV->measurmentCompleteCBFunc )
        {
            /* mark that all types has failed */
            TI_INT32 i;
            for ( i = 0; i < MAX_NUM_OF_MSR_TYPES_IN_PARALLEL; i++ )
            {
                pMeasurementSRV->msrReply.msrTypes[ i ].status = TI_NOK;
            }
            /* call the complete CB */
            pMeasurementSRV->measurmentCompleteCBFunc( pMeasurementSRV->measurementCompleteCBObj, 
                                                       &(pMeasurementSRV->msrReply));
        }
        else
        {
            TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_ERROR, ": Start measurement failure and response and complete CBs are NULL!!!\n");
        }
    }

    return TI_OK;
}


static void measurementSRVSM_requestMeasureStartResponseCB(TI_HANDLE hMeasurementSRV, TI_UINT32 uMboxStatus)
{
	measurementSRV_t* pMeasurementSRV = (measurementSRV_t*)hMeasurementSRV;
	TI_INT32 i;

    TRACE1( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": FW has responded with CMD_STATUS = %d\n", uMboxStatus);

	if (uMboxStatus == TI_OK) 
	{
        TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": FW has responded with CMD_STATUS_SUCCESS!\n");

		if ( NULL != pMeasurementSRV->commandResponseCBFunc )
        {
            pMeasurementSRV->commandResponseCBFunc( pMeasurementSRV->commandResponseCBObj, TI_OK );
        }
	}
	else
	{
		if (uMboxStatus == SG_REJECT_MEAS_SG_ACTIVE) 
		{
            TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": FW has responded with CMD_STATUS_REJECT_MEAS_SG_ACTIVE!\n");
		}

        TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, ": FW has responded with CMD_STATUS NOK!!!\n");


		/* if a timer is running, stop it */
		if ( TI_TRUE == pMeasurementSRV->bStartStopTimerRunning )
		{
            TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_INFORMATION, "***** STOP TIMER 8 *****\n");
			tmr_StopTimer( pMeasurementSRV->hStartStopTimer );
			pMeasurementSRV->bStartStopTimerRunning = TI_FALSE;
		}
		for ( i = 0; i < MAX_NUM_OF_MSR_TYPES_IN_PARALLEL; i++ )
		{
			if ( TI_TRUE == pMeasurementSRV->bRequestTimerRunning[ i ] )
			{
				tmr_StopTimer( pMeasurementSRV->hRequestTimer[ i ] );
				pMeasurementSRV->bRequestTimerRunning[ i ] = TI_FALSE;
			}
		}
		
		measurementSRVSM_SMEvent( hMeasurementSRV, &(pMeasurementSRV->SMState),
											 MSR_SRV_EVENT_START_FAILURE );
	}
}


/**
 * \\n
 * \date 23-December-2005\n
 * \brief Handles a stop request when no stop is needed (SM is either idle or already send stop command to FW.\n
 *
 * Function Scope \e Private.\n
 * \param hMeasurementSrv - handle to the measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS measurementSRVSRVSM_dummyStop( TI_HANDLE hMeasurementSrv )
{
    measurementSRV_t *pMeasurementSRV = (measurementSRV_t*)hMeasurementSrv;

    TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_WARNING, ": sending unnecessary stop measurement command to FW...\n");

    /* send a stop command to FW, to obtain a different context in ehich to cal the command response CB */
    cmdBld_CmdMeasurementStop (pMeasurementSRV->hCmdBld, 
                               (void *)pMeasurementSRV->commandResponseCBFunc,
                               pMeasurementSRV->commandResponseCBObj);

    pMeasurementSRV->commandResponseCBFunc = NULL;
    pMeasurementSRV->commandResponseCBObj = NULL;

    return TI_OK;
}

/**
 * \\n
 * \date 17-November-2005\n
 * \brief Handles an unexpected event.\n
 *
 * Function Scope \e Private.\n
 * \param hMeasurementSrv - handle to the measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS actionUnexpected( TI_HANDLE hMeasurementSrv ) 
{
    measurementSRV_t *pMeasurementSRV = (measurementSRV_t*)hMeasurementSrv;
    TI_INT32 i;

    TRACE0( pMeasurementSRV->hReport, REPORT_SEVERITY_SM, ": measurement SRV state machine error, unexpected Event\n");

    if (pMeasurementSRV->bStartStopTimerRunning)
    {
		tmr_StopTimer (pMeasurementSRV->hStartStopTimer);
        pMeasurementSRV->bStartStopTimerRunning = TI_FALSE;
    }

    for (i = 0; i < MAX_NUM_OF_MSR_TYPES_IN_PARALLEL; i++)
    {
        if (pMeasurementSRV->bRequestTimerRunning[i])
        {
			tmr_StopTimer (pMeasurementSRV->hRequestTimer[i]);
            pMeasurementSRV->bRequestTimerRunning[i] = TI_FALSE;
        }
    }

    /* we must clean the old command response CB since they are no longer relevant 
      since the state machine may be corrupted */
    pMeasurementSRV->commandResponseCBFunc = NULL;
    pMeasurementSRV->commandResponseCBObj = NULL;

    /* indicate the unexpected event in the return status */
    pMeasurementSRV->returnStatus = TI_NOK;
    
    return TI_OK;
}

/**
 * \\n
 * \date 10-Jan-2005\n
 * \brief Handles an event that doesn't require any action.\n
 *
 * Function Scope \e Private.\n
 * \param hMeasurementSrv - handle to the measurement SRV object.\n
 * \return always TI_OK.\n
 */
TI_STATUS actionNop( TI_HANDLE hMeasurementSrv )
{   
    return TI_OK;
}

