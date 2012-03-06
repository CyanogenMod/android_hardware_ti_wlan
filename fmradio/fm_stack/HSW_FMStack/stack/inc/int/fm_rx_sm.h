/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and  
 * limitations under the License.
 */


#ifndef __FM_RX_SM_H
#define __FM_RX_SM_H

#include "fm_rx.h"
#include "fmc_core.h"
#include "fmc_commoni.h"
#include "mcp_bts_script_processor.h"
typedef enum _tagFmRxSmContextState {
    FM_RX_SM_CONTEXT_STATE_DESTROYED,
    FM_RX_SM_CONTEXT_STATE_DISABLED,
    FM_RX_SM_CONTEXT_STATE_DISABLING,
    FM_RX_SM_CONTEXT_STATE_ENABLING,
    FM_RX_SM_CONTEXT_STATE_ENABLED,
} FmRxSmContextState;

/*
    This is the function that is invoked to process FM task events when
    FM RX is enabled
*/

void Fm_Rx_Stack_EventCallback(FmcOsEvent evtMask);
typedef struct
{
    /* 
        Indicates if a transport event has been received but not processed yet.
        This is useful for:
        1. Debuggging - verifying that when an event arrives the previous event has
            already been processed
        2. Makes sure that when the main event processor (_FM_RX_SM_ProcessEvents())
            is called, we will not attempt to process a transport event that has already 
            been processed
    */
    FMC_BOOL                eventWaitsForProcessing;
    FmcCoreEventType    type;
    FmcStatus               status;

    FMC_U16                 read_param;
    /* [ToDo] Specify the max length using some meaningful symbolic value (probably use some fmc_core.h constant) */
    /* The size of data should be related to the max size of HCI event*/
    FMC_U8                  data[FMC_FW_RX_RDS_THRESHOLD_MAX*3 + 1];
    FMC_U8                  dataLen;
} _FmRxSmTransportEventData;

struct  _FmRxContext{
    /*FMC_BOOL isAllocated;*/
    FmRxSmContextState  state;


    _FmRxSmTransportEventData   transportEventData;

    /* Script Processor context that is used when executing various scripts */
    McpBtsSpContext         tiSpContext;

    /* The "application" callback function */
    FmRxCallBack            appCb;
    
    /* Event that is sent to the user of the FM RX API ("application") */
    FmRxEvent           appEvent;

} ;





typedef struct {
    FmcBaseCmd op;
} FmRxGenCmd;
typedef struct {
    FmcBaseCmd op;
    FMC_INT         paramIn; 
} FmRxSimpleSetOneParamCmd;
typedef struct {
    FmcBaseCmd op;
    FmRxMonoStereoMode         mode; 
} FmRxMoStSetCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT         mode; 
} FmRxBandSetCmd;
typedef struct {
    FmcBaseCmd op;
    FMC_INT        mode; 
} FmRxAudioRoutingSetCmd;
typedef struct {
    FmcBaseCmd op;
    FMC_INT         mode; 
} FmRxMuteCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT         gain; 
} FmRxVolumeSetCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT         mode; 
} FmRxRdsSetCmd;
typedef struct {
    FmcBaseCmd op;
    FMC_INT        mask; 
} FmRxSetRdsMaskCmd;
typedef struct {
    FmcBaseCmd op;
    FMC_INT         freq; 
} FmRxTuneCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_U8      dir; 

    FMC_U8      curStage;   /* Stage of seek/stop seek operation */
    FMC_U8      seekStageBeforeStop;
    FMC_U8      status;
} FmRxSeekCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      mode; 
} FmRxSetAFCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      mode; 
} FmRxSetStereoBlendCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      mode; 
} FmRxSetDeemphasisModeCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT     rssi_level; 
} FmRxSetRssiSearchLevelCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      pause_level; 
} FmRxSetPauseLevelCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      pause_duration; 
} FmRxSetPauseDurationCmd;

typedef struct {
    FmcBaseCmd op;
    FMC_INT      mode; 
} FmRxSetRdsRbdsModeCmd;



typedef struct {
    FmcBaseCmd op;
} FmRxTimeoutCmd;

typedef struct  {
    FmcBaseCmd      base;
    FmRxAudioTargetMask         rxTargetMask;
    ECAL_SampleFrequency eSampleFreq;
} FmRxSetAudioTargetCmd;

typedef struct  {
    FmcBaseCmd      base;
    ECAL_SampleFrequency eSampleFreq;
} FmRxSetDigitalAudioConfigurationCmd;


typedef struct {
    FmcBaseCmd op;
    FmcChannelSpacing channelSpacing; 
} FmRxSpacingSetCommand;


/*Upper events options */
typedef enum
{   
    FM_RX_SM_UPPER_EVENT_NONE,
    FM_RX_SM_UPPER_EVENT_STOP_SEEK,
    FM_RX_SM_UPPER_EVENT_STOP_COMPLETE_SCAN,
    FM_RX_SM_UPPER_EVENT_GET_COMPLETE_SCAN_PROGRESS,
} FmRxSmUpperEvent;



/********************************************************************************
 *
 *
 *
 *******************************************************************************/
FmRxStatus FM_RX_SM_Init(void);
FmRxStatus FM_RX_SM_Deinit(void);

FmRxStatus FM_RX_SM_Create(const FmRxCallBack fmCallback, FmRxContext **fmContext);
FmRxStatus FM_RX_SM_Destroy(FmRxContext **fmContext);

/*
    This is the function that is invoked to process FM task events when
    FM RX is enabled
*/
void FM_RX_SM_TaskEventCb(FmcOsEvent osEvent);
/*
    Returns the single context

    [ToDo] - Add support for multiple contexts
    [ToDo] Consider completely hiding the context within fm_tx_sm.c and only exposing functions that provide the
    required services, such as: IsContextAllocated(contex), instead of FM_TX_SM_GetContext()
*/
FmRxContext* FM_RX_SM_GetContext(void);

FmRxSmContextState FM_RX_SM_GetContextState(void);

FMC_BOOL FM_RX_SM_IsContextEnabled(void);

FmRxCmdType FM_RX_SM_GetRunningCmd(void);

void FM_RX_SM_SetUpperEvent(FMC_U8 upperEvt);

/*
    Allocates space for a command structure object from one of 2 pools:
    1. If the command is no an "RDS-command" (see below), space is allocated 
        from the shared commands pool.
    2. If the command is an RDS-command, space is allocated from the internal
        TX memory space that is created for that purpose.

    An RDS-command is a command that carries data for the RDS FIFO. The following 
    command are RDS commands:
    1. Set RDS PS text (in RDS automatic mode)
    2. Set RDS RT text (in RDS automatic mode)
    3. Set RDS raw data (in RDS manual mode)
        
*/
FmTxStatus FM_RX_SM_AllocateCmdAndAddToQueue(   FmRxContext     *context,
                                                                FmRxCmdType cmdType,
                                                                FmcBaseCmd      **cmd);

/*
    Checks whether the command queue contains a command structure object that macthes
    the specified command type.

    If an object was found, the function returns FMC_TRUE. Otherwise, it returns FMC_FALSE
*/
FMC_BOOL FM_RX_SM_IsCmdPending(FmRxCmdType cmdType);

/*
  Return TRUE if CompleteScan is in progress and we can actually stop it 
  (at the end stages the scan cannot be stopped (anyway only few ms are left)
  Stopping the Scan at the final stages can cause a deadlock
*/
FMC_BOOL FM_RX_SM_IsCompleteScanStoppable();

/*
    The function does the following:
    1. Searches the command queue for the last pending tune command structure object. If 
        an object was found, its frequency is returned (this is the "pending tuned frequency"); Otherwise
    2. If there is a tuned frequency, it returned that frequency; Otherwise
    3. It returns FMC_UNDEFINED_FREQ (this is possible since there is no default frequency)
*/
/*FmcFreq FM_RX_SM_GetCurrentAndPendingTunedFreq(FmTxContext *context);*/


/********************************************************************************
 *
 * 
 *
 *******************************************************************************/


#endif /*__FM_RX_SM_H*/
