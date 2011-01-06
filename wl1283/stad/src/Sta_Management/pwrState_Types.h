/*
 * pwrState_Types.h
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


/** \file	pwrState_Types.h
 *
 *  \brief	Power State module types

 *  \date	02-Nov-2010
 */

#ifndef PWRSTATE_TYPES_H_
#define PWRSTATE_TYPES_H_

#include "STADExternalIf.h"

/*****************************************************************************
 **         Constants                                                       **
 *****************************************************************************/

/* NDTIM value to use in LowOn mode */
#define PWRSTATE_SUSPEND_NDTIM_MIN	0
#define PWRSTATE_SUSPEND_NDTIM_MAX	100
#define PWRSTATE_SUSPEND_NDTIM_DEF	3

/* Timeout period (in ms) when entering LongDoze mode */
#define PWRSTATE_DOZE_TIMEOUT_MIN	0
#define PWRSTATE_DOZE_TIMEOUT_MAX	10000
#define PWRSTATE_DOZE_TIMEOUT_DEF	1000

/* Command Mailbox timeout period */
#define PWRSTATE_CMD_TIMEOUT_MIN	0
#define PWRSTATE_CMD_TIMEOUT_MAX	10000
#define PWRSTATE_CMD_TIMEOUT_DEF	500


/*****************************************************************************
 **         Enumerations                                                    **
 *****************************************************************************/

/*
 * State-machine states. Transition to one of these states is requested using PWR_ON,DOZE,SLEEP,PWR_OFF commands
 */
typedef enum
{
	/* Steady states */
	PWRSTATE_STATE_FULLY_ON,			/* Device is fully on */
	PWRSTATE_STATE_LOW_ON,				/* Aggressive Long Doze Power State mode, with no timers */
	PWRSTATE_STATE_SLEEP,				/* Not connected, no scan, no timers */
	PWRSTATE_STATE_OFF,					/* Device is shut down */

	/* Intermediate states */
	PWRSTATE_STATE_STANDBY,				/* Not connected, connection scan */
	PWRSTATE_STATE_WAIT_FW_INIT,		/* (intermediate state) waiting for FW initialization to be done */
	PWRSTATE_STATE_WAIT_DRV_START,		/* (intermediate state) waiting for drvMain to start */
	PWRSTATE_STATE_WAIT_DRV_STOP,		/* (intermediate state) waiting for drvMain to stop */
	PWRSTATE_STATE_WAIT_SCAN_STOP,		/* (intermediate state) waiting for scanCncn to stop */
	PWRSTATE_STATE_WAIT_SME_STOP,		/* (intermediate state) waiting for sme to stop */
	PWRSTATE_STATE_WAIT_MEASUR_STOP,	/* (intermediate state) waiting for measurementMgr to stop */
	PWRSTATE_STATE_WAIT_DOZE,			/* (intermediate state) waiting for powerMgr to suspend */

	PWRSTATE_STATE_LAST					/* pseudo state. do not use */
} EPwrStateSmState;

/*
 * State-machine events
 */
typedef enum
{
	PWRSTATE_EVNT_NONE,
	PWRSTATE_EVNT_CREATE,			/* Issued after the state-machine is created */
	PWRSTATE_EVNT_ON,				/* Request to transition to state On */
	PWRSTATE_EVNT_DOZE,				/* Request to transition to state Doze */
	PWRSTATE_EVNT_SLEEP,			/* Request to transition to state Sleep */
	PWRSTATE_EVNT_OFF,				/* Request to transition to state Off */
	PWRSTATE_EVNT_FW_INIT_DONE,		/* Notification from drvMain */
	PWRSTATE_EVNT_DRVMAIN_STARTED,	/* Notification from drvMain */
	PWRSTATE_EVNT_DRVMAIN_STOPPED,	/* Notification from drvMain */
	PWRSTATE_EVNT_DOZE_DONE,		/* Notification from powerMgr */
	PWRSTATE_EVNT_DOZE_TIMEOUT,		/* Notification from powerMgr */
	PWRSTATE_EVNT_SME_STOPPED,		/* Notification from sme */
	PWRSTATE_EVNT_SME_CONNECTED,	/* Notification from sme */
	PWRSTATE_EVNT_SME_DISCONNECTED,	/* Notification from sme */
	PWRSTATE_EVNT_SCANCNCN_STOPPED,	/* Notification from scanCncn */
	PWRSTATE_EVNT_MEASUR_STOPPED,	/* Notification from measurmentMgr */
} EPwrStateSmEvent;

/*
 * Suspend Type - what to do upon suspend
 */
typedef enum
{
	PWRSTATE_SUSPEND_NONE		= 0,	/* Do nothing upon suspend */
	PWRSTATE_SUSPEND_LOWON		= 1,	/* Upon suspend, if the STA is not connected, then move to state Off or to state Sleep (configurable); if connected, then move to state Low On (equivalent to aggressive Long Doze power management mode, but with no timers  in the driver) (issue a Doze command) */
	PWRSTATE_SUSPEND_RESERVED	= 2,	/* Reserved. Do not use */
	PWRSTATE_SUSPEND_SLEEP		= 3, 	/* Upon suspend, if the power is on then move to Sleep (not connected, no scan, no timers) (issue a Sleep command) */
	PWRSTATE_SUSPEND_OFF		= 4,	/* Shut down the device upon suspend (issue a PwrOff command) */

	PWRSTATE_SUSPEND_LAST
} EPwrStateSuspendType;

/*
 * Filter Usage - whether to use filter when suspended
 */
typedef enum
{
	PWRSTATE_FILTER_USAGE_NONE		= 0,	/* The RX Filter configuration is not changed upon suspend/resume */
	PWRSTATE_FILTER_USAGE_RXFILTER	= 1,	/* Upon suspend, replace the RX Filter with the configured one. Upon resume, revert */

	PWRSTATE_FILTER_USAGE_LAST
} EPwrStateFilterUsage;

/*
 * Standby Next State - what to do upon a Doze event when in Standby state
 */
typedef enum
{
	PWRSTATE_STNDBY_NEXT_STATE_RESERVED1	= 0,	/* Reserved. Do not use */
	PWRSTATE_STNDBY_NEXT_STATE_RESERVED2	= 1,	/* Reserved. Do not use */
	PWRSTATE_STNDBY_NEXT_STATE_STANDBY		= 2,	/* Stay in standby */
	PWRSTATE_STNDBY_NEXT_STATE_SLEEP		= 3,	/* Move to Sleep */
	PWRSTATE_STNDBY_NEXT_STATE_OFF			= 4,	/* Move to Off */

	PWRSTATE_STNDBY_NEXT_STATE_LAST
} EPwrStateStndbyNextState;

/*****************************************************************************
 **         Types                                                           **
 *****************************************************************************/

/* State function prototype */
typedef TI_STATUS (*FPwrStateSMState)(TI_HANDLE hPwrState, EPwrStateSmEvent eEvent);

/*****************************************************************************
 **         Structures                                                      **
 *****************************************************************************/

typedef struct {
	EPwrStateSuspendType	 eSuspendType;

	/* Additional parameters for LowOn suspend type */
	TI_UINT32				 uSuspendNDTIM;			/* Number of DTIMs to be used for Long Doze upon suspend. Between PWRSTATE_SUSPEND_NDTIM_MIN and PWRSTATE_SUSPEND_NDTIM_MAX */
	EPwrStateStndbyNextState eStandbyNextState;		/* Defines what state to move to upon a Doze event when in Standby state (or upon PS enter failure) */
	EPwrStateFilterUsage	 eSuspendFilterUsage;	/* Defines whether to use filter when suspended */
	TRxDataFilterRequest	 tSuspendRxFilterValue;	/* RxFilter value to use in case of eSuspendRxFilterUsage == PWRSTATE_FILTER_USAGE_RXFILTER */

	TI_UINT32				 uDozeTimeout;			/* time period before declaring the PS enter process failed */

	TTwdSuspendConfig		 tTwdSuspendConfig;		/* suspend parameters for TWD */
} TPwrStateCfg;

/*
 * Module object
 */
typedef struct
{
	TI_HANDLE			hOs;
	TI_HANDLE			hReport;
	TI_HANDLE			hDrvMain;
	TI_HANDLE			hScanCncn;
	TI_HANDLE			hSme;
	TI_HANDLE			hMeasurementMgr;
	TI_HANDLE			hPowerMgr;
	TI_HANDLE   		hTimer;
	TI_HANDLE			hTWD;
	TI_HANDLE			hDozeTimer;
	FPwrStateSMState	fCurrentState;				/* current state function */

	TPwrStateCfg		tConfig;

	/* Describes the current (steady state to steady state) transition */
	struct {
		void(*fCompleteCb)(TI_HANDLE,int,void*);	/* callback to invoke upon completion of state transition (when entering a steady state) */
		TI_HANDLE				hCompleteCb;		/* context for fTransitionCompleteCb */
		int						iCompleteStatus;	/* status for fTransitionCompleteCb */
		TI_BOOL					bContinueToOff;		/* whether or not to continue to Off state after reaching Sleep state */
		TI_BOOL					bAsyncTransition;   /* whether this transition was started and completed in the same context */
		TI_HANDLE				hCompleteTimer;     /* used to asynchronously complete a synchronous command */
	} tCurrentTransition;
} TPwrState;

#endif /* PWRSTATE_TYPES_H_ */


