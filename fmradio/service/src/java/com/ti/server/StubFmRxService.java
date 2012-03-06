/*
 * TI's FM
 *
 * Copyright 2001-2010 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010, 2011 Sony Ericsson Mobile Communications AB.
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
 *   FILE NAME:      StubFmRxService.java
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
package com.ti.server;

import com.ti.fm.FmReceiver;
import com.ti.fm.IFmReceiver;
import com.ti.fm.FmReceiverIntent;
import com.ti.fm.IFmConstants;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.RemoteException;
import android.os.Handler;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.os.Bundle;

import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue; /*  (additional packages required) */
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import com.ti.jfm.core.*;

/*************************************************************************************************
 * Provides FM Receiver functionality as a service in the Application Framework.
 * There is a single instance of this class that is created during system
 * startup as part of the system server. The class exposes its services via
 * IFmReceiver.aidl.
 * 
 * @hide
 *************************************************************************************************/

public class StubFmRxService extends IFmReceiver.Stub implements
		JFmRx.ICallback, IFmConstants {

	private static final String TAG = "FMRXService";
	private static final boolean DBG = true;
	private static final String FMRX_ADMIN_PERM = "ti.permission.FMRX_ADMIN";
	private static final String FMRX_PERM = "ti.permission.FMRX";

	public static final String FM_RX_SERVICE = "StubFmRxService";

//	private static final String FM_RADIO_ACTIVE_KEY = "fm_radio_active";
//	private static final String FM_RADIO_VALUE_ON = "on";
//	private static final String FM_RADIO_VALUE_OFF = "off";

	private static final String FM_ENABLED = "fm_enabled";
	private static final String FM_RESTORE_VALUES = "com.ti.server.fmrestorecmd";

	private static final int BLOCKING_TIMEOUT_IN_SEC = 4;

	private AudioManager mAudioManager = null;

	/*
	 * variable to make sure that the next volume change happens after the
	 * current volume request has been completed.
	 */
	private boolean mVolState = VOL_REQ_STATE_IDLE;

	TelephonyManager tmgr;

	private boolean mResumeAfterCall = false; // Varibale to check whether to
	// resume FM after call.

	private Object mVolumeSynchronizationLock = new Object();
	private boolean isFmCreated = false;

	// to make the get API
	// Synchronous

	private Handler mDelayedDisableHandler;
	private DelayedDisable mDelayedDisable;
	private DelayedPauseDisable mDelayedPauseDisable;
	private static final int FM_DISABLE_DELAY = 50;
	private WakeLock mWakeLock = null;
	/*  ***********Constants *********************** */

	private JFmRx mJFmRx;
	private int mState = FmReceiver.STATE_DEFAULT; // State of the FM Service

	private Context mContext = null;
	/* Variable to store the current Band */
	private static int mCurrentBand = FM_BAND_EUROPE_US;

	/* Variable to store the current tuned frequency */
	private static int mCurrentFrequency = FM_FIRST_FREQ_US_EUROPE_KHZ;

	/* Variable to store the current mute mode */
	private static int mCurrentMuteMode = FM_NOT_MUTE;

	/*
	 * Variable to store the Mode , so that when the value is changed ,intimate
	 * to the application
	 */
	private JFmRx.JFmRxMonoStereoMode mMode = JFmRx.JFmRxMonoStereoMode.FM_RX_STEREO;
	/*
	 * Variable to store the PiCode , so that when the value is changed
	 * ,intimate to the application
	 */
	static int last_msg_printed = 255;

	private IntentFilter mIntentFilter;

	/* Variables to store the return value of the get APIs */
	private static int getBandValue = FM_BAND_EUROPE_US;
	private static int getMonoStereoModeValue = 0;
	private static int getMuteModeValue = FM_NOT_MUTE;
	private static int getRfDependentMuteModeValue = FM_RF_DEP_MUTE_OFF;
	private static int getRssiThresholdValue = FM_RSSI_THRESHHOLD;
	private static int getDeEmphasisFilterValue = 0;
	private static int getVolumeValue = FM_MAX_VOLUME / 2;
	private static int getChannelSpaceValue = FM_CHANNEL_SPACE;
	private static int getTunedFrequencyValue = FM_UNDEFINED_FREQ;
	private static int getRssiValue = 0;
	private static int getRdsSystemValue = 0;
	private static long getRdsGroupMaskValue = FM_UNDEFINED_FREQ;
	private static int getRdsAfSwitchModeValue = 0;
	private static double getFwVersion = 0.0;
	private static int getScanProgress = 0;
	private static boolean mIsValidChannel = false;
	private static boolean mIsCompleteScanInProgress = false;
	private static boolean mIsSeekInProgress = false;
	private static boolean mIsTuneInProgress = false;
	private static int mStopCompleteScanStatus = 0;

	/*************************************************************************************************
	 * // Constructor
	 *************************************************************************************************/
	public StubFmRxService() {
		super();
	}

	/*************************************************************************************************
	 * Must be called after construction, and before any other method.
	 *************************************************************************************************/
	public synchronized void init(Context context) {
		JFmRxStatus status;
		try {
			// create a single new JFmRx instance
			mJFmRx = new JFmRx();
		} catch (Exception e) {
			Log.e(TAG, "init: Exception thrown during init (" + e.toString()
					+ ")");
			return;
		}

		// Save the context to be used later
		mContext = context;
		mState = FmReceiver.STATE_DEFAULT;

		// Delayed Disable to avoid clicks
		mDelayedDisable = new DelayedDisable();
		mDelayedPauseDisable = new DelayedPauseDisable();
		mDelayedDisableHandler = new Handler();

		PowerManager powerManager = (PowerManager) mContext
				.getSystemService(Context.POWER_SERVICE);
		if (powerManager != null) {
			mWakeLock = powerManager.newWakeLock(
					PowerManager.PARTIAL_WAKE_LOCK, TAG);
		} else {
			Log.e(TAG, "Failed to get Power Manager.");
			mWakeLock = null;
		}

		if (mWakeLock != null) {
			mWakeLock.setReferenceCounted(false);
		} else {
			Log.e(TAG, "Failed to create WakeLock.");
		}

		mAudioManager = (AudioManager) mContext
				.getSystemService(Context.AUDIO_SERVICE);
	}

	/*************************************************************************************************
	 * Must be called when the service is no longer needed.But is the service
	 * really going down?
	 *************************************************************************************************/
	public synchronized void close() {
		JFmRxStatus status;
		try {
			mJFmRx = null;
		} catch (Exception e) {
			Log.e(TAG, "init: Exception thrown during close (" + e.toString()
					+ ")");
			return;
		}

	}

	/*************************************************************************************************
	 * Must be called after construction, and before any other method.
	 *************************************************************************************************/

	public boolean isEnabled() {
		mContext.enforceCallingOrSelfPermission(FMRX_PERM,
				"Need FMRX_PERM permission");
		return (mState == FmReceiver.STATE_ENABLED);

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public int getFMState() {
		mContext.enforceCallingOrSelfPermission(FMRX_PERM,
				"Need FMRX_PERM permission");

		return mState;
	}

	/*************************************************************************************************
	 * Implementation of conversion from Unsigned to nSigned Integer
	 *************************************************************************************************/
	public int convertUnsignedToSignedInt(long a) {

		int nReturnVal;

		if ((a > 65535) || (a < 0)) {
			if (DBG)
				Log
						.d(
								TAG,
								"  convertUnsignedToSignedInt: Error in conversion from Unsigned to nSigned Integer");
			nReturnVal = 0;
			return nReturnVal;
		}

		if (a > 32767)
			nReturnVal = (int) (a - 65536);
		else
			nReturnVal = (int) a;

		return nReturnVal;
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean create() {

		// Always call create
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		try {

			/*
			 * resister for the master Volume control Intent and music/video
			 * playback
			 */
			mIntentFilter = new IntentFilter();
			mIntentFilter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
			mIntentFilter.addAction(FM_RESTORE_VALUES);
			mContext.registerReceiver(mFmRxIntentReceiver, mIntentFilter);

			JFmRxStatus status = mJFmRx.create(this);
			if (DBG)
				Log
						.i(TAG, "mJFmRx.create returned status "
								+ status.toString());

			if (JFmRxStatus.SUCCESS != status) {
				Log
						.e(TAG, "mJFmRx.create returned status "
								+ status.toString());
				return false;
			}
		} catch (Exception e) {
			Log.e(TAG, "create: Exception thrown during create ("
					+ e.toString() + ")");
			return false;
		}

		isFmCreated = true;
		return true;
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	public synchronized boolean enable() {

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		/* make sure we turn off any running timers */
		mDelayedDisableHandler.removeCallbacks(mDelayedDisable);
		mDelayedDisableHandler.removeCallbacks(mDelayedPauseDisable);
		if ((mWakeLock != null) && (mWakeLock.isHeld())) {
			mWakeLock.release();
		}
		try {
			JFmRxStatus status = mJFmRx.enable();
			if (DBG)
				Log
						.i(TAG, "mJFmRx.enable returned status "
								+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log
						.e(TAG, "mJFmRx.enable returned status "
								+ status.toString());
				return false;
			}
		} catch (Exception e) {
			Log.e(TAG, "enable: Exception thrown during enable ("
					+ e.toString() + ")");
			return false;
		}
		mState = FmReceiver.STATE_ENABLING;

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean destroy() {

		isFmCreated = false;
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");

		try {
			mContext.unregisterReceiver(mFmRxIntentReceiver);

			JFmRxStatus status = mJFmRx.destroy();
			if (DBG)
				Log.d(TAG, "mJFmRx.destroy returned status "
						+ status.toString());
			if (JFmRxStatus.SUCCESS != status) {
				Log
						.e(TAG, "mJFmRx.enable returned status "
								+ status.toString());
				return false;
			}

		} catch (Exception e) {
			Log.e(TAG, "destroy: Exception thrown during destroy ("
					+ e.toString() + ")");
			return false;
		}

		mState = FmReceiver.STATE_DEFAULT;

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	public synchronized boolean disable() {
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "disable error: fm not enabled " + mState);
			return false;
		}

//		mAudioManager.setParameters(FM_RADIO_ACTIVE_KEY + "=off");

		if (mWakeLock != null) {
			mWakeLock.acquire();
		}
		mDelayedDisableHandler.postDelayed(mDelayedDisable, FM_DISABLE_DELAY);

		return true;

	}

	/*************************************************************************************************
	 * Implementation DelayedDisable runnable class
	 *************************************************************************************************/
	private class DelayedDisable implements Runnable {

		public final void run() {
			if ((mWakeLock != null) && (mWakeLock.isHeld())) {
				mWakeLock.release();
			}

			/* check that state is still valid */
			if (mState == FmReceiver.STATE_ENABLED) {
				boolean success = true;
				try {
					JFmRxStatus status = mJFmRx.disable();
					if (DBG)
						Log.d(TAG, "mJFmRx.disable returned status "
								+ status.toString());
					if (JFmRxStatus.PENDING != status) {
						success = false;
						Log.e(TAG, "mJFmRx.disable returned status "
								+ status.toString());
					}
				} catch (Exception e) {
					success = false;
					Log.e(TAG, "disable: Exception thrown during disable ("
							+ e.toString() + ")");
				}
				if (success) {
					mState = FmReceiver.STATE_DISABLING;
				}
			} else {
				Log.e(TAG, "disable error: fm not enabled " + mState);
			}
		}
	}

	/*************************************************************************************************
	 * Implementation DelayedPauseDisable runnable class
	 *************************************************************************************************/
	private class DelayedPauseDisable implements Runnable {

		public final void run() {
			if ((mWakeLock != null) && (mWakeLock.isHeld())) {
				mWakeLock.release();
			}

			/* check that state is still valid */
			if (mState == FmReceiver.STATE_ENABLED) {
				boolean success = true;
				try {
					JFmRxStatus status = mJFmRx.disable();
					if (DBG)
						Log.d(TAG, "mJFmRx.disable returned status "
								+ status.toString());
					
					if (JFmRxStatus.PENDING != status) {
						success = false;
						Log.e(TAG, "mJFmRx.disable returned status "
								+ status.toString());
					}
				} catch (Exception e) {
					success = false;
					Log.e(TAG, "disable: Exception thrown during disable ("
							+ e.toString() + ")");
				}
				if (success) {
					mState = FmReceiver.STATE_PAUSE;
				}
			} else {
				Log.e(TAG, "disable error: fm not enabled " + mState);
			}
		}
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetBandSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setBand(int band) {
		mCurrentBand = band;
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setBand: failed, fm not enabled state " + mState);
			return false;
		}

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxBand lBand = JFmUtils.getEnumConst(
					JFmRx.JFmRxBand.class, band);
			if (lBand == null) {
				Log.e(TAG, "StubFmRxService:setBand invalid  lBand " + lBand);
				return false;
			}

			mSetBandSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setBand(lBand);
			if (DBG)
				Log.d(TAG, "mJFmRx.setBand returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.setBand returned status "
						+ status.toString());
				return false;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */

			try {
				String syncString = mSetBandSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setBand-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:setBand():--------- Exiting... ");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mBandSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getBand() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getBand: failed, fm not enabled  state " + mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mBandSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getBand();
			if (DBG)
				Log.d(TAG, "mJFmRx.getBand returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getBand returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */

			try {
				String syncString = mBandSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getBand-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getBand():--------- Exiting... ");
		return getBandValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean setMonoStereoMode(int mode) {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setMonoStereoMode: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxMonoStereoMode lMode = JFmUtils.getEnumConst(
					JFmRx.JFmRxMonoStereoMode.class, mode);
			if (lMode == null) {
				Log.e(TAG, "StubFmRxService:setMonoStereoMode invalid  lBand "
						+ lMode);
				return false;
			}
			JFmRxStatus status = mJFmRx.setMonoStereoMode(lMode);
			if (DBG)
				Log.d(TAG, "mJFmRx.setMonoStereoMode returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.setMonoStereoMode returned status "
						+ status.toString());
				return false;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}

		if (DBG)
			Log.d(TAG, "StubFmRxService:setMonoStereoMode exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mMonoStereoModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getMonoStereoMode() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getMonoStereoMode: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mMonoStereoModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getMonoStereoMode();
			if (DBG)
				Log.d(TAG, "mJFmRx.getMonoStereoMode returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getMonoStereoMode returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				String syncString = mMonoStereoModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getMonoStereoMode-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getMonoStereoMode(): -------- Exiting ");

		return getMonoStereoModeValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mSetMuteModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setMuteMode(int muteMode) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setMuteMode: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxMuteMode lMode = JFmUtils.getEnumConst(
					JFmRx.JFmRxMuteMode.class, muteMode);
			if (lMode == null) {
				Log.e(TAG, "StubFmRxService:setMuteMode invalid  lBand "
						+ lMode);
				return false;
			}
			mSetMuteModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setMuteMode(lMode);
			if (DBG)
				Log.d(TAG, "mJFmRx.SetMuteMode returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.SetMuteMode returned status "
						+ status.toString());
				return false;
			}

			/*
			 * (When muting turn off audio paths to lower power consumption and
			 * reduce noise)
			 */
			if (lMode == JFmRx.JFmRxMuteMode.FMC_MUTE) {

//				mAudioManager.setParameters(FM_RADIO_ACTIVE_KEY + "=off");
			} else {

//				mAudioManager.setParameters(FM_RADIO_ACTIVE_KEY + "=on");
			}
			/*
			 * (When muting turn off audio paths to lower power consumption and
			 * reduce noise)
			 */

			/* OMAPS00207918:implementation to make the set API Synchronous */

			try {
				String syncString = mSetMuteModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setMuteMode-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:SetMuteMode exiting");

		mCurrentMuteMode = muteMode;
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mMuteModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getMuteMode() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getMuteMode: failed, fm not enabled  state " + mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mMuteModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getMuteMode();
			if (DBG)
				Log.d(TAG, "mJFmRx.getMuteMode returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getMuteMode returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mMuteModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getMuteMode-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getMuteMode(): -------- Exiting... ");
		return getMuteModeValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetRfDependentMuteModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setRfDependentMuteMode(int rfMuteMode) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setRfDependentMuteMode: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxRfDependentMuteMode lrfMute = JFmUtils.getEnumConst(
					JFmRx.JFmRxRfDependentMuteMode.class, rfMuteMode);
			if (lrfMute == null) {
				Log.e(TAG,
						"StubFmRxService:setRfDependentMuteMode invalid  lrfMute "
								+ lrfMute);
				return false;
			}

			mSetRfDependentMuteModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setRfDependentMuteMode(lrfMute);
			if (DBG)
				Log.d(TAG, "mJFmRx.setRfDependentMuteMode returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.setRfDependentMuteMode returned status "
						+ status.toString());
				return false;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				Log
						.i(TAG,
								"StubFmRxService:setRfDependentMuteMode(): -------- Waiting... ");
				String syncString = mSetRfDependentMuteModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log
						.e(TAG,
								"mJFmRx.setRfDependentMuteMode-- Wait() s Exception!!!");
				return false;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:setRfDependentMuteMode exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRfDependentMuteModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getRfDependentMuteMode() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRfDependentMuteMode: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRfDependentMuteModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRfDependentMute();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRfDependentMuteMode returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getRfDependentMuteMode returned status "
						+ status.toString());

				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRfDependentMuteModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log
						.e(TAG,
								"mJFmRx.getRfDependentMuteMode-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getRfDependentMuteMode(): --------- Exiting... ");
		return getRfDependentMuteModeValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetRssiThresholdSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setRssiThreshold(int threshhold) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setRssiThreshold: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxRssi lrssiThreshhold = new JFmRx.JFmRxRssi(threshhold);
			if (lrssiThreshhold == null) {
				Log.e(TAG, "StubFmRxService:setRssiThreshold invalid rssi "
						+ lrssiThreshhold);
				return false;
			}
			if (DBG)
				Log.d(TAG, "StubFmRxService:setRssiThreshold  "
						+ lrssiThreshhold);
			mSetRssiThresholdSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setRssiThreshold(lrssiThreshhold);
			if (DBG)
				Log.d(TAG, "mJFmRx.setRssiThreshold returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.setRssiThreshold returned status "
						+ status.toString());
				return false;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				Log
						.i(TAG,
								"StubFmRxService:setRssiThreshold(): -------- Waiting... ");
				String syncString = mSetRssiThresholdSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setRssiThreshold-- Wait() s Exception!!!");
				return false;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:setRssiThreshold exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRssiThresholdSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getRssiThreshold() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRssiThreshold: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRssiThresholdSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRssiThreshold();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRssiThreshold returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getRssiThreshold returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRssiThresholdSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getRssiThreshold-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG,
					"StubFmRxService:getRssiThreshold(): ---------- Exiting ");
		return getRssiThresholdValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mSetDeEmphasisFilterSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setDeEmphasisFilter(int filter) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setDeEmphasisFilter: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxEmphasisFilter lFilter = JFmUtils.getEnumConst(
					JFmRx.JFmRxEmphasisFilter.class, filter);
			if (lFilter == null) {
				Log.e(TAG,
						"StubFmRxService:setDeEmphasisFilter invalid  lBand "
								+ lFilter);
				return false;
			}
			mSetDeEmphasisFilterSyncQueue.clear();
			JFmRxStatus status = mJFmRx.SetDeEmphasisFilter(lFilter);
			if (DBG)
				Log.d(TAG, "mJFmRx.setDeEmphasisFilter returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.setDeEmphasisFilter returned status "
						+ status.toString());
				return false;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				Log
						.i(TAG,
								"StubFmRxService:setDeEmphasisFilter(): -------- Waiting... ");
				String syncString = mSetDeEmphasisFilterSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log
						.e(TAG,
								"mJFmRx.setDeEmphasisFilter-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:setDeEmphasisFilter exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mDeEmphasisFilterSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getDeEmphasisFilter() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getDeEmphasisFilter: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mDeEmphasisFilterSyncQueue.clear();
			JFmRxStatus status = mJFmRx.GetDeEmphasisFilter();
			if (DBG)
				Log.d(TAG, "mJFmRx.getDeEmphasisFilter returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getDeEmphasisFilter returned status "
						+ status.toString());
				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mDeEmphasisFilterSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log
						.e(TAG,
								"mJFmRx.getDeEmphasisFilter-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG,
					"StubFmRxService:getDeEmphasisFilter(): -------- Exiting ");
		return getDeEmphasisFilterValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private boolean setVolume(int volume) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setVolume: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");

			JFmRx.JFmRxVolume lVolume = new JFmRx.JFmRxVolume(volume);
			if (lVolume == null) {
				Log.e(TAG, "StubFmRxService:setVolume invalid  lVolume " + lVolume);
				return false;
			}

			synchronized (mVolumeSynchronizationLock) {

				JFmRxStatus status = mJFmRx.setVolume(lVolume);
				if (DBG)
					Log.d(TAG, "mJFmRx.setVolume returned status "
							+ status.toString());
				if (JFmRxStatus.PENDING != status) {
					Log.e(TAG, "mJFmRx.setVolume returned status "
							+ status.toString());
					return false;
				}

				mVolState = VOL_REQ_STATE_PENDING;
			}

		return true;
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetChannelSpacingSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setChannelSpacing(int channelSpace) {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setChannelSpacing: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxChannelSpacing lChannelSpace = JFmUtils.getEnumConst(
					JFmRx.JFmRxChannelSpacing.class, channelSpace);

			if (lChannelSpace == null) {
				Log.e(TAG,
						"StubFmRxService:setChannelSpacing invalid  lChannelSpace "
								+ lChannelSpace);
				return false;
			}
			mSetChannelSpacingSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setChannelSpacing(lChannelSpace);
			if (DBG)
				Log.d(TAG, "mJFmRx.setChannelSpacing returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.setChannelSpacing returned status "
						+ status.toString());
				return false;
			}

			try {
				Log
						.i(TAG,
								"StubFmRxService:setChannelSpacing(): -------- Waiting... ");
				String syncString = mSetChannelSpacingSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setChannelSpacing-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:setChannelSpacing exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mVolumeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	private synchronized int getVolume() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getVolume: failed, fm not enabled  state " + mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mVolumeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getVolume();
			if (DBG)
				Log.d(TAG, "mJFmRx.getVolume returned status "
						+ status.toString());

			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getVolume returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mVolumeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getVolume-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getVolume(): -------- Exiting... ");
		return getVolumeValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mChannelSpacingSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getChannelSpacing() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getChannelSpacing: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mChannelSpacingSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getChannelSpacing();
			if (DBG)
				Log.d(TAG, "mJFmRx.getChannelSpacing returned status "
						+ status.toString());

			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getChannelSpacing returned status "
						+ status.toString());
				return 0;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mChannelSpacingSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getChannelSpacing-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getChannelSpacing() --Exiting ");
		return getChannelSpaceValue;

	}

	static int BaseFreq() {
		return mCurrentBand == FM_BAND_JAPAN ? FM_FIRST_FREQ_JAPAN_KHZ
				: FM_FIRST_FREQ_US_EUROPE_KHZ;
	}

	static int LastFreq() {
		return mCurrentBand == FM_BAND_JAPAN ? FM_LAST_FREQ_JAPAN_KHZ
				: FM_LAST_FREQ_US_EUROPE_KHZ;
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean tune(int freq) {

		mCurrentFrequency = freq;
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "tune: failed, fm not enabled  state " + mState);
			return false;
		}
		if (freq < BaseFreq() || freq > LastFreq()) {
			Log.e(TAG, "StubFmRxService:tune invalid frequency not in range "
					+ freq);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		JFmRx.JFmRxFreq lFreq = new JFmRx.JFmRxFreq(freq);
		if (lFreq == null) {
			Log.e(TAG, "StubFmRxService:tune invalid frequency " + lFreq);
			return false;
		}
		if(DBG)
			Log.d(TAG, "StubFmRxService:tune  " + lFreq);
		JFmRxStatus status = mJFmRx.tune(lFreq);
		if (DBG)
			Log.d(TAG, "mJFmRx.tune returned status " + status.toString());
		if (status != JFmRxStatus.PENDING) {
			Log.e(TAG, "mJFmRx.tune returned status " + status.toString());
			return false;
		}
		mIsTuneInProgress = true;
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mTunedFrequencySyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getTunedFrequency() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getTunedFrequency: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mTunedFrequencySyncQueue.clear();
			JFmRxStatus status = mJFmRx.getTunedFrequency();
			if (DBG)
				Log.d(TAG, "mJFmRx.getTunedFrequency returned status "
						+ status.toString());

			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getTunedFrequency returned status "
						+ status.toString());
				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mTunedFrequencySyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getTunedFrequency-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getTunedFrequency(): ------- Exiting ");
		return getTunedFrequencyValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean seek(int direction) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "seek: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");

		if (mIsSeekInProgress == false) {
			JFmRx.JFmRxSeekDirection lDir = JFmUtils.getEnumConst(
					JFmRx.JFmRxSeekDirection.class, direction);
			if (lDir == null) {
				Log.e(TAG, "StubFmRxService:seek invalid  lDir " + lDir);
				return false;
			}
			JFmRxStatus status = mJFmRx.seek(lDir);
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.seek returned status " + status.toString());
				return false;
			}

			mIsSeekInProgress = true;
			return true;
		} else {
			Log.e(TAG, "seek is in progress.cannot call the API");
			return false;
		}
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mStopSeekSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean stopSeek() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "stopSeek: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mStopSeekSyncQueue.clear();
			JFmRxStatus status = mJFmRx.stopSeek();
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.stopSeek returned status "
						+ status.toString());
				return false;
			}

			try {
				Log.i(TAG, "StubFmRxService:stopSeek(): -------- Waiting... ");
				String syncString = mStopSeekSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.stopSeek-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "tune is in progress.cannot call the API");
			return false;
		}
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRssiSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getRssi() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRssi: failed, fm not enabled  state " + mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRssiSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRssi();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRssi returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getRssi returned status "
						+ status.toString());
				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRssiSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getRssi-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getRssi(): ---------- Exiting ");
		return getRssiValue;
	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRdsSystemSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getRdsSystem() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRdsSystem: failed, fm not enabled  state " + mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRdsSystemSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRdsSystem();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRdsSystem returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getRdsSystem returned status "
						+ status.toString());
				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRdsSystemSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getRdsSystem-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:getRdsSystem(): ----------- Exiting ");
		return getRdsSystemValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetRdsSystemSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setRdsSystem(int system) {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setRdsSystem: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxRdsSystem lSystem = JFmUtils.getEnumConst(
					JFmRx.JFmRxRdsSystem.class, system);
			if (lSystem == null) {
				Log.e(TAG, "StubFmRxService:setRdsSystem invalid  lSystem "
						+ lSystem);
				return false;
			}
			if (DBG)
				Log.d(TAG, "StubFmRxService:setRdsSystem   lSystem " + lSystem);
			mSetRdsSystemSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setRdsSystem(lSystem);
			if (DBG)
				Log.d(TAG, "mJFmRx.setRdsSystem returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.setRdsSystem returned status "
						+ status.toString());
				return false;
			}

			try {
				Log.i(TAG,
						"StubFmRxService:setRdsSystem(): -------- Waiting... ");
				String syncString = mSetRdsSystemSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setRdsSystem-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mEnableRdsSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean enableRds() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "enableRds: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mEnableRdsSyncQueue.clear();
			JFmRxStatus status = mJFmRx.enableRDS();
			if (DBG)
				Log.d(TAG, "mJFmRx.enableRds returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.enableRds returned status "
						+ status.toString());
				return false;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				Log.d(TAG, "StubFmRxService:enableRds(): -------- Waiting... ");
				String syncString = mEnableRdsSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.enableRds-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:enableRds exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mDisableRdsSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean disableRds() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "disableRds: failed, fm not enabled  state " + mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mDisableRdsSyncQueue.clear();
			JFmRxStatus status = mJFmRx.DisableRDS();
			if (DBG)
				Log.d(TAG, "mJFmRx.disableRds returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.disableRds returned status "
						+ status.toString());
				return false;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				Log
						.i(TAG,
								"StubFmRxService:disableRds(): -------- Waiting... ");
				String syncString = mDisableRdsSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.disableRds-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG, "StubFmRxService:disableRds exiting");
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetRdsGroupMaskSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setRdsGroupMask(int mask) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setRdsGroupMask: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxRdsGroupTypeMask lMask = JFmUtils.getEnumConst(
					JFmRx.JFmRxRdsGroupTypeMask.class, (long) mask);
			mSetRdsGroupMaskSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setRdsGroupMask(lMask);
			if (DBG)
				Log.d(TAG, "mJFmRx.setRdsGroupMask returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.setRdsGroupMask returned status "
						+ status.toString());
				return false;
			}

			try {
				Log
						.i(TAG,
								"StubFmRxService:setRdsGroupMask(): -------- Waiting... ");
				String syncString = mSetRdsGroupMaskSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setRdsGroupMask-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRdsGroupMaskSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized long getRdsGroupMask() {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRdsGroupMask: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRdsGroupMaskSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRdsGroupMask();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRdsGroupMask returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getRdsGroupMask returned status "
						+ status.toString());
				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRdsGroupMaskSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getRdsGroupMask-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getRdsGroupMask(): ---------- Exiting ");
		return getRdsGroupMaskValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mSetRdsAfSwitchModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public boolean setRdsAfSwitchMode(int mode) {
		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "setRdsAfSwitchMode: failed, fm not enabled  state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			JFmRx.JFmRxRdsAfSwitchMode lMode = JFmUtils.getEnumConst(
					JFmRx.JFmRxRdsAfSwitchMode.class, mode);
			if (lMode == null) {
				Log.e(TAG, "StubFmRxService:setRdsAfSwitchMode invalid  lMode "
						+ lMode);
				return false;
			}
			mSetRdsAfSwitchModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.setRdsAfSwitchMode(lMode);
			if (DBG)
				Log.d(TAG, "mJFmRx.setRdsAfSwitchMode returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.setRdsAfSwitchMode returned status "
						+ status.toString());

				return false;
			}
			try {
				Log
						.i(TAG,
								"StubFmRxService:setRdsAfSwitchMode(): -------- Waiting... ");
				String syncString = mSetRdsAfSwitchModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.setRdsAfSwitchMode-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	private BlockingQueue<String> mRdsAfSwitchModeSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getRdsAfSwitchMode() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "getRdsAfSwitchMode: failed, fm not enabled  state "
					+ mState);
			return 0;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mRdsAfSwitchModeSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getRdsAfSwitchMode();
			if (DBG)
				Log.d(TAG, "mJFmRx.getRdsAfSwitchMode returned status "
						+ status.toString());
			if (status != JFmRxStatus.PENDING) {
				Log.e(TAG, "mJFmRx.getRdsAfSwitchMode returned status "
						+ status.toString());

				return 0;
			}
			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mRdsAfSwitchModeSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getRdsAfSwitchMode-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getRdsAfSwitchMode(): ---------- Exiting... ");
		return getRdsAfSwitchModeValue;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean changeAudioTarget(int mask, int digitalConfig) {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "changeAudioTarget: failed, already in state " + mState);
			return false;
		}

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		JFmRx.JFmRxEcalSampleFrequency lconfig = JFmUtils.getEnumConst(
				JFmRx.JFmRxEcalSampleFrequency.class, digitalConfig);
		JFmRx.JFmRxAudioTargetMask lMask = JFmUtils.getEnumConst(
				JFmRx.JFmRxAudioTargetMask.class, mask);
		if (lMask == null || lconfig == null) {
			Log.e(TAG,
					"StubFmRxService:changeAudioTarget invalid  lMask , lconfig"
							+ lMask + "" + lconfig);
			return false;
		}

		JFmRxStatus status = mJFmRx.changeAudioTarget(lMask, lconfig);
		if (DBG)
			Log.d(TAG, "mJFmRx.changeAudioTarget returned status "
					+ status.toString());
		if (status != JFmRxStatus.PENDING) {
			Log.e(TAG, "mJFmRx.changeAudioTarget returned status "
					+ status.toString());
			return false;
		}

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean changeDigitalTargetConfiguration(int digitalConfig) {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG,
					"changeDigitalTargetConfiguration: failed, already in state "
							+ mState);
			return false;
		}

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		JFmRx.JFmRxEcalSampleFrequency lconfig = JFmUtils.getEnumConst(
				JFmRx.JFmRxEcalSampleFrequency.class, digitalConfig);
		if (lconfig == null) {
			Log.e(TAG,
					"StubFmRxService:changeDigitalTargetConfiguration invalid   lconfig"
							+ lconfig);
			return false;
		}

		JFmRxStatus status = mJFmRx.changeDigitalTargetConfiguration(lconfig);
		if (DBG)
			Log.d(TAG,
					"mJFmRx.changeDigitalTargetConfiguration returned status "
							+ status.toString());
		if (status != JFmRxStatus.PENDING) {
			Log.e(TAG,
					"mJFmRx.changeDigitalTargetConfiguration returned status "
							+ status.toString());

			return false;
		}

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean enableAudioRouting() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log
					.e(TAG, "enableAudioRouting: failed, already in state "
							+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");

		/*
		 * EnableAudioRouting is not supported in this release of the FM stack,
		 * VAC configurations are disabled
		 */
		// JFmRxStatus status = mJFmRx.enableAudioRouting();
		JFmRxStatus status = JFmRxStatus.NOT_SUPPORTED;

		if (DBG)
			Log.d(TAG, "mJFmRx.enableAudioRouting returned status "
					+ status.toString());
		if (status == JFmRxStatus.NOT_SUPPORTED) {
			Log
					.e(TAG,
							"mJFmRx.enableAudioRouting returned status true for backward compatibility ");

			return true;
		}
		if (status != JFmRxStatus.PENDING) {
			Log.e(TAG, "mJFmRx.enableAudioRouting returned status "
					+ status.toString());

			return false;
		}
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean disableAudioRouting() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "disableAudioRouting: failed, already in state "
					+ mState);
			return false;
		}
		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		/*
		 * DisableAudioRouting is not supported in this release of the FM stack,
		 * VAC configurations are disabled
		 */
		// JFmRxStatus status = mJFmRx.disableAudioRouting();
		JFmRxStatus status = JFmRxStatus.NOT_SUPPORTED;

		if (DBG)
			Log.d(TAG, "mJFmRx.disableAudioRouting returned status "
					+ status.toString());
		if (status == JFmRxStatus.NOT_SUPPORTED) {
			Log
					.e(TAG,
							"mJFmRx.disableAudioRouting returned status true for backward compatibility ");

			return true;
		}		
		if (status != JFmRxStatus.PENDING) {
			Log.e(TAG, "mJFmRx.disableAudioRouting returned status "
					+ status.toString());
			return false;
		}
		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public boolean pauseFm() {
		if (isEnabled() == true) {

			if (mState != FmReceiver.STATE_ENABLED) {
				Log.e(TAG, "pauseFm: failed, already in state " + mState);
				return false;
			}
			mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
					"Need FMRX_ADMIN_PERM permission");

//			mAudioManager.setParameters(FM_RADIO_ACTIVE_KEY + "=off");

			if (mWakeLock != null) {
				mWakeLock.acquire();
			}
			mDelayedDisableHandler.postDelayed(mDelayedPauseDisable,
					FM_DISABLE_DELAY);

			return true;
		} else
			return false;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mStopCompleteScanSyncQueue = new LinkedBlockingQueue<String>(5);

	public boolean completeScan() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "completeScan: failed, fm not enabled state " + mState);
			return false;
		}

		mStopCompleteScanSyncQueue.clear();

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");

		JFmRxStatus status = mJFmRx.completeScan();
		if (DBG)
			Log.d(TAG, "mJFmRx.completeScan returned status "
					+ status.toString());
		if (JFmRxStatus.PENDING != status) {
			Log.e(TAG, "mJFmRx.completeScan returned status "
					+ status.toString());
			return false;
		}

		mIsCompleteScanInProgress = true;

		return true;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/

	public int stopCompleteScan() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log.e(TAG, "stopCompleteScan: failed, fm not enabled state "
					+ mState);
			return 0;
		}

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)) {
			Log.i(TAG, "stubFmRxService:stopCompleteScan started");
			mStopCompleteScanSyncQueue.clear();

			JFmRxStatus status = mJFmRx.stopCompleteScan();
			if (DBG)
				Log.d(TAG, "mJFmRx.stopCompleteScan returned status "
						+ status.toString());
			if (JFmRxStatus.COMPLETE_SCAN_IS_NOT_IN_PROGRESS == status) {
					if(DBG)
						Log.d(TAG,"complete scan already stopped");	
					return status.getValue();
			} else if(JFmRxStatus.PENDING != status){
				Log.e(TAG, "mJFmRx.stopCompleteScan returned status "
						+ status.toString());
					return 0;
			}

			try {
				Log
						.i(TAG,
								"StubFmRxService:stopCompleteScan(): -------- Waiting... ");
				String syncString = mStopCompleteScanSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string received: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.stopCompleteScan-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek/tune is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)

			Log
					.d(TAG,
							"StubFmRxService:stopCompleteScan(): ---------- Exiting... ");
		return mStopCompleteScanStatus;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mValidChannelSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized boolean isValidChannel() {

		if (mState != FmReceiver.STATE_ENABLED) {
			Log
					.e(TAG, "isValidChannel: failed, fm not enabled state "
							+ mState);
			return false;
		}

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mValidChannelSyncQueue.clear();
			JFmRxStatus status = mJFmRx.isValidChannel();
			if (DBG)
				Log.d(TAG, "mJFmRx.isValidChannel returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.isValidChannel returned status "
						+ status.toString());
				return false;
			}

			/* OMAPS00207918:implementation to make the get/set API Synchronous */
			try {
				String syncString = mValidChannelSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return false;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.isValidChannel-- Wait() s Exception!!!");
				return false;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return false;
		}
		if (DBG)
			Log.d(TAG,
					"StubFmRxService:isValidChannel(): ---------- Exiting... ");
		return mIsValidChannel;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mFwVersionSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized double getFwVersion() {

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)
				&& (mIsCompleteScanInProgress == false)) {
			mFwVersionSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getFwVersion();
			if (DBG)
				Log.d(TAG, "mJFmRx.getFwVersion returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getFwVersion returned status "
						+ status.toString());
				return 0;
			}

			try {
				String syncString = mFwVersionSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log.e(TAG, "mJFmRx.getFwVersion-- Wait() s Exception!!!");
				return 0;
			}

		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getFwVersion(): ---------- Exiting... ");
		return getFwVersion;

	}

	/*************************************************************************************************
	 * Implementation of IFmReceiver IPC interface
	 *************************************************************************************************/
	private BlockingQueue<String> mCompleteScanProgressSyncQueue = new LinkedBlockingQueue<String>(
			5);

	public synchronized int getCompleteScanProgress() {

		mContext.enforceCallingOrSelfPermission(FMRX_ADMIN_PERM,
				"Need FMRX_ADMIN_PERM permission");
		if ((mIsSeekInProgress == false) && (mIsTuneInProgress == false)) {
			mCompleteScanProgressSyncQueue.clear();
			JFmRxStatus status = mJFmRx.getCompleteScanProgress();
			if (DBG)
				Log.d(TAG, "mJFmRx.getCompleteScanProgress returned status "
						+ status.toString());
			if (JFmRxStatus.PENDING != status) {
				Log.e(TAG, "mJFmRx.getCompleteScanProgress returned status "
						+ status.toString());
				if (JFmRxStatus.COMPLETE_SCAN_IS_NOT_IN_PROGRESS == status) {
					return status.getValue();
				} else
					return 0;
			}
			try {
				String syncString = mCompleteScanProgressSyncQueue.poll(BLOCKING_TIMEOUT_IN_SEC, TimeUnit.SECONDS);
				if (null == syncString || !syncString.equals("*")) {
					Log.e(TAG, "wrong sync string receieved: " + syncString);
					return 0;
				}
			} catch (InterruptedException e) {
				Log
						.e(TAG,
								"mJFmRx.getCompleteScanProgress-- Wait() s Exception!!!");
				return 0;
			}
		} else {
			Log.e(TAG, "Seek is in progress.cannot call the API");
			return FM_SEEK_IN_PROGRESS;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:getCompleteScanProgress(): ---------- Exiting... ");
		return getScanProgress;

	}

	/*************************************************************************************************
	 * JFmRxlback interface for receiving its events and for broadcasting them
	 * as intents
	 *************************************************************************************************/

	public void fmRxRawRDS(JFmRxStatus status,
			JFmRx.JFmRxRdsGroupTypeMask bitInMaskValue, byte[] groupData)

	{

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxRawRDS status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxRawRDS bitInMaskValue = "
					+ bitInMaskValue.getValue());
		}

	}

	public void fmRxRadioText(JFmRxStatus status, boolean resetDisplay,
			byte[] msg1, int len, int startIndex,
			JFmRx.JFmRxRepertoire repertoire) {

		if (DBG) {
			Log.d(TAG,
					"StubFmRxService:fmRxRadioText status = , msg1 =  ,len =  , startIndex  = "
							+ status.toString() + " " + /* + msg1 + " " */" "
							+ len + " " + startIndex);
			Log
					.d(TAG,
							"StubFmRxService:sending intent RDS_TEXT_CHANGED_ACTION");
		}

		Intent intentRds = new Intent(FmReceiverIntent.RDS_TEXT_CHANGED_ACTION);
		if (FM_SEND_RDS_IN_BYTEARRAY == true) {
			Bundle b = new Bundle();
			b.putByteArray(FmReceiverIntent.RDS, msg1);// Send Byte Array to App
			intentRds.putExtras(b);
		} else {
			/*
			 * OMAPS00207258/OMAPS00207261: Convert the received Byte Array to
			 * appropriate String
			 */
			String radioTextString = findFromLookupTable(msg1, repertoire);
			intentRds.putExtra(FmReceiverIntent.RADIOTEXT_CONVERTED,
					radioTextString);// converted String
		}

		intentRds.putExtra(FmReceiverIntent.STATUS, status.getValue());// Status

		mContext.sendBroadcast(intentRds, FMRX_PERM);
	}

	public void fmRxPiCodeChanged(JFmRxStatus status, JFmRx.JFmRxRdsPiCode pi) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxPiCodeChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxPiCodeChanged pi = "
					+ pi.getValue());
		}

		if (status == JFmRxStatus.SUCCESS) {
			/* simple mechanism to prevent displaying repetitious messages */
			if (pi.getValue() != last_msg_printed) {
				last_msg_printed = pi.getValue();
				Log
						.d(TAG,
								"StubFmRxService:sending intent PI_CODE_CHANGED_ACTION");
				Intent intentPi = new Intent(
						FmReceiverIntent.PI_CODE_CHANGED_ACTION);
				intentPi.putExtra(FmReceiverIntent.PI, pi.getValue());
				intentPi.putExtra(FmReceiverIntent.STATUS, status.getValue());
				mContext.sendBroadcast(intentPi, FMRX_PERM);
			}
		}
	}

	public void fmRxPtyCodeChanged(JFmRxStatus status, JFmRx.JFmRxRdsPtyCode pty) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxPtyCodeChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxPtyCodeChanged pty = "
					+ pty.getValue());
		}

	}

	public void fmRxPsChanged(JFmRxStatus status, JFmRx.JFmRxFreq frequency,
			byte[] name, JFmRx.JFmRxRepertoire repertoire) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxPsChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxPsChanged frequency = "
					+ frequency.getValue());
			Log.d(TAG, "StubFmRxService:fmRxPsChanged name = " /* + name */);
			Log.d(TAG, "StubFmRxService:fmRxPsChanged repertoire = "
					+ repertoire.getValue());
			Log.d(TAG, "StubFmRxService:sending intent PS_CHANGED_ACTION");
		}

		Intent intentPs = new Intent(FmReceiverIntent.PS_CHANGED_ACTION);
		if (FM_SEND_RDS_IN_BYTEARRAY == true) {
			Bundle b = new Bundle();
			b.putByteArray(FmReceiverIntent.PS, name);// Send Byte Array to App
			intentPs.putExtras(b);
		} else {
			/*
			 * OMAPS00207258/OMAPS00207261:Convert the received Byte Array to
			 * appropriate String Broadcast the PS data Byte Array, Converted
			 * string to App
			 */
			String psString = findFromLookupTable(name, repertoire);
			if (DBG)
				Log.d(TAG, "fmRxPsChanged--> psString = " + psString);
			intentPs.putExtra(FmReceiverIntent.PS_CONVERTED, psString);// converted
			// PS
			// String
		}

		intentPs.putExtra(FmReceiverIntent.REPERTOIRE, repertoire.getValue());// repertoire
		intentPs.putExtra(FmReceiverIntent.STATUS, status.getValue());// status

		mContext.sendBroadcast(intentPs, FMRX_PERM);

	}

	public void fmRxMonoStereoModeChanged(JFmRxStatus status,
			JFmRx.JFmRxMonoStereoMode mode) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxMonoStereoModeChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxMonoStereoModeChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxMonoStereoModeChanged mode = "
					+ mode.getValue());
		}

		/* simple mechanism to prevent displaying repetitious messages */
		if (mode == mMode)
			return;

		mMode = mode;
		switch (mMode) {
		case FM_RX_MONO:
			if (DBG)
				Log.d(TAG, "Mono Mode");
			break;
		case FM_RX_STEREO:
			if (DBG)
				Log.d(TAG, "Stereo Mode");
			break;
		default:
			if (DBG)
				Log.d(TAG, "Illegal stereo mode received from stack: " + mode);
			break;
		}
		if (DBG)
			Log
					.d(TAG,
							"StubFmRxService:sending intent DISPLAY_MODE_MONO_STEREO_ACTION");
		Intent intentMode = new Intent(
				FmReceiverIntent.DISPLAY_MODE_MONO_STEREO_ACTION);
		intentMode.putExtra(FmReceiverIntent.MODE_MONO_STEREO, mode.getValue());
		intentMode.putExtra(FmReceiverIntent.STATUS, status.getValue());
		mContext.sendBroadcast(intentMode, FMRX_PERM);
	}

	public void fmRxAfSwitchFreqFailed(JFmRxStatus status,
			JFmRx.JFmRxRdsPiCode pi, JFmRx.JFmRxTuneFreq tunedFreq,
			JFmRx.JFmRxAfFreq afFreq) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchFreqFailed status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchFreqFailed pi = "
					+ pi.getValue());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchFreqFailed tunedFreq = "
					+ tunedFreq.getTuneFreq());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchFreqFailed afFreq = "
					+ afFreq.getAfFreq());
		}

	}

	public void fmRxAfSwitchStart(JFmRxStatus status, JFmRx.JFmRxRdsPiCode pi,
			JFmRx.JFmRxTuneFreq tunedFreq, JFmRx.JFmRxAfFreq afFreq) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchStart status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchStart pi = "
					+ pi.getValue());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchStart tunedFreq = "
					+ tunedFreq.getTuneFreq());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchStart afFreq = "
					+ afFreq.getAfFreq());
		}

	}

	public void fmRxAfSwitchComplete(JFmRxStatus status,
			JFmRx.JFmRxRdsPiCode pi, JFmRx.JFmRxTuneFreq tunedFreq,
			JFmRx.JFmRxAfFreq afFreq) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchComplete status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchComplete pi = "
					+ pi.getValue());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchComplete tunedFreq = "
					+ tunedFreq.getTuneFreq());
			Log.d(TAG, "StubFmRxService:fmRxAfSwitchComplete afFreq = "
					+ afFreq.getAfFreq());
		}
	}

	public void fmRxAfListChanged(JFmRxStatus status, JFmRx.JFmRxRdsPiCode pi,
			byte[] afList, JFmRx.JFmRxAfListSize afListSize) {

		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxAfListChanged status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxAfListChanged pi = "
					+ pi.getValue());
			Log.d(TAG, "StubFmRxService:fmRxAfListChanged afListSize = "
					+ afListSize.getValue());
		}

	}

	public void fmRxCompleteScanDone(JFmRxStatus status, int numOfChannels,
			int[] channelsData) {

		mIsCompleteScanInProgress = false;

		Log.i(TAG, "StubFmRxService.fmRxCompleteScanDone");
		if (DBG) {
			Log.d(TAG, "StubFmRxService:fmRxCompleteScanDone status = "
					+ status.toString());
			Log.d(TAG, "StubFmRxService:fmRxCompleteScanDone numOfChannels = "
					+ numOfChannels);
			for (int i = 0; i < numOfChannels; i++)
				Log.d(TAG,
						"StubFmRxService:fmRxCompleteScanDone channelsData = "
								+ i + "  " + +channelsData[i]);
		}

		Intent intentscan = new Intent(
				FmReceiverIntent.COMPLETE_SCAN_DONE_ACTION);
		Bundle b = new Bundle();
		b.putIntArray(FmReceiverIntent.SCAN_LIST, channelsData);
		b.putInt(FmReceiverIntent.STATUS, status.getValue());
		b.putInt(FmReceiverIntent.SCAN_LIST_COUNT, numOfChannels);
		intentscan.putExtras(b);
		mContext.sendBroadcast(intentscan, FMRX_PERM);
		try {
			mStopCompleteScanSyncQueue.put("*");
		} catch (InterruptedException e) {
			Log.e(TAG, "InterruptedException on queue wakeup!");
		}
		;

	}

	public void fmRxCmdError(JFmRxStatus status) {

		if (DBG)
			Log.d(TAG, "Sending intent FM_ERROR_ACTION");
		/*
		 * Intent intentError = new Intent(FmReceiverIntent.FM_ERROR_ACTION);
		 * mContext.sendBroadcast(intentError);
		 */

	}

	public void fmRxCmdDone(JFmRxStatus status, int command, long value) {

		if (DBG) {
			Log.d(TAG, "  fmRxCmdDone	 done (command )" + command);
			Log.d(TAG, "  fmRxCmdDone	 done (status: , value: )" + status + ""
					+ value);
		}

		switch (command) {
		case JFmRxCommand.CMD_ENABLE:
			if (DBG)
				Log.d(TAG, "StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_ENABLE");

			mState = FmReceiver.STATE_ENABLED;

			if (DBG)
				Log.d(TAG, "Sending restore values intent");
			Intent restoreValuesIntent = new Intent(FM_RESTORE_VALUES);
			mContext.sendBroadcast(restoreValuesIntent);

			break;

		case JFmRxCommand.CMD_DISABLE:
			if (DBG) {
				Log.d(TAG, "StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_DISABLE");
				Log.d(TAG, "  fmRxCmdDisable ( command: , status: , value: )" + command + "" + status + "" + value);
			}

			mState = FmReceiver.STATE_DISABLED;

			if (DBG)
				Log.d(TAG, "StubFmRxService:sending intent FM_DISABLED_ACTION");
			Intent intentDisable = new Intent(FmReceiverIntent.FM_DISABLED_ACTION);
			intentDisable.putExtra(FmReceiverIntent.STATUS, status.getValue());
			mContext.sendBroadcast(intentDisable, FMRX_PERM);
			break;

		case JFmRxCommand.CMD_SET_BAND:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_BAND");
				Log.d(TAG, "  fmRxCmdSetBand ( command: , status: , value: )"
						+ command + "" + status + "" + value);
				Log.d(TAG, "StubFmRxService:sending intent BAND_CHANGE_ACTION");
			}

			/* OMAPS00207918:implementation to make the set API Synchronous */
			try {
				mSetBandSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_BAND:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_BAND");
				Log.d(TAG, "  fmRxCmdGetBand ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}

			getBandValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mBandSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetBand ( getBandValue: )" + getBandValue);
			break;

		case JFmRxCommand.CMD_SET_MONO_STEREO_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_MONO_STEREO_MODE");
				Log.d(TAG,
						"  fmRxCmdSetMonoStereoMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			break;

		case JFmRxCommand.CMD_GET_MONO_STEREO_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_MONO_STEREO_MODE");
				Log.d(TAG,
						"  fmRxCmdGetMonoStereoMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			getMonoStereoModeValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mMonoStereoModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetMonoStereoMode ( getMonoStereoModeValue: )"
								+ getMonoStereoModeValue);

			break;

		case JFmRxCommand.CMD_SET_MUTE_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_MUTE_MODE");

				Log.d(TAG,
						"  fmRxCmdSetMuteMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			/* OMAPS00207918:implementation to make the set API Synchronous */
			try {
				mSetMuteModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_MUTE_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_MUTE_MODE");
				Log.d(TAG,
						"  fmRxCmdGetMuteMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			getMuteModeValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mMuteModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetMuteMode ( getMuteModeValue: )"
						+ getMuteModeValue);

			break;

		case JFmRxCommand.CMD_SET_RF_DEPENDENT_MUTE_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_RF_DEPENDENT_MUTE_MODE");
				Log.d(TAG,
						"  fmRxCmdSetRfDependentMuteMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			/* OMAPS00207918:implementation to make the set API Synchronous */
			try {
				mSetRfDependentMuteModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RF_DEPENDENT_MUTE_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RF_DEPENDENT_MUTE_MODE");

				Log.d(TAG,
						"  fmRxCmdGetRfDependentMuteMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getRfDependentMuteModeValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRfDependentMuteModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetRfDependentMuteMode ( getRfDependentMuteModeValue: )"
								+ getRfDependentMuteModeValue);

			break;

		case JFmRxCommand.CMD_SET_RSSI_THRESHOLD:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_RSSI_THRESHOLD");
				Log.d(TAG,
						"  fmRxCmdSetRssiThreshhold ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mSetRssiThresholdSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RSSI_THRESHOLD:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RSSI_THRESHOLD");
				Log.d(TAG,
						"fmRxCmdGetRssiThreshhold ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getRssiThresholdValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRssiThresholdSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetRssiThreshhold ( getRssiThresholdValue: )"
								+ getRssiThresholdValue);

			break;

		case JFmRxCommand.CMD_SET_DEEMPHASIS_FILTER:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_DEEMPHASIS_FILTER");
				Log.d(TAG,
						"  fmRxCmdSetDeemphasisFilter ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mSetDeEmphasisFilterSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_DEEMPHASIS_FILTER:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_DEEMPHASIS_FILTER");
				Log.d(TAG,
						"  fmRxCmdGetDeemphasisFilter ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getDeEmphasisFilterValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mDeEmphasisFilterSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetDeemphasisFilter ( getDeEmphasisFilterValue: )"
								+ getDeEmphasisFilterValue);

			break;

		case JFmRxCommand.CMD_SET_VOLUME:

			/*
			 * volume change will be completed here. So set the state to idle,
			 * so that user can set other volume.
			 */
			synchronized (mVolumeSynchronizationLock) {
				mVolState = VOL_REQ_STATE_IDLE;
			}
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_VOLUME");
				Log.d(TAG, "  fmRxCmdSetVolume ( command: , status: , value: )"
						+ command + "" + status + "" + value);
				Log.d(TAG,
						"StubFmRxService:sending intent VOLUME_CHANGED_ACTION");
			}

			break;

		case JFmRxCommand.CMD_SET_CHANNEL_SPACING:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_CHANNEL_SPACING");
				Log.d(TAG,
						"  fmRxCmdSetChannelSpacing ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			try {
				mSetChannelSpacingSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_VOLUME:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_VOLUME");
				Log.d(TAG, "  fmRxCmdGetVolume ( command: , status: , value: )"
						+ command + "" + status + "" + value);
				getVolumeValue = (int) value;
			}

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mVolumeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetVolume ( getVolumeValue: )"
						+ getVolumeValue);

			break;

		case JFmRxCommand.CMD_GET_CHANNEL_SPACING:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_CHANNEL_SPACING");
				Log.d(TAG,
						"  fmRxCmdGetChannelSpacing ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			getChannelSpaceValue = (int) value;
			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mChannelSpacingSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetChannelSpacing ( getChannelSpaceValue: )"
								+ getChannelSpaceValue);

			break;

		case JFmRxCommand.CMD_TUNE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_TUNE");
				Log.d(TAG, "  fmRxCmdTune ( command: , status: , value: )"
						+ command + "" + status + "" + value);
				Log.d(TAG,
						"StubFmRxService:sending intent TUNE_COMPLETE_ACTION");
			}
			mIsTuneInProgress = false;
			mCurrentFrequency = (int) value;
			Intent intentTune = new Intent(FmReceiverIntent.TUNE_COMPLETE_ACTION);
			intentTune.putExtra(FmReceiverIntent.TUNED_FREQUENCY, mCurrentFrequency);
			intentTune.putExtra(FmReceiverIntent.STATUS, status.getValue());
			mContext.sendBroadcast(intentTune, FMRX_PERM);

			break;

		case JFmRxCommand.CMD_GET_TUNED_FREQUENCY:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_TUNED_FREQUENCY");
				Log.d(TAG,
						"  fmRxCmdGetTunedFrequency ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			getTunedFrequencyValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mTunedFrequencySyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetTunedFrequency ( getTunedFrequencyValue: )"
								+ getTunedFrequencyValue);

			break;

		case JFmRxCommand.CMD_SEEK:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SEEK");
				Log.d(TAG, "  fmRxCmdSeek ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}
			mIsSeekInProgress = false;
			mCurrentFrequency = (int) value;
			if (DBG)
				Log.d(TAG, "StubFmRxService:sending intent SEEK_ACTION");
			
			Intent intentstart = new Intent(FmReceiverIntent.SEEK_ACTION);
			intentstart.putExtra(FmReceiverIntent.SEEK_FREQUENCY,
					mCurrentFrequency);
			intentstart.putExtra(FmReceiverIntent.STATUS, status.getValue());
			mContext.sendBroadcast(intentstart, FMRX_PERM);

			break;

		case JFmRxCommand.CMD_STOP_SEEK:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_STOP_SEEK");
				Log.d(TAG, "  fmRxCmdStopSeek ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}
			try {
				mStopSeekSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RSSI:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RSSI");
				Log.d(TAG, "  fmRxCmdGetRssi ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}

			/*
			 * FW is sending int8 which is read as uint16 in the stack, so we
			 * are converting it back to int.RSSI range is -127 to 128
			 */
                     
			getRssiValue = convertUnsignedToSignedInt(value);

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRssiSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetRssi ( getRssiValue: )" + getRssiValue);
			break;

		case JFmRxCommand.CMD_ENABLE_RDS:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_ENABLE_RDS");
				Log.d(TAG, "  fmRxCmdEnableRds ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}
			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mEnableRdsSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_DISABLE_RDS:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_DISABLE_RDS");
				Log.d(TAG,
						"  fmRxCmdDisableRds ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mDisableRdsSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_SET_RDS_SYSTEM:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_RDS_SYSTEM");
				Log.d(TAG,
						"  fmRxCmdSetRdsSystem ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			try {
				mSetRdsSystemSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RDS_SYSTEM:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RDS_SYSTEM");
				Log.d(TAG,
						"  fmRxCmdGetRdsSystem ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getRdsSystemValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRdsSystemSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetRdsSystem ( getRdsSystemValue: )"
						+ getRdsSystemValue);

			break;

		case JFmRxCommand.CMD_SET_RDS_GROUP_MASK:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_RDS_GROUP_MASK");
				Log.d(TAG,
						"  fmRxCmdSetRdsGroupMask ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			try {
				mSetRdsGroupMaskSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RDS_GROUP_MASK:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RDS_GROUP_MASK");
				Log.d(TAG,
						"  fmRxCmdGetRdsGroupMask ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getRdsGroupMaskValue = value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRdsGroupMaskSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetRdsGroupMask ( getRdsGroupMaskValue: )"
						+ getRdsGroupMaskValue);

			break;

		case JFmRxCommand.CMD_SET_RDS_AF_SWITCH_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_SET_RDS_AF_SWITCH_MODE");
				Log.d(TAG,
						"  fmRxCmdSetRdsAfSwitchMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			try {
				mSetRdsAfSwitchModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			break;

		case JFmRxCommand.CMD_GET_RDS_AF_SWITCH_MODE:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_GET_RDS_AF_SWITCH_MODE");
				Log.d(TAG,
						"  fmRxCmdGetRdsAfSwitchMode ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			getRdsAfSwitchModeValue = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mRdsAfSwitchModeSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetRdsAfSwitchMode ( getRdsAfSwitchModeValue: )"
								+ getRdsAfSwitchModeValue);

			break;

		case JFmRxCommand.CMD_ENABLE_AUDIO:
			if (DBG)
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_ENABLE_AUDIO");

			break;

		case JFmRxCommand.CMD_DISABLE_AUDIO:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_DISABLE_AUDIO");
				Log.d(TAG,
						"  fmRxCmdDisableAudio ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			break;

		case JFmRxCommand.CMD_DESTROY:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_DESTROY");
				Log.d(TAG, "  fmRxCmdDestroy ( command: , status: , value: )"
						+ command + "" + status + "" + value);
			}

			break;

		case JFmRxCommand.CMD_CHANGE_AUDIO_TARGET:
			if (DBG)
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  JFmRxCommand.CMD_CHANGE_AUDIO_TARGET");

			break;

		case JFmRxCommand.CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION");
				Log.d(TAG,
						"  fmRxCmdChangeDigitalAudioConfiguration ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			break;

		case JFmRxCommand.CMD_GET_FW_VERSION:
			if (DBG) {
				Log.d(TAG, "StubFmRxService:fmRxCmdDone  CMD_GET_FW_VERSION");
				Log.d(TAG,
						"  fmRxCmdGetFwVersion ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getFwVersion = ((double) value / 1000);

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mFwVersionSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdGetFwVersion ( getFwVersion: )"
						+ getFwVersion);

			break;

		case JFmRxCommand.CMD_COMPLETE_SCAN_PROGRESS:
			if (DBG) {
				Log
						.d(TAG,
								"StubFmRxService:fmRxCmdDone  CMD_COMPLETE_SCAN_PROGRESS");
				Log.d(TAG,
						"  fmRxCmdGetCompleteScanProgress ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			getScanProgress = (int) value;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mCompleteScanProgressSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;

			if (DBG)
				Log.d(TAG,
						"  fmRxCmdGetCompleteScanProgress ( getScanProgress: )"
								+ getScanProgress);

			break;

		case JFmRxCommand.CMD_STOP_COMPLETE_SCAN:
			if (DBG) {
				Log.d(TAG,
						"StubFmRxService:fmRxCmdDone  CMD_STOP_COMPLETE_SCAN");
				Log.d(TAG,
						"  fmRxCmdStopCompleteScan ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}

			mIsCompleteScanInProgress = false;
			mStopCompleteScanStatus = status.getValue();

			try {
				mStopCompleteScanSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG,
						"  fmRxCmdStopCompleteScan ( mStopCompleteScanStatus: )"
								+ mStopCompleteScanStatus);
			break;

		case JFmRxCommand.CMD_IS_CHANNEL_VALID:
			if (DBG) {
				Log.d(TAG, "StubFmRxService:fmRxCmdDone  CMD_IS_CHANNEL_VALID");
				Log.d(TAG,
						"  fmRxCmdIsValidChannel- ( command: , status: , value: )"
								+ command + "" + status + "" + value);
			}
			if (value > 0)
				mIsValidChannel = true;
			else
				mIsValidChannel = false;

			/* OMAPS00207918:implementation to make the get API Synchronous */
			try {
				mValidChannelSyncQueue.put("*");
			} catch (InterruptedException e) {
				Log.e(TAG, "InterruptedException on queue wakeup!");
			}
			;
			if (DBG)
				Log.d(TAG, "  fmRxCmdIsValidChannel ( isValidChannel: )"
						+ mIsValidChannel);

			break;

		default:
			Log.e(TAG, "Received completion event of unknown FM RX command: "
					+ command);
			break;
		}

	}

	// Receiver of the Master Volume Control Intent Broadcasted by AudioService
	private final BroadcastReceiver mFmRxIntentReceiver = new BroadcastReceiver() {

		public void onReceive(Context context, Intent intent) {

			String fmRxAction = intent.getAction();
			boolean setVolume = false;
			int volType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, AudioManager.STREAM_MUSIC);

			if (DBG)
				Log.d(TAG, " mFmRxIntentReceiver--->fmRxAction" + fmRxAction);

			/*
			 * Intent received when the volume has been changed from the master
			 * volume control keys. Convert it to the FM specific volume and set
			 * the volume.
			 */
			if (fmRxAction.equals(AudioManager.VOLUME_CHANGED_ACTION)) {
				if (DBG)
					Log.d(TAG, " AUDIOMANGER_VOLUME_CHANGED_ACTION    ");

				if (mState == FmReceiver.STATE_ENABLED &&
				    volType == AudioManager.STREAM_MUSIC) {
					if (DBG)
						Log.d(TAG, "will really change volume");

					setVolume = true;
				} // mState & volType

				else {
					if (DBG)
						Log
								.d(TAG,
										"VOLUME_CHANGED_ACTION Intent is Ignored, as FM is not yet Enabled");

				}
			} // volume _change

			if (fmRxAction.equals(FM_RESTORE_VALUES)) {
				if (DBG)
					Log.d(TAG, "FM_RESTORE_VALUES intent received");

				setVolume = true;

				if (DBG)
					Log.d(TAG, "sending intent FM_ENABLED_ACTION");
				Intent intentEnable = new Intent(
							FmReceiverIntent.FM_ENABLED_ACTION);
				mContext.sendBroadcast(intentEnable, FMRX_PERM);

				/*
				 * Tell the Audio Hardware interface that FM is enabled, so that
				 * routing happens appropriately
				 */
//				mAudioManager.setParameters(FM_RADIO_ACTIVE_KEY + "=on");
			}

			if (setVolume) {
				boolean volState;

				synchronized (mVolumeSynchronizationLock) {
					volState = mVolState;
				}
				if (mVolState) {
					int mVolume = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, 0);
					if (mVolume == 0) {
						Log.i(TAG, "no volume setting in intent");
						// read the current volume
						mVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
					}
					Log.i(TAG, " mVolume  " + mVolume);
					// Convert the stream volume to the FM specific volume.

					mVolume = (mVolume * FM_MAX_VOLUME)
							/ (mAudioManager
									.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
					if (DBG)
						Log.d(TAG, " mVolume  " + mVolume);
					
					if (!setVolume(mVolume)) {
						Log.e(TAG, "Not able, to set volume ");
					}
				} // mVolState
				else {
					Log.e(TAG, "previous volume set not complete ");
				}
			}
		}
	};

	/***************************************************************
	 * OMAPS00207258/OMAPS00207261 findAndPrintFromLookup():
	 * 
	 ****************************************************************/
	public String findFromLookupTable(byte[] indexArray,
			JFmRx.JFmRxRepertoire repertoire) {
		StringBuilder sb = new StringBuilder("");
		// TextView tv = (TextView)findViewById(R.id.hello_text);

		switch (repertoire) {
		case FMC_RDS_REPERTOIRE_G0_CODE_TABLE:
			if (DBG)
				Log.d(TAG, "Repertoire= " + repertoire);
			
			for (int i = 0; i < indexArray.length; i++) {
				int msb = (indexArray[i] & 0xF0) >> 4;
				int lsb = (indexArray[i] & 0x0F);
				sb.append(lookUpTable_G0[msb][lsb]);
			}
			break;

		case FMC_RDS_REPERTOIRE_G1_CODE_TABLE:
			if (DBG)
				Log.d(TAG, "Repertoire= " + repertoire);
			
			for (int i = 0; i < indexArray.length; i++) {
				int msb = (indexArray[i] & 0xF0) >> 4;
				int lsb = (indexArray[i] & 0x0F);
				sb.append(lookUpTable_G1[msb][lsb]);
			}
			break;

		case FMC_RDS_REPERTOIRE_G2_CODE_TABLE:
			if (DBG)
				Log.d(TAG, "Repertoire= " + repertoire);

			for (int i = 0; i < indexArray.length; i++) {
				int msb = (indexArray[i] & 0xF0) >> 4;
				int lsb = (indexArray[i] & 0x0F);
				sb.append(lookUpTable_G2[msb][lsb]);
			}
			break;

		default:
			if (DBG)
				Log.d(TAG, "Incorrect Repertoire received...");
			
			String convertedPsString = "???????????";
			break;
		}

		String convertedString = sb.toString();
		return convertedString;

	}

}
