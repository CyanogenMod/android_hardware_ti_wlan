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
/*******************************************************************************\
*
*   FILE NAME:      fm_tx_sm.h
*
*   BRIEF:          FM Tx State Machine & Main Implementation file
*
*   DESCRIPTION:  
*
*   TBD 
*                   
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/

#ifndef __FM_TX_SM_H
#define __FM_TX_SM_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_utils.h"
#include "fmc_common.h"


/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

typedef enum _tagFmTxSmContextState {
    FM_TX_SM_CONTEXT_STATE_DESTROYED,
    FM_TX_SM_CONTEXT_STATE_DISABLED,
    FM_TX_SM_CONTEXT_STATE_ENABLING,
    FM_TX_SM_CONTEXT_STATE_ENABLED,
    FM_TX_SM_CONTEXT_STATE_TRANSMITTING,
    FM_TX_SM_CONTEXT_STATE_DISABLING,
    FM_TX_SM_CONTEXT_STATE_WAITING_FOR_CC,
    FM_TX_SM_CONTEXT_STATE_WAITING_FOR_INT,
    FM_TX_SM_CONTEXT_STATE_WAITING_FOR_GENERAL_EVENT
} FmTxSmContextState;

/*-------------------------------------------------------------------------------
 * Definitions of TX command structures
 *
 *  There is a separate structure per every FM TX command. Every structure contains fields
 *  that store the values of all applicable command arguments. The values are copied by
 *  the corresponding API call (see fm_tx.c). An object of the type of the command structure
 *  is allocated when the command API is invoked. The values are populated and then the
 *  object is placed on the commands queue.
 */
 typedef struct  {
    FmcBaseCmd      base;
    
    FMC_UINT        param;
} FmTxSimpleSetOneParamCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxEnableCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxDisableCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxStartTransmissionCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxStopTransmissionCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcFreq         freq;
} FmTxTuneCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxGetTunedFrequencyCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmTxPowerLevel  level;
} FmTxSetPowerLevelCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcBand         band;
} FmTxSetBandCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetBandCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxGetPowerLevelCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmTxMonoStereoMode          monoMode;
} FmTxSetMonoStereoModeCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetMonoStereoModeCmd;
typedef struct  {
    FmcBaseCmd      base;
    
    FmcEmphasisFilter           emphasisfilter;
} FmTxSetPreEmphasisFilterCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetPreEmphasisFilterCmd;
typedef struct  {
    FmcBaseCmd      base;
    
    FmTxRdsTransmissionMode         transmissionMode;
} FmTxSetRdsTransmissionModeCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsTransmissionModeCmd;
typedef struct  {
    FmcBaseCmd      base;
    
    FmcAfCode           afCode;
} FmTxSetRdsAfCodeCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsAfCodeCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsPiCode            piCode;
} FmTxSetRdsPiCodeCmd;
typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsPtyCode           ptyCode;
} FmTxSetRdsPtyCodeCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsPtyCodeCmd;


typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsRepertoire            repertoire;
} FmTxSetRdsRepertoireCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsRepertoireCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsPsScrollSpeed             psScrollSpeed;
} FmTxSetRdsPsScrollSpeedCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsScrollParmsCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsTaCode            taCode;
    FmcRdsTpCode                tpCode;
} FmTxSetRdsTrafficCodesCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsTrafficCodesCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsMusicSpeechFlag           musicSpeechFlag;
} FmTxSetRdsMusicSpeechFlagCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsMusicSpeechFlagCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcRdsExtendedCountryCode           countryCode;
} FmTxSetRdsExtendedCountryCodeCmd;
typedef struct  {
    FmcBaseCmd      base;
} FmTxGetRdsExtendedCountryCodeCmd;

typedef struct {
    FmcBaseCmd      base;

    FMC_U8          rawData[FM_RDS_RAW_MAX_MSG_LEN];
    FMC_UINT        rawLen;
} FmTxSetRdsRawDataCmd;

typedef struct  {
    FmcBaseCmd      base;
    
    FmcMuteMode     mode;
} FmTxSetMuteModeCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxGetMuteModeCmd;

typedef struct  {
FmcBaseCmd      base;
} FmTxEnableRdsCmd;

typedef struct  {
    FmcBaseCmd      base;
} FmTxDisableRdsCmd;

typedef struct {
    FmcBaseCmd      base;
} FmTxGetRdsPiCodeCmd;

typedef struct {
    FmcBaseCmd      base;

    FMC_U8          msg[FM_RDS_PS_MAX_MSG_LEN];
    FMC_UINT        len;
} FmTxSetRdsTextPsMsgCmd;

typedef struct {
    FmcBaseCmd      base;
} FmTxGetRdsTextPsMsgCmd;

typedef struct {
    FmcBaseCmd          base;
    FmcRdsPsDisplayMode mode;
} FmTxSetRdsTextPsScrollModeCmd;

typedef struct {
    FmcBaseCmd      base;
} FmTxGetRdsTextPsScrollModeCmd;


typedef struct {
    FmcBaseCmd      base;

    FmcRdsRtMsgType rtType;
    FMC_U8          msg[FM_RDS_RT_B_MAX_MSG_LEN];
    FMC_UINT        len;
} FmTxSetRdsTextRtMsgCmd;

typedef struct {
    FmcBaseCmd      base;
} FmTxGetRdsTextRtMsgCmd;

typedef struct {
    FmcBaseCmd                  base;
    FmTxRdsTransmittedGroupsMask    fieldsMask;
} FmTxSetRdsTransmittedFieldsMaskCmd;

typedef struct {
    FmcBaseCmd      base;
} FmTxGetRdsTransmittedFieldsMaskCmd;

typedef struct  {
    FmcBaseCmd      base;
    FmTxAudioSource txSource;
    ECAL_SampleFrequency eSampleFreq;
} FmTxSetAudioSourceCmd;

typedef struct  {
    FmcBaseCmd      base;
    ECAL_SampleFrequency eSampleFreq;
} FmTxSetDigitalAudioConfigurationCmd;


FmTxStatus FM_TX_SM_Init(void);
FmTxStatus FM_TX_SM_Deinit(void);

FmTxStatus FM_TX_SM_Create(const FmTxCallBack fmCallback, FmTxContext **fmContext);
FmTxStatus FM_TX_SM_Destroy(FmTxContext **fmContext);

/*
    This is the function that is invoked to process FM task events when
    FM TX is enabled
*/
void FM_TX_SM_TaskEventCb(FmcOsEvent osEvent);
/*
*   This function returns the SM RDS Transmission Mode (Manual/Auto)
*/
FmTxRdsTransmissionMode FM_TX_SM_GetRdsMode(void);
/*
*   This function returns the state of RDS (Enabled/Disabled)
*/
FMC_BOOL FM_TX_SM_IsRdsEnabled(void);
/*
    Returns the single context

    [ToDo] - Add support for multiple contexts
    [ToDo] Consider completely hiding the context within fm_tx_sm.c and only exposing functions that provide the
    required services, such as: IsContextAllocated(contex), instead of FM_TX_SM_GetContext()
*/
FmTxContext* FM_TX_SM_GetContext(void);

FmTxSmContextState FM_TX_SM_GetContextState(void);

FMC_BOOL FM_TX_SM_IsContextEnabled(void);

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
FmTxStatus FM_TX_SM_AllocateCmdAndAddToQueue(   FmTxContext     *context,
                                                                FmTxCmdType cmdType,
                                                                FmcBaseCmd      **cmd);

/*
    Searches the commands queue for a command structure object that matches
    the specified command type. The queue is searched from end to start, so that
    if an object is found, it is the object that corresponds to the last command that
    was issued, of that type

    If no object was found, the function returns NULL
*/
FmcBaseCmd *FM_TX_SM_FindLatestCmdInQueueByType(FmTxCmdType cmdType);


/*
    Checks whether the command queue contains a command structure object that macthes
    the specified command type.

    If an object was found, the function returns FMC_TRUE. Otherwise, it returns FMC_FALSE
*/
FMC_BOOL FM_TX_SM_IsCmdPending(FmTxCmdType cmdType);

/*
    The function does the following:
    1. Searches the command queue for the last pending tune command structure object. If 
        an object was found, its frequency is returned (this is the "pending tuned frequency"); Otherwise
    2. If there is a tuned frequency, it returned that frequency; Otherwise
    3. It returns FMC_UNDEFINED_FREQ (this is possible since there is no default frequency)
*/
FmcFreq FM_TX_SM_GetCurrentAndPendingTunedFreq(FmTxContext *context);

#endif /* ifndef __FM_TX_SM_H */

