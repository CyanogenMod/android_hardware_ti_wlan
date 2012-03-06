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

#include "fmc_types.h"
#include "fmc_defs.h"
#include "fmc_debug.h"
#include "fmc_log.h"
#include "fm_txi.h"
#include "fm_tx_sm.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMTX);

/*
	Possible initialization states
*/
typedef enum _tagFmTxState {
	_FM_TX_INIT_STATE_NOT_INITIALIZED,
	_FM_TX_INIT_STATE_INITIALIZED,
	_FM_TX_INIT_STATE_INIT_FAILED
} _FmTxInitState;
typedef struct
{
	FMC_BOOL cond;
	FmTxStatus	errorStatus;
}_FmTxConditionParams;

/* current init state */
static _FmTxInitState _fmTxInitState = _FM_TX_INIT_STATE_NOT_INITIALIZED;

/* Macro that should be used at the start of APIs that require the FM TX to be enabled to start */
#define _FM_TX_FUNC_START_AND_LOCK_ENABLED(funcName)										\
			FMC_FUNC_START_AND_LOCK(funcName);												\
			FMC_VERIFY_ERR(	((FM_TX_SM_IsContextEnabled() == FMC_TRUE) &&					\
								(FM_TX_SM_IsCmdPending(FM_TX_CMD_DISABLE) == FMC_FALSE)),	\
								FM_TX_STATUS_CONTEXT_NOT_ENABLED,								\
			("%s May Be Called only when FM TX is enabled", funcName))

#define _FM_TX_FUNC_END_AND_UNLOCK_ENABLED()	FMC_FUNC_END_AND_UNLOCK()

/*
	Auxiliary argument verfication functions
*/
FMC_STATIC FmTxStatus _FM_TX_SetRdsTextPsMsgVerifyParms(	const FMC_U8 		*msg,
																		FMC_UINT 			len);

FMC_STATIC FmTxStatus _FM_TX_SetRdsTextRtMsgVerifyParms(	FmcRdsRtMsgType		msgType, 
																		const FMC_U8 		*msg,
																		FMC_UINT 			len);

FMC_STATIC FmTxStatus _FM_TX_SetRdsTransmittedFieldsMaskVerifyParms(FmTxRdsTransmittedGroupsMask fieldsMask);


/*	_FM_TX_SimpleCmdAndCopyParams()
*	
*	This function is for sending command with a parameter and a condition on the parameter (Usually for set commands). 
*
*/
FMC_STATIC FmTxStatus _FM_TX_SimpleCmdAndCopyParams(FmTxContext *fmContext, 
								FMC_UINT paramIn,
								FMC_BOOL condition,
								FmTxCmdType cmdT,
								const char * funcName);
/*	_FM_TX_SimpleCmd()
*	
*	This function is for sending simple command without parameter (Usually used for get commands).  
*
*/

FMC_STATIC FmTxStatus _FM_TX_SimpleCmd(FmTxContext *fmContext, 
								FmTxCmdType cmdT,
								const char * funcName);

FMC_STATIC FMC_U8 *_FM_TX_FmTxStatusStr(FmTxStatus status);

FMC_STATIC FMC_BOOL _FM_TX_IsRdsAutoAction(FmTxCmdType cmdT);

FmTxStatus FM_TX_Init(void)
{
	/*FmcStatus	fmcStatus = FM_TX_STATUS_SUCCESS;*/
	FmTxStatus	status = FM_TX_STATUS_SUCCESS;
	FmcStatus  fmcStatus;
	FMC_FUNC_START(("FM_TX_Init"));

	FMC_VERIFY_ERR((_fmTxInitState == _FM_TX_INIT_STATE_NOT_INITIALIZED), FM_TX_STATUS_NOT_DE_INITIALIZED,
						("FM_TX_Init: Invalid call while FM TX Is not De-Initialized"));

	/* Assume failure. If we fail before reaching the end, we will stay in this state */
	_fmTxInitState = _FM_TX_INIT_STATE_INIT_FAILED;

	/* Init RX & TX common module */
	fmcStatus = FMCI_Init();
	FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), fmcStatus, 
						("FM_TX_Init: FMCI_Init Failed (%s)", FMC_DEBUG_FmcStatusStr(fmcStatus)));

	/* Init FM TX state machine */
	FM_TX_SM_Init();
	
	
	_fmTxInitState = _FM_TX_INIT_STATE_INITIALIZED;

	FMC_LOG_INFO(("FM_TX_Init: FM TX Initialization completed Successfully"));
	
	FMC_FUNC_END();
	
	return status;
}

FmTxStatus FM_TX_Deinit(void)
{
	FmTxStatus	status = FM_TX_STATUS_SUCCESS;

	FMC_FUNC_START(("FM_TX_Deinit"));

	if (_fmTxInitState !=_FM_TX_INIT_STATE_INITIALIZED)
	{
		FMC_LOG_INFO(("FM_TX_Deinit: Module wasn't properly initialized. Exiting without de-initializing"));
		FMC_RET(FM_TX_STATUS_SUCCESS);
	}

	FMC_VERIFY_FATAL((FM_TX_SM_GetContextState() ==  FM_TX_SM_CONTEXT_STATE_DESTROYED), 
						FM_TX_STATUS_CONTEXT_NOT_DESTROYED, ("FM_TX_Deinit: FM TX Context must first be destoryed"));

	/* De-Init FM TX state machine */
	FM_TX_SM_Deinit();

	/* De-Init RX & TX common module */
	FMCI_Deinit();
	
	_fmTxInitState = _FM_TX_INIT_STATE_NOT_INITIALIZED;
	
	FMC_FUNC_END();
	
	return status;
}

FmTxStatus FM_TX_Create(FmcAppHandle *appHandle, const FmTxCallBack fmCallback, FmTxContext **fmContext)
{
	FmTxStatus	status = FM_TX_STATUS_SUCCESS;

	FMC_FUNC_START(("FM_TX_Create"));

	FMC_VERIFY_ERR((appHandle == NULL), FMC_STATUS_NOT_SUPPORTED, ("FM_TX_Create: appHandle Must be null currently"));
	FMC_VERIFY_ERR((_fmTxInitState == _FM_TX_INIT_STATE_INITIALIZED), FM_TX_STATUS_NOT_INITIALIZED,
						("FM_TX_Create: FM TX Not Initialized"));
	FMC_VERIFY_FATAL((FM_TX_SM_GetContextState() ==  FM_TX_SM_CONTEXT_STATE_DESTROYED), 
						FM_TX_STATUS_CONTEXT_NOT_DESTROYED, ("FM_TX_Deinit: FM TX Context must first be destoryed"));

	status = FM_TX_SM_Create(fmCallback, fmContext);
	FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), status, 
						("FM_TX_Create: FM_TX_SM_Create Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));
	
	
	FMC_LOG_INFO(("FM_TX_Create: Successfully Create FM TX Context"));
	
	FMC_FUNC_END();
	
	return status;
}

FmTxStatus FM_TX_Destroy(FmTxContext **fmContext)
{
	FmTxStatus	status = FM_TX_STATUS_SUCCESS;

	FMC_FUNC_START(("FM_TX_Destroy"));

	FMC_VERIFY_ERR((*fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_Destroy: Invalid Context Ptr"));
	FMC_VERIFY_ERR((FM_TX_SM_GetContextState() ==  FM_TX_SM_CONTEXT_STATE_DISABLED), 
						FM_TX_STATUS_CONTEXT_NOT_DISABLED, ("FM_TX_Destroy: FM TX Context must be disabled before destroyed"));

	status = FM_TX_SM_Destroy(fmContext);
	FMC_VERIFY_FATAL((status == FM_TX_STATUS_SUCCESS), status, 
						("FM_TX_Destroy: FM_TX_SM_Destroy Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

	/* Make sure pointer will not stay dangling */
	*fmContext = NULL;

	FMC_LOG_INFO(("FM_TX_Destroy: Successfully Destroyed FM TX Context"));
	
	FMC_FUNC_END();
	
	return status;
}

FmTxStatus FM_TX_Enable(FmTxContext *fmContext)
{
	FmTxStatus		status;
	FmTxEnableCmd	*enableCmd = NULL;

	FMC_FUNC_START_AND_LOCK(("FM_TX_Enable"));

	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_Enable: Invalid Context Ptr"));
	FMC_VERIFY_ERR((FM_TX_SM_GetContextState() ==  FM_TX_SM_CONTEXT_STATE_DISABLED), 
						FM_TX_STATUS_CONTEXT_NOT_DISABLED, ("FM_TX_Enable: FM TX Context Is Not Disabled"));
	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_ENABLE) == FMC_FALSE), FM_TX_STATUS_IN_PROGRESS,
						("FM_TX_Enable: Enabling Already In Progress"));
	
	/* When we are disabled there must not be any pending commands in the queue */
	FMC_VERIFY_FATAL((FMC_IsListEmpty(FMCI_GetCmdsQueue()) == FMC_TRUE), FMC_STATUS_INTERNAL_ERROR,
						("FM_TX_Enable: Context is Disabled but there are pending command in the queue"));
	
	/* 
		Direct FM task events to the TX's event handler.
		This must be done here (and not in fm_tx_sm.c) since only afterwards
		we will be able to send the event the event handler
	*/
	status = FMCI_SetEventCallback(FM_TX_SM_TaskEventCb);
	FMC_VERIFY_FATAL((status == FMC_STATUS_SUCCESS), status, 
						("FM_TX_Enable: FMCI_SetEventCallback Failed (%s)", FMC_DEBUG_FmcStatusStr(status)));

	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_ENABLE, (FmcBaseCmd**)&enableCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_Enable"));

	status = FM_TX_STATUS_PENDING;

	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	FMC_FUNC_END_AND_UNLOCK();
	
	return status;
}

FmTxStatus FM_TX_Disable(FmTxContext *fmContext)
{
	FmTxStatus		status;
	FmTxDisableCmd	*disableCmd = NULL;

	FMC_FUNC_START_AND_LOCK(("FM_TX_Disable"));

	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_Disable: Invalid Context Ptr"));

	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_DISABLE) == FMC_FALSE), FM_TX_STATUS_IN_PROGRESS, 
						("Disabling already in progress"));

	if (FM_TX_SM_GetContextState() == FM_TX_SM_CONTEXT_STATE_DISABLED)
	{
		FMC_LOG_INFO(("FM_TX_Disable: Context already disabled - completing successfully immediately"));
		return FM_TX_STATUS_SUCCESS;
	}
	
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_DISABLE, (FmcBaseCmd**)&disableCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_Disable"));

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_DISABELING);
	
	FMC_FUNC_END_AND_UNLOCK();
	
	return status;
}

FmTxStatus FM_TX_Tune(FmTxContext *fmContext, FmcFreq freq)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								freq,
								(FMC_UTILS_IsFreqValid( freq) == FMC_TRUE),
								FM_TX_CMD_TUNE,
								"FM_TX_Tune");
}

FmTxStatus FM_TX_GetTunedFrequency(FmTxContext *fmContext)
{
	return	_FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_TUNED_FREQUENCY,
								"FM_TX_GetTunedFrequency");
}

FmTxStatus FM_TX_StartTransmission(FmTxContext *fmContext)
{
	FmTxStatus					status;
	FmTxStartTransmissionCmd		*startTransmissionCmd = NULL;
	FmcFreq						freq;

	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_StartTransmission");

	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_StartTransmission: Invalid Context Ptr"));

	/* If transmission is already on, exit successfully, nothing to do */
	if (FM_TX_SM_GetContextState() == FM_TX_SM_CONTEXT_STATE_TRANSMITTING)
	{
		FMC_LOG_INFO(("FM_TX_StartTransmission: Transmission already On - nothing to do"));
		FMC_RET(FM_TX_STATUS_SUCCESS);
	}

	/* Verfiy that the transmitter is tuned to some frequency (or that tuning is pending) */
	freq = FM_TX_SM_GetCurrentAndPendingTunedFreq(fmContext);	
	FMC_VERIFY_ERR((freq != FMC_UNDEFINED_FREQ), FM_TX_STATUS_TRANSMITTER_NOT_TUNED,
						("FM_TX_StartTransmission: Transmitter must be tuned before starting transmission"));
		
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_START_TRANSMISSION, (FmcBaseCmd**)&startTransmissionCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_StartTransmission"));

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_StopTransmission(FmTxContext *fmContext)
{
	FmTxStatus					status;
	FmTxStopTransmissionCmd		*stopTransmissionCmd = NULL;

	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_StopTransmission");

	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_Enable: Invalid Context Ptr"));

	/* At this point TX is enabled, and potentially transmission is on. If transmission is off
		(state == ENABLED) exit successfully (nothing to do) */
	if (FM_TX_SM_GetContextState() == FM_TX_SM_CONTEXT_STATE_ENABLED)
	{
		FMC_LOG_INFO(("FM_TX_StopTransmission: Transmission already Stopped - nothing to do"));
		FMC_RET(FM_TX_STATUS_SUCCESS);
	}
			
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_STOP_TRANSMISSION, (FmcBaseCmd**)&stopTransmissionCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_StopTransmission"));

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_SetPowerLevel(FmTxContext *fmContext, FmTxPowerLevel level)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								level,
								(level <= FM_TX_POWER_LEVEL_MAX),
								FM_TX_CMD_SET_POWER_LEVEL,
								"FM_TX_SetPowerLevel");
}

FmTxStatus FM_TX_GetPowerLevel(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_POWER_LEVEL,
								"FM_TX_GetPowerLevel");
}

FmTxStatus FM_TX_SetMuteMode(FmTxContext *fmContext, FmcMuteMode mode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								mode,
								((mode == FMC_MUTE) || (mode == FMC_NOT_MUTE)),
								FM_TX_CMD_SET_MUTE_MODE,
								"FM_TX_SetMuteMode");
}
		
FmTxStatus FM_TX_GetMuteMode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_MUTE_MODE,
								"FM_TX_GetMuteMode");
}

FmTxStatus FM_TX_EnableRds(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_ENABLE_RDS,
								"FM_TX_EnableRds");

}

FmTxStatus FM_TX_DisableRds(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_DISABLE_RDS,
								"FM_TX_DisableRds");

}


FmTxStatus FM_TX_SetRdsPsDisplayMode(FmTxContext *fmContext, FmcRdsPsDisplayMode scrollMode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								scrollMode,
								((scrollMode == FMC_RDS_PS_DISPLAY_MODE_STATIC) || (scrollMode == FMC_RDS_PS_DISPLAY_MODE_SCROLL)),
								FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE,
								"FM_TX_SetRdsPsDisplayMode");

}

FmTxStatus FM_TX_GetRdsPsDisplayMode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE,
								"FM_TX_GetRdsPsDisplayMode");
}

FmTxStatus FM_TX_SetRdsTextPsMsg(	FmTxContext 			*fmContext, 
											const FMC_U8 		*msg,
											FMC_UINT 			len)
{
	FmTxStatus				status;
	FmTxSetRdsTextPsMsgCmd	*cmd = NULL;

	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_SetRdsTextPsMsg");
	
	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_SetRdsTextPsMsg: Invalid Context Ptr"));

	FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
					FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
					("FM_TX_SetRdsTextPsMsg: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));
	
	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
					, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
					("FM_TX_SetRdsTextPsMsg: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));

	/* Verify Parameters */
	status = _FM_TX_SetRdsTextPsMsgVerifyParms(msg, len);
	FMC_VERIFY_ERR((status  == FM_TX_STATUS_SUCCESS), status, ("FM_TX_SetRdsTextPsMsg"));

	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_SET_RDS_TEXT_PS_MSG, (FmcBaseCmd**)&cmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_SetRdsTextPsMsg"));

	/* Copy cmd parms for the cmd execution phase*/
	FMC_OS_MemCopy(cmd->msg, msg, len);
	cmd->len = len;
	
	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_GetRdsTextPsMsg(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TEXT_PS_MSG,
								"FM_TX_GetRdsTextPsMsg");

}

FmTxStatus FM_TX_SetRdsTextRtMsg(	FmTxContext 			*fmContext, 
											FmcRdsRtMsgType		msgType, 
											const FMC_U8 		*msg,
											FMC_UINT 			len)
{
	FmTxStatus				status;
	FmTxSetRdsTextRtMsgCmd	*cmd = NULL;

	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_SetRdsTextRtMsg");
	
	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_SetRdsTextRtMsg: Invalid Context Ptr"));
	FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
					FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
					("FM_TX_SetRdsTextRtMsg: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));
	
	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
					, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
					("FM_TX_SetRdsTextRtMsg: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));
	/* Verify Parameters */
	status = _FM_TX_SetRdsTextRtMsgVerifyParms(msgType, msg, len);
	FMC_VERIFY_ERR((status  == FM_TX_STATUS_SUCCESS), status, ("FM_TX_SetRdsTextRtMsg"));
	
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_SET_RDS_TEXT_RT_MSG, (FmcBaseCmd**)&cmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_SetRdsTextRtMsg"));

	/* Copy cmd parms for the cmd execution phase*/
	cmd->rtType = msgType;
	FMC_OS_MemCopy(cmd->msg, msg, len);
	cmd->len = len;
	
	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_GetRdsTextRtMsg(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TEXT_RT_MSG,
								"FM_TX_GetRdsTextRtMsg");

	
}

FmTxStatus FM_TX_SetRdsTransmittedGroupsMask(	FmTxContext 					*fmContext, 
															FmTxRdsTransmittedGroupsMask 	fieldsMask)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								fieldsMask,
								(_FM_TX_SetRdsTransmittedFieldsMaskVerifyParms(fieldsMask)  == FM_TX_STATUS_SUCCESS),
								FM_TX_CMD_SET_RDS_TRANSMITTED_MASK,
								"FM_TX_SetRdsTransmittedGroupsMask");


}

FmTxStatus FM_TX_GetRdsTransmittedGroupsMask(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TRANSMITTED_MASK,
								"FM_TX_GetRdsTransmittedGroupsMask");

}

FmTxStatus FM_TX_SetMonoStereoMode(FmTxContext *fmContext, FmTxMonoStereoMode mode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								mode,
								((mode == FM_TX_STEREO_MODE) || (mode == FM_TX_MONO_MODE)),
								FM_TX_CMD_SET_MONO_STEREO_MODE,
								"FM_TX_SetMonoStereoMode");
}
FmTxStatus FM_TX_GetMonoStereoMode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_MONO_STEREO_MODE,
								"FM_TX_GetMonoStereoMode");

}
FmTxStatus FM_TX_SetPreEmphasisFilter(FmTxContext *fmContext, FmcEmphasisFilter filter)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								filter,
								(filter <= FMC_EMPHASIS_FILTER_75_USEC),
								FM_TX_CMD_SET_PRE_EMPHASIS_FILTER,
								"FM_TX_SetPreEmphasisFilter");

}

FmTxStatus FM_TX_GetPreEmphasisFilter(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_PRE_EMPHASIS_FILTER,
								"FM_TX_GetPreEmphasisFilter");


}
FmTxStatus FM_TX_SetRdsTransmissionMode(FmTxContext *fmContext, FmTxRdsTransmissionMode mode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								mode,
	/*Rds Must be disabled if changing mode*/  ((!FM_TX_SM_IsRdsEnabled())&&((mode == FMC_RDS_TRANSMISSION_AUTOMATIC) || (mode == FMC_RDS_TRANSMISSION_MANUAL))),
								FM_TX_CMD_SET_RDS_TRANSMISSION_MODE,
								"FM_TX_SetRdsTransmissionMode");
}
FmTxStatus FM_TX_GetRdsTransmissionMode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TRANSMISSION_MODE,
								"FM_TX_GetRdsTransmissionMode");

}

FmTxStatus FM_TX_SetRdsAfCode(FmTxContext *fmContext, FmcAfCode afCode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								afCode,
								(FMC_UTILS_IsAfCodeValid( afCode) == FMC_TRUE),
								FM_TX_CMD_SET_RDS_AF_CODE,
								"FM_TX_SetRdsAfCode");
}

FmTxStatus FM_TX_GetRdsAfCode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_AF_CODE,
								"FM_TX_GetRdsAfCode");

}
FmTxStatus FM_TX_SetRdsPiCode(FmTxContext *fmContext, FmcRdsPiCode piCode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								piCode,
								FMC_TRUE,
								FM_TX_CMD_SET_RDS_PI_CODE,
								"FM_TX_SetRdsPiCode");
}
FmTxStatus FM_TX_GetRdsPiCode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_PI_CODE,
								"FM_TX_GetRdsPiCode");

}
FmTxStatus FM_TX_SetRdsPtyCode(FmTxContext *fmContext, FmcRdsPtyCode ptyCode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								ptyCode,
								(ptyCode<=FMC_RDS_PTY_CODE_MAX_VALUE),
								FM_TX_CMD_SET_RDS_PTY_CODE,
								"FM_TX_SetRdsPtyCode");
}

FmTxStatus FM_TX_GetRdsPtyCode(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_PTY_CODE,
								"FM_TX_GetRdsPtyCode");
}

FmTxStatus FM_TX_SetRdsTextRepertoire(FmTxContext *fmContext, FmcRdsRepertoire repertoire)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								repertoire,
								(repertoire<=FMC_RDS_REPERTOIRE_G2_CODE_TABLE),
								FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE,
								"FM_TX_SetRdsTextRepertoire");
}

FmTxStatus FM_TX_GetRdsTextRepertoire(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE,
								"FM_TX_GetRdsTextRepertoire");
}
FmTxStatus FM_TX_SetRdsPsScrollSpeed(FmTxContext 		*fmContext, 
												FmcRdsPsScrollSpeed 		speed)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								speed,
								FMC_TRUE,
								FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED,
								"FM_TX_SetRdsPsScrollSpeed");
}
FmTxStatus FM_TX_GetRdsPsScrollSpeed(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED,
								"FM_TX_GetRdsPsScrollSpeed");
}

FmTxStatus FM_TX_SetRdsTrafficCodes(FmTxContext 		*fmContext, 
											FmcRdsTaCode	taCode,
											FmcRdsTpCode 	tpCode)
{
	FmTxStatus							status;
	FmTxSetRdsTrafficCodesCmd	*setRdsTrafficCodesCmd = NULL;
	
	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_SetRdsTrafficCodes");

	FMC_VERIFY_ERR(	(fmContext == FM_TX_SM_GetContext()), 
						FMC_STATUS_INVALID_PARM, 
						("FM_TX_SetRdsTrafficCodes: Invalid Context Ptr"));

	FMC_VERIFY_ERR(((taCode<=1)&&( tpCode <= 1)) , 
						FM_TX_STATUS_INVALID_PARM, ("FM_TX_SetRdsTrafficCodes: Invalid params (%d , %d)", taCode,tpCode));
	FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
				FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
				("FM_TX_SetRdsTrafficCodes: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));
	
	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
				, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
				("FM_TX_SetRdsTrafficCodes: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(	fmContext, 
													FM_TX_CMD_SET_RDS_TRAFFIC_CODES, 
													(FmcBaseCmd**)&setRdsTrafficCodesCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_SetRdsTrafficCodes"));

	/* Copy cmd parms for the cmd execution phase*/
	setRdsTrafficCodesCmd->taCode= taCode;
	setRdsTrafficCodesCmd->tpCode= tpCode;

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
	
	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_GetRdsTrafficCodes(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_TRAFFIC_CODES,
								"FM_TX_GetRdsTrafficCodes");
}

FmTxStatus FM_TX_SetRdsMusicSpeechFlag(FmTxContext *fmContext, FmcRdsMusicSpeechFlag	musicSpeechFlag)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								musicSpeechFlag,
								(musicSpeechFlag == FMC_RDS_SPEECH)||(musicSpeechFlag == FMC_RDS_MUSIC),
								FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG,
								"FM_TX_SetRdsMusicSpeechFlag");
}

FmTxStatus FM_TX_GetRdsMusicSpeechFlag(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG,
								"FM_TX_GetRdsMusicSpeechFlag");
}

FmTxStatus FM_TX_SetRdsECC(FmTxContext *fmContext, FmcRdsExtendedCountryCode countryCode)
{
	return  _FM_TX_SimpleCmdAndCopyParams(fmContext, 
								          countryCode,
								          (countryCode <= 255),
								          FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE,
								          "FM_TX_SetRdsECC");
}

FmTxStatus FM_TX_GetRdsECC(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
							 FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE,
							 "FM_TX_GetRdsECC");
}

FmTxStatus FM_TX_WriteRdsRawData(FmTxContext *fmContext, const FMC_U8 *rdsRawData, FMC_UINT len)
{
	FmTxStatus				status;
	FmTxSetRdsRawDataCmd	*cmd = NULL;

	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_WriteRdsRawData");
	
	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("FM_TX_WriteRdsRawData: Invalid Context Ptr"));

	FMC_VERIFY_ERR((len <= FM_RDS_RAW_MAX_MSG_LEN), FMC_STATUS_INVALID_PARM, ("FM_TX_WriteRdsRawData: Raw data longer then max size"));

	FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_MANUAL), 
					FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON, 
					("FM_TX_WriteRdsRawData: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON)));
	
	FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
					, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
					("FM_TX_WriteRdsRawData: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));

	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, FM_TX_CMD_WRITE_RDS_RAW_DATA, (FmcBaseCmd**)&cmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_WriteRdsRawData"));

	/* Copy cmd parms for the cmd execution phase*/
	FMC_OS_MemCopy(cmd->rawData, rdsRawData, len);
	cmd->rawLen = len;
	
	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}

FmTxStatus FM_TX_ReadRdsRawData(FmTxContext *fmContext)
{
	return  _FM_TX_SimpleCmd(fmContext, 
								FM_TX_CMD_READ_RDS_RAW_DATA,
								"FM_TX_ReadRdsRawData");

}

FmTxStatus FM_TX_ChangeAudioSource(FmTxContext *fmContext, FmTxAudioSource txSource, ECAL_SampleFrequency eSampleFreq)
{
	FmTxStatus							status;
	FmTxSetAudioSourceCmd	*setAudioSourceCmd = NULL;
	
	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_SetAudioSource");
	
	FMC_VERIFY_ERR( (fmContext == FM_TX_SM_GetContext()), 
						FMC_STATUS_INVALID_PARM, 
						("FM_TX_SetAudioSource: Invalid Context Ptr"));
	
	/*FMC_VERIFY_ERR(((taCode<=1)&&( tpCode <= 1)) , 
						FM_TX_STATUS_INVALID_PARM, ("FM_TX_SetAudioSource: Invalid params (%d , %d)", taCode,tpCode));*/
	
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue( fmContext, 
													FM_TX_CMD_CHANGE_AUDIO_SOURCE, 
													(FmcBaseCmd**)&setAudioSourceCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_SetAudioSource"));
	
	/* Copy cmd parms for the cmd execution phase*/
	setAudioSourceCmd->txSource= txSource;
	setAudioSourceCmd->eSampleFreq = eSampleFreq;
	
	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
	
	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();
	
	return status;
}

FmTxStatus FM_TX_ChangeDigitalSourceConfiguration(FmTxContext *fmContext, ECAL_SampleFrequency eSampleFreq)
{
	FmTxStatus							status;
	FmTxSetDigitalAudioConfigurationCmd 	*setDigitalSourceConfigurationCmd = NULL;
	
	_FM_TX_FUNC_START_AND_LOCK_ENABLED("FM_TX_ChangeDigitalSourceConfiguration");
	
	FMC_VERIFY_ERR( (fmContext == FM_TX_SM_GetContext()), 
						FMC_STATUS_INVALID_PARM, 
						("FM_TX_ChangeDigitalSourceConfiguration: Invalid Context Ptr"));
	
	/*FMC_VERIFY_ERR(((taCode<=1)&&( tpCode <= 1)) , 
						FM_TX_STATUS_INVALID_PARM, ("FM_RX_ChangeDigitalSourceConfiguration: Invalid params (%d , %d)", taCode,tpCode));*/
	/*FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
				FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
				("FM_TX_ChangeDigitalSourceConfiguration: %s",_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));*/
	

	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue( fmContext, 
													FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION, 
													(FmcBaseCmd**)&setDigitalSourceConfigurationCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, ("FM_TX_ChangeDigitalSourceConfiguration"));
	
	/* Copy cmd parms for the cmd execution phase*/
	setDigitalSourceConfigurationCmd->eSampleFreq = eSampleFreq;
	
	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
	
	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();
	
	return status;

}
FmTxStatus _FM_TX_SimpleCmdAndCopyParams(FmTxContext *fmContext, 
								FMC_UINT paramIn,
								FMC_BOOL condition,
								FmTxCmdType cmdT,
								const char * funcName)
{
	FmTxStatus		status;
	FmTxSimpleSetOneParamCmd * baseCmd;
	_FM_TX_FUNC_START_AND_LOCK_ENABLED(funcName);
	
	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("%s: Invalid Context Ptr",funcName));

	/* Verify that the band value is valid */
	FMC_VERIFY_ERR(condition, 
						FM_TX_STATUS_INVALID_PARM, ("%s: Invalid param",funcName));

	/*Rds Auto Actions can be preformed only if we are in Auto mode and no change RDS mode is in the queue*/
	if(_FM_TX_IsRdsAutoAction(cmdT))
	{
		FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
					FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
					("%s: %s",funcName,_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));
	
		FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
					, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
					("%s: %s",funcName,_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));
	}
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, cmdT, (FmcBaseCmd**)&baseCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, (funcName));

	/* Copy cmd parms for the cmd execution phase*/
	/*we assume here strongly that the stract pointed by base command as as its first field base command and another field with the param*/
	baseCmd->param = paramIn;

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);

	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}
FmTxStatus _FM_TX_SimpleCmd(FmTxContext *fmContext, 
								FmTxCmdType cmdT,
								const char * funcName)
{
	FmTxStatus				status;
	FmTxSimpleSetOneParamCmd * baseCmd;
	_FM_TX_FUNC_START_AND_LOCK_ENABLED(funcName);
	
	FMC_VERIFY_ERR((fmContext == FM_TX_SM_GetContext()), FMC_STATUS_INVALID_PARM, ("%s: Invalid Context Ptr",funcName));

	/*Rds Auto Actions can be preformed only if we are in Auto mode and no change RDS mode is in the queue*/
	if(_FM_TX_IsRdsAutoAction(cmdT))
	{
		FMC_VERIFY_ERR((FM_TX_SM_GetRdsMode()==FMC_RDS_TRANSMISSION_AUTOMATIC), 
					FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON, 
					("%s: %s",funcName,_FM_TX_FmTxStatusStr(FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON)));
	
		FMC_VERIFY_ERR((FM_TX_SM_IsCmdPending(FM_TX_CMD_SET_RDS_TRANSMISSION_MODE)==FMC_FALSE)
					, FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS, 
					("%s: %s",funcName,_FM_TX_FmTxStatusStr(FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS)));
	}
	/* Allocates the command and insert to commands queue */
	status = FM_TX_SM_AllocateCmdAndAddToQueue(fmContext, cmdT, (FmcBaseCmd**)&baseCmd);
	FMC_VERIFY_ERR((status == FMC_STATUS_SUCCESS), status, (funcName));

	status = FM_TX_STATUS_PENDING;
	
	/* Trigger TX SM to execute the command in FM Task context */
	FMCI_NotifyFmTask(FMC_OS_EVENT_GENERAL);
	
	_FM_TX_FUNC_END_AND_UNLOCK_ENABLED();

	return status;
}



/*
	A PS Message is valid if Its length is between 1 - FM_RDS_PS_MAX_MSG_LEN
*/
FmTxStatus _FM_TX_SetRdsTextPsMsgVerifyParms(	const FMC_U8 		*msg,
															FMC_UINT 			len)
{
	FmTxStatus status;

	FMC_FUNC_START("_FM_TX_SetRdsTextPsMsgVerifyParms");

	FMC_VERIFY_ERR((msg != NULL), FM_TX_STATUS_INVALID_PARM,
						("FM_TX_SetRdsTextPsMsg: Null msg pointer"));
	FMC_VERIFY_ERR((len > 0), FM_TX_STATUS_INVALID_PARM,
						("FM_TX_SetRdsTextPsMsg: Empty Msg"));
	FMC_VERIFY_ERR((len <= FM_RDS_PS_MAX_MSG_LEN), FM_TX_STATUS_INVALID_PARM,
						("FM_TX_SetRdsTextPsMsg: Msg too long (%d), Max: %d", len, FM_RDS_PS_MAX_MSG_LEN));
	
	status = FM_TX_STATUS_SUCCESS;
	
	FMC_FUNC_END();

	return status;
}

/*
	An RT Message is valid if:
	1. The RT type is either A or B
	2. The length is between 1 - FM_RDS_RT_A_MAX_MSG_LEN (for type A) or FM_RDS_RT_B_MAX_MSG_LEN (for type B)
*/

FmTxStatus _FM_TX_SetRdsTextRtMsgVerifyParms(	FmcRdsRtMsgType		msgType, 
															const FMC_U8 		*msg,
															FMC_UINT 			len)
{
	FmTxStatus status;

	FMC_FUNC_START("_FM_TX_SetRdsTextRtMsgVerifyParms");

	FMC_VERIFY_ERR(((msgType == FMC_RDS_RT_MSG_TYPE_AUTO)||(msgType == FMC_RDS_RT_MSG_TYPE_A) || (msgType == FMC_RDS_RT_MSG_TYPE_B)),
						FM_TX_STATUS_INVALID_PARM, ("_FM_TX_SetRdsTextRtMsgVerifyParms: Invalid Type (%d)", msgType));

	FMC_VERIFY_ERR((msg != NULL), FM_TX_STATUS_INVALID_PARM, ("_FM_TX_SetRdsTextRtMsgVerifyParms: Null msg"));
	
	if (msgType == FMC_RDS_RT_MSG_TYPE_A)
	{
		FMC_VERIFY_ERR((len <= FM_RDS_RT_A_MAX_MSG_LEN), FM_TX_STATUS_INVALID_PARM,
							("_FM_TX_SetRdsTextRtMsgVerifyParms: Msg too long (%d) for RT Type A (Max: %d)",
							len, FM_RDS_RT_A_MAX_MSG_LEN));
	}
	else/*B or Auto*/
	{
		/* In Auto mode the BB will decide ,depending on the length, wich group type to send*/
		FMC_VERIFY_ERR((len <= FM_RDS_RT_B_MAX_MSG_LEN), FM_TX_STATUS_INVALID_PARM,
							("_FM_TX_SetRdsTextRtMsgVerifyParms: Msg too long (%d) for RT Type B (Max: %d)",
							len, FM_RDS_RT_B_MAX_MSG_LEN));
	}

	status = FM_TX_STATUS_SUCCESS;
	
	FMC_FUNC_END();

	return status;
}

/*
	A fields mask is valid if:
	1. At least one field is transmitted
*/
FmTxStatus _FM_TX_SetRdsTransmittedFieldsMaskVerifyParms(FmTxRdsTransmittedGroupsMask fieldsMask)
{
	FmTxStatus status;

	FMC_FUNC_START("_FM_TX_SetRdsTransmittedFieldsMaskVerifyParms");
	
	/* Verify that either PS or RT are transmitted */

	FMC_VERIFY_ERR(((fieldsMask & FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK ) || 
							(fieldsMask & FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK )||
							(fieldsMask & FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK )),
						FM_TX_STATUS_INVALID_PARM,
						("_FM_TX_SetRdsTransmittedFieldsMaskVerifyParms: Either PS or RT or ECC must be transmitted"));

	status = FM_TX_STATUS_SUCCESS;

	FMC_FUNC_END();

	return status;
}
/*
*	Thses RDS actions are relevant only when in Auto mode
*
*/
FMC_BOOL _FM_TX_IsRdsAutoAction(FmTxCmdType cmdT)
{
	if((cmdT == FM_TX_CMD_SET_RDS_AF_CODE)||
		(cmdT == FM_TX_CMD_GET_RDS_AF_CODE)||
		(cmdT == FM_TX_CMD_SET_RDS_PI_CODE)||
		(cmdT == FM_TX_CMD_GET_RDS_PI_CODE)||
		(cmdT == FM_TX_CMD_SET_RDS_PTY_CODE)||
		(cmdT == FM_TX_CMD_GET_RDS_PTY_CODE)||
		(cmdT == FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE)||
		(cmdT == FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE)||
		(cmdT == FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE)||
		(cmdT == FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE)||
		(cmdT == FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED)||
		(cmdT == FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED)||
		(cmdT == FM_TX_CMD_SET_RDS_TEXT_PS_MSG)||
		(cmdT == FM_TX_CMD_GET_RDS_TEXT_PS_MSG)||
		(cmdT == FM_TX_CMD_SET_RDS_TEXT_RT_MSG)||
		(cmdT == FM_TX_CMD_GET_RDS_TEXT_RT_MSG)||
		(cmdT == FM_TX_CMD_SET_RDS_TRANSMITTED_MASK)||
		(cmdT == FM_TX_CMD_GET_RDS_TRANSMITTED_MASK)||
		(cmdT == FM_TX_CMD_SET_RDS_TRAFFIC_CODES)||
		(cmdT == FM_TX_CMD_GET_RDS_TRAFFIC_CODES)||
		(cmdT == FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG)||
		(cmdT == FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG)||
		(cmdT == FM_TX_CMD_SET_PRE_EMPHASIS_FILTER)||
		(cmdT == FM_TX_CMD_GET_PRE_EMPHASIS_FILTER)||
		(cmdT == FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE)||
		(cmdT == FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE))
	{
		return FMC_TRUE;
	}
	return FMC_FALSE;
}
FMC_U8 *_FM_TX_FmTxStatusStr(FmTxStatus status)
{
	switch (status)
	{
		case FM_TX_STATUS_SUCCESS: return (FMC_U8*)"SUCCESS";
		case FM_TX_STATUS_FAILED: return (FMC_U8*)"FAILED";
		case FM_TX_STATUS_PENDING: return (FMC_U8*)"PENDING";
		case FM_TX_STATUS_INVALID_PARM: return (FMC_U8*)"INVALID_PARM";
		case FM_TX_STATUS_IN_PROGRESS: return (FMC_U8*)"IN_PROGRESS";
		case FM_TX_STATUS_NOT_APPLICABLE: return (FMC_U8*)"NOT_APPLICABLE";
		case FM_TX_STATUS_NOT_SUPPORTED: return (FMC_U8*)"NOT_SUPPORTED";
		case FM_TX_STATUS_INTERNAL_ERROR: return (FMC_U8*)"INTERNAL_ERROR";
		case FM_TX_STATUS_TRANSPORT_INIT_ERR: return (FMC_U8*)"TRANSPORT_INIT_ERR";
		case FM_TX_STATUS_HARDWARE_ERR: return (FMC_U8*)"HARDWARE_ERR";
		case FM_TX_STATUS_NO_VALUE_AVAILABLE: return (FMC_U8*)"NO_VALUE_AVAILABLE";
		case FM_TX_STATUS_CONTEXT_DOESNT_EXIST: return (FMC_U8*)"CONTEXT_DOESNT_EXIST";
		case FM_TX_STATUS_CONTEXT_NOT_DESTROYED: return (FMC_U8*)"CONTEXT_NOT_DESTROYED";
		case FM_TX_STATUS_CONTEXT_NOT_ENABLED: return (FMC_U8*)"CONTEXT_NOT_ENABLED";
		case FM_TX_STATUS_CONTEXT_NOT_DISABLED: return (FMC_U8*)"CONTEXT_NOT_DISABLED";
		case FM_TX_STATUS_NOT_DE_INITIALIZED: return (FMC_U8*)"NOT_DE_INITIALIZED";
		case FM_TX_STATUS_NOT_INITIALIZED: return (FMC_U8*)"NOT_INITIALIZED";
		case FM_TX_STATUS_TOO_MANY_PENDING_CMDS: return (FMC_U8*)"TOO_MANY_PENDING_CMDS";
		case FM_TX_STATUS_DISABLING_IN_PROGRESS: return (FMC_U8*)"DISABLING_IN_PROGRESS";

		case FM_TX_STATUS_RDS_NOT_ENABLED: return (FMC_U8*)"RDS_NOT_ENABLED";
		case FM_TX_STATUS_TRANSMITTER_NOT_TUNED: return (FMC_U8*)"TRANSMITTER_NOT_TUNED";
		case FM_TX_STATUS_TRANSMISSION_IS_NOT_ON: return (FMC_U8*)"TRANSMISSION_IS_NOT_ON";
		case FM_TX_STATUS_FM_RX_ALREADY_ENABLED: return (FMC_U8*)"FM_RX_ALREADY_ENABLED";
		case FM_TX_STATUS_AUTO_MODE_NOT_ON: return (FMC_U8*)"AUTO_MODE_NOT_ON";
		case FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON: return (FMC_U8*)"RDS_AUTO_MODE_NOT_ON";
		case FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON: return (FMC_U8*)"RDS_MANUAL_MODE_NOT_ON";
		case FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS: return (FMC_U8*)"CONFLICTING_RDS_CMD_IN_PROGRESS";
		case FM_TX_STATUS_PROCESS_TIMEOUT_FAILURE: return (FMC_U8*)"FM_TX_STATUS_PROCESS_TIMEOUT_FAILURE";
		default:	return (FMC_U8*)"INVALID Status";
	}
}

