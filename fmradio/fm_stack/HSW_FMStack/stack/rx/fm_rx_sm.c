/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010, 2011 Sony Ericsson Mobile Communications AB
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

#include <stdlib.h>
#include <errno.h>

#include "fmc_os.h"
#include "fmc_config.h"

#include "fmc_defs.h"
#include "fmc_debug.h"
#include "fmc_os.h"
#include "fmc_log.h"
#include "fm_rx_smi.h"

#include "fmc_fw_defs.h"
#include "fmc_config.h"
#include "fmc_utils.h"
#include "fmc_log.h"
#include "mcp_bts_script_processor.h"
#include "mcp_rom_scripts_db.h"
#include "mcp_hal_string.h"
#include "mcp_unicode.h"

#include "ccm.h"
#include "ccm_vac.h"

#include "fm_trace.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMRXSM);

#if FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED



/***********************************************************/
typedef enum {
    _FM_RX_SM_EVENT_START,
    _FM_RX_SM_EVENT_TRANSPORT_EVENT,
    _FM_RX_SM_EVENT_SCRIPT_EXEC_COMPLETE,
    _FM_RX_SM_EVENT_CONTINUE
} _FmRxSmEvent;

typedef enum {
    _FM_RX_SM_STATE_NONE,
    _FM_RX_SM_STATE_WAITING_FOR_CC,
    _FM_RX_SM_STATE_WAITING_FOR_INTERRUPT,
}_FmRxSmState;

typedef struct {
    FmcBaseCmd      *baseCmd;
    FMC_UINT        stageIndex;
    FmRxStatus      status;
    _FmRxSmState smState;
} _FmRxSmCurrCmdInfo;

typedef struct {
    FmcRdsPiCode piCode;
    FmcRdsPtyCode ptyCode;
    FmcRdsRepertoire repertoire;
    FmcRdsTaCode ta;
    FmcRdsTpCode tp;
    
} _FmRxSmCurRdsInfo;
typedef struct {

    FmRxAudioTargetMask audioTargetsMask;

    FmRxMonoStereoMode monoStereoMode;

    FMC_BOOL isAudioRoutingEnabled;
    
    ECAL_SampleFrequency eSampleFreq;

    TCCM_VAC_UnavailResourceList ptUnavailResources;

    ECAL_Operation lastOperationStarted;
    
} _FmRxSmVacParams;

typedef enum {
	_STOP_COMPLETE_IDLE,
    _STOP_COMPLETE_REQUEST,
    _STOP_COMPLETE_STARTED
} _FmRxStopCompleteScan;

typedef struct {
     FmcBaseCmd op;

    FMC_U8      curStage; 
    FMC_U8      status;       
    FMC_BOOL    isCompleteScanProgressFinish;
	_FmRxStopCompleteScan     stopCompleteScanStatus;

} _FmRxCompleteScanInfo;

typedef struct _tagFmRxSmData {
    /*Parameters/State currently set in FM RX*/
    /****************************/
    FmcBand band;
    FmcFreq tunedFreq;
    FMC_UINT volume;
    FmcMuteMode muteMode;
    FmRxRfDependentMuteMode rfDependedMute;
    FMC_INT rssiThreshold;
    FmRxRdsAfSwitchMode afMode;

    FmcChannelSpacing channelSpacing;

    _FmRxSmVacParams vacParams;
    
    
    FmcEmphasisFilter deemphasisFilter;

    FMC_BOOL rdsOn;
    FmcRdsGroupTypeMask rdsGroupMask;
    FmcRdsSystem rdsRdbsSystem;
    /****************************/
    
    /* This flag indicates the read command complete type*/
    FmRxSmReadState             fmRxReadState;
    /****************************/

    /*Info related to interrupts*/
    FmRxInterruptInfo               interruptInfo;
    /****************************/
    /*ASIC ID/Version are used to define the fm init script that should be loaded*/
    FMC_U16 fmAsicId;
    FMC_U16 fmAsicVersion;
    /****************************/
    FmRxContext         context;
    /****************************/
    _FmRxSmCurrCmdInfo  currCmdInfo;
    _FmRxSmCurRdsInfo rdsData;
    /****************************/
    FMC_BOOL                upperEventWait; /* indicates whether the upper event should wait for cmd complete or not */ 
    FMC_U8              callReason;     /* When calling the operation handler say if it was called because of cmd_complete event, interrupt or upper_event */   
    
    FMC_U8              upperEvent;     /* Event from the upper api */
    
    
    FmRxSeekCmd             seekOp;
    
    RdsParams               rdsParams;          /* Params of a specific RDS read */
    
    TunedStationParams      curStationParams;
    FMC_U8                  curAfJumpIndex;     /* Holds the index of the current AF jump */
    FmcFreq freqBeforeJump;     /* Will hold the frequency before the jump */

 /*Info related to Complete Scan*/
    _FmRxCompleteScanInfo               completeScanOp;
    
} _FmRxSmData;
FMC_STATIC _FmRxSmData  _fmRxSmData;


FMC_BOOL  fmRxSendDisableEventToApp = FMC_TRUE;



/*Define the Bit that indicates  the channels validation */
#define FM_IS_VALID_CHANNEL_BIT	(0x0080)

extern  FmRxCallBack fmRxInitAsyncAppCallback;
extern  FmRxErrorCallBack fmRxErrorAppCallback;

typedef void (*_FmRxSmSetCmdCompleteFunc)(void);

typedef void (*_FmRxSmCmdCompletedFillEventDataCb)(FmRxEvent *event, void *eventData);


/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC McpBtsSpStatus _FM_RX_SM_TiSpSendHciScriptCmd(   McpBtsSpContext *context,
                                                        McpU16  hciOpcode, 
                                                        McpU8   *hciCmdParms, 
                                                        McpUint len);
FMC_STATIC void _FM_RX_SM_TiSpExecuteCompleteCb(McpBtsSpContext *context, McpBtsSpStatus status);

FMC_STATIC void _FM_RX_SM_TransportEventCb(const FmcCoreEvent *eventParms);
FMC_STATIC void _FM_RX_SM_TccmVacCb(ECAL_Operation eOperation,ECCM_VAC_Event eEvent, ECCM_VAC_Status eStatus);


FMC_STATIC FmRxStatus _FM_RX_SM_RemoveFromQueueAndFreeCmd(FmcBaseCmd    **cmd);



FMC_STATIC FmRxStatus _FM_RX_SM_AllocateCmd(    FmRxContext     *context,
                                                FmRxCmdType cmdType,
                                                FmcBaseCmd      **cmd);

FMC_STATIC FmRxStatus _FM_RX_SM_FreeCmd( FmcBaseCmd **cmd);


FMC_STATIC FmRxStatus _FM_RX_SM_HandleCompletionOfCurrCmd( _FmRxSmSetCmdCompleteFunc    cmdCompleteFunc ,
                                                            _FmRxSmCmdCompletedFillEventDataCb fillEventDataCb, 
                                                            void *eventData,
                                                            FmRxEventType       evtTypes);
FMC_STATIC void _FM_RX_SM_SendAppEvent( FmRxContext                         *context, 
                                                        FmRxStatus                      status,
                                                        FmRxCmdType                 cmdType,
                                                        FmRxEventType       evtType);


FMC_STATIC FMC_U16 _FM_RX_UTILS_findNextIndex(FmcBand band,FmRxSeekDirection dir, FMC_U16 index);


FMC_STATIC void _getResourceProperties(FmRxAudioTargetMask targetMask,TCAL_ResourceProperties *pProperties);
FMC_STATIC ECAL_ResourceMask _convertRxTargetsToCal(FmRxAudioTargetMask targetMask,TCAL_ResourceProperties *pProperties);
FMC_STATIC FMC_U32 _getRxChannelNumber(void);
FMC_STATIC FMC_U16 _getRxAudioEnableParam(void);
FMC_STATIC void _goToLastStageAndFinishOp(void);
FMC_STATIC void _handleVacOpStateAndMove2NextStage(ECCM_VAC_Status vacStatus);
FMC_STATIC FMC_U16 _getGainValue(FMC_UINT   gain);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void startReadInt(void);
FMC_STATIC void handleReadInt(void);

FMC_STATIC void _FmRxSmHandleFmInterrupt(void);
FMC_STATIC void _FmRxSmHandleGenInterrupts(void);

FMC_STATIC void _FM_RX_SM_ResetStationParams(FMC_U32 freq);

FMC_STATIC void _FM_RX_SM_InitCmdsTable(void);
FMC_STATIC void _FM_RX_SM_ResetRdsData(void);

FMC_STATIC FMC_BOOL checkUpperEvent(void);


FMC_STATIC void FM_FinishPowerOn(FmRxStatus status);


/*FMC_STATIC void send_fm_event_audio_path_changed(FmRxAudioPath audioPath);*/
FMC_STATIC void send_fm_event_ps_changed(FMC_U32 freq, FMC_U8 *data);
FMC_STATIC void send_fm_event_af_list_changed(FMC_U16 pi, FMC_U8 afListSize, FmcFreq *afList);
FMC_STATIC void send_fm_event_af_jump(FmRxEventType eventType,FMC_U8 status, FMC_U16 Pi, FMC_U32 oldFreq, FMC_U32 newFreq);
FMC_STATIC void send_fm_event_radio_text(FMC_BOOL changed, FMC_U8 length, FMC_U8 *radioText,FMC_U8 dataStartIndex);
FMC_STATIC void send_fm_event_most_mode_changed(FMC_U8 mode);
FMC_STATIC void send_fm_event_pty_changed(FmcRdsPtyCode ptyCode);
FMC_STATIC void send_fm_event_pi_changed(FmcRdsPiCode piCode);
FMC_STATIC void send_fm_event_raw_rds(FMC_U16 len, FMC_U8 *data,FmcRdsGroupTypeMask gType);


/* Handlers prototypes */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandlePowerOnStart(void);
FMC_STATIC void HandlePowerOnWakeupFm(void);
FMC_STATIC void HandlePowerOnReadAsicId(void);
FMC_STATIC void HandlePowerOnReadAsicVersion(void);
FMC_STATIC void HandleFmcPowerOnStartInitScript(void);
FMC_STATIC void HandlePowerOnRunScript(void);
FMC_STATIC void HandleFmRxPowerOnStartInitScript(void);
FMC_STATIC void HandlePowerOnRxRunScript(void);
FMC_STATIC void HandlePowerOnApplyDefualtConfiguration(void);
FMC_STATIC void HandlePowerOnEnableInterrupts(void);
FMC_STATIC void HandlePowerOnFinish(void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandlePowerOffStart(void);
FMC_STATIC void HandlePowerOffTransportOff(void);
FMC_STATIC void HandlePowerOffFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMoStSetStart(void);
FMC_STATIC void HandleMoStBlendSetStart(void);
FMC_STATIC void HandleMoStSetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleBandSetStart(void);
FMC_STATIC void HandleBandSetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMuteStart(void);
FMC_STATIC void HandleMuteFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRfDependentMuteStart(void);
FMC_STATIC void HandleRfDependentMuteFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleVolumeSetStart(void);
FMC_STATIC void HandleVolumeSetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRdsSetStart(void);    
FMC_STATIC void HandleRdsOffFinishOnFlushFifo(void);
FMC_STATIC void HandleRdsSetThreshold(void);
FMC_STATIC void HandleRdsSetClearFlag(void);
FMC_STATIC void HandleRdsSetEnableInt(void);
FMC_STATIC void HandleRdsSetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleIsValidChannelStart(void);
FMC_STATIC void HandleIsValidChannelFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRdsUnSetStart(void);  
FMC_STATIC void HandleRdsOffFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleTuneSetPower(void);
FMC_STATIC void HandleTuneSetFreq(void);
FMC_STATIC void HandleTuneClearFlag(void);
FMC_STATIC void HandleTuneEnableInterrupts(void);
FMC_STATIC void HandleTuneStartTuning(void);
FMC_STATIC void HandleTuneWaitStartTuneCmdComplete(void);
FMC_STATIC void HandleTuneFinishedReadFreq(void);
FMC_STATIC void HandleTuneFinishedEnableDefaultInts(void);
FMC_STATIC void HandleTuneFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleChangeAudioTargetsStart(void);
FMC_STATIC void HandleChangeAudioTargetsSendChangeResourceCmdComplete( void);
FMC_STATIC void HandleChangeAudioTargetsStopFmOverBtIfNeeded( void);
FMC_STATIC void HandleChangeAudioTargetsEnableAudioTargets( void);
FMC_STATIC void HandleChangeAudioTargetsFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleChangeDigitalAudioConfigSendChangeConfigurationCmdStart( void);
FMC_STATIC void HandleChangeDigitalAudioConfigChangeDigitalAudioConfigCmdComplete( void);
FMC_STATIC void HandleChangeDigitalAudioConfigEnableAudioSource( void);
FMC_STATIC void HandleChangeDigitalAudioConfigComplete( void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRssiGetStart(void);
FMC_STATIC void HandleRssiGetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSeekMain(void);
FMC_STATIC void HandleSeekStart(void);
FMC_STATIC void HandleSeekSetFreq(void);
FMC_STATIC void HandleSeekSetDir(void);
FMC_STATIC void HandleSeekClearFlag(void);
FMC_STATIC void HandleSeekEnableInterrupts(void);
FMC_STATIC void HandleSeekStartTuning(void);
FMC_STATIC void HandleSeekWaitStartTuneCmdComplete(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleStopSeekStart(void);
FMC_STATIC void HandleStopSeekWaitCmdCompleteOrInt(void);
FMC_STATIC void HandleSeekStopSeekFinishedReadFreq(void);
/*todo ram - This is a workaround due to a FW defect we must Set the current  frequency again at the end of the seek sequence 
            THIS NEED TO BE REMOVED FOR PG 2.0 */
FMC_STATIC void HandleSeekStopSeekFinishedSetFreq(void);
FMC_STATIC void HandleSeekStopSeekFinishedEnableDefaultInts(void);
FMC_STATIC void HandleSeekStopSeekFinish(void);
FMC_STATIC void HandleStopSeekBeforeSeekStarted(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleCompleteScanMain(void);
FMC_STATIC void HandleCompleteScanStart(void);
FMC_STATIC void HandleCompleteScanEnableInterrupts(void);
FMC_STATIC void HandleCompleteScanStartTuning(void);
FMC_STATIC void HandleCompleteScanWaitStartTuneCmdComplete(void);
FMC_STATIC void HandleCompleteScanReadChannels(void);
FMC_STATIC void HandleCompleteScanChannels(void);
FMC_STATIC void HandleCompleteScanFinishEnableDeafultInts(void);
FMC_STATIC void HandleCompleteScanFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleCompleteScanProgressStart(void);
FMC_STATIC void HandleCompleteScanProgressEnd(void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleStopCompleteScanStart(void);
FMC_STATIC void HandleStopCompleteScanWaitCmdCompleteOrInt(void);
FMC_STATIC void HandleCompleteScanStopCompleteScanFinishedReadFreq(void);
FMC_STATIC void HandleCompleteScanStopCompleteScanFinishedEnableDefaultInts(void);
FMC_STATIC void HandleSeekStopSeekFinishReadFreq(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetAFStart(void);
FMC_STATIC void HandleSetAfFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC void HandleSetStereoBlendStart(void);
FMC_STATIC void HandleSetStereoBlendFinish(void);
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetDeemphasisModeStart(void);
FMC_STATIC void HandleSetDeemphasisModeFinish(void);

FMC_STATIC void HandleSetRssiSearchLevelStart(void);
FMC_STATIC void HandleSetRssiSearchLevelFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC void HandleSetPauseLevelStart(void);
FMC_STATIC void HandleSetPauseLevelFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetPauseDurationStart(void);
FMC_STATIC void HandleSetPauseDurationFinish(void);
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetRdsRbdsStart(void);
FMC_STATIC void HandleSetRdsRbdsFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMoStGetStart(void);
FMC_STATIC void HandleMoStGetFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetChannelSpacingStart(void);
FMC_STATIC void HandleSetChannelSpacingFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleGetChannelSpacingStart(void);
FMC_STATIC void HandleGetChannelSpacingFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleGetFwVersionStart(void);
FMC_STATIC void HandleGetFwVersionFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleDisableAudioRoutingStart(void);
FMC_STATIC void HandleDisableAudioRoutingStopVacOperation(void);
FMC_STATIC void HandleDisableAudioRoutingStopVacFmOBtOperation(void);
FMC_STATIC void HandleDisableAudioRoutingFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleEnableAudioRoutingStart(void);
FMC_STATIC void HandleEnableAudioRoutingVacOperationStarted(void);
FMC_STATIC void HandleEnableAudioRoutingEnableAudioTargets(void);
FMC_STATIC void HandleEnableAudioRoutingFinish(void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleGenIntMal(void);
FMC_STATIC void HandleGenIntStereoChange(void);
FMC_STATIC void HandleGenIntRds(void);
FMC_STATIC void HandleGenIntLowRssi(void);
FMC_STATIC void HandleGenIntEnableInt(void);
FMC_STATIC void HandleGenIntFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleReadRdsStart(void);
FMC_STATIC void HandleReadStatusRegisterToAvoidRdsEmptyInterrupt(void);
FMC_STATIC void HandleReadRdsAnalyze(void);
FMC_STATIC void FmHandleRdsRx(void);
/*******************************************************************************************************************/
FMC_STATIC void GetRDSBlock(void);
FMC_STATIC FmcRdsGroupTypeMask handleRdsGroup(FmRxRdsDataFormat  *rdsFormat);
FMC_STATIC void handleRdsGroup0(FmRxRdsDataFormat  *rdsFormat);
FMC_STATIC FMC_BOOL checkNewAf(FMC_U8 af);
FMC_STATIC void handleRdsGroup2(FmRxRdsDataFormat  *rdsFormat);
FMC_STATIC FMC_BOOL rdsParseFunc_CheckIfReachedEndOfText(FMC_U8 * index, FMC_U8 rtIndex);
FMC_STATIC void rdsParseFunc_sendRtAndResetIndexes(FMC_U8 zeroD_Index);
FMC_STATIC void           rdsParseFunc_Switch2DolphinForm(FmRxRdsDataFormat  *rdsFormat);
FMC_STATIC FmcRdsGroupTypeMask rdsParseFunc_getGroupType(FMC_U8 group);
FMC_STATIC FMC_BOOL         rdsParseFunc_updateRepertoire(FMC_U8 byte1,FMC_U8 byte2);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleStereoChange(void);
FMC_STATIC void FmHandleMalfunction(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleAfJump(void);
FMC_STATIC FMC_BOOL isAfJumpValid(void);
FMC_STATIC void initAfJumpParams(void);
FMC_STATIC void HandleAfJumpStartSetPi(void);
FMC_STATIC void HandleAfJumpSetPiMask(void);
FMC_STATIC void HandleAfJumpSetAfFreq(void);
FMC_STATIC void HandleAfJumpEnableInt(void);
FMC_STATIC void HandleAfJumpStartAfJump(void);
FMC_STATIC void HandleAfJumpWaitCmdComplete(void);
FMC_STATIC void HandleAfJumpReadFreq(void);
FMC_STATIC void HandleAfJumpFinished(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleStereoChangeStart(void);
FMC_STATIC void HandleStereoChangeFinish(void);

FMC_STATIC void HandleHwMalStartFinish(void);

FMC_STATIC void HandleTimeoutStart(void);
FMC_STATIC void HandleTimeoutFinish(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetGroupMaskStartEnd(void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
//FMC_STATIC void HandleGetTunedFreqStartEnd(void);
FMC_STATIC void HandleGetTunedFreqStart(void);
FMC_STATIC void HandleGetTunedFreqEnd(void);

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleGetGroupMaskStartEnd(void);
FMC_STATIC void HandleGetRdsSystemStartEnd(void);
FMC_STATIC void HandleGetBandStartEnd(void);
FMC_STATIC void HandleGetMuteModeStartEnd(void);
FMC_STATIC void HandleGetRfDepMuteModeStartEnd(void);
FMC_STATIC void HandleGetRssiThresholdStartEnd(void);
FMC_STATIC void HandleGetDeemphasisFilterStartEnd(void);
FMC_STATIC void HandleGetVolumeStartEnd(void);
FMC_STATIC void HandleGetAfSwitchModeStartEnd(void);
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC void HandleGetAudioRoutingModeStartEnd(void);
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void _FM_RX_SM_Process(void);
FMC_STATIC void _FM_RX_SM_Timer_Process(void);
FMC_STATIC void _FM_RX_SM_Interrupts_Process(void);
FMC_STATIC void _FM_RX_SM_Commands_Process(void);
FMC_STATIC void _FM_RX_SM_Events_Process(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

/* Prototypes */

FMC_STATIC void fm_recvd_transportOnOffCmplt(void);
FMC_STATIC void fm_recvd_initCmdCmplt(void);
FMC_STATIC void fm_recvd_readCmdCmplt(FMC_U8 *data, FMC_UINT    dataLen);
FMC_STATIC void fm_recvd_writeCmdCmplt(void);
FMC_STATIC void fm_recvd_intterupt(void);
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
/* A function that handles the current state of the operation */
typedef void (*FmRxOpCurHandler)(void);

/* An array of handlers of the current operation */
typedef FmRxOpCurHandler* FmRxOpHandlerArray;
typedef struct
{
    FmRxOpHandlerArray opHandlerArray;
    FMC_U8      numOfCmd;
}_FmRxSmCmdInfo;

/* Handlers for FM_RX_CMD_ENABLE */
FMC_STATIC FmRxOpCurHandler powerOnHandler[] = {HandlePowerOnStart,
                                                        HandlePowerOnWakeupFm,
                                                        HandlePowerOnReadAsicId,
                                                        HandlePowerOnReadAsicVersion,
                                                        HandleFmcPowerOnStartInitScript,
                                                        HandlePowerOnRunScript,
                                                        HandleFmRxPowerOnStartInitScript,
                                                        HandlePowerOnRxRunScript,
                                                        HandlePowerOnApplyDefualtConfiguration,
                                                        HandlePowerOnEnableInterrupts,
                                                        HandlePowerOnFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_powerOnHandler = {powerOnHandler, 
                                                        sizeof(powerOnHandler) / sizeof(FmRxOpCurHandler)};
/******************************************************************************************************************************************************
*   These simple commands Defualt values will be set in the enable process
*******************************************************************************************************************************************************/
typedef struct
{
    FmcFwOpcode cmdType;
    FMC_U16 defualtValue;
    
}_FmRxSmDefualtConfigValue;

FMC_STATIC _FmRxSmDefualtConfigValue _fmRxEnableDefualtSimpleCommandsToSet[] = {
    {FMC_FW_OPCODE_RX_MOST_MODE_SET_GET ,FMC_CONFIG_RX_MOST_MODE },
    {FMC_FW_OPCODE_RX_MOST_BLEND_SET_GET, FMC_CONFIG_RX_MOST_BLEND}, 
    {FMC_FW_OPCODE_RX_DEMPH_MODE_SET_GET, FMC_CONFIG_RX_DEMPH_MODE},
    {FMC_FW_OPCODE_RX_SEARCH_LVL_SET_GET, FMC_CONFIG_RX_SEARCH_LVL},
    {FMC_FW_OPCODE_RX_BAND_SET_GET, FMC_CONFIG_RX_BAND},
    {FMC_FW_OPCODE_RX_MUTE_STATUS_SET_GET, FMC_CONFIG_RX_MUTE_STATUS},
    {FMC_FW_OPCODE_RX_RDS_MEM_SET_GET ,FMC_CONFIG_RX_RDS_MEM},
    {FMC_FW_OPCODE_RX_RDS_SYSTEM_SET_GET, FMC_CONFIG_RX_RDS_SYSTEM},
    {FMC_FW_OPCODE_RX_VOLUME_SET_GET, FMC_CONFIG_RX_VOLUME},
    {FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET ,FMC_CONFIG_RX_AUDIO_ENABLE},
    {FMC_FW_OPCODE_CMN_INTX_CONFIG_SET_GET ,FMC_CONFIG_CMN_INTX_CONFIG}
};

/* Handlers for FM_RX_CMD_DISABLE */
FMC_STATIC FmRxOpCurHandler powerOffHandler[] = {HandlePowerOffStart,
                                                            HandlePowerOffTransportOff,
                                                            HandlePowerOffFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_powerOffHandler = {powerOffHandler, 
                                                        sizeof(powerOffHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_MONO_STEREO_MODE */
FMC_STATIC FmRxOpCurHandler mostSetHandler[] = {HandleMoStSetStart,
                                    HandleMoStBlendSetStart,
                                     HandleMoStSetFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_mostSetHandler = {mostSetHandler, 
                                                        sizeof(mostSetHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_BAND */
FMC_STATIC FmRxOpCurHandler bandSetHandler[] = {HandleBandSetStart,
                                                        HandleBandSetFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_bandSetHandler = {bandSetHandler, 
                                                        sizeof(bandSetHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_MUTE_MODE */
FMC_STATIC FmRxOpCurHandler muteHandler[] = {HandleMuteStart,
                                                 HandleMuteFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_muteHandler = {muteHandler, 
                                                        sizeof(muteHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_MUTE_MODE */
FMC_STATIC FmRxOpCurHandler rfDependentMuteHandler[] = {HandleRfDependentMuteStart,
                                                            HandleRfDependentMuteFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_rfDependentMuteHandler = {rfDependentMuteHandler, 
                                                        sizeof(rfDependentMuteHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_VOLUME */
FMC_STATIC FmRxOpCurHandler volumeSetHandler[] = {HandleVolumeSetStart,
                                                            HandleVolumeSetFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_volumeSetHandler = {volumeSetHandler, 
                                                        sizeof(volumeSetHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_ENABLE_RDS */
FMC_STATIC FmRxOpCurHandler rdsSetHandler[] = {HandleRdsSetStart,
                                                      HandleRdsOffFinishOnFlushFifo,
                                                      HandleRdsSetClearFlag,
                                                      HandleRdsSetThreshold,
                                                      HandleRdsSetEnableInt,
                                                      HandleRdsSetFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_rdsSetHandler = {rdsSetHandler, 
                                                        sizeof(rdsSetHandler) / sizeof(FmRxOpCurHandler)};

/* Handlers for FM_RX_CMD_IS_CHANNEL_VALID */
FMC_STATIC FmRxOpCurHandler isValidChannelHandler[] = {HandleIsValidChannelStart,
                                    HandleIsValidChannelFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_isValidChannelHandler = {isValidChannelHandler, 
                                                        sizeof(isValidChannelHandler) / sizeof(FmRxOpCurHandler)};

/* Handlers for FM_RX_CMD_DISABLE_RDS */
FMC_STATIC FmRxOpCurHandler rdsUnSetHandler[] = {HandleRdsUnSetStart,
                                    HandleRdsOffFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_rdsUnSetHandler = {rdsUnSetHandler, 
                                                        sizeof(rdsUnSetHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_TUNE */
FMC_STATIC FmRxOpCurHandler tuneHandler[] = {HandleTuneSetPower,
                                                   HandleTuneSetFreq,
                                                   HandleTuneClearFlag,
                                                   HandleTuneEnableInterrupts,
                                                   HandleTuneStartTuning,
                                                   HandleTuneWaitStartTuneCmdComplete,
                                                   HandleTuneFinishedReadFreq,
                                                   HandleTuneFinishedEnableDefaultInts,
                                                   HandleTuneFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_tuneHandler = {tuneHandler, 
                                                        sizeof(tuneHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_GET_RSSI */
FMC_STATIC FmRxOpCurHandler getRssiHandler[] = {HandleRssiGetStart,
                                                            HandleRssiGetFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getRssiHandler = {getRssiHandler, 
                                                        sizeof(getRssiHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SEEK (and STOP SEEK) */
FMC_STATIC FmRxOpCurHandler seekMainHandler[] = {HandleSeekMain};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_seekMainHandler= {seekMainHandler, 
                                                        sizeof(seekMainHandler) / sizeof(FmRxOpCurHandler)};
/****************************************************************************/
/* Internal seek / stop seek handlers. Called by seekMainHandler            */
FMC_STATIC FmRxOpCurHandler seekHandler[] = {HandleSeekStart,
                                    HandleSeekSetFreq,
                                    HandleSeekSetDir,
                                    HandleSeekClearFlag,
                                    HandleSeekEnableInterrupts,
                                    HandleSeekStartTuning,
                                    HandleSeekWaitStartTuneCmdComplete,
                                    HandleSeekStopSeekFinishedReadFreq,
/*todo ram - This is a work around due to a FW defect we must Set the current  frequency again at the end of the seek sequence 
            THIS NEED TO BE REMOVED FOR PG 2.0 */
                     HandleSeekStopSeekFinishedSetFreq, 
                                    HandleSeekStopSeekFinishedEnableDefaultInts,
                                    HandleSeekStopSeekFinish};

FMC_STATIC FmRxOpCurHandler stopSeekHandler[] = {HandleStopSeekStart,
                                        HandleStopSeekWaitCmdCompleteOrInt,                                                 
                                        HandleSeekStopSeekFinishedReadFreq,
                                        HandleSeekStopSeekFinishedEnableDefaultInts,
                                        HandleSeekStopSeekFinish,
                                        HandleSeekStopSeekFinishedEnableDefaultInts, /*It appears twice on purpose */
                                        HandleStopSeekBeforeSeekStarted};
/****************************************************************************/

/* Handlers for FM_RX_CMD_COMPLETE_SCAN */
FMC_STATIC FmRxOpCurHandler completeScanMainHandler[] = {HandleCompleteScanMain};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_completeScanMainHandler = {completeScanMainHandler, 
                                                        sizeof(completeScanMainHandler) / sizeof(FmRxOpCurHandler)};

/* Internal Complete Scan handlers. Called by HandleCompleteScanMain            */
FMC_STATIC FmRxOpCurHandler completeScanHandler[] = {HandleCompleteScanStart,
                                    HandleCompleteScanEnableInterrupts,
                                    HandleCompleteScanStartTuning,
                                    HandleCompleteScanWaitStartTuneCmdComplete,
                                    HandleCompleteScanReadChannels,
                                    HandleCompleteScanChannels,
                                    HandleCompleteScanFinishEnableDeafultInts,
                                    HandleCompleteScanFinish};

FMC_STATIC FmRxOpCurHandler getCompleteScanProgressHandler[] = {HandleCompleteScanProgressStart,
                                                                HandleCompleteScanProgressEnd};                                                       

FMC_STATIC FmRxOpCurHandler stopCompleteScanHandler[] = {HandleStopCompleteScanStart,
                                                         HandleStopCompleteScanWaitCmdCompleteOrInt,
                                                         HandleCompleteScanStopCompleteScanFinishedReadFreq,
                                                         HandleCompleteScanStopCompleteScanFinishedEnableDefaultInts,
                                                         HandleSeekStopSeekFinishReadFreq};
 
/* 
	Macros related to the stage of the completeScan:
	We can only stop the completeScan in stage HandleCompleteScanWaitStartTuneCmdComplete 
	or HandleCompleteScanReadChannels
*/
#define COMPLETE_SCAN_FIRST_STAGE_TO_STOP_SCAN 3
#define COMPLETE_SCAN_LAST_STAGE_TO_STOP_SCAN 4
 

/* Handlers for FM_RX_CMD_SET_RDS_AF_SWITCH_MODE */
FMC_STATIC FmRxOpCurHandler setAfHandler[] = {HandleSetAFStart,
                                                    HandleSetAfFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setAfHandler = {setAfHandler, 
                                                        sizeof(setAfHandler) / sizeof(FmRxOpCurHandler)};
#if 0 /* warning removal - unused code (yet) */
/* Handlers for FM_CMD_SET_STEREO_BLEND */
FMC_STATIC FmRxOpCurHandler setStereoBlendHandler[] = {HandleSetStereoBlendStart,
                                                                        HandleSetStereoBlendFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setStereoBlendHandler = {setStereoBlendHandler, 
                                                        sizeof(setStereoBlendHandler) / sizeof(FmRxOpCurHandler)};
#endif /* 0 - warning removal - unused code (yet) */

/* Handlers for FM_CMD_SET_DEEMPHASIS_MODE */
FMC_STATIC FmRxOpCurHandler setDeemphasisModeHandler[] = {HandleSetDeemphasisModeStart,
                                                                             HandleSetDeemphasisModeFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setDeemphasisModeHandler = {setDeemphasisModeHandler, 
                                                        sizeof(setDeemphasisModeHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_SET_RSSI_THRESHOLD */
FMC_STATIC FmRxOpCurHandler setRssiSearchLevelHandler[] = {HandleSetRssiSearchLevelStart,
                                                                                HandleSetRssiSearchLevelFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setRssiSearchLevelHandler = {setRssiSearchLevelHandler, 
                                                        sizeof(setRssiSearchLevelHandler) / sizeof(FmRxOpCurHandler)};
#if 0 /* warning removal - unused code (yet) */
/* Handlers for FM_CMD_SET_PAUSE_LEVEL */
FMC_STATIC FmRxOpCurHandler setPauseLevelHandler[] = {HandleSetPauseLevelStart,
                                                                     HandleSetPauseLevelFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setPauseLevelHandler = {setPauseLevelHandler, 
                                                        sizeof(setPauseLevelHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_CMD_SET_PAUSE_DURATION */
FMC_STATIC FmRxOpCurHandler setPauseDurationHandler[] = {HandleSetPauseDurationStart,
                                                                            HandleSetPauseDurationFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setPauseDurationHandler = {setPauseDurationHandler, 
                                                        sizeof(setPauseDurationHandler) / sizeof(FmRxOpCurHandler)};
#endif /* 0 - warning removal - unused code (yet) */

/* Handlers for FM_RX_CMD_SET_RDS_SYSTEM */
FMC_STATIC FmRxOpCurHandler setRdsRbdsModeHandler[] = {HandleSetRdsRbdsStart,
                                                                        HandleSetRdsRbdsFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setRdsRbdsModeHandler = {setRdsRbdsModeHandler, 
                                                        sizeof(setRdsRbdsModeHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_GET_MONO_STEREO_MODE */
FMC_STATIC FmRxOpCurHandler mostGetHandler[] = {HandleMoStGetStart,
                                                        HandleMoStGetFinish};

FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_mostGetHandler = {mostGetHandler, 
                                                        sizeof(mostGetHandler) / sizeof(FmRxOpCurHandler)};


/* Handlers for FM_RX_CMD_SET_CHANNEL_SPACING */
FMC_STATIC FmRxOpCurHandler setChannelSpacingHandler[] = {HandleSetChannelSpacingStart,
                                                        HandleSetChannelSpacingFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setChannelSpacing = {setChannelSpacingHandler, 
                                                        sizeof(setChannelSpacingHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_CMD_GET_CHANNEL_SPACING */
FMC_STATIC FmRxOpCurHandler getChannelSpacingHandler[] = {HandleGetChannelSpacingStart,
                                                        HandleGetChannelSpacingFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getChannelSpacing = {getChannelSpacingHandler, 
                                                        sizeof(getChannelSpacingHandler) / sizeof(FmRxOpCurHandler)};

/* Handlers for FM_RX_CMD_GET_FW_VERSION*/
FMC_STATIC FmRxOpCurHandler getFwVersionHandler[] = {HandleGetFwVersionStart,
                                                        HandleGetFwVersionFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getFwVersion = {getFwVersionHandler, 
                                                        sizeof(getFwVersionHandler) / sizeof(FmRxOpCurHandler)};

FMC_STATIC FmRxOpCurHandler changeAudioTargetHandler[] = {HandleChangeAudioTargetsStart,
                                                        HandleChangeAudioTargetsSendChangeResourceCmdComplete,
                                                        HandleChangeAudioTargetsStopFmOverBtIfNeeded,
                                                        HandleChangeAudioTargetsEnableAudioTargets,
                                                        HandleChangeAudioTargetsFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_changeAudioTargetHandler = {changeAudioTargetHandler, 
                                                        sizeof(changeAudioTargetHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler changeDigitalAudioConfigHandler[] = {HandleChangeDigitalAudioConfigSendChangeConfigurationCmdStart,
                                                        HandleChangeDigitalAudioConfigChangeDigitalAudioConfigCmdComplete,
                                                        HandleChangeDigitalAudioConfigEnableAudioSource,
                                                        HandleChangeDigitalAudioConfigComplete};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_changeDigitalAudioConfigHandler = {changeDigitalAudioConfigHandler, 
                                                        sizeof(changeDigitalAudioConfigHandler) / sizeof(FmRxOpCurHandler)};

/* Handlers for FM_CMD_AUDIO_ENABLE */
FMC_STATIC FmRxOpCurHandler enableAudioRoutingHandler[] = {HandleEnableAudioRoutingStart,
                                                    HandleEnableAudioRoutingVacOperationStarted,
                                                    HandleEnableAudioRoutingEnableAudioTargets,
                                                                HandleEnableAudioRoutingFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_enableAudioRoutingHandler = {enableAudioRoutingHandler, 
                                                        sizeof(enableAudioRoutingHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_CMD_AUDIO_ENABLE */
FMC_STATIC FmRxOpCurHandler disableAudioRoutingHandler[] = {HandleDisableAudioRoutingStart,
                                            HandleDisableAudioRoutingStopVacOperation,
                                            HandleDisableAudioRoutingStopVacFmOBtOperation,
                                                                HandleDisableAudioRoutingFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_disableAudioRoutingHandler = {disableAudioRoutingHandler, 
                                                        sizeof(disableAudioRoutingHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_HANDLE_GEN_INT */
FMC_STATIC FmRxOpCurHandler genIntHandler[] = {HandleGenIntMal,
                                                        HandleGenIntStereoChange,
                                                        HandleGenIntRds,
                                                        HandleGenIntLowRssi,
                                                        HandleGenIntEnableInt,
                                                        HandleGenIntFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_genIntHandler = {genIntHandler, 
                                                        sizeof(genIntHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_READ_RDS */
FMC_STATIC FmRxOpCurHandler readRdsHandler[] = {HandleReadRdsStart,
    HandleReadStatusRegisterToAvoidRdsEmptyInterrupt,

                                                        HandleReadRdsAnalyze};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_readRdsHandler = {readRdsHandler, 
                                                        sizeof(readRdsHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_HANDLE_AF_JUMP */
FMC_STATIC FmRxOpCurHandler afJumpHandler[] = {HandleAfJumpStartSetPi,
                                    HandleAfJumpSetPiMask,
                                    HandleAfJumpSetAfFreq,
                                    HandleAfJumpEnableInt,
                                    HandleAfJumpStartAfJump,
                                    HandleAfJumpWaitCmdComplete,                                                        
                                    HandleAfJumpReadFreq,
                                    HandleAfJumpFinished};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_afJumpHandler = {afJumpHandler, 
                                                        sizeof(afJumpHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_HANDLE_STEREO_CHANGE */
FMC_STATIC FmRxOpCurHandler stereoChangedHandler[] = {HandleStereoChangeStart,
                                                                    HandleStereoChangeFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_stereoChangedHandler = {stereoChangedHandler, 
                                                        sizeof(stereoChangedHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_HANDLE_HW_MAL */
FMC_STATIC FmRxOpCurHandler hwMalHandler[] = {HandleHwMalStartFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_hwMalHandler = {hwMalHandler, 
                                                        sizeof(hwMalHandler) / sizeof(FmRxOpCurHandler)};
/* Handlers for FM_RX_INTERNAL_HANDLE_TIMEOUT */
FMC_STATIC FmRxOpCurHandler timeoutHandler[] = {HandleTimeoutStart,
                                                       HandleTimeoutFinish};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_timeoutHandler = {timeoutHandler, 
                                                        sizeof(timeoutHandler) / sizeof(FmRxOpCurHandler)};

/* Handlers for Get Tuned Freq */
FMC_STATIC FmRxOpCurHandler getTunedFreqHandler[] = {HandleGetTunedFreqStart,
                                                        HandleGetTunedFreqEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getTunedFreqHandler = {getTunedFreqHandler, 
                                                        sizeof(getTunedFreqHandler) / sizeof(FmRxOpCurHandler)};

FMC_STATIC FmRxOpCurHandler getGroupMaskHandler[] = {HandleGetGroupMaskStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getGroupMaskHandler = {getGroupMaskHandler, 
                                                        sizeof(getGroupMaskHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler setGroupMaskHandler[] = {HandleSetGroupMaskStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_setGroupMaskHandler = {setGroupMaskHandler, 
                                                        sizeof(setGroupMaskHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getRdsSystemHandler[] = {HandleGetRdsSystemStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getRdsSystemHandler = {getRdsSystemHandler, 
                                                        sizeof(getRdsSystemHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getBandHandler[] = {HandleGetBandStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getBandHandler= {getBandHandler, 
                                                        sizeof(getBandHandler) / sizeof(FmRxOpCurHandler)};


FMC_STATIC FmRxOpCurHandler getMuteModeHandler[] = {HandleGetMuteModeStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getMuteModeHandler= {getMuteModeHandler, 
                                                        sizeof(getMuteModeHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getRfDepMuteModeHandler[] = {HandleGetRfDepMuteModeStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getRfDepMuteModeHandler= {getRfDepMuteModeHandler, 
                                                        sizeof(getRfDepMuteModeHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getRssiThresholdHandler[] = {HandleGetRssiThresholdStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getRssiThresholdHandler= {getRssiThresholdHandler, 
                                                        sizeof(getRssiThresholdHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getDeemphasisFilterHandler[] = {HandleGetDeemphasisFilterStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getDeemphasisFilterHandler= {getDeemphasisFilterHandler, 
                                                        sizeof(getDeemphasisFilterHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getVolumeHandler[] = {HandleGetVolumeStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getVolumeHandler= {getVolumeHandler, 
                                                        sizeof(getVolumeHandler) / sizeof(FmRxOpCurHandler)};
FMC_STATIC FmRxOpCurHandler getAfSwitchModeHandler[] = {HandleGetAfSwitchModeStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getAfSwitchModeHandler= {getAfSwitchModeHandler, 
                                                        sizeof(getAfSwitchModeHandler) / sizeof(FmRxOpCurHandler)};
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC FmRxOpCurHandler getAudioRoutingModeHandler[] = {HandleGetAudioRoutingModeStartEnd};
FMC_STATIC _FmRxSmCmdInfo _fmRxSmCmdInfo_getAudioRoutingModeHandler= {getAudioRoutingModeHandler, 
                                                        sizeof(getAudioRoutingModeHandler) / sizeof(FmRxOpCurHandler)};
#endif /* 0 - warning removal - unused code (yet) */

FMC_STATIC _FmRxSmCmdInfo fmOpAllHandlersArray[FM_RX_LAST_CMD] ;

FMC_STATIC FMC_U8 smRxNumOfCmdInQueue = 0;

#define FM_RX_VAC_RX_OPERATION_MASK (FM_RX_TARGET_MASK_I2SH|FM_RX_TARGET_MASK_PCMH|FM_RX_TARGET_MASK_FM_ANALOG)


/********************************************************************************
 *
 *
 *
 *******************************************************************************/
FMC_STATIC void _FM_RX_SM_InitCmdsTable(void)
{
    fmOpAllHandlersArray[FM_RX_CMD_ENABLE] = _fmRxSmCmdInfo_powerOnHandler;
    fmOpAllHandlersArray[FM_RX_CMD_DISABLE] = _fmRxSmCmdInfo_powerOffHandler;

    fmOpAllHandlersArray[FM_RX_CMD_SET_BAND] = _fmRxSmCmdInfo_bandSetHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_BAND] = _fmRxSmCmdInfo_getBandHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_MONO_STEREO_MODE] = _fmRxSmCmdInfo_mostSetHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_MONO_STEREO_MODE] = _fmRxSmCmdInfo_mostGetHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_MUTE_MODE] = _fmRxSmCmdInfo_muteHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_MUTE_MODE] = _fmRxSmCmdInfo_getMuteModeHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_RF_DEPENDENT_MUTE_MODE] = _fmRxSmCmdInfo_rfDependentMuteHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_RF_DEPENDENT_MUTE_MODE] = _fmRxSmCmdInfo_getRfDepMuteModeHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_RSSI_THRESHOLD] = _fmRxSmCmdInfo_setRssiSearchLevelHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_RSSI_THRESHOLD] = _fmRxSmCmdInfo_getRssiThresholdHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_DEEMPHASIS_FILTER] = _fmRxSmCmdInfo_setDeemphasisModeHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_DEEMPHASIS_FILTER] = _fmRxSmCmdInfo_getDeemphasisFilterHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_VOLUME] = _fmRxSmCmdInfo_volumeSetHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_VOLUME] = _fmRxSmCmdInfo_getVolumeHandler;

    fmOpAllHandlersArray[FM_RX_CMD_TUNE] = _fmRxSmCmdInfo_tuneHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_TUNED_FREQUENCY] = _fmRxSmCmdInfo_getTunedFreqHandler;

    fmOpAllHandlersArray[FM_RX_CMD_IS_CHANNEL_VALID] = _fmRxSmCmdInfo_isValidChannelHandler;

    fmOpAllHandlersArray[FM_RX_CMD_SEEK] = _fmRxSmCmdInfo_seekMainHandler;


    fmOpAllHandlersArray[FM_RX_CMD_COMPLETE_SCAN] = _fmRxSmCmdInfo_completeScanMainHandler;


    fmOpAllHandlersArray[FM_RX_CMD_GET_RSSI] = _fmRxSmCmdInfo_getRssiHandler;

    fmOpAllHandlersArray[FM_RX_CMD_ENABLE_RDS] = _fmRxSmCmdInfo_rdsSetHandler;
    fmOpAllHandlersArray[FM_RX_CMD_DISABLE_RDS] = _fmRxSmCmdInfo_rdsUnSetHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_RDS_SYSTEM] = _fmRxSmCmdInfo_setRdsRbdsModeHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_RDS_SYSTEM] = _fmRxSmCmdInfo_getRdsSystemHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_RDS_GROUP_MASK] = _fmRxSmCmdInfo_setGroupMaskHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_RDS_GROUP_MASK] = _fmRxSmCmdInfo_getGroupMaskHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_SET_RDS_AF_SWITCH_MODE] = _fmRxSmCmdInfo_setAfHandler;
    fmOpAllHandlersArray[FM_RX_CMD_GET_RDS_AF_SWITCH_MODE] = _fmRxSmCmdInfo_getAfSwitchModeHandler;
    
    fmOpAllHandlersArray[FM_RX_CMD_ENABLE_AUDIO] = _fmRxSmCmdInfo_enableAudioRoutingHandler;
    fmOpAllHandlersArray[FM_RX_CMD_DISABLE_AUDIO] = _fmRxSmCmdInfo_disableAudioRoutingHandler;

    fmOpAllHandlersArray[FM_RX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION] = _fmRxSmCmdInfo_changeDigitalAudioConfigHandler;

    fmOpAllHandlersArray[FM_RX_CMD_CHANGE_AUDIO_TARGET] = _fmRxSmCmdInfo_changeAudioTargetHandler;

    fmOpAllHandlersArray[FM_RX_CMD_SET_CHANNEL_SPACING] = _fmRxSmCmdInfo_setChannelSpacing;
    fmOpAllHandlersArray[FM_RX_CMD_GET_CHANNEL_SPACING] = _fmRxSmCmdInfo_getChannelSpacing;

    fmOpAllHandlersArray[FM_RX_CMD_GET_FW_VERSION] = _fmRxSmCmdInfo_getFwVersion;

    fmOpAllHandlersArray[FM_RX_INTERNAL_HANDLE_GEN_INT] = _fmRxSmCmdInfo_genIntHandler;
    fmOpAllHandlersArray[FM_RX_INTERNAL_READ_RDS] = _fmRxSmCmdInfo_readRdsHandler;
    fmOpAllHandlersArray[FM_RX_INTERNAL_HANDLE_AF_JUMP] = _fmRxSmCmdInfo_afJumpHandler;
    fmOpAllHandlersArray[FM_RX_INTERNAL_HANDLE_STEREO_CHANGE] = _fmRxSmCmdInfo_stereoChangedHandler;
    fmOpAllHandlersArray[FM_RX_INTERNAL_HANDLE_HW_MAL] = _fmRxSmCmdInfo_hwMalHandler;
    fmOpAllHandlersArray[FM_RX_INTERNAL_HANDLE_TIMEOUT] = _fmRxSmCmdInfo_timeoutHandler;
    
}


FMC_STATIC void _FM_RX_SM_InitDefualtValues(void)
{
    _fmRxSmData.upperEventWait = FMC_FALSE; /* indicates whether the upper event should wait for cmd complete or not */ 
    _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_NONE;     /* When calling the operation handler say if it was called because of cmd_complete event, interrupt or upper_event */   
    
    _fmRxSmData.upperEvent = FM_RX_SM_UPPER_EVENT_NONE;     /* Event from the upper api */
    
    
    /*FmRxSeekCmd           seekOp;*/
    
    _fmRxSmData.curAfJumpIndex = 0;     /* Holds the index of the current AF jump */
    _fmRxSmData.freqBeforeJump = FMC_UNDEFINED_FREQ;    /* Will hold the frequency before the jump */
    
    /* Initialize mute variables */
    _fmRxSmData.fmRxReadState = FM_RX_NONE;

    _fmRxSmData.band = FMC_CONFIG_RX_BAND;
    _fmRxSmData.tunedFreq = FMC_UNDEFINED_FREQ;
    _fmRxSmData.volume = FMC_FW_RX_FM_VOLUMN_INITIAL_VALUE/FMC_FW_RX_FM_GAIN_STEP;
    _fmRxSmData.muteMode = FMC_NOT_MUTE;
    _fmRxSmData.rfDependedMute = FM_RX_RF_DEPENDENT_MUTE_OFF;
    _fmRxSmData.rssiThreshold = FMC_CONFIG_RX_SEARCH_LVL;
    _fmRxSmData.afMode = FM_RX_RDS_AF_SWITCH_MODE_OFF ;

    _fmRxSmData.vacParams.monoStereoMode = FMC_CONFIG_RX_MOST_MODE;
    _fmRxSmData.vacParams.audioTargetsMask = FM_RX_TARGET_MASK_FM_ANALOG;
    _fmRxSmData.vacParams.isAudioRoutingEnabled= FMC_FALSE;
    _fmRxSmData.vacParams.eSampleFreq = CAL_SAMPLE_FREQ_8000;
    _fmRxSmData.vacParams.lastOperationStarted = CAL_OPERATION_INVALID;

    _fmRxSmData.deemphasisFilter = FMC_CONFIG_RX_DEMPH_MODE;

    _fmRxSmData.channelSpacing = FMC_CHANNEL_SPACING_100_KHZ;

    _fmRxSmData.rdsOn = FMC_FALSE;
    
    _fmRxSmData.rdsGroupMask = FM_RDS_GROUP_TYPE_MASK_ALL;
    _fmRxSmData.rdsRdbsSystem = FMC_CONFIG_RX_RDS_SYSTEM;

    
        
}
FMC_STATIC void _FM_RX_SM_ResetStationParams(FMC_U32 freq)
{
    /*Was intiated when Low RSSI interrupt was recieved*/
    FMCI_OS_CancelTimer();
    
    _fmRxSmData.tunedFreq= freq;
    _FM_RX_SM_ResetRdsData();

    /* If AF feature is on, enable low level RSSI interrupt in global parameter */
    if(FMC_TRUE == _fmRxSmData.afMode)
    {
        _fmRxSmData.interruptInfo.gen_int_mask |= FMC_FW_MASK_LEV;
    }
}

FMC_STATIC void _FM_RX_SM_ResetRdsData(void)
{
    _fmRxSmData.rdsParams.last_block_index = RDS_BLOCK_INDEX_UNKNOWN; 
    _fmRxSmData.rdsParams.nextPsIndex = RDS_NEXT_PS_INDEX_RESET; 
    _fmRxSmData.rdsParams.psNameSameCount = 0;
    _fmRxSmData.curStationParams.piCode = NO_PI_CODE; 
    _fmRxSmData.curStationParams.afListSize = 0; 
    FMC_OS_MemCopy(_fmRxSmData.curStationParams.psName, (FMC_U8*)("\0"), RDS_PS_NAME_SIZE); 
    
    _fmRxSmData.rdsParams.prevABFlag = RDS_PREV_RT_AB_FLAG_RESET; 
    /*when we reset the rds data should be cleaned*/
    _fmRxSmData.rdsParams.abFlagChanged = FMC_TRUE; 
    _fmRxSmData.rdsParams.wasRepertoireUpdated = FMC_FALSE; 
    _fmRxSmData.rdsParams.rtLength = 0;
    _fmRxSmData.rdsParams.nextRtIndex = RDS_NEXT_RT_INDEX_RESET; 
    
    _fmRxSmData.rdsData.piCode = 0;
    _fmRxSmData.rdsData.ptyCode= FMC_RDS_PTY_CODE_NO_PROGRAM_UNDEFINED;
    _fmRxSmData.rdsData.ta = FMC_RDS_TA_OFF;
    _fmRxSmData.rdsData.tp = FMC_RDS_TP_OFF;
    _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
}

FmRxStatus FM_RX_SM_Init(void)
{
    FMC_FUNC_START("FM_RX_SM_Init");

    FMC_OS_MemSet(&_fmRxSmData.context, 0, sizeof(FmRxContext));
    
    _FM_RX_SM_InitCmdsTable();
    _FM_RX_SM_InitDefualtValues();
    
    _FM_RX_SM_ResetStationParams(FMC_UNDEFINED_FREQ);
    
    
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_DESTROYED;
    _fmRxSmData.currCmdInfo.smState = _FM_RX_SM_STATE_NONE;

#if obc
    CCM_VAC_RegisterCallback(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX,_FM_RX_SM_TccmVacCb);
    CCM_VAC_RegisterCallback(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_SCO,_FM_RX_SM_TccmVacCb);
    CCM_VAC_RegisterCallback(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_A3DP,_FM_RX_SM_TccmVacCb);
#endif
    FMC_FUNC_END();
    
    return FM_RX_STATUS_SUCCESS;
}

FmRxStatus FM_RX_SM_Deinit(void)
{
    FMC_FUNC_START("FM_RX_SM_Deinit");

    FMC_FUNC_END();
    
    return FM_RX_STATUS_SUCCESS;
}

FmRxStatus FM_RX_SM_Create(const FmRxCallBack fmCallback, FmRxContext **fmContext)
{
    FMC_FUNC_START("FM_RX_SM_Create");

    /* [ToDo] Protect against multiple creations for the same context */
    
    _fmRxSmData.currCmdInfo.baseCmd = NULL;
    
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_DISABLED;
    

    _fmRxSmData.context.appCb = fmCallback;


    /* Currently, there is a single context */
    *fmContext = &_fmRxSmData.context;
    
    FMC_FUNC_END();
    
    return FM_RX_STATUS_SUCCESS;
}

FmRxStatus FM_RX_SM_Destroy(FmRxContext **fmContext)
{
    FmRxStatus status = FM_RX_STATUS_SUCCESS;
    
    FMC_FUNC_START("FM_RX_SM_Destroy");
    
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_DESTROYED;
    _fmRxSmData.context.appCb = NULL;

    *fmContext = NULL;
    
    FMC_FUNC_END();
    
    return status;
}

FmRxContext* FM_RX_SM_GetContext(void)
{
    return &_fmRxSmData.context;

}

FmRxSmContextState FM_RX_SM_GetContextState(void)
{
    return _fmRxSmData.context.state;

}

FMC_BOOL FM_RX_SM_IsContextEnabled(void)
{
    if (    (_fmRxSmData.context.state != FM_RX_SM_CONTEXT_STATE_DISABLED)&&
        (_fmRxSmData.context.state != FM_RX_SM_CONTEXT_STATE_DISABLING)&&
        (_fmRxSmData.context.state != FM_RX_SM_CONTEXT_STATE_DESTROYED)&&
        (_fmRxSmData.context.state != FM_RX_SM_CONTEXT_STATE_ENABLING))
    {
        return FMC_TRUE;
    }   
    else
    {
        return FMC_FALSE;
    }
}
FmRxCmdType FM_RX_SM_GetRunningCmd(void)
{
    if(_fmRxSmData.currCmdInfo.baseCmd!= NULL)
    {
        return _fmRxSmData.currCmdInfo.baseCmd->cmdType;
    }
    else
    {
        /* No Cmd running don't retrurn any commands value*/
        return FM_RX_CMD_NONE;
    }
}
void FM_RX_SM_SetUpperEvent(FMC_U8 upperEvt)
{
    _fmRxSmData.upperEvent = upperEvt; 
}


/*
        Verify disable status , and send Destory accordingly.        
*/

FmRxStatus FM_RX_SM_Verify_Disable_Status(FmRxStatus status)
{
  FmRxContext *context=FM_RX_SM_GetContext();

     /*
        Verify that the disable was successfully.
    */
    if (status!=FM_RX_STATUS_SUCCESS)
        {
         return FM_RX_STATUS_FAILED; 
        }
    /*
        Need to destroy after the Disable proccess 
    */
    status = FM_RX_SM_Destroy(&context);
    if(status !=FM_RX_STATUS_SUCCESS)
        if (status!=FM_RX_STATUS_SUCCESS)
        {
             return FM_RX_STATUS_FAILED; 
        }

    return FM_RX_STATUS_SUCCESS;

}



/********************************************************************************
 *
 * 
 *
 *******************************************************************************/

void Fm_Rx_Stack_EventCallback(FmcOsEvent evtMask)
{
    if (evtMask & FMC_OS_EVENT_FMC_STACK_TASK_PROCESS)
    {
        _FM_RX_SM_Process();
    }
    if (evtMask & FMC_OS_EVENT_FMC_TIMER_EXPIRED)
    {
        _FM_RX_SM_Timer_Process();
    }
}

FMC_STATIC void _FM_RX_SM_Process(void)
{
    /*********************************************************************
        KEEP THIS ORDER. WHEN HANDLING FIRST THE INTERRUPTS IT CAN 
        MAKE A DEADLOCK FOR THE COMMANDS. WHEN RECEIVING INTERRUPT
        ALL THE TIME (LIKE LOW_RSSI WHEN AF_ON AND WE ARE ON BAD CHANNEL) 
        AND TRYING TO SEND COMMAND IT MIGHT TAKE VERY LONG UNTIL IT WILL 
        BE DONE.
    *********************************************************************/

    /* If we are not in the middle of first handling of the interrupt
       (Read mask/flag and handle if it's operation-specific */
    if(_fmRxSmData.interruptInfo.readIntState != INT_STATE_READ_INT)
    {
        _FM_RX_SM_Events_Process();
        _FM_RX_SM_Commands_Process();
    }
    _FM_RX_SM_Interrupts_Process();
}
/*---------------------------------------------------------------------------
 *            _FM_RX_SM_Events_Process()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  If possible start the next operation on the queue. 
 *
 * Return:    void
 */
FMC_STATIC void _FM_RX_SM_Events_Process(void)
{
    /* If a Command Complete event was received */
    if (_fmRxSmData.context.transportEventData.eventWaitsForProcessing) 
    {
        _fmRxSmData.fmRxReadState = FM_RX_NONE;
        /* An interrupt was received before but we had to wait for a cmd_complete event
           to arrive before we can start handling it. Now event arrived and we can start 
           handling the interrupt */
        if(_fmRxSmData.interruptInfo.readIntState == INT_STATE_WAIT_FOR_CMD_CMPLT)
        {
            startReadInt();
        }
        /* No interrupt was waiting - now we can handle the cmd complete event.
           If the upper event waited for cmd complete -
           call the operation handler only with upper event reason,
           Otherwise - call it with cmd complete reason. */
        else 
        {

            _fmRxSmData.currCmdInfo.smState = _FM_RX_SM_STATE_NONE;
            _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_FALSE;
            
            
            /* If the upper event waited for the cmd complete - call the 
               Operation handler with upper event and ignore the cmd complete */
            if(_fmRxSmData.upperEventWait) 
            {
                _fmRxSmData.upperEventWait = FMC_FALSE; 
                
                /* Only if the current event fits the current running operation.
                   It will only be sent in that case but this check is to prevent 
                   race condition */
                if(checkUpperEvent())
                {
                    _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_UPPER_EVT; 
                }
                else
                {
                    _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT; 
                }

                fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
           
				/* Resetting the upperEvent must appear after the checkUpperEvent and after the function call */
                _fmRxSmData.upperEvent = FM_RX_SM_UPPER_EVENT_NONE; 

            }
            
            else
            {
                /* Call the handler with callReason cmd_complete event */
                _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT; 
                fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
            }
        }
    }

    /* If an event from the upper API received - handle it */
    if(_fmRxSmData.upperEvent != FM_RX_SM_UPPER_EVENT_NONE) 
    {
        if(_fmRxSmData.currCmdInfo.smState== _FM_RX_SM_STATE_WAITING_FOR_CC)
        {
            _fmRxSmData.upperEventWait = FMC_TRUE; 
        }
        else
        {           
            /* Only if the current event fits the current running operation.
               It will only be sent in that case but this check is to prevent 
               race condition */
            if(checkUpperEvent())
            {
                _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_UPPER_EVT; 
                fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
            }
            /* Resetting the upperEvent must appear after the checkUpperEvent */
            _fmRxSmData.upperEvent = FM_RX_SM_UPPER_EVENT_NONE; 

        }
    }
}
/*---------------------------------------------------------------------------
 *            _FM_RX_SM_Commands_Process()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  If possible start the next operation on the queue. 
 *
 * Return:    void
 */
FMC_STATIC void _FM_RX_SM_Commands_Process(void)
{
    /* Start the next command only if there is no current command handled */
    if (_fmRxSmData.currCmdInfo.baseCmd == NULL) 
    {
        /* There is no active operation so start the next one on the queue */
        if (FMC_IsListEmpty(FMCI_GetCmdsQueue()) == FMC_FALSE) 
        {
            /* Lock the stack when changing thecurOp */
            _fmRxSmData.currCmdInfo.baseCmd = (FmcBaseCmd*)FMC_GetHeadList(FMCI_GetCmdsQueue());
            
            _fmRxSmData.currCmdInfo.stageIndex = 0;
            _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
            /* We copy the param in order to allow the application to send another 
               command of the same type without overriding the current performed operation */
        

            _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_START; 
                fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
        }
    }
}

/* Handle received interrupts: Read the flag and mask in order to figure
   out if this is a general interrupt or a specific-operation interrupt.
   A general interrupt will be handled after the operation is finished */
FMC_STATIC void _FM_RX_SM_Interrupts_Process(void)
{
    /* An interrupt was received */
    if(_fmRxSmData.interruptInfo.interruptInd)
    {
        /* If we are waiting for cmd complete - wait to receive it before handling
           the interrupt */
        if(_fmRxSmData.currCmdInfo.smState == _FM_RX_SM_STATE_WAITING_FOR_CC)
        {
            _fmRxSmData.interruptInfo.readIntState = INT_STATE_WAIT_FOR_CMD_CMPLT;
        }
        /* Not waiting for cmd complete - can start handling the interrupt */
        else
        {
            startReadInt();
        }
    }

    /* This event is set when we receive a cmd complete for the read flag
       and read mask commands - we can proceed in handling the interrupt */
    if(_fmRxSmData.interruptInfo.intReadEvt)
    {
        _fmRxSmData.interruptInfo.intReadEvt = FMC_FALSE;
        
        handleReadInt();
    }

}

FMC_STATIC FMC_BOOL checkUpperEvent(void)
{
    switch(_fmRxSmData.upperEvent) 
    {
        case FM_RX_SM_UPPER_EVENT_STOP_SEEK:
            if((_fmRxSmData.currCmdInfo.baseCmd!= NULL) && (_fmRxSmData.currCmdInfo.baseCmd->cmdType == FM_RX_CMD_SEEK))
                return FMC_TRUE;
            else
                return FMC_FALSE;
        case FM_RX_SM_UPPER_EVENT_GET_COMPLETE_SCAN_PROGRESS:
            if((_fmRxSmData.currCmdInfo.baseCmd!= NULL) && (_fmRxSmData.currCmdInfo.baseCmd->cmdType == FM_RX_CMD_COMPLETE_SCAN))
                return FMC_TRUE;
            else
                return FMC_FALSE;

        case FM_RX_SM_UPPER_EVENT_STOP_COMPLETE_SCAN:
            if((_fmRxSmData.currCmdInfo.baseCmd!= NULL) && (_fmRxSmData.currCmdInfo.baseCmd->cmdType == FM_RX_CMD_COMPLETE_SCAN))
                return FMC_TRUE;
            else
                return FMC_FALSE;

        default:
            return FMC_FALSE;
    }
}
/*---------------------------------------------------------------------------
 *            _FM_RX_SM_Timer_Process()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  handel timeout event.
 *
 * Return:    void
 */
FMC_STATIC void _FM_RX_SM_Timer_Process(void)
{
    FmRxGenCmd * timeout = NULL;
    FMC_LOG_INFO(("AF suspension timeout finished"));

    /* Check if the command is not already waiting in the queue */
    if(FM_RX_SM_IsCmdPending(FM_RX_INTERNAL_HANDLE_TIMEOUT))
    {
        FMC_LOG_INFO(("Timeout operation is already pending"));
    }
    else
    {

        /* Lock the stack when inserting the operation into the queue */
        FM_RX_SM_AllocateCmdAndAddToQueue(&_fmRxSmData.context,
                                                FM_RX_INTERNAL_HANDLE_TIMEOUT,
                                                (FmcBaseCmd **)&timeout);

        /* Notify FM stack about the operation */
        FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
    }
}




FMC_STATIC void startReadInt(void)
{
    /* Start handling the interrupt - Clear the flag */
    _fmRxSmData.interruptInfo.interruptInd = FMC_FALSE;

    /* Do nothing, if we are going power down or already OFF or we are in the enable process */
    if (!FM_RX_SM_IsContextEnabled())

    {
       _fmRxSmData.interruptInfo.readIntState = INT_STATE_NONE;
        FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
    }
    else
    {
        _fmRxSmData.interruptInfo.readIntState = INT_STATE_READ_INT;
        handleReadInt();
   }
}

FMC_STATIC void handleReadInt(void)
{
    FMC_STATIC FMC_U8 int_state = INT_READ_FLAG;
    
    if(int_state == INT_READ_FLAG)
    {
        int_state = INT_HANDLE_INT;
        _fmRxSmData.fmRxReadState = FM_RX_READING_STATUS_REGISTER;
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
    }
    else
    {
        int_state = INT_READ_FLAG;
        _fmRxSmData.interruptInfo.readIntState = INT_STATE_NONE;
        
        _FmRxSmHandleFmInterrupt();
    }
}

/* This function is called after reading the interrupt flag. 
    it can be either a general interrupt or a specific interrupt that
    the operation enabled */
FMC_STATIC void _FmRxSmHandleFmInterrupt(void)
{
    FMC_U16 intSetBits = (FMC_U16)(_fmRxSmData.interruptInfo.fmFlag & _fmRxSmData.interruptInfo.fmMask);

    /* If one of the bits of the opHandler interrupt occurred - call it */
    if(intSetBits & (_fmRxSmData.interruptInfo.opHandler_int_mask))
    {
        _fmRxSmData.interruptInfo.opHandlerIntSetBits = intSetBits;
        _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_INTERRUPT; 

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    }
    /* If it's one of the general interrupts - add it to the queue and 
       handle it after the current operation is done */
    /* Note: A scenario can happen that an interrupt occur and the flag and mask are read
       right after we clear the flag in the middle of an operation (like seek). in 
       this situation we will read flag 0 - so we will handle it like a general int
       just in order to enable the interrupts again. */
    else
    {
        _fmRxSmData.interruptInfo.genIntSetBits = intSetBits;
        _FmRxSmHandleGenInterrupts();       
    }

}
FMC_STATIC void _FmRxSmHandleGenInterrupts(void)
{
    FmRxGenCmd * genInterrupts = NULL;
    /* If we already have a general interrupt in the queue - no need to add another one.
       Note: This situation can happen when there is a general interrupt in the queue, then we
       start an operation with interrupt (like seek) and then after receiving the interrupt
       and enabling again the general interrupt - we will receive an interrupt again and try
       to add it to the queue */
    if(FM_RX_SM_IsCmdPending(FM_RX_INTERNAL_HANDLE_GEN_INT))
    {       
        return;
    }

    /* Add the general interrupts handling to the beginning of the list to generate it before other operations */
    FM_RX_SM_AllocateCmdAndAddToQueue(&_fmRxSmData.context,
                                            FM_RX_INTERNAL_HANDLE_GEN_INT,
                                            (FmcBaseCmd **)&genInterrupts);
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}



FMC_STATIC void prepareNextStage(_FmRxSmState wait, FMC_U8 nextStage)
{
    /* Update the operation handler */
    if(nextStage == INCREMENT_STAGE)
    {
        _fmRxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        _fmRxSmData.currCmdInfo.stageIndex = nextStage; 
    }

    _fmRxSmData.currCmdInfo.smState = wait;
}
/******************************************************************************************************************
 *******************************************************************************************************************
 *******************************************************************************************************************
 *******************************************************************************************************************
 *******************************************************************************************************************
 *******************************************************************************************************************/
FMC_STATIC void HandlePowerOnStart(void)
{
    FmcStatus status = FM_RX_STATUS_FAILED;;

    if(_fmRxSmData.context.state == FM_RX_SM_CONTEXT_STATE_DISABLED)
    {
        /* Direct transport events to me from now on */
        status = FMC_CORE_SetCallback(_FM_RX_SM_TransportEventCb);
        
        _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_ENABLING;
        
        status = FMC_CORE_TransportOn();

        if (status == FM_RX_STATUS_SUCCESS)
        {
            prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);
            
                fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
        }
        else if(status == FM_RX_STATUS_PENDING)
        {
            /* Else - need to wait for the FM On to finish and call the callback */
            prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
        }
        else
        {
            FM_FinishPowerOn(FM_RX_STATUS_FAILED);
        }
    }
    else
    {
        FM_FinishPowerOn(status);
    }
}

FMC_STATIC void HandlePowerOnWakeupFm(void)
{

    _FM_RX_SM_ResetStationParams(FMC_UNDEFINED_FREQ);
    
    _fmRxSmData.interruptInfo.gen_int_mask = FMC_FW_MASK_MAL | FMC_FW_MASK_STIC;
    
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    
    /* Wakeup the FM module.
       In UART mode - this command mux the I2C to work internally and wakeup the FM module.
       In I2C mode - Power_Set command needs to be sent in order to wakeup the FM. 
       No Ack should be received. */
    FMC_CORE_SendPowerModeCommand(FMC_TRUE);
    /* Must wait 20msec before starting to send commands to the FM. As it may
     * take time to stack to send this command to the chip, we will start delay
     * after receiving Command Complete Event */
}

FMC_STATIC void HandlePowerOnReadAsicId(void)
{   
    /* Must wait 20msec before starting to send commands to the FM after command
     * FMC_FW_FM_CORE_POWER_UP was sent. */
    FMC_OS_Sleep(FMC_CONFIG_WAKEUP_TIMEOUT_MS);

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send read version command */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_ASIC_ID_GET,2);
}
FMC_STATIC void HandlePowerOnReadAsicVersion(void)
{   
    _fmRxSmData.fmAsicId = _fmRxSmData.context.transportEventData.read_param;
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send read version command */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_ASIC_VER_GET,2);
}
FMC_STATIC void HandleFmcPowerOnStartInitScript(void)
{
    FMC_U8  fmStatus = FM_RX_STATUS_SUCCESS;
    McpBtsSpStatus              btsSpStatus;
    McpBtsSpScriptLocation      scriptLocation;
    McpBtsSpExecuteScriptCbData scriptCbData;
    char                        fileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS *
                                         MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];
    McpUtf8                     scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                                                   MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];


    _fmRxSmData.fmAsicVersion = _fmRxSmData.context.transportEventData.read_param;
    
    MCP_HAL_STRING_Sprintf(fileName, "%s_%x.%d.bts", 
                           FMC_CONFIG_SCRIPT_FILES_FMC_INIT_NAME,
                           _fmRxSmData.fmAsicId,
                           _fmRxSmData.fmAsicVersion);

    MCP_StrCpyUtf8(scriptFullFileName,
                   (const McpUtf8 *)FMC_CONFIG_SCRIPT_FILES_FULL_PATH_LOCATION);
    MCP_StrCatUtf8(scriptFullFileName, (const McpUtf8 *)fileName);

    scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    scriptLocation.locationData.fullFileName = scriptFullFileName;
    
    scriptCbData.sendHciCmdCb = _FM_RX_SM_TiSpSendHciScriptCmd;
    scriptCbData.setTranParmsCb = NULL;
    scriptCbData.execCompleteCb = _FM_RX_SM_TiSpExecuteCompleteCb;
    
    btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmRxSmData.context.tiSpContext);

    if (btsSpStatus == MCP_BTS_SP_STATUS_FILE_NOT_FOUND)
    {       
        /* Script was not found in FFS => check in ROM */
        
        McpBool memInitScriptFound;
        char scriptFileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS];

        FMC_LOG_INFO(("%s Not Found on FFS - Checking Memory", scriptFullFileName));

        MCP_HAL_STRING_Sprintf((char *)scriptFileName, "%s_%x.%d.bts", 
                            FMC_CONFIG_SCRIPT_FILES_FMC_INIT_NAME,
                            _fmRxSmData.fmAsicId, _fmRxSmData.fmAsicVersion);

        /* Check if memory has a script for this version and load its parameters if found */
        memInitScriptFound =
            MCP_RomScriptsGetMemInitScriptData((const char *)scriptFileName, 
                                               &scriptLocation.locationData.memoryData.size,
                                               (const McpU8 **)(&scriptLocation.locationData.memoryData.address));

        if (memInitScriptFound == MCP_TRUE)
        {
            /* Updating location, rest of fields were already set above and are correct as is */            
            scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_MEMORY;

            /* Retry script execution - this time from memory */
            btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmRxSmData.context.tiSpContext);
        }

        /* Now continue checking the return status... */
    }
    
    if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* advance 2 stages to HandlePowerOnFinish()*/
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);        
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);    
        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    }
    else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
    {
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);      
    }
    else
    {
        fmStatus = FM_RX_STATUS_FAILED;
        FM_FinishPowerOn(fmStatus);
    }

}
FMC_STATIC void HandlePowerOnRunScript(void)
{
    if(_fmRxSmData.context.state == FM_RX_SM_CONTEXT_STATE_ENABLING)
    {
        _fmRxSmData.currCmdInfo.smState = _FM_RX_SM_STATE_WAITING_FOR_CC;
        MCP_BTS_SP_HciCmdCompleted(&_fmRxSmData.context.tiSpContext, 
                                NULL,
                                0);
    }
}
FMC_STATIC void HandleFmRxPowerOnStartInitScript(void)
{
    FMC_U8  fmStatus = FM_RX_STATUS_SUCCESS;
    McpBtsSpStatus              btsSpStatus;
    McpBtsSpScriptLocation      scriptLocation;
    McpBtsSpExecuteScriptCbData scriptCbData;
    char                        fileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS *
                                         MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];
    McpUtf8                     scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                                                   MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];

    MCP_HAL_STRING_Sprintf(fileName, "%s_%x.%d.bts", 
                           FMC_CONFIG_SCRIPT_FILES_FM_RX_INIT_NAME,
                           _fmRxSmData.fmAsicId,
                           _fmRxSmData.fmAsicVersion);

    MCP_StrCpyUtf8(scriptFullFileName,
                   (const McpUtf8 *)FMC_CONFIG_SCRIPT_FILES_FULL_PATH_LOCATION);
    MCP_StrCatUtf8(scriptFullFileName, (const McpUtf8 *)fileName);

    scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    scriptLocation.locationData.fullFileName = scriptFullFileName;
    
    scriptCbData.sendHciCmdCb = _FM_RX_SM_TiSpSendHciScriptCmd;
    scriptCbData.setTranParmsCb = NULL;
    scriptCbData.execCompleteCb = _FM_RX_SM_TiSpExecuteCompleteCb;
    
    btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmRxSmData.context.tiSpContext);

    if (btsSpStatus == MCP_BTS_SP_STATUS_FILE_NOT_FOUND)
    {       
        /* Script was not found in FFS => check in ROM */
        
        McpBool memInitScriptFound;
        char scriptFileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS];

        FMC_LOG_INFO(("%s Not Found on FFS - Checking Memory", scriptFullFileName));

        MCP_HAL_STRING_Sprintf((char *)scriptFileName,
                               "%s_%x.%d.bts", 
                               FMC_CONFIG_SCRIPT_FILES_FM_RX_INIT_NAME,
                               _fmRxSmData.fmAsicId,
                               _fmRxSmData.fmAsicVersion);

        /* Check if memory has a script for this version and load its parameters if found */
        memInitScriptFound =
            MCP_RomScriptsGetMemInitScriptData((const char *)scriptFileName, 
                                               &scriptLocation.locationData.memoryData.size,
                                               (const McpU8 **)(&scriptLocation.locationData.memoryData.address));

        if (memInitScriptFound == MCP_TRUE)
        {
            /* Updating location, rest of fields were already set above and are correct as is */            
            scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_MEMORY;

            /* Retry script execution - this time from memory */
            btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmRxSmData.context.tiSpContext);
        }

        /* Now continue checking the return status... */
    }
    
    if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* advance 2 stages to HandlePowerOnFinish()*/
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);        
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);    
        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    }
    else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
    {
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);      
    }
    else
    {
        fmStatus = FM_RX_STATUS_FAILED;
        FM_FinishPowerOn(fmStatus);
    }

}
FMC_STATIC void HandlePowerOnRxRunScript(void)
{
    if(_fmRxSmData.context.state == FM_RX_SM_CONTEXT_STATE_ENABLING)
    {
        _fmRxSmData.currCmdInfo.smState = _FM_RX_SM_STATE_WAITING_FOR_CC;
        MCP_BTS_SP_HciCmdCompleted(&_fmRxSmData.context.tiSpContext, 
                                NULL,
                                0);
    }
}
FMC_STATIC void HandlePowerOnApplyDefualtConfiguration(void)
{
    FMC_STATIC FMC_U8 configStage = 0;
    
    if(configStage< sizeof(_fmRxEnableDefualtSimpleCommandsToSet)/sizeof(_FmRxSmDefualtConfigValue))
    {
        FMC_CORE_SendWriteCommand(_fmRxEnableDefualtSimpleCommandsToSet[configStage].cmdType, 
                                            _fmRxEnableDefualtSimpleCommandsToSet[configStage].defualtValue);
        /* config the current value and move to the value to configure*/
        configStage++;
    }
    else
    {   
        /*All simple defualt values are configured continue*/
        configStage = 0;
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);    
        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    }
    
}
FMC_STATIC void HandlePowerOnEnableInterrupts(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    
    _fmRxSmData.interruptInfo.fmMask =  _fmRxSmData.interruptInfo.gen_int_mask;
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.gen_int_mask);
}

FMC_STATIC void HandlePowerOnFinish(void)
{
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_ENABLED;
    FM_FinishPowerOn(FM_RX_STATUS_SUCCESS);
}

FMC_STATIC void FM_FinishPowerOn(FmRxStatus status)
{
    _fmRxSmData.currCmdInfo.status = status;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
    
    /* wakeup the stack to check whether more commands are available */
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandlePowerOffStart(void)
{
    _fmRxSmData.interruptInfo.gen_int_mask = 0;
    
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_DISABLING;    
   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send power off command */
    FMC_CORE_SendPowerModeCommand(FMC_FALSE);
}
FMC_STATIC void HandlePowerOffTransportOff(void)
{
    FmcStatus status;
    
    status = FMC_CORE_TransportOff();
    
    if (status == FMC_STATUS_SUCCESS)
    {
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    }
    else
    {
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    }

}

FMC_STATIC void HandlePowerOffFinish(void)
{
    FMC_CORE_SetCallback(NULL);

    _FM_RX_SM_ResetStationParams(FMC_UNDEFINED_FREQ);
    /* Send event to the applicatoin */
    _fmRxSmData.context.state = FM_RX_SM_CONTEXT_STATE_DISABLED;
    
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);

    FMCI_SetEventCallback(NULL);
    
    
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMoStSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_MOST_MODE_SET_GET, 
                                (FMC_U16)((FmRxMoStSetCmd *)(_fmRxSmData.currCmdInfo.baseCmd))->mode);

}
FMC_STATIC void HandleMoStBlendSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set Stereo Blend(soft) command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_MOST_BLEND_SET_GET, 1);

}
FMC_STATIC void HandleMoStSetFinish(void)
{
    _fmRxSmData.vacParams.monoStereoMode = ((FmRxMoStSetCmd *)(_fmRxSmData.currCmdInfo.baseCmd))->mode;
    /* Send event to the applicatoin */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleBandSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_BAND_SET_GET, 
                                (FMC_U16)((FmRxBandSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode);
}
FMC_STATIC void HandleBandSetFinish(void)
{
    _fmRxSmData.band = ((FmRxBandSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode;
    /* Send event to the applicatoin */
    
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMuteStart(void)
{
    FMC_U8 muteMode = FMC_FW_RX_MUTE_UNMUTE_MODE;
    FmRxMuteCmd * muteCmd = (FmRxMuteCmd *)_fmRxSmData.currCmdInfo.baseCmd;
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    switch(muteCmd->mode)
    {
        case FMC_MUTE:
            muteMode = FMC_FW_RX_MUTE_AC_MUTE_MODE;
            break;
        case FMC_ATTENUATE:
            muteMode = FMC_FW_RX_MUTE_SOFT_MUTE_FORCE_MODE;
            break;
        case FMC_NOT_MUTE:
            muteMode = FMC_FW_RX_MUTE_UNMUTE_MODE;
            break;
        default:
            FMC_LOG_INFO(("HandleMuteStart muteMode not define"));
            FMC_ASSERT(0);          
    }

    /* Update the mute mode with RF dependent bit */
    if (FM_RX_RF_DEPENDENT_MUTE_ON == _fmRxSmData.rfDependedMute)
    {
        muteMode |= FMC_FW_RX_MUTE_RF_DEP_MODE;
    }
    else
    {
        muteMode &= ~FMC_FW_RX_MUTE_RF_DEP_MODE;
    }
    _fmRxSmData.muteMode = muteCmd->mode;
    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_MUTE_STATUS_SET_GET, (FMC_U16)muteMode);
}
FMC_STATIC void HandleMuteFinish(void)
{
    /* Send event to the applicatoin */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRfDependentMuteStart(void)
{
    FmcMuteMode muteMode = _fmRxSmData.muteMode;
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    _fmRxSmData.rfDependedMute =    ((FmRxSimpleSetOneParamCmd *)_fmRxSmData.currCmdInfo.baseCmd)->paramIn;
    /* Update the mute mode with RF dependent bit */
    if (FM_RX_RF_DEPENDENT_MUTE_ON == _fmRxSmData.rfDependedMute)
    {
        muteMode |= FMC_FW_RX_MUTE_RF_DEP_MODE;
    }
    else
    {
        muteMode &= ~FMC_FW_RX_MUTE_RF_DEP_MODE;
    }
    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_MUTE_STATUS_SET_GET, (FMC_U16)muteMode);
}
FMC_STATIC void HandleRfDependentMuteFinish(void)
{
    /* Send event to the applicatoin */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleVolumeSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_VOLUME_SET_GET,
                                _getGainValue((FMC_U16)((FmRxVolumeSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->gain));
}
FMC_STATIC void HandleVolumeSetFinish(void)
{
    _fmRxSmData.volume = (FMC_UINT)((FmRxVolumeSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->gain;

    /* Send event to the applicatoin */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRdsSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_POWER_SET_GET, FMC_FW_RX_POWER_SET_FM_AND_RDS_ON);
}


FMC_STATIC void HandleRdsOffFinishOnFlushFifo(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_CNTRL_SET, FMC_FW_RX_RDS_FLUSH_FIFO);
}

FMC_STATIC void HandleRdsSetThreshold(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_MEM_SET_GET, FMC_FW_RX_RDS_THRESHOLD);
}

FMC_STATIC void HandleRdsSetClearFlag(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Read the flag to clear status */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
}

FMC_STATIC void HandleRdsSetEnableInt(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Update global parameter int_mask to enable also RDS interrupt */
    _fmRxSmData.interruptInfo.gen_int_mask |= FMC_FW_MASK_RDS;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Enable the interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.gen_int_mask);
}

FMC_STATIC void HandleRdsSetFinish(void)
{
    _fmRxSmData.rdsOn = FMC_TRUE;
    
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value = ((FmRxRdsSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRdsUnSetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_POWER_SET_GET, FMC_FW_RX_POWER_SET_FM_ON_RDS_OFF);
}


FMC_STATIC void HandleRdsOffFinish(void)
{
    /* If RDS is turned off - finished. Send event and end operation */
    
    _fmRxSmData.rdsOn = FMC_FALSE;
    
    _FM_RX_SM_ResetRdsData();
    _fmRxSmData.interruptInfo.gen_int_mask &= ~(FMC_FW_MASK_RDS);
    _fmRxSmData.context.appEvent.p.cmdDone.value = ((FmRxRdsSetCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}

/*******************************************************************************************************************
 * HANDLE TUNE FUNCTIONS
 *******************************************************************************************************************/
FMC_STATIC void HandleTuneSetPower(void)
{   
        /* Update to next handler */
        prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);
        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
FMC_STATIC void HandleTuneSetFreq(void)
{   
    FMC_U32 freq = ((FmRxTuneCmd *)_fmRxSmData.currCmdInfo.baseCmd)->freq;
    FMC_U16 index = 0;
    FMC_U8 status = FM_RX_STATUS_SUCCESS;

    if(_fmRxSmData.band== FMC_BAND_EUROPE_US)
    {
        if((freq < FMC_FIRST_FREQ_US_EUROPE_KHZ) || (freq > FMC_LAST_FREQ_US_EUROPE_KHZ))
            status = FM_RX_STATUS_INVALID_PARM;
    }
    /* Japan band */
    else
    {
        if((freq < FMC_FIRST_FREQ_JAPAN_KHZ) || (freq > FMC_LAST_FREQ_JAPAN_KHZ))
            status = FM_RX_STATUS_INVALID_PARM;
    }

    if(status == FM_RX_STATUS_INVALID_PARM)
    {
        _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.tunedFreq;

        _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
    }
    else
    {
        /*[ToDo Zvi] the first channel should depend on the band*/
        index = FMC_UTILS_FreqToFwChannelIndex(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,freq);
        /* Update to next handler */
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

        /* Send Set_Frequency command */    
        FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET, index);
    }


}

FMC_STATIC void HandleTuneClearFlag(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Read the flag to clear status */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
}

FMC_STATIC void HandleTuneEnableInterrupts(void)
{   
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /*Eliminate a race condition that a general interrupt received right before 
      enabling the new specific interrupts. don't update the stage so we enter 
      the same function again after clearing*/
    if(_fmRxSmData.interruptInfo.interruptInd)
    {
        _fmRxSmData.interruptInfo.interruptInd = FMC_FALSE;
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 3);/*[ToDo Zvi]*/
        /* Read the flag to clear status*/
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
    }
    else
    {
    _fmRxSmData.interruptInfo.opHandler_int_mask = FMC_FW_MASK_FR;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.opHandler_int_mask;
    /* Enable FR interrupt */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.opHandler_int_mask);
    
}
}

FMC_STATIC void HandleTuneStartTuning(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Start tuning */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET, FMC_FW_RX_TUNER_MODE_PRESET_MODE);
}

FMC_STATIC void HandleTuneWaitStartTuneCmdComplete(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);

    FMC_ASSERT(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT); 

/*  FMC_LOG_INFO("FMFMFM expected 1 - got %d", FMC(callReason));*/
    /* Got command complete - Just wait for interrupt */
}

FMC_STATIC void HandleTuneFinishedReadFreq(void)
{   
    FMC_ASSERT(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_INTERRUPT); 
/*  FMC_LOG_INFO("FMFMFM expected 2 - got %d", FMC(callReason));*/

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Verify we got an interrupt - the tuning is finished */
    FMC_ASSERT(_fmRxSmData.interruptInfo.opHandlerIntSetBits & FMC_FW_MASK_FR);

    /* Read frequency to notify the application */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}

FMC_STATIC void HandleTuneFinishedEnableDefaultInts(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    _fmRxSmData.interruptInfo.opHandler_int_mask = 0;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Disable FR and enable the default interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.gen_int_mask);

}
FMC_STATIC void HandleTuneFinish(void)
{   
    FMC_U32 freq;

    freq = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);

    _fmRxSmData.tunedFreq = freq;
    
    _FM_RX_SM_ResetStationParams(freq);

    _fmRxSmData.context.appEvent.p.cmdDone.value = freq;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleIsValidChannelStart(void)
{
   /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_CHANNEL,2);    
}

FMC_STATIC void HandleIsValidChannelFinish(void)
{

     /*If the channel is valid set value to 1 else set value to 0*/
    if (_fmRxSmData.context.transportEventData.read_param&FM_IS_VALID_CHANNEL_BIT) 
            _fmRxSmData.context.appEvent.p.cmdDone.value=1;
    else
            _fmRxSmData.context.appEvent.p.cmdDone.value=0;

    /* Send event to the application */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);

}

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleRssiGetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_RSSI_LEVEL_GET,2);

}
FMC_STATIC void HandleRssiGetFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.context.transportEventData.read_param;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSeekMain(void)
{
    FMC_STATIC FmRxOpHandlerArray handlerArray;

    /* If we start the command update the handler and stage */
    if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_START) 
    {
        _fmRxSmData.seekOp.curStage = 0; 
        handlerArray = seekHandler;
    }
    /* If an upper event was received update the handler to stop seek */
    else if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_UPPER_EVT) 
    {
            /* The seek is already finished - just finish the operation
                Call the next seek stage with cmd complete because upper
                event always wait for the cmd complete event */
            if((_fmRxSmData.seekOp.curStage) > 7) 
            {
                _fmRxSmData.callReason = FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT; 
            }
                else
                {
                    _fmRxSmData.seekOp.seekStageBeforeStop = _fmRxSmData.seekOp.curStage; 
                _fmRxSmData.seekOp.curStage = 0; 
                handlerArray = stopSeekHandler;
            }
             }

    /* Call the handler and update to next stage */
    handlerArray[_fmRxSmData.seekOp.curStage](); 
    _fmRxSmData.seekOp.curStage++; 
}

FMC_STATIC void HandleSeekStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /* Read frequency to know which frequency to start at */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}
FMC_STATIC void HandleSeekSetFreq(void)
{
    FMC_U16 index;
    FMC_U16 new_index;

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    index = _fmRxSmData.context.transportEventData.read_param;
    
    new_index = _FM_RX_UTILS_findNextIndex(_fmRxSmData.band,((FmRxSeekCmd *)_fmRxSmData.currCmdInfo.baseCmd)->dir, index);
    
    /* Send Set_Frequency command */    
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET, new_index);
}

FMC_STATIC void HandleSeekSetDir(void)
{

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
    
    /* Send Set_Frequency command */    
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_SEARCH_DIR_SET_GET, (FMC_U16) ((FmRxSeekCmd *)_fmRxSmData.currCmdInfo.baseCmd)->dir);
}
FMC_STATIC void HandleSeekClearFlag(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
    /* Read the flag to clear status */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
}

FMC_STATIC void HandleSeekEnableInterrupts(void)
{   
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /*Eliminate a race condition that a general interrupt received right before 
    enabling the new specific interrupts. don't update the stage so we enter 
    the same function again after clearing */
    
    if(_fmRxSmData.interruptInfo.interruptInd)
    {
        _fmRxSmData.interruptInfo.interruptInd = FMC_FALSE;
        _fmRxSmData.seekOp.curStage--; 
        /* Read the flag to clear status */
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
    }
    else
    {
    _fmRxSmData.interruptInfo.opHandler_int_mask = FMC_FW_MASK_FR | FMC_FW_MASK_BL;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.opHandler_int_mask;
    /* Enable FR and BL interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.opHandler_int_mask);
}
}

FMC_STATIC void HandleSeekStartTuning(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /* Start tuning */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET,  FMC_FW_RX_TUNER_MODE_AUTO_SEARCH_MODE);
}

FMC_STATIC void HandleSeekWaitStartTuneCmdComplete(void)
{   
    
    FMC_ASSERT(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT); 
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, 0);

}


FMC_STATIC void HandleStopSeekStart(void)
{
    /* If the seek didn't start yet, no need to stop.
       Just go to the last stages in order to finish properly */
    if(_fmRxSmData.seekOp.seekStageBeforeStop <= 5) 
    {
        _fmRxSmData.seekOp.status = FM_RX_STATUS_SEEK_STOPPED; 
        _fmRxSmData.seekOp.curStage = 5; 
        stopSeekHandler[_fmRxSmData.seekOp.curStage]();  
    }
    else
    {
        /* Update to next handler */
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        /* Stop the seek */
        FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET,  FMC_FW_RX_TUNER_MODE_STOP_SEARCH);
    }
}

FMC_STATIC void HandleStopSeekWaitCmdCompleteOrInt(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, 0);

    /* If we have received cmd complete event first then the stop seek command
       was sent. Now wait for the interrupt to be received */
    if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT) 
    {
        _fmRxSmData.seekOp.status = FM_RX_STATUS_STOP_SEEK; 
    }
    /* If we got an interrupt although we expected a cmd complete event
       then the interrupt was received on the seek operation and not on
       the stop seek.
       Still we will have to wait for the cmd complete event */
    else if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_INTERRUPT) 
    {
        FMC_ASSERT( _fmRxSmData.interruptInfo.opHandlerIntSetBits & (FMC_FW_MASK_FR|FMC_FW_MASK_BL));
     /*We got here due to a racing condition,Seek operation
       Ended at the point that we requested to Stop the seek
       Operation,We will miss the Command complete.
       This is the reason we need to signal the SM to keep
       The operation going if we reach this point */
        _fmRxSmData.seekOp.status = FM_RX_STATUS_STOP_SEEK;         
        FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
    }
}

/* Got interrupt either from seek or stop seek operation */
FMC_STATIC void HandleSeekStopSeekFinishedReadFreq(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
    
    /* We could have got a cmd complete in a case of stop seek
       when we first got interrupt and now we get the cmd complete
       for the stop seek. */
    if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_INTERRUPT) 
    {
        FMC_ASSERT( _fmRxSmData.interruptInfo.opHandlerIntSetBits & (FMC_FW_MASK_FR|FMC_FW_MASK_BL));

        if(_fmRxSmData.interruptInfo.opHandlerIntSetBits & FMC_FW_MASK_BL)
        {
            _fmRxSmData.seekOp.status = FM_RX_STATUS_SEEK_REACHED_BAND_LIMIT; 
        }
        else
        {
            if(_fmRxSmData.seekOp.status == FM_RX_STATUS_STOP_SEEK) 
            {
                _fmRxSmData.seekOp.status = FM_RX_STATUS_SEEK_STOPPED; 
            }
            else
            {
                _fmRxSmData.seekOp.status = FM_RX_STATUS_SUCCESS; 
            }
        }
    }

    /* Read frequency to notify the application */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}


/********************************************START - WORKAROUND*************************************************/

/*todo ram - This is a workaround due to a FW defect we must Set the current  frequency again at the end of the seek sequence 
            THIS NEED TO BE REMOVED FOR PG 2.0 */
FMC_STATIC void HandleSeekStopSeekFinishedSetFreq(void)
{   

    FMC_U16 index;

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /*Get the current Freq from the evnt */ 
    index = _fmRxSmData.context.transportEventData.read_param;
    
    /* Send Set_Frequency command  with the current frequency*/    
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET, index);    
}

/********************************************END - WORKAROUND*************************************************/

FMC_STATIC void HandleSeekStopSeekFinishedEnableDefaultInts(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    _fmRxSmData.interruptInfo.opHandler_int_mask = 0;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Disable FR and enable the default interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.gen_int_mask);

}
FMC_STATIC void HandleSeekStopSeekFinish(void)
{   
    FMC_U32 freq;

    freq = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);
    _FM_RX_SM_ResetStationParams(freq);

    _fmRxSmData.context.appEvent.p.cmdDone.value = freq;
    _fmRxSmData.currCmdInfo.status = _fmRxSmData.seekOp.status; 

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}

FMC_STATIC void HandleStopSeekBeforeSeekStarted(void)
{
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.tunedFreq;
    _fmRxSmData.currCmdInfo.status = _fmRxSmData.seekOp.status; 
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleSetAFStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    
    _fmRxSmData.afMode = ((FmRxSetAFCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    if(_fmRxSmData.afMode == FM_RX_RDS_AF_SWITCH_MODE_ON)
    {
        /* Update global parameter gen_int_mask to enable also low rssi interrupt */
        _fmRxSmData.interruptInfo.gen_int_mask |= FMC_FW_MASK_LEV;
        _fmRxSmData.afMode = FM_RX_RDS_AF_SWITCH_MODE_ON;
    }
    else
    {
        /* Cancel inactivity timer started at the beginning of AF jumps */
        FMCI_OS_CancelTimer();
        /* Update global parameter gen_int_mask to disable low rssi interrupt */
        _fmRxSmData.interruptInfo.gen_int_mask &= ~(FMC_FW_MASK_LEV);
        _fmRxSmData.afMode = FM_RX_RDS_AF_SWITCH_MODE_OFF;
    }
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Enable the interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.gen_int_mask);
}
FMC_STATIC void HandleSetAfFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value = ((FmRxSetAFCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC void HandleSetStereoBlendStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
        
    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_MOST_BLEND_SET_GET, (FMC_U16) ((FmRxSetStereoBlendCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode);
}
FMC_STATIC void HandleSetStereoBlendFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetStereoBlendCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetDeemphasisModeStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_DEMPH_MODE_SET_GET, (FMC_U16) ((FmRxSetDeemphasisModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode);
}
FMC_STATIC void HandleSetDeemphasisModeFinish(void)
{
    _fmRxSmData.deemphasisFilter = ((FmRxSetDeemphasisModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetDeemphasisModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetRssiSearchLevelStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_SEARCH_LVL_SET_GET, (FMC_U16)((FmRxSetRssiSearchLevelCmd*)_fmRxSmData.currCmdInfo.baseCmd)->rssi_level);
}
FMC_STATIC void HandleSetRssiSearchLevelFinish(void)
{
    _fmRxSmData.rssiThreshold = ((FmRxSetRssiSearchLevelCmd*)_fmRxSmData.currCmdInfo.baseCmd)->rssi_level;

    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetRssiSearchLevelCmd*)_fmRxSmData.currCmdInfo.baseCmd)->rssi_level;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
#if 0 /* warning removal - unused code (yet) */
FMC_STATIC void HandleSetPauseLevelStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_PAUSE_LVL_SET_GET, (FMC_U16)((FmRxSetPauseLevelCmd*)_fmRxSmData.currCmdInfo.baseCmd)->pause_level);
}
FMC_STATIC void HandleSetPauseLevelFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetPauseLevelCmd*)_fmRxSmData.currCmdInfo.baseCmd)->pause_level;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetPauseDurationStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_PAUSE_DUR_SET_GET,(FMC_U16) ((FmRxSetPauseDurationCmd*)_fmRxSmData.currCmdInfo.baseCmd)->pause_duration);
}
FMC_STATIC void HandleSetPauseDurationFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetPauseDurationCmd*)_fmRxSmData.currCmdInfo.baseCmd)->pause_duration;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetRdsRbdsStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_SYSTEM_SET_GET, (FMC_U16)((FmRxSetRdsRbdsModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode);
}

FMC_STATIC void HandleSetRdsRbdsFinish(void)
{
    _fmRxSmData.rdsRdbsSystem =  ((FmRxSetRdsRbdsModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =((FmRxSetRdsRbdsModeCmd*)_fmRxSmData.currCmdInfo.baseCmd)->mode;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleMoStGetStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_MOST_MODE_SET_GET,2);

}
FMC_STATIC void HandleMoStGetFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.context.transportEventData.read_param;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}


/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleSetChannelSpacingStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_MoSt command */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_CHANNEL_SPACING_SET_GET,(FMC_U16)((FmRxSpacingSetCommand *)_fmRxSmData.currCmdInfo.baseCmd)->channelSpacing);
}
FMC_STATIC void HandleSetChannelSpacingFinish(void)
{
	_fmRxSmData.channelSpacing = (FMC_UINT)((FmRxSpacingSetCommand *)_fmRxSmData.currCmdInfo.baseCmd)->channelSpacing;
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.channelSpacing;

    /* Send event to the applicatoin */
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleGetChannelSpacingStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_CHANNEL_SPACING_SET_GET,2);

}
FMC_STATIC void HandleGetChannelSpacingFinish(void)
{
    /* Send event to the applicatoin */
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.context.transportEventData.read_param;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleGetFwVersionStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FIRM_VER_GET,2);

}
FMC_STATIC void HandleGetFwVersionFinish(void)
{
    /* Send event to the application */
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.context.transportEventData.read_param;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/


FMC_STATIC void HandleCompleteScanMain(void)
{

    FMC_STATIC FmRxOpHandlerArray handlerArray;

    /* If we start the command update the handler and stage */
    if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_START) 
    {
        _fmRxSmData.completeScanOp.stopCompleteScanStatus = _STOP_COMPLETE_IDLE;
        _fmRxSmData.completeScanOp.isCompleteScanProgressFinish = FMC_FALSE;
        _fmRxSmData.completeScanOp.curStage = 0; 
        handlerArray = completeScanHandler;
    }
  /* If an upper event was received update the handler to stop seek */
    else if(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_UPPER_EVT) 
    {
      /*If we got Get progress in the middle of the operation change the handler to the correct one*/ 
      if( _fmRxSmData.upperEvent==FM_RX_SM_UPPER_EVENT_GET_COMPLETE_SCAN_PROGRESS)
        {         
            _fmRxSmData.completeScanOp.curStage = 0; 
            handlerArray = getCompleteScanProgressHandler;
        }

		/*If we got Stop Complete Scan in the middle of the operation mark it for further handling */ 
      else if( _fmRxSmData.upperEvent==FM_RX_SM_UPPER_EVENT_STOP_COMPLETE_SCAN)
        {           					
			_fmRxSmData.completeScanOp.stopCompleteScanStatus = _STOP_COMPLETE_REQUEST;
    }
    }
    /*If getting the progress has ended go back to Complete Scan handel to the correct stage
      The stage before getting the event */
    else if((_fmRxSmData.completeScanOp.isCompleteScanProgressFinish == FMC_TRUE)&&
                (_fmRxSmData.completeScanOp.stopCompleteScanStatus == _STOP_COMPLETE_IDLE))
                {
                        _fmRxSmData.completeScanOp.isCompleteScanProgressFinish = FMC_FALSE;
                        _fmRxSmData.completeScanOp.curStage = 4; 
                         handlerArray = completeScanHandler;
                  }  

	/* If stopCompleteScan() was called and we are in the right stage, switch to the stop handler */
	if (( _fmRxSmData.completeScanOp.stopCompleteScanStatus == _STOP_COMPLETE_REQUEST ) &&
		( _fmRxSmData.completeScanOp.curStage >= COMPLETE_SCAN_FIRST_STAGE_TO_STOP_SCAN))
	{
		/* Assertion in this case means that the CMD_COMPLETE for stopCompleteScan is never called */
        FMC_ASSERT(_fmRxSmData.completeScanOp.curStage <= COMPLETE_SCAN_LAST_STAGE_TO_STOP_SCAN);

		_fmRxSmData.completeScanOp.stopCompleteScanStatus = _STOP_COMPLETE_STARTED;
		_fmRxSmData.completeScanOp.curStage = 0; 
		handlerArray = stopCompleteScanHandler;
	}

    /* Call the handler and update to next stage */      
    handlerArray[_fmRxSmData.completeScanOp.curStage](); 
    _fmRxSmData.completeScanOp.curStage++; 
}

FMC_STATIC void HandleCompleteScanStart(void)
{   
     /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
    /* Read the flag to clear status */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2); 
}

FMC_STATIC void HandleCompleteScanEnableInterrupts(void)
{   
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /*Eliminate a race condition that a general interrupt received right before 
    enabling the new specific interrupts. don't update the stage so we enter 
    the same function again after clearing */
    
    if(_fmRxSmData.interruptInfo.interruptInd)
    {
        _fmRxSmData.interruptInfo.interruptInd = FMC_FALSE;
        _fmRxSmData.seekOp.curStage--; 
        /* Read the flag to clear status */
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
    }
    else
    {
    _fmRxSmData.interruptInfo.opHandler_int_mask = FMC_FW_MASK_FR;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.opHandler_int_mask;
    /* Enable FR interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.opHandler_int_mask);
}
}

FMC_STATIC void HandleCompleteScanStartTuning(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /* Start tuning */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET,  FMC_FW_RX_TUNER_MODE_COMPLETE_SCAN);
}

FMC_STATIC void HandleCompleteScanWaitStartTuneCmdComplete(void)
{   
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, 0);
}

FMC_STATIC void HandleCompleteScanReadChannels(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
    
    /*Verif that we got here by getting the correct interupt */   
    FMC_ASSERT( _fmRxSmData.interruptInfo.opHandlerIntSetBits & (FMC_FW_MASK_FR));       

   /* TODO RAM - Need to add the same machanisem when getting a stop command */
    
    /* Read the flag to clear status */
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_CHANNEL,2);
}


FMC_STATIC void HandleCompleteScanChannels(void)
{
    /* Get Number of channels on bits 0-6 only */
    _fmRxSmData.context.appEvent.p.completeScanData.numOfChannels =(FMC_U8) (_fmRxSmData.context.transportEventData.read_param&0x7f);
    
    /*If we got some channels we need to read the data  */
    if(_fmRxSmData.context.appEvent.p.completeScanData.numOfChannels>0)
        {
               /* Update to next handler */  
              prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);
             FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_RDS_DATA_GET,(FMC_U16)(_fmRxSmData.context.appEvent.p.completeScanData.numOfChannels*2) );
        }
    else
        {
            /*Set the Number of channels to zero */
            _fmRxSmData.context.appEvent.p.completeScanData.numOfChannels=0;

            /*We are calling to the next function to enable the deafult Interupts
              Since we are not sending any command to the chip now
              We update the Current stage beacuse the HandleCompleteScanMain 
              Calls the function and when we return it will update the Curent stage
 */            
          HandleCompleteScanFinishEnableDeafultInts();
          _fmRxSmData.completeScanOp.curStage++;
        }
    
}


FMC_STATIC void HandleCompleteScanFinishEnableDeafultInts(void)
{
     FMC_U8 len = (FMC_U8)(_fmRxSmData.context.transportEventData.dataLen/2);
     FMC_U8 *data = _fmRxSmData.context.transportEventData.data;
     FMC_U16 index = 0;
     FMC_U8 i;

      /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    /*Clear The struct */    
    FMC_OS_MemSet(_fmRxSmData.context.appEvent.p.completeScanData.channelsData,0,FM_RX_COMPLETE_SCAN_MAX_NUM); 

    /*Read the Data only if we found any channel */
 if(_fmRxSmData.context.appEvent.p.completeScanData.numOfChannels>0)
   {   
        for(i=0;i<len;i++)
        {
            /*The data is in BE need to take the LSB and put it first to convert it to Host16 */

            /*Get the LSB and shift it left*/
            index=(FMC_U16)((data[1])<<8);             

            /*Add the MSB to the end */
            index=(FMC_U16)(index+(data[0]));

            /*Convert the Index to the appropriate frequency */
            _fmRxSmData.context.appEvent.p.completeScanData.channelsData[i] = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,index);       
        
            /*Set the pointer to the next Data */
            data=data+2;        
        }
    }

    _fmRxSmData.interruptInfo.opHandler_int_mask = 0;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Disable FR and enable the default interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.gen_int_mask);
}

FMC_STATIC void HandleCompleteScanFinish(void)
{        
    _fmRxSmData.currCmdInfo.status=FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_COMPLETE_SCAN_DONE);
}



/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleCompleteScanProgressStart(void)
{

        _fmRxSmData.completeScanOp.isCompleteScanProgressFinish = FMC_FALSE;

        /* Update to next handler */
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        /* Read frequency to notify the application */
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);  

}

FMC_STATIC void HandleCompleteScanProgressEnd(void)
{         
     FmRxStatus  status;

     status = _fmRxSmData.currCmdInfo.status;
        _fmRxSmData.context.appEvent.p.cmdDone.value = 
     FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);


    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, status, FM_RX_CMD_COMPLETE_SCAN_PROGRESS, FM_RX_EVENT_CMD_DONE);
    _fmRxSmData.completeScanOp.isCompleteScanProgressFinish = FMC_TRUE;
}

/******************************************************************************************************************
 *                  
 *******************************************************************************************************************/


FMC_STATIC void HandleStopCompleteScanStart(void)
{
 
        /* Update to next handler */
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        /* Stop the seek */
        FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET,  FMC_FW_RX_TUNER_MODE_STOP_SEARCH);    
}

FMC_STATIC void HandleStopCompleteScanWaitCmdCompleteOrInt(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, 0);

    /*We Should get Both Command Complete and an Event that will transfer us to the next handler
       Without sending any commands  */     
}

/* Got interrupt either from seek or stop seek operation */
FMC_STATIC void HandleCompleteScanStopCompleteScanFinishedReadFreq(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);                    
     
    /* Read frequency to notify the application */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}

FMC_STATIC void HandleCompleteScanStopCompleteScanFinishedEnableDefaultInts(void)
{   
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    _fmRxSmData.interruptInfo.opHandler_int_mask = 0;
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* Disable FR and enable the default interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,  _fmRxSmData.interruptInfo.gen_int_mask);

}

FMC_STATIC void HandleSeekStopSeekFinishReadFreq(void)
{   
    FMC_U32 freq;

    freq = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);
    _FM_RX_SM_ResetStationParams(freq);

    _fmRxSmData.context.appEvent.p.cmdDone.value = freq;
    _fmRxSmData.currCmdInfo.status =FM_RX_STATUS_COMPLETE_SCAN_STOPPED ; 

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}



/******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
/*
*   Start VAC operation - gets all params needed - sample freq, channel num.
*
*/
ECCM_VAC_Status StartAudioOperation(ECAL_Operation operation)
{
#if obc
    TCAL_DigitalConfig ptConfig;

    /* get the digital configuration params*/
    ptConfig.eSampleFreq = _fmRxSmData.vacParams.eSampleFreq;
    ptConfig.uChannelNumber = _getRxChannelNumber();
    
    /*  remember the starting operation. this is needed if recieving unavailible resources we need to 
    *   forward the app the operation, which can be ither the fm rx op or the fm_o_sco/a3dp 
    */
    _fmRxSmData.vacParams.lastOperationStarted = operation;
    
    return CCM_VAC_StartOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                        operation,
                        &ptConfig,
                        &_fmRxSmData.vacParams.ptUnavailResources);
#endif
    return CCM_VAC_STATUS_SUCCESS;
}
FMC_STATIC void HandleEnableAudioRoutingStart(void)
{
#if obc
    /* if rx operation needed (rx and not fm_o_sco of fm_o_a3dp)*/
    if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_VAC_RX_OPERATION_MASK)
    {
        /* start vac opration and move to next stage sync or async*/
        _handleVacOpStateAndMove2NextStage( StartAudioOperation(CAL_OPERATION_FM_RX));
    }
    else
    {   /* if fm rx operation not needed simulate success in vac call to move to next stage*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
    }
#endif
}
FMC_STATIC void HandleEnableAudioRoutingVacOperationStarted(void)
{
#if obc
    TCAL_ResourceProperties pProperties;
    /* check that the vac completed succesfully*/
    if( _fmRxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {   /* set the properties so the cal will configure properly*/
        _getResourceProperties(_fmRxSmData.vacParams.audioTargetsMask,&pProperties);
        CCM_VAC_SetResourceProperties(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                        CAL_RESOURCE_FMIF,  
                                        &pProperties);
        if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_SCO)
        {   /* start the sco operation*/
            _handleVacOpStateAndMove2NextStage( StartAudioOperation(CAL_OPERATION_FM_RX_OVER_SCO));
        }
        else if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)
        {   /* start the fm over a3dp operation*/
            _handleVacOpStateAndMove2NextStage( StartAudioOperation(CAL_OPERATION_FM_RX_OVER_A3DP));
        }
        else
        {   /* no fm o bt - sinulate success and move to next stage*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
        }
    }
    else 
    {   /* error accured save status and finish operation*/
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
        _goToLastStageAndFinishOp();
    }
#endif
}
FMC_STATIC void HandleEnableAudioRoutingEnableAudioTargets( void)
{
#if obc
    if( _fmRxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {       /* enable the audio acording to audio configurations*/
            prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
            FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET, _getRxAudioEnableParam());
    }
    else
    {   /* error accured save status and finish operation*/
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
        _goToLastStageAndFinishOp();
    }
#endif
}

FMC_STATIC void HandleEnableAudioRoutingFinish(void)
{
#if obc
    if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_SUCCESS)
    {   /* audio routing started successfully - flag that audio is on*/
        _fmRxSmData.vacParams.isAudioRoutingEnabled= FMC_TRUE;
    }
    else if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)
    {   /* the operation failed since vac could not start the operation - report to the app*/
        _fmRxSmData.vacParams.isAudioRoutingEnabled= FMC_FALSE;
        _fmRxSmData.context.appEvent.p.audioCmdDone.operation = _fmRxSmData.vacParams.lastOperationStarted;
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = &_fmRxSmData.vacParams.ptUnavailResources;
    }
    else
    {
        /* unknown internal error*/
        _fmRxSmData.vacParams.isAudioRoutingEnabled= FMC_FALSE;
        
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = NULL;
    }
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
#endif obc
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleDisableAudioRoutingStart(void)
{
#if obc
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET, 0);
#endif
}
FMC_STATIC void HandleDisableAudioRoutingStopVacOperation(void)
{   
#if obc
    if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_VAC_RX_OPERATION_MASK)
    {   /* if rx vac operation stated stop it*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX));
    }
    else
    {   /* else simulate success and move to next stage*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
    }
#endif
}
FMC_STATIC void HandleDisableAudioRoutingStopVacFmOBtOperation(void)
{
#if obc
    if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_SCO)
    {   /* if fm o sco vac operation stated stop it*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_SCO));
    }
    else if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)
    {/* if fm o a3dp vac operation stated stop it*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_A3DP));
    }
    else
    {/* else simulate success and move to next stage*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
    }
#endif
}
FMC_STATIC void HandleDisableAudioRoutingFinish(void)
{
#if obc
    _fmRxSmData.vacParams.isAudioRoutingEnabled= FMC_FALSE;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
#endif
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
ECCM_VAC_Status SendChangeAudioTargets(ECAL_Operation operation)
{
#if obc
    TCAL_DigitalConfig ptConfig;
    TCAL_ResourceProperties pProperties;
    ECAL_ResourceMask targetsMask;
    FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;

    /* get the digital configuration*/
    ptConfig.eSampleFreq = setAudioTargetsCmd->eSampleFreq;
    ptConfig.uChannelNumber = _getRxChannelNumber();

    /* get the target converted to cal values and fill the properties*/
    targetsMask = _convertRxTargetsToCal(setAudioTargetsCmd->rxTargetMask,&pProperties);

    /* set properties of FM  IF resources so that i2s pcm will be configured acording to the selected target */
    CCM_VAC_SetResourceProperties(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                    CAL_RESOURCE_FMIF,  
                                    &pProperties);
    /* change the resource*/
    return CCM_VAC_ChangeResource(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                        operation,
                                        targetsMask,
                                        &ptConfig,
                                        &_fmRxSmData.vacParams.ptUnavailResources);
    
#endif
	return CCM_VAC_STATUS_SUCCESS;
}

FMC_STATIC void HandleChangeAudioTargetsStart(void)
{
#if obc
    FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;
    
    if(setAudioTargetsCmd->rxTargetMask&FM_RX_VAC_RX_OPERATION_MASK)
    {
        /* rx vac operation target was requested - get the targets and change the resources of rx op according*/
        _handleVacOpStateAndMove2NextStage( SendChangeAudioTargets(CAL_OPERATION_FM_RX));
    }
    else
    {
        if((_fmRxSmData.vacParams.isAudioRoutingEnabled)&&
            (_fmRxSmData.vacParams.audioTargetsMask&FM_RX_VAC_RX_OPERATION_MASK))
        {
            /* If RX VAC operation was already started stop it to release unrequested resources*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX));
        }
        else
        {   /* RX Vac operation (analog/pcm/i2s) was not requested or audio is not enabled don't do anything with vac rx op*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
        }
    }
#endif
}
FMC_STATIC void HandleChangeAudioTargetsSendChangeResourceCmdComplete( void)
{
#if obc
    FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;
    TCAL_ResourceProperties pProperties;
    if( _fmRxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {   /* if audio was enabled check if any fm o bt operation should be started*/
        if(_fmRxSmData.vacParams.isAudioRoutingEnabled)
        {   /* set the properaties so that fm o bt will configure the pcm (ans not i2s)*/
            _getResourceProperties(setAudioTargetsCmd->rxTargetMask,&pProperties);
            CCM_VAC_SetResourceProperties(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                            CAL_RESOURCE_FMIF,  
                                            &pProperties);
            
            if(setAudioTargetsCmd->rxTargetMask&FM_RX_TARGET_MASK_FM_OVER_SCO)
            {   /* fm o sco target requested - start the vac operation*/    
                _handleVacOpStateAndMove2NextStage( StartAudioOperation(CAL_OPERATION_FM_RX_OVER_SCO));
            }
            else if(setAudioTargetsCmd->rxTargetMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)
            {   /* fm o a3dp target requested - start the vac operation*/   
                _handleVacOpStateAndMove2NextStage( StartAudioOperation(CAL_OPERATION_FM_RX_OVER_A3DP));
            }
            else 
            {   /* simulate success in vac operation and move to next stage*/
                _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
            }
        }
        else
        {
            /*Audio is not enabled no need to start the vac fm over .. operation - just update the sm target mask*/
            _goToLastStageAndFinishOp();
        }
    }
    else
    {
        /* internal error accured - probably in the transport layer*/
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
        _goToLastStageAndFinishOp();
    }
#endif
}
FMC_STATIC void HandleChangeAudioTargetsStopFmOverBtIfNeeded( void)
{
#if obc
    FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;

    /*If we change the target from fm o bt to digital or analog - theen the fm o bt operation should be stoped. */
    if((_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_SCO)&&
        !(setAudioTargetsCmd->rxTargetMask&FM_RX_TARGET_MASK_FM_OVER_SCO))
    {
        /* audio enabled, fm o sco was active and now was disabled*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_SCO));
    }
    else if((_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)&&
        !(setAudioTargetsCmd->rxTargetMask&FM_RX_TARGET_MASK_FM_OVER_A3DP))
    {
        /* audio enabled, fm o a3dp was active and now was disabled*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_RX_OVER_A3DP));
    }
    else
    {
        /* fm o bt was not started (of it was started and still needs to stay active)*/
        _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
    }
#endif
}   

FMC_STATIC void HandleChangeAudioTargetsEnableAudioTargets( void)
{
#if obc
    if( _fmRxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {
        FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;
        /* if we reached this place the vac operation ended successfully - we can update the sm cach*/  
        _fmRxSmData.vacParams.eSampleFreq = setAudioTargetsCmd->eSampleFreq;
        _fmRxSmData.vacParams.audioTargetsMask = setAudioTargetsCmd->rxTargetMask;
        /* if audio audio routing enabled  - enable analog digital according to the targets*/
        if(_fmRxSmData.vacParams.isAudioRoutingEnabled)
        {
            prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
            FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET, _getRxAudioEnableParam());
        }
        else
        {
            _goToLastStageAndFinishOp();
        }
    }
    else
    {
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
        _goToLastStageAndFinishOp();
    }
#endif
}

FMC_STATIC void HandleChangeAudioTargetsFinish(void)
{
#if obc
    FmRxSetAudioTargetCmd *setAudioTargetsCmd = (FmRxSetAudioTargetCmd*)_fmRxSmData.currCmdInfo.baseCmd;
    if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)
    {   /* save the vac operation that failed and the resources and send to the app */
        _fmRxSmData.context.appEvent.p.audioCmdDone.operation = _fmRxSmData.vacParams.lastOperationStarted;
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = &_fmRxSmData.vacParams.ptUnavailResources;
    }
    else if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_SUCCESS)
    {   /* update the cach*/
        _fmRxSmData.vacParams.eSampleFreq = setAudioTargetsCmd->eSampleFreq;
        _fmRxSmData.vacParams.audioTargetsMask = setAudioTargetsCmd->rxTargetMask;
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = NULL;
    }
    else
    {   /* don't update the cach if the command failed*/
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = NULL;
    }
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
#endif
}
/********************************************************************************************************/
/********************************************************************************************************/
/********************************************************************************************************/

FMC_STATIC void HandleChangeDigitalAudioConfigSendChangeConfigurationCmdStart( void)
{
#if obc
    TCAL_DigitalConfig ptConfig;
    
    FmRxSetDigitalAudioConfigurationCmd *setAudioConfigurationCmd = (FmRxSetDigitalAudioConfigurationCmd*)_fmRxSmData.currCmdInfo.baseCmd;
    /* get the digital configuration*/
    ptConfig.eSampleFreq = setAudioConfigurationCmd->eSampleFreq;
    ptConfig.uChannelNumber = _getRxChannelNumber();
    /* change the digital configuration*/
    if(_fmRxSmData.vacParams.isAudioRoutingEnabled)
    {   /* simulate success - and move to next stage which is to enable the audio*/
        if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_SCO)
        {
            /* audio enabled, fm o sco was active and now was disabled*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_ChangeConfiguration(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                              CAL_OPERATION_FM_RX_OVER_SCO,
                                              &ptConfig));
        }
        else if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)
        {
            /* audio enabled, fm o a3dp was active and now was disabled*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_ChangeConfiguration(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                                CAL_OPERATION_FM_RX_OVER_A3DP,
                                                &ptConfig));
        }
        else if(_fmRxSmData.vacParams.audioTargetsMask&(FM_RX_TARGET_MASK_PCMH|FM_RX_TARGET_MASK_I2SH))
        {
            /* fm o bt was not started (of it was started and still needs to stay active)*/
    _handleVacOpStateAndMove2NextStage( CCM_VAC_ChangeConfiguration(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                CAL_OPERATION_FM_RX,
                                &ptConfig));
}
        else
        {
            _goToLastStageAndFinishOp();
        }
    }
    else
    {   /* audio routing was not enabled so no need to enable audio*/
        _goToLastStageAndFinishOp();
    }
#endif
}
FMC_STATIC void HandleChangeDigitalAudioConfigChangeDigitalAudioConfigCmdComplete( void)
{
#if obc
    /* check that the vac operation ended successfuly*/
    if( _fmRxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {   /* if audio routing enabled*/
        if(_fmRxSmData.vacParams.isAudioRoutingEnabled)
        {   /* simulate success - and move to next stage which is to enable the audio*/
            _handleVacOpStateAndMove2NextStage(CCM_VAC_STATUS_SUCCESS);
        }
        else
        {   /* audio routing was not enabled so no need to enable audio*/
            _goToLastStageAndFinishOp();
        }
    }
    else 
    {
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
        _goToLastStageAndFinishOp();
    }
#endif
}
FMC_STATIC void HandleChangeDigitalAudioConfigEnableAudioSource( void)
{
#if obc
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    /* enable the analog/digital audio*/
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET, _getRxAudioEnableParam());
#endif
}
FMC_STATIC void HandleChangeDigitalAudioConfigComplete( void)
{
#if obc
    FmRxSetDigitalAudioConfigurationCmd *setAudioConfigurationCmd = (FmRxSetDigitalAudioConfigurationCmd*)_fmRxSmData.currCmdInfo.baseCmd;
    if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)
    {   /*forword the operation and the unavailible resources to the app*/
        _fmRxSmData.context.appEvent.p.audioCmdDone.operation = _fmRxSmData.vacParams.lastOperationStarted;
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = &_fmRxSmData.vacParams.ptUnavailResources;
    }
    else if(_fmRxSmData.currCmdInfo.status == FM_RX_STATUS_SUCCESS)
    {/* update the sm cache*/
        _fmRxSmData.vacParams.eSampleFreq = setAudioConfigurationCmd->eSampleFreq;
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = NULL;
    }
    else
    {   /* don't update the sm cach*/
        _fmRxSmData.context.appEvent.p.audioCmdDone.ptUnavailResources = NULL;
    }
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
#endif
}
/*
########################################################################################################################
    Ens VAC related
########################################################################################################################
*/
FMC_STATIC void HandleGenIntMal(void)
{

    if(_fmRxSmData.interruptInfo.genIntSetBits & _fmRxSmData.interruptInfo.gen_int_mask & FMC_FW_MASK_MAL)
    {
        FmHandleMalfunction();
    }
    else
    {
        genIntHandler[GEN_INT_AFTER_MAL_STAGE]();
    }
}
FMC_STATIC void HandleGenIntStereoChange(void)
{
    /* Handle specific interrupts */
    if(_fmRxSmData.interruptInfo.genIntSetBits &_fmRxSmData.interruptInfo.gen_int_mask & FMC_FW_MASK_STIC)
    {
        FmHandleStereoChange();
    }
    else
    {
        genIntHandler[GEN_INT_AFTER_STEREO_CHANGE_STAGE]();
    }
}

FMC_STATIC void HandleGenIntRds(void)
{
    /* Handle specific interrupts */
    if(_fmRxSmData.interruptInfo.genIntSetBits & _fmRxSmData.interruptInfo.gen_int_mask & FMC_FW_MASK_RDS)
    {
        FmHandleRdsRx();
    }
    else
    {
        genIntHandler[GEN_INT_AFTER_RDS_STAGE]();
    }
}

FMC_STATIC void HandleGenIntLowRssi(void)
{
    FMC_BOOL go_to_next = FMC_FALSE;
    FmcStatus status;
    
    /* If low RSSI interrupt occurred  */
    if(_fmRxSmData.interruptInfo.genIntSetBits & _fmRxSmData.interruptInfo.gen_int_mask & FMC_FW_MASK_LEV)
    {
        status = FMCI_OS_ResetTimer(FMC_OS_MS_TO_TICKS(FM_CONFIG_RX_AF_TIMER_MS)) ;                             
        FMC_ASSERT(status == FMC_STATUS_SUCCESS);
            
        /* Update global parameter gen_int_mask to disable low RSSI interrupt -
         * we do not need it during FM_CONFIG_RX_AF_TIMER_MS timeout */
        _fmRxSmData.interruptInfo.gen_int_mask &= ~FMC_FW_MASK_LEV;
        
        if(isAfJumpValid())
        {
            
            send_fm_event_af_jump(FM_RX_EVENT_AF_SWITCH_START,FM_RX_STATUS_AF_IN_PROGRESS, _fmRxSmData.curStationParams.piCode, _fmRxSmData.freqBeforeJump, _fmRxSmData.freqBeforeJump); 
            FmHandleAfJump();
        }
        else
        {
            go_to_next = FMC_TRUE;
        }
    }
    else
    {
        go_to_next = FMC_TRUE;
    }
    
    if(go_to_next)
    {
        genIntHandler[GEN_INT_AFTER_LOW_RSSI_STAGE]();
    }
}

FMC_STATIC void HandleGenIntEnableInt(void)
{
    /* Update operation back to general interrupts */
    _fmRxSmData.currCmdInfo.baseCmd->cmdType = FM_RX_INTERNAL_HANDLE_GEN_INT;
    
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, GEN_INT_AFTER_FINISH_STAGE);
    _fmRxSmData.interruptInfo.fmMask = _fmRxSmData.interruptInfo.gen_int_mask;
    /* After handling all interrupts write to the mask again to enable interrupts (otherwise it is still disabled) */
    /* Must be the last thing done in this operation. otherwise another general interrupt possible in parallel */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,_fmRxSmData.interruptInfo.gen_int_mask);
}

FMC_STATIC void HandleGenIntFinish(void)
{
    

    _FM_RX_SM_RemoveFromQueueAndFreeCmd(&_fmRxSmData.currCmdInfo.baseCmd);

    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);

}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleRdsRx(void)
{
    _fmRxSmData.currCmdInfo.baseCmd->cmdType = FM_RX_INTERNAL_READ_RDS;

    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleAfJump(void)
{
    _fmRxSmData.currCmdInfo.baseCmd->cmdType = FM_RX_INTERNAL_HANDLE_AF_JUMP;
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

    initAfJumpParams();

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleStereoChange(void)
{
    _fmRxSmData.currCmdInfo.baseCmd->cmdType = FM_RX_INTERNAL_HANDLE_STEREO_CHANGE;

    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void FmHandleMalfunction(void)
{
    _fmRxSmData.currCmdInfo.baseCmd->cmdType = FM_RX_INTERNAL_HANDLE_HW_MAL;

    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 0);

        fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC FMC_BOOL isAfJumpValid(void)
{
    FMC_U8 isValid = FMC_TRUE;

    /* If AF feature is off - do nothing */
    if(_fmRxSmData.afMode == FMC_FALSE)
    {
        isValid = FMC_FALSE;
    }

    /* If we are not tuned or the AF list is empty - do nothing */
    if((_fmRxSmData.tunedFreq== FMC_UNDEFINED_FREQ) || (_fmRxSmData.curStationParams.afListSize == 0)) 
    {
        isValid = FMC_FALSE;
    }

    return isValid;
}

FMC_STATIC void initAfJumpParams(void)
{
    /* Update parameters before starting the jump */
    _fmRxSmData.curAfJumpIndex = 0; 
    /* Save the frequency before the jump to compare later if a jump was done */
    _fmRxSmData.freqBeforeJump = _fmRxSmData.tunedFreq; 
}

FMC_STATIC void HandleAfJumpStartSetPi(void)
{
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Set PI code - must be updated if the af list is not empty */ 
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_PI_SET_GET, _fmRxSmData.curStationParams.piCode); 
}
FMC_STATIC void HandleAfJumpSetPiMask(void)
{
    FMC_U16 piMask = 0xFFFF;

    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Set PI code - must be updated if the af list is not empty */ 
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_RDS_PI_MASK_SET_GET, piMask);
}

FMC_STATIC void HandleAfJumpSetAfFreq(void)
{
    FMC_U16 freqIndex;
    
    freqIndex = FMC_UTILS_FreqToFwChannelIndex(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.curStationParams.afList[_fmRxSmData.curAfJumpIndex]); 

    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Set_Frequency command */    
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_AF_FREQ_SET_GET, freqIndex);
}

FMC_STATIC void HandleAfJumpEnableInt(void)
{
    
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /*Eliminate a race condition that a general interrupt received right before 
    enabling the new specific interrupts. don't update the stage so we enter 
    the same function again after clearing */
    if(_fmRxSmData.interruptInfo.interruptInd)
    {
        _fmRxSmData.interruptInfo.interruptInd = FMC_FALSE;
             prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, 2);/*[ToDo Zvi] the stage index should be connected to the location*/
              /* Read the flag to clear status */
        FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
    }
    else
    {
    _fmRxSmData.interruptInfo.opHandler_int_mask = FMC_FW_MASK_FR;
    _fmRxSmData.interruptInfo.fmMask =  _fmRxSmData.interruptInfo.opHandler_int_mask;
    /* Enable FR interrupt */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.opHandler_int_mask);
    }
}

FMC_STATIC void HandleAfJumpStartAfJump(void)
{
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Start tuning */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_RX_TUNER_MODE_SET, FMC_FW_RX_TUNER_MODE_ALTER_FREQ_JUMP);
}

FMC_STATIC void HandleAfJumpWaitCmdComplete(void)
{
    FMC_ASSERT(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT); 

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);

    /* Got command complete - Just wait for interrupt */
}

FMC_STATIC void HandleAfJumpReadFreq(void)
{
    FMC_ASSERT(_fmRxSmData.callReason == FM_RX_SM_CALL_REASON_INTERRUPT); 

    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    
    /* Verify we got an interrupt - the tuning is finished */
    FMC_ASSERT(_fmRxSmData.interruptInfo.opHandlerIntSetBits & FMC_FW_MASK_FR);

    /* Read frequency to notify the application */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}

FMC_STATIC void HandleAfJumpFinished(void)
{   
    FMC_U32 read_freq, jumped_freq;
    FMC_U16 curPi = _fmRxSmData.curStationParams.piCode; 

    /* No need to enable all interrupt again - it will be done at the end of handling the general interrupts */

    read_freq = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);
    jumped_freq = _fmRxSmData.curStationParams.afList[_fmRxSmData.curAfJumpIndex]; 
     
    /* If the frequency was changed the jump succeeded */
    if(read_freq != _fmRxSmData.freqBeforeJump) 
    {       
        /* There was a jump - make sure it was to the frequency we set */
        FMC_ASSERT(jumped_freq == read_freq);

        _FM_RX_SM_ResetStationParams(read_freq);
        _fmRxSmData.curStationParams.piCode = curPi; /* The PI should stay the same */ 
        
        send_fm_event_af_jump(FM_RX_EVENT_AF_SWITCH_COMPLETE,FM_RX_STATUS_SUCCESS, curPi, _fmRxSmData.freqBeforeJump, read_freq); 

        /* Reset the int_mask */
        _fmRxSmData.interruptInfo.opHandler_int_mask = 0;
        
        /* Call the next stage of general interrupts handler to handle other interrupts */
        genIntHandler[GEN_INT_AFTER_LOW_RSSI_STAGE]();
    }
    /* Tried to jump but jumped back to the original frequency - jump to the next freq in the af list */
    else
    {
        _fmRxSmData.curAfJumpIndex++;   /* Go to next index in the af list */ 
        
        /* If we reached the end of the list - stop searching */
        if(_fmRxSmData.curAfJumpIndex >= _fmRxSmData.curStationParams.afListSize) 
        {
            send_fm_event_af_jump(FM_RX_EVENT_AF_SWITCH_COMPLETE,FM_RX_STATUS_AF_SWITCH_FAILED_LIST_EXHAUSTED, curPi, read_freq, jumped_freq);

            /* Reset the int_mask */
            _fmRxSmData.interruptInfo.opHandler_int_mask = 0;

            /* Call the next stage of general interrupts handler to handle other interrupts */
            genIntHandler[GEN_INT_AFTER_LOW_RSSI_STAGE]();
        }
        /* List is not over - try next one */
        else
        {
            send_fm_event_af_jump(FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED,FM_RX_STATUS_AF_IN_PROGRESS, curPi, read_freq, jumped_freq);

            /* Call the first handler again */
            prepareNextStage(_FM_RX_SM_STATE_NONE, 0);
        
            fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
        }   
    }

}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleStereoChangeStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_STEREO_GET,2);
}

FMC_STATIC void HandleStereoChangeFinish(void)
{
    FMC_U8  monoMode;

    /* 0 = Mono; 1 = Stereo */
    if ((FMC_U8)_fmRxSmData.context.transportEventData.read_param == 0)
    {
        monoMode = 1;
    }
    else
    {
        monoMode = 0;
    }
    
    /* Send event to the applicatoin */
    send_fm_event_most_mode_changed(monoMode);

    /* Finished analyzing - call the next stage of general interrupts handler to handle other interrupts */
    genIntHandler[GEN_INT_AFTER_STEREO_CHANGE_STAGE]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleHwMalStartFinish(void)
{
    FMC_LOG_INFO(("Hw MAL interrupt received - do nothing"));
    /* Finished analyzing - call the next stage of general interrupts handler to handle other interrupts */
    genIntHandler[GEN_INT_AFTER_MAL_STAGE]();
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleReadRdsStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Send Read RDS command */ 
    _fmRxSmData.fmRxReadState = FM_RX_READING_RDS_DATA;
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_RDS_DATA_GET,FMC_FW_RX_RDS_THRESHOLD*3 );
}
FMC_STATIC void HandleReadStatusRegisterToAvoidRdsEmptyInterrupt(void)
{
    FMC_OS_MemSet(_fmRxSmData.rdsParams.rdsGroup, 0, RDS_GROUP_SIZE); 

    GetRDSBlock();
    
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);
    
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET,2);
}
FMC_STATIC void HandleReadRdsAnalyze(void)
{

    /* Finished analyzing - call the next stage of general interrupts handler to handle other interrupts */
    genIntHandler[GEN_INT_AFTER_RDS_STAGE]();
}
/*******************************************************************************************************************/
FMC_STATIC void GetRDSBlock(void)
{
    FMC_U16 len = _fmRxSmData.context.transportEventData.dataLen;
    FMC_U8 metaData; 
    FMC_U8  type; 
    FMC_U8 index;
    FMC_U8 *data = _fmRxSmData.context.transportEventData.data;
    FmRxRdsDataFormat  rdsFormat;
    
    /* Parse the RDS data */
    while (len >= RDS_BLOCK_SIZE)
    {
        metaData = data[2];

        /* Get the type:
         0=A, 1=B, 2=C, 3=C', 4=D, 5=E */
        type = (FMC_U8)(metaData & 0x07);

        /* Transform the block type into an index sequence (0, 1, 2, 3, 4) */
        index =  (FMC_U8)(type <= RDS_BLOCK_C ? type : (type - 1));

/*      FMC_LOG_INFO("Block index = %d", index);*/
        /* Is it block A or is it a sequence block after the previous one? and has no errors?*/
        if (((metaData&RDS_STATUS_ERROR_MASK) == 0)&&
            (index == RDS_BLOCK_INDEX_A ||
                (index == _fmRxSmData.rdsParams.last_block_index + 1 && 
                index <= RDS_BLOCK_INDEX_D)))
        {
            /* Copy it and save the index   */
            FMC_OS_MemCopy(&rdsFormat.rdsData.groupDataBuff.rdsBuff[index * (RDS_BLOCK_SIZE -1)], data, (RDS_BLOCK_SIZE-1)); 
            
            _fmRxSmData.rdsParams.last_block_index = index; 

            /* If completed a whole group then handle it */
            if (index == RDS_BLOCK_INDEX_D)
            {
/*              FMC_LOG_INFO("Block good - handle group");*/

                rdsParseFunc_Switch2DolphinForm(&rdsFormat);

                handleRdsGroup(&rdsFormat); 

                 if ((rdsFormat.groupBitInMask&_fmRxSmData.rdsGroupMask)) 
                 {
                     /* Send the Raw RDS data to the application */
                     send_fm_event_raw_rds(8, rdsFormat.rdsData.groupDataBuff.rdsBuff,rdsFormat.groupBitInMask); 
                     
                }
            }
        }
        else
        {
            FMC_LOG_INFO(("Block sequence mismatch\n"));
            _fmRxSmData.rdsParams.last_block_index = RDS_BLOCK_INDEX_UNKNOWN; 
        }

        len -= RDS_BLOCK_SIZE;
        data += RDS_BLOCK_SIZE;
    }

}

FMC_STATIC FmcRdsGroupTypeMask handleRdsGroup(FmRxRdsDataFormat  *rdsFormat)
{
    FmcRdsGroupTypeMask gType = FM_RDS_GROUP_TYPE_MASK_NONE;

    /*
    |    block A           |    block B                                                        |   block C               |   block D        | 
    |    16it    |8-bit   |    4-bit        |1-bit  |1-bit |5-bit| 5 -bit|8-bit   |    16 -bit   |8-bit   |    16-bit|8-bit | 
    |   PI code|c.w     |   group type|B type|tp     |pty   |other  |c.w     |   PI code   |c.w     |   PI code|c.w  |
    */

    rdsFormat->piCode = FMC_UTILS_BEtoHost16((FMC_U8 *)&rdsFormat->rdsData.groupGeneral.piData);
    /* If PI is unknown et - read it */
    if(_fmRxSmData.curStationParams.piCode != rdsFormat->piCode) 
    {
        _fmRxSmData.curStationParams.piCode = rdsFormat->piCode; 
        send_fm_event_pi_changed(rdsFormat->piCode);
    }

    rdsFormat->groupBitInMask = rdsParseFunc_getGroupType((FMC_U8)((rdsFormat->rdsData.groupGeneral.blockB_byte1& RDS_BLOCK_B_GROUP_TYPE_MASK) >> 3));
    rdsFormat->ptyCode = (FMC_UTILS_BEtoHost16((FMC_U8 *)&rdsFormat->rdsData.groupGeneral.blockB_byte1)&RDS_BLOCK_B_PTY_MASK)>>RDS_NUM_OF_BITS_BEFOR_PTY;

    if(rdsFormat->ptyCode != _fmRxSmData.rdsData.ptyCode)
    {
        _fmRxSmData.rdsData.ptyCode = rdsFormat->ptyCode;
        send_fm_event_pty_changed(rdsFormat->ptyCode);
    }

    /*If the group type was requested in the group mask*/
    if(_fmRxSmData.rdsGroupMask&rdsFormat->groupBitInMask)
    {
            
        /* If it's group type 0 - handle AF and PS */
        if ((rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_0A)||(rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_0B))
        {

            handleRdsGroup0(rdsFormat);
        }
        else if ((rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_2A)||(rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_2B))
        {
            handleRdsGroup2(rdsFormat);
        }
        else
        {
            FMC_LOG_INFO(("Not handling group type %d.\n", rdsFormat->groupBitInMask));
        }
    }
    return gType;
}
FMC_STATIC void handleRdsGroup0(FmRxRdsDataFormat  *rdsFormat)
{
    FMC_U8 psIndex =0;
    FMC_U16 dataB =0;
    FMC_BOOL changed1 = FMC_FALSE;
    FMC_BOOL changed2 = FMC_FALSE;
    int i ;

    dataB = FMC_UTILS_BEtoHost16((FMC_U8 *)&rdsFormat->rdsData.groupGeneral.blockB_byte1);

    psIndex = (FMC_U8)(dataB & RDS_BLOCK_B_PS_INDEX_MASK);
/*      FMC_LOG_INFO("PS index = %d", psIndex);*/
    /* Check if the name index is expected  */
    if (_fmRxSmData.rdsParams.nextPsIndex == psIndex || 
        psIndex == 0)
    {
        _fmRxSmData.rdsParams.nextPsIndex = (FMC_U8)(psIndex + 1); 

        if(psIndex == 0)/*this is the first data byte*/
        {
            rdsParseFunc_updateRepertoire( rdsFormat->rdsData.group0A.firstPsByte , 
                            rdsFormat->rdsData.group0A.secondPsByte);
        }
        /*[ToDo Zvi] The bytes shouldn't be copied if these 2 byte where used for repertoire*/
        _fmRxSmData.rdsParams.psName[psIndex*2] = rdsFormat->rdsData.group0A.firstPsByte; 
        _fmRxSmData.rdsParams.psName[psIndex*2+1] = rdsFormat->rdsData.group0A.secondPsByte; 
	FMC_LOG_INFO(("PSCHARS %d: %c %c\n", psIndex, rdsFormat->rdsData.group0A.firstPsByte, rdsFormat->rdsData.group0A.secondPsByte));


        /* Is this is the last index?   */
        if (psIndex + 1 >= RDS_PS_NAME_LAST_INDEX)
        {
            /* If ps name was changed or it's the first time - update */
            if(!FMC_OS_MemCmp((FMC_U8*)(_fmRxSmData.rdsParams.psName), RDS_PS_NAME_SIZE,_fmRxSmData.curStationParams.psName, RDS_PS_NAME_SIZE)) 
            {
		_fmRxSmData.rdsParams.psNameSameCount = 1;
                FMC_OS_MemCopy(_fmRxSmData.curStationParams.psName, (FMC_U8*)(_fmRxSmData.rdsParams.psName), RDS_PS_NAME_SIZE); 

            }
            else
            {
                if(_fmRxSmData.rdsParams.psNameSameCount <= 1)
                {
                   _fmRxSmData.rdsParams.psNameSameCount++;

                   if(_fmRxSmData.rdsParams.psNameSameCount == 2)
                    {
                        send_fm_event_ps_changed(_fmRxSmData.tunedFreq, _fmRxSmData.curStationParams.psName);
                       _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
                    }
                }
/*                  FMC_LOG_INFO("PS name NOT changed\n");                  */
            }
        }
    }
    else 
    {
        /* Reset the expected index to prevent accidentally receive the correct index of the next ps name */
        _fmRxSmData.rdsParams.nextPsIndex = RDS_NEXT_PS_INDEX_RESET; 

        FMC_LOG_INFO(("PS Name not in sequence - ignored.\n"));
    }           


/*****************************
 * Check Alternate Frequencies
 *****************************/
 
    /* Check that we work with 0A group (and not 0B) that contains AF 
        Check no errors in block C (contains AF) */
    if (rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_0A)
    {

        changed1 = checkNewAf(rdsFormat->rdsData.group0A.firstAf);
        changed2 = checkNewAf(rdsFormat->rdsData.group0A.secondAf);

        if (changed1 || changed2)
        {
            send_fm_event_af_list_changed(_fmRxSmData.curStationParams.piCode, _fmRxSmData.curStationParams.afListSize, _fmRxSmData.curStationParams.afList); 
        }
    }
}

FMC_STATIC FMC_BOOL checkNewAf(FMC_U8 af)
{
    FMC_U32 freq;
    FMC_U32 base_freq;
    FMC_U8 index;

    FMC_BOOL changed = FMC_FALSE;
    
    /* First AF indicates the number of AF follows. Reset the list */
    if((af >= RDS_1_AF_FOLLOWS) && (af <= RDS_25_AF_FOLLOWS))
    {
        _fmRxSmData.curStationParams.afListCurMaxSize = (FMC_U8)(af - RDS_1_AF_FOLLOWS + 1); 
        _fmRxSmData.curStationParams.afListSize = 0; 
        changed = FMC_TRUE;
    }
    else if (((af >= RDS_MIN_AF) && (_fmRxSmData.band == FMC_BAND_EUROPE_US) && (af <= RDS_MAX_AF)) ||
            ((af >= RDS_MIN_AF) && (_fmRxSmData.band == FMC_BAND_JAPAN) && (af <= RDS_MAX_AF_JAPAN)))
    {
        
        base_freq = _fmRxSmData.band? FMC_FIRST_FREQ_JAPAN_KHZ : FMC_FIRST_FREQ_US_EUROPE_KHZ;
        freq = base_freq + (af * 100);

        /* If the af frequency is the same as the tuned station frequency - ignore it */
        if(freq == _fmRxSmData.tunedFreq)
            return changed;

        for(index=0; ((index<_fmRxSmData.curStationParams.afListSize) && (index < _fmRxSmData.curStationParams.afListCurMaxSize)); index++) 
        {
            if(_fmRxSmData.curStationParams.afList[index] == freq) 
                break;
        }
        
        /* Reached the limit of the list - ignore the next AF */
        if(index == _fmRxSmData.curStationParams.afListCurMaxSize) 
        {
/*          FMC_LOG_INFO("Error - af list is longer than expected.\n");*/
            return changed;
        }
        /*If we reached the end of the list then this AF is not in the list - add it */
        if(index == _fmRxSmData.curStationParams.afListSize) 
        {
            changed = FMC_TRUE;
            _fmRxSmData.curStationParams.afList[index] = freq; 
            _fmRxSmData.curStationParams.afListSize++; 
        }
    }

    return changed;
}

FMC_STATIC void handleRdsGroup2(FmRxRdsDataFormat  *rdsFormat)
{
    /*segment address*/
    FMC_U8 rtIndex;
    
    FMC_U8 rtStartIndex =0;
    FMC_U8 zeroD_Index =0;
    FMC_U16 dataB = 0;
    FMC_BOOL isAGroup;
    FMC_BOOL abFlag;
    FMC_BOOL wasRepertoireUpdated = FMC_FALSE;
    
    /* get the bytes of the Block B in the Group 2 rds data buffer*/    
    dataB = FMC_UTILS_BEtoHost16((FMC_U8 *)&rdsFormat->rdsData.groupGeneral.blockB_byte1);

    /* Check if it's A or B group type */
    if (rdsFormat->groupBitInMask == FM_RDS_GROUP_TYPE_MASK_2A)
    {
        /* 
            RT can Be in 2A or 2B group - in each 2B group we have 2 bytes (in block D) of RT data (the first 2 are used for PI code). 
            in 2A we have 4 bytes (in blocks C,D).
        */
        _fmRxSmData.rdsParams.numOfRtBytesInGroup = 4;
        isAGroup = FMC_TRUE;
    }
    else
    {
        _fmRxSmData.rdsParams.numOfRtBytesInGroup = 2;
        isAGroup = FMC_FALSE;
    }
        
    /* In RT it possible that we will recieve only substring of the full RT string, in this case the app should replace only the sub string of the full text.
        Our method of work is: 
        - If A/B flag was changed send the data recieved untill now and start new data sequence with changed flag true
        - save the index of first byte recieved(after previos data sent to app/reseted due to A/B flag).
        - while recieveing bytes ,in order, copy and continue.
        - if recieved data not in order - this is a substring update - so send it to app before continue.
        - if reached end of text/max_size send to app.
      */
    
    /*get the index of the current bytes in the RT string*/ 
    rtIndex = (FMC_U8)(dataB & RDS_BLOCK_B_RT_INDEX_MASK);
    rtStartIndex = (FMC_U8)(rtIndex*_fmRxSmData.rdsParams.numOfRtBytesInGroup);

    /*get the A/B flag - if it is toggeled the recieved data is a new massege and the previos should be cleaned*/
    abFlag = ((dataB & RDS_BLOCK_B_AB_FLAG_MASK) != 0);

    /*If there is no real 0x0D to indicate end of text emulate as if it is the first byte in next group*/
    zeroD_Index = _fmRxSmData.rdsParams.numOfRtBytesInGroup;
    /*  
        If this is not the Insex we expected(next index) - then send only this substring to application reset indexes and start from begining
        If the A/B flag was changed send the data recieved so fur and only then reset the indexes and indicate that the flag was changed
    */
    if((_fmRxSmData.rdsParams.nextRtIndex != rtIndex)||(abFlag != _fmRxSmData.rdsParams.prevABFlag)) 
    {
        rdsParseFunc_sendRtAndResetIndexes(zeroD_Index);
        
        /* Flag that A/B flag was changed for the next time you send data to app*/
        if(abFlag != _fmRxSmData.rdsParams.prevABFlag) 
        {
            /**/
            _fmRxSmData.rdsParams.abFlagChanged = FMC_TRUE; 
            _fmRxSmData.rdsParams.prevABFlag = (FMC_U8)(abFlag); 
        }
    }
    if(isAGroup)
    {
        if(rtStartIndex == 0)
        {
            wasRepertoireUpdated = rdsParseFunc_updateRepertoire( rdsFormat->rdsData.group2A.firstRtByte ,rdsFormat->rdsData.group2A.secondRtByte);
            _fmRxSmData.rdsParams.wasRepertoireUpdated = wasRepertoireUpdated;
        }
        if(!wasRepertoireUpdated)
        {/*I the repertoire was updated the first 2 bytes are not data bytes - they only indicate the reperetoire - and therefor should be ignored*/
            _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] = rdsFormat->rdsData.group2A.firstRtByte;  
            _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] = rdsFormat->rdsData.group2A.secondRtByte; 
        }
        _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] = rdsFormat->rdsData.group2A.thirdRtByte; 
        _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] = rdsFormat->rdsData.group2A.fourthRtByte; 
    }       
    else
    {
        if(rtStartIndex == 0)
        {
            wasRepertoireUpdated = rdsParseFunc_updateRepertoire( rdsFormat->rdsData.group2B.firstRtByte ,rdsFormat->rdsData.group2B.secondRtByte);
            _fmRxSmData.rdsParams.wasRepertoireUpdated = wasRepertoireUpdated;
        }
        if(!wasRepertoireUpdated)
        {/*I the repertoire was updated the first 2 bytes are not data bytes - they only indicate the reperetoire - and therefor should be ignored*/
            _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] =  rdsFormat->rdsData.group2B.firstRtByte; 
            _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength++] = rdsFormat->rdsData.group2B.secondRtByte; 
        }
    }
    _fmRxSmData.rdsParams.nextRtIndex = (FMC_U8)(rtIndex + 1); 
    /* Is this the last index? */
    if (rdsParseFunc_CheckIfReachedEndOfText(&zeroD_Index,rtIndex))
    {
        rdsParseFunc_sendRtAndResetIndexes(zeroD_Index);
    }
}
/*
*   This function sends the data if such exists and reset the indexes.
*    - the on parameter index is used to indicate the location of "0x0D" in the last group 
*       whitch is needed to calculate corectly the start location of the data.
*       if "0x0D" was not resieved zero will be sent as index   
*/
FMC_STATIC void rdsParseFunc_sendRtAndResetIndexes(FMC_U8 zeroD_Index)
{
    FMC_U8 indexOfstartLocationOfData;
    /*I repertoire was updated then first 2 bytes in first group where repertoire and not rt data*/
    FMC_U8 numOfNonRtBytesInFirstGroup = 0;
    if(_fmRxSmData.rdsParams.rtLength!=0) 
    {
        
        indexOfstartLocationOfData = (FMC_U8)(_fmRxSmData.rdsParams.nextRtIndex*_fmRxSmData.rdsParams.numOfRtBytesInGroup - 
            _fmRxSmData.rdsParams.rtLength);
        /*get the start index of the data */
        if(_fmRxSmData.rdsParams.wasRepertoireUpdated)
        {
            numOfNonRtBytesInFirstGroup = 2;
        }
        /*we should remove from the length the bytes in the last group after the 0x0D appeared (the byte in the group indicated by index)*/
        _fmRxSmData.rdsParams.rtLength =  (FMC_U8)(_fmRxSmData.rdsParams.rtLength - 
                                            _fmRxSmData.rdsParams.numOfRtBytesInGroup+
                                            numOfNonRtBytesInFirstGroup +
                                            zeroD_Index); 
        _fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength] = '\0'; 
        send_fm_event_radio_text(_fmRxSmData.rdsParams.abFlagChanged,  
                            _fmRxSmData.rdsParams.rtLength,  
                            (FMC_U8*)(_fmRxSmData.rdsParams.radioText), 
                            indexOfstartLocationOfData); 
        FMC_LOG_INFO(("handleRdsGroup2: Sent RT to app length = %d start index = %d .\n",
                    _fmRxSmData.rdsParams.rtLength, 
                    indexOfstartLocationOfData)); 
    }

    /*******reset indexes********/

    /* Te changed indication should be sent to application only once*/
    _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
    _fmRxSmData.rdsParams.abFlagChanged = FMC_FALSE;
    _fmRxSmData.rdsParams.wasRepertoireUpdated = FMC_FALSE;
    _fmRxSmData.rdsParams.rtLength = 0; 
    FMC_OS_MemSet(_fmRxSmData.rdsParams.radioText, 0, RDS_RADIO_TEXT_SIZE+1); 
    _fmRxSmData.rdsParams.nextRtIndex = RDS_NEXT_RT_INDEX_RESET;
}
/*
*
*/
FMC_STATIC FMC_BOOL rdsParseFunc_CheckIfReachedEndOfText(FMC_U8 * index, FMC_U8 rtIndex)
{
    FMC_U8 zeroDindex  = 0; 
    /* Check if the RT is finished before the maximum characters received ("0x0D" indicates end of string)*/
    for(zeroDindex = 0; zeroDindex < _fmRxSmData.rdsParams.numOfRtBytesInGroup; zeroDindex++)
    {
        if(_fmRxSmData.rdsParams.radioText[_fmRxSmData.rdsParams.rtLength - _fmRxSmData.rdsParams.numOfRtBytesInGroup +zeroDindex] == RDS_RT_END_CHARACTER) 
        {
            *index = zeroDindex;
            return FMC_TRUE;
    
        }
    }
    /* If we reached the last valid index */
    /* Udi - Replace RDS_RADIO_TEXT_SIZEwith 64 / 32 depending on group type */
    if(rtIndex + 1 >= (RDS_RADIO_TEXT_SIZE/_fmRxSmData.rdsParams.numOfRtBytesInGroup))
    {
        *index = zeroDindex;
        return FMC_TRUE;
    }
    *index = zeroDindex;
    return FMC_FALSE;
    
}
/*
*
*/
FMC_STATIC void rdsParseFunc_Switch2DolphinForm(FmRxRdsDataFormat  *rdsFormat)
{
    /*  [ToDo Zvi]Since in Orca the 2 RDS Data bytes are in Little endian and in Dolphin they are in Big endian
    *   The Parsing of the RDS Data is Chip Depended and should be moved to some HAL related
    *   Location.
    */  
    
    FMC_U8 byte1;
    FMC_U8 index =0;

    if(_fmRxSmData.fmAsicId != 0x6350)
    {
        while(index+1<8)
        {
            byte1 = rdsFormat->rdsData.groupDataBuff.rdsBuff[index]; 
            rdsFormat->rdsData.groupDataBuff.rdsBuff[index] = rdsFormat->rdsData.groupDataBuff.rdsBuff[index+1]; 
            rdsFormat->rdsData.groupDataBuff.rdsBuff[index+1] = byte1; 
            index+=2;
        }
    }
}
/*
    The translation of RDS text data bytes is determined using the code-tables in figures E.1 (G0), E.2 (G1), E.3 (G2) from the spec. 
    In the spec it is defined that by default the code table in use will be the one in figure E.1 (G0) in the spec 
    (I am looking at "UNITED STATES RBDS STANDARD April 9, 1998").
    
    If different code table is needed, the first 2 bytes of the text data (PS, RT …), the bytes of index 0,
    will contain a code (that is not part of the code-table) to indicate the code-table needed to translate the text data bytes.
    
    The codes used are: 
    
    - 0/15, 0/15: code-table of figure E.1(G0)
    
    - 0/14, 0/14: code-table of figure E.2(G1)
    
    - 1/11, 6/14: code-table of figure E.3(G2)
    
*/

FMC_STATIC FMC_BOOL rdsParseFunc_updateRepertoire(FMC_U8 byte1,FMC_U8 byte2)
{
    
    FMC_U8 repertoire1,repertoire2;
    FMC_U8 repertoire3,repertoire4;
    FMC_BOOL RepertoireFound = FMC_FALSE;
    /*replace to nibble high/low*/
    repertoire1 =  (FMC_U8)(byte1&RDS_BIT_0_TO_BIT_3);
    repertoire2 =  (FMC_U8)((byte1&RDS_BIT_4_TO_BIT_7)>>4);
    repertoire3 =  (FMC_U8)(byte2&RDS_BIT_0_TO_BIT_3);
    repertoire4 =  (FMC_U8)((byte2&RDS_BIT_4_TO_BIT_7)>>4);

    if((repertoire2==0)&&(repertoire1==15)&&(repertoire4==0)&&(repertoire3==15))
    {
        _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
        RepertoireFound = FMC_TRUE;
    }
    else if((repertoire2==0)&&(repertoire1==14)&&(repertoire4==0)&&(repertoire3==14))
    {
        _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G1_CODE_TABLE;
        RepertoireFound = FMC_TRUE;
    }
    else if ((repertoire2==1)&&(repertoire1==11)&&(repertoire4==6)&&(repertoire3==14))
    {
        _fmRxSmData.rdsData.repertoire = FMC_RDS_REPERTOIRE_G2_CODE_TABLE;
        RepertoireFound = FMC_TRUE;
    }

    return RepertoireFound;
}
/*
*
*/
FMC_STATIC FmcRdsGroupTypeMask rdsParseFunc_getGroupType(FMC_U8 group)
{

    FMC_U16 towPower = group;
    FMC_U32 value = 1;
    while(towPower>0)
    {
        value *=2;
        towPower --;
    }
    return value;
    
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
FMC_STATIC void HandleTimeoutStart(void)
{
    /* Update to next handler */
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);

    /* Update global parameter gen_int_mask to enable low RSSI interrupt -
     * it was disabled during FM_CONFIG_RX_AF_TIMER_MS timeout */
    _fmRxSmData.interruptInfo.gen_int_mask |= FMC_FW_MASK_LEV;
    _fmRxSmData.interruptInfo.fmMask =  _fmRxSmData.interruptInfo.gen_int_mask;
    /* Enable the interrupts */
    FMC_CORE_SendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET, _fmRxSmData.interruptInfo.gen_int_mask);
}

FMC_STATIC void HandleTimeoutFinish(void)
{
    
    _FM_RX_SM_RemoveFromQueueAndFreeCmd(&_fmRxSmData.currCmdInfo.baseCmd);
    
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
    
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/

FMC_STATIC void HandleGetTunedFreqStart(void)
{   
   
    /* Update to next handler */ 
    prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);    

    /* Read frequency to notify the application */
    FMC_CORE_SendReadCommand(FMC_FW_OPCODE_RX_FREQ_SET_GET,2);
}

FMC_STATIC void HandleGetTunedFreqEnd(void)
{   
    FMC_U32 freq;

    freq = FMC_UTILS_FwChannelIndexToFreq(_fmRxSmData.band,FMC_CHANNEL_SPACING_50_KHZ,_fmRxSmData.context.transportEventData.read_param);

    _fmRxSmData.tunedFreq = freq;
       
    _fmRxSmData.context.appEvent.p.cmdDone.value = freq;

    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}


/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetGroupMaskStartEnd(void)
{
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.rdsGroupMask;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetRdsSystemStartEnd(void)
{
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.rdsRdbsSystem;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetBandStartEnd(void)
{
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.band;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetMuteModeStartEnd(void)

{
_fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.muteMode;
_fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
_FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);

}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetRfDepMuteModeStartEnd(void)

{
_fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.rfDependedMute;
_fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
_FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);

}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetRssiThresholdStartEnd(void)
{
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.rssiThreshold;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetDeemphasisFilterStartEnd(void)
{
    
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.deemphasisFilter;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetVolumeStartEnd(void)
{
    /*[ToDo Zvi] check what are we getting here - it it only the relative gain, or absolute volume- if gain we need to request for vol */
    
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.volume;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleGetAfSwitchModeStartEnd(void)
{
    
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.afMode;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
#if 0 /* warning removal - unused code (yet) */
void HandleGetAudioRoutingModeStartEnd(void)
{
    
    _fmRxSmData.context.appEvent.p.cmdDone.value = _fmRxSmData.vacParams.audioTargetsMask;

    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
#endif /* 0 - warning removal - unused code (yet) */
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
void HandleSetGroupMaskStartEnd(void)
{
    _fmRxSmData.rdsGroupMask = ((FmRxSetRdsMaskCmd *)_fmRxSmData.currCmdInfo.baseCmd)->mask;
    
    _fmRxSmData.context.appEvent.p.cmdDone.value =_fmRxSmData.rdsGroupMask;
    _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_SUCCESS;
    _FM_RX_SM_HandleCompletionOfCurrCmd(NULL,NULL,NULL,FM_RX_EVENT_CMD_DONE);
}
/*******************************************************************************************************************
 *                  
 *******************************************************************************************************************/
 
FMC_U16 _FM_RX_UTILS_findNextIndex(FmcBand band,FmRxSeekDirection dir, FMC_U16 index)
{

    FMC_S16 new_index;
    FMC_U32 max_index;
    FMC_U32 min_index=0;
    FMC_UINT channelSpacing = FMC_UTILS_ChannelSpacingInKhz(FMC_CHANNEL_SPACING_50_KHZ);

    max_index = (band == FMC_BAND_EUROPE_US)? 
                ((FMC_LAST_FREQ_US_EUROPE_KHZ - FMC_FIRST_FREQ_US_EUROPE_KHZ)/channelSpacing) : 
                ((FMC_LAST_FREQ_JAPAN_KHZ - FMC_FIRST_FREQ_JAPAN_KHZ)/channelSpacing);

 
    /* Check the offset in order to be aligned to the spacing that was set  */ 
    
    if(dir == FM_RX_SEEK_DIRECTION_UP)
    {
        new_index = (FMC_S16)(index + _fmRxSmData.channelSpacing);
    }
    else
    {
        new_index = (FMC_S16)(index - _fmRxSmData.channelSpacing);
    }

/*If we got to an out of scop index while the direction is up start from min_index
    If we got to an out of scop index while the direction is down start from max_index */
    if(new_index < 0)
    {       
      new_index = (FMC_S16)(max_index);                
    }
    else if((FMC_U32)new_index > max_index)
    {
        new_index = (FMC_S16)(min_index);
    }
    return (FMC_U16)new_index;
}

McpBtsSpStatus _FM_RX_SM_TiSpSendHciScriptCmd(  McpBtsSpContext *context,
                                                        McpU16  hciOpcode, 
                                                        McpU8   *hciCmdParms, 
                                                        McpUint len)
{
        
    FMC_UNUSED_PARAMETER(context);

    /* Send the next HCI Script command */
    FMC_CORE_SendHciScriptCommand(  hciOpcode, 
                                                                    hciCmdParms, 
                                                                    len);

    /* [ToDo] - Convert fmcStatus to TiSpStatus */
    return MCP_BTS_SP_STATUS_PENDING;
}

/*
    This is the callback function that is called by the script processor to notify that script execution completed

*/
void _FM_RX_SM_TiSpExecuteCompleteCb(McpBtsSpContext *context, McpBtsSpStatus status)
{
    FMC_UNUSED_PARAMETER(context);
    FMC_UNUSED_PARAMETER(status);
    /* Update to next handler */
    
    prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);

    fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
    /* Activate the appropriate stage handler of the current process */
}

FmRxStatus FM_RX_SM_AllocateCmdAndAddToQueue(   FmRxContext     *context,
                                                                FmRxCmdType cmdType,
                                                                FmcBaseCmd      **cmd)
{
    FmcStatus   status;

    FMC_FUNC_START("FM_RX_SM_AllocateCmdAndAddToQueue");

    /* Allocate space for the new command */
    status = _FM_RX_SM_AllocateCmd(context, cmdType, cmd);
    FMC_VERIFY_ERR((status == FM_RX_STATUS_SUCCESS), status, ("FM_RX_SM_AllocateCmdAndAddToQueue"));

    /* Init base fields of cmd structure*/
    FMC_InitializeListNode(&((*cmd)->node));
    (*cmd)->context = context;
    (*cmd)->cmdType = cmdType;

    /* Insert the cmd as the last cmd in the cmds queue */
    FMC_InsertTailList(FMCI_GetCmdsQueue(), &((*cmd)->node));

    smRxNumOfCmdInQueue++;

    FMC_FUNC_END();
    
    return status;
}

FmRxStatus _FM_RX_SM_RemoveFromQueueAndFreeCmd(FmcBaseCmd **cmd)
{
    FmcStatus   status;

    FMC_FUNC_START("_FM_RX_SM_RemoveFromQueueAndFreeCmd");

    /* Remove the cmd element from the queue */
    FMC_RemoveNodeList(&((*cmd)->node));

    /* Free the cmd structure space */
    status = _FM_RX_SM_FreeCmd(cmd);
    FMC_VERIFY_FATAL((status == FM_RX_STATUS_SUCCESS), status, ("_FM_RX_SM_RemoveFromQueueAndFreeCmd"));
    
    smRxNumOfCmdInQueue--;
    FMC_FUNC_END();
    
    return status;
}

/* Allocate space for the new cmd data */
FmRxStatus _FM_RX_SM_AllocateCmd(   FmRxContext     *context,
                                            FmRxCmdType cmdType,
                                            FmcBaseCmd      **cmd)
{
    FmRxStatus      status = FM_RX_STATUS_SUCCESS;

    FMC_FUNC_START("FM_RX_SM_AllocateCmd");

    FMC_UNUSED_PARAMETER(context);
    
    *cmd = (FmcBaseCmd *)malloc(sizeof(FmcBaseCmd));

    if (!(*cmd))
    {
        FMC_LOG_ERROR(("FM RX: Memory exhausted? (%s)", strerror(errno)));
        FMC_RET(FMC_STATUS_TOO_MANY_PENDING_CMDS);
    }
    memset(*cmd, 0, sizeof(FmcBaseCmd));

    FMC_FUNC_END();
    
    return status;
}

FmRxStatus _FM_RX_SM_FreeCmd(FmcBaseCmd **cmd)
{
    /* Allocate space for the new cmd data */
    FmRxStatus      status = FM_RX_STATUS_SUCCESS;
    FmRxCmdType cmdType;
        
    FMC_FUNC_START("FM_RX_SM_FreeCmd");

    memset(*cmd, 0xF0, sizeof(FmcBaseCmd));
    free(*cmd);
    *cmd = NULL;

    FMC_FUNC_END();
    
    return status;
}
FmRxStatus _FM_RX_SM_HandleCompletionOfCurrCmd( _FmRxSmSetCmdCompleteFunc   cmdCompleteFunc ,
                                                            _FmRxSmCmdCompletedFillEventDataCb fillEventDataCb, 
                                                            void *eventData,
                                                            FmRxEventType       evtType)
{
    FmRxStatus status;
    
    /* Make a local copy of the current cmd info - we are going to "destroy" the official values */
    FmcBaseCmd  currCmd = *_fmRxSmData.currCmdInfo.baseCmd;
    FmRxStatus  cmdCompletionStatus = _fmRxSmData.currCmdInfo.status;


    FMC_FUNC_START("_FM_RX_SM_HandleCompletionOfCurrCmd");

    FMC_UNUSED_PARAMETER(cmdCompleteFunc);
    FMC_UNUSED_PARAMETER(fillEventDataCb);
    FMC_UNUSED_PARAMETER(eventData);
    /* If a function is specified, call it */

	/*If disable coammnd is given. Make sure you comeplete all the processing and then inform App*/
	if((evtType == FM_RX_EVENT_CMD_DONE)&&
	(currCmd.cmdType == FM_RX_CMD_DISABLE))
	{

	status = _FM_RX_SM_RemoveFromQueueAndFreeCmd(&_fmRxSmData.currCmdInfo.baseCmd);
	FMC_VERIFY_FATAL((status == FM_RX_STATUS_SUCCESS), status, ("_FM_RX_SM_HandleCompletionOfCurrCmd"));

	/* Notify the appliction about completion of the command */
	_FM_RX_SM_SendAppEvent( currCmd.context, 
		    cmdCompletionStatus, 
		    currCmd.cmdType,
		    evtType);
	}
	else
	{
	/* Notify the appliction about completion of the command */
	_FM_RX_SM_SendAppEvent( currCmd.context, 
		    cmdCompletionStatus, 
		    currCmd.cmdType,
		    evtType);

	status = _FM_RX_SM_RemoveFromQueueAndFreeCmd(&_fmRxSmData.currCmdInfo.baseCmd);
	FMC_VERIFY_FATAL((status == FM_RX_STATUS_SUCCESS), status, ("_FM_RX_SM_HandleCompletionOfCurrCmd"));
	}
    
    FMC_FUNC_END();

    return status;
}
void _FM_RX_SM_SendAppEvent( FmRxContext                        *context, 
                                                        FmRxStatus                      status,
                                                        FmRxCmdType                 cmdType,
                                                        FmRxEventType       evtType)
{  

    _fmRxSmData.context.appEvent.context = context;    
    _fmRxSmData.context.appEvent.eventType = evtType;
    _fmRxSmData.context.appEvent.status = status;

    if(evtType == FM_RX_EVENT_CMD_DONE)
        _fmRxSmData.context.appEvent.p.cmdDone.cmd = cmdType;

    /*Verify if the event is a disable event sent by the FM_RX_Init_Async
      If it is dont pass the event to the App*/
     if((evtType == FM_RX_EVENT_CMD_DONE)&&
         (cmdType==FM_RX_CMD_DISABLE)&&
         (fmRxSendDisableEventToApp==FMC_FALSE))
	 {   

            /*Verify that the Disable command was sent successfully
            And send Destroy accordingly and save the status for the callback */
            _fmRxSmData.context.appEvent.status= FM_RX_SM_Verify_Disable_Status(status);
            /*Change the cmd type since its an event from the FM_RX_Init_Asynch */
            _fmRxSmData.context.appEvent.p.cmdDone.cmd = FM_RX_INIT_ASYNC;

             /*Call the FM_RX_Init_Async callback function*/
             (fmRxInitAsyncAppCallback)(&_fmRxSmData.context.appEvent);

             /*Reset The disable flag to TRUE
              To be able to get the disable callback*/
             fmRxSendDisableEventToApp=FMC_TRUE;		
		 return;
	 }

    /* 
        If we got STOP_SEEK command when we are in seek mode
        we need to send the FM_RX_CMD_STOP_SEEK command to the APP
        Acorrding to the API
    */
	if((_fmRxSmData.context.appEvent.status==FM_RX_STATUS_SEEK_STOPPED)&&
            (_fmRxSmData.context.appEvent.p.cmdDone.cmd==FM_RX_CMD_SEEK))
            _fmRxSmData.context.appEvent.p.cmdDone.cmd=FM_RX_CMD_STOP_SEEK;


 /* 
        If we got STOP_COMPLETE_SCAN command when we are in complete scan operation
        we need to send the FM_RX_CMD_STOP_COMPLETE_SCAN.       
    */
	if((_fmRxSmData.context.appEvent.status==FM_RX_STATUS_COMPLETE_SCAN_STOPPED)&&
            (_fmRxSmData.context.appEvent.p.cmdDone.cmd==FM_RX_CMD_COMPLETE_SCAN))
            _fmRxSmData.context.appEvent.p.cmdDone.cmd=FM_RX_CMD_STOP_COMPLETE_SCAN;

    
    (_fmRxSmData.context.appCb)(&_fmRxSmData.context.appEvent);
}


        
/*FMC_STATIC void send_fm_event_audio_path_changed(FmRxAudioPath audioPath)
{

    _fmRxSmData.context.appEvent.p.cmdDone.value = audioPath;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_AUDIO_PATH_CHANGED);
}*/

FMC_STATIC void send_fm_event_ps_changed(FMC_U32 freq, FMC_U8 *data)
{
    _fmRxSmData.context.appEvent.p.psData.repertoire = _fmRxSmData.rdsData.repertoire;
    _fmRxSmData.context.appEvent.p.psData.frequency = freq;
    _fmRxSmData.context.appEvent.p.psData.name = data;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_PS_CHANGED);
}

FMC_STATIC void send_fm_event_af_list_changed(FMC_U16 pi, FMC_U8 afListSize, FmcFreq *afList)
{
    _fmRxSmData.context.appEvent.p.afListData.pi = pi;
    _fmRxSmData.context.appEvent.p.afListData.afListSize = afListSize;
    _fmRxSmData.context.appEvent.p.afListData.afList = afList;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_AF_LIST_CHANGED);
}

FMC_STATIC void send_fm_event_af_jump(FmRxEventType eventType,FMC_U8 status, FMC_U16 pi, FMC_U32 oldFreq, FMC_U32 newFreq)
{
    _fmRxSmData.context.appEvent.p.afSwitchData.pi = pi;
    _fmRxSmData.context.appEvent.p.afSwitchData.tunedFreq = oldFreq;
    _fmRxSmData.context.appEvent.p.afSwitchData.afFreq = newFreq;
    
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, status, FM_RX_CMD_NONE, eventType);
}

FMC_STATIC void send_fm_event_radio_text(FMC_BOOL changed, FMC_U8 length, FMC_U8 *radioText,FMC_U8 dataStartIndex)
{
    _fmRxSmData.context.appEvent.p.radioTextData.repertoire = _fmRxSmData.rdsData.repertoire;
    _fmRxSmData.context.appEvent.p.radioTextData.resetDisplay = changed;
    _fmRxSmData.context.appEvent.p.radioTextData.startIndex = dataStartIndex;
    _fmRxSmData.context.appEvent.p.radioTextData.len = length;
    _fmRxSmData.context.appEvent.p.radioTextData.msg = radioText;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_RADIO_TEXT);
}

FMC_STATIC void send_fm_event_most_mode_changed(FMC_U8 mode)
{
    _fmRxSmData.context.appEvent.p.monoStereoMode.mode = mode;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_MONO_STEREO_MODE_CHANGED);
}

FMC_STATIC void send_fm_event_pty_changed(FmcRdsPtyCode ptyCode)
{
    _fmRxSmData.context.appEvent.p.ptyChangedData.pty= ptyCode;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_PTY_CODE_CHANGED);
}
FMC_STATIC void send_fm_event_pi_changed(FmcRdsPiCode piCode)
{
    _fmRxSmData.context.appEvent.p.piChangedData.pi = piCode;
    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_PI_CODE_CHANGED);
}
FMC_STATIC void send_fm_event_raw_rds(FMC_U16 len, FMC_U8 *data, FmcRdsGroupTypeMask gType)
{
    FMC_U16 maxLen = RDS_RAW_GROUP_DATA_LEN;

    if(len < RDS_RAW_GROUP_DATA_LEN)
    {
        maxLen = len;
    }

    _fmRxSmData.context.appEvent.p.rawRdsGroupData.groupBitInMask = gType;

    FMC_OS_MemCopy(_fmRxSmData.context.appEvent.p.rawRdsGroupData.groupData, data, maxLen);

    _FM_RX_SM_SendAppEvent(&_fmRxSmData.context, FM_RX_STATUS_SUCCESS, FM_RX_CMD_NONE, FM_RX_EVENT_RAW_RDS);
}

void _FM_RX_SM_TccmVacCb(ECAL_Operation eOperation,ECCM_VAC_Event eEvent, ECCM_VAC_Status eStatus)
{
    /* Handel vac events only if waiting for VAC Op to complete*/
    if((_fmRxSmData.currCmdInfo.smState == _FM_RX_SM_STATE_WAITING_FOR_CC)&&
        ((eOperation == CAL_OPERATION_FM_RX)||
        (eOperation == CAL_OPERATION_FM_RX_OVER_SCO)||
        (eOperation == CAL_OPERATION_FM_RX_OVER_A3DP)))
    {
        FmcCoreEvent fmEventParms;
        FMC_UNUSED_PARAMETER(eOperation);
        fmEventParms.type = FMC_UTILS_ConvertVacEvent2FmcEvent(eEvent);
        fmEventParms.status = FMC_UTILS_ConvertVacStatus2FmcStatus(eStatus);
        fmEventParms.dataLen= 0;
        fmEventParms.fmInter = FMC_FALSE;
        
        /* Use the transport callback mechanism to notify the FM for the event*/    
        _FM_RX_SM_TransportEventCb(&fmEventParms);
    }
}

void _FM_RX_SM_TransportEventCb(const FmcCoreEvent *eventParms)
{
    FMC_FUNC_START("_FM_RX_SM_TransportEventCb");
    /* Copy the event data to a local storage */
    if(!eventParms->fmInter)
    {
    
        FMC_VERIFY_FATAL_NORET((eventParms->status == FMC_STATUS_SUCCESS), 
                                    ("_FM_RX_SM_TransportEventCb: Transport Event with Error Status"));

        _fmRxSmData.context.transportEventData.dataLen= (FMC_U8)eventParms->dataLen;
        _fmRxSmData.context.transportEventData.status = eventParms->status;
        
        FMC_OS_MemSet(_fmRxSmData.context.transportEventData.data, 0, FMC_FW_RX_RDS_THRESHOLD_MAX*RDS_BLOCK_SIZE);
        FMC_OS_MemCopy(_fmRxSmData.context.transportEventData.data, eventParms->data, eventParms->dataLen);
    
        
        switch (eventParms->type) {
            
            case FMC_CORE_EVENT_TRANSPORT_ON_COMPLETE:
            case FMC_CORE_EVENT_TRANSPORT_OFF_COMPLETE:
        
                fm_recvd_transportOnOffCmplt();
                
            break;
            case FMC_CORE_EVENT_SCRIPT_CMD_COMPLETE:
                fm_recvd_initCmdCmplt();
                break;
            case FMC_VAC_EVENT_OPERATION_STARTED:
            case FMC_VAC_EVENT_OPERATION_STOPPED:
            case FMC_VAC_EVENT_RESOURCE_CHANGED:
         case FMC_VAC_EVENT_CONFIGURATION_CHANGED:
            case FMC_CORE_EVENT_WRITE_COMPLETE:
            case FMC_CORE_EVENT_POWER_MODE_COMMAND_COMPLETE:
                fm_recvd_writeCmdCmplt();
                break;
            case FMC_CORE_EVENT_READ_COMPLETE:
                fm_recvd_readCmdCmplt(_fmRxSmData.context.transportEventData.data,_fmRxSmData.context.transportEventData.dataLen);
                break;
            default:
                FMC_LOG_INFO((""));
        }
    }
    else
    {
    
        _fmRxSmData.interruptInfo.interruptInd = FMC_TRUE;
        fm_recvd_intterupt();
    }
    FMC_FUNC_END();
}

FMC_STATIC void fm_recvd_transportOnOffCmplt()
{
    _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;
        
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}

FMC_STATIC void fm_recvd_initCmdCmplt(void)
{   
    _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;

    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}

FMC_STATIC void fm_recvd_readCmdCmplt(FMC_U8 *data, FMC_UINT    dataLen)
{
    FMC_UNUSED_PARAMETER(dataLen);
    if(_fmRxSmData.fmRxReadState ==FM_RX_NONE)
    {
        _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;

        _fmRxSmData.context.transportEventData.read_param = FMC_UTILS_BEtoHost16(data);
    }
    else if (_fmRxSmData.fmRxReadState ==FM_RX_READING_RDS_DATA)
    {
        _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;
    }
    else if(_fmRxSmData.fmRxReadState ==FM_RX_READING_STATUS_REGISTER)
    {
        _fmRxSmData.interruptInfo.intReadEvt = FMC_TRUE;
        _fmRxSmData.interruptInfo.fmFlag = FMC_UTILS_BEtoHost16(data);
    }
    _fmRxSmData.fmRxReadState = FM_RX_NONE;
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}

FMC_STATIC void fm_recvd_writeCmdCmplt(void)
{   
    _fmRxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;

    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);
}


FMC_STATIC void fm_recvd_intterupt(void)
{
    /*  Locking of the Sem was removed to avoid deadlock when bts tries to get the fm sem and holds the stack sem
    *   and in the same time the fm stack tries to send command and therfore tries to lock the stack sem
    */
    
    FMCI_NotifyFmTask(FMC_OS_EVENT_FMC_STACK_TASK_PROCESS);

}
/*
    This is a comparison function that is used when searching the commands queue for command of a specific
    command type.
    
    See FM_RX_SM_FindLatestCmdInQueueByType() for details
*/
FMC_BOOL _FM_RX_SM_CompareCmdTypes(const FMC_ListNode *entryToMatch, const FMC_ListNode* checkedEntry)
{
    const FmcBaseCmd *cmdToMatch = (FmcBaseCmd*)entryToMatch;
    const FmcBaseCmd *checkedCmdEntry = (FmcBaseCmd*)checkedEntry;

    /* Declare equality when command types are equal */
    if (cmdToMatch->cmdType == checkedCmdEntry->cmdType)
    {
        return FMC_TRUE;
    }
    else
    {
        return FMC_FALSE;
    }
}

/*
    Traverse the commands queue from the last command to the first, and search for a command object with
    the specified command type 
*/
FmcBaseCmd *FM_RX_SM_FindLatestCmdInQueueByType(FmRxCmdType cmdType)
{
    FmcBaseCmd      templateEntry;
    FMC_ListNode        *matchingEntryAsListEntry = NULL;

    FMC_FUNC_START("FM_RX_SM_FindLatestCmdInQueueByType");

    /* Init the template entry so that the entry with the matching cmd type will be searched for */
    templateEntry.cmdType = cmdType;
    
    /* Seach the commands queue for the last entry */
    matchingEntryAsListEntry = FMC_UTILS_FindMatchingListNode(  FMCI_GetCmdsQueue(),                /* Head */
                                            NULL,                               /* Start from last node */
                                            FMC_UTILS_SEARCH_LAST_TO_FIRST, /* Search for last entry */
                                            &(templateEntry.node),
                                            _FM_RX_SM_CompareCmdTypes);
    
    FMC_FUNC_END();
    
    return  (FmcBaseCmd*)matchingEntryAsListEntry;
}

/*
    Check if a command of the specified type is pending execution
*/
FMC_BOOL FM_RX_SM_IsCmdPending(FmRxCmdType cmdType)
{
    FmcBaseCmd *pendingCmd = FM_RX_SM_FindLatestCmdInQueueByType(cmdType);

    if (pendingCmd != NULL)
    {
        return FMC_TRUE;
    }
    else
    {
        return FMC_FALSE;
    }
}

/*
 * Return TRUE if CompleteScan is in progress and we can actually stop it. 
 * at the end stages the scan cannot be stopped (anyway only few ms are left).
 * Stopping the Scan at the final stages can cause a deadlock
 */
FMC_BOOL FM_RX_SM_IsCompleteScanStoppable()
{
	if(((FM_RX_SM_GetRunningCmd() == FM_RX_CMD_COMPLETE_SCAN) ||
	    (FM_RX_SM_IsCmdPending(FM_RX_CMD_COMPLETE_SCAN) == FMC_TRUE)) &&
		(_fmRxSmData.completeScanOp.curStage <= 4))
		return FMC_TRUE;
	else
		return FMC_FALSE;
}
/*
*   convertRxTargetsToCal()
*
*   ap the fm_rx.h target types to the cal types - resources
*/
ECAL_ResourceMask _convertRxTargetsToCal(FmRxAudioTargetMask targetMask,TCAL_ResourceProperties *pProperties)
{
    ECAL_ResourceMask resourceMask = CAL_RESOURCE_MASK_NONE; 

    /*get the properties for according to the targets*/
    _getResourceProperties(targetMask,pProperties);

    if( targetMask&FM_RX_TARGET_MASK_I2SH)
    {
        resourceMask|= CAL_RESOURCE_MASK_I2SH;
    }
    else if(targetMask&FM_RX_TARGET_MASK_PCMH)
    {
        resourceMask|= CAL_RESOURCE_MASK_PCMH;
    }
    if( targetMask&FM_RX_TARGET_MASK_FM_ANALOG)
    {
        resourceMask|= CAL_RESOURCE_MASK_FM_ANALOG;
    }
    
    return resourceMask;
}
void _getResourceProperties(FmRxAudioTargetMask targetMask,TCAL_ResourceProperties *pProperties)
{
    /* in the CAL properties will be checked befor setting the fm IF - 
    *   1 will be used to enable digital in I2S protocol mode.
    *   0 will be used to enable digital in pcm protocol mode.
    */
    if( targetMask&FM_RX_TARGET_MASK_I2SH)
    {
        pProperties->tResourcePorpeties[0] = 1;
        pProperties->uPropertiesNumber = 1;
    }
    else if(targetMask&FM_RX_TARGET_MASK_PCMH)
    {
        pProperties->tResourcePorpeties[0] = 0;
        pProperties->uPropertiesNumber = 1;
    }
    else if(targetMask&FM_RX_TARGET_MASK_FM_OVER_SCO)
    {
        pProperties->tResourcePorpeties[0] = 1;
        pProperties->uPropertiesNumber = 1;
    }
    else if(targetMask&FM_RX_TARGET_MASK_FM_OVER_A3DP)
    {
        pProperties->tResourcePorpeties[0] = 1;
        pProperties->uPropertiesNumber = 1;
    }
}
FMC_U32 _getRxChannelNumber(void)
{
    if(_fmRxSmData.vacParams.monoStereoMode== FM_RX_MONO_MODE)
        return 1;
    else if(_fmRxSmData.vacParams.monoStereoMode == FM_RX_STEREO_MODE)
        return 2;
    else/*Invalid state*/
        return 0;
}

FMC_U16 _getRxAudioEnableParam(void)
{
    FMC_U16 enableAudioFwParam = 0;
    if((_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_I2SH)||(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_PCMH)||
        (_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_SCO)||(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_OVER_A3DP))
    {
        /* enable the fm digital interface*/
        enableAudioFwParam|= 1;
    }
    if(_fmRxSmData.vacParams.audioTargetsMask&FM_RX_TARGET_MASK_FM_ANALOG)
    {
        /*enable the analog interface*/
        enableAudioFwParam|= 2;
    }
    return enableAudioFwParam;
}
void _goToLastStageAndFinishOp(void)
{
    /* set stage index to be the size of operations array -1 and go to this operation */
    _fmRxSmData.currCmdInfo.stageIndex = (FMC_U8)(fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].numOfCmd -1);
    fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
}
void _handleVacOpStateAndMove2NextStage(ECCM_VAC_Status vacStatus)
{
    /* if recieved pending from the vac wait for command complete and then move to next stage*/
    if (vacStatus == CCM_VAC_STATUS_PENDING) 
    {   /* wait for vac cc event- which will be simulated as a transport event*/
        prepareNextStage(_FM_RX_SM_STATE_WAITING_FOR_CC, INCREMENT_STAGE);      
    
    }
    else
    {
        /* if success move to next stage*/
        if(vacStatus == CCM_VAC_STATUS_SUCCESS)
        {
            /* since vac cb simulates transport callback - simulate success in transport callback*/
            _fmRxSmData.context.transportEventData.status = FMC_STATUS_SUCCESS;
            /*go directy to the next operation handler*/    
            prepareNextStage(_FM_RX_SM_STATE_NONE, INCREMENT_STAGE);    
            fmOpAllHandlersArray[_fmRxSmData.currCmdInfo.baseCmd->cmdType].opHandlerArray[_fmRxSmData.currCmdInfo.stageIndex]();
        }
        else if(vacStatus == CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES)
        {
            /* set the operation status to unavail res and finish the operation*/
            _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES;
            _goToLastStageAndFinishOp();
        }
    else if(vacStatus == CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED)
    {
        /* set the operation status to unavail res and finish the operation*/
        _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_NOT_APPLICABLE;
        _goToLastStageAndFinishOp();
    }
        else
        {
            /* set the operation status to internal error and finish the operation*/
            _fmRxSmData.currCmdInfo.status = FM_RX_STATUS_INTERNAL_ERROR;
            _goToLastStageAndFinishOp();
        }
    }
}

FMC_U16 _getGainValue(FMC_UINT  gain)
{
/*Moved the implementation to HAL layer*/
return MCP_HAL_MISC_GetGainValue(gain);
}


 void send_fm_error_statusToApp()
{

	FmRxStatus      status = FM_RX_STATUS_INTERNAL_ERROR ;
	/*The callback will be passed to the APP layer only if FM Context is available*/
	if(FM_RX_SM_GetContextState() ==  FM_RX_SM_CONTEXT_STATE_DESTROYED)
	{
		FM_TRACE("send_fm_error_statusToApp -no context");
	}
	else
	{
		FM_TRACE("send_fm_error_statusToApp");
		/*Call the FM_RX_Error callback function*/
		(fmRxErrorAppCallback)(status);

	}
}



#endif /*FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED*/


