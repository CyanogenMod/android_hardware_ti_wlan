/*
 * MacServices_api.h
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

/** \file MacServicesApi.h
 *  \brief This file include public definitions for the MacServices module, comprising its API.
 *  \
 *  \date 6-Oct-2005
 */

#ifndef __MACSERVICESAPI_H__
#define __MACSERVICESAPI_H__

#include "osApi.h"

/*
 ***********************************************************************
 *	Constant definitions.
 ***********************************************************************
 */

/*
 ***********************************************************************
 *	Enums.
 ***********************************************************************
 */

/*
 ***********************************************************************
 *	Typedefs.
 ***********************************************************************
 */



/*
 ***********************************************************************
 *	Structure definitions.
 ***********************************************************************
 */

/*
 ***********************************************************************
 *	External data definitions.
 ***********************************************************************
 */

/*
 ***********************************************************************
 *	External functions definitions
 ***********************************************************************
 */


/***********************************************************************
 *	Measurement SRV API functions
 ***********************************************************************/

/**
 * \date 08-November-2005\n
 * \brief Creates the measurement SRV object
 *
 * Function Scope \e Public.\n
 * \param hOS - handle to the OS object.\n
 * \return a handle to the measurement SRV object, NULL if an error occurred.\n
 */
TI_HANDLE MacServices_measurementSRV_create( TI_HANDLE hOS );

/**
 * \date 08-November-2005\n
 * \brief Initializes the measurement SRV object
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 * \param hReport - handle to the report object.\n
 * \param hCmdBld - handle to the Command Builder object.\n
 */
TI_STATUS MacServices_measurementSRV_init (TI_HANDLE hMeasurementSRV, 
                                           TI_HANDLE hReport, 
                                           TI_HANDLE hCmdBld,
                                           TI_HANDLE hEventMbox,
                                           TI_HANDLE hTimer);

/**
 * \date 08-November-2005\n
 * \brief Destroys the measurement SRV object
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_destroy( TI_HANDLE hMeasurementSRV );

/**
 * \\n
 * \date 09-November-2005\n
 * \brief Starts a measurement operation.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the MeasurementSRV object.\n
 * \param pMsrRequest - a structure containing measurement parameters.\n
 * \param timeToRequestexpiryMs - the time (in milliseconds) the measurement SRV has to start the request.\n
 * \param cmdResponseCBFunc - callback function to used for command response.\n
 * \param cmdResponseCBObj - handle to pass to command response CB.\n
 * \param cmdCompleteCBFunc - callback function to be used for command complete.\n
 * \param cmdCompleteCBObj - handle to pass to command complete CB.\n
 * \return TI_OK if successful (various, TBD codes if not).\n
 */ 
TI_STATUS MacServices_measurementSRV_startMeasurement( TI_HANDLE hMeasurementSRV, 
                                                       TMeasurementRequest* pMsrRequest,
													   TI_UINT32 timeToRequestExpiryMs,
                                                       TCmdResponseCb cmdResponseCBFunc,
                                                       TI_HANDLE cmdResponseCBObj,
                                                       TMeasurementSrvCompleteCb cmdCompleteCBFunc,
                                                       TI_HANDLE cmdCompleteCBObj );

/**
 * \\n
 * \date 09-November-2005\n
 * \brief Stops a measurement operation in progress.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the MeasurementSRV object.\n
 * \param bSendNullData - whether to send NULL data when exiting driver mode.\n
 * \param cmdResponseCBFunc - callback function to used for command response.\n
 * \param cmdResponseCBObj - handle to pass to command response CB.\n
 * \return TI_OK if successful (various, TBD codes if not).\n
 */
TI_STATUS MacServices_measurementSRV_stopMeasurement( TI_HANDLE hMeasurementSRV,
													  TI_BOOL bSendNullData,
                                                      TCmdResponseCb cmdResponseCBFunc,
                                                      TI_HANDLE cmdResponseCBObj );

/**
 * \\n
 * \date 09-November-2005\n
 * \brief Notifies the measurement SRV of a FW reset (recovery).\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the MeasurementSRV object.\n
 */
void MacServices_measurementSRV_FWReset( TI_HANDLE hMeasurementSRV );

/** 
 * \\n
 * \date 14-November-2005\n
 * \brief callback function used by the HAL for measure start event (sent when the FW 
 * has started measurement operation, i.e. switched channel and changed RX filters).\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_measureStartCB( TI_HANDLE hMeasurementSRV );

/** 
 * \\n
 * \date 14-November-2005\n
 * \brief callback function used by the HAL for measure stop event (sent when the FW 
 * has finished measurement operation, i.e. switched channel to serving channel and changed back RX filters).\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_measureCompleteCB( TI_HANDLE hMeasurementSRV, char* str, TI_UINT32 strLen );

/** 
 * \\n
 * \date 14-November-2005\n
 * \brief callback function used by the HAL for AP discovery stop event (sent when the FW 
 * has finished AP discovery operation).\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_apDiscoveryCompleteCB( TI_HANDLE hMeasurementSRV );

/** 
 * \\n
 * \date 16-November-2005\n
 * \brief Callback for channel load get param call.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 * \param status - the get_param call status.\n
 * \param CB_buf - pointer to the results buffer (already on the measurement SRV object)
 */
void MacServices_measurementSRV_channelLoadParamCB( TI_HANDLE hMeasurementSRV, TI_STATUS status, TI_UINT8* CB_buf );

/** 
 * \date 03-January-2005\n
 * \brief Dummy callback for channel load get param call. Used to clear the channel load tracker.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 * \param status - the get_param call status.\n
 * \param CB_buf - pointer to the results buffer (already on the measurement SRV object)
 */
void MacServices_measurementSRV_dummyChannelLoadParamCB( TI_HANDLE hMeasurementSRV, TI_STATUS status, TI_UINT8* CB_buf );

/** 
 * \\n
 * \date 16-November-2005\n
 * \brief Callback for noise histogram get param call.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasurementSRV - handle to the measurement SRV object.\n
 * \param status - the get_param call status.\n
 * \param CB_buf - pointer to the results buffer (already on the measurement SRV object)
 */
void MacServices_measurementSRV_noiseHistCallBack(TI_HANDLE hMeasurementSRV, TI_STATUS status, TI_UINT8* CB_buf);

/**
 * \\n
 * \date 14-November-2005\n
 * \brief called when a measurement FW guard timer expires.
 *
 * Function Scope \e Public.\n
 * \param hMeasuremntSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_startStopTimerExpired (TI_HANDLE hMeasurementSRV, TI_BOOL bTwdInitOccured);

/**
 * \\n
 * \date 15-November-2005\n
 * \brief called when a measurement type timer expires.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasuremntSRV - handle to the measurement SRV object.\n
 */
void MacServices_measurementSRV_requestTimerExpired (TI_HANDLE hMeasurementSRV, TI_BOOL bTwdInitOccured);

/**
 * \\n
 * \date 15-November-2005\n
 * \brief Registers a failure event callback for scan error notifications.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasuremntSRV - handle to the measurement SRV object.\n
 * \param failureEventCB - the failure event callback function.\n
 * \param hFailureEventObj - handle to the object passed to the failure event callback function.\n
 */
void MacServices_measurementSRV_registerFailureEventCB( TI_HANDLE hMeasurementSRV,
														void * failureEventCB,
														TI_HANDLE hFailureEventObj );

/**
 * \\n
 * \date 15-November-2005\n
 * \brief Restart the measurement SRV object upon recover.\n
 *
 * Function Scope \e Public.\n
 * \param hMeasuremntSRV - handle to the measurement SRV object.
 */
void MacServices_measurementSRV_restart( TI_HANDLE hMeasurementSRV);


#endif /* __MACSERVICESAPI_H__ */
