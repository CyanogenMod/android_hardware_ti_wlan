/*
 * TI's FM
 *
 * Copyright 2001-2010 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright (C) 2010, 2011 Sony Ericsson Mobile Communications AB.
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
 *   FILE NAME:      FmReceiver.java
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
package com.ti.fm;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.content.ServiceConnection;
import android.util.Log;

/**
 * The Android FmReceiver API is not finalized, and *will* change. Use at your
 * own risk.
 * 
 * Public API for controlling the FmReceiver Service. FmReceiver is a proxy
 * object for controlling the Fm Reception Service via IPC.
 * 
 * Creating a FmReceiver object will create a binding with the FMRX service.
 * Users of this object should call close() when they are finished with the
 * FmReceiver, so that this proxy object can unbind from the service.
 * 
 * 
 * @hide
 */
public class FmReceiver {

	private static final String TAG = "FmReceiver";
	private static final boolean DBG = true;

	private IFmReceiver mService;
	private Context mContext;
	private ServiceListener mServiceListener;

	public static final int STATE_ENABLED = 0;
	public static final int STATE_DISABLED = 1;
	public static final int STATE_ENABLING = 2;
	public static final int STATE_DISABLING = 3;
	public static final int STATE_PAUSE = 4;
	public static final int STATE_RESUME = 5;
	public static final int STATE_DEFAULT = 6;

	// Constructor
	private FmReceiver() {

	}

	private ServiceConnection mConnection = new ServiceConnection() {
		public void onServiceConnected(ComponentName className, IBinder service) {
			// This is called when the connection with the service has been
			// established, giving us the service object we can use to
			// interact with the service. We are communicating with our
			// service through an IDL interface, so get a client-side
			// representation of that from the raw service object.

			mService = IFmReceiver.Stub.asInterface(service);
			if (mServiceListener != null) {	
				Log.i(TAG, "Sending callback");
				mServiceListener.onServiceConnected();
			} else {
				Log.i(TAG, "mService is NULL");
			}
		}

		public void onServiceDisconnected(ComponentName className) {
			// This is called when the connection with the service has been
			// unexpectedly disconnected -- that is, its process crashed.

			mService = null;
			if (mServiceListener != null) {
				mServiceListener.onServiceDisconnected();
			}
		}
	};

	public FmReceiver(Context context, ServiceListener listener) {
		mContext = context;
		mServiceListener = listener;


		mContext.bindService(new Intent("com.ti.server.FmRxService"),
						mConnection, Context.BIND_AUTO_CREATE);
	}

	/**
	 * An interface for notifying FmReceiver IPC clients when they have been
	 * connected to the FMRX service.
	 */
	public interface ServiceListener {
		/**
		 * Called to notify the client when this proxy object has been connected
		 * to the FMRX service. Clients must wait for this callback before
		 * making IPC calls on the FMRX service.
		 */
		public void onServiceConnected();

		/**
		 * Called to notify the client that this proxy object has been
		 * disconnected from the FMRX service. Clients must not make IPC calls
		 * on the FMRX service after this callback. This callback will currently
		 * only occur if the application hosting the BluetoothAg service, but
		 * may be called more often in future.
		 */
		public void onServiceDisconnected();
	}

	protected void finalize() throws Throwable {
		if (DBG) Log.d(TAG, "FmReceiver:finalize");
		try {
			close();
		} finally {
			super.finalize();
		}
	}

	/**
	 * Close the connection to the backing service. Other public functions of
	 * BluetoothAg will return default error results once close() has been
	 * called. Multiple invocations of close() are ok.
	 */
	public synchronized void close() {

		if (mService != null)
		{
			try {
				mContext.unbindService(mConnection);
			} catch (IllegalArgumentException ex) {
				Log.w(TAG, "unbindService failed: " +ex);
			}
			mService = null;
		}
		mConnection = null;
	}

	public boolean create() {

		if (mService != null) {
			try {
				return mService.create();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean isEnabled() {

		if (mService != null) {
			try {
				return mService.isEnabled();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getFMState() {

		if (mService != null) {
			try {
				return mService.getFMState();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return -1;
	}

	/**
	 * Returns true if the FM RX is enabled. Returns false if not enabled, or if
	 * this proxy object if not currently connected to the FmReceiver service.
	 */
	public boolean enable() {

		if (mService != null) {
			try {
				return mService.enable();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	/**
	 * Returns true if the EM RX is disabled. Returns false if not enabled, or
	 * if this proxy object if not currently connected to the FmReceiver
	 * service.
	 */
	public boolean disable() {


		if (mService != null) {
			try {
				return mService.disable();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean destroy() {

		if (mService != null) {
			try {
				return mService.destroy();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean setBand(int band) {

		if (mService != null) {
			try {
				return mService.setBand(band);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getBand() {

		if (mService != null) {
			try {
				return mService.getBand();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setMonoStereoMode(int mode) {


		if (mService != null) {
			try {
				return mService.setMonoStereoMode(mode);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean isValidChannel() {


		if (mService != null) {
			try {
				return mService.isValidChannel();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean completeScan() {


		if (mService != null) {
			try {
				return mService.completeScan();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int stopCompleteScan() {


		if (mService != null) {
			try {
				return mService.stopCompleteScan();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public double getFwVersion() {


		if (mService != null) {
			try {
				return mService.getFwVersion();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public int getCompleteScanProgress() {


		if (mService != null) {
			try {
				return mService.getCompleteScanProgress();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public int getMonoStereoMode() {


		if (mService != null) {
			try {
				return mService.getMonoStereoMode();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setMuteMode(int muteMode) {


		if (mService != null) {
			try {
				return mService.setMuteMode(muteMode);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getMuteMode() {


		if (mService != null) {
			try {
				return mService.getMuteMode();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setRfDependentMuteMode(int rfMuteMode) {

		if (mService != null) {
			try {
				return mService.setRfDependentMuteMode(rfMuteMode);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getRfDependentMuteMode() {


		if (mService != null) {
			try {
				return mService.getRfDependentMuteMode();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setRssiThreshold(int threshhold) {


		if (mService != null) {
			try {
				return mService.setRssiThreshold(threshhold);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getRssiThreshold() {

		if (mService != null) {
			try {
				return mService.getRssiThreshold();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setDeEmphasisFilter(int filter) {

		if (mService != null) {
			try {
				return mService.setDeEmphasisFilter(filter);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getDeEmphasisFilter() {

		if (mService != null) {
			try {
				return mService.getDeEmphasisFilter();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setChannelSpacing(int channelSpace) {

		if (mService != null) {
			try {
				return mService.setChannelSpacing(channelSpace);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getChannelSpacing() {

		if (mService != null) {
			try {
				return mService.getChannelSpacing();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean tune(int freq) {

		if (mService != null) {
			try {
				return mService.tune(freq);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getTunedFrequency() {

		if (mService != null) {
			try {
				return mService.getTunedFrequency();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean seek(int direction) {

		if (mService != null) {
			try {
				return mService.seek(direction);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean stopSeek() {


		if (mService != null) {
			try {
				return mService.stopSeek();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getRdsSystem() {


		if (mService != null) {
			try {
				return mService.getRdsSystem();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public int getRssi() {


		if (mService != null) {
			try {
				return mService.getRssi();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setRdsSystem(int system) {


		if (mService != null) {
			try {
				return mService.setRdsSystem(system);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean enableRds() {


		if (mService != null) {
			try {
				return mService.enableRds();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean disableRds() {


		if (mService != null) {
			try {
				return mService.disableRds();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean setRdsGroupMask(int mask) {

		if (mService != null) {
			try {
				return mService.setRdsGroupMask(mask);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public long getRdsGroupMask() {

		if (mService != null) {
			try {
				return mService.getRdsGroupMask();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean setRdsAfSwitchMode(int mode) {

		if (mService != null) {
			try {
				return mService.setRdsAfSwitchMode(mode);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public int getRdsAfSwitchMode() {

		if (mService != null) {
			try {
				return mService.getRdsAfSwitchMode();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return 0;
	}

	public boolean changeAudioTarget(int mask, int digitalConfig) {

		if (mService != null) {
			try {
				return mService.changeAudioTarget(mask, digitalConfig);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean changeDigitalTargetConfiguration(int digitalConfig) {

		if (mService != null) {
			try {
				return mService.changeDigitalTargetConfiguration(digitalConfig);
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean enableAudioRouting() {

		if (mService != null) {
			try {
				return mService.enableAudioRouting();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

	public boolean disableAudioRouting() {

		if (mService != null) {
			try {
				return mService.disableAudioRouting();
			} catch (RemoteException e) {
				Log.e(TAG, e.toString());
			}
		} else {
			Log.w(TAG, "Proxy not attached to service");
			if (DBG) Log.d(TAG, Log.getStackTraceString(new Throwable()));
		}
		return false;
	}

}
