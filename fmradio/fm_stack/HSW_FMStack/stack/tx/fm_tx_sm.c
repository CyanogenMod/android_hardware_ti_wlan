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


#include "fmc_defs.h"
#include "fm_txi.h"
#include "fm_tx_sm.h"
#include "fmc_core.h"
#include "fmc_debug.h"
#include "fmc_log.h"
#include "fmc_fw_defs.h"
#include "fmc_config.h"
#include "fmc_utils.h"
#include "fmc_log.h"
#include "mcp_bts_script_processor.h"
#include "mcp_rom_scripts_db.h"
#include "mcp_hal_string.h"
#include "mcp_unicode.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMTXSM);

typedef enum {
    _FM_TX_SM_EVENT_START,
    _FM_TX_SM_EVENT_TRANSPORT_EVENT,
    _FM_TX_SM_EVENT_SCRIPT_EXEC_COMPLETE,
    _FM_TX_SM_EVENT_CONTINUE
} _FmTxSmEvent;

typedef enum {
    _FM_TX_SM_STATE_NONE,
    _FM_TX_SM_STATE_RUNNING_PROCESS,    
    _FM_TX_SM_STATE_WAITING_FOR_CC,
    _FM_TX_SM_STATE_WAITING_FOR_INTERRUPT,
}_FmTxSmState;
/*
    A local copy of the latest transport event.

    It is populated when a transport event is received. The event is received in the context 
    of the transport, but is processed in the context of the FM stack. It is kept in this structure 
    waiting for processing.
*/
typedef struct
{
    /* 
        Indicates if a transport event has been received but not processed yet.
        This is useful for:
        1. Debuggging - verifying that when an event arrives the previous event has
            already been processed
        2. Makes sure that when the main event processor (_FM_TX_SM_ProcessEvents())
            is called, we will not attempt to process a transport event that has already 
            been processed
    */
    FMC_BOOL                eventWaitsForProcessing;
    FmcCoreEventType    type;
    FmcStatus               status;
    
    /* [ToDo] Specify the max length using some meaningful symbolic value (probably use some fmc_core.h constant) */
    /* The size of data should be related to the max size of HCI event*/
    FMC_U8                  data[100];
    FMC_U8                  dataLen;
} _FmTxSmTransportEventData;
typedef struct
{
    /*VAC related fields*/
    FmTxAudioSource             audioSource;
    ECAL_SampleFrequency eSampleFreq;
    FmTxMonoStereoMode      monoStereoMode;
    TCCM_VAC_UnavailResourceList  ptUnavailResources;
    
}_FmTxSmVacParams;
/*
    Cache of values that have been configured in the chip, but there is a reason
    to duplicate their values in the host's RAM. Values that should be kept in the 
    cache are:
    1. Write-only values (can't be read from the chip).
    2. Values that are needed synchronously (e.g., for argument verfication)

    The cache is updated whenever a cache value successfully completes its setting 
    in the chip (except for the PS / RT fields)
*/
typedef struct
{
    FMC_U16         fwVersion;
    FMC_U16             asicId;
    FMC_U16             asicVersion;
    
    FmcMuteMode     muteMode;
    FMC_BOOL        transmissionOn;
    FMC_BOOL        rdsEnabled;
    FmTxRdsTransmissionMode transmissionMode;
    FmcFreq         tunedFreq;
    FmcRdsExtendedCountryCode   countryCode;
    FmTxRdsTransmittedGroupsMask    fieldsMask;

    FmcEmphasisFilter   emphasisfilter;

    FmcAfCode       afCode;     

    FmcRdsPtyCode   rdsPtyCode;

    FmcRdsPiCode    rdsPiCode;

    FmcRdsRepertoire    rdsRepertoire;

    FmcRdsPsScrollSpeed rdsScrollSpeed;

    FmcRdsPsDisplayMode rdsScrollMode;

    FmcRdsTaCode        rdsTaCode;

    FmcRdsTpCode        rdsTpCode;

    FmcRdsMusicSpeechFlag   rdsRusicSpeachFlag;

    FmTxPowerLevel          powerLevel;

    
    /* 
        Stores the latest PS text message that was specified in the call to FM_TX_SetRdsTextPsMsg()

        The actual text that is set in the RDS FIFO depends on the transmitted fields mask (fieldsMask)
    */
    FMC_U8              psMsg[FM_RDS_PS_MAX_MSG_LEN + 1];

    /* 
        Stores the latest RT text type & message that was specified in the call to FM_TX_SetRdsTextRtMsg()

        The actual text that is set in the RDS FIFO depends on the transmitted fields mask (fieldsMask)
    */
    FmcRdsRtMsgType rtType;
    FMC_U8              rtMsg[FM_RDS_RT_B_MAX_MSG_LEN + 1];

    /* 
        Stores the latest Raw Data
    */
    FMC_U8              rawData[FM_RDS_RAW_MAX_MSG_LEN + 1];
    
} _FmTxSmFwCache;

 struct  _FmTxContext{
    FmTxSmContextState  state;

    /* Added 1 for terminating NULL string char */
    FMC_U8              rdsRawDataCmdBuf[sizeof(FmTxSetRdsRawDataCmd) + 1];
    FMC_BOOL            rdsRawDataCmdBufAllocated;

    FMC_U8              rdsRtCmdBuf[sizeof(FmTxSetRdsTextRtMsgCmd)+1];
    FMC_BOOL            rdsRtCmdBufAllocated;

    FMC_U8              rdsPsCmdBuf[sizeof(FmTxSetRdsTextPsMsgCmd)  + 1];
    FMC_BOOL            rdsPsCmdBufAllocated;

    _FmTxSmTransportEventData   transportEventData;

    /* Script Processor context that is used when executing various scripts */
    McpBtsSpContext         tiSpContext;

    /* The "application" callback function */
    FmTxCallBack            appCb;
    
    /* Event that is sent to the user of the FM TX API ("application") */
    FmTxEvent           appEvent;

    _FmTxSmFwCache      fwCache;
    
    /* 
        transmission must be stopped when setting the channel (tuning). It is possible
        to call FM_TX_Tune() when transmission is on. In that case, transmission is 
        stopped and resumed internally. This flag indicates whether this occurred.
    */
    FMC_BOOL            transmissionStoppedForTune;


    /* Variables that are used to segment a long RDS data set command to chunks */
    FMC_U8*             rdsText;
    FMC_U8          rdsTextLen;
    /*
        0 - Manual Mode
        1 - PS Mode
        2 - RT Automatic
        3 - RT A
        4 - RT B
    */
    FMC_U8          rdsGroupText;
    
    FMC_UINT            rdsChunkSize;
    FMC_UINT            numOfSetRdsChars;

    _FmTxSmVacParams vacParams;

} ;

typedef void (*_FmTxSmCmdStageHandler)(FMC_UINT event, void *eventData);

typedef FmcFwCmdParmsValue (*_FmTxSmSetCmdGetCmdParmValueFunc)(void);

typedef void (*_FmTxSmSetCmdCompleteFunc)(void);

typedef void (*_FmTxSmCmdCompletedFillEventDataCb)(FmTxEvent *event, void *eventData);
    
typedef struct
{
    FMC_UINT                stageType;
    _FmTxSmCmdStageHandler  stageHandler;
} _FmTxSmCmdStageInfo;

typedef _FmTxSmCmdStageInfo *_FmTxSmCmdProcessor;

typedef struct
{
    _FmTxSmCmdProcessor     cmdProcessor;
    FMC_UINT                numOfStages;
} _FmTxSmCmdInfo;

FMC_STATIC _FmTxSmCmdInfo* _fmTxSmCmdsTable[FM_TX_LAST_CMD_TYPE + 1];

FMC_STATIC const FMC_UINT _fmTxSmNumOfCmdsInCmdsTable = sizeof(_fmTxSmCmdsTable) / sizeof(_FmTxSmCmdInfo);

FMC_STATIC FmcFwOpcode                              _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_LAST_CMD_TYPE + 1];
FMC_STATIC _FmTxSmSetCmdGetCmdParmValueFunc     _fmTxSmSetCmdGetCmdParmValue[FM_TX_LAST_CMD_TYPE + 1];
FMC_STATIC _FmTxSmSetCmdCompleteFunc            _fmTxSmSetCmdCompleteMap[FM_TX_LAST_CMD_TYPE + 1];
FMC_STATIC _FmTxSmCmdCompletedFillEventDataCb           _fmTxSmSetCmdFillEventDataMap[FM_TX_LAST_CMD_TYPE + 1];

FMC_STATIC FmcFwOpcode                              _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_LAST_CMD_TYPE + 1];
FMC_STATIC _FmTxSmCmdCompletedFillEventDataCb           _fmTxSmSimpleGetCmdFillEventDataMap[FM_TX_LAST_CMD_TYPE + 1];


typedef struct {
    FmcBaseCmd      *baseCmd;
    FMC_UINT        stageIndex;
    FmTxStatus      status;
    _FmTxSmState smState;
} _FmTxSmCurrCmdInfo;

typedef struct _tagFmTxSmData {
    FmTxContext         context;
    _FmTxSmCurrCmdInfo  currCmdInfo;
} _FmTxSmData;

FMC_STATIC _FmTxSmData  _fmTxSmData;

typedef struct {
    /* was the interrupt mask indicated by intMask enabled? */
    FMC_BOOL intMaskEnabled;
    /* was interrupt recieved but not read? (in the case we where waiting for command complete) */
    FMC_BOOL intRecievedButNotRead;
    /* started reading interrupt's mask and waiting for read complete
    */
    FMC_BOOL waitingForReadIntComplete;
    /*  At any point in the time should hold the mask of interrupts we are interested in recieving.
    *   This value should be updated at 2 points:
    *       1.In the init to indicat the general interrupts we are interested in (like MAL).
    *       2.
    *
    */
    FmcFwIntMask intMask;
    /*  Saves the interrupts recieved and not yet handled. If an interrupt handler is inserted to the list it is removed from this mask
    *   The value should be set with the mask recieved after reading the interrupt status register.
    *   An Interrupt(relevant bit) should be cleaned in the following cases:
    *       1.This is an interrupt we are waiting for - then remove from the mask the relevant interrupt.
    *       2.For general Interrupts -the relevant bit should be removed after inserting the interrupt handler to the command queue(relevant for RX interrupts).
    *       3.The interrupt handler is elready in the queue.    
    */
    FmcFwIntMask intMaskReadButNotHandled;

    /*  This the interrupt we are waiting for - the current process is pending and waites for this interrupt to be recieved. 
    *   The value should be set befor setting the interrupt mask with this interrupt.
    *   The fieled should be cleaned after recieving the interrupt, or in the case of timout when the timout will be recieved.
    */
    FmcFwIntMask intWaitingFor;
    
    FmcFwIntMask intRecieved;

}_FmTxSmInterrupts;

FMC_STATIC _FmTxSmInterrupts _fmTxSmIntMask;

FMC_STATIC void _FM_TX_SM_ProcessEvents(void);

FMC_STATIC void _FM_TX_SM_TccmVacCb(ECAL_Operation eOperation,ECCM_VAC_Event eEvent, ECCM_VAC_Status eStatus);

FMC_STATIC void _FM_TX_SM_TransportEventCb(const FmcCoreEvent *eventParms);

FMC_STATIC void _FM_TX_SM_Dispatch(_FmTxSmEvent  smEvent, void *eventData);
FMC_STATIC void _FM_TX_SM_CheckInterruptsAndThenDispatch(_FmTxSmEvent    smEvent, void *eventData);

FMC_STATIC _FmTxSmCmdStageHandler _FM_TX_SM_GetStageHandler(FmTxCmdType cmdType, FMC_UINT stageIndex);
FMC_STATIC  FmcFwIntMask _FM_TX_SM_HandlerInterrupt_GetInterruptStatusRegCmd( void *eventData);

FMC_STATIC  void _FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd( void);

FMC_UINT _getIndexOfLastStage(void);


/********************************************************************************************************
        VAC related Utility functions 
*********************************************************************************************************/
void _FM_TX_SM_HandleVacStatusAndMove2NextStage(ECCM_VAC_Status vacStatus,void *eventData);

ECAL_ResourceMask _convertTxSourceToCal(FmTxAudioSource source,TCAL_ResourceProperties *pProperties);
FMC_U32 _getChannelNumber(FmTxMonoStereoMode        monoStereoMode);
FMC_U16 _getAudioIoParam(FmTxAudioSource            audioSource);

/*
########################################################################################################################
    ENABLE Command Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerEnable_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_SendFmOnCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_SendReadFmAsicIdCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_WaitReadFmAsicIdCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_SendReadFmAsicVersionCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_WaitReadFmAsicVersionCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_StartFmInitScriptExec( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_WaitFmInitScriptExecComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_StartFmTxInitScriptExec( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_WaitFmTxInitScriptExecComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_SendFmTxOnCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_ApplyDfltConfig( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerEnable_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    DISABLE Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerDisable_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerDisable_SendFmOffCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerDisable_SendTransportFmOffCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerDisable_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    TUNE Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerTune_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_SendSetInterruptMaskCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_SendTuneCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_WaitTuneCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_WaitTunedInterrupt( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_SendInterruptMaskSetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_WaitInterruptMaskSetCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTune_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Start Transmission Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_VacStartOperationCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_SendAudioIoSetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_SendPowerLevelSetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_WaitPowerLevelSetCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_SendTransmissionOnCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    StopTransmission Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerStopTransmission_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStopTransmission_SendTransmissionOffCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStopTransmission_WaitTransmissionOffCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerStopTransmission_Complete( FMC_UINT event, void *eventData);

/*
########################################################################################################################
    Set/Get Transmission Mode
########################################################################################################################
*/


FMC_STATIC void _FM_TX_SM_HandlerTransmissionMode_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsPsMsgCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmissionMode_SendSetRdsEccMsgCmd   ( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsEccMsgCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmissionMode_Complete( FMC_UINT event, void *eventData);
/*
    Get Transmission Mode
*/
FMC_STATIC void _FM_TX_SM_HandlerGetRdsTransmissionModeCmd_Start( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Set Transmission Field Mask
########################################################################################################################
*/

FMC_STATIC void _FM_TX_SM_HandlerTransmitMask_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmitMask_WaitSetRdsPsMsgCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmitMask_SendSetRdsEccMsgCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerTransmitMask_Complete( FMC_UINT event, void *eventData);
/*
    GetTransmission Field Mask
*/
FMC_STATIC void _FM_TX_SM_HandlerGetRdsTransmissionFieldMaskCmd_Start( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Set traffic codes Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerSetTrafficCodes_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetTrafficCodes_SendTaSetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetTrafficCodes_SendTpSetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetTrafficCodes_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Get traffic codes Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerGetTrafficCodes_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGetTrafficCodes_SendTaGetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGetTrafficCodes_SendTpGetCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGetTrafficCodes_Complete( FMC_UINT event, void *eventData);


/*
########################################################################################################################
    Change Audio Resources Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_SendEnableAudioSource( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Change digital audio configuration Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendChangeConfigurationCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_ChangeDigitalAudioConfigCmdComplete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendEnableAudioSource( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Change mono stereo mode
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_SendChangeConfigurationCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_SetMonoModeCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_Complete( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Simple SET Commands Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerSimpleSetCmd_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSimpleSetCmd_Complete( FMC_UINT event, void *eventData);

/*
########################################################################################################################
    Simple GET Commands Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerSimpleGetCmd_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSimpleGetCmd_Complete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_SimpleGetCmd_FillEventData(FmTxEvent *event, void *eventData);


/*
########################################################################################################################
    Set RDS Text PS Message Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Complete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_RdsTextPsMsg_FillEventData(FmTxEvent *event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerGetRdsTextPsMsgCmd_Start( FMC_UINT event, void *eventData);

/*
########################################################################################################################
    Set RDS Text RT Message Command HAndlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Complete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_RdsTextRtMsg_FillEventData(FmTxEvent *event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerGetRdsTextRtMsgCmd_Start( FMC_UINT event, void *eventData);
/*
########################################################################################################################
    Set RDS raw data
########################################################################################################################
*/

FMC_STATIC void _FM_TX_SM_HandlerSetRdsRawDataCmd_Start( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsRawDataCmd_Complete( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_RdsRawData_FillEventData(FmTxEvent *event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerGetRdsRawDataCmd_Start( FMC_UINT event, void *eventData);

/*
########################################################################################################################
    Simple Set Commands Get Parameter Handlers
########################################################################################################################
*/
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetInterruptRegister_GetCmdParmValue(void);

FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerTune_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetPowerLevel_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetMuteMode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerStartTransmission_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerStopTransmission_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerEnableRds_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerDisableRds_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextScrollMode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextScrollSpeed_GetCmdParmValue(void);

FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTransmittedMask_GetCmdParmValue(void);

FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetPreEmphasisFilter_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTransmissionMode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsAfCode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsPiCode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsPtyCode_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextRepertoire_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_GetCmdParmValue(void);
FMC_STATIC FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsExtendedCountryCode_GetCmdParmValue(void);
/*
########################################################################################################################
    Simple Set Commands Update Cache Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerTune_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetMuteMode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_CmdComplete(void);
FMC_STATIC void _FM_TX_SM_HandlerStopTransmission_CmdComplete(void);
FMC_STATIC void _FM_TX_SM_HandlerEnableRds_UpdateCache(void);
FMC_STATIC void  _FM_TX_SM_HandlerPowerLvl_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerDisableRds_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsRTransmittedMask_UpdateCache(void);

FMC_STATIC void _FM_TX_SM_HandlerSetRdsPsDisplayMode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPsScrollSpeed_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTrafficCodes_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPsMsg_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsRtMsg_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsRawData_UpdateCache(void);

FMC_STATIC void _FM_TX_SM_HandlerSetPreEmphasisFilter_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTransmissionMode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsAfCode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPiCode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPtyCode_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextRepertoire_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsExtendedCountryCode_UpdateCache(void);

FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_UpdateCache(void);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_UpdateCache(void);

FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_UpdateCache(void);
/*
########################################################################################################################
    Simple Set Commands Fill Event Data Handlers
########################################################################################################################
*/
FMC_STATIC void _FM_TX_SM_HandlerTune_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetPowerLevel_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetMuteMode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextScrollMode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsRTransmittedMask_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPsScrollSpeed_FillEventData(FmTxEvent *event, void *eventData); 
 
FMC_STATIC void _FM_TX_SM_HandlerSetPreEmphasisFilter_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetECC_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTransmissionMode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsAfCode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPiCode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPtyCode_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTextRepertoire_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsPsScrollSpeed_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerSetRdsTrafficCodes_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGetRdsTrafficCodes_FillEventData(FmTxEvent *event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerStartTransmission_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeAudioSource_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeDigitalAudioConfig_FillEventData(FmTxEvent *event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerChangeMonoStereoMode_FillEventData(FmTxEvent *event, void *eventData);
/*
########################################################################################################################

########################################################################################################################
*/
FMC_STATIC McpBtsSpStatus _FM_TX_SM_TiSpSendHciScriptCmd(   McpBtsSpContext *context,
                                                            McpU16  hciOpcode, 
                                                            McpU8   *hciCmdParms, 
                                                            McpUint len);

FMC_STATIC  void _FM_TX_SM_TiSpExecuteCompleteCb(McpBtsSpContext *context, McpBtsSpStatus status);


FMC_STATIC void _FM_TX_SM_InitCmdsTable(void);
FMC_STATIC void _FM_TX_SM_InitSimpleSetCmd2FwOpcodeMap(void);
FMC_STATIC void _FM_TX_SM_InitSetCmdGetCmdParmValueMap(void);
FMC_STATIC void _FM_TX_SM_InitSetCmdCompleteMap(void);
FMC_STATIC void _FM_TX_SM_InitSetCmdFillEventDataMap(void);

FMC_STATIC void _FM_TX_SM_InitSimpleGetCmd2FwOpcodeMap(void);

FMC_STATIC FmTxStatus _FM_TX_SM_HandleCompletionOfCurrCmd(  _FmTxSmSetCmdCompleteFunc   cmdCompleteFunc ,
                                                            _FmTxSmCmdCompletedFillEventDataCb fillEventDataCb, 
                                                            void *eventData);


FMC_STATIC void _FM_TX_SM_SendAppCmdCompleteEvent(  FmTxContext                         *context, 
                                                                FmTxStatus                      completionStatus,
                                                                FmTxCmdType                 cmdType);

FMC_STATIC FmTxStatus _FM_TX_SM_RemoveFromQueueAndFreeCmd(FmcBaseCmd    **cmd);



FMC_STATIC FmTxStatus _FM_TX_SM_AllocateCmd(    FmTxContext     *context,
                                                FmTxCmdType cmdType,
                                                FmcBaseCmd      **cmd);

FMC_STATIC FmTxStatus _FM_TX_SM_FreeCmd( FmcBaseCmd **cmd);

FMC_STATIC void _FM_TX_SM_UpdateState(_FmTxSmState state);
FMC_STATIC _FmTxSmState _FM_TX_SM_getState(void);

FMC_STATIC FmcStatus _FM_TX_SM_UptadeSmStateSendWriteCommand(FmcFwOpcode fmOpcode, FMC_U32 fmCmdParms);
FMC_STATIC FmcStatus _FM_TX_SM_UptadeSmStateSendReadCommand(FmcFwOpcode fmOpcode);
FMC_STATIC void _FM_TX_SM_HandleRecievedInterrupts(FmcFwIntMask mask);
FMC_STATIC void _FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd( void);
/*
########################################################################################################################
########################################################################################################################
########################################################################################################################
########################################################################################################################
*/

FMC_STATIC void _FM_TX_SM_InitDefualtConfigValues(void);

FMC_STATIC void _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue( FMC_UINT event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerGeneral_SendSetIntMaskCmd( FmcFwIntMask mask,FMC_UINT event, void *eventData);

FMC_STATIC void _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt( FMC_UINT event, void *eventData);

/*
    Send Text general sequence
*/
FMC_STATIC void _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd( FMC_UINT event, void *eventData);
FMC_STATIC void _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd( FMC_UINT event, void *eventData);

/*
    End -Send Text general sequence
*/
/*##################################################################################################################################################################################*/






/*########################################################################################################################
    Enable Process Handlers
########################################################################################################################*/

enum {
    _FM_TX_SM_STAGE_ENABLE_START,
    _FM_TX_SM_STAGE_ENABLE_WAIT_TRANSPORT_FM_ON,
    _FM_TX_SM_STAGE_ENABLE_SEND_FM_ON_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_ON_CMD_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_SEND_FM_READ_ASIC_ID_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_READ_ASIC_ID_CMD_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_SEND_FM_READ_ASIC_VERSION_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_READ_ASIC_VERSION_CMD_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_START_FM_INIT_SCRIPT_EXEC,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_INIT_SCRIPT_EXEC_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_START_FM_TX_INIT_SCRIPT_EXEC,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_TX_INIT_SCRIPT_EXEC_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_SEND_FM_TX_ON_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_FM_TX_ON_CMD_COMPLETE,
    _FM_TX_SM_STAGE_ENABLE_APPLY_DFLT_CONFIG,
    _FM_TX_SM_STAGE_ENABLE_SEND_SET_RDS_TEXT_RT_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_SET_RDS_TEXT_RT_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_ENABLE_SEND_SET_RDS_TEXT_RT_MSG_CMD,
    _FM_TX_SM_STAGE_ENABLE_WAIT_SET_RDS_TEXT_RT_MSG_CMD,
    _FM_TX_SM_STAGE_ENABLE_COMPLETE
 } _FmTxSmStageType_Enable;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_Enable[] =  {
    {_FM_TX_SM_STAGE_ENABLE_START,                              _FM_TX_SM_HandlerEnable_Start},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_TRANSPORT_FM_ON,               _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_ENABLE_SEND_FM_ON_CMD,                         _FM_TX_SM_HandlerEnable_SendFmOnCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_ON_CMD_COMPLETE,            _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},/*[ToDo Zvi] when the version will be read as it should general command can be used*/
    {_FM_TX_SM_STAGE_ENABLE_SEND_FM_READ_ASIC_ID_CMD,           _FM_TX_SM_HandlerEnable_SendReadFmAsicIdCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_READ_ASIC_ID_CMD_COMPLETE,  _FM_TX_SM_HandlerEnable_WaitReadFmAsicIdCmdComplete},
    {_FM_TX_SM_STAGE_ENABLE_SEND_FM_READ_ASIC_VERSION_CMD,          _FM_TX_SM_HandlerEnable_SendReadFmAsicVersionCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_READ_ASIC_VERSION_CMD_COMPLETE, _FM_TX_SM_HandlerEnable_WaitReadFmAsicVersionCmdComplete},
    {_FM_TX_SM_STAGE_ENABLE_START_FM_INIT_SCRIPT_EXEC,          _FM_TX_SM_HandlerEnable_StartFmInitScriptExec},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_INIT_SCRIPT_EXEC_COMPLETE,  _FM_TX_SM_HandlerEnable_WaitFmInitScriptExecComplete},
    {_FM_TX_SM_STAGE_ENABLE_START_FM_TX_INIT_SCRIPT_EXEC,           _FM_TX_SM_HandlerEnable_StartFmTxInitScriptExec},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_TX_INIT_SCRIPT_EXEC_COMPLETE,   _FM_TX_SM_HandlerEnable_WaitFmTxInitScriptExecComplete},
    {_FM_TX_SM_STAGE_ENABLE_SEND_FM_TX_ON_CMD,                  _FM_TX_SM_HandlerEnable_SendFmTxOnCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_FM_TX_ON_CMD_COMPLETE,             _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_ENABLE_APPLY_DFLT_CONFIG,                      _FM_TX_SM_HandlerEnable_ApplyDfltConfig},
    {_FM_TX_SM_STAGE_ENABLE_SEND_SET_RDS_TEXT_RT_MSG_LEN_CMD,   _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_SET_RDS_TEXT_RT_MSG_LEN_CMD,   _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_ENABLE_SEND_SET_RDS_TEXT_RT_MSG_CMD,       _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_ENABLE_WAIT_SET_RDS_TEXT_RT_MSG_CMD,       _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_ENABLE_COMPLETE,                               _FM_TX_SM_HandlerEnable_Complete}
};
FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_Enable = { _fmTxSmCmdProcessor_Enable, 
                                                sizeof(_fmTxSmCmdProcessor_Enable) / sizeof(_FmTxSmCmdStageInfo)};

/******************************************************************************************************************************************************
*   These simple commands Defualt values will be set in the enable process
*******************************************************************************************************************************************************/
typedef struct
{
    FmTxCmdType cmdType;
    FMC_UINT    defualtValue;
    
}_FmTxSmDefualtConfigValue;

FMC_STATIC _FmTxSmDefualtConfigValue _fmTxEnableDefualtSimpleCommandsToSet[] = {
    {FM_TX_CMD_SET_POWER_LEVEL, FMC_CONFIG_TX_DEFUALT_POWER_LEVEL},
    {FM_TX_CMD_SET_MUTE_MODE,FMC_CONFIG_TX_DEFUALT_MUTE_MODE},
    {FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE,FMC_CONFIG_TX_DEFUALT_RDS_TEXT_SCROLL_MODE},
    {FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED,FMC_CONFIG_TX_DEFUALT_RDS_PS_DISPLAY_SPEED},
    {FM_TX_CMD_SET_RDS_PI_CODE,FMC_CONFIG_TX_DEFAULT_RDS_PI_CODE},
    {FM_TX_CMD_SET_MONO_STEREO_MODE,FMC_CONFIG_TX_DEFUALT_STEREO_MONO_MODE},
    {FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE,FMC_CONFIG_TX_DEFUALT_REPERTOIRE_CODE_TABLE},
    {FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG,FMC_CONFIG_TX_DEFUALT_MUSIC_SPEECH_FLAG},
    {FM_TX_CMD_SET_PRE_EMPHASIS_FILTER,FMC_CONFIG_TX_DEFUALT_PRE_EMPHASIS_FILTER}
};
    


/*
########################################################################################################################
    Disable
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_DISABLE_START,
    _FM_TX_SM_STAGE_DISABLE_SEND_FM_OFF_CMD,
    _FM_TX_SM_STAGE_DISABLE_WAIT_FM_OFF_CMD_COMPLETE,
    _FM_TX_SM_STAGE_DISABLE_SEND_TRANSPORT_FM_OFF,
    _FM_TX_SM_STAGE_DISABLE_WAIT_TRANSPORT_FM_OFF,
    _FM_TX_SM_STAGE_DISABLE_COMPLETE
 } _FmTxSmStageType_Disable;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_Disable[] =  {
    {_FM_TX_SM_STAGE_DISABLE_START,                                 _FM_TX_SM_HandlerDisable_Start},
    {_FM_TX_SM_STAGE_DISABLE_SEND_FM_OFF_CMD,                   _FM_TX_SM_HandlerDisable_SendFmOffCmd},
    {_FM_TX_SM_STAGE_DISABLE_WAIT_FM_OFF_CMD_COMPLETE,          _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_DISABLE_SEND_TRANSPORT_FM_OFF,             _FM_TX_SM_HandlerDisable_SendTransportFmOffCmd},
    {_FM_TX_SM_STAGE_DISABLE_WAIT_TRANSPORT_FM_OFF,                 _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_DISABLE_COMPLETE,                          _FM_TX_SM_HandlerDisable_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_Disable = {    _fmTxSmCmdProcessor_Disable, 
                                                sizeof(_fmTxSmCmdProcessor_Disable) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Tune
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_TUNE_START,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_POW_ENB_INTERRUPT_MASK_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE,  
    _FM_TX_SM_STAGE_TUNE_SEND_TRANSMISSION_OFF_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_TRANSMISSION_OFF_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_WAIT_POW_ENB_INTERRUPT,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_INTERRUPT_MASK_CMD,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_INTERRUPT_MASK_CMD_COMPLETE,  
    _FM_TX_SM_STAGE_TUNE_SEND_TUNE_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_TUNE_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_WAIT_TUNED_INTERRUPT,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_MASK_CMD,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_MASK_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_SEND_AUDIO_IO_SET_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_AUDIO_IO_SET_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_SEND_POWER_LEVEL_SET_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_POWER_LEVEL_SET_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_SEND_SET_POW_ENB_2_INTERRUPT_MASK_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_SET_POW_ENB_2_INTERRUPT_MASK_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_SEND_TRANSMISSION_ON_CMD,
    _FM_TX_SM_STAGE_TUNE_WAIT_TRANSMISSION_ON_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TUNE_WAIT_POW_ENB_2_INTERRUPT,
    _FM_TX_SM_STAGE_TUNE_SEND_TUNE_SCRIPT,
    _FM_TX_SM_STAGE_TUNE_WAIT_TUNE_SCRIPT_COMPLETION,
    _FM_TX_SM_STAGE_TUNE_COMPLETE
 } _FmTxSmStageType_Tune;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_Tune[] =  {
    {_FM_TX_SM_STAGE_TUNE_START,                                    _FM_TX_SM_HandlerTune_Start},
/******************************************************************************************************************************************************************
*   Start Sub sequence of Stop transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_POW_ENB_INTERRUPT_MASK_CMD,          _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_TUNE_SEND_TRANSMISSION_OFF_CMD,            _FM_TX_SM_HandlerStopTransmission_SendTransmissionOffCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_TRANSMISSION_OFF_CMD_COMPLETE,   _FM_TX_SM_HandlerStopTransmission_WaitTransmissionOffCmdComplete},
    {_FM_TX_SM_STAGE_TUNE_WAIT_POW_ENB_INTERRUPT,               _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},
/******************************************************************************************************************************************************************
*   End Sub sequence of Stop transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_INTERRUPT_MASK_CMD,          _FM_TX_SM_HandlerTune_SendSetInterruptMaskCmd},
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_INTERRUPT_MASK_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_TUNE_SEND_TUNE_CMD,                            _FM_TX_SM_HandlerTune_SendTuneCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_TUNE_CMD_COMPLETE,               _FM_TX_SM_HandlerTune_WaitTuneCmdComplete},
    {_FM_TX_SM_STAGE_TUNE_WAIT_TUNED_INTERRUPT,                     _FM_TX_SM_HandlerTune_WaitTunedInterrupt},
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_MASK_CMD,                         _FM_TX_SM_HandlerTune_SendInterruptMaskSetCmd},
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_MASK_CMD_COMPLETE,           _FM_TX_SM_HandlerTune_WaitInterruptMaskSetCmdComplete},
/******************************************************************************************************************************************************************
*   Start Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_TUNE_SEND_AUDIO_IO_SET_CMD,                    _FM_TX_SM_HandlerStartTransmission_SendAudioIoSetCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_AUDIO_IO_SET_CMD_COMPLETE,       _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_TUNE_SEND_POWER_LEVEL_SET_CMD,                 _FM_TX_SM_HandlerStartTransmission_SendPowerLevelSetCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_POWER_LEVEL_SET_CMD_COMPLETE,    _FM_TX_SM_HandlerStartTransmission_WaitPowerLevelSetCmdComplete},
    {_FM_TX_SM_STAGE_TUNE_SEND_SET_POW_ENB_2_INTERRUPT_MASK_CMD,            _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_SET_POW_ENB_2_INTERRUPT_MASK_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_TUNE_SEND_TRANSMISSION_ON_CMD,             _FM_TX_SM_HandlerStartTransmission_SendTransmissionOnCmd},
    {_FM_TX_SM_STAGE_TUNE_WAIT_TRANSMISSION_ON_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_TUNE_WAIT_POW_ENB_2_INTERRUPT,                 _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},
/******************************************************************************************************************************************************************
*   End Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_TUNE_COMPLETE,                                 _FM_TX_SM_HandlerTune_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_Tune = {   _fmTxSmCmdProcessor_Tune, 
                                                sizeof(_fmTxSmCmdProcessor_Tune) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Start Transmission
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_START_TRANSMISSION_START,

    _FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_START_VAC_CMD_COMPLETE,
        
    _FM_TX_SM_STAGE_START_TRANSMISSION_SEND_AUDIO_IO_SET_CMD,
    _FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_AUDIO_IO_SET_CMD_COMPLETE,

    _FM_TX_SM_STAGE_START_TRANSMISSION_SEND_POWER_LEV_CMD,
    _FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_POWER_LEV_SET_CMD_COMPLETE,

    _FM_TX_SM_STAGE_START_TRANSMISSION_SET_POW_ENB_INTERRUPT_MASK_CMD,
    _FM_TX_SM_STAGE_START_TRANSMISSION_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE,

    _FM_TX_SM_STAGE_START_TRANSMISSION_SEND_TRANSMISSION_ON_CMD,
    _FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_TRANSMISSION_ON_CMD_COMPLETE,

    _FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_POW_ENB_INTERRUPT,

    _FM_TX_SM_STAGE_START_TRANSMISSION_COMPLETE
 } _FmTxSmStageType_StartTransmission;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_StartTransmission[] =  {
    {_FM_TX_SM_STAGE_START_TRANSMISSION_START,                                  _FM_TX_SM_HandlerStartTransmission_Start},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_START_VAC_CMD_COMPLETE,            _FM_TX_SM_HandlerStartTransmission_VacStartOperationCmdComplete},
/******************************************************************************************************************************************************************
*   Start Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_START_TRANSMISSION_SEND_AUDIO_IO_SET_CMD,                  _FM_TX_SM_HandlerStartTransmission_SendAudioIoSetCmd},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_AUDIO_IO_SET_CMD_COMPLETE,         _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},

    {_FM_TX_SM_STAGE_START_TRANSMISSION_SEND_POWER_LEV_CMD,                     _FM_TX_SM_HandlerStartTransmission_SendPowerLevelSetCmd},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_POWER_LEV_SET_CMD_COMPLETE,        _FM_TX_SM_HandlerStartTransmission_WaitPowerLevelSetCmdComplete},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_SET_POW_ENB_INTERRUPT_MASK_CMD,             _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_SEND_TRANSMISSION_ON_CMD,               _FM_TX_SM_HandlerStartTransmission_SendTransmissionOnCmd},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_TRANSMISSION_ON_CMD_COMPLETE,      _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_START_TRANSMISSION_WAIT_POW_ENB_INTERRUPT,                     _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},
/******************************************************************************************************************************************************************
*   End Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_START_TRANSMISSION_COMPLETE,                               _FM_TX_SM_HandlerStartTransmission_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_StartTransmission = {  _fmTxSmCmdProcessor_StartTransmission, 
                                                        sizeof(_fmTxSmCmdProcessor_StartTransmission) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Start Transmission
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_START,
        
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_SEND_SET_POW_ENB_INTERRUPT_MASK_CMD,
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE, 
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_SEND_TRANSMISSION_OFF_CMD,
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_TRANSMISSION_OFF_CMD_COMPLETE,
    _FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_POW_ENB_INTERRUPT,

    _FM_TX_SM_STAGE_STOP_TRANSMISSION_COMPLETE
 } _FmTxSmStageType_StopTransmission;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_StopTransmission[] =  {
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_START,                                   _FM_TX_SM_HandlerStopTransmission_Start},
/******************************************************************************************************************************************************************
*   Start Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_SEND_SET_POW_ENB_INTERRUPT_MASK_CMD,         _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd},
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_SET_POW_ENB_INTERRUPT_MASK_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_SEND_TRANSMISSION_OFF_CMD,           _FM_TX_SM_HandlerStopTransmission_SendTransmissionOffCmd},
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_TRANSMISSION_OFF_CMD_COMPLETE,  _FM_TX_SM_HandlerStopTransmission_WaitTransmissionOffCmdComplete},
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_WAIT_POW_ENB_INTERRUPT,              _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},
/******************************************************************************************************************************************************************
*   End Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
    {_FM_TX_SM_STAGE_STOP_TRANSMISSION_COMPLETE,                                _FM_TX_SM_HandlerStopTransmission_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_StopTransmission = {   _fmTxSmCmdProcessor_StopTransmission, 
                                                        sizeof(_fmTxSmCmdProcessor_StopTransmission) / sizeof(_FmTxSmCmdStageInfo)};

/*
########################################################################################################################
    Set Transmission Mode
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_START,

    _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,

    _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_TO_RT,


    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,


    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_TO_ECC,


    _FM_TX_SM_STAGE_TRANSMISSION_MODE_ECC_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_ECC_SEND_SET_MSG_LEN_CMD_COMPLETE,


    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_CMD_COMPLETE,
    
    _FM_TX_SM_STAGE_TRANSMISSION_MODE_COMPLETE
 } _FmTxSmStageType_transmissionMode;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_transmissionMode[] =  {
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_START,                                   _FM_TX_SM_HandlerTransmissionMode_Start},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_LEN_CMD, _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_CMD,     _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,        _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_TO_RT,        _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsPsMsgCmd},

    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_LEN_CMD, _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_CMD,     _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,        _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},

    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_TO_ECC,       _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},

    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_ECC_SEND_SET_MSG_LEN_CMD,    _FM_TX_SM_HandlerTransmissionMode_SendSetRdsEccMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_ECC_SEND_SET_MSG_LEN_CMD_COMPLETE,   _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsEccMsgCmd},

    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_LEN_CMD,   _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_LEN_CMD_COMPLETE,  _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_CMD,       _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_RAW_DATA_SEND_SET_MSG_CMD_COMPLETE,      _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    
    {_FM_TX_SM_STAGE_TRANSMISSION_MODE_COMPLETE,                                _FM_TX_SM_HandlerTransmitMask_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_transmissionMode = {   _fmTxSmCmdProcessor_transmissionMode, 
                                                        sizeof(_fmTxSmCmdProcessor_transmissionMode) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Set Transmission Field Mask
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_TRANSMIT_MASK_START,

    _FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,

    _FM_TX_SM_STAGE_TRANSMIT_MASK_PS_TO_RT,


    _FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,


    _FM_TX_SM_STAGE_TRANSMIT_MASK_RT_TO_ECC,


    _FM_TX_SM_STAGE_TRANSMIT_MASK_ECC_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_TRANSMIT_MASK_ECC_SEND_SET_MSG_LEN_CMD_COMPLETE,


    _FM_TX_SM_STAGE_TRANSMIT_MASK_COMPLETE
 } _FmTxSmStageType_transmitMask;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_transmitMask[] =  {
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_START,                                   _FM_TX_SM_HandlerTransmitMask_Start},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_LEN_CMD, _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_CMD,     _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,        _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_PS_TO_RT,        _FM_TX_SM_HandlerTransmitMask_WaitSetRdsPsMsgCmd},

    {_FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_LEN_CMD, _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_CMD,     _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,        _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},

    {_FM_TX_SM_STAGE_TRANSMIT_MASK_RT_TO_ECC,       _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},

    {_FM_TX_SM_STAGE_TRANSMIT_MASK_ECC_SEND_SET_MSG_LEN_CMD,    _FM_TX_SM_HandlerTransmitMask_SendSetRdsEccMsgCmd},
    {_FM_TX_SM_STAGE_TRANSMIT_MASK_ECC_SEND_SET_MSG_LEN_CMD_COMPLETE,   _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},

    {_FM_TX_SM_STAGE_TRANSMIT_MASK_COMPLETE,                                _FM_TX_SM_HandlerTransmissionMode_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_transmitMask = {   _fmTxSmCmdProcessor_transmitMask, 
                                                        sizeof(_fmTxSmCmdProcessor_transmitMask) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Set Traffic Codes
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_START,
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TA_CMD,
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TA_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TP_CMD,
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TP_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_TRAFFIC_CODES_COMPLETE
 } _FmTxSmStageType_SetTrafficCodes;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SetTrafficCodes[] =  {
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_START,                                   _FM_TX_SM_HandlerSetTrafficCodes_Start},
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TA_CMD,              _FM_TX_SM_HandlerSetTrafficCodes_SendTaSetCmd},
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TA_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TP_CMD,              _FM_TX_SM_HandlerSetTrafficCodes_SendTpSetCmd},
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_SET_TP_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_SET_TRAFFIC_CODES_COMPLETE,                        _FM_TX_SM_HandlerSetTrafficCodes_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SetTrafficCodes = {    _fmTxSmCmdProcessor_SetTrafficCodes, 
                                                        sizeof(_fmTxSmCmdProcessor_SetTrafficCodes) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Get Traffic Codes
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_START,
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TA_CMD,
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TA_CMD_COMPLETE,
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TP_CMD,
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TP_CMD_COMPLETE,
    _FM_TX_SM_STAGE_GET_TRAFFIC_CODES_COMPLETE
 } _FmTxSmStageType_GetTrafficCodes;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetTrafficCodes[] =  {
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_START,                                   _FM_TX_SM_HandlerGetTrafficCodes_Start},
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TA_CMD,              _FM_TX_SM_HandlerGetTrafficCodes_SendTaGetCmd},
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TA_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TP_CMD,              _FM_TX_SM_HandlerGetTrafficCodes_SendTpGetCmd},
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_SET_TP_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_GET_TRAFFIC_CODES_COMPLETE,                        _FM_TX_SM_HandlerGetTrafficCodes_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetTrafficCodes = {    _fmTxSmCmdProcessor_GetTrafficCodes, 
                                                        sizeof(_fmTxSmCmdProcessor_GetTrafficCodes) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Set Audio Source
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_START,
    _FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_SEND_VAC_CMD,
    _FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_SEND_VAC_CMD_COMPLETE,
    _FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_ENABLE_AUDIO_CMD,
    _FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_COMPLETE
 } _FmTxSmStageType_ChangeAudioSource;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_ChangeAudioSource[] =    {
    {_FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_START,                                 _FM_TX_SM_HandlerChangeAudioSource_Start},
    {_FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_SEND_VAC_CMD,              _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmd},
    {_FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_SEND_VAC_CMD_COMPLETE,     _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmdComplete},
    {_FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_ENABLE_AUDIO_CMD,          _FM_TX_SM_HandlerChangeAudioSource_SendEnableAudioSource},
    {_FM_TX_SM_STAGE_CHANGE_AUDIO_SOURCE_COMPLETE,                      _FM_TX_SM_HandlerChangeAudioSource_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_ChangeAudioSource = {  _fmTxSmCmdProcessor_ChangeAudioSource, 
                                                        sizeof(_fmTxSmCmdProcessor_ChangeAudioSource) / sizeof(_FmTxSmCmdStageInfo)};

/*
########################################################################################################################
    Set Digital Audio Configuration
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_START,
    _FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_SEND_VAC_CMD,
    _FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_SEND_VAC_CMD_COMPLETE,
    _FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_ENABLE_AUDIO_CMD,
    _FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_COMPLETE
 } _FmTxSmStageType_ChangeDigitalAudioConfiguration;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_ChangeDigitalAudioConfiguration[] =  {
    {_FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_START,                                 _FM_TX_SM_HandlerChangeDigitalAudioConfig_Start},
    {_FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_SEND_VAC_CMD,              _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendChangeConfigurationCmd},
    {_FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_SEND_VAC_CMD_COMPLETE,     _FM_TX_SM_HandlerChangeDigitalAudioConfig_ChangeDigitalAudioConfigCmdComplete},
    {_FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_ENABLE_AUDIO_CMD,          _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendEnableAudioSource},
    {_FM_TX_SM_STAGE_CHANGE_DIGITAL_AUDIO_CONFIG_COMPLETE,                      _FM_TX_SM_HandlerChangeDigitalAudioConfig_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_ChangeDigitalAudioConfiguration = {    _fmTxSmCmdProcessor_ChangeDigitalAudioConfiguration, 
                                                        sizeof(_fmTxSmCmdProcessor_ChangeDigitalAudioConfiguration) / sizeof(_FmTxSmCmdStageInfo)};
/*
########################################################################################################################
    Change Mono Stereo Mode
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_START,
    _FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_SEND_VAC_CMD,
    _FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_SET_MONO_STEREO_MODE_CMD,
    _FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_COMPLETE
 } _FmTxSmStageType_ChangeMonoStereoMode;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_ChangeMonoStereoMode[] = {
    {_FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_START,                                 _FM_TX_SM_HandlerChangeMonoStereoMode_Start},
    {_FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_SEND_VAC_CMD,              _FM_TX_SM_HandlerChangeMonoStereoMode_SendChangeConfigurationCmd},
    {_FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_SET_MONO_STEREO_MODE_CMD,          _FM_TX_SM_HandlerChangeMonoStereoMode_SetMonoModeCmd},
    {_FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_COMPLETE,                      _FM_TX_SM_HandlerChangeMonoStereoMode_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_ChangeMonoStereoMode = {   _fmTxSmCmdProcessor_ChangeMonoStereoMode, 
                                                        sizeof(_fmTxSmCmdProcessor_ChangeMonoStereoMode) / sizeof(_FmTxSmCmdStageInfo)};

/*
########################################################################################################################
    Init Simple SET Command Processing Variables
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_SIMPLE_SET_CMD_START,
    _FM_TX_SM_STAGE_SIMPLE_SET_CMD_WAIT_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SIMPLE_SET_CMD_COMPLETE
 } _FmTxSmStageType_SimpleSetCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SimpleSetCmd[] =  {
    {_FM_TX_SM_STAGE_SIMPLE_SET_CMD_START,                      _FM_TX_SM_HandlerSimpleSetCmd_Start},
    {_FM_TX_SM_STAGE_SIMPLE_SET_CMD_WAIT_CMD_COMPLETE,          _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_SIMPLE_SET_CMD_COMPLETE,                   _FM_TX_SM_HandlerSimpleSetCmd_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SimpleSetCmd = {   _fmTxSmCmdProcessor_SimpleSetCmd, 
                                                    sizeof(_fmTxSmCmdProcessor_SimpleSetCmd) / sizeof(_FmTxSmCmdStageInfo)};

/*
########################################################################################################################
    Init Simple Get Command Processing Variables
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_SIMPLE_GET_CMD_START,
    _FM_TX_SM_STAGE_SIMPLE_GET_CMD_WAIT_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SIMPLE_GET_CMD_COMPLETE
 } _FmTxSmStageType_SimpleGetCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SimpleGetCmd[] =  {
    {_FM_TX_SM_STAGE_SIMPLE_GET_CMD_START,                      _FM_TX_SM_HandlerSimpleGetCmd_Start},
    {_FM_TX_SM_STAGE_SIMPLE_GET_CMD_WAIT_CMD_COMPLETE,          _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
    {_FM_TX_SM_STAGE_SIMPLE_GET_CMD_COMPLETE,                   _FM_TX_SM_HandlerSimpleGetCmd_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SimpleGetCmd = {   _fmTxSmCmdProcessor_SimpleGetCmd, 
                                                    sizeof(_fmTxSmCmdProcessor_SimpleGetCmd) / sizeof(_FmTxSmCmdStageInfo)};

/*
########################################################################################################################
    Set PS Text
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_START,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_COMPLETE
} _FmTxSmStageType_SetRdsTextPsMsgCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SetRdsTextPsMsgCmd[] =  {
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_START,                                 _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Start},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_LEN_CMD,  _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_CMD,      _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_SEND_SET_MSG_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_PS_MSG_COMPLETE,                          _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SetRdsTextPsMsgCmd = { _fmTxSmCmdProcessor_SetRdsTextPsMsgCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_SetRdsTextPsMsgCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};

/*
########################################################################################################################
    Set RT Text
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_START,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_COMPLETE
} _FmTxSmStageType_SetRdsTextRtMsgCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SetRdsTextRtMsgCmd[] =  {
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_START,                                 _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Start},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_LEN_CMD,  _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_LEN_CMD_COMPLETE, _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_CMD,      _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_SEND_SET_MSG_CMD_COMPLETE,     _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_TEXT_RT_MSG_COMPLETE,                          _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SetRdsTextRtMsgCmd = { _fmTxSmCmdProcessor_SetRdsTextRtMsgCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_SetRdsTextRtMsgCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*
########################################################################################################################
    Set Raw Data
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_START,
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_LEN_CMD,
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_LEN_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_CMD,
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_CMD_COMPLETE,
    _FM_TX_SM_STAGE_SET_RDS_RAW_DATA_COMPLETE
} _FmTxSmStageType_SetRdsRawDataCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_SetRdsRawDataCmd[] =  {
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_START,                                _FM_TX_SM_HandlerSetRdsRawDataCmd_Start},
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_LEN_CMD, _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_LEN_CMD_COMPLETE,    _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd},
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_CMD,     _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_SEND_SET_MSG_CMD_COMPLETE,        _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd},
    {_FM_TX_SM_STAGE_SET_RDS_RAW_DATA_COMPLETE,                         _FM_TX_SM_HandlerSetRdsRawDataCmd_Complete}
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_SetRdsRawDataCmd = {   _fmTxSmCmdProcessor_SetRdsRawDataCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_SetRdsRawDataCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*
########################################################################################################################
    Get Field Mask
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_GET_RDS_TRANSMISSION_FIELD_MASK_START
 } _FmTxSmStageType_GetRdsTransmissionFieldMaskCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetRdsTransmissionFieldMaskCmd[] =  {
    {_FM_TX_SM_STAGE_GET_RDS_TRANSMISSION_FIELD_MASK_START,                     _FM_TX_SM_HandlerGetRdsTransmissionFieldMaskCmd_Start}};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetRdsTransmissionFieldMaskCmd = { _fmTxSmCmdProcessor_GetRdsTransmissionFieldMaskCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_GetRdsTransmissionFieldMaskCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*
########################################################################################################################
    Get Transmission Mode
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_GET_RDS_TRANSMISSION_MODE_START
 } _FmTxSmStageType_GetRdsTransmissionModeCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetRdsTransmissionModeCmd[] =  {
    {_FM_TX_SM_STAGE_GET_RDS_TRANSMISSION_MODE_START,                   _FM_TX_SM_HandlerGetRdsTransmissionModeCmd_Start}};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetRdsTransmissionModeCmd = {  _fmTxSmCmdProcessor_GetRdsTransmissionModeCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_GetRdsTransmissionModeCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};


/*
########################################################################################################################
    Get PS text
########################################################################################################################
*/

enum {
    _FM_TX_SM_STAGE_GET_RDS_TEXT_PS_MSG_STAPS
 } _FmTxSmStageType_GetRdsTextPsMsgCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetRdsTextPsMsgCmd[] =  {
    {_FM_TX_SM_STAGE_GET_RDS_TEXT_PS_MSG_STAPS,                     _FM_TX_SM_HandlerGetRdsTextPsMsgCmd_Start},
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetRdsTextPsMsgCmd = { _fmTxSmCmdProcessor_GetRdsTextPsMsgCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_GetRdsTextPsMsgCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*
########################################################################################################################
    Get RT text
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_GET_RDS_TEXT_RT_MSG_START
 } _FmTxSmStageType_GetRdsTextRtMsgCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetRdsTextRtMsgCmd[] =  {
    {_FM_TX_SM_STAGE_GET_RDS_TEXT_RT_MSG_START,                     _FM_TX_SM_HandlerGetRdsTextRtMsgCmd_Start},
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetRdsTextRtMsgCmd = { _fmTxSmCmdProcessor_GetRdsTextRtMsgCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_GetRdsTextRtMsgCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*
########################################################################################################################
    Get Raw Data Text
########################################################################################################################
*/
enum {
    _FM_TX_SM_STAGE_GET_RDS_RAW_DATA_START
 } _FmTxSmStageType_GetRdsRawDataCmd;

FMC_STATIC _FmTxSmCmdStageInfo _fmTxSmCmdProcessor_GetRdsRawDataCmd[] =  {
    {_FM_TX_SM_STAGE_GET_RDS_RAW_DATA_START,                    _FM_TX_SM_HandlerGetRdsRawDataCmd_Start},
};

FMC_STATIC _FmTxSmCmdInfo _fmTxSmCmdInfo_GetRdsRawDataCmd = {   _fmTxSmCmdProcessor_GetRdsRawDataCmd, 
                                                                (sizeof(_fmTxSmCmdProcessor_GetRdsRawDataCmd) / 
                                                                    sizeof(_FmTxSmCmdStageInfo))};
/*######################################################################################################################################################################################*/



/*
*   [ToDo Zvi] Decide if we need these definiotions and if we do maybe we should locate them in more apropriate place
*/

FMC_STATIC FMC_BOOL recievedDisable = FMC_FALSE;

/* [ToDo Zvi] We nees this flag so if one wishes to disable after enabeling timout accures the disable process will not dispatch the enable command again*/
/* FMC_STATIC FMC_BOOL timerExpiredWhileEnable = FMC_FALSE; */

FMC_STATIC FMC_BOOL timerExp = FMC_FALSE;


FMC_STATIC FMC_U8 smNumOfCmdInQueue = 0;

/*
*   Timer definitions for the processes time out
*
*   Long command: The Enable - loading the init script may take long time
*
*/
#define FM_TX_SM_TIMOUT_TIME_FOR_LONG_CMD       (50000)
#define FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD        (5000)

/*
*   end
*/
/*######################################################################################################################################################################################*/

/*
*   
*
*/
void _FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);


}
void _FM_TX_SM_HandlerGeneral_SendSetIntMaskCmd( FmcFwIntMask mask,FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _fmTxSmIntMask.intMask = (FmcFwIntMask)(FMC_FW_MASK_MAL|mask);
    
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET , _fmTxSmIntMask.intMask);

    _fmTxSmData.currCmdInfo.stageIndex++;
}
void _FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd( FMC_UINT event, void *eventData)
{
    _FM_TX_SM_HandlerGeneral_SendSetIntMaskCmd(FMC_FW_MASK_POW_ENB,event,eventData);
}
void _FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt( FMC_UINT event, void *eventData)
{
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    

    _fmTxSmIntMask.intMask &= ~FMC_FW_MASK_POW_ENB;

    _fmTxSmData.currCmdInfo.stageIndex++;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}

/*
########################################################################################################################

    Set RDS Text    functions
    
########################################################################################################################    
*/
void _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd( FMC_UINT event, void *eventData)
{
    FmcStatus   status;
    FMC_U16     rtMsgLenAndGroup;
    FMC_U16     rdsTextLen;
    FMC_U16     rdsGroupText;
    FMC_FUNC_START("_FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    if(_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_ENABLING)
    {
        rdsTextLen =    FMC_OS_StrLen((const FMC_U8 *)FMC_CONFIG_TX_DEFAULT_RDS_RT_MSG);
        rdsGroupText = 2;/*[ToDo Zvi] define macros for the groups*/
    }
    else
    {
        rdsTextLen = _fmTxSmData.context.rdsTextLen;
        rdsGroupText = _fmTxSmData.context.rdsGroupText;
    }
    
    rtMsgLenAndGroup = (FMC_U16)((FMC_U16)rdsTextLen|(FMC_U16)rdsGroupText<<8);
    
    status = _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_CONFIG_DATA_SET, rtMsgLenAndGroup);
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_PENDING), status, ("_FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgLenGoupCmd"));
    _fmTxSmData.currCmdInfo.stageIndex++;

    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgLenGoupCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    _fmTxSmData.context.numOfSetRdsChars = 0;

    _fmTxSmData.currCmdInfo.stageIndex++;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}

/*
    RDS data is set in chunks of up to FMC_FW_TX_MAX_RDS_DATA_SET_LEN bytes. 
    The process is:

    In this stage:
    -----------
    Calculate the total number of characters that are still to be set

    If there are still characters to set
        Calculate the number of chars in the next chunk (up to FMC_FW_TX_MAX_RDS_DATA_SET_LEN)
        Send them to the chip
        Wait for command completion
    Else
        Set stage index to point at the stage after RDS data setting stages (2 stages more)
        Call the dispatcher to move to that stage
    End If

    In the next stage (waiting for comamnd completion)
    -------------------------------------------
    Update number of sent characters (add number of chars in the chunk just set)
    Set the stage index to point at the previous stage
    Call the dispatcher to move to that stage (and continue the RDS data setting process)
*/
void _FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd( FMC_UINT event, 
                                                                    void *eventData)
{
    FmcStatus   status;
    FMC_UINT    numOfRdsCharsLeftToSet;
    FMC_U8      *nextChunk = NULL;
    FMC_U8      *rdsText;   

    FMC_FUNC_START("_FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd");

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    
    /* Locate the position of the first char of the next chunk */
    if(_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_ENABLING)
    {
        rdsText = (FMC_U8   *)FMC_CONFIG_TX_DEFAULT_RDS_RT_MSG;
        
    }
    else
    {
        rdsText = _fmTxSmData.context.rdsText;
    }
    nextChunk = &rdsText[_fmTxSmData.context.numOfSetRdsChars];
    
    numOfRdsCharsLeftToSet = (FMC_UINT)FMC_OS_StrLen(nextChunk);
    
    if (numOfRdsCharsLeftToSet > 0)
    {
        /* Calculate size of next chunk */
        if (numOfRdsCharsLeftToSet <= FMC_FW_TX_MAX_RDS_DATA_SET_LEN)
        {
            _fmTxSmData.context.rdsChunkSize = numOfRdsCharsLeftToSet;
        }
        else
        {
            _fmTxSmData.context.rdsChunkSize  = FMC_FW_TX_MAX_RDS_DATA_SET_LEN;
        }

        FMC_LOG_INFO(("_FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd: Next Rds Sement is of Len %d chars)", 
                        _fmTxSmData.context.rdsChunkSize));

        /* Send the next chunk */
        status = FMC_CORE_SendWriteRdsDataCommand(  FMC_FW_OPCODE_TX_RDS_DATA_SET, 
                                                            (FMC_U8*)nextChunk,
                                                            _fmTxSmData.context.rdsChunkSize);
        FMC_VERIFY_FATAL((status == FM_TX_STATUS_PENDING), status, ("_FM_TX_SM_HandlerGeneral_SendSetRdsTextMsgCmd"));
        _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    /* Set the entire message - continue the process after the sending stage */
    else
    {
        _fmTxSmData.currCmdInfo.stageIndex += 2;
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    
    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerGeneral_WaitSetRdsTextMsgCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Successfully completed setting another chunk */
    _fmTxSmData.context.numOfSetRdsChars += _fmTxSmData.context.rdsChunkSize ;
    
    /* Go back to send the next message chunk (if any characters left to set) */
    _fmTxSmData.currCmdInfo.stageIndex--;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
/*
########################################################################################################################

    End - Set RDS Text 
    
########################################################################################################################    
*/


/*
########################################################################################################################
*/

void    _FM_TX_SM_InitDefualtConfigValues(void)
{

    /*no need to configure on startup */
    _fmTxSmData.context.fwCache.tunedFreq = FMC_UNDEFINED_FREQ;/*no need to configure on startup */
    _fmTxSmData.context.fwCache.transmissionOn = FMC_FALSE;/*no need to configure on startup */
    _fmTxSmData.context.fwCache.muteMode = FMC_CONFIG_TX_DEFUALT_MUTE_MODE;/*no need to configure on startup */
    _fmTxSmData.context.fwCache.rdsEnabled = FMC_FALSE;/*no need to configure on startup */
    _fmTxSmData.context.fwCache.transmissionMode = FMC_RDS_TRANSMISSION_AUTOMATIC;/*no need to configure on startup */

    _fmTxSmData.context.fwCache.countryCode = FMC_RDS_ECC_NONE;/*no need to configure on startup */
    
    _fmTxSmData.context.fwCache.fieldsMask = FMC_CONFIG_TX_DEFAULT_RDS_TRANSMITTED_FIELDS_MASK;/*need to configure on startup */

    _fmTxSmData.context.fwCache.emphasisfilter = FMC_CONFIG_TX_DEFUALT_PRE_EMPHASIS_FILTER;/*no need to configure on startup */

    _fmTxSmData.context.fwCache.afCode = FMC_AF_CODE_NO_AF_AVAILABLE;/*no need to configure on startup */

    _fmTxSmData.context.fwCache.rdsPtyCode = 0;/*no need to configure on startup */

    _fmTxSmData.context.fwCache.rdsPiCode = FMC_CONFIG_TX_DEFAULT_RDS_PI_CODE;/*need to configure on startup */

    _fmTxSmData.context.fwCache.rdsRepertoire = FMC_CONFIG_TX_DEFUALT_REPERTOIRE_CODE_TABLE;

    _fmTxSmData.context.fwCache.rdsScrollSpeed = FMC_CONFIG_TX_DEFUALT_RDS_PS_DISPLAY_SPEED;

    _fmTxSmData.context.fwCache.rdsScrollMode = FMC_CONFIG_TX_DEFUALT_RDS_TEXT_SCROLL_MODE;

    _fmTxSmData.context.fwCache.rdsTaCode = FMC_RDS_TA_OFF;

    _fmTxSmData.context.fwCache.rdsTpCode = FMC_RDS_TP_OFF;

    _fmTxSmData.context.fwCache.rdsRusicSpeachFlag = FMC_CONFIG_TX_DEFUALT_MUSIC_SPEECH_FLAG;

    _fmTxSmData.context.fwCache.powerLevel = FMC_CONFIG_TX_DEFUALT_POWER_LEVEL;/*need to configure on startup */

    _fmTxSmData.context.vacParams.monoStereoMode = FMC_CONFIG_TX_DEFUALT_STEREO_MONO_MODE;/*no need to configure on startup */

    _fmTxSmData.context.vacParams.eSampleFreq = CAL_SAMPLE_FREQ_48000;

    _fmTxSmData.context.vacParams.audioSource = FM_TX_AUDIO_SOURCE_FM_ANALOG;

    FMC_OS_StrCpy(_fmTxSmData.context.fwCache.psMsg, (const FMC_U8 *)FMC_CONFIG_TX_DEFAULT_RDS_PS_MSG);/*need to configure on startup */
    
    _fmTxSmData.context.fwCache.rtType = FMC_CONFIG_TX_DEFAULT_RDS_RT_TYPE;/*need to configure on startup */
    
    FMC_OS_StrCpy(_fmTxSmData.context.fwCache.rtMsg, (const FMC_U8 *)FMC_CONFIG_TX_DEFAULT_RDS_RT_MSG);/*need to configure on startup */
    
}
/*######################################################################################################################################################################################*/
FmTxStatus FM_TX_SM_Init(void)
{
    
    FMC_FUNC_START("FM_TX_SM_Init");

    /* Init command dispatching FMC_STATIC tables */    
    
    _FM_TX_SM_InitCmdsTable();

    _FM_TX_SM_InitSimpleSetCmd2FwOpcodeMap();
    _FM_TX_SM_InitSetCmdGetCmdParmValueMap();
    _FM_TX_SM_InitSetCmdCompleteMap();
    _FM_TX_SM_InitSetCmdFillEventDataMap();

    _FM_TX_SM_InitSimpleGetCmd2FwOpcodeMap();

    _FM_TX_SM_InitDefualtConfigValues();
    
    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_DESTROYED;
    _fmTxSmData.currCmdInfo.smState = _FM_TX_SM_STATE_NONE;

    /*Init the interrupts structer*/
    _fmTxSmIntMask.intMask = 0;
    _fmTxSmIntMask.intMaskReadButNotHandled = 0;
    _fmTxSmIntMask.intMaskEnabled = FMC_FALSE;
    _fmTxSmIntMask.intRecievedButNotRead = FMC_FALSE;
    _fmTxSmIntMask.waitingForReadIntComplete = FMC_FALSE;
    _fmTxSmIntMask.intWaitingFor = 0;

    CCM_VAC_RegisterCallback(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_TX,_FM_TX_SM_TccmVacCb);
    
    FMC_FUNC_END();
    
    return FM_TX_STATUS_SUCCESS;
}

FmTxStatus FM_TX_SM_Deinit(void)
{
    FMC_FUNC_START("FM_TX_SM_Deinit");

    FMC_FUNC_END();
    
    return FM_TX_STATUS_SUCCESS;
}

FmTxStatus FM_TX_SM_Create(const FmTxCallBack fmCallback, FmTxContext **fmContext)
{
    FMC_FUNC_START("FM_TX_SM_Create");

    /* [ToDo] Protect against multiple creations for the same context */
    
    _fmTxSmData.currCmdInfo.baseCmd = NULL;
    
    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_DISABLED;
    
    _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_FALSE;
    _fmTxSmData.context.appCb = fmCallback;
    _fmTxSmData.context.rdsRawDataCmdBufAllocated = FMC_FALSE;
    _fmTxSmData.context.rdsPsCmdBufAllocated = FMC_FALSE;
    _fmTxSmData.context.rdsRtCmdBufAllocated = FMC_FALSE;

    _fmTxSmData.context.transmissionStoppedForTune = FMC_FALSE;

    /* Currently, there is a single context */
    *fmContext = &_fmTxSmData.context;
    
    FMC_FUNC_END();
    
    return FM_TX_STATUS_SUCCESS;
}

FmTxStatus FM_TX_SM_Destroy(FmTxContext **fmContext)
{
    FmTxStatus status = FM_TX_STATUS_SUCCESS;
    
    FMC_FUNC_START("FM_TX_SM_Destroy");
    
    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_DESTROYED;
    _fmTxSmData.context.appCb = NULL;


    FMC_VERIFY_FATAL(((_fmTxSmData.context.rdsRawDataCmdBufAllocated == FMC_FALSE)&&(_fmTxSmData.context.rdsPsCmdBufAllocated == FMC_FALSE)&&(_fmTxSmData.context.rdsRtCmdBufAllocated == FMC_FALSE)), FM_TX_STATUS_INTERNAL_ERROR,("FM_TX_SM_Destroy: RDS FIFO Buffer is still allocated"));

    *fmContext = NULL;
    
    FMC_FUNC_END();
    
    return status;
}
FMC_BOOL FM_TX_SM_IsRdsEnabled(void)
{
    return _fmTxSmData.context.fwCache.rdsEnabled;
}
FmTxRdsTransmissionMode FM_TX_SM_GetRdsMode(void)
{
    return _fmTxSmData.context.fwCache.transmissionMode;
}
FmTxContext* FM_TX_SM_GetContext(void)
{
    return &_fmTxSmData.context;
}

FmTxSmContextState FM_TX_SM_GetContextState(void)
{
    return _fmTxSmData.context.state;
}

FMC_BOOL FM_TX_SM_IsContextEnabled(void)
{
    if (    (_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_ENABLED) ||
        (_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING))
    {
        return FMC_TRUE;
    }   
    else
    {
        return FMC_FALSE;
    }
}

/*
    Initializes the mappings between the type of a command and its command handling information object
*/
void _FM_TX_SM_InitCmdsTable(void)
{
    _fmTxSmCmdsTable[FM_TX_CMD_ENABLE] = &_fmTxSmCmdInfo_Enable;
    _fmTxSmCmdsTable[FM_TX_CMD_DISABLE] = &_fmTxSmCmdInfo_Disable;

    _fmTxSmCmdsTable[FM_TX_CMD_TUNE] = &_fmTxSmCmdInfo_Tune;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_TUNED_FREQUENCY] = &_fmTxSmCmdInfo_SimpleGetCmd;
    
    _fmTxSmCmdsTable[FM_TX_CMD_SET_INTERRUPT_MASK] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_INTERRUPT_SATUS] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_POWER_LEVEL] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_POWER_LEVEL] = &_fmTxSmCmdInfo_SimpleGetCmd;
    
    _fmTxSmCmdsTable[FM_TX_CMD_SET_MUTE_MODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_MUTE_MODE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_START_TRANSMISSION] = &_fmTxSmCmdInfo_StartTransmission;
    _fmTxSmCmdsTable[FM_TX_CMD_STOP_TRANSMISSION] = &_fmTxSmCmdInfo_StopTransmission;
    
    _fmTxSmCmdsTable[FM_TX_CMD_ENABLE_RDS] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_DISABLE_RDS] = &_fmTxSmCmdInfo_SimpleSetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_PI_CODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_PI_CODE] = &_fmTxSmCmdInfo_SimpleGetCmd;
    
    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TEXT_PS_MSG] = &_fmTxSmCmdInfo_SetRdsTextPsMsgCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TEXT_PS_MSG] = &_fmTxSmCmdInfo_GetRdsTextPsMsgCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TEXT_RT_MSG] = &_fmTxSmCmdInfo_SetRdsTextRtMsgCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TEXT_RT_MSG] = &_fmTxSmCmdInfo_GetRdsTextRtMsgCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TRANSMITTED_MASK] = &_fmTxSmCmdInfo_transmitMask;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TRANSMITTED_MASK] = &_fmTxSmCmdInfo_GetRdsTransmissionFieldMaskCmd;

    /*the set mono stereo should call the VAC if transmitting*/
    _fmTxSmCmdsTable[FM_TX_CMD_SET_MONO_STEREO_MODE] = &_fmTxSmCmdInfo_ChangeMonoStereoMode;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_MONO_STEREO_MODE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TRANSMISSION_MODE] = &_fmTxSmCmdInfo_transmissionMode;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TRANSMISSION_MODE] = &_fmTxSmCmdInfo_GetRdsTransmissionModeCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_AF_CODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_AF_CODE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_PTY_CODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_PTY_CODE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_PRE_EMPHASIS_FILTER] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_PRE_EMPHASIS_FILTER] = &_fmTxSmCmdInfo_SimpleGetCmd;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_TRAFFIC_CODES] = &_fmTxSmCmdInfo_SetTrafficCodes;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_TRAFFIC_CODES] = &_fmTxSmCmdInfo_GetTrafficCodes;

    _fmTxSmCmdsTable[FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE] = &_fmTxSmCmdInfo_SimpleSetCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE] = &_fmTxSmCmdInfo_SimpleGetCmd;
    
    _fmTxSmCmdsTable[FM_TX_CMD_WRITE_RDS_RAW_DATA] = &_fmTxSmCmdInfo_SetRdsRawDataCmd;
    _fmTxSmCmdsTable[FM_TX_CMD_READ_RDS_RAW_DATA] = &_fmTxSmCmdInfo_GetRdsRawDataCmd;

    
    _fmTxSmCmdsTable[FM_TX_CMD_CHANGE_AUDIO_SOURCE] = &_fmTxSmCmdInfo_ChangeAudioSource;

    _fmTxSmCmdsTable[FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION] = &_fmTxSmCmdInfo_ChangeDigitalAudioConfiguration;
}

void _FM_TX_SM_InitSimpleSetCmd2FwOpcodeMap(void)
{
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_INTERRUPT_MASK] = FMC_FW_OPCODE_CMN_INT_MASK_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_POWER_LEVEL] = FMC_FW_OPCODE_TX_POWER_LEVEL_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_MUTE_MODE] = FMC_FW_OPCODE_TX_MUTE_MODE_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_STOP_TRANSMISSION] = FMC_FW_OPCODE_TX_POWER_ENB_SET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_ENABLE_RDS] = FMC_FW_OPCODE_TX_RDS_DATA_ENB_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_DISABLE_RDS] = FMC_FW_OPCODE_TX_RDS_DATA_ENB_SET_GET; 
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE] = FMC_FW_OPCODE_TX_RDS_PS_DISPLAY_MODE_SET_GET; 
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED] =FMC_FW_OPCODE_TX_RDS_PS_SCROLL_SPEED_SET_GET; 
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_PI_CODE] = FMC_FW_OPCODE_TX_PI_CODE_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_AF_CODE] = FMC_FW_OPCODE_TX_RDS_AF_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_PTY_CODE] = FMC_FW_OPCODE_TX_RDS_PTY_CODE_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE] = FMC_FW_OPCODE_TX_RDS_REPERTOIRE_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG] = FMC_FW_OPCODE_TX_RDS_MUSIC_SPEECH_FLAG_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_PRE_EMPHASIS_FILTER] = FMC_FW_OPCODE_TX_PREMPH_SET_GET;
    _fmTxSmSimpleSetCmd2FwOpcodeMap[FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE] = FMC_FW_OPCODE_TX_RDS_ECC_SET_GET;

    
}

void _FM_TX_SM_InitSetCmdGetCmdParmValueMap(void)
{
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_INTERRUPT_MASK] = _FM_TX_SM_HandlerSetInterruptRegister_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_TUNE] = _FM_TX_SM_HandlerTune_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_POWER_LEVEL] = _FM_TX_SM_HandlerSetPowerLevel_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_MUTE_MODE] = _FM_TX_SM_HandlerSetMuteMode_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_START_TRANSMISSION] = _FM_TX_SM_HandlerStartTransmission_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_STOP_TRANSMISSION] = _FM_TX_SM_HandlerStopTransmission_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_ENABLE_RDS] = _FM_TX_SM_HandlerEnableRds_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_DISABLE_RDS] = _FM_TX_SM_HandlerDisableRds_GetCmdParmValue; 
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE] = _FM_TX_SM_HandlerSetRdsTextScrollMode_GetCmdParmValue; 
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED] =_FM_TX_SM_HandlerSetRdsTextScrollSpeed_GetCmdParmValue; 
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_TRANSMITTED_MASK] = _FM_TX_SM_HandlerSetRdsTransmittedMask_GetCmdParmValue; 

    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_PI_CODE] = _FM_TX_SM_HandlerSetRdsPiCode_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_TRANSMISSION_MODE] = _FM_TX_SM_HandlerSetRdsTransmissionMode_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_AF_CODE] = _FM_TX_SM_HandlerSetRdsAfCode_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_PTY_CODE] = _FM_TX_SM_HandlerSetRdsPtyCode_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE] = _FM_TX_SM_HandlerSetRdsTextRepertoire_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG] = _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_PRE_EMPHASIS_FILTER] = _FM_TX_SM_HandlerSetPreEmphasisFilter_GetCmdParmValue;
    _fmTxSmSetCmdGetCmdParmValue[FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE] = _FM_TX_SM_HandlerSetRdsExtendedCountryCode_GetCmdParmValue;
}

/*
*   [ToDo Zvi] This is relevant for all processes
*/
void _FM_TX_SM_InitSetCmdCompleteMap(void)
{
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_POWER_LEVEL] = _FM_TX_SM_HandlerPowerLvl_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_TUNE] = _FM_TX_SM_HandlerTune_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_MUTE_MODE] = _FM_TX_SM_HandlerSetMuteMode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_START_TRANSMISSION] = _FM_TX_SM_HandlerStartTransmission_CmdComplete;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_STOP_TRANSMISSION] = _FM_TX_SM_HandlerStopTransmission_CmdComplete;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_ENABLE_RDS] = _FM_TX_SM_HandlerEnableRds_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_DISABLE_RDS] = _FM_TX_SM_HandlerDisableRds_UpdateCache; 
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TRANSMITTED_MASK] = _FM_TX_SM_HandlerSetRdsRTransmittedMask_UpdateCache; 
    
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE] =_FM_TX_SM_HandlerSetRdsPsDisplayMode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED] =_FM_TX_SM_HandlerSetRdsPsScrollSpeed_UpdateCache; 
    
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TRAFFIC_CODES] =_FM_TX_SM_HandlerSetRdsTrafficCodes_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TEXT_PS_MSG] =_FM_TX_SM_HandlerSetRdsPsMsg_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TEXT_RT_MSG] =_FM_TX_SM_HandlerSetRdsRtMsg_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_WRITE_RDS_RAW_DATA] = _FM_TX_SM_HandlerSetRdsRawData_UpdateCache;
    
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_PI_CODE] = _FM_TX_SM_HandlerSetRdsPiCode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_MONO_STEREO_MODE] = _FM_TX_SM_HandlerChangeMonoStereoMode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TRANSMISSION_MODE] = _FM_TX_SM_HandlerSetRdsTransmissionMode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_AF_CODE] = _FM_TX_SM_HandlerSetRdsAfCode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_PTY_CODE] = _FM_TX_SM_HandlerSetRdsPtyCode_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE] = _FM_TX_SM_HandlerSetRdsTextRepertoire_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG] = _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_PRE_EMPHASIS_FILTER] = _FM_TX_SM_HandlerSetPreEmphasisFilter_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE] = _FM_TX_SM_HandlerSetRdsExtendedCountryCode_UpdateCache;

    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION] = _FM_TX_SM_HandlerChangeDigitalAudioConfig_UpdateCache;
    _fmTxSmSetCmdCompleteMap[FM_TX_CMD_CHANGE_AUDIO_SOURCE] = _FM_TX_SM_HandlerChangeAudioSource_UpdateCache;

}
    
void _FM_TX_SM_InitSetCmdFillEventDataMap(void)
{
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_TUNE] = _FM_TX_SM_HandlerTune_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_POWER_LEVEL] = _FM_TX_SM_HandlerSetPowerLevel_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_MUTE_MODE] = _FM_TX_SM_HandlerSetMuteMode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE] = _FM_TX_SM_HandlerSetRdsTextScrollMode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TRANSMITTED_MASK] = _FM_TX_SM_HandlerSetRdsRTransmittedMask_FillEventData;

    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED] =_FM_TX_SM_HandlerSetRdsPsScrollSpeed_FillEventData; 
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TRAFFIC_CODES] =_FM_TX_SM_HandlerSetRdsTrafficCodes_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_GET_RDS_TRAFFIC_CODES] =_FM_TX_SM_HandlerGetRdsTrafficCodes_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TEXT_PS_MSG] =_FM_TX_SM_RdsTextPsMsg_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TEXT_RT_MSG] =_FM_TX_SM_RdsTextRtMsg_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_WRITE_RDS_RAW_DATA] = _FM_TX_SM_RdsRawData_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_PI_CODE] = _FM_TX_SM_HandlerSetRdsPiCode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_MONO_STEREO_MODE] = _FM_TX_SM_HandlerChangeMonoStereoMode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TRANSMISSION_MODE] = _FM_TX_SM_HandlerSetRdsTransmissionMode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_AF_CODE] = _FM_TX_SM_HandlerSetRdsAfCode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_PTY_CODE] = _FM_TX_SM_HandlerSetRdsPtyCode_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE] = _FM_TX_SM_HandlerSetRdsTextRepertoire_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG] = _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_PRE_EMPHASIS_FILTER] = _FM_TX_SM_HandlerSetPreEmphasisFilter_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE] = _FM_TX_SM_HandlerSetECC_FillEventData;

    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_START_TRANSMISSION] = _FM_TX_SM_HandlerStartTransmission_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION] = _FM_TX_SM_HandlerChangeDigitalAudioConfig_FillEventData;
    _fmTxSmSetCmdFillEventDataMap[FM_TX_CMD_CHANGE_AUDIO_SOURCE] = _FM_TX_SM_HandlerChangeAudioSource_FillEventData;

}

void _FM_TX_SM_InitSimpleGetCmd2FwOpcodeMap(void)
{
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_INTERRUPT_SATUS] = FMC_FW_OPCODE_CMN_FLAG_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_TUNED_FREQUENCY] = FMC_FW_OPCODE_TX_CHANL_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_POWER_LEVEL] = FMC_FW_OPCODE_TX_POWER_LEVEL_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_MUTE_MODE] = FMC_FW_OPCODE_TX_MUTE_MODE_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_PI_CODE] = FMC_FW_OPCODE_TX_PI_CODE_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE] = FMC_FW_OPCODE_TX_RDS_PS_DISPLAY_MODE_SET_GET;

    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED] = FMC_FW_OPCODE_TX_RDS_PS_SCROLL_SPEED_SET_GET;
    
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_MONO_STEREO_MODE] = FMC_FW_OPCODE_TX_MONO_SET_GET;

    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_AF_CODE] = FMC_FW_OPCODE_TX_RDS_AF_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_PTY_CODE] = FMC_FW_OPCODE_TX_RDS_PTY_CODE_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE] = FMC_FW_OPCODE_TX_RDS_REPERTOIRE_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG] = FMC_FW_OPCODE_TX_RDS_MUSIC_SPEECH_FLAG_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_PRE_EMPHASIS_FILTER] = FMC_FW_OPCODE_TX_PREMPH_SET_GET;
    _fmTxSmSimpleGetCmd2FwOpcodeMap[FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE] = FMC_FW_OPCODE_TX_RDS_ECC_SET_GET;
}

void FM_TX_SM_TaskEventCb(FmcOsEvent osEvent)
{
    FMC_UNUSED_PARAMETER(osEvent);
    
    switch(osEvent)
    {
        case FMC_OS_EVENT_INTERRUPT_RECIEVED:
            /* To avoid scenario like CHANL_SET is sent while tune operation then disable asked.
            *   the disable starts working and then in the middle we receive interrupt(FR) we are 
            *   not interested in handling interrupts when disabling. 
            */
            if(!recievedDisable)
            {
                _fmTxSmIntMask.intRecievedButNotRead = FMC_TRUE;
                /*if we are waiting for Command Complete we should first wait for the command 
                *  complete and only then handle the interrupt
                */
                if(_FM_TX_SM_getState() != _FM_TX_SM_STATE_WAITING_FOR_CC)
                    _FM_TX_SM_ProcessEvents();
            }
            break;
        case FMC_OS_EVENT_GENERAL:
            _FM_TX_SM_ProcessEvents();
            break;
        case FMC_OS_EVENT_TIMER_EXPIRED:
            /* The most logical case in the real world would be _FM_TX_SM_STATE_WAITING_FOR_INTERRUPT */
            _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_PROCESS_TIMEOUT_FAILURE;
            
            timerExp = FMC_TRUE;
            /*In case of timer expired - Force to move to next stage - by calling explicitly to the dispatch.
            * The dispatch is called directly to ignore the case we are waiting for CC. If timer expired we should 
            * Stop waiting.
            */
            _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_NONE);
            _FM_TX_SM_Dispatch(_FM_TX_SM_EVENT_TRANSPORT_EVENT,&_fmTxSmData.context.transportEventData);
            
            break;
        case FMC_OS_EVENT_DISABELING:
            /*[ToDo Zvi] Consider the case of disable while waiting for Interrupt */
            
            /* We assume inside this function that the disable command was inserted to the end of the queue*/
            /*
            *   cases:
            *       1.Waiting for interrupt.
            *       2.Waiting for command complete. 
            *       3.processing command
            *       4.not processing a command.
            *
            *       in all cases one should consider these cases:
            *           1.commands queue free
            *           2.there are more commands in the commands queue
            *
            */
            
            /*if disable received after timer expired we ignore the timer expired and start disabling.*/
            timerExp = FMC_FALSE;
            
            recievedDisable = FMC_TRUE;
            /*if we are not waiting for CC - Continue handling the current command.
            * The disable will be implemented by moving to the end of every process (via the stage handler).
            * The Stage handler will use the "recievedDisable" flag to know if to move the process to its end.
            */
            if(_fmTxSmData.currCmdInfo.smState != _FM_TX_SM_STATE_WAITING_FOR_CC)
                FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

            break;
        default:
            break;
    }
}

/*
[ToDo Zvi] Update the state machine Logic.


    Events Processing pseudo code:

    If no command is in progress
    
        Get the first command in the commands queue

        If a command is pending execution
            Initialize new command processing variables
            Call _FM_TX_SM_Dispatch() to start processing the command
        End If
        
    Else (a command is processed)

        If a transport event is pending processing

            Indicate that the event is being processed

            If the transport event is for a completed script command
            
                Call TI_SP_HciCmdCompleted() to let the script processor
                    handle the event as part of its script execution process
                    
            Else (the event should be processed by this module)

                Call _FM_TX_SM_Dispatch() to continue processing within the handlers
                    of the current executing command
            End If
        End If
    End If

    Notes:
    1. There is at most one command executing at a time
    2. [ToDo] - Interrupts are not handled at the moment
    3. [ToDo] - Watchdog that wraps a command execution process and verifies that 
                the execution completes within a maximum time period
        
*/
void _FM_TX_SM_ProcessEvents(void)
{
    /* No Command is currently running*/
    if (_fmTxSmData.currCmdInfo.baseCmd == NULL)
    {
            /* This case is to handle Interrupts that where received while no command was processing- this is possible scenario*/
            if(_fmTxSmIntMask.intRecievedButNotRead)
                _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_TRANSPORT_EVENT, NULL);
            /*Do we have more commands in the queue to process*/
            else if (FMC_IsListEmpty(FMCI_GetCmdsQueue()) == FMC_FALSE)
            {
                /*If we do get the first command and start processing- the command will be freed in its completion.*/
                _fmTxSmData.currCmdInfo.baseCmd = (FmcBaseCmd*)FMC_GetHeadList(FMCI_GetCmdsQueue());
                _fmTxSmData.currCmdInfo.stageIndex = 0;
                _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_SUCCESS;

                _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_START, NULL);
            }
        /* else => no command is pending execution. The event was sent when the previous command completed
            execution to make sure we do not leave a command in the queue. There is nothing to do in this case */
    }
    else
    {
        /* Have we received Command complete ?*/
        if (_fmTxSmData.context.transportEventData.eventWaitsForProcessing == FMC_TRUE)
        {   
            /*are we executing script - if we do the script processor should run the next command. - if we started disable we will not continue*/
            if ((_fmTxSmData.context.transportEventData.type == FMC_CORE_EVENT_SCRIPT_CMD_COMPLETE)&&!recievedDisable)
            {
                _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_FALSE;

                MCP_BTS_SP_HciCmdCompleted(&_fmTxSmData.context.tiSpContext, 
                                        _fmTxSmData.context.transportEventData.data,
                                        _fmTxSmData.context.transportEventData.dataLen);


            }
            else 
            {   /*continue processing - usually will get here when command sent by FM and Wait's for CC*/
                _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_TRANSPORT_EVENT, &_fmTxSmData.context.transportEventData);
            }
            
        }
        /* if we haven't received command complete - did we receive interrupt*/
        else if(_fmTxSmIntMask.intRecievedButNotRead)
        {
            /*
             *  We will enter here if we received interrupt indication and; 
             *  -we are running a fm process and waiting for interrupt(the SM state will be according.
             *  -we are running a fm process and we are not waiting for command complete.
             */
            _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_TRANSPORT_EVENT, &_fmTxSmData.context.transportEventData);
        }
        else
        {
            /* This is for the case we are in a middle of a process and processed an interrupt and read its value and now we wish to continue the process*/
            /*[ToDo Zvi] Do we need this case - it looks like it should be removed*/
            /*FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);*/
            FMC_LOG_INFO(("_FM_TX_SM_ProcessEvents: Recieved notification during sequence execution which is not command complete or interrupt"));
        }

    }
}
/*
    [ToDo Zvi] Update the description
    if (recieved event) or (dispatching (moving to the next stage) and no interrupt)of(we finished running script)
        if (didn't recieved interrupt but not read yet) or (recieved interrupt and not read yed ant this call is in the context of the event and not of the interrupt)
            dispatch
        else
            do nothing it will notified again
    else if we didn't recieve event and recieved interrupt
        if we are waiting for command complete
            do nothing it will notified again
        else
            read interrupt


*/
void _FM_TX_SM_CheckInterruptsAndThenDispatch(_FmTxSmEvent   smEvent, void *eventData)
{

    FMC_FUNC_START("_FM_TX_SM_CheckInterruptsAndThenDispatch");
    /*
    *   continue dispatching the base command is not NULL and if:
    *       1.we recieved event (most cases command complete) and the command complete is not for the read status register command
    *       2.We have just finished running a script. (if interrupt was recieved it will be handeled when the process that runs the script will dispatch.)
    *       3.We are not waiting for interrupt. And no interrupt was recieved.
    *
    */
    if((_fmTxSmData.currCmdInfo.baseCmd != NULL)&&((_fmTxSmData.context.transportEventData.eventWaitsForProcessing && !_fmTxSmIntMask.waitingForReadIntComplete)||
        (smEvent == _FM_TX_SM_EVENT_SCRIPT_EXEC_COMPLETE)||
        !_fmTxSmIntMask.intRecievedButNotRead)&&(_FM_TX_SM_getState() != _FM_TX_SM_STATE_WAITING_FOR_INTERRUPT))
        
    {
        _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_FALSE;
        
        _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_RUNNING_PROCESS);
        _FM_TX_SM_Dispatch(smEvent,eventData);
    }
    /* We recieved interrupt and didn't finish handeling it */
    else if (_fmTxSmIntMask.intRecievedButNotRead )
    {
        _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_FALSE;
        /* we didn't send the read status register command yet*/
        if(!_fmTxSmIntMask.waitingForReadIntComplete )
        {
            
             _FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd();
            _fmTxSmIntMask.waitingForReadIntComplete = FMC_TRUE;
            _fmTxSmIntMask.intMaskReadButNotHandled = 0; /* [ToDo Zvi] it should hold the mask and we have to indicate after checking that the needed interrupt recieved if we are waiting for one*/
        }
        /*This is a command Complete on the read status register command sent previoslly*/
        else
        {
            /* Get the interrupt status register value*/
            _fmTxSmIntMask.intMaskReadButNotHandled = _FM_TX_SM_HandlerInterrupt_GetInterruptStatusRegCmd(&_fmTxSmData.context.transportEventData);
            /* [ToDo Zvi] it should hold the mask and we have to indicate after checking that the needed interrupt received if we are waiting for one*/
            _fmTxSmIntMask.intRecievedButNotRead = FMC_FALSE;
            _fmTxSmIntMask.waitingForReadIntComplete = FMC_FALSE;
            /* Decide what do do with these interrupts*/
            _FM_TX_SM_HandleRecievedInterrupts(_fmTxSmIntMask.intMaskReadButNotHandled);
        }
    }
    else /* In case of watchdog timeout we may received events that will not be handled, so we ignore this event */
    {

    }
    FMC_FUNC_END();

}
/*
    This is the main command handling dispatcher

    It calls the handler that should handle the current processing stage of the current command
*/
void _FM_TX_SM_Dispatch(_FmTxSmEvent     smEvent, void *eventData)
{
    FmTxCmdType             cmdType = _fmTxSmData.currCmdInfo.baseCmd->cmdType;
    _FmTxSmCmdStageHandler  stageHandler = NULL;

    /* Extract the handler that handles the current stage of the current command */

    stageHandler = _FM_TX_SM_GetStageHandler(cmdType, _fmTxSmData.currCmdInfo.stageIndex);
    
    /* Call the handler */
    (stageHandler)(smEvent, eventData);
}

void _FM_TX_SM_TccmVacCb(ECAL_Operation eOperation,ECCM_VAC_Event eEvent, ECCM_VAC_Status eStatus)
{
    FMC_FUNC_START("_FM_TX_SM_TccmVacCb");

    if(eOperation == CAL_OPERATION_FM_TX){
        
    FMC_VERIFY_FATAL_NO_RETVAR((_fmTxSmData.context.transportEventData.eventWaitsForProcessing == FMC_FALSE), 
                                ("_FM_TX_SM_TransportEventCb: Previous Transport Event Not handled yet"));
    
    /* Verify that the previous event was already processed */
    
    _fmTxSmData.context.transportEventData.type = FMC_UTILS_ConvertVacEvent2FmcEvent(eEvent);
    _fmTxSmData.context.transportEventData.status = FMC_UTILS_ConvertVacStatus2FmcStatus(eStatus);
    _fmTxSmData.context.transportEventData.dataLen= 0;
    
    /* 
        Record the fact that a new transport event waits processing
    
        The event data must be copied before setting this flag to avoid a scenario where the FM task 
        might preempt the transport context (in whose context this function is called), check
        the flag, think the event data is ready, and start processing it before it is really there.
    */
    
    _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;
    /* Trigger the event processing in the context of the FM task */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    }
    else
    {
        FMC_VERIFY_FATAL_NO_RETVAR((eOperation == CAL_OPERATION_FM_TX), 
                                    ("_FM_TX_SM_TransportEventCb:Fm Tx recieved Vac callbeck , but was not waiting for it."));
    }
    FMC_FUNC_END();
}

void _FM_TX_SM_TransportEventCb(const FmcCoreEvent *eventParms)
{
    FMC_FUNC_START("_FM_TX_SM_TransportEventCb");

    /* Copy the event data to a local storage */
    if(!eventParms->fmInter)
    {
        /* Verify that the previous event was already processed */
        FMC_VERIFY_FATAL_NO_RETVAR((_fmTxSmData.context.transportEventData.eventWaitsForProcessing == FMC_FALSE), 
                                    ("_FM_TX_SM_TransportEventCb: Previous Transport Event Not handled yet"));

        FMC_VERIFY_FATAL_NORET((eventParms->status == FMC_STATUS_SUCCESS), 
                                    ("_FM_TX_SM_TransportEventCb: Transport Event with Error Status"));
        _fmTxSmData.context.transportEventData.type = eventParms->type;
        _fmTxSmData.context.transportEventData.status = eventParms->status;
        _fmTxSmData.context.transportEventData.dataLen= (FMC_U8)eventParms->dataLen;

        FMC_OS_MemCopy(_fmTxSmData.context.transportEventData.data, eventParms->data, eventParms->dataLen);

        /* 
            Record the fact that a new transport event waits processing

            The event data must be copied before setting this flag to avoid a scenario where the FM task 
            might preempt the transport context (in whose context this function is called), check
            the flag, think the event data is ready, and start processing it before it is really there.
        */
        
        _fmTxSmData.context.transportEventData.eventWaitsForProcessing = FMC_TRUE;
        /* Trigger the event processing in the context of the FM task */
        FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
    }
    else
    {

        /* Trigger the event processing in the context of the FM task */
        FMCI_NotifyFmTask(FMC_OS_EVENT_INTERRUPT_RECIEVED);
    }
    

    FMC_FUNC_END();
}




_FmTxSmCmdStageHandler _FM_TX_SM_GetStageHandler(FmTxCmdType cmdType, FMC_UINT stageIndex)
{
    _FmTxSmCmdInfo* cmdInfo = NULL;
    _FmTxSmCmdStageHandler stageHandler = NULL;

    FMC_FUNC_START("_FM_TX_SM_GetStageHandler");
    
/* Verify that the command is a valid command */
    FMC_VERIFY_FATAL_SET_RETVAR((cmdType <= FM_TX_LAST_CMD_TYPE), (stageHandler = NULL), 
                                    ("Command not valid -%s", FMC_DEBUG_FmTxCmdStr(cmdType)));

    /* Extract the appropriate command handler info object */
    cmdInfo = _fmTxSmCmdsTable[(FMC_UINT)cmdType];

    /* Verify that a handler object exists for the command */
    FMC_VERIFY_FATAL_SET_RETVAR((cmdInfo != NULL), (stageHandler = NULL), 
                                    ("Missing command handler info for %s", FMC_DEBUG_FmTxCmdStr(cmdType)));

    /*  the current process is not disable and the timer expired or disable requested*/
    if((timerExp||recievedDisable)&&(cmdType!= FM_TX_CMD_DISABLE))
    {
        /*go to the last stage since the process stopped due to disable operation started or the timer expiered*/
        stageIndex = cmdInfo->numOfStages - 1;
    }
    /* Extract the handler for the current stage of the command */
    stageHandler = (cmdInfo->cmdProcessor)[stageIndex].stageHandler;

    
    /* Verify that a handler exists for the stage */
    FMC_VERIFY_FATAL_SET_RETVAR((stageHandler != NULL), (stageHandler = NULL), ("Null stageHandler"));

    FMC_FUNC_END();
    
    return stageHandler;
}
/*
*   Send the FM command read interrupt status register
*/

void _FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd( void)
{
    FmcStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd");
    
    status = _FM_TX_SM_UptadeSmStateSendReadCommand(FMC_FW_OPCODE_CMN_FLAG_GET);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd: _FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd returned (%s)", 
                                        FMC_DEBUG_FmTxStatusStr(status)));
    
    FMC_FUNC_END();
}
/*
*   Get the interrupt status register, after reading the status register.
*/

FmcFwIntMask _FM_TX_SM_HandlerInterrupt_GetInterruptStatusRegCmd( void *eventData)
{
    FmcFwIntMask mask=0;
    _FmTxSmTransportEventData *transportEventData = NULL;

    FMC_FUNC_START("_FM_TX_SM_HandlerInterrupt_GetInterruptStatusRegCmd");
    transportEventData = eventData;
    
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_RUNNING_PROCESS);

    mask = (FmcFwIntMask)FMC_UTILS_BEtoHost16(&transportEventData->data[0]);
    
    FMC_FUNC_END();
    return mask;
}
/*
*   set the Interrupt mask register with the interrupts in the imtMask
*/
void _FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd( void)
{
    FmcStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerInterrupt_SendReadInterruptStatusRegCmd");

    
    status = _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET,_fmTxSmIntMask.intMask);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd: _FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd returned (%s)", 
                                        FMC_DEBUG_FmTxStatusStr(status)));
    
    FMC_FUNC_END();
}
/*
*   Update the state of the SM.
*/
void _FM_TX_SM_UpdateState(_FmTxSmState state)
{
    _fmTxSmData.currCmdInfo.smState = state;
}
/*
*   Get the state of the SM.
*/
_FmTxSmState _FM_TX_SM_getState(void)
{
    return _fmTxSmData.currCmdInfo.smState;
}
/*
*
*/
FmcStatus _FM_TX_SM_UptadeSmStateSendWriteCommand(FmcFwOpcode fmOpcode, FMC_U32 fmCmdParms)
{
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    return FMC_CORE_SendWriteCommand(fmOpcode, (FMC_U16)fmCmdParms);
}
/*
*
*/
FmcStatus _FM_TX_SM_UptadeSmStateSendReadCommand(FmcFwOpcode fmOpcode)
{
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    return FMC_CORE_SendReadCommand(fmOpcode,2);
}

void _FM_TX_SM_HandleRecievedInterrupts(FmcFwIntMask mask)
{
    FMC_VERIFY_FATAL_NORET(!(mask & FMC_FW_MASK_MAL),("_FM_TX_SM Recieved Hardware Malfunction"));

    /* Insert Interrupts handlers to the processes queue */
    _fmTxSmIntMask.intRecieved = mask;
    if(_FM_TX_SM_getState() == _FM_TX_SM_STATE_WAITING_FOR_INTERRUPT)
    {
        if(!(mask&_fmTxSmIntMask.intWaitingFor))
        {
            /* Move the stage to the process stage of enabling the interrupts! */
            _fmTxSmIntMask.intMask = (FMC_U16)(FMC_FW_MASK_MAL | _fmTxSmIntMask.intWaitingFor);
            _FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd();
        }
        /* Insert Interrupts handlers to the processes queue */
    }
    else
    {
        /*enable interrupts needed but not received*/
        /* enable all interrupts that are not in the queue */
        /* Insert Interrupts handlers to the processes queue */
    }
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_RUNNING_PROCESS);
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}

/*######################################################################################################################
########################################################################################################################
    Precesses
########################################################################################################################
######################################################################################################################*/


/*
########################################################################################################################
    Enable Process
########################################################################################################################
*/
void _FM_TX_SM_HandlerEnable_Start(FMC_UINT event, void *eventData)
{
    FmTxStatus status = FM_TX_STATUS_SUCCESS;

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_Start");

    FMC_UNUSED_PARAMETER(eventData);
    
    FMC_VERIFY_FATAL((event == _FM_TX_SM_EVENT_START), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerEnable_Start: Unexpected Event (%d)", event));

    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_ENABLING;
    
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_RUNNING_PROCESS);
    /* Direct transport events to me from now on */
    status = FMC_CORE_SetCallback(_FM_TX_SM_TransportEventCb);


    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_SUCCESS),
                                    ("_FM_TX_SM_HandlerEnable_Start: FMC_CORE_SetCallback Failed (%s)", 
                                    FMC_DEBUG_FmTxStatusStr(status)));

    /* Make sure the chip is ready for FM module on command */
    status = FMC_CORE_TransportOn();

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_LONG_CMD);
    
    if (status == FMC_STATUS_SUCCESS)
    {
        /* Stage completed synchronously => skip the waiting for transport on completion stage */
        _fmTxSmData.currCmdInfo.stageIndex += 2;

        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    else if (status == FMC_STATUS_PENDING)
    {
        /* Exist and wait for the transport event to continue the process */
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        FMC_FATAL_NO_RETVAR(("_FM_TX_SM_HandlerEnable_Start: FMC_CORE_TransportOn Failed"));
    }
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerEnable_SendFmOnCmd( FMC_UINT event, void *eventData)
{
    FmTxStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_Start");

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    
    status = FMC_CORE_SendPowerModeCommand(FMC_TRUE);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING),
                                    ("_FM_TX_SM_HandlerEnable_SendFmOnCmd: FMC_CORE_SendPowerModeCommand returned (%s)", 
                                    FMC_DEBUG_FmTxStatusStr(status)));
    
    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerEnable_SendReadFmAsicIdCmd( FMC_UINT event, void *eventData)
{
    FmcStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_SendReadFmAsicIdCmd");
    /* Must wait 20msec before starting to send commands to the FM after command
     * FMC_FW_FM_CORE_POWER_UP was sent. */
    FMC_OS_Sleep(FMC_CONFIG_WAKEUP_TIMEOUT_MS);
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    status = _FM_TX_SM_UptadeSmStateSendReadCommand(FMC_FW_OPCODE_CMN_ASIC_ID_GET);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerEnable_SendReadFmAsicIdCmd: _FM_TX_SM_UptadeSmStateSendReadCommand returned (%s)", 
                                        FMC_DEBUG_FmTxStatusStr(status)));
    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerEnable_WaitReadFmAsicIdCmdComplete( FMC_UINT event, void *eventData)
{
    _FmTxSmTransportEventData *transportEventData = NULL;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_WaitReadFmAsicIdCmdComplete");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    transportEventData = eventData;

    /* Extract FW Version from response and convert from BE */
    _fmTxSmData.context.fwCache.asicId=FMC_UTILS_BEtoHost16(&transportEventData->data[0]);
    
    /* Stage completed synchronously => move to the appropriate next stage in the process */
    _fmTxSmData.currCmdInfo.stageIndex++;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);

    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerEnable_SendReadFmAsicVersionCmd( FMC_UINT event, void *eventData)
{
    FmcStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_SendReadFmAsicVersionCmd");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    status = _FM_TX_SM_UptadeSmStateSendReadCommand(FMC_FW_OPCODE_CMN_ASIC_VER_GET);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerEnable_SendReadFmAsicVersionCmd: _FM_TX_SM_UptadeSmStateSendReadCommand returned (%s)", 
                                        FMC_DEBUG_FmTxStatusStr(status)));
    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerEnable_WaitReadFmAsicVersionCmdComplete( FMC_UINT event, void *eventData)
{
    _FmTxSmTransportEventData *transportEventData = NULL;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_WaitReadFmAsicVersionCmdComplete");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    transportEventData = eventData;

    /* Extract FW Version from response and convert from BE */
    _fmTxSmData.context.fwCache.asicVersion=FMC_UTILS_BEtoHost16(&transportEventData->data[0]);
    
    /* Stage completed synchronously => move to the appropriate next stage in the process */
    _fmTxSmData.currCmdInfo.stageIndex++;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);

    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerEnable_StartFmInitScriptExec( FMC_UINT event, void *eventData)
{

    _CcmImStatus                status=_CCM_IM_STATUS_SUCCESS;
    McpBtsSpStatus              btsSpStatus;
    McpBtsSpScriptLocation      scriptLocation;
    McpBtsSpExecuteScriptCbData scriptCbData;
    char                        fileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS *
                                         MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];
    McpUtf8                     scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                                                   MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_StartFmInitScriptExec");

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Neptune Warning cleanup */
    FMC_UNUSED_PARAMETER(status);

    MCP_HAL_STRING_Sprintf(fileName,
                           "%s_%x.%d.bts",
                           FMC_CONFIG_SCRIPT_FILES_FMC_INIT_NAME, 
                           _fmTxSmData.context.fwCache.asicId,
                           _fmTxSmData.context.fwCache.asicVersion);

    MCP_StrCpyUtf8(scriptFullFileName,
                   (const McpUtf8 *)FMC_CONFIG_SCRIPT_FILES_FULL_PATH_LOCATION);
    MCP_StrCatUtf8(scriptFullFileName, (const McpUtf8 *)fileName);
    
    scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    scriptLocation.locationData.fullFileName = scriptFullFileName;
    
    scriptCbData.sendHciCmdCb = _FM_TX_SM_TiSpSendHciScriptCmd;
    scriptCbData.setTranParmsCb = NULL;
    scriptCbData.execCompleteCb = _FM_TX_SM_TiSpExecuteCompleteCb;
    
    btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmTxSmData.context.tiSpContext);

    if (btsSpStatus == MCP_BTS_SP_STATUS_FILE_NOT_FOUND)
    {       
        /* Script was not found in FFS => check in ROM */
        
        McpBool memInitScriptFound;
        char scriptFileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS];

        FMC_LOG_INFO(("%s Not Found on FFS - Checking Memory", scriptFullFileName));

        MCP_HAL_STRING_Sprintf((char *)scriptFileName,
                               "%s_%x.%d.bts", 
                               FMC_CONFIG_SCRIPT_FILES_FMC_INIT_NAME,
                               _fmTxSmData.context.fwCache.asicId,
                               _fmTxSmData.context.fwCache.asicVersion);

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
            btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmTxSmData.context.tiSpContext);
        }

        /* Now continue checking the return status... */
    }
    
    if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Stage completed synchronously => skip the waiting for script completion stage */
        _fmTxSmData.currCmdInfo.stageIndex += 2;
        
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
    {
        _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
            /* Exist and wait for the transport event to continue the process */
            _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        FMC_FATAL(FM_TX_STATUS_INTERNAL_ERROR, ("_FM_TX_SM_HandlerEnable_StartFmInitScriptExec: FMC_CORE_TransportOn Failed"));
    }
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerEnable_WaitFmInitScriptExecComplete( FMC_UINT event, void *eventData)
{
    McpBtsSpStatus      scriptExecStatus;
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* Extract the script execution completion status */
    scriptExecStatus = (McpBtsSpStatus)eventData;

    if (scriptExecStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Script execution completed successfully => move to next stage */
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    /* Script execution failed =>  update enable completion status and abort the enabling process */
    else
    {
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_SCRIPT_EXEC_FAILED;
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_ENABLE_COMPLETE;
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
void _FM_TX_SM_HandlerEnable_StartFmTxInitScriptExec( FMC_UINT event, void *eventData)
{

    _CcmImStatus                status=_CCM_IM_STATUS_SUCCESS;
    McpBtsSpStatus              btsSpStatus;
    McpBtsSpScriptLocation      scriptLocation;
    McpBtsSpExecuteScriptCbData scriptCbData;
    char                        fileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS *
                                         MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];
    McpUtf8                     scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                                                   MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];
    
    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_StartFmInitScriptExec");

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /*Neptune Warning cleanup */
    FMC_UNUSED_PARAMETER(status);

    MCP_HAL_STRING_Sprintf(fileName,
                           "%s_%x.%d.bts",
                           FMC_CONFIG_SCRIPT_FILES_FM_TX_INIT_NAME,
                           _fmTxSmData.context.fwCache.asicId,
                           _fmTxSmData.context.fwCache.asicVersion);
    
    MCP_StrCpyUtf8(scriptFullFileName,
                   (const McpUtf8 *)FMC_CONFIG_SCRIPT_FILES_FULL_PATH_LOCATION);
    MCP_StrCatUtf8(scriptFullFileName, (const McpUtf8 *)fileName);

    scriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    scriptLocation.locationData.fullFileName = scriptFullFileName;
    
    scriptCbData.sendHciCmdCb = _FM_TX_SM_TiSpSendHciScriptCmd;
    scriptCbData.setTranParmsCb = NULL;
    scriptCbData.execCompleteCb = _FM_TX_SM_TiSpExecuteCompleteCb;
    
    btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmTxSmData.context.tiSpContext);

    if (btsSpStatus == MCP_BTS_SP_STATUS_FILE_NOT_FOUND)
    {       
        /* Script was not found in FFS => check in ROM */
        
        McpBool memInitScriptFound;
        char scriptFileName[MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS];

        FMC_LOG_INFO(("%s Not Found on FFS - Checking Memory", scriptFullFileName));

        MCP_HAL_STRING_Sprintf(scriptFileName,
                               "%s_%x.%d.bts", 
                               FMC_CONFIG_SCRIPT_FILES_FM_TX_INIT_NAME,
                               _fmTxSmData.context.fwCache.asicId,
                               _fmTxSmData.context.fwCache.asicVersion);

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
            btsSpStatus = MCP_BTS_SP_ExecuteScript(&scriptLocation, &scriptCbData, &_fmTxSmData.context.tiSpContext);
        }

        /* Now continue checking the return status... */
    }
    
    if (btsSpStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Stage completed synchronously => skip the waiting for script completion stage */
        _fmTxSmData.currCmdInfo.stageIndex += 2;
        
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    else if (btsSpStatus == MCP_BTS_SP_STATUS_PENDING)
    {
        _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
            /* Exist and wait for the transport event to continue the process */
            _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        FMC_FATAL(FM_TX_STATUS_INTERNAL_ERROR, ("_FM_TX_SM_HandlerEnable_StartFmInitScriptExec: FMC_CORE_TransportOn Failed"));
    }
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerEnable_WaitFmTxInitScriptExecComplete( FMC_UINT event, void *eventData)
{
    McpBtsSpStatus      scriptExecStatus;
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* Extract the script execution completion status */
    scriptExecStatus = (McpBtsSpStatus)eventData;

    if (scriptExecStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Script execution completed successfully => move to next stage */
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    /* Script execution failed =>  update enable completion status and abort the enabling process */
    else
    {
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_SCRIPT_EXEC_FAILED;
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_ENABLE_COMPLETE;
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
void _FM_TX_SM_HandlerEnable_SendFmTxOnCmd( FMC_UINT event, void *eventData)
{
    FmcStatus status=FMC_STATUS_FAILED;

    FMC_FUNC_START("_FM_TX_SM_HandlerEnable_SendFmTxOnCmd");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Send TX Power UP command */
    status = _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_POWER_UP_DOWN_SET, FMC_FW_TX_POWER_UP);
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_PENDING), FM_TX_STATUS_INTERNAL_ERROR, 
                        ("_FM_TX_SM_HandlerEnable_SendFmTxOnCmd"));
    _fmTxSmData.currCmdInfo.stageIndex++;

    FMC_FUNC_END();
}


void _FM_TX_SM_HandlerEnable_ApplyDfltConfig( FMC_UINT event, void *eventData)
{
    FMC_STATIC FMC_U8 configStage = 0;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    if(configStage< sizeof(_fmTxEnableDefualtSimpleCommandsToSet)/sizeof(_FmTxSmDefualtConfigValue))
    {
            _FM_TX_SM_UptadeSmStateSendWriteCommand(
                    _fmTxSmSimpleSetCmd2FwOpcodeMap[(_fmTxEnableDefualtSimpleCommandsToSet[configStage]).cmdType], 
                    (FMC_U16)(_fmTxEnableDefualtSimpleCommandsToSet[configStage]).defualtValue);
        /* config the current value and move to the value to configure*/
        configStage++;
    }
    else
    {   
        /*All simple defualt values are configured continue*/
        configStage = 0;
        _fmTxSmData.currCmdInfo.stageIndex++;
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    
}
void _FM_TX_SM_HandlerEnable_Complete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    

    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_ENABLED;

    FMCI_OS_CancelTimer();
    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,NULL, eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}

/*
########################################################################################################################
    Disable
########################################################################################################################
*/

void _FM_TX_SM_HandlerDisable_Start(FMC_UINT event, void *eventData)
{
    FmTxStatus status = FM_TX_STATUS_SUCCESS;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerDisable_Start");

    FMC_UNUSED_PARAMETER(eventData);
    
    /*Neptune Warrning cleanup */
    FMC_UNUSED_PARAMETER(status);

    if(event != _FM_TX_SM_EVENT_START)

    FMC_VERIFY_FATAL((event == _FM_TX_SM_EVENT_START), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerDisable_Start: Unexpected Event (%d)", event));

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_LONG_CMD);

    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_DISABLING;
    
    _fmTxSmData.currCmdInfo.stageIndex++;

      /*Remove the VAC resources if allocated
        Since this is a tx operation we know it will end sync */
     CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_TX);

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerDisable_SendFmOffCmd( FMC_UINT event, void *eventData)
{
    FmTxStatus status;

    FMC_FUNC_START("_FM_TX_SM_HandlerDisable_SendFmOffCmd");

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    status = FMC_CORE_SendPowerModeCommand(FMC_FALSE);
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_PENDING), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerDisable_SendFmOffCmd: FMC_CORE_SendPowerModeCommand returned (%s)", 
                            FMC_DEBUG_FmTxStatusStr(status)));
    
    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}


void _FM_TX_SM_HandlerDisable_SendTransportFmOffCmd( FMC_UINT event, void *eventData)
{
    FmTxStatus status;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerDisable_SendTransportFmOffCmd");
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    /* Make sure the chip is ready for FM module on command */
    status = FMC_CORE_TransportOff();

    if (status == FMC_STATUS_SUCCESS)
    {
        /* Stage completed synchronously => move to the appropriate next stage in the process */
        _fmTxSmData.currCmdInfo.stageIndex += 2;

        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    }
    else if (status == FMC_STATUS_PENDING)
    {
        /* Exist and wait for the transport event to continue the process */
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        FMC_FATAL_NO_RETVAR(("_FM_TX_SM_HandlerDisable_SendTransportFmOffCmd: FMC_CORE_TransportOff Failed"));
    }

    FMC_FUNC_END();
}


void _FM_TX_SM_HandlerDisable_Complete( FMC_UINT event, void *eventData)
{
    FmTxStatus  status=FM_TX_STATUS_FAILED;

    FMC_FUNC_START("_FM_TX_SM_HandlerDisable_Complete");

    FMC_UNUSED_PARAMETER(event);

    /*Neptune Warrning cleanup */
    FMC_UNUSED_PARAMETER(status);

    FMCI_OS_CancelTimer();
    /* Direct transport events to me from now on */
    status = FMC_CORE_SetCallback(NULL);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_SUCCESS),
                                    ("_FM_TX_SM_HandlerDisable_Complete: FMC_CORE_SetCallback Failed (%s)", 
                                        FMC_DEBUG_FmTxStatusStr(status)));

    /* Stop directing FM task events to the TX's task event handler */
    status = FMCI_SetEventCallback(NULL);
    FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
                        ("FM_TX_Enable: FMCI_SetEventCallback Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));
    recievedDisable = FMC_FALSE;
    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_DISABLED;
    _fmTxSmData.context.fwCache.transmissionOn = FMC_FALSE;

    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,NULL, eventData);

    /* 
        We do not trigger the FM task as we do at the end of all other commands:
        1. After disabling FM TX there must not be any pending commands in the queue
        2. We have just cleared the FMCI callback => we will not receive events anyway
    */
    
    FMC_FUNC_END();
}
/*
########################################################################################################################
    Tune
########################################################################################################################
*/

void _FM_TX_SM_HandlerTune_Start( FMC_UINT event, void *eventData)
{

    FmTxStatus status = FM_TX_STATUS_SUCCESS;

    FMC_FUNC_START("_FM_TX_SM_HandlerTune_Start");

    FMC_UNUSED_PARAMETER(eventData);
    
    /*Neptune Warrning cleanup */
    FMC_UNUSED_PARAMETER(status);
    
    FMC_VERIFY_FATAL((event == _FM_TX_SM_EVENT_START), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerTune_Start: Unexpected Event (%d)", event));

    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_RUNNING_PROCESS);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);
    /* 
        We can tune only when transmission is off. Therefore, if it is on, we must first stop it, and
        resume when we complete tuning
    */
    if (_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)
    {
        /* Record the fact that we need to resume transmission */
        _fmTxSmData.context.transmissionStoppedForTune = FMC_TRUE;

        /* Move to transmission off stage */
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        /* No need to resume transmission later */
        _fmTxSmData.context.transmissionStoppedForTune = FMC_FALSE;

        /* Skip transmission off stages */
        _fmTxSmData.currCmdInfo.stageIndex += 6;
    }
    
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    
    FMC_FUNC_END();
}
/*_FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd*/
/*_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue*/
/*_FM_TX_SM_HandlerStopTransmission_SendTransmissionOffCmd*/
/*_FM_TX_SM_HandlerStopTransmission_WaitTransmissionOffCmdComplete*/
/*_FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt*/

void _FM_TX_SM_HandlerTune_SendSetInterruptMaskCmd( FMC_UINT event, void *eventData)
{
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _fmTxSmIntMask.intMask = FMC_FW_MASK_MAL|FMC_FW_MASK_FR|FMC_FW_MASK_INVALID_PARAM;
    
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_CMN_INT_MASK_SET_GET , _fmTxSmIntMask.intMask);

    _fmTxSmData.currCmdInfo.stageIndex++;
}
/*_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},*/

void _FM_TX_SM_HandlerTune_SendTuneCmd( FMC_UINT event, void *eventData)
{
    FmcFwCmdParmsValue  channelIndex;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Get the channel index to set from the command parameters */
    channelIndex = _FM_TX_SM_HandlerTune_GetCmdParmValue();
    _fmTxSmIntMask.intWaitingFor = FMC_FW_MASK_FR|FMC_FW_MASK_INVALID_PARAM;
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_CHANL_SET_GET, channelIndex);

    _fmTxSmData.currCmdInfo.stageIndex++;
}

void _FM_TX_SM_HandlerTune_WaitTuneCmdComplete( FMC_UINT event, void *eventData)
{
    FmTxTuneCmd *tuneCmd = (FmTxTuneCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);


    /* Record the new tuned frequency (record the frequency, not the FW index) in the cache */
    _fmTxSmData.context.fwCache.tunedFreq = tuneCmd->freq;

    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_INTERRUPT);
    
    _fmTxSmData.currCmdInfo.stageIndex++;
}
void _FM_TX_SM_HandlerTune_WaitTunedInterrupt( FMC_UINT event, void *eventData)
{
    FmTxTuneCmd *tuneCmd = (FmTxTuneCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    
    FMC_UNUSED_PARAMETER(tuneCmd);

    /* Record the new tuned frequency (record the frequency, not the FW index) in the cache */
    
    _fmTxSmIntMask.intMask &= ~FMC_FW_MASK_FR;
    if(_fmTxSmIntMask.intRecieved&FMC_FW_MASK_INVALID_PARAM)
    {
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_TUNE_COMPLETE;
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INVALID_PARM;
    }
    else
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
/*
    According to Manav's directions, we must set the audio path AFTER setting the channel
    
[ToDo] - This stage must be adapted to the audio routing method that will be adopted in the future. Currently
we always set the audio source to analog
*/
void _FM_TX_SM_HandlerTune_SendInterruptMaskSetCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _FM_TX_SM_HandlerInterrupt_SendSetInterruptMaskRegCmd();

    _fmTxSmData.currCmdInfo.stageIndex++;
}
void _FM_TX_SM_HandlerTune_WaitInterruptMaskSetCmdComplete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);


    if (_fmTxSmData.context.transmissionStoppedForTune == FMC_FALSE)
    {
        /* No need to restart, skip these stages */
        _fmTxSmData.currCmdInfo.stageIndex += 10;
    }
    else
    {
        /* Need to restart, move to that stage */
    _fmTxSmData.currCmdInfo.stageIndex++;
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
/*
_FM_TX_SM_HandlerStartTransmission_SendAudioIoSetCmd},
_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
_FM_TX_SM_HandlerStartTransmission_SendPowerLevelSetCmd},
_FM_TX_SM_HandlerStartTransmission_WaitPowerLevelSetCmdComplete},
_FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd},
_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
_FM_TX_SM_HandlerStartTransmission_SendTransmissionOnCmd},
_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},
_FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt}
,*/
void _FM_TX_SM_HandlerTune_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerTune_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    /* Complete the process and set the tuned frequency in the event */
    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _FM_TX_SM_HandlerTune_FillEventData, 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();
}
/***************************************************************************************************************************************************
***************************************************************************************************************************************************/
void _FM_TX_SM_HandlerTune_UpdateCache(void)
{
    FmTxTuneCmd *tuneCmd = (FmTxTuneCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _fmTxSmData.context.fwCache.tunedFreq = tuneCmd->freq;
}

void _FM_TX_SM_HandlerTune_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    
    event->p.cmdDone.v.value = (FMC_U32)(_fmTxSmData.context.fwCache.tunedFreq);
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerTune_GetCmdParmValue(void)
{
    FmTxTuneCmd     *tuneCmd = (FmTxTuneCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FMC_U16)(tuneCmd->freq/10);
}
/*
########################################################################################################################
    Start VAC related
########################################################################################################################
*/


/*
########################################################################################################################
    Change audio resources
########################################################################################################################
*/

void _FM_TX_SM_HandlerChangeAudioSource_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);  

}
void _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmd( FMC_UINT event, void *eventData)
{
    TCAL_DigitalConfig ptConfig;
    TCAL_ResourceProperties pProperties;
    ECAL_ResourceMask resource;

    FmTxSetAudioSourceCmd *setAudioSourceCmd = (FmTxSetAudioSourceCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    

    /* get the digital configuration params*/
    ptConfig.eSampleFreq = setAudioSourceCmd->eSampleFreq;
    ptConfig.uChannelNumber = _getChannelNumber(_fmTxSmData.context.vacParams.monoStereoMode);
    /* get the audio source*/
    resource = _convertTxSourceToCal(setAudioSourceCmd->txSource,&pProperties);
    /*set the properties - this will determin inside the cal if the i2s or pcm will be enabled*/
    CCM_VAC_SetResourceProperties(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),
                                    CAL_RESOURCE_FMIF,  
                                    &pProperties);
    /* change the resource and handle sync and async complition*/
    _FM_TX_SM_HandleVacStatusAndMove2NextStage( CCM_VAC_ChangeResource(
                                                    CCM_GetVac(
                                                    _FMC_CORE_GetCcmObjStackHandle()),
                                                    CAL_OPERATION_FM_TX,
                                                    resource,
                                                    &ptConfig,
                                                    &_fmTxSmData.context.vacParams.ptUnavailResources),
                                                    eventData);

}
void _FM_TX_SM_HandlerChangeAudioSource_SendChangeResourceCmdComplete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    
    if( _fmTxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {
        /* if transmiting enable audio acording to source*/
        if(_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)
        {
            /* move to next stage - which is: enable audio*/
            _fmTxSmData.currCmdInfo.stageIndex++;
        }
        else
        {
            /*no need to enable audio go to last handler*/
            _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
        }
    }
    else 
    {
        /* indicate that the operation failed and go to last handler*/
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INTERNAL_ERROR;
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);
}
void _FM_TX_SM_HandlerChangeAudioSource_SendEnableAudioSource( FMC_UINT event, void *eventData)
{
    FmTxSetAudioSourceCmd *setAudioSourceCmd = (FmTxSetAudioSourceCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* enable the digital or analog source*/
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_AUDIO_IO_SET, _getAudioIoParam(setAudioSourceCmd->txSource));

    _fmTxSmData.currCmdInfo.stageIndex++;
}
void _FM_TX_SM_HandlerChangeAudioSource_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerChangeAudioSource_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();

}
void _FM_TX_SM_HandlerChangeAudioSource_UpdateCache(void)
{
    FmTxSetAudioSourceCmd *setAudioSourceCmd = (FmTxSetAudioSourceCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    /* save the new configuration*/
    _fmTxSmData.context.vacParams.eSampleFreq = setAudioSourceCmd->eSampleFreq;
    _fmTxSmData.context.vacParams.audioSource = setAudioSourceCmd->txSource;
}
void _FM_TX_SM_HandlerChangeAudioSource_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    FMC_UNUSED_PARAMETER(event);
    if(_fmTxSmData.currCmdInfo.status == FM_TX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES )
    {
        /* if resources unavailble give a pointer to the app*/
        event->p.cmdDone.v.audioOperation.ptUnavailResources = &_fmTxSmData.context.vacParams.ptUnavailResources;
    }
    
}
/*
########################################################################################################################
    Change Digital audio Configuration
########################################################################################################################
*/
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    if(_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        /* no need to configure anything if transmission was not started*/
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);  

}
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendChangeConfigurationCmd( FMC_UINT event, void *eventData)
{
    TCAL_DigitalConfig ptConfig;
    FmTxSetDigitalAudioConfigurationCmd *setAudioConfigurationCmd = (FmTxSetDigitalAudioConfigurationCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* get digital source configuration */
    ptConfig.eSampleFreq = setAudioConfigurationCmd->eSampleFreq;
    ptConfig.uChannelNumber = _getChannelNumber(_fmTxSmData.context.vacParams.monoStereoMode);

    /* change configuration of digital source and handle vac return status sync and async*/
    _FM_TX_SM_HandleVacStatusAndMove2NextStage(CCM_VAC_ChangeConfiguration(
                                                    CCM_GetVac(
                                                    _FMC_CORE_GetCcmObjStackHandle()),
                                                    CAL_OPERATION_FM_TX,
                                                    &ptConfig),
                                                    eventData);
}
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_ChangeDigitalAudioConfigCmdComplete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    
    if( _fmTxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {
        /* if transmiting enable audio acording to source*/
        if(_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)
        {
            /* move to next stage - which is: enable audio*/
            _fmTxSmData.currCmdInfo.stageIndex++;
        }
        else
        {
            /*no need to enable audio go to last handler*/
            _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
        }
    }
    else 
    {
        /* indicate that the operation failed and go to last handler*/
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INTERNAL_ERROR;
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);
}

void _FM_TX_SM_HandlerChangeDigitalAudioConfig_SendEnableAudioSource( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* enable the digital or analog source*/
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_AUDIO_IO_SET, _getAudioIoParam(_fmTxSmData.context.vacParams.audioSource));

    _fmTxSmData.currCmdInfo.stageIndex++;
}
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_Complete( FMC_UINT event, void *eventData)
{

    FMC_FUNC_START("_FM_TX_SM_HandlerChangeDigitalAudioConfig_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();

}
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_UpdateCache(void)
{
    FmTxSetDigitalAudioConfigurationCmd *setAudioConfigurationCmd = (FmTxSetDigitalAudioConfigurationCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    _fmTxSmData.context.vacParams.eSampleFreq = setAudioConfigurationCmd->eSampleFreq;
}
void _FM_TX_SM_HandlerChangeDigitalAudioConfig_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    FMC_UNUSED_PARAMETER(event);
    
}
/*
########################################################################################################################
    Mono Stereo Mode
########################################################################################################################
*/
void _FM_TX_SM_HandlerChangeMonoStereoMode_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    /* if transmiting and we work in digital mode call vac to chage configuration*/
    if((_fmTxSmData.context.state == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)&&
        (_fmTxSmData.context.vacParams.audioSource!=FM_TX_AUDIO_SOURCE_FM_ANALOG))
    {
        /* move to next stage - which is: enable audio*/
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        /*no need to change configuration simulate success and move to set mono stereo */
        _fmTxSmData.context.transportEventData.status = FMC_STATUS_SUCCESS;
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_CHANGE_MONO_STEREO_MODE_SET_MONO_STEREO_MODE_CMD;
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);  

}
void _FM_TX_SM_HandlerChangeMonoStereoMode_SendChangeConfigurationCmd( FMC_UINT event, void *eventData)
{
    TCAL_DigitalConfig ptConfig;
    FmTxSetMonoStereoModeCmd    *setMonoModeCmd = (FmTxSetMonoStereoModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* get digital source configuration */
    ptConfig.eSampleFreq = _fmTxSmData.context.vacParams.eSampleFreq;
    ptConfig.uChannelNumber = _getChannelNumber(setMonoModeCmd->monoMode);

    /* change configuration of digital source and handle vac return status sync and async*/
    _FM_TX_SM_HandleVacStatusAndMove2NextStage(CCM_VAC_ChangeConfiguration(
                                                    CCM_GetVac(
                                                    _FMC_CORE_GetCcmObjStackHandle()),
                                                    CAL_OPERATION_FM_TX,
                                                    &ptConfig),
                                                    eventData);
}
void _FM_TX_SM_HandlerChangeMonoStereoMode_SetMonoModeCmd( FMC_UINT event, void *eventData)
{
    FmTxSetMonoStereoModeCmd    *setMonoModeCmd = (FmTxSetMonoStereoModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* if changing digital audio operation configuration succeeded - configure the fm mono stereo mode*/
    if(_fmTxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {
        _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_MONO_SET_GET, setMonoModeCmd->monoMode);
        
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        /* mark operation as failed and complete operation*/
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INTERNAL_ERROR;
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);  
    }

}
void _FM_TX_SM_HandlerChangeMonoStereoMode_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerChangeMonoStereoMode_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();

}
void _FM_TX_SM_HandlerChangeMonoStereoMode_UpdateCache(void)
{
    FmTxSetMonoStereoModeCmd    *setMonoModeCmd = (FmTxSetMonoStereoModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    _fmTxSmData.context.vacParams.monoStereoMode = setMonoModeCmd->monoMode;
}
void _FM_TX_SM_HandlerChangeMonoStereoMode_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    event->p.cmdDone.v.value = (FMC_U32)(_fmTxSmData.context.vacParams.monoStereoMode);
}
/*
########################################################################################################################
    End VAC related
########################################################################################################################
*/
/*
########################################################################################################################
    Start Transmission
########################################################################################################################
*/

void _FM_TX_SM_HandlerStartTransmission_Start( FMC_UINT event, void *eventData)
{

    TCAL_DigitalConfig ptConfig;
    
    
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* get the digital configuration params*/
    ptConfig.eSampleFreq = _fmTxSmData.context.vacParams.eSampleFreq;
    ptConfig.uChannelNumber = _getChannelNumber(_fmTxSmData.context.vacParams.monoStereoMode);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _FM_TX_SM_HandleVacStatusAndMove2NextStage(CCM_VAC_StartOperation(
                                                    CCM_GetVac(
                                                    _FMC_CORE_GetCcmObjStackHandle()),
                                                    CAL_OPERATION_FM_TX,
                                                    &ptConfig,
                                                    &_fmTxSmData.context.vacParams.ptUnavailResources),
                                                    eventData);
    
}
void _FM_TX_SM_HandlerStartTransmission_VacStartOperationCmdComplete( FMC_UINT event, void *eventData)
{

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    if( _fmTxSmData.context.transportEventData.status == FMC_STATUS_SUCCESS)
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
    }
    else
    {
        /* if starting the operation failed end the command and report internal error.*/
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INTERNAL_ERROR;
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);

}
/******************************************************************************************************************************************************************
*   Start Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
void _FM_TX_SM_HandlerStartTransmission_SendAudioIoSetCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_AUDIO_IO_SET, _getAudioIoParam(_fmTxSmData.context.vacParams.audioSource));
    
    _fmTxSmData.currCmdInfo.stageIndex++;
}
/*_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},*/
void _FM_TX_SM_HandlerStartTransmission_SendPowerLevelSetCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* 
        [ToDo] For the demo there is a stage here that sets the power level to maximum (0) - However
        this stage should not be part of the tune sequence (unless Manav says that power level must be
        re-set every time transmission is off or the channel is set
    */
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_POWER_LEVEL_SET_GET, _fmTxSmData.context.fwCache.powerLevel);


    _fmTxSmData.currCmdInfo.stageIndex++;

}

void _FM_TX_SM_HandlerStartTransmission_WaitPowerLevelSetCmdComplete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);


    _fmTxSmData.currCmdInfo.stageIndex++;

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
/*
_FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd}
_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},,
*/
void _FM_TX_SM_HandlerStartTransmission_SendTransmissionOnCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    _fmTxSmIntMask.intWaitingFor = FMC_FW_MASK_POW_ENB;

    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_POWER_ENB_SET, FMC_FW_TX_POWER_ENABLE);

    _fmTxSmData.currCmdInfo.stageIndex++;
}
/*_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},*/
/******************************************************************************************************************************************************************
*   End Sub sequence of Start transmission - used also in Tune
******************************************************************************************************************************************************************/
/*_FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},*/

void _FM_TX_SM_HandlerStartTransmission_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerStartTransmission_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandlerStartTransmission_CmdComplete();
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerStartTransmission_CmdComplete(void)
{
    if(_fmTxSmData.currCmdInfo.status == FM_TX_STATUS_SUCCESS)
    {
        _fmTxSmData.context.fwCache.transmissionOn = FMC_TRUE;
        _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_TRANSMITTING;
    }
}
void _FM_TX_SM_HandlerStartTransmission_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    FMC_UNUSED_PARAMETER(event);
    if(_fmTxSmData.currCmdInfo.status==FM_TX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES )
    {
        event->p.cmdDone.v.audioOperation.ptUnavailResources = &_fmTxSmData.context.vacParams.ptUnavailResources;
    }
    
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerStartTransmission_GetCmdParmValue(void)
{
    return FMC_FW_TX_POWER_ENABLE;
}
/*
########################################################################################################################
    StopTransmission
########################################################################################################################
*/

void _FM_TX_SM_HandlerStopTransmission_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _FM_TX_SM_HandleVacStatusAndMove2NextStage(CCM_VAC_StopOperation(CCM_GetVac(_FMC_CORE_GetCcmObjStackHandle()),CAL_OPERATION_FM_TX),eventData);
    
}
/******************************************************************************************************************************************************************
*   Start Sub sequence of Stop transmission - used also in Tune
******************************************************************************************************************************************************************/
/*
_FM_TX_SM_HandlerGeneral_SendSetPowEnbIntMaskCmd}
_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},,
*/
void _FM_TX_SM_HandlerStopTransmission_SendTransmissionOffCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Send TX Power DOWN command */
    _fmTxSmIntMask.intWaitingFor = FMC_FW_MASK_POW_ENB;
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_POWER_ENB_SET, FMC_FW_TX_POWER_DISABLE);

    _fmTxSmData.currCmdInfo.stageIndex++;
}

void _FM_TX_SM_HandlerStopTransmission_WaitTransmissionOffCmdComplete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_INTERRUPT);

}
/*_FM_TX_SM_HandlerGeneral_GeneralCmdCompleteAndContinue},*/
/******************************************************************************************************************************************************************
*   End Sub sequence of Stop transmission - used also in Tune
******************************************************************************************************************************************************************/
/*_FM_TX_SM_HandlerGeneral_WaitPowEnbInterrupt},*/

void _FM_TX_SM_HandlerStopTransmission_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerStopTransmission_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            NULL, 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerStopTransmission_CmdComplete(void)
{
    _fmTxSmData.context.fwCache.transmissionOn = FMC_FALSE;
    _fmTxSmData.context.state = FM_TX_SM_CONTEXT_STATE_ENABLED;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerStopTransmission_GetCmdParmValue(void)
{
    return FMC_FW_TX_POWER_DISABLE;
}


/*
########################################################################################################################
    Set Transmission Mode
########################################################################################################################
*/
/*
    Description:
    First RDS Must be disabled
*/
void _FM_TX_SM_HandlerTransmissionMode_Start( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FmcStatus status=FMC_STATUS_SUCCESS;
    FMC_FUNC_START("_FM_TX_SM_HandlerTransmissionMode_Start");
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /*Neptune Warrning cleanup */
    FMC_UNUSED_PARAMETER(status);

    FMC_VERIFY_FATAL((!_fmTxSmData.context.fwCache.rdsEnabled), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerTransmissionMode_Start: transmission must be OFF when changing modes "));

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);
    if(setRdsTransmissionModeCmd->transmissionMode == _fmTxSmData.context.fwCache.transmissionMode)
    {
        /* We are elready in this mode - do nothing*/
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    else
    {
        if(setRdsTransmissionModeCmd->transmissionMode == FMC_RDS_TRANSMISSION_MANUAL)
        {
            /* We entered here if we are in auto mode and changing to menual */

            _fmTxSmData.currCmdInfo.stageIndex++;
            _fmTxSmData.context.rdsText = (FMC_U8 *)"";
            _fmTxSmData.context.rdsTextLen = 0;
            _fmTxSmData.context.rdsGroupText = 1;/*[ToDo Zvi] macros*/

        }
        else if(_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK)
        {
            
            _fmTxSmData.currCmdInfo.stageIndex++;
            _fmTxSmData.context.rdsText = _fmTxSmData.context.fwCache.psMsg;
            _fmTxSmData.context.rdsTextLen =  (FMC_U8)FMC_OS_StrLen(_fmTxSmData.context.fwCache.psMsg);
            _fmTxSmData.context.rdsGroupText = 1;/*[ToDo Zvi] macros*/
        }
        else
        {
        /* we don't need to set the PS go to next step which is to set RT*/
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_TRANSMISSION_MODE_PS_TO_RT;
        }
    }

    
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsPsMsgCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    if(setRdsTransmissionModeCmd->transmissionMode == FMC_RDS_TRANSMISSION_MANUAL)
    {
        /* We entered here if we are in auto mode and changing to menual */

        _fmTxSmData.currCmdInfo.stageIndex++;
        _fmTxSmData.context.rdsText = (FMC_U8 *)"";
        _fmTxSmData.context.rdsTextLen = 0;
        _fmTxSmData.context.rdsGroupText = (FMC_U8)_fmTxSmData.context.fwCache.rtType;/*[ToDo Zvi] macros*/
    }
    else if (_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK)
    {
        /* Set the current RT messege avilible*/
        _fmTxSmData.currCmdInfo.stageIndex++;
        _fmTxSmData.context.rdsText = _fmTxSmData.context.fwCache.rtMsg;
        _fmTxSmData.context.rdsTextLen =  (FMC_U8)FMC_OS_StrLen(_fmTxSmData.context.fwCache.rtMsg);
        _fmTxSmData.context.rdsGroupText = (FMC_U8)_fmTxSmData.context.fwCache.rtType;/*[ToDo Zvi] macros*/
    }
    else /* We are going to AUTO mode but the RT bit in transmitted field mask is not enabled*/
    {
        /* we don't need to set the RT go to next step which is to set ECC*/
        _fmTxSmData.currCmdInfo.stageIndex = _FM_TX_SM_STAGE_TRANSMISSION_MODE_RT_TO_ECC;
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);

}
void _FM_TX_SM_HandlerTransmissionMode_SendSetRdsEccMsgCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_U16 countryCodeParam;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    if(setRdsTransmissionModeCmd->transmissionMode == FMC_RDS_TRANSMISSION_MANUAL)
    {
        _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_ECC_SET_GET, 0);

        _fmTxSmData.currCmdInfo.stageIndex++;
        
    }
    else if(_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK)
    {
        /* Enable and set the defualt ECC */
        countryCodeParam = (FMC_U16)((_fmTxSmData.context.fwCache.countryCode<<8)|(0x0001));
        _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_ECC_SET_GET, countryCodeParam);
        _fmTxSmData.currCmdInfo.stageIndex++;

    }
    else/* We are going to AUTO mode but the RT bit in transmitted field mask is not enabled*/
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);
    }
}

void _FM_TX_SM_HandlerTransmissionMode_WaitSetRdsEccMsgCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    if(setRdsTransmissionModeCmd->transmissionMode == FMC_RDS_TRANSMISSION_MANUAL)
    {
        /* We entered here if we are in auto mode and changing to menual */

        _fmTxSmData.currCmdInfo.stageIndex++;
        _fmTxSmData.context.rdsText = _fmTxSmData.context.fwCache.rawData;
        _fmTxSmData.context.rdsTextLen =  (FMC_U8)FMC_OS_StrLen(_fmTxSmData.context.fwCache.rawData);
        _fmTxSmData.context.rdsGroupText = 0;/*[ToDo Zvi] macros*/
        
    }
    else /* We are going to AUTO mode but the RT bit in transmitted field mask is not enabled*/
    {
        /* we don't need to set the Raw Data go to next step*/
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);
}

void _FM_TX_SM_HandlerTransmissionMode_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerTransmissionMode_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();
}
/**/
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTransmissionMode_GetCmdParmValue(void)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsTransmissionModeCmd->transmissionMode;
}
void _FM_TX_SM_HandlerSetRdsTransmissionMode_UpdateCache(void)
{
    FmTxSetRdsTransmissionModeCmd   *setRdsTransmissionModeCmd = (FmTxSetRdsTransmissionModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.transmissionMode = setRdsTransmissionModeCmd->transmissionMode;
}
void _FM_TX_SM_HandlerSetRdsTransmissionMode_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(_fmTxSmData.context.fwCache.transmissionMode);
    
}
void _FM_TX_SM_HandlerGetRdsTransmissionModeCmd_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    

    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,_FM_TX_SM_HandlerSetRdsTransmissionMode_FillEventData, eventData);
    
    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}

/*
########################################################################################################################
    Set Transmission Field Mask
########################################################################################################################
*/
void _FM_TX_SM_HandlerTransmitMask_Start( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmittedFieldsMaskCmd  *setRdsTransmitMaskCmd = (FmTxSetRdsTransmittedFieldsMaskCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FmcStatus status=FMC_STATUS_SUCCESS;
    FMC_FUNC_START("_FM_TX_SM_HandlerTransmitMask_Start");
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /*Neptune Warrning cleanup */
    FMC_UNUSED_PARAMETER(status);

    FMC_VERIFY_FATAL((_fmTxSmData.context.fwCache.transmissionMode == FMC_RDS_TRANSMISSION_AUTOMATIC), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerTransmissionMode_Start: tmast be in auto made to set mask "));

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _fmTxSmData.context.rdsGroupText = 1;/*[ToDo Zvi] macros*/
    if(setRdsTransmitMaskCmd->fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK)
    {
        _fmTxSmData.context.rdsText = _fmTxSmData.context.fwCache.psMsg;
        _fmTxSmData.context.rdsTextLen =  (FMC_U8)FMC_OS_StrLen(_fmTxSmData.context.fwCache.psMsg);
    }
    else
    {
        /* This will disable this group*/
        _fmTxSmData.context.rdsText = (FMC_U8 *)"";
        _fmTxSmData.context.rdsTextLen =  0;
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerTransmitMask_WaitSetRdsPsMsgCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmittedFieldsMaskCmd  *setRdsTransmitMaskCmd = (FmTxSetRdsTransmittedFieldsMaskCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _fmTxSmData.currCmdInfo.stageIndex++;
    _fmTxSmData.context.rdsGroupText = (FMC_U8)_fmTxSmData.context.fwCache.rtType;/*[ToDo Zvi] macros*/
    if(setRdsTransmitMaskCmd->fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK)
    {
        _fmTxSmData.context.rdsText = _fmTxSmData.context.fwCache.rtMsg;
        _fmTxSmData.context.rdsTextLen =  (FMC_U8)FMC_OS_StrLen(_fmTxSmData.context.fwCache.rtMsg);
    }
    else
    {
        /* This will disable this group*/
        _fmTxSmData.context.rdsText = (FMC_U8 *)"";
        _fmTxSmData.context.rdsTextLen =  0;
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);
}

void _FM_TX_SM_HandlerTransmitMask_SendSetRdsEccMsgCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTransmittedFieldsMaskCmd  *setRdsTransmitMaskCmd = (FmTxSetRdsTransmittedFieldsMaskCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_U16 countryCodeParam;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    if(setRdsTransmitMaskCmd->fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK)
    {
        countryCodeParam = (FMC_U16)((_fmTxSmData.context.fwCache.countryCode<<8)|(0x0001));
    }
    else
    {
        countryCodeParam = (FMC_U16)((_fmTxSmData.context.fwCache.countryCode<<8)|(0x0000));
    }
    /* Enable and set the defualt ECC */
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_ECC_SET_GET, countryCodeParam);
    _fmTxSmData.currCmdInfo.stageIndex++;
}


void _FM_TX_SM_HandlerTransmitMask_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerTransmissionMode_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();
}
void _FM_TX_SM_HandlerSetRdsRTransmittedMask_FillEventData(FmTxEvent *event, void *eventData)
{   
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(_fmTxSmData.context.fwCache.fieldsMask);
}
void _FM_TX_SM_HandlerSetRdsRTransmittedMask_UpdateCache(void)
{
    FmTxSetRdsTransmittedFieldsMaskCmd  *setTransmittedMaskCmd = 
                                            (FmTxSetRdsTransmittedFieldsMaskCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _fmTxSmData.context.fwCache.fieldsMask = setTransmittedMaskCmd->fieldsMask;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTransmittedMask_GetCmdParmValue(void)
{
    FmTxSetRdsTransmittedFieldsMaskCmd  *setTransmittedMaskCmd = (FmTxSetRdsTransmittedFieldsMaskCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    return (FmcFwCmdParmsValue)setTransmittedMaskCmd->fieldsMask;
}
void _FM_TX_SM_HandlerGetRdsTransmissionFieldMaskCmd_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    

    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,_FM_TX_SM_HandlerSetRdsRTransmittedMask_FillEventData, eventData);
    
    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*
########################################################################################################################
    Set Traffic Codes
########################################################################################################################
*/

void _FM_TX_SM_HandlerSetTrafficCodes_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);

}
void _FM_TX_SM_HandlerSetTrafficCodes_SendTaSetCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTrafficCodesCmd   *trafficCodesCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    trafficCodesCmd  = (FmTxSetRdsTrafficCodesCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_TA_SET_GET, (FMC_U16)trafficCodesCmd->taCode);

    _fmTxSmData.currCmdInfo.stageIndex++;

}
void _FM_TX_SM_HandlerSetTrafficCodes_SendTpSetCmd( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTrafficCodesCmd   *trafficCodesCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    trafficCodesCmd  = (FmTxSetRdsTrafficCodesCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _FM_TX_SM_UptadeSmStateSendWriteCommand(FMC_FW_OPCODE_TX_RDS_TP_SET_GET, (FMC_U16)trafficCodesCmd->tpCode);

    _fmTxSmData.currCmdInfo.stageIndex++;

}
void _FM_TX_SM_HandlerSetTrafficCodes_Complete( FMC_UINT event, void *eventData)
{
    FMC_FUNC_START("_FM_TX_SM_HandlerSetTrafficCodes_Complete");
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();

}
void _FM_TX_SM_HandlerSetRdsTrafficCodes_UpdateCache(void)
{
    FmTxSetRdsTrafficCodesCmd       *setRdsTrafficCodesCmd = (FmTxSetRdsTrafficCodesCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    _fmTxSmData.context.fwCache.rdsTaCode = setRdsTrafficCodesCmd->taCode;
    _fmTxSmData.context.fwCache.rdsTpCode = setRdsTrafficCodesCmd->tpCode;
}
void _FM_TX_SM_HandlerSetRdsTrafficCodes_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.trafficCodes.taCode = (FMC_U32)(_fmTxSmData.context.fwCache.rdsTaCode);
    event->p.cmdDone.v.trafficCodes.tpCode = (FMC_U32)(_fmTxSmData.context.fwCache.rdsTpCode);
}
/*
########################################################################################################################
    Get Traffic Codes
########################################################################################################################
*/

void _FM_TX_SM_HandlerGetTrafficCodes_Start( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    _fmTxSmData.currCmdInfo.stageIndex++;
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);

}
void _FM_TX_SM_HandlerGetTrafficCodes_SendTaGetCmd( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    _FM_TX_SM_UptadeSmStateSendReadCommand(FMC_FW_OPCODE_TX_RDS_TA_SET_GET);

    _fmTxSmData.currCmdInfo.stageIndex++;

}
void _FM_TX_SM_HandlerGetTrafficCodes_SendTpGetCmd( FMC_UINT event, void *eventData)
{
    _FmTxSmTransportEventData *transportEventData = NULL;

    FMC_UNUSED_PARAMETER(event);    
    transportEventData=eventData;
    _fmTxSmData.context.fwCache.rdsTaCode=FMC_UTILS_BEtoHost16(&transportEventData->data[0]);
    _FM_TX_SM_UptadeSmStateSendReadCommand(FMC_FW_OPCODE_TX_RDS_TP_SET_GET);

    _fmTxSmData.currCmdInfo.stageIndex++;

}
void _FM_TX_SM_HandlerGetTrafficCodes_Complete( FMC_UINT event, void *eventData)
{
        _FmTxSmTransportEventData *transportEventData = NULL;
    FMC_FUNC_START("_FM_TX_SM_HandlerGetTrafficCodes_Complete");

    transportEventData=eventData;
    _fmTxSmData.context.fwCache.rdsTpCode=FMC_UTILS_BEtoHost16(&transportEventData->data[0]);
        
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType], 
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

    FMC_FUNC_END();

}


void _FM_TX_SM_HandlerGetRdsTrafficCodes_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.trafficCodes.taCode = (FMC_U32)(_fmTxSmData.context.fwCache.rdsTaCode);
    event->p.cmdDone.v.trafficCodes.tpCode = (FMC_U32)(_fmTxSmData.context.fwCache.rdsTpCode);
}

/*
########################################################################################################################
    Set PS Text
########################################################################################################################
*/

/*
[ToDo] - See remarks in the enable process regarding PS text setting
In addition, use the process there as a code reference
*/
void _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Start( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTextPsMsgCmd  *cmd = (FmTxSetRdsTextPsMsgCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    /* Set the RDS Data FIFO only if we transmitting PS text. Otherwise, just record the string */
    if (_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK)
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
        _fmTxSmData.context.rdsText = cmd->msg;
        _fmTxSmData.context.rdsTextLen = (FMC_U8)cmd->len;
        _fmTxSmData.context.rdsGroupText = 1;/*[ToDo Zvi] macros*/
    }
    else
    {
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }

    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
void _FM_TX_SM_HandlerSetRdsTextPsMsgCmd_Complete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();
        
    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/

void _FM_TX_SM_HandlerSetRdsPsMsg_UpdateCache(void)
{
    FmTxSetRdsTextPsMsgCmd  *cmd = (FmTxSetRdsTextPsMsgCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    /* Record the new string in the cache */
    FMC_OS_MemCopy(_fmTxSmData.context.fwCache.psMsg, cmd->msg, cmd->len);
    _fmTxSmData.context.fwCache.psMsg[cmd->len] = '\0';
        

}
void _FM_TX_SM_RdsTextPsMsg_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* Fill psMsg part of the event data using cached info */
    _fmTxSmData.context.appEvent.p.cmdDone.v.psMsg.len = FMC_OS_StrLen(_fmTxSmData.context.fwCache.psMsg);

    /* The event holds a pointer to the string, not a copy */
    _fmTxSmData.context.appEvent.p.cmdDone.v.psMsg.msg = (FMC_U8*)_fmTxSmData.context.fwCache.psMsg;
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/

void _FM_TX_SM_HandlerGetRdsTextPsMsgCmd_Start( FMC_UINT event, void *eventData)
{ 
    FMC_UNUSED_PARAMETER(event);    
    /*[ToDo Zvi] call to this function should be preformed as part of command complete process like in the set commands*/
    _FM_TX_SM_RdsTextPsMsg_FillEventData((FmTxEvent *)event,eventData);
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,NULL, eventData);
    
    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*
########################################################################################################################
    Set RT Text
########################################################################################################################
*/

/*
[ToDo] - See remarks in the enable process regarding PS text setting
In addition, use the process there as a code reference
*/
void _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Start( FMC_UINT event, void *eventData)
{
    FmTxSetRdsTextRtMsgCmd  *cmd = (FmTxSetRdsTextRtMsgCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    /* Set the RDS Data FIFO only if we transmitting RT text. Otherwise, just record the string */
    if (_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK)
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
        _fmTxSmData.context.rdsText = cmd->msg;
        _fmTxSmData.context.rdsTextLen = (FMC_U8)cmd->len;
        _fmTxSmData.context.rdsGroupText = 2;/*[ToDo Zvi] macros*/
    }
    else
    {
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
    }
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
void _FM_TX_SM_HandlerSetRdsTextRtMsgCmd_Complete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    

    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/
void _FM_TX_SM_HandlerSetRdsRtMsg_UpdateCache(void)
{
    FmTxSetRdsTextRtMsgCmd  *cmd = (FmTxSetRdsTextRtMsgCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_OS_MemCopy(_fmTxSmData.context.fwCache.rtMsg, cmd->msg, cmd->len);
    _fmTxSmData.context.fwCache.rtMsg[cmd->len] = '\0';
}

void _FM_TX_SM_RdsTextRtMsg_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    /* Fill rtMsg part of the event data using cached info */
    _fmTxSmData.context.appEvent.p.cmdDone.v.rtMsg.rtMsgType = _fmTxSmData.context.fwCache.rtType;
    _fmTxSmData.context.appEvent.p.cmdDone.v.rtMsg.len = FMC_OS_StrLen(_fmTxSmData.context.fwCache.rtMsg);
    _fmTxSmData.context.appEvent.p.cmdDone.v.rtMsg.msg = (FMC_U8*)_fmTxSmData.context.fwCache.rtMsg;
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/

void _FM_TX_SM_HandlerGetRdsTextRtMsgCmd_Start( FMC_UINT event, void *eventData)
{ 
    FMC_UNUSED_PARAMETER(event);    

    _FM_TX_SM_RdsTextRtMsg_FillEventData((FmTxEvent *)event,eventData);
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,NULL, eventData);
    
    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}

/*
########################################################################################################################
    Set Raw Data
########################################################################################################################
*/


void _FM_TX_SM_HandlerSetRdsRawDataCmd_Start( FMC_UINT event, void *eventData)
{
    FmTxSetRdsRawDataCmd    *cmd = (FmTxSetRdsRawDataCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);

    _fmTxSmData.currCmdInfo.stageIndex++;

    _fmTxSmData.context.rdsText = cmd->rawData;
    _fmTxSmData.context.rdsTextLen = (FMC_U8)cmd->rawLen;
    _fmTxSmData.context.rdsGroupText = 0;/*[ToDo Zvi] macros*/
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, NULL);
}
void _FM_TX_SM_HandlerSetRdsRawDataCmd_Complete( FMC_UINT event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
        
    _FM_TX_SM_RdsRawData_FillEventData((FmTxEvent *)event,eventData);
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(_fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType],
                                            eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/
void _FM_TX_SM_HandlerSetRdsRawData_UpdateCache(void)
{
    FmTxSetRdsRawDataCmd    *cmd = (FmTxSetRdsRawDataCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    FMC_OS_MemCopy(_fmTxSmData.context.fwCache.rawData, cmd->rawData, cmd->rawLen);
    _fmTxSmData.context.fwCache.rawData[cmd->rawLen] = '\0';

}

void _FM_TX_SM_RdsRawData_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(event);    
    FMC_UNUSED_PARAMETER(eventData);
    
    /* Fill rtMsg part of the event data using cached info */
    _fmTxSmData.context.appEvent.p.cmdDone.v.rawRds.len = FMC_OS_StrLen(_fmTxSmData.context.fwCache.rawData);
    _fmTxSmData.context.appEvent.p.cmdDone.v.rawRds.data = (FMC_U8*)_fmTxSmData.context.fwCache.rawData;
}
/*****************************************************************************************************************************************************

*****************************************************************************************************************************************************/

void _FM_TX_SM_HandlerGetRdsRawDataCmd_Start( FMC_UINT event, void *eventData)
{ 
    FMC_UNUSED_PARAMETER(event);    

    _FM_TX_SM_RdsRawData_FillEventData((FmTxEvent *)event,eventData);
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,NULL, eventData);
    
    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*
########################################################################################################################
    End
########################################################################################################################
*/

/*
########################################################################################################################
########################################################################################################################
*/

/*
    Simple SET command stages:
    ------------------------
    1. Send the command to the chip
        1.1 Get the firmware opcode for the command using the mapping tables
        1.2 Get the function that provides the firmware command parameter using the mapping tables
        1.3 Call that function and obtain the firmware value
        1.4 Send the command to the chip
        
    2. Wait for command completion
    
    3. Complete the process (send event to application)
        3.1 Get the optional function that should be invoked when the command completes
        3.2 If a function is specified, call it
        3.3 Get the optional function that fills event data before sending it to the application
        3.4 Complete the process and pass a pointer to the filling function (or NULL if no function is specified)
*/
void _FM_TX_SM_HandlerSimpleSetCmd_Start( FMC_UINT event, void *eventData)
{
    FmTxStatus          status = FM_TX_STATUS_SUCCESS;
    _FmTxSmSetCmdGetCmdParmValueFunc    getCmdParmsFunc = NULL;
    FmcFwOpcode     fwOpCode;
    FmcFwCmdParmsValue  fwSimpleSetCmdParmsValue;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerSimpleSetCmd_Start");

    FMC_UNUSED_PARAMETER(eventData);
    
    FMC_VERIFY_FATAL((event == _FM_TX_SM_EVENT_START), FM_TX_STATUS_INTERNAL_ERROR,
                        ("_FM_TX_SM_HandlerSimpleSetCmd_Start: Unexpected Event (%d)", event));

    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    /* Map the command type to a FW opcode */
    fwOpCode = _fmTxSmSimpleSetCmd2FwOpcodeMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType];

    /* Map the command type to a function that provides the FW command parameter value (there must be such a function) */
    getCmdParmsFunc = _fmTxSmSetCmdGetCmdParmValue[_fmTxSmData.currCmdInfo.baseCmd->cmdType];
    FMC_VERIFY_FATAL((getCmdParmsFunc != NULL), FM_TX_STATUS_INTERNAL_ERROR, 
                        ("_FM_TX_SM_HandlerSimpleSetCmd_Start: NULL getCmdParmsFunc"));

    /* Call the function to obtain the FW value */
    fwSimpleSetCmdParmsValue = getCmdParmsFunc();

    /* Send the command to the chip */
    status = _FM_TX_SM_UptadeSmStateSendWriteCommand(fwOpCode, fwSimpleSetCmdParmsValue);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerSimpleSetCmd_Start: _FM_TX_SM_UptadeSmStateSendWriteCommand returned (%s)", 
                                    FMC_DEBUG_FmTxStatusStr(status)));

    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}


void _FM_TX_SM_HandlerSimpleSetCmd_Complete( FMC_UINT event, void *eventData)
{
    _FmTxSmSetCmdCompleteFunc       cmdCompleteFunc = NULL;
    _FmTxSmCmdCompletedFillEventDataCb  fillEventDataFunc = NULL;       

    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    /* Get the optional function that should be invoked when the command completes */
    cmdCompleteFunc = _fmTxSmSetCmdCompleteMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType];

    /* Get the optional function that fills event data before sending it to the application */
    fillEventDataFunc = _fmTxSmSetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType];

    _FM_TX_SM_HandleCompletionOfCurrCmd(cmdCompleteFunc,fillEventDataFunc, eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*
########################################################################################################################
    Simple GET command
########################################################################################################################
*/

/*
    Simple GET command stages:
    ------------------------
    1. Send the command to the chip
        1.1 Get the firmware opcode for the command using the mapping tables
        
    2. Wait for command completion
    
    3. Complete the process (send event to application)
        3.1 Get the optional function that fills event data before sending it to the application
        3.2 If no function is specified, use the default function (copy value from transport event to event value)
        3.3 Complete the process and pass a pointer to the filling function (or NULL if no function is specified)

    [ToDo] - There should almost always be a function that converts the firmware value that was read to an API
    value that should be filled in the event. Currently the values are mostly copied as is. It works since
    the values for firmware and API are the same. However, we must not rely on this.
*/

void _FM_TX_SM_HandlerSimpleGetCmd_Start( FMC_UINT event, void *eventData)
{
    FmTxStatus          status = FM_TX_STATUS_SUCCESS;
    FmcFwOpcode     fwOpCode;
    
    FMC_FUNC_START("_FM_TX_SM_HandlerSimpleGetCmd_Start");

    FMC_UNUSED_PARAMETER(eventData);
    
    FMC_VERIFY_FATAL_NO_RETVAR((event == _FM_TX_SM_EVENT_START),
                        ("_FM_TX_SM_HandlerSimpleGetCmd_Start: Unexpected Event (%d)", event));


    FMCI_OS_ResetTimer(FM_TX_SM_TIMOUT_TIME_FOR_GENERAL_CMD);

    fwOpCode = _fmTxSmSimpleGetCmd2FwOpcodeMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType];
        
    status = _FM_TX_SM_UptadeSmStateSendReadCommand(fwOpCode);
    FMC_VERIFY_FATAL_NO_RETVAR((status == FM_TX_STATUS_PENDING), 
                                    ("_FM_TX_SM_HandlerSimpleGetCmd_Start: _FM_TX_SM_UptadeSmStateSendReadCommand returned (%s)", 
                                    FMC_DEBUG_FmTxStatusStr(status)));

    _fmTxSmData.currCmdInfo.stageIndex++;
    
    FMC_FUNC_END();
}

void _FM_TX_SM_HandlerSimpleGetCmd_Complete( FMC_UINT event, void *eventData)
{
    _FmTxSmCmdCompletedFillEventDataCb  fiilEventDataFunc = NULL;
    
    FMC_UNUSED_PARAMETER(event);    

    FMCI_OS_CancelTimer();

    fiilEventDataFunc = _fmTxSmSimpleGetCmdFillEventDataMap[_fmTxSmData.currCmdInfo.baseCmd->cmdType];

    /* If no function is explicitly specified, use the default one */
    if (fiilEventDataFunc == NULL)
    {
        fiilEventDataFunc = _FM_TX_SM_SimpleGetCmd_FillEventData;
    }
    
    _FM_TX_SM_HandleCompletionOfCurrCmd(NULL,fiilEventDataFunc, eventData);

    /* Make sure additional commands in queue are processed (if available) */
    FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
}
/*

*/

/*****************************************************************************************************************************************************
    This is the default function that fills event data for simple get processes

    It should probably be modified / removed to make sure firmware values are converted to API values
*****************************************************************************************************************************************************/
void _FM_TX_SM_SimpleGetCmd_FillEventData(FmTxEvent *event, void *eventData)
{
    _FmTxSmTransportEventData           *transportEvent = (_FmTxSmTransportEventData*)eventData;
    FmcFwCmdParmsValue                  readValue;

    FMC_FUNC_START("_FM_TX_SM_SimpleGetCmd_FillEventData");

    if(transportEvent == NULL)
    FMC_VERIFY_FATAL_NO_RETVAR((transportEvent != NULL), ("_FM_TX_SM_SimpleGetCmd_FillEventData: Null eventData"));
    
    FMC_VERIFY_FATAL_NO_RETVAR((transportEvent->dataLen == sizeof(FmcFwCmdParmsValue)), 
                                    ("_FM_TX_SM_SimpleGetCmd_FillEventData: Invalid dataLen (%d)", transportEvent->dataLen));

    readValue = FMC_UTILS_BEtoHost16(&transportEvent->data[0]);
    /* Set the appropriate field in the event structure */
    event->p.cmdDone.v.value = (FMC_U32)(readValue);

    FMC_FUNC_END();
}



/*
########################################################################################################################
########################################################################################################################
    Get Command Param Value
########################################################################################################################
########################################################################################################################
*/
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetInterruptRegister_GetCmdParmValue(void)
{
    return _fmTxSmIntMask.intMask;
}


FmcFwCmdParmsValue _FM_TX_SM_HandlerSetPowerLevel_GetCmdParmValue(void)
{
    FmTxSetPowerLevelCmd    *setPowerLevelCmd = (FmTxSetPowerLevelCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    /* Convert API power level value to firmware power level value */
    FmcFwCmdParmsValue      fwPowerLevelCmdValue = FMC_UTILS_Api2FwPowerLevel(setPowerLevelCmd->level);

    return fwPowerLevelCmdValue;
}

FmcFwCmdParmsValue _FM_TX_SM_HandlerSetMuteMode_GetCmdParmValue(void)
{
    FmTxSetMuteModeCmd      *setMuteCmd = (FmTxSetMuteModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FmcFwCmdParmsValue      fwMuteCmdValue;
    
    if (setMuteCmd->mode == FMC_MUTE)
    {
        fwMuteCmdValue = FMC_FW_TX_MUTE;
    }
    else
    {
        fwMuteCmdValue = FMC_FW_TX_UNMUTE;
    }

    return fwMuteCmdValue;
}


FmcFwCmdParmsValue _FM_TX_SM_HandlerEnableRds_GetCmdParmValue(void)
{
    return FMC_FW_TX_RDS_ENABLE_START;
}

FmcFwCmdParmsValue _FM_TX_SM_HandlerDisableRds_GetCmdParmValue(void)
{
    return FMC_FW_TX_RDS_ENABLE_STOP;
}

FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextScrollMode_GetCmdParmValue(void)
{
    FmTxSetRdsTextPsScrollModeCmd   *setScrollModeCmd = (FmTxSetRdsTextPsScrollModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FmcFwCmdParmsValue              fwScrollModeCmdValue;

    if (setScrollModeCmd->mode == FMC_RDS_PS_DISPLAY_MODE_SCROLL)
    {
        fwScrollModeCmdValue = FMC_FW_TX_RDS_PS_DISPLAY_MODE_SCROLL_ON;
    }
    else
    {
        fwScrollModeCmdValue = FMC_FW_TX_RDS_PS_DISPLAY_MODE_SCROLL_OFF;
    }

    return fwScrollModeCmdValue;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextScrollSpeed_GetCmdParmValue(void)
{
    FmTxSetRdsPsScrollSpeedCmd  *setScrollSpeed = (FmTxSetRdsPsScrollSpeedCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setScrollSpeed->psScrollSpeed;
    
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetPreEmphasisFilter_GetCmdParmValue(void)
{
    FmTxSetPreEmphasisFilterCmd *setPreEmphasisFilterCmd = (FmTxSetPreEmphasisFilterCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setPreEmphasisFilterCmd->emphasisfilter;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsAfCode_GetCmdParmValue(void)
{
    FmTxSetRdsAfCodeCmd *setRdsAfCodeCmd = (FmTxSetRdsAfCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsAfCodeCmd->afCode;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsPiCode_GetCmdParmValue(void)
{
    FmTxSetRdsPiCodeCmd *setRdsPiCodeCmd = (FmTxSetRdsPiCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsPiCodeCmd->piCode;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsPtyCode_GetCmdParmValue(void)
{
    FmTxSetRdsPtyCodeCmd    *setRdsPtyCodeCmd = (FmTxSetRdsPtyCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsPtyCodeCmd->ptyCode;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsTextRepertoire_GetCmdParmValue(void)
{
    FmTxSetRdsRepertoireCmd *setRdsRepertoireCmd = (FmTxSetRdsRepertoireCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsRepertoireCmd->repertoire;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_GetCmdParmValue(void)
{
    FmTxSetRdsMusicSpeechFlagCmd    *setRdsMusicSpeechFlagCmd = (FmTxSetRdsMusicSpeechFlagCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    return (FmcFwCmdParmsValue)setRdsMusicSpeechFlagCmd->musicSpeechFlag;
}
FmcFwCmdParmsValue _FM_TX_SM_HandlerSetRdsExtendedCountryCode_GetCmdParmValue(void)
{
    FmTxSetRdsExtendedCountryCodeCmd    *setRdsExtendedCountryCodeCmd = (FmTxSetRdsExtendedCountryCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    FMC_U16 countryCodeParam;
    /*
    *   16 bit entity - 
    *   Bit 0: '1' indicates ECC is enabled and '0' indicates ECC is disabled
    *   Bits 15 to 7: Specifies the 8 bit ECC code.
    *
    */
    if(_fmTxSmData.context.fwCache.fieldsMask&FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK)
    {
        countryCodeParam = (FMC_U16)((setRdsExtendedCountryCodeCmd->countryCode<<8)|(0x0001));
    }
    else
    {
        countryCodeParam = (FMC_U16)((setRdsExtendedCountryCodeCmd->countryCode<<8)|(0x0000));
    }

    return (FmcFwCmdParmsValue)(countryCodeParam);
}
/*
########################################################################################################################
########################################################################################################################
    Update FM TX SM Cache
########################################################################################################################
########################################################################################################################
*/

void _FM_TX_SM_HandlerSetMuteMode_UpdateCache(void)
{
    FmTxSetMuteModeCmd  *setMuteModeCmd = (FmTxSetMuteModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.muteMode = setMuteModeCmd->mode;
}



void _FM_TX_SM_HandlerEnableRds_UpdateCache(void)
{
    _fmTxSmData.context.fwCache.rdsEnabled = FMC_TRUE;
}
void _FM_TX_SM_HandlerPowerLvl_UpdateCache(void)
{
    _fmTxSmData.context.fwCache.powerLevel =_FM_TX_SM_HandlerSetPowerLevel_GetCmdParmValue();
}

void _FM_TX_SM_HandlerDisableRds_UpdateCache(void)
{
    _fmTxSmData.context.fwCache.rdsEnabled = FMC_FALSE;
}

void _FM_TX_SM_HandlerSetRdsPsDisplayMode_UpdateCache(void)
{   
    FmTxSetRdsTextPsScrollModeCmd   *setPsScrollModeCmd = (FmTxSetRdsTextPsScrollModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.rdsScrollMode = setPsScrollModeCmd->mode;
}
void _FM_TX_SM_HandlerSetRdsPsScrollSpeed_UpdateCache(void)
{
    FmTxSetRdsPsScrollSpeedCmd  *setPsScrollSpeedCmd = (FmTxSetRdsPsScrollSpeedCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.rdsScrollSpeed = setPsScrollSpeedCmd->psScrollSpeed;
}
void _FM_TX_SM_HandlerSetPreEmphasisFilter_UpdateCache(void)
{
    FmTxSetPreEmphasisFilterCmd *setPreEmphasisFilterCmd = (FmTxSetPreEmphasisFilterCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.emphasisfilter = setPreEmphasisFilterCmd->emphasisfilter;
}
void _FM_TX_SM_HandlerSetRdsAfCode_UpdateCache(void)
{
    FmTxSetRdsAfCodeCmd *setRdsAfCodeCmd = (FmTxSetRdsAfCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _fmTxSmData.context.fwCache.afCode = setRdsAfCodeCmd->afCode;
}
void _FM_TX_SM_HandlerSetRdsPiCode_UpdateCache(void)
{
    FmTxSetRdsPiCodeCmd *setRdsPiCodeCmd = (FmTxSetRdsPiCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.rdsPiCode = setRdsPiCodeCmd->piCode;
}
void _FM_TX_SM_HandlerSetRdsPtyCode_UpdateCache(void)
{
    FmTxSetRdsPtyCodeCmd    *setRdsPtyCodeCmd = (FmTxSetRdsPtyCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.rdsPtyCode = setRdsPtyCodeCmd->ptyCode;
}
void _FM_TX_SM_HandlerSetRdsTextRepertoire_UpdateCache(void)
{
    FmTxSetRdsRepertoireCmd*setRdsRepertoireCmd = (FmTxSetRdsRepertoireCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    _fmTxSmData.context.fwCache.rdsRepertoire = setRdsRepertoireCmd->repertoire;
}
void _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_UpdateCache(void)
{
    FmTxSetRdsMusicSpeechFlagCmd    *setRdsMusicSpeechFlagCmd = (FmTxSetRdsMusicSpeechFlagCmd*)_fmTxSmData.currCmdInfo.baseCmd;

    _fmTxSmData.context.fwCache.rdsRusicSpeachFlag = setRdsMusicSpeechFlagCmd->musicSpeechFlag;
}
void _FM_TX_SM_HandlerSetRdsExtendedCountryCode_UpdateCache(void)
{
    FmTxSetRdsExtendedCountryCodeCmd    *setRdsExtendedCountryCode = (FmTxSetRdsExtendedCountryCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    _fmTxSmData.context.fwCache.countryCode = setRdsExtendedCountryCode->countryCode;
}
/*
########################################################################################################################
########################################################################################################################
    Fill Event Data - fill the data for application
########################################################################################################################
########################################################################################################################
*/
void _FM_TX_SM_HandlerSetMuteMode_FillEventData(FmTxEvent *event, void *eventData)
{
    FMC_UNUSED_PARAMETER(eventData);
    
    event->p.cmdDone.v.value = (FMC_U32)(_fmTxSmData.context.fwCache.muteMode);
}

void _FM_TX_SM_HandlerSetRdsTextScrollMode_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsTextPsScrollModeCmd *setScrollModeCmd = (FmTxSetRdsTextPsScrollModeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setScrollModeCmd->mode);
}

void _FM_TX_SM_HandlerSetRdsPsScrollSpeed_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsPsScrollSpeedCmd      *setRdsScrollParamsCmd = (FmTxSetRdsPsScrollSpeedCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);
    
    event->p.cmdDone.v.value = (FMC_U32)(setRdsScrollParamsCmd->psScrollSpeed);
}
void _FM_TX_SM_HandlerSetPowerLevel_FillEventData(FmTxEvent *event, void *eventData)
{   
    FmTxSetPowerLevelCmd *setPowerLevelCmd = (FmTxSetPowerLevelCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setPowerLevelCmd->level);
}




void _FM_TX_SM_HandlerSetPreEmphasisFilter_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetPreEmphasisFilterCmd *setPreEmphasisFilterCmd = (FmTxSetPreEmphasisFilterCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setPreEmphasisFilterCmd->emphasisfilter);

}
void _FM_TX_SM_HandlerSetECC_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsExtendedCountryCodeCmd *setECCCmd = (FmTxSetRdsExtendedCountryCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setECCCmd->countryCode);

}
void _FM_TX_SM_HandlerSetRdsAfCode_FillEventData(FmTxEvent *event, void *eventData)
{

    FmTxSetRdsAfCodeCmd *setRdsAfCodeCmd = (FmTxSetRdsAfCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setRdsAfCodeCmd->afCode);
}
void _FM_TX_SM_HandlerSetRdsPiCode_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsPiCodeCmd *setRdsPiCodeCmd = (FmTxSetRdsPiCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setRdsPiCodeCmd->piCode);

}
void _FM_TX_SM_HandlerSetRdsPtyCode_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsPtyCodeCmd *setRdsPtyCodeCmd = (FmTxSetRdsPtyCodeCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setRdsPtyCodeCmd->ptyCode);

}
void _FM_TX_SM_HandlerSetRdsTextRepertoire_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsRepertoireCmd *setRepertoireCmd = (FmTxSetRdsRepertoireCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setRepertoireCmd->repertoire);

}
void _FM_TX_SM_HandlerSetRdsMusicSpeechFlag_FillEventData(FmTxEvent *event, void *eventData)
{
    FmTxSetRdsMusicSpeechFlagCmd *setRdsMusicSpeechFlagCmd = (FmTxSetRdsMusicSpeechFlagCmd*)_fmTxSmData.currCmdInfo.baseCmd;
    
    FMC_UNUSED_PARAMETER(eventData);

    event->p.cmdDone.v.value = (FMC_U32)(setRdsMusicSpeechFlagCmd->musicSpeechFlag);

}
/*
########################################################################################################################
########################################################################################################################
    END
########################################################################################################################
########################################################################################################################
*/

/*
    This function is called at the completion of execution of every command. It does the following:
    - Fills event data if required
    - Remove the command structure object from the commands queue
    - Free the memory occupied by the command structure object
    - Send an event to the application's callback to nitify about command completion
*/
FmTxStatus _FM_TX_SM_HandleCompletionOfCurrCmd( _FmTxSmSetCmdCompleteFunc   cmdCompleteFunc ,
                                                            _FmTxSmCmdCompletedFillEventDataCb fillEventDataCb, 
                                                            void *eventData)
{
    FmTxStatus status;
    
    /* Make a local copy of the current cmd info - we are going to "destroy" the official values */
    FmcBaseCmd  currCmd = *_fmTxSmData.currCmdInfo.baseCmd;
    FmTxStatus  cmdCompletionStatus = _fmTxSmData.currCmdInfo.status;

    FMC_FUNC_START("_FM_TX_SM_HandleCompletionOfCurrCmd");

    /* Update SM state only if command succeeded.*/
    if(_fmTxSmData.currCmdInfo.status == FM_TX_STATUS_SUCCESS)
    {
        /* If a function is specified, call it */
        if ((cmdCompleteFunc != NULL)&&!(recievedDisable&&(currCmd.cmdType!=FM_TX_CMD_DISABLE)))
        {
            cmdCompleteFunc();
        }
    }
    /* Filling event data before destroying current command to be able to get the data from the current command */
    if ((fillEventDataCb != NULL)&&!(recievedDisable&&(currCmd.cmdType!=FM_TX_CMD_DISABLE)))
    {
        fillEventDataCb(&_fmTxSmData.context.appEvent, eventData);
    }
    /* The remove was previosly here*/

    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_NONE);

    if(recievedDisable&&(currCmd.cmdType!=FM_TX_CMD_DISABLE))
        cmdCompletionStatus = FM_TX_STATUS_DISABLING_IN_PROGRESS;

    if(timerExp)
        cmdCompletionStatus = FM_TX_STATUS_PROCESS_TIMEOUT_FAILURE;
    /* Notify the appliction about completion of the command */
    _FM_TX_SM_SendAppCmdCompleteEvent(  currCmd.context, 
                                            cmdCompletionStatus, 
                                            currCmd.cmdType);
    /*[ToDo Zvi] I think that we should free the command after we send to the application so it will be able to copy it befor calling in the gui context*/
    status = _FM_TX_SM_RemoveFromQueueAndFreeCmd(&_fmTxSmData.currCmdInfo.baseCmd);
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), status, ("_FM_TX_SM_HandleCompletionOfCurrCmd"));

    
    /*The processed command ended - clear the timer flag*/
    timerExp = FMC_FALSE;
    
    FMC_FUNC_END();

    return status;
}

void _FM_TX_SM_SendAppCmdCompleteEvent(FmTxContext *context, 
                                       FmTxStatus completionStatus,
                                       FmTxCmdType cmdType)
{
    FMC_UNUSED_PARAMETER(context);

    _fmTxSmData.context.appEvent.context = context;
    _fmTxSmData.context.appEvent.eventType = FM_TX_EVENT_CMD_DONE;
    _fmTxSmData.context.appEvent.status = completionStatus;
    _fmTxSmData.context.appEvent.p.cmdDone.cmdType = cmdType;

    switch(cmdType)
    {
        case FM_TX_CMD_GET_MUTE_MODE:
            /* When issuing a FM_TX_CMD_GET_MUTE_MODE we need to parse the
             * values to the FmcMuteMode values. */
            switch(_fmTxSmData.context.appEvent.p.cmdDone.v.value)
            {
                case FMC_FW_TX_UNMUTE:
                    _fmTxSmData.context.appEvent.p.cmdDone.v.value = FMC_NOT_MUTE;
                    break;
                
                case FMC_FW_TX_MUTE:
                    _fmTxSmData.context.appEvent.p.cmdDone.v.value = FMC_MUTE;
                    break;

                default:
                    FMC_ASSERT(0);
                    break;
            }
            break;

        case FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE:
            /* Get only Extended Country Code - bits 15-8 */
            _fmTxSmData.context.appEvent.p.cmdDone.v.value =
                            _fmTxSmData.context.appEvent.p.cmdDone.v.value >> 8;
            break;

        default:
            break;
    }
    
    (_fmTxSmData.context.appCb)(&_fmTxSmData.context.appEvent);
}

/*
    This is the callback that is passed to the script processor for sending HCI script commands to the chip.

    All it is is a middle-man. It receives the request from the script processor and forwards the request 
    to the transport layer via the appropriate transport layer interface function
    
*/
McpBtsSpStatus _FM_TX_SM_TiSpSendHciScriptCmd(  McpBtsSpContext *context,
                                                        McpU16  hciOpcode, 
                                                        McpU8   *hciCmdParms, 
                                                        McpUint len)
{
        
    FMC_UNUSED_PARAMETER(context);

    _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    /* Send the next HCI Script command */
    FMC_CORE_SendHciScriptCommand(  hciOpcode, 
                                                                    hciCmdParms, 
                                                                    len);

    /* [ToDo] - Convert fmcStatus to TiSpStatus */
    return MCP_BTS_SP_STATUS_PENDING;
}

/*
    This is the callback function that is called by the script processor to notify that script execution completed

    [ToDo] - Verify that script processor execution status is checked and acted upon
*/
void _FM_TX_SM_TiSpExecuteCompleteCb(McpBtsSpContext *context, McpBtsSpStatus status)
{
    FMC_UNUSED_PARAMETER(context);

    /* Activate the appropriate stage handler of the current process */
    _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_SCRIPT_EXEC_COMPLETE, (void*)status);
}
/*
    Checks whether the specified command type is of a comamnd the is setting the RDS FIFO
*/
FMC_BOOL _FM_TX_SM_IsAnRdsFifoCmd(FmTxCmdType   cmdType)
{
    if (    (cmdType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG) ||
        (cmdType == FM_TX_CMD_SET_RDS_TEXT_RT_MSG) ||
        (cmdType == FM_TX_CMD_WRITE_RDS_RAW_DATA))
    {
        return FMC_TRUE;
    }
    else
    {
        return FMC_FALSE;
    }
}

FmTxStatus FM_TX_SM_AllocateCmdAndAddToQueue(   FmTxContext     *context,
                                                                FmTxCmdType cmdType,
                                                                FmcBaseCmd      **cmd)
{
    FmcStatus   status;

    FMC_FUNC_START("FM_TX_SM_AllocateCmdAndAddToQueue");

    /* Allocate space for the new command */
    status = _FM_TX_SM_AllocateCmd(context, cmdType, cmd);
    FMC_VERIFY_ERR((status == FM_TX_STATUS_SUCCESS), status, ("FM_TX_SM_AllocateCmdAndAddToQueue"));

    /* Init base fields of cmd structure*/
    FMC_InitializeListNode(&((*cmd)->node));
    (*cmd)->context = context;
    (*cmd)->cmdType = cmdType;

    /* Insert the cmd as the last cmd in the cmds queue */
    FMC_InsertTailList(FMCI_GetCmdsQueue(), &((*cmd)->node));

    smNumOfCmdInQueue++;

    FMC_LOG_INFO(("FM TX: %s Inserted into Commands Queue", FMC_DEBUG_FmTxCmdStr(cmdType)));
    
    FMC_FUNC_END();
    
    return status;
}

FmTxStatus _FM_TX_SM_RemoveFromQueueAndFreeCmd(FmcBaseCmd **cmd)
{
    FmcStatus   status;

    FMC_FUNC_START("_FM_TX_SM_RemoveFromQueueAndFreeCmd");

    /* Remove the cmd element from the queue */
    FMC_RemoveNodeList(&((*cmd)->node));

    FMC_LOG_INFO(("FM TX: %s Removed Commands Queue", FMC_DEBUG_FmTxCmdStr((*cmd)->cmdType)));

    /* Free the cmd structure space */
    status = _FM_TX_SM_FreeCmd(cmd);
    FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), status, ("_FM_TX_SM_RemoveFromQueueAndFreeCmd"));
    
    smNumOfCmdInQueue--;
    FMC_FUNC_END();
    
    return status;
}

/* Allocate space for the new cmd data */
FmTxStatus _FM_TX_SM_AllocateCmd(   FmTxContext     *context,
                                            FmTxCmdType cmdType,
                                            FmcBaseCmd      **cmd)
{
    FmTxStatus      status = FM_TX_STATUS_SUCCESS;

    FMC_FUNC_START("FM_TX_SM_AllocateCmd");

    FMC_UNUSED_PARAMETER(context);
    
    /* Check if the command is one of the RDS-FIFO commands */
    if (_FM_TX_SM_IsAnRdsFifoCmd(cmdType) == FMC_FALSE)
    {
        /* It is not an RDS-FIFO command => allocate from general commands pool */
        status = FMC_POOL_Allocate(FMCI_GetCmdsPool(), (void **)cmd);

        if (status == FMC_STATUS_NO_RESOURCES)
        {
            FMC_LOG_ERROR(("FM TX: Too many pending Commands. (%s)", FMC_DEBUG_FmTxCmdStr(cmdType)));
            FMC_RET(FMC_STATUS_TOO_MANY_PENDING_CMDS);
        }
        else if (status != FMC_STATUS_SUCCESS)
        {
            FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, ("_FM_TX_AllocateCmdAndAddToQueue: FMC_POOL_Allocate Failed (%s)", 
                                                    FMC_DEBUG_FmcStatusStr(status)));
        }
    }
    /* RDS-FIFO commands are using a special buffer - however, only a single command at a time */
    else
    {
        if(cmdType == FM_TX_CMD_WRITE_RDS_RAW_DATA)
        {
            if (_fmTxSmData.context.rdsRawDataCmdBufAllocated == FMC_FALSE)
            {
                _fmTxSmData.context.rdsRawDataCmdBufAllocated = FMC_TRUE;
                *cmd = (FmcBaseCmd*)_fmTxSmData.context.rdsRawDataCmdBuf;
            }
            else 
            {
                /* Indicate the exact error cause */
                FMC_ERR(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
                        ("FM_TX_SM_AllocateCmd: RDS RAW Cmd buffer already allocated"));
            }
        }
        else if(cmdType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG)
        {
            if (_fmTxSmData.context.rdsPsCmdBufAllocated == FMC_FALSE)
            {
                _fmTxSmData.context.rdsPsCmdBufAllocated = FMC_TRUE;
                *cmd = (FmcBaseCmd*)_fmTxSmData.context.rdsPsCmdBuf;
            }
            else 
            {
                /* Indicate the exact error cause */
                FMC_ERR(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
                        ("FM_TX_SM_AllocateCmd: RDS PS Cmd buffer already allocated"));
            }
        }
        else if(cmdType == FM_TX_CMD_SET_RDS_TEXT_RT_MSG)
        {
            if (_fmTxSmData.context.rdsRtCmdBufAllocated == FMC_FALSE)
            {
                _fmTxSmData.context.rdsRtCmdBufAllocated = FMC_TRUE;
                *cmd = (FmcBaseCmd*)_fmTxSmData.context.rdsRtCmdBuf;
            }
            else 
            {
                /* Indicate the exact error cause */
                FMC_ERR(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
                        ("FM_TX_SM_AllocateCmd: RDS RT Cmd buffer already allocated"));
            }
        }
    }
    
    FMC_FUNC_END();
    
    return status;
}

FmTxStatus _FM_TX_SM_FreeCmd(FmcBaseCmd **cmd)
{
    /* Allocate space for the new cmd data */
    FmTxStatus      status = FM_TX_STATUS_SUCCESS;
    FmTxCmdType cmdType;
        
    FMC_FUNC_START("FM_TX_SM_FreeCmd");

    cmdType = (*cmd)->cmdType;
    
    /* Check if the command is one of the RDS-FIFO commands */
    if (_FM_TX_SM_IsAnRdsFifoCmd(cmdType) == FMC_FALSE)
    {
        /* It is not an RDS-FIFO command => allocate from general commands pool */
        status = FMC_POOL_Free(FMCI_GetCmdsPool(), (void **)cmd);       
        FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), FM_TX_STATUS_INTERNAL_ERROR,
                            ("FM_TX_SM_FreeCmd: FMC_POOL_Free Failed (%s) for %s", 
                            FMC_DEBUG_FmcStatusStr(status), FMC_DEBUG_FmTxCmdStr(cmdType)));
    }
    /* RDS-FIFO commands are using a special buffer - however, only a single command at a time */
    else
    {
        if(cmdType == FM_TX_CMD_WRITE_RDS_RAW_DATA)
        {
            FMC_VERIFY_FATAL((_fmTxSmData.context.rdsRawDataCmdBufAllocated == FMC_TRUE), FM_TX_STATUS_INTERNAL_ERROR,
                                ("FM_TX_SM_FreeCmd: %s (RDS RAW Cmd) but rdsFifoCmdBufAllocated is FALSE", 
                                    FMC_DEBUG_FmTxCmdStr(cmdType)));    
                
            *cmd = 0;
            _fmTxSmData.context.rdsRawDataCmdBufAllocated = FMC_FALSE;
        }
        else if(cmdType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG)
        {
            FMC_VERIFY_FATAL((_fmTxSmData.context.rdsPsCmdBufAllocated == FMC_TRUE), FM_TX_STATUS_INTERNAL_ERROR,
                                ("FM_TX_SM_FreeCmd: %s (RDS RAW Cmd) but rdsFifoCmdBufAllocated is FALSE", 
                                    FMC_DEBUG_FmTxCmdStr(cmdType)));    
                
            *cmd = 0;
            _fmTxSmData.context.rdsPsCmdBufAllocated = FMC_FALSE;
        }
        else if(cmdType == FM_TX_CMD_SET_RDS_TEXT_RT_MSG)
        {
            FMC_VERIFY_FATAL((_fmTxSmData.context.rdsRtCmdBufAllocated == FMC_TRUE), FM_TX_STATUS_INTERNAL_ERROR,
                                ("FM_TX_SM_FreeCmd: %s (RDS RAW Cmd) but rdsFifoCmdBufAllocated is FALSE", 
                                    FMC_DEBUG_FmTxCmdStr(cmdType)));    
                
            *cmd = 0;
            _fmTxSmData.context.rdsRtCmdBufAllocated = FMC_FALSE;
        }
    }
    
    FMC_FUNC_END();
    
    return status;
}

/*
    This is a comparison function that is used when searching the commands queue for command of a specific
    command type.
    
    See FM_TX_SM_FindLatestCmdInQueueByType() for details
*/
FMC_BOOL _FM_TX_SM_CompareCmdTypes(const FMC_ListNode *entryToMatch, const FMC_ListNode* checkedEntry)
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
FmcBaseCmd *FM_TX_SM_FindLatestCmdInQueueByType(FmTxCmdType cmdType)
{
    FmcBaseCmd      templateEntry;
    FMC_ListNode        *matchingEntryAsListEntry = NULL;

    FMC_FUNC_START("FM_TX_SM_FindLatestCmdInQueueByType");

    /* Init the template entry so that the entry with the matching cmd type will be searched for */
    templateEntry.cmdType = cmdType;
    
    /* Seach the commands queue for the last entry */
    matchingEntryAsListEntry = FMC_UTILS_FindMatchingListNode(  FMCI_GetCmdsQueue(),                /* Head */
                                            NULL,                               /* Start from last node */
                                            FMC_UTILS_SEARCH_LAST_TO_FIRST, /* Search for last entry */
                                            &(templateEntry.node),
                                            _FM_TX_SM_CompareCmdTypes);
    
    FMC_FUNC_END();
    
    return  (FmcBaseCmd*)matchingEntryAsListEntry;
}

/*
    Check if a command of the specified type is pending execution
*/
FMC_BOOL FM_TX_SM_IsCmdPending(FmTxCmdType cmdType)
{
    FmcBaseCmd *pendingCmd = FM_TX_SM_FindLatestCmdInQueueByType(cmdType);

    if (pendingCmd != NULL)
    {
        return FMC_TRUE;
    }
    else
    {
        return FMC_FALSE;
    }
}

FmcFreq FM_TX_SM_GetCurrentAndPendingTunedFreq(FmTxContext *context)
{
    FmcFreq freq;
    
    /* Find latest tune command */
    FmTxTuneCmd *pendingTuneCmd = (FmTxTuneCmd*)FM_TX_SM_FindLatestCmdInQueueByType(FM_TX_CMD_TUNE);
    
    FMC_UNUSED_PARAMETER(context);

    if (pendingTuneCmd != NULL)
    {
        freq =  pendingTuneCmd->freq;
    }
    else
    {
        freq = _fmTxSmData.context.fwCache.tunedFreq;
    }
    
    return freq;
}
FMC_UINT _getIndexOfLastStage(void)
{
    return ((_fmTxSmCmdsTable[(FMC_UINT)_fmTxSmData.currCmdInfo.baseCmd->cmdType]->numOfStages) - 1);
}
/********************************************************************************************************
        VAC related Utility functions 
*********************************************************************************************************/
void _FM_TX_SM_HandleVacStatusAndMove2NextStage(ECCM_VAC_Status vacStatus,void *eventData)
{
    if (vacStatus == CCM_VAC_STATUS_PENDING) 
    {
        _fmTxSmData.currCmdInfo.stageIndex++;
        _FM_TX_SM_UpdateState(_FM_TX_SM_STATE_WAITING_FOR_CC);
    }
    else
    {
        
        if(vacStatus == CCM_VAC_STATUS_SUCCESS)
        {
            _fmTxSmData.context.transportEventData.status = FMC_STATUS_SUCCESS;
            _fmTxSmData.currCmdInfo.stageIndex++;
        }
        else if(vacStatus == CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES)
        {
            _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES;
            _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
        }
     else if(vacStatus == CCM_VAC_STATUS_FAILURE_OPERATION_NOT_SUPPORTED)
     {
        _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_NOT_APPLICABLE;
        _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
     }
     else 
     {
         _fmTxSmData.currCmdInfo.status = FM_TX_STATUS_INTERNAL_ERROR;
         _fmTxSmData.currCmdInfo.stageIndex = _getIndexOfLastStage();
     }
        _FM_TX_SM_CheckInterruptsAndThenDispatch(_FM_TX_SM_EVENT_CONTINUE, eventData);  
    }
}

ECAL_ResourceMask _convertTxSourceToCal(FmTxAudioSource source,TCAL_ResourceProperties *pProperties)
{
    switch(source)
    {
        case FM_TX_AUDIO_SOURCE_I2SH :
            pProperties->tResourcePorpeties[0] = 1;
            pProperties->uPropertiesNumber = 1;
            return CAL_RESOURCE_MASK_I2SH;
        case FM_TX_AUDIO_SOURCE_PCMH:
            pProperties->tResourcePorpeties[0] = 0;
            pProperties->uPropertiesNumber = 1;
            return CAL_RESOURCE_MASK_PCMH;
        case FM_TX_AUDIO_SOURCE_FM_ANALOG:
            pProperties->uPropertiesNumber = 0;
            return CAL_RESOURCE_MASK_FM_ANALOG;
        default:/*Ileagal source*/
            return CAL_RESOURCE_MASK_NONE;
    }
}

FMC_U32 _getChannelNumber(FmTxMonoStereoMode        monoStereoMode)
{
    if(monoStereoMode == FM_TX_MONO_MODE)
        return 1;
    else if(monoStereoMode == FM_TX_STEREO_MODE)
        return 2;
    else/*Invalid state*/
        return 0;
}

FMC_U16 _getAudioIoParam(FmTxAudioSource            audioSource)
{
    if(audioSource == FM_TX_AUDIO_SOURCE_FM_ANALOG)
    {
        return  FMC_FW_TX_AUDIO_IO_SET_ANALOG;
    }
    else /* PCM/I2S */
    {
        return FMC_FW_TX_AUDIO_IO_SET_I2S;
    }
}
/********************************************************************************************************
        End of VAC related Utility functions 
*********************************************************************************************************/


