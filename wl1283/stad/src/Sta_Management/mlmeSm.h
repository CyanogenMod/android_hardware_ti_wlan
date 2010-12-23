/*
 * mlmeSm.h
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

 /** \file mlmeSm.h
 *  \brief 802.11 MLME SM
 *
 *  \see mlmeSm.c
 */


/***************************************************************************/
/*                                                                         */
/*      MODULE: mlmeSm.h                                                   */
/*    PURPOSE:  802.11 MLME SM                                     */
/*                                                                         */
/***************************************************************************/

#ifndef _MLME_SM_H
#define _MLME_SM_H

#include "GenSM.h"
#include "mlmeApi.h"
#include "mlme.h"

/* Constants */

/* changed to fit double buffer size - S.G */
/*#define MAX_MANAGEMENT_FRAME_BODY_LEN 2312*/
#define MAX_MANAGEMENT_FRAME_BODY_LEN   1476

/* Enumerations */

/* State machine states */
typedef enum 
{
	MLME_SM_STATE_IDLE			= 0,
	MLME_SM_STATE_AUTH_WAIT		= 1,
	MLME_SM_STATE_SHARED_WAIT	= 2,
	MLME_SM_STATE_ASSOC_WAIT	= 3,
	MLME_SM_NUMBER_OF_STATES
} mlme_smStates_t;

/* State machine events */
typedef enum 
{
	MLME_SM_EVENT_START				= 0,
	MLME_SM_EVENT_STOP				= 1,
	MLME_SM_EVENT_FAIL				= 2,
	MLME_SM_EVENT_SUCCESS    		= 3,
	MLME_SM_EVENT_SHARED_RECV	 	= 4,
	MLME_SM_EVENT_AUTH_TIMEOUT		= 5,
	MLME_SM_EVENT_SHARED_TIMEOUT	= 6,
	MLME_SM_EVENT_ASSOC_TIMEOUT		= 7,
	MLME_SM_NUMBER_OF_EVENTS
} mlme_smEvents_t;

/* Typedefs */

/* Structures */

/* External data definitions */

/* External functions definitions */

/* Function prototypes */

void mlme_smTimeout(TI_HANDLE hMlme);

/* local functions */

void mlme_smEvent(TI_HANDLE hGenSm, TI_UINT32 uEvent, void* pData);

void mlme_smToIdle(mlme_t *pMlme);


TI_STATUS mlme_smReportStatus(mlme_t *pMlme);

/* Timer expirey callbacks */
void mlme_authTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured);
void mlme_sharedTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured);
void mlme_assocTmrExpiry (TI_HANDLE hMlme, TI_BOOL bTwdInitOccured);

/* State Machine variables */
extern TGenSM_actionCell mlmeMatrix[ MLME_SM_NUMBER_OF_STATES ][ MLME_SM_NUMBER_OF_EVENTS ];
extern TI_INT8* uMlmeStateDescription[];
extern TI_INT8* uMlmeEventDescription[]; 

#endif

