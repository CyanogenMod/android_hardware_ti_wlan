/*
 * TI's FM
 *
 * Copyright 2001-2010 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010 Sony Ericsson Mobile Communications AB.
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
/*
 * JFmRxNative.cpp
 */

#define LOG_TAG "JFmRxNative"

#ifdef DEBUG_TRACE_ON
#define FM_LOGD(x...) LOGD( x )
#else
#define FM_LOGD(x...) do {} while(0)
#endif

#include "android_runtime/AndroidRuntime.h"
#include "jni.h"
#include "JNIHelp.h"
#include "utils/Log.h"

#define DEBUG_TRACE_ON
extern "C" {

#include "fm_rx.h"
#include "fmc_debug.h"
#include "fmc_core.h"
#include "fmc_defs.h"
#include "fm_trace.h"
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include "bluetooth.h"
#include <cutils/properties.h>

/* transient stage */
typedef FmcStatus fm_status;
typedef FmRxStatus fm_rx_status;
typedef FmRxContext fm_rx_context_s;
typedef FmRxEvent fm_rx_event_s;
void fmapp_rx_callback(const fm_rx_event_s *event);
void fmrx_error_callback(const fm_rx_status status);

}

namespace android {
} //namespace android

JNIEnv *_jFmEnvTest = NULL;
static JavaVM *g_jVM = NULL;
static jclass _sJClass;

static jmethodID _sMethodId_nativeCb_fmRxRawRDS;
static jmethodID _sMethodId_nativeCb_fmRxRadioText;
static jmethodID _sMethodId_nativeCb_fmRxPiCodeChanged;
static jmethodID _sMethodId_nativeCb_fmRxPtyCodeChanged;
static jmethodID _sMethodId_nativeCb_fmRxPsChanged;
static jmethodID _sMethodId_nativeCb_fmRxMonoStereoModeChanged;
static jmethodID _sMethodId_nativeCb_fmRxAudioPathChanged;
static jmethodID _sMethodId_nativeCb_fmRxAfSwitchFreqFailed;
static jmethodID _sMethodId_nativeCb_fmRxAfSwitchStart;
static jmethodID _sMethodId_nativeCb_fmRxAfSwitchComplete;
static jmethodID _sMethodId_nativeCb_fmRxAfListChanged;
static jmethodID _sMethodId_nativeCb_fmRxCmdDone;
static jmethodID _sMethodId_nativeCb_fmRxCompleteScanDone;
static jmethodID _sMethodId_nativeCb_fmapp_rx_callback;
    static jmethodID _sMethodId_nativeCb_fmrx_error_callback;
    static jmethodID _sMethodId_nativeCb_fmRxCmdError;

static bool fm_stack_debug = false;


static void fmapp_termination_handler(int sig)
{

	FM_LOGD("fmapp_termination_handler");

	/* silence compiler warning */
	(void)sig;


}

fm_status register_fmsig_handlers(void)
{
	struct sigaction sa;
	fm_status ret = FMC_STATUS_SUCCESS;

	FM_LOGD("register_fmsig_handlers");


	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fmapp_termination_handler;

	if (-1 == sigaction(SIGTERM, &sa, NULL)) {
		LOGE("failed to register SIGTERM");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	if (-1 == sigaction(SIGINT, &sa, NULL)) {
		LOGE("failed to register SIGINT");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	if (-1 == sigaction(SIGQUIT, &sa, NULL)) {
		LOGE("failed to register SIGQUIT");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

out:

	FM_LOGD("register_fmsig_handlers exit");

	return ret;
}

static int nativeJFmRx_Create(JNIEnv *env,jobject obj,jobject jContextValue)
{
    FmRxStatus fmStatus;
    FmRxContext *fmRxContext =NULL;
    jclass contextCls = NULL;
    jmethodID setValueMethodId = NULL;

	int ret = 0;
	static int g_fmapp_hci_dev = 0;

	char prop[PROPERTY_VALUE_MAX];


	FM_LOGD("%s: Entered", __func__);


	property_get("ro.semc.fm_stack.debug", prop, "0");

	if (0 == strncmp("1", prop, PROPERTY_VALUE_MAX)) {
		ret = fm_trace_init();
		fm_stack_debug = true;
	}

	if (bt_chip_enable() < 0) {

		FM_LOGD("bt_chip_enable() failed");

		goto CLEANUP;
	} else {

		FM_LOGD("bt_chip_enable() done");

	}

	register_fmsig_handlers();
	ret = fm_open_cmd_socket(g_fmapp_hci_dev);
	if (ret) {
		LOGE("failed to open cmd socket ret %d",ret);
		goto CLEANUP;
	}


		FM_LOGD("nativeJFmRx_Create(): Calling FM_RX_Init");



	fmStatus = FM_RX_Init(fmrx_error_callback);
	if (fmStatus) {
	LOGE("failed to init FM rx stack context");
	goto CLEANUP;

	}
		
	fmStatus = FM_RX_Create(NULL, fmapp_rx_callback, &fmRxContext);

	if (fmStatus) {
		LOGE("failed to create FM rx stack context");
		goto CLEANUP;
	} else {

		FM_LOGD("create FM rx stack context ret = %x",
		     (unsigned int)fmRxContext);

	}

	FM_LOGD("%s: FM_RX_Create returned %d, context: %x", __func__,
	     (int)fmStatus,
	     (unsigned int)fmRxContext);


	FM_LOGD("%s: Setting context value in jContext out parm", __func__);


	/* Pass received fmContext via the jContextValue object */
	contextCls = env->GetObjectClass(jContextValue);
	if (contextCls == NULL)	{

		FM_LOGD("%s: Failed obtaining class for JBtlProfileContext",
		     __func__);

		goto CLEANUP;
	}

	setValueMethodId = env->GetMethodID(contextCls, "setValue", "(I)V");
	if (setValueMethodId == NULL)
	{
	   	LOGE("nativeJFmRx_create: Failed getting setValue method id");
		goto CLEANUP;
	}
	
	FM_LOGD("nativeJFmRx_create: Calling Java setValue(ox%x) in context's class", (unsigned int)fmRxContext);


	env->CallVoidMethod(jContextValue, setValueMethodId, (unsigned int)fmRxContext);
	if (env->ExceptionOccurred())
	{
		LOGE("nativeJFmRx_create: Calling CallVoidMethod(setValue) failed");
		env->ExceptionDescribe();
		goto CLEANUP;
	}

	FM_LOGD("nativeJFmRx_create:Exiting Successfully");


	return fmStatus;


CLEANUP:
	
	LOGE("nativeJFmRx_create(): Exiting With a Failure indication");
	return FMC_STATUS_FAILED;
}



static jint nativeJFmRx_Destroy(JNIEnv *env, jobject obj,jlong jContextValue)
{
    FmRxContext * fmRxContext = (FmRxContext *)jContextValue;
    FmRxStatus  status ;
	
	FM_LOGD("%s: Entered, calling FM_RX_Destroy", __func__);


	status = FM_RX_Destroy(&fmRxContext);

	if (status) {
		LOGE("failed to destroy FM rx stack context");
		return status;

	} else{
			
			FM_LOGD("destroy FM rx stack context Success");

	}
	
	FM_LOGD("nativeJFmRx_destroy(): After calling FM_RX_Destroy ret val = %d", status);

	FM_LOGD("nativeJFmRx_destroy(): calling FM_RX_Deinit");
		

	status = FM_RX_Deinit();

	FM_LOGD("%s: After calling FM_RX_Deinit", __func__);


	if (status) {
		LOGE("failed to deinit FM rx stack context");
		return status;

	} else {

	FM_LOGD("deinit FM rx stack context Success");

	}
		
	fm_close_cmd_socket();

	
	FM_LOGD("%s: calling bt_chip_disable", __func__);

	if(bt_chip_disable() < 0) {
		LOGE("bt_chip_disable failed");
	} else {

		FM_LOGD("bt_chip_disable Success");

	}


	FM_LOGD("%s: calling fm_close_cmd_socket", __func__);


	if (fm_stack_debug) {
		fm_trace_deinit();
	}


	FM_LOGD("%s: Exit", __func__);

	return status;

}


static int nativeJFmRx_Enable(JNIEnv *env, jobject obj, jlong jContextValue)
{
    FmRxContext * fmRxContext = (FmRxContext *)jContextValue;


	FM_LOGD("%s: Entered", __func__);


    FmRxStatus  status =FM_RX_Enable(fmRxContext);
	
    FM_LOGD("%s: FM_RX_Enable() returned %d",__func__,(int)status);

	FM_LOGD("%s: Exit", __func__);

    return status;
}



static int nativeJFmRx_Disable(JNIEnv *env, jobject obj, jlong jContextValue)
{
    FmRxContext * fmRxContext = (FmRxContext *)jContextValue;


	FM_LOGD("%s: Entered", __func__);


    FmRxStatus  status =FM_RX_Disable(fmRxContext);
	
    FM_LOGD("%s: FM_RX_Disable() returned %d",__func__,(int)status);

	FM_LOGD("%s: Exit", __func__);

    return status;
}



static int nativeJFmRx_SetBand(JNIEnv *env, jobject obj,jlong jContextValue, jint jFmBand)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;
	FmcBand band = (FmcBand)jFmBand;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_SetBand(fmRxContext, (FmcBand)band);

	FM_LOGD("%s: FM_RX_SetBand() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetBand(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus  status =FM_RX_GetBand(fmRxContext);

	FM_LOGD("%s: FM_RX_GetBand() returned %d", __func__, (int)status);

	FM_LOGD("%s: nativeJFmRx_getBand(): Exit", __func__);

	return status;
}


static int nativeJFmRx_Tune(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmFreq)
{
	FmRxContext *fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus  status =FM_RX_Tune(fmRxContext, (FmcFreq)jFmFreq);

	FM_LOGD("%s: FM_RX_Tune() returned %d", __func__, (int)status);


	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetTunedFrequency(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_GetTunedFrequency(fmRxContext);

	FM_LOGD("%s: FM_RX_GetTunedFrequency() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_SetMonoStereoMode(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmMode)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_SetMonoStereoMode(fmRxContext,
				(FmRxMonoStereoMode)jFmMode);

	FM_LOGD("%s: FM_RX_SetMonoStereoMode() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetMonoStereoMode(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetMonoStereoMode(fmRxContext);

	FM_LOGD("%s: FM_RX_GetMonoStereoMode() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_SetMuteMode(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmMuteMode)
{

	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetMuteMode(fmRxContext,
				(FmcMuteMode)jFmMuteMode);

	FM_LOGD("%s: FM_RX_SetMuteMode() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetMuteMode(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetMuteMode(fmRxContext);

	FM_LOGD("%s: FM_RX_GetMuteMode() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_SetRssiThreshold(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmRssi)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_SetRssiThreshold(fmRxContext,
				(FMC_INT)jFmRssi);

	FM_LOGD("%s: FM_RX_SetRssiThreshold() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_GetRssiThreshold(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRssiThreshold(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRssiThreshold() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_GetRssi(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRssi(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRssi() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_SetVolume(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmVolume)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetVolume(fmRxContext,(FMC_UINT)jFmVolume);

	FM_LOGD("%s: FM_RX_SetVolume() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}

static int nativeJFmRx_GetVolume(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_GetVolume(fmRxContext);

	FM_LOGD("%s: FM_RX_GetVolume() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_SetChannelSpacing(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmChannelSpacing)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_SetChannelSpacing(fmRxContext,
				(FMC_UINT)jFmChannelSpacing);

	FM_LOGD("%s: FM_RX_SetChannelSpacing() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}

static int nativeJFmRx_GetChannelSpacing(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetChannelSpacing(fmRxContext);

	FM_LOGD("%s: FM_RX_GetChannelSpacing() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static jint nativeJFmRx_SetDeEmphasisFilter(JNIEnv *env, jobject obj,jlong jContextValue,jint jFmEmphasisFilter)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetDeEmphasisFilter(fmRxContext,
				(FmcEmphasisFilter) jFmEmphasisFilter);

	FM_LOGD("%s: FM_RX_SetDeEmphasisFilter() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetDeEmphasisFilter(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetDeEmphasisFilter(fmRxContext);

	FM_LOGD("%s: FM_RX_GetDeEmphasisFilter() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}



static int nativeJFmRx_Seek(JNIEnv *env, jobject obj,jlong jContextValue,jint jdirection)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;
	FmRxSeekDirection direction = (FmRxSeekDirection)jdirection;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_Seek(fmRxContext,
				(FmRxSeekDirection)direction);

	FM_LOGD("%s: FM_RX_Seek() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_StopSeek(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext =(FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_StopSeek(fmRxContext);

	FM_LOGD("%s: FM_RX_StopSeek() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_EnableRDS(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_EnableRds(fmRxContext);

	FM_LOGD("%s: FM_RX_EnableRds() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_DisableRDS(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_DisableRds(fmRxContext);

	FM_LOGD("%s: FM_RX_DisableRds() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_EnableAudioRouting(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_EnableAudioRouting(fmRxContext);

	FM_LOGD("%s: FM_RX_EnableAudioRouting() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int  nativeJFmRx_DisableAudioRouting(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_DisableAudioRouting(fmRxContext);

	FM_LOGD("%s: FM_RX_DisableAudioRouting() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_SetRdsAfSwitchMode(JNIEnv *env, jobject obj,jlong jContextValue,jint jRdsAfSwitchMode)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_SetRdsAfSwitchMode(fmRxContext,
				(FmRxRdsAfSwitchMode)jRdsAfSwitchMode);

	FM_LOGD("%s: FM_RX_SetRdsAfSwitchMode() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_GetRdsAfSwitchMode(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRdsAfSwitchMode(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRdsAfSwitchMode() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_ChangeAudioTarget (JNIEnv *env, jobject obj,jlong jContextValue, jint jFmRxAudioTargetMask, jint digitalConfig)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_ChangeAudioTarget(fmRxContext,
				(FmRxAudioTargetMask)jFmRxAudioTargetMask,
				(ECAL_SampleFrequency)digitalConfig);

	FM_LOGD("%s: FM_RX_ChangeAudioTarget() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}


static int nativeJFmRx_ChangeDigitalTargetConfiguration(JNIEnv *env, jobject obj,jlong jContextValue,jint digitalConfig)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_ChangeDigitalTargetConfiguration(
				fmRxContext,
				(ECAL_SampleFrequency)digitalConfig);

	FM_LOGD("%s: FM_RX_ChangeDigitalTargetConfiguration() returned %d",
	     __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}


static int nativeJFmRx_SetRfDependentMuteMode(JNIEnv *env, jobject obj,jlong jContextValue, jint mode)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetRfDependentMuteMode(fmRxContext,
				(FmRxRfDependentMuteMode)mode);

	FM_LOGD("%s: FM_RX_SetRfDependentMuteMode() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetRfDependentMute(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRfDependentMute(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRfDependentMute() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}


static int nativeJFmRx_SetRdsSystem(JNIEnv *env, jobject obj,jlong jContextValue, jint rdsSystem)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetRdsSystem(fmRxContext,
				(FmcRdsSystem)rdsSystem);

	FM_LOGD("%s: FM_RX_SetRdsSystem() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetRdsSystem(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRdsSystem(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRdsSystem() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_SetRdsGroupMask(JNIEnv *env, jobject obj,jlong jContextValue, jlong groupMask)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_SetRdsGroupMask(fmRxContext,
				(FmcRdsGroupTypeMask)groupMask);

	FM_LOGD("%s: FM_RX_SetRdsGroupMask() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;

}

static int  nativeJFmRx_GetRdsGroupMask(JNIEnv *env, jobject obj,jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetRdsGroupMask(fmRxContext);

	FM_LOGD("%s: FM_RX_GetRdsGroupMask() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_CompleteScan(JNIEnv *env, jobject obj, jlong jContextValue)
{
	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);

	FmRxStatus status = FM_RX_CompleteScan(fmRxContext);

	FM_LOGD("%s: FM_RX_CompleteScan() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_GetCompleteScanProgress(JNIEnv *env, jobject obj, jlong jContextValue)
{

	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_GetCompleteScanProgress(fmRxContext);

	FM_LOGD("%s: FM_RX_GetCompleteScanProgress() returned %d", __func__,
	     (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_StopCompleteScan(JNIEnv *env, jobject obj, jlong jContextValue)
{

	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_StopCompleteScan(fmRxContext);

	FM_LOGD("%s: FM_RX_StopCompleteScan() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}

static int nativeJFmRx_IsValidChannel(JNIEnv *env, jobject obj, jlong jContextValue)
{

	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);


	FmRxStatus status = FM_RX_IsValidChannel(fmRxContext);

	FM_LOGD("%s: FM_RX_IsValidChannel() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


static int nativeJFmRx_GetFwVersion(JNIEnv *env, jobject obj, jlong jContextValue)
{

	FmRxContext * fmRxContext = (FmRxContext *)jContextValue;

	FM_LOGD("%s: Entered", __func__);
	FM_LOGD("%s: Exit", __func__);

	FmRxStatus status = FM_RX_GetFwVersion(fmRxContext);

	FM_LOGD("%s: FM_RX_GetFwVersion() returned %d", __func__, (int)status);

	FM_LOGD("%s: Exit", __func__);

	return status;
}


//##############################################################################
//								 SIGNALS
//##############################################################################

extern "C"
{


/**********************************************************************
*				Callback registration

***********************************************************************/


void fmrx_error_callback(const fm_rx_status status)
{

	LOGI("fmrx_error_callback: Entered, ");

	JNIEnv* env = NULL;
	bool attachedThread = false;
	int jRet ;

        /* check whether the current thread is attached to a virtual machine
           instance, if no only then try to attach to the current thread. */

	jRet = g_jVM->GetEnv((void **)&env,JNI_VERSION_1_4);

	if(jRet < 0)
	{
		LOGE("failed to get JNI env,assuming native thread");
		jRet = g_jVM->AttachCurrentThread((&env), NULL);

		if(jRet != JNI_OK)
		{
			LOGE("failed to atatch to current thread %d",jRet);
			return ;
		}

		attachedThread = true;
	}

	if(env == NULL)
	{
		LOGE("fmrx_error_callback: Entered, env is null");
		return;
	}



		FM_LOGD("fmrx_error_callback:Calling CallStaticVoidMethod");


		env->CallStaticVoidMethod(_sJClass,_sMethodId_nativeCb_fmRxCmdError, (jint)status
						);

  if (env->ExceptionOccurred())    {
    LOGE("fmrx_error_callback:  ExceptionOccurred");
    goto CLEANUP;
    }

    FM_LOGD("fmrx_error_callback: Exiting, Calling DetachCurrentThread at the END");


    if(attachedThread == true)
        g_jVM->DetachCurrentThread();

    return;
    
    CLEANUP:
	LOGE("fmrx_error_callback: Exiting due to failure");
	if (env->ExceptionOccurred())    {
	env->ExceptionDescribe();
	env->ExceptionClear();
	}
//Sujesh ChipManager Start
    if(attachedThread == true)
        g_jVM->DetachCurrentThread();
//Sujesh ChipManager End
        return;

}

void fmapp_rx_callback(const fm_rx_event_s *event)
{

	FM_LOGD("%s: Entered, got event %d", __func__, event->eventType);

	JNIEnv* env = NULL;
	jbyteArray jAfListData = NULL;
	jbyteArray jNameString= NULL;
	jbyteArray jRadioTxtMsg = NULL;
	jbyteArray jGroupData = NULL;
	jintArray jChannelsData = NULL ;
	jsize len = NULL ;

	g_jVM->AttachCurrentThread((&env), NULL);

	if(env == NULL)	{
		LOGI("%s: Entered, env is null", __func__);
	} else {

		FM_LOGD("%s: jEnv %p", __func__, (void *)env);

	}

	switch (event->eventType) {
		case FM_RX_EVENT_CMD_DONE:

		FM_LOGD("%s: Calling CallStaticVoidMethod", __func__);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxCmdDone,
				(jint)event->status,
				(jint)event->p.cmdDone.cmd,
				(jlong)event->p.cmdDone.value);
		break;

	case FM_RX_EVENT_MONO_STEREO_MODE_CHANGED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_MONO_STEREO_MODE_CHANGED",
		     __func__);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxMonoStereoModeChanged,
				(jint)event->status,
				(jint)event->p.monoStereoMode.mode);
		break;

	case FM_RX_EVENT_PI_CODE_CHANGED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_PI_CODE_CHANGED", __func__);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxPiCodeChanged,
				(jint)event->status,
				(jint)event->p.piChangedData.pi);
		break;

	case FM_RX_EVENT_AF_SWITCH_START:
		
		FM_LOGD("fmapp_rx_callback():EVENT --------------->FM_RX_EVENT_AF_SWITCH_START");    
		FM_LOGD("AF switch process has started...");
		
		env->CallStaticVoidMethod(_sJClass,_sMethodId_nativeCb_fmRxAfSwitchStart,
						(jint)event->status,
						(jint)event->p.afSwitchData.pi,
						(jint)event->p.afSwitchData.tunedFreq,
						(jint)event->p.afSwitchData.afFreq);

		break;

	case FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED",
		      __func__);
		FM_LOGD("AF switch to %d failed", event->p.afSwitchData.afFreq);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxAfSwitchFreqFailed,
				(jint)event->status,
				(jint)event->p.afSwitchData.pi,
				(jint)event->p.afSwitchData.tunedFreq,
				(jint)event->p.afSwitchData.afFreq);
		break;

	case FM_RX_EVENT_AF_SWITCH_COMPLETE:

		FM_LOGD("%s: EVENT FM_RX_EVENT_AF_SWITCH_COMPLETE", __func__);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxAfSwitchComplete,
				(jint)event->status,
				(jint)event->p.afSwitchData.pi,
				(jint)event->p.afSwitchData.tunedFreq,
				(jint)event->p.afSwitchData.afFreq);
		break;

	case FM_RX_EVENT_AF_LIST_CHANGED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_AF_LIST_CHANGED", __func__);

		jAfListData = env->NewByteArray(event->p.afListData.afListSize);
		if (jAfListData == NULL) {
			LOGE("%s: Failed converting elements", __func__);
			goto CLEANUP;
		}

		env->SetByteArrayRegion(jAfListData,
				0,
				event->p.afListData.afListSize,
				(jbyte*)event->p.afListData.afList);

		if (env->ExceptionOccurred()) {
			LOGE("%s: Calling nativeCb_fmRxAfListChanged failed",
			     __func__);
			goto CLEANUP;
		}

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxAfListChanged,
				(jint)event->status,
				(jint)event->p.afListData.pi,
				jAfListData,
				(jint)event->p.afListData.afListSize);
		break;

	case FM_RX_EVENT_PS_CHANGED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_PS_CHANGED '%s'",
		     __func__, (char *)(event->p.psData.name));

		len = strlen((char *)(event->p.psData.name));

		FM_LOGD("%s: EVENT FM_RX_EVENT_PS_CHANGED len %d", __func__, len);

		for(int i = 0; i< len ; i++)

			FM_LOGD("%s: fmRxPsChanged name[] =  %x",
			     __func__, event->p.psData.name[i]);

		jNameString = env->NewByteArray(len);
		if (jNameString == NULL) {
			LOGE("%s: Failed converting elements", __func__);
			goto CLEANUP;
		}

		env->SetByteArrayRegion(jNameString,
				0,
				len,
				(jbyte*)event->p.psData.name);

		if (env->ExceptionOccurred()) {
			LOGE("%s: nativeCb_fmRxRadioText failed", __func__);
			goto CLEANUP;
		}

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxPsChanged,
				(jint)event->status,
				(jint)event->p.psData.frequency,
				jNameString,
				(jint)event->p.psData.repertoire);
		break;

	case FM_RX_EVENT_RADIO_TEXT:

		FM_LOGD("%s: EVENT FM_RX_EVENT_RADIO_TEXT", __func__);

		jRadioTxtMsg = env->NewByteArray(event->p.radioTextData.len);
		if (jRadioTxtMsg == NULL) {
			LOGE("%s: Failed converting elements", __func__);
			goto CLEANUP;
		}

		env->SetByteArrayRegion(jRadioTxtMsg,
				0,
				event->p.radioTextData.len,
				(jbyte*)event->p.radioTextData.msg);

		if (env->ExceptionOccurred()) {
			LOGE("%s: Calling nativeCb_fmRxRadioText failed",
			     __func__);
			goto CLEANUP;
		}

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxRadioText,
				(jint)event->status,
				(jboolean)event->p.radioTextData.resetDisplay,
				jRadioTxtMsg,
				(jint)event->p.radioTextData.len,
				(jint)event->p.radioTextData.startIndex,
				(jint)event->p.radioTextData.repertoire);
		break;

	case FM_RX_EVENT_RAW_RDS:

		FM_LOGD("%s: EVENT FM_RX_EVENT_RAW_RDS", __func__);

		len = sizeof(event->p.rawRdsGroupData.groupData);

		jGroupData = env->NewByteArray(len);

		if (jGroupData == NULL)	{
			LOGE("%s: Failed converting elements", __func__);
			goto CLEANUP;
		}

		env->SetByteArrayRegion(jGroupData,
				0,
				len,
				(jbyte*)event->p.rawRdsGroupData.groupData);

		if (env->ExceptionOccurred()) {
			LOGE("%s: Calling jniCb_fmRxRadioText failed",
			     __func__);
			goto CLEANUP;
		}

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxRawRDS,
				(jint)event->status,
				(jint)event->p.rawRdsGroupData.groupBitInMask,
				jGroupData);
		break;

	case FM_RX_EVENT_AUDIO_PATH_CHANGED:

		FM_LOGD("Audio Path Changed Event received");

		break;

	case FM_RX_EVENT_PTY_CODE_CHANGED:

		FM_LOGD("%s: EVENT FM_RX_EVENT_PTY_CODE_CHANGED", __func__);
		FM_LOGD("RDS PTY Code has changed to %d",
		     event->p.ptyChangedData.pty);

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxPtyCodeChanged,
				(jint)event->status,
				(jint)event->p.ptyChangedData.pty);
		break;

	case FM_RX_EVENT_COMPLETE_SCAN_DONE:

		FM_LOGD("%s: EVENT FM_RX_EVENT_COMPLETE_SCAN_DONE", __func__);

		len = sizeof(event->p.completeScanData.channelsData) /
			sizeof(int);

		FM_LOGD("len %d",len);

		jChannelsData = env->NewIntArray(len);
		if (jChannelsData == NULL) {
			LOGE("%s: Failed converting elements", __func__);
			goto CLEANUP;
		}

		env->SetIntArrayRegion(jChannelsData,
				0,
				len,
				(jint*)event->p.completeScanData.channelsData);

		if (env->ExceptionOccurred()) {
			LOGE("%s: Calling jniCb_fmRxRadioText failed",
			     __func__);
			goto CLEANUP;
		}

		env->CallStaticVoidMethod(_sJClass,
				_sMethodId_nativeCb_fmRxCompleteScanDone,
				(jint)event->status,
				(jint)event->p.completeScanData.numOfChannels,
				jChannelsData);
		break;

	default:

		FM_LOGD("%s: unhandled fm event %d", __func__, event->eventType);

		break;
	} //end switch

	if (env->ExceptionOccurred()) {
		LOGE("fmapp_rx_callback:  ExceptionOccurred");
		goto CLEANUP;
	}

//Delete the local references
	if(jAfListData!= NULL)
		env->DeleteLocalRef(jAfListData);
	if(jRadioTxtMsg!= NULL)
		env->DeleteLocalRef(jRadioTxtMsg);
	if(jNameString!= NULL)
		env->DeleteLocalRef(jNameString);
	if(jGroupData!= NULL)
		env->DeleteLocalRef(jGroupData);

	if(jChannelsData!= NULL)
		env->DeleteLocalRef(jChannelsData);

	FM_LOGD("%s: Exiting, Calling DetachCurrentThread at the END", __func__);

	g_jVM->DetachCurrentThread();

	FM_LOGD("fmapp_rx_callback: Exiting");

	return;

CLEANUP:
	LOGE("fmapp_rx_callback: Exiting due to failure");
	if(jAfListData!= NULL)
		env->DeleteLocalRef(jAfListData);
	if(jRadioTxtMsg!= NULL)
		env->DeleteLocalRef(jRadioTxtMsg);
	if(jNameString!= NULL)
		env->DeleteLocalRef(jNameString);
	if(jGroupData!= NULL)
		env->DeleteLocalRef(jGroupData);
	if(jChannelsData!= NULL)
		env->DeleteLocalRef(jChannelsData);
	if (env->ExceptionOccurred()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
	g_jVM->DetachCurrentThread();
}

}
/**********************************************************************
*				Callback registration

***********************************************************************/
#define VERIFY_METHOD_ID(methodId) \
	if (!_VerifyMethodId(methodId, #methodId)) { \
		LOGE("Error obtaining method id for %s", #methodId);	\
		return; 	\
	}

 static bool _VerifyMethodId(jmethodID methodId, const char *name)
{
	bool result = true;

	if (methodId == NULL) {
		LOGE("_VerifyMethodId: Failed getting method id of %s", name);
		result = false;
	}

	return result;
}



void nativeJFmRx_ClassInitNative(JNIEnv* env, jclass clazz)
{

	FM_LOGD("%s: Entered", __func__);

	_jFmEnvTest = env;


	if (NULL == _jFmEnvTest) {

		FM_LOGD("%s: NULL == _jFmEnvTest", __func__);

	}


	/*
	 * Save class information in global reference in order to prevent
	 * class unloading
	 */
	_sJClass = (jclass)env->NewGlobalRef(clazz);

	/* Initialize structure of RBTL callbacks */

	FM_LOGD("%s: Initializing FMRX callback structure", __func__);
	FM_LOGD("%s: Obtaining method IDs", __func__);

	_sMethodId_nativeCb_fmRxRawRDS = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxRawRDS",
				"(II[B)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxRawRDS);


	_sMethodId_nativeCb_fmRxRadioText = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxRadioText",
				"(IZ[BIII)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxRadioText);

	_sMethodId_nativeCb_fmRxPiCodeChanged  = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxPiCodeChanged",
				"(II)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxPiCodeChanged);

	_sMethodId_nativeCb_fmRxPtyCodeChanged = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxPtyCodeChanged",
				"(II)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxPtyCodeChanged);

	_sMethodId_nativeCb_fmRxPsChanged  = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxPsChanged",
				"(II[BI)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxPsChanged);

	_sMethodId_nativeCb_fmRxMonoStereoModeChanged =
		env->GetStaticMethodID(clazz,
				"nativeCb_fmRxMonoStereoModeChanged",
				"(II)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxMonoStereoModeChanged);

	_sMethodId_nativeCb_fmRxAudioPathChanged  = env->GetStaticMethodID(
				clazz,
				"nativeCb_fmRxAudioPathChanged",
				"(I)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxAudioPathChanged);

	_sMethodId_nativeCb_fmRxAfSwitchFreqFailed = env->GetStaticMethodID(
				clazz,
				"nativeCb_fmRxAfSwitchFreqFailed",
				"(IIII)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxAfSwitchFreqFailed);


	_sMethodId_nativeCb_fmRxAfSwitchStart  = env->GetStaticMethodID(clazz,
								"nativeCb_fmRxAfSwitchStart",
								"(IIII)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxAfSwitchStart);

	_sMethodId_nativeCb_fmRxAfSwitchComplete = env->GetStaticMethodID(
				clazz,
				"nativeCb_fmRxAfSwitchComplete",
				"(IIII)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxAfSwitchComplete);

	_sMethodId_nativeCb_fmRxAfListChanged = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxAfListChanged",
				"(II[BI)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxAfListChanged);

	_sMethodId_nativeCb_fmRxCmdDone = env->GetStaticMethodID(clazz,
				"nativeCb_fmRxCmdDone",
				"(IIJ)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxCmdDone);

	_sMethodId_nativeCb_fmRxCompleteScanDone = env->GetStaticMethodID(
				clazz,
				"nativeCb_fmRxCompleteScanDone",
				"(II[I)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxCompleteScanDone);


	_sMethodId_nativeCb_fmRxCmdError = env->GetStaticMethodID(clazz,
								"nativeCb_fmRxCmdError",
								"(I)V");
	VERIFY_METHOD_ID(_sMethodId_nativeCb_fmRxCmdError);

    }
    
    static JNINativeMethod JFmRxNative_sMethods[] = {
    /* name, signature, funcPtr */
    {"nativeJFmRx_ClassInitNative", "()V", (void*)nativeJFmRx_ClassInitNative},
    {"nativeJFmRx_Create", "(Lcom/ti/jfm/core/JFmContext;)I", (void*)nativeJFmRx_Create},
    {"nativeJFmRx_Destroy", "(J)I", (void*)nativeJFmRx_Destroy},
    {"nativeJFmRx_Enable", "(J)I", (void*)nativeJFmRx_Enable},
    {"nativeJFmRx_Disable", "(J)I", (void*)nativeJFmRx_Disable},
    {"nativeJFmRx_SetBand","(JI)I", (void*)nativeJFmRx_SetBand},
    {"nativeJFmRx_GetBand","(J)I", (void*)nativeJFmRx_GetBand},
    {"nativeJFmRx_Tune","(JI)I", (void*)nativeJFmRx_Tune},
    {"nativeJFmRx_GetTunedFrequency","(J)I", (void*)nativeJFmRx_GetTunedFrequency},
    {"nativeJFmRx_SetMonoStereoMode","(JI)I", (void*)nativeJFmRx_SetMonoStereoMode},
    {"nativeJFmRx_GetMonoStereoMode","(J)I", (void*)nativeJFmRx_GetMonoStereoMode},
    {"nativeJFmRx_SetMuteMode","(JI)I", (void*)nativeJFmRx_SetMuteMode},
    {"nativeJFmRx_GetMuteMode","(J)I", (void*)nativeJFmRx_GetMuteMode},
    {"nativeJFmRx_SetRssiThreshold","(JI)I", (void*)nativeJFmRx_SetRssiThreshold},
    {"nativeJFmRx_GetRssiThreshold","(J)I", (void*)nativeJFmRx_GetRssiThreshold},
    {"nativeJFmRx_GetRssi","(J)I", (void*)nativeJFmRx_GetRssi},
    {"nativeJFmRx_SetVolume","(JI)I", (void*)nativeJFmRx_SetVolume},
    {"nativeJFmRx_GetVolume","(J)I", (void*)nativeJFmRx_GetVolume},
    {"nativeJFmRx_SetChannelSpacing","(JI)I", (void*)nativeJFmRx_SetChannelSpacing},
    {"nativeJFmRx_GetChannelSpacing","(J)I", (void*)nativeJFmRx_GetChannelSpacing},
    {"nativeJFmRx_SetDeEmphasisFilter","(JI)I", (void*)nativeJFmRx_SetDeEmphasisFilter},
    {"nativeJFmRx_GetDeEmphasisFilter","(J)I", (void*)nativeJFmRx_GetDeEmphasisFilter},
    {"nativeJFmRx_Seek","(JI)I", (void*)nativeJFmRx_Seek},
    {"nativeJFmRx_StopSeek","(J)I", (void*)nativeJFmRx_StopSeek},
    {"nativeJFmRx_EnableRDS","(J)I", (void*)nativeJFmRx_EnableRDS},
    {"nativeJFmRx_DisableRDS","(J)I", (void*)nativeJFmRx_DisableRDS},
    {"nativeJFmRx_EnableAudioRouting","(J)I", (void*)nativeJFmRx_EnableAudioRouting},
    {"nativeJFmRx_DisableAudioRouting","(J)I", (void*)nativeJFmRx_DisableAudioRouting},
    {"nativeJFmRx_SetRdsAfSwitchMode","(JI)I", (void*)nativeJFmRx_SetRdsAfSwitchMode},
    {"nativeJFmRx_GetRdsAfSwitchMode","(J)I", (void*)nativeJFmRx_GetRdsAfSwitchMode},
    {"nativeJFmRx_ChangeAudioTarget","(JII)I",(void*)nativeJFmRx_ChangeAudioTarget},
    {"nativeJFmRx_ChangeDigitalTargetConfiguration","(JI)I",(void*)nativeJFmRx_ChangeDigitalTargetConfiguration},
    {"nativeJFmRx_SetRfDependentMuteMode","(JI)I",(void*)nativeJFmRx_SetRfDependentMuteMode},
    {"nativeJFmRx_GetRfDependentMute","(J)I",(void*)nativeJFmRx_GetRfDependentMute},
    {"nativeJFmRx_SetRdsSystem","(JI)I",(void*)nativeJFmRx_SetRdsSystem},
    {"nativeJFmRx_GetRdsSystem","(J)I",(void*)nativeJFmRx_GetRdsSystem},
    {"nativeJFmRx_SetRdsGroupMask","(JJ)I",(void*)nativeJFmRx_SetRdsGroupMask},
    {"nativeJFmRx_GetRdsGroupMask","(J)I",(void*)nativeJFmRx_GetRdsGroupMask},
    {"nativeJFmRx_CompleteScan","(J)I",(void*)nativeJFmRx_CompleteScan},
    {"nativeJFmRx_IsValidChannel","(J)I",(void*)nativeJFmRx_IsValidChannel},
    {"nativeJFmRx_GetFwVersion","(J)I",(void*)nativeJFmRx_GetFwVersion},
    {"nativeJFmRx_GetCompleteScanProgress","(J)I",(void*)nativeJFmRx_GetCompleteScanProgress},
    {"nativeJFmRx_StopCompleteScan","(J)I",(void*)nativeJFmRx_StopCompleteScan}
    };

/*
 * Register several native methods for one class.
 */
static int registerNatives(JNIEnv* env, const char* className,
			   JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("Can not find class %s\n", className);
		return JNI_FALSE;
	}

	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGE("Can not RegisterNatives\n");
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	LOGE("OnLoad");

	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		goto bail;
	}

	if (!registerNatives(env,
			     "com/ti/jfm/core/JFmRx",
	                     JFmRxNative_sMethods,
			     NELEM(JFmRxNative_sMethods))) {
		goto bail;
	}

	env->GetJavaVM(&g_jVM);

	/* success -- return valid version number */
	result = JNI_VERSION_1_4;

bail:
	return result;
}

