/*
 * pwrState.h
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


/** \file pwrState.h
 *
 *  \brief This is the Power State module declarations.

 *  \date 02-Nov-2010
 */

#ifndef PWRSTATE_H_
#define PWRSTATE_H_

#include "tidef.h"
#include "paramOut.h"
#include "pwrState_Types.h"

/*****************************************************************************
 **         Functions                                                       **
 *****************************************************************************/

TI_HANDLE	pwrState_Create		(TI_HANDLE hOs);
void		pwrState_Init		(TStadHandlesList *pStadHandles);
TI_STATUS	pwrState_SetDefaults(TI_HANDLE hPwrState, TPwrStateInitParams *pInitParams);
TI_STATUS	pwrState_Destroy	(TI_HANDLE hPwrState);

TI_STATUS	pwrState_SetParam	(TI_HANDLE hPwrState, paramInfo_t *pParam);
TI_STATUS	pwrState_GetParam	(TI_HANDLE hPwrState, paramInfo_t *pParam);

void	pwrState_FwInitDone			(TI_HANDLE hPwrState);
void	pwrState_DrvMainStarted		(TI_HANDLE hPwrState);
void	pwrState_DrvMainStopped		(TI_HANDLE hPwrState);
void	pwrState_DozeDone			(TI_HANDLE hPwrState);
void	pwrState_DozeTimeout		(TI_HANDLE hPwrState, TI_BOOL bTWDInitOccured);
void	pwrState_SmeStopped			(TI_HANDLE hPwrState);
void	pwrState_SmeConnected		(TI_HANDLE hPwrState);
void	pwrState_SmeDisconnected	(TI_HANDLE hPwrState);
void	pwrState_ScanCncnStopped	(TI_HANDLE hPwrState);
void	pwrState_MeasurementStopped	(TI_HANDLE hPwrState);


#endif /* PWRSTATE_H_ */




