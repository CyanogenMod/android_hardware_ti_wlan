/*
 * TI's FM
 *
 * Copyright 2001-2010 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
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
 *   FILE NAME:      JFmRx.java
 *
 *   BRIEF:          This file defines the API of the FM Rx stack.
 *
 *   DESCRIPTION:    General
 *
 *
 *
 *   AUTHOR:
 *
 \*******************************************************************************/
package com.ti.jfm.core;

import android.util.Log;
import java.util.HashMap;
import com.ti.jfm.core.*;

/** Class for providing connection with JFmRxNative.cpp module */

public class JFmRx {

	private static final String TAG = "JFmRx";
	private static final boolean DBG = true;
	private ICallback callback = null;
	private static JFmContext context;
	/** Hash table to store JFmContext handles based upon JFmRx. */
	private static HashMap<JFmContext, JFmRx> mRxContextsTable = new HashMap<JFmContext, JFmRx>();

	/** Events path */
	static {
		try {
		    Log.i(TAG, "Loading libfmrx.so");
		    System.loadLibrary("fmrx");
		    JFmRx.nativeJFmRx_ClassInitNative();
		}
		catch (UnsatisfiedLinkError ule) {   
		    Log.e(TAG, "WARNING: Could not load libfmrx.so");    
		} 
		catch (Exception e) {
			Log.e("JFmRx", "Exception during NativeJFmRx_ClassInitNative ("
					+ e.toString() + ")");
		}
	}

	public interface ICallback {

		void fmRxRawRDS(JFmRxStatus status,
				JFmRxRdsGroupTypeMask bitInMaskValue, byte[] groupData);

		void fmRxRadioText(JFmRxStatus status, boolean resetDisplay,
				byte[] msg1, int len, int startIndex, JFmRxRepertoire repertoire);

		void fmRxPiCodeChanged(JFmRxStatus status, JFmRxRdsPiCode pi);

		void fmRxPtyCodeChanged(JFmRxStatus status, JFmRxRdsPtyCode pty);

		void fmRxPsChanged(JFmRxStatus status, JFmRxFreq frequency,
				byte[] name, JFmRxRepertoire repertoire);

		void fmRxMonoStereoModeChanged(JFmRxStatus status,
				JFmRxMonoStereoMode mode);

		void fmRxAfSwitchFreqFailed(JFmRxStatus status, JFmRxRdsPiCode pi,
				JFmRxTuneFreq tunedFreq, JFmRxAfFreq afFreq);

		void fmRxAfSwitchStart(JFmRxStatus status, JFmRxRdsPiCode pi,
				JFmRxTuneFreq tunedFreq, JFmRxAfFreq afFreq);

		void fmRxAfSwitchComplete(JFmRxStatus status, JFmRxRdsPiCode pi,
				JFmRxTuneFreq tunedFreq, JFmRxAfFreq afFreq);

		void fmRxAfListChanged(JFmRxStatus status, JFmRxRdsPiCode pi,
				byte[] afList, JFmRxAfListSize afListSize);

		void fmRxCmdDone(JFmRxStatus status, int command, long value);

		void fmRxCmdError(JFmRxStatus status);

		void fmRxCompleteScanDone(JFmRxStatus status, int numOfChannels,
				int[] channelsData);

	}

	/**
	 * 
	 * Datatype Classes
	 * 
	 */

	public static class JFmRxRdsPiCode {
		private int value = 0;

		public JFmRxRdsPiCode(int val) {
			this.value = val;
		}

		public int getValue() {
			return value;
		}
	}

	public static class JFmRxRdsPtyCode {
		private int value = 0;

		public JFmRxRdsPtyCode(int value) {
			this.value = value;
		}

		public int getValue() {
			return value;
		}
	}

	public static class JFmRxAfFreq {

		private int value = 0;

		public JFmRxAfFreq(int value) {
			this.value = value;
		}

		public int getAfFreq() {
			return value;
		}
	}

	public static class JFmRxAfListSize {
		private int value = 0;

		public JFmRxAfListSize(int value) {
			this.value = value;
		}

		public int getValue() {
			return value;
		}

	}

	// ***************************************************************
	public static enum JFmRxEcalResource implements IJFmEnum<Integer> {

		CAL_RESOURCE_I2SH(0x00), CAL_RESOURCE_PCMH(0x01), CAL_RESOURCE_PCMT_1(
				0x02), CAL_RESOURCE_PCMT_2(0x03), CAL_RESOURCE_PCMT_3(0x04), CAL_RESOURCE_PCMT_4(
				0x05), CAL_RESOURCE_PCMT_5(0x06), CAL_RESOURCE_PCMT_6(0x07), CAL_RESOURCE_FM_ANALOG(
				0x08), CAL_RESOURCE_LAST_EL_RESOURCE(0x08), CAL_RESOURCE_PCMIF(
				0x09), CAL_RESOURCE_FMIF(0x0A), CAL_RESOURCE_CORTEX(0x0B), CAL_RESOURCE_FM_CORE(
				0x0C), CAL_RESOURCE_MAX_NUM(0x0D), CAL_RESOURCE_INVALID(0x0E);

		private final int ecalResource;

		private JFmRxEcalResource(int ecalResource) {
			this.ecalResource = ecalResource;
		}

		public Integer getValue() {
			return ecalResource;
		}
	}

	public static enum JFmRxEcalOperation implements IJFmEnum<Integer> {

		CAL_OPERATION_FM_TX(0x00), CAL_OPERATION_FM_RX(0x01), CAL_OPERATION_A3DP(
				0x02), CAL_OPERATION_BT_VOICE(0x03), CAL_OPERATION_WBS(0x04), CAL_OPERATION_AWBS(
				0x05), CAL_OPERATION_FM_RX_OVER_SCO(0x06), CAL_OPERATION_FM_RX_OVER_A3DP(
				0x07), CAL_OPERATION_MAX_NUM(0x08), CAL_OPERATION_INVALID(0x09);

		private final int ecalOperation;

		private JFmRxEcalOperation(int ecalOperation) {
			this.ecalOperation = ecalOperation;
		}

		public Integer getValue() {
			return ecalOperation;
		}
	}

	public static enum JFmRxEcalSampleFrequency implements IJFmEnum<Integer> {
		CAL_SAMPLE_FREQ_8000(0x00), CAL_SAMPLE_FREQ_11025(0x01), CAL_SAMPLE_FREQ_12000(
				0x02), CAL_SAMPLE_FREQ_16000(0x03), CAL_SAMPLE_FREQ_22050(0x04), CAL_SAMPLE_FREQ_24000(
				0x05), CAL_SAMPLE_FREQ_32000(0x06), CAL_SAMPLE_FREQ_44100(0x07), CAL_SAMPLE_FREQ_48000(
				0x08);

		private final int ecalSampleFreq;

		private JFmRxEcalSampleFrequency(int ecalSampleFreq) {
			this.ecalSampleFreq = ecalSampleFreq;
		}

		public Integer getValue() {
			return ecalSampleFreq;
		}
	}

	public static enum JFmRxRdsGroupTypeMask implements IJFmEnum<Long> {

		FM_RDS_GROUP_TYPE_MASK_0A(0x00000001), FM_RDS_GROUP_TYPE_MASK_0B(
				0x00000002), FM_RDS_GROUP_TYPE_MASK_1A(0x00000004), FM_RDS_GROUP_TYPE_MASK_1B(
				0x00000008), FM_RDS_GROUP_TYPE_MASK_2A(0x00000010), FM_RDS_GROUP_TYPE_MASK_2B(
				0x00000020), FM_RDS_GROUP_TYPE_MASK_3A(0x00000040), FM_RDS_GROUP_TYPE_MASK_3B(
				0x00000080), FM_RDS_GROUP_TYPE_MASK_4A(0x00000100), FM_RDS_GROUP_TYPE_MASK_4B(
				0x00000200), FM_RDS_GROUP_TYPE_MASK_5A(0x00000400), FM_RDS_GROUP_TYPE_MASK_5B(
				0x00000800), FM_RDS_GROUP_TYPE_MASK_6A(0x00001000), FM_RDS_GROUP_TYPE_MASK_6B(
				0x00002000), FM_RDS_GROUP_TYPE_MASK_7A(0x00004000), FM_RDS_GROUP_TYPE_MASK_7B(
				0x00008000), FM_RDS_GROUP_TYPE_MASK_8A(0x00010000), FM_RDS_GROUP_TYPE_MASK_8B(
				0x00020000), FM_RDS_GROUP_TYPE_MASK_9A(0x00040000), FM_RDS_GROUP_TYPE_MASK_9B(
				0x00080000), FM_RDS_GROUP_TYPE_MASK_10A(0x00100000), FM_RDS_GROUP_TYPE_MASK_10B(
				0x00200000), FM_RDS_GROUP_TYPE_MASK_11A(0x00400000), FM_RDS_GROUP_TYPE_MASK_11B(
				0x00800000), FM_RDS_GROUP_TYPE_MASK_12A(0x01000000), FM_RDS_GROUP_TYPE_MASK_12B(
				0x02000000), FM_RDS_GROUP_TYPE_MASK_13A(0x04000000), FM_RDS_GROUP_TYPE_MASK_13B(
				0x08000000), FM_RDS_GROUP_TYPE_MASK_14A(0x10000000), FM_RDS_GROUP_TYPE_MASK_14B(
				0x20000000), FM_RDS_GROUP_TYPE_MASK_15A(0x40000000), FM_RDS_GROUP_TYPE_MASK_15B(
				0x80000000),

		FM_RDS_GROUP_TYPE_MASK_NONE(0x00000000), FM_RDS_GROUP_TYPE_MASK_ALL(
				0xFFFFFFFF);

		private final long rdsGroupTypeMask;

		private JFmRxRdsGroupTypeMask(long rdsGroupTypeMask) {
			this.rdsGroupTypeMask = rdsGroupTypeMask;
		}

		public Long getValue() {
			return rdsGroupTypeMask;
		}
	}

	public static enum JFmRxRdsSystem implements IJFmEnum<Integer> {
		FM_RDS_SYSTEM_RDS(0x00), FM_RDS_SYSTEM_RBDS(0x01);

		private final int rdsSystem;

		private JFmRxRdsSystem(int rdsSystem) {
			this.rdsSystem = rdsSystem;
		}

		public Integer getValue() {
			return rdsSystem;
		}
	}

	public static enum JFmRxAudioRouteMask implements IJFmEnum<Integer> {

		FMC_AUDIO_ROUTE_MASK_I2S(0x00000001), FMC_AUDIO_ROUTE_MASK_ANALOG(
				0x00000002), FMC_AUDIO_ROUTE_MASK_NONE(0x00000000), FMC_AUDIO_ROUTE_MASK_ALL(
				0x00000003);

		private final int audioRouteMask;

		private JFmRxAudioRouteMask(int audioRouteMask) {
			this.audioRouteMask = audioRouteMask;
		}

		public Integer getValue() {
			return audioRouteMask;
		}
	}

	public static class JFmRxTuneFreq {

		private int tuneFreq;

		public JFmRxTuneFreq(int tuneFreq) {
			this.tuneFreq = tuneFreq;
		}

		public int getTuneFreq() {
			return tuneFreq;
		}

		public void setTuneFreq(int tuneFreq) {
			this.tuneFreq = tuneFreq;
		}
	}

	public static class JFmRxRssi {

		private int jFmRssi;

		public JFmRxRssi(int jFmRssi) {
			this.jFmRssi = jFmRssi;
		}

		public int getRssi() {
			return jFmRssi;
		}

		public void setRssi(int jFmRssi) {
			this.jFmRssi = jFmRssi;
		}
	}

	public static class JFmRxVolume {

		private int jFmVolume;

		public JFmRxVolume(int jFmVolume) {
			this.jFmVolume = jFmVolume;
		}

		public int getVolume() {
			return jFmVolume;
		}

		public void setVolume(int jFmVolume) {
			this.jFmVolume = jFmVolume;
		}
	}

	public static class JFmRxFreq {

		private int value = 0;

		public JFmRxFreq(int value) {
			this.value = value;
		}

		public int getValue() {
			return value;
		}

		public void setValue(int value) {
			this.value = value;
		}
	}

	public static enum JFmRxMuteMode implements IJFmEnum<Integer> {
		FMC_MUTE(0x00), FMC_NOT_MUTE(0x01), FMC_ATTENUATE(0x02);

		private final int muteMode;

		private JFmRxMuteMode(int muteMode) {
			this.muteMode = muteMode;
		}

		public Integer getValue() {
			return muteMode;
		}
	}

	public static enum JFmRxChannelSpacing implements IJFmEnum<Integer> {
		FMC_CHANNEL_SPACING_50_KHZ(0x01), FMC_CHANNEL_SPACING_100_KHZ(0x02), FMC_CHANNEL_SPACING_200_KHZ(
				0x04);

		private final int channelSpace;

		private JFmRxChannelSpacing(int channelSpace) {
			this.channelSpace = channelSpace;
		}

		public Integer getValue() {
			return channelSpace;
		}
	}

	public static enum JFmRxBand implements IJFmEnum<Integer> {
		FMC_BAND_EUROPE_US(0x00), FMC_BAND_JAPAN(0x01);

		private final int band;

		private JFmRxBand(int band) {
			this.band = band;
		}

		public Integer getValue() {
			return band;
		}
	}

	public static enum JFmRxAudioTargetMask implements IJFmEnum<Integer> {
		FM_RX_TARGET_MASK_INVALID(0), FM_RX_TARGET_MASK_I2SH(1), FM_RX_TARGET_MASK_PCMH(
				2), FM_RX_TARGET_MASK_FM_OVER_SCO(4), FM_RX_TARGET_MASK_FM_OVER_A3DP(
				8), FM_RX_TARGET_MASK_FM_ANALOG(16);

		private final int audioTargetMask;

		private JFmRxAudioTargetMask(int audioTargetMask) {
			this.audioTargetMask = audioTargetMask;
		}

		public Integer getValue() {
			return audioTargetMask;
		}
	}

	public static enum JFmRxRfDependentMuteMode implements IJFmEnum<Integer> {
		FM_RX_RF_DEPENDENT_MUTE_ON(0x01), FM_RX_RF_DEPENDENT_MUTE_OFF(0x00);

		private final int rfDependentMuteMode;

		private JFmRxRfDependentMuteMode(int rfDependentMuteMode) {
			this.rfDependentMuteMode = rfDependentMuteMode;
		}

		public Integer getValue() {
			return rfDependentMuteMode;
		}
	}

	public static enum JFmRxRdsAfSwitchMode implements IJFmEnum<Integer> {
		FM_RX_RDS_AF_SWITCH_MODE_ON(0x01), FM_RX_RDS_AF_SWITCH_MODE_OFF(0x00);

		private final int rdsAfSwitchMode;

		private JFmRxRdsAfSwitchMode(int rdsAfSwitchMode) {
			this.rdsAfSwitchMode = rdsAfSwitchMode;
		}

		public Integer getValue() {
			return rdsAfSwitchMode;
		}
	}

	public static enum JFmRxAudioPath implements IJFmEnum<Integer> {
		FM_RX_AUDIO_PATH_OFF(0x00), FM_RX_AUDIO_PATH_HEADSET(0x01), FM_RX_AUDIO_PATH_HANDSET(
				0x02);

		private final int audioPath;

		private JFmRxAudioPath(int audioPath) {
			this.audioPath = audioPath;
		}

		public Integer getValue() {
			return audioPath;
		}
	}

	public static enum JFmRxMonoStereoMode implements IJFmEnum<Integer> {
		FM_RX_MONO(0x01), FM_RX_STEREO(0x00);

		private final int monoStereoModer;

		private JFmRxMonoStereoMode(int monoStereoModer) {
			this.monoStereoModer = monoStereoModer;
		}

		public Integer getValue() {
			return monoStereoModer;
		}
	}

	public static enum JFmRxEmphasisFilter implements IJFmEnum<Integer> {
		FM_RX_EMPHASIS_FILTER_NONE(0x00), FM_RX_EMPHASIS_FILTER_50_USEC(0x01), FM_RX_EMPHASIS_FILTER_75_USEC(
				0x02);

		private final int emphasisFilter;

		private JFmRxEmphasisFilter(int emphasisFilter) {
			this.emphasisFilter = emphasisFilter;
		}

		public Integer getValue() {
			return emphasisFilter;
		}
	}

	public static enum JFmRxRepertoire implements IJFmEnum<Integer> {
		FMC_RDS_REPERTOIRE_G0_CODE_TABLE(0x00), FMC_RDS_REPERTOIRE_G1_CODE_TABLE(
				0x01), FMC_RDS_REPERTOIRE_G2_CODE_TABLE(0x02);

		private final int repertoire;

		private JFmRxRepertoire(int repertoire) {
			this.repertoire = repertoire;
		}

		public Integer getValue() {
			return repertoire;
		}
	}

	public static enum JFmRxSeekDirection implements IJFmEnum<Integer> {
		FM_RX_SEEK_DIRECTION_DOWN(0x00), FM_RX_SEEK_DIRECTION_UP(0x01);

		private final int direction;

		private JFmRxSeekDirection(int direction) {
			this.direction = direction;
		}

		public Integer getValue() {
			return direction;
		}
	}

	/*******************************************************************************
	 * 
	 * Class Methods
	 * 
	 *******************************************************************************/

	public JFmRxStatus create(ICallback callback) {
		JFmRxStatus jFmRxStatus;

		try {

			JFmContext c = new JFmContext();
			if (DBG)
				Log.d(TAG, "Calling nativeJFmRx_Create");
			int fmStatus = nativeJFmRx_Create(c);
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Create, status = "
						+ jFmRxStatus.toString() + ", Context = "
						+ c.getValue());

			if (jFmRxStatus != JFmRxStatus.SUCCESS) {
				return jFmRxStatus;
			}

			/**
			 *Record the caller's callback and returned native context for
			 * nativeJFmRx_create
			 */

			this.context = c;
			this.callback = callback;
			mRxContextsTable.put(context, this);

		} catch (Exception e) {
			Log.e(TAG, "create: exception during nativeJFmRx_create ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus destroy() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_Destroy(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Destroy, status = "
						+ jFmRxStatus.toString());

			if (jFmRxStatus != JFmRxStatus.SUCCESS) {
				return jFmRxStatus;
			}

			/*
			 * Remove a pair of JFmContext-JFmRx related to the destroyed
			 * context from the HashMap
			 */

			mRxContextsTable.remove(context);

		} catch (Exception e) {
			Log.e(TAG, "destroy: exception during nativeJFmRx_Destroy ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		if (DBG)
			Log.d(TAG, "destroy: exiting");

		return jFmRxStatus;

	}

	public JFmRxStatus enable() {

		JFmRxStatus jFmRxStatus;
		if (-1 == context.getValue()) {
			Log.e(TAG, "enable: context.getvalue is -1 ; value = "
					+ context.getValue());
			jFmRxStatus = JFmRxStatus.CONTEXT_DOESNT_EXIST;
			return jFmRxStatus;
		}

		try {
			int fmStatus = nativeJFmRx_Enable(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Enable, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "enable: exception during nativeJFmRx_enable ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus disable() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_Disable(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Disable, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "disable: exception during nativeJFmRx_Disable ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus setBand(JFmRxBand jFmRxBand) {

		JFmRxStatus jFmRxStatus;
		try {

			int status = nativeJFmRx_SetBand(context.getValue(), jFmRxBand
					.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetBand, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "setBand: exception during nativeJFmRx_SetBand ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus getBand() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetBand(context.getValue());

			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetBand, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "getBand: exception during nativeJFmRx_GetBand ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus tune(JFmRxFreq jfmRxFreq) {

		JFmRxStatus jFmRxStatus;

		try {

			int status = nativeJFmRx_Tune(context.getValue(), jfmRxFreq
					.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Tune, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "tune: exception during nativeJFmRx_Tune ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus getTunedFrequency() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetTunedFrequency(context.getValue());

			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetTunedFrequency, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getTunedFrequency: exception during nativeJFmRx_GetTunedFrequency ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setMonoStereoMode(JFmRxMonoStereoMode jFmRxMonoStereoMode) {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_SetMonoStereoMode(context.getValue(),
					jFmRxMonoStereoMode.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetMonoStereoMode, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"setMonoStereoMode: exception during nativeJFmRx_SetMonoStereoMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus isValidChannel() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_IsValidChannel(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_IsValidChannel, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"isValidChannel: exception during nativeJFmRx_IsValidChannel ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus completeScan() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_CompleteScan(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_CompleteScan, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"completeScan: exception during nativeJFmRx_CompleteScan ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus getFwVersion() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetFwVersion(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetFwVersion, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getFwVersion: exception during nativeJFmRx_GetFwVersion ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus getCompleteScanProgress() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetCompleteScanProgress(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG,
						"After nativeJFmRx_GetCompleteScanProgress, status = "
								+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log
					.e(
							TAG,
							"getCompleteScanProgress: exception during nativeJFmRx_GetCompleteScanProgress ("
									+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus stopCompleteScan() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_StopCompleteScan(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_StopCompleteScan, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"stopCompleteScan: exception during nativeJFmRx_StopCompleteScan ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus getMonoStereoMode() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetMonoStereoMode(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetMonoStereoMode, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"getMonoStereoMode: exception during nativeJFmRx_GetMonoStereoMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus setMuteMode(JFmRxMuteMode jFmRxMuteMode) {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_SetMuteMode(context.getValue(),
					jFmRxMuteMode.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetMuteMode, status = "
						+ jFmRxStatus.toString());

		} catch (Exception e) {
			Log.e(TAG,
					"setMuteMode: exception during nativeJFmRx_SetMuteMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getMuteMode() {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_GetMuteMode(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetMuteMode, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getMuteMode: exception during nativeJFmRx_GetMuteMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus setRssiThreshold(JFmRxRssi jFmRssi) {

		JFmRxStatus jFmRxStatus;

		try {
			int status = nativeJFmRx_SetRssiThreshold(context.getValue(),
					jFmRssi.getRssi());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, status);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetRssiThreshold, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"setRssiThreshold: exception during nativeJFmRx_SetRssiThreshold ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;

	}

	public JFmRxStatus getRssiThreshold() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRssiThreshold(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRssiThreshold, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getRssiThreshold: exception during nativeJFmRx_GetRssiThreshold ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus getRssi() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRssi(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRssi, status = "
						+ jFmRxStatus.toString());

		} catch (Exception e) {
			Log.e(TAG, "getRssi: exception during nativeJFmRx_GetRssi ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus setVolume(JFmRxVolume jFmVolume) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetVolume(context.getValue(), jFmVolume
					.getVolume());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetVolume, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "setVolume: exception during nativeJFmRx_SetVolume ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getVolume() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetVolume(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetVolume, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "getVolume: exception during nativeJFmRx_GetVolume ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setChannelSpacing(JFmRxChannelSpacing jFmRxChannelSpacing) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetChannelSpacing(context.getValue(),
					jFmRxChannelSpacing.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetChannelSpacing, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"setChannelSpacing: exception during nativeJFmRx_SetChannelSpacing ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getChannelSpacing() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetChannelSpacing(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetChannelSpacing, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getChannelSpacing: exception during nativeJFmRx_GetChannelSpacing ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus SetDeEmphasisFilter(
			JFmRxEmphasisFilter jFmRxEmphasisFilter) {

		JFmRxStatus jFmRxStatus;
		try {
			int fmStatus = nativeJFmRx_SetDeEmphasisFilter(context.getValue(),
					jFmRxEmphasisFilter.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetDeEmphasisFilter, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"SetDeEmphasisFilter: exception during nativeJFmRx_SetDeEmphasisFilter ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus GetDeEmphasisFilter() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetDeEmphasisFilter(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetDeEmphasisFilter, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"GetDeEmphasisFilter: exception during nativeJFmRx_GetDeEmphasisFilter ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus seek(JFmRxSeekDirection jFmRxSeekDirection) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_Seek(context.getValue(),
					jFmRxSeekDirection.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_Seek, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG, "seek: exception during nativeJFmRx_Seek ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		return jFmRxStatus;
	}

	public JFmRxStatus stopSeek() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_StopSeek(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_StopSeek, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG, "stopSeek: exception during nativeJFmRx_StopSeek ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus enableRDS() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_EnableRDS(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_EnableRDS, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG, "enableRDS: exception during nativeJFmRx_EnableRDS ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus DisableRDS() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_DisableRDS(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_DisableRDS, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG, "DisableRDS: exception during nativeJFmRx_DisableRDS ("
					+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus enableAudioRouting() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_EnableAudioRouting(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_EnableAudioRouting, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"enableAudioRouting: exception during nativeJFmRx_EnableAudioRouting ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus disableAudioRouting() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_DisableAudioRouting(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_DisableAudioRouting, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"disableAudioRouting: exception during nativeJFmRx_DisableAudioRouting ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setRdsAfSwitchMode(JFmRxRdsAfSwitchMode jRdsAfSwitchMode) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetRdsAfSwitchMode(context.getValue(),
					jRdsAfSwitchMode.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetRdsAfSwitchMode, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"setRdsAfSwitchMode: exception during nativeJFmRx_SetRdsAfSwitchMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getRdsAfSwitchMode() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRdsAfSwitchMode(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRdsAfSwitchMode, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"getRdsAfSwitchMode: exception during nativeJFmRx_GetRdsAfSwitchMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;
	}

	public JFmRxStatus changeAudioTarget(
			JFmRxAudioTargetMask jFmRxAudioTargetMask,
			JFmRxEcalSampleFrequency digitalConfig) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_ChangeAudioTarget(context.getValue(),
					jFmRxAudioTargetMask.getValue(), digitalConfig.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_ChangeAudioTarget, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"changeAudioTarget: exception during nativeJFmRx_ChangeAudioTarget ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus changeDigitalTargetConfiguration(
			JFmRxEcalSampleFrequency digitalConfig) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_ChangeDigitalTargetConfiguration(context
					.getValue(), digitalConfig.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG,
						"After nativeJFmRx_ChangeDigitalTargetConfiguration, status = "
								+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log
					.e(
							TAG,
							"changeDigitalTargetConfiguration: exception during nativeJFmRx_ChangeDigitalTargetConfiguration ("
									+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setRfDependentMuteMode(JFmRxRfDependentMuteMode mode) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetRfDependentMuteMode(context
					.getValue(), mode.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG,
						"After nativeJFmRx_SetRfDependentMuteMode, status = "
								+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"setRfDependentMuteMode: exception during nativeJFmRx_SetRfDependentMuteMode ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getRfDependentMute() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRfDependentMute(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRfDependentMute, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"getRfDependentMute: exception during nativeJFmRx_GetRfDependentMute ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setRdsSystem(JFmRxRdsSystem rdsSystem) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetRdsSystem(context.getValue(),
					rdsSystem.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetRdsSystem, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"setRdsSystem: exception during nativeJFmRx_SetRdsSystem ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getRdsSystem() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRdsSystem(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRdsSystem, status = "
						+ jFmRxStatus.toString());
		} catch (Exception e) {
			Log.e(TAG,
					"getRdsSystem: exception during nativeJFmRx_GetRdsSystem ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus setRdsGroupMask(JFmRxRdsGroupTypeMask groupMask) {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_SetRdsGroupMask(context.getValue(),
					groupMask.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_SetRdsGroupMask, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"setRdsGroupMask: exception during nativeJFmRx_SetRdsGroupMask ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}

		return jFmRxStatus;

	}

	public JFmRxStatus getRdsGroupMask() {

		JFmRxStatus jFmRxStatus;

		try {
			int fmStatus = nativeJFmRx_GetRdsGroupMask(context.getValue());
			jFmRxStatus = JFmUtils.getEnumConst(JFmRxStatus.class, fmStatus);
			if (DBG)
				Log.d(TAG, "After nativeJFmRx_GetRdsGroupMask, status = "
						+ jFmRxStatus.toString());
			
		} catch (Exception e) {
			Log.e(TAG,
					"getRdsGroupMask: exception during nativeJFmRx_GetRdsGroupMask ("
							+ e.toString() + ")");
			jFmRxStatus = JFmRxStatus.FAILED;
		}
		if (DBG)
			Log.d(TAG, "getRdsGroupMask: exiting");

		return jFmRxStatus;

	}

	/*--------------------------------------------------------------------------
	 *			NATIVE PART 
	 *------------------------------------------------------------------------*/

	/* RBTL Calls */
	private static native void nativeJFmRx_ClassInitNative();

	private static native int nativeJFmRx_Create(JFmContext contextValue);

	private static native int nativeJFmRx_Destroy(long contextValue);

	private static native int nativeJFmRx_Enable(long contextValue);

	private static native int nativeJFmRx_Disable(long contextValue);

	private static native int nativeJFmRx_SetBand(long contextValue, int jFmBand);

	private static native int nativeJFmRx_GetBand(long contextValue);

	private static native int nativeJFmRx_Tune(long contextValue, int jFmFreq);

	private static native int nativeJFmRx_GetTunedFrequency(long contextValue);

	private static native int nativeJFmRx_SetMonoStereoMode(long contextValue,
			int jFmMonoStereoMode);

	private static native int nativeJFmRx_GetMonoStereoMode(long contextValue);

	private static native int nativeJFmRx_SetMuteMode(long contextValue,
			int jFmMuteMode);

	private static native int nativeJFmRx_GetMuteMode(long contextValue);

	private static native int nativeJFmRx_SetRssiThreshold(long contextValue,
			int jFmRssi);

	private static native int nativeJFmRx_GetRssiThreshold(long contextValue);

	private static native int nativeJFmRx_GetRssi(long contextValue);

	private static native int nativeJFmRx_SetVolume(long contextValue,
			int jFmVolume);

	private static native int nativeJFmRx_GetVolume(long contextValue);

	private static native int nativeJFmRx_SetChannelSpacing(long contextValue,
			int jFmChannelSpacing);

	private static native int nativeJFmRx_GetChannelSpacing(long contextValue);

	private static native int nativeJFmRx_SetDeEmphasisFilter(
			long contextValue, int jFmEmphasisFilter);

	private static native int nativeJFmRx_GetDeEmphasisFilter(long contextValue);

	private static native int nativeJFmRx_Seek(long contextValue,
			int jFmDirection);

	private static native int nativeJFmRx_StopSeek(long contextValue);

	private static native int nativeJFmRx_EnableRDS(long contextValue);

	private static native int nativeJFmRx_DisableRDS(long contextValue);

	private static native int nativeJFmRx_EnableAudioRouting(long contextValue);

	private static native int nativeJFmRx_DisableAudioRouting(long contextValue);

	private static native int nativeJFmRx_SetRdsAfSwitchMode(long contextValue,
			int jRdsAfSwitchMode);

	private static native int nativeJFmRx_GetRdsAfSwitchMode(long contextValue);

	private static native int nativeJFmRx_ChangeAudioTarget(long contextValue,
			int audioTargetMask, int digitalConfig);

	private static native int nativeJFmRx_ChangeDigitalTargetConfiguration(
			long contextValue, int digitalConfig);

	private static native int nativeJFmRx_SetRfDependentMuteMode(
			long contextValue, int mode);

	private static native int nativeJFmRx_GetRfDependentMute(long contextValue);

	private static native int nativeJFmRx_SetRdsSystem(long contextValue,
			int rdsSystem);

	private static native int nativeJFmRx_GetRdsSystem(long contextValue);

	private static native int nativeJFmRx_SetRdsGroupMask(long contextValue,
			long groupMask);

	private static native int nativeJFmRx_GetRdsGroupMask(long contextValue);

	private static native int nativeJFmRx_IsValidChannel(long contextValue);

	private static native int nativeJFmRx_CompleteScan(long contextValue);

	private static native int nativeJFmRx_GetFwVersion(long contextValue);

	private static native int nativeJFmRx_GetCompleteScanProgress(
			long contextValue);

	private static native int nativeJFmRx_StopCompleteScan(long contextValue);

	/*
	 * -------------------------------- NATIVE PART
	 * --------------------------------
	 */

	/*
	 * ---------------------------------------------- Callbacks from the
	 * JFmRxNative.cpp module ----------------------------------------------
	 */

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxRawRDS(int status, int bitInMaskValue,
			byte[] groupData) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxRawRDS: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsGroupTypeMask bitInMask = JFmUtils.getEnumConst(
					JFmRxRdsGroupTypeMask.class, (long) bitInMaskValue);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxRawRDS: calling callback");

			callback.fmRxRawRDS(rxStatus, bitInMask, groupData);

		}
	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxRadioText(int status, boolean resetDisplay,
			byte[] msg1, int len, int startIndex, int repertoire) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxRadioText: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRepertoire jRepertoire = JFmUtils.getEnumConst(
					JFmRxRepertoire.class, repertoire);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxRadioText: calling callback");

			callback.fmRxRadioText(rxStatus, resetDisplay, msg1, len,
					startIndex, jRepertoire);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxPiCodeChanged(int status, int piValue) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
						"nativeCb_fmRxPiCodeChanged: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsPiCode pi = new JFmRxRdsPiCode(piValue);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxPiCodeChanged: calling callback");

			callback.fmRxPiCodeChanged(rxStatus, pi);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxPtyCodeChanged(int status, int ptyValue) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
					"nativeCb_fmRxPtyCodeChanged: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsPtyCode pty = new JFmRxRdsPtyCode(ptyValue);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxPtyCodeChanged: calling callback");

			callback.fmRxPtyCodeChanged(rxStatus, pty);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxPsChanged(int status, int frequency,
			byte[] name, int repertoire) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxPsChanged: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRepertoire jRepertoire = JFmUtils.getEnumConst(
					JFmRxRepertoire.class, repertoire);

			JFmRxFreq jFreq = new JFmRxFreq(frequency);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxPsChanged: calling callback");

			callback.fmRxPsChanged(rxStatus, jFreq, name, jRepertoire);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxMonoStereoModeChanged(int status,
			int modeValue) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
						"nativeCb_fmRxMonoStereoModeChanged: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxMonoStereoMode monoStereoMode = JFmUtils.getEnumConst(
					JFmRxMonoStereoMode.class, modeValue);

			if (DBG)
				Log.d(TAG,
						"nativeCb_fmRxMonoStereoModeChanged: calling callback");

			callback.fmRxMonoStereoModeChanged(rxStatus, monoStereoMode);

		}
	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxAudioPathChanged(int status) {
	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxAfSwitchFreqFailed(int status, int piValue,
			int tunedFreq, int afFreqValue) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
					"nativeCb_fmRxAfSwitchFreqFailed: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsPiCode pi = new JFmRxRdsPiCode(piValue);

			JFmRxTuneFreq tuneFreq = new JFmRxTuneFreq(tunedFreq);

			JFmRxAfFreq afFreq = new JFmRxAfFreq(afFreqValue);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxAfSwitchFreqFailed: calling callback");

			callback.fmRxAfSwitchFreqFailed(rxStatus, pi, tuneFreq, afFreq);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxAfSwitchStart(int status, int piValue,
			int tunedFreq, int afFreq) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;
			if (DBG)
				Log.d(TAG,
						"nativeCb_fmRxAfSwitchStart: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);
			JFmRxRdsPiCode pi = new JFmRxRdsPiCode(piValue);

			JFmRxTuneFreq tuneFreq = new JFmRxTuneFreq(tunedFreq);

			JFmRxAfFreq aFreq = new JFmRxAfFreq(afFreq);
			if (DBG)
				Log.d(TAG, "nativeCb_fmRxAfSwitchStart: calling callback");

			callback.fmRxAfSwitchStart(rxStatus, pi, tuneFreq, aFreq);

		}
	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxAfListChanged(int status, int piValue,
			byte[] afList, int afListSize) {

		JFmRx mJFmRx = getJFmRx(context.getValue());
		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
						"nativeCb_fmRxAfListChanged: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsPiCode pi = new JFmRxRdsPiCode(piValue);

			JFmRxAfListSize jafListSize = new JFmRxAfListSize(afListSize);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxAfListChanged: calling callback");

			callback.fmRxAfListChanged(rxStatus, pi, afList, jafListSize);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxAfSwitchComplete(int status, int piValue,
			int tunedFreq, int afFreq) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
					"nativeCb_fmRxAfSwitchComplete: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			JFmRxRdsPiCode pi = new JFmRxRdsPiCode(piValue);

			JFmRxTuneFreq tuneFreq = new JFmRxTuneFreq(tunedFreq);

			JFmRxAfFreq aFreq = new JFmRxAfFreq(afFreq);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxAfSwitchComplete: calling callback");

			callback.fmRxAfSwitchComplete(rxStatus, pi, tuneFreq, aFreq);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxCmdDone(int status, int cmd, long value) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxCmdDone: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxCmdDone: calling callback");

			callback.fmRxCmdDone(rxStatus, cmd, value);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxCmdError(int status) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			if (DBG)
				Log.d(TAG, "Context is not null here: calling callback");
			ICallback callback = mJFmRx.callback;
			if (DBG)
				Log.d(TAG, "nativeCb_fmRxCmdError: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);
			if (DBG)
				Log.d(TAG, "nativeCb_fmRxCmdError: calling callback");

			callback.fmRxCmdError(rxStatus);

		}

	}

	@SuppressWarnings("unused")
	public static void nativeCb_fmRxCompleteScanDone(int status,
			int numOfChannels, int[] channelsData) {

		JFmRx mJFmRx = getJFmRx(context.getValue());

		if (mJFmRx != null) {

			ICallback callback = mJFmRx.callback;

			if (DBG)
				Log.d(TAG,
					"nativeCb_fmRxCompleteScanDone: converting callback args");

			JFmRxStatus rxStatus = JFmUtils.getEnumConst(JFmRxStatus.class,
					status);

			if (DBG)
				Log.d(TAG, "nativeCb_fmRxCompleteScanDone: calling callback");

			callback
					.fmRxCompleteScanDone(rxStatus, numOfChannels, channelsData);

		}

	}

	/******************************************************************************************
	 * Private utility functions
	 * 
	 *******************************************************************************************/

	private static JFmRx getJFmRx(long contextValue) {
		JFmRx jFmRx;

		JFmContext profileContext = new JFmContext(contextValue);

		if (mRxContextsTable.containsKey(profileContext)) {
			jFmRx = mRxContextsTable.get(profileContext);
		} else {
			Log.e(TAG, "JFmRx: Failed mapping context " + contextValue
					+ " to a callback");
			jFmRx = null;
		}

		return jFmRx;
	}

} /* End class */
