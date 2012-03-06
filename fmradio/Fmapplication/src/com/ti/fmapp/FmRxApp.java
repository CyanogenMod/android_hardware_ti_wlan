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
 *   FILE NAME:      FmRxApp.java
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
package com.ti.fmapp;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.OrientationListener;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import com.ti.fm.FmReceiver;
import com.ti.fm.FmReceiverIntent;
import com.ti.fm.IFmConstants;
import com.ti.fm.FmReceiver;
import android.app.AlertDialog;
import android.os.Bundle;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.view.Window;
import android.media.AudioManager;
import android.widget.Toast;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.ViewGroup;

public class FmRxApp extends Activity implements View.OnClickListener,
		IFmConstants, FmRxAppConstants, FmReceiver.ServiceListener {
	public static final String TAG = "FmRxApp";
	private static final boolean DBG = true;

	/********************************************
	 * Widgets
	 ********************************************/

	private ImageView imgFmPower, imgFmMode, imgFmVolume, imgFmAudiopath;
	private ImageButton imgFmSeekUp, imgFmSeekDown;
	private TextView txtStatusMsg, txtRadioText;
	private TextView txtPsText;
	static TextView txtStationName;
	private static Button btnStation1, btnStation2, btnStation3;
	private static Button btnStation4, btnStation5, btnStation6;
	private ProgressDialog pd = null, configPd;

	/********************************************
	 * Menu Constants
	 ********************************************/
	public static final int MENU_CONFIGURE = Menu.FIRST + 1;
	public static final int MENU_EXIT = Menu.FIRST + 2;
	public static final int MENU_ABOUT = Menu.FIRST + 3;
	public static final int MENU_PRESET = Menu.FIRST + 4;
	public static final int MENU_SETFREQ = Menu.FIRST + 5;

	/********************************************
	 * private variables
	 ********************************************/
	private int mToggleMode = 0; // To toggle between the mono/stereo
	private int mToggleAudio = 1; // To toggle between the speaker/headset
	private int mToggleMute = FM_UNMUTE; // To toggle between the mute/unmute

	/* Default values */
	private int mMode = INITIAL_VAL;
	private boolean mRds = DEFAULT_RDS;
	private boolean mRdsAf = DEFAULT_RDS_AF;
	private int mRdsSystem = INITIAL_VAL;
	private int mDeEmpFilter = INITIAL_VAL;
	private int mRssi = INITIAL_RSSI;
	// Seek up/down direction
	private int mDirection = FM_SEEK_UP;

	/* State values */

	// variable to make sure that the next configuration change happens after
	// the current configuration request has been completed.
	private int configurationState = CONFIGURATION_STATE_IDLE;
	// variable to make sure that the next seek happens after the current seek
	// request has been completed.
	private boolean mSeekState = SEEK_REQ_STATE_IDLE;

	private boolean mStatus;
	private int mIndex = 0;
	private int mStationIndex;

	/*
	 * Variable to identify whether we need to do the default setting when
	 * entering the FM application. Based on this variable,the default
	 * configurations for the FM will be done for the first time
	 */

	private static boolean sdefaultSettingOn = false;
	private NotificationManager mNotificationManager;
	private int FM_NOTIFICATION_ID;

	static final String FM_INTERRUPTED_KEY = "fm_interrupted";
	static final String FM_STATE_KEY = "fm_state";
	/* Flag to know whether FM App was interrupted due to orientation change */
	boolean mFmInterrupted = false;
	/********************************************
	 * public variables
	 ********************************************/
	public static int sBand = DEFAULT_BAND;
	public static int sChannelSpace = DEFAULT_CHANNELSPACE;

	public static Float lastTunedFrequency = (float) DEFAULT_FREQ_EUROPE;
	public static FmReceiver sFmReceiver;

	/** Arraylist of stations */
	public static ArrayList<HashMap<String, String>> stations = new ArrayList<HashMap<String, String>>(
			6);
	public static TextView txtFmRxTunedFreq;
	private OrientationListener mOrientationListener;

	Context mContext;

	/** Called when the activity is first created. */

	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		mContext = this;
		/* Retrieve the fm_state and find out whether FM App was interrupted */

		if (savedInstanceState != null) {
			Bundle fmState = savedInstanceState.getBundle(FM_STATE_KEY);
			if (fmState != null) {
				mFmInterrupted = fmState.getBoolean(FM_INTERRUPTED_KEY, false);

			}
		}

		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setVolumeControlStream(AudioManager.STREAM_MUSIC);

		// Register for FM intent broadcasts.
		IntentFilter intentFilter = new IntentFilter();
		intentFilter.addAction(FmReceiverIntent.FM_ENABLED_ACTION);
		intentFilter.addAction(FmReceiverIntent.FM_DISABLED_ACTION);
		intentFilter.addAction(FmReceiverIntent.SEEK_ACTION);
		intentFilter.addAction(FmReceiverIntent.RDS_TEXT_CHANGED_ACTION);
		intentFilter.addAction(FmReceiverIntent.PS_CHANGED_ACTION);
		intentFilter.addAction(FmReceiverIntent.TUNE_COMPLETE_ACTION);
		intentFilter
				.addAction(FmReceiverIntent.DISPLAY_MODE_MONO_STEREO_ACTION);
		intentFilter.addAction(FmReceiverIntent.PI_CODE_CHANGED_ACTION);
		intentFilter.addAction(FmReceiverIntent.COMPLETE_SCAN_DONE_ACTION);
		intentFilter.addAction(FmReceiverIntent.FM_ERROR_ACTION);

		registerReceiver(mReceiver, intentFilter);

		/*
		 * Need to enable the FM if it is not enabled earlier
		 */

		sFmReceiver = new FmReceiver(this, this);
	}

	private void startup() {

		switch (sFmReceiver.getFMState()) {
		/* FM is alreary enabled. Just update the UI. */
		case FmReceiver.STATE_ENABLED:
			// Fm app is on and we are entering from the other screens
			setContentView(R.layout.fmrxmain);
			initControls();
			loadDefaultConfiguration();
			// Clear the notification which was displayed.
			this.mNotificationManager.cancel(FM_NOTIFICATION_ID);
			break;

		/* FM enable has been started from service. Inform the user */
		case FmReceiver.STATE_RESUME:
			/* Display the dialog till FM is enabled */
			pd = ProgressDialog.show(this, "Please wait..",
					"Powering on Radio", true, false);
			break;

		case FmReceiver.STATE_DISABLED:

			mStatus = sFmReceiver.enable();
			if (mStatus == false) {
				showAlert(this, "FmRadio", "Cannot enable Radio!!!!");

			}

			else { /* Display the dialog till FM is enabled */
				pd = ProgressDialog.show(this, "Please wait..",
						"Powering on Radio", true, false);
			}

			break;
		/* FM has not been started. Start the FM */
		case FmReceiver.STATE_DEFAULT:

			/*
			 * Make sure not to start the FM_Enable() again, if it has been
			 * already called before orientation change
			 */
			if (mFmInterrupted == false) {
				mStatus = sFmReceiver.create();
				if (mStatus == false) {
					showAlert(this, "FmRadio", "Cannot create Radio!!!!");

				}
				mStatus = sFmReceiver.enable();
				if (mStatus == false) {
					showAlert(this, "FmRadio", "Cannot enable Radio!!!!");

				}

				else { /* Display the dialog till FM is enabled */
					pd = ProgressDialog.show(this, "Please wait..",
							"Powering on Radio", true, false);
				}
			} else {
				Log.i(TAG, "mFmInterrupted is true dont  call enable");

			}
			break;
		}
	}

	public void onServiceConnected() {
		Log.i(TAG, "onServiceConnected");
		startup();
	}

	public void onServiceDisconnected() {
		Log.i(TAG, "Lost connection to service");
		sFmReceiver = null;
	}

	/*
	 * When the user exits the FMApplication by selecting back keypresst ,FM App
	 * screen will be closed But the FM will be still on. A notification will be
	 * shown to the user with the current playing FM frequency. User can select
	 * the notification and relaunch the FM application
	 */

	private void showNotification(int statusBarIconID, int app_rx,
			CharSequence text, boolean showIconOnly) {
		Notification notification = new Notification(statusBarIconID, text,
				System.currentTimeMillis());

		// The PendingIntent to launch our activity if the user selects this
		// notification
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
				new Intent(this, FmRxApp.class), 0);

		// Set the info for the views that show in the notification panel.
		notification.setLatestEventInfo(this, getText(R.string.app_rx), text,
				contentIntent);

		// Send the notification.
		// We use a layout id because it is a unique number. We use it later to
		// cancel.
		mNotificationManager.notify(FM_NOTIFICATION_ID, notification);
	}

	/*
	 * Set the arraylist with the selected station name and display the name on
	 * main screen
	 */
	public static void UpdateRenameStation(int index, String name) {
		txtStationName.setText(name.toString());
		// Store the name in the selected index in Arraylist
		SetStation(index, "", name);

	}

	/*
	 * Set the arraylist and the buttons with the selected station and make the
	 * button enabled so that user can select it to select the stored stations
	 */
	public static void updateSetStation(int index, String freq, String name) {
		switch (index) {
		case 0:
			if (freq.equals(""))
				btnStation1.setEnabled(false);
			else {
				btnStation1.setEnabled(true);
				btnStation1.setText(freq);
			}

			break;
		case 1:
			if (freq.equals(""))
				btnStation2.setEnabled(false);
			else {
				btnStation2.setEnabled(true);
				btnStation2.setText(freq);
			}

			break;
		case 2:
			if (freq.equals(""))
				btnStation3.setEnabled(false);
			else {
				btnStation3.setEnabled(true);
				btnStation3.setText(freq);
			}
			break;
		case 3:
			if (freq.equals(""))
				btnStation4.setEnabled(false);
			else {
				btnStation4.setEnabled(true);
				btnStation4.setText(freq);
			}

			break;
		case 4:
			if (freq.equals(""))
				btnStation5.setEnabled(false);
			else {
				btnStation5.setEnabled(true);
				btnStation5.setText(freq);
			}

			break;
		case 5:
			if (freq.equals(""))
				btnStation6.setEnabled(false);
			else {
				btnStation6.setEnabled(true);
				btnStation6.setText(freq);
			}
			break;
		}
		// Store the freq in the selected index in Arraylist
		SetStation(index, freq, "");

	}

	/*
	 * Unset the arraylist and the button with the empty station and make the
	 * button disabled so that user cannot select it
	 */

	public static void updateUnSetStation(int index) {
		switch (index) {
		case 0:
			btnStation1.setEnabled(false);
			btnStation1.setText("1");
			break;
		case 1:
			btnStation2.setEnabled(false);
			btnStation2.setText("2");
			break;
		case 2:
			btnStation3.setEnabled(false);
			btnStation3.setText("3");
			break;
		case 3:
			btnStation4.setEnabled(false);
			btnStation4.setText("4");
			break;
		case 4:

			btnStation5.setEnabled(false);
			btnStation5.setText("4");
			break;
		case 5:
			btnStation6.setEnabled(false);
			btnStation6.setText("6");
			break;
		}

		UnSetStation(index, "", "");

	}

	/* Set particular station name or frequency in the stations arraylist */
	public static void SetStation(Integer index, String freq, String name) {
		Integer pos = new Integer(index.intValue() + 1);
		try {
			HashMap<String, String> item = stations.get(index.intValue());
			item.put(ITEM_KEY, pos.toString());
			// Update the name only
			if ((freq.equals(""))) {
				item.put(ITEM_NAME, name);

				// Empty name string is passed. Update the name
				if ((name.equals("")))
					item.put(ITEM_NAME, name);
			}

			// Update the frequency only
			else if ((name.equals(""))) {
				item.put(ITEM_VALUE, freq);

			}
			stations.set(index.intValue(), item);
		} catch (NullPointerException e) {
			Log.e(TAG, "Tried to add null value");
		}

	}

	/* UnSet particular station name and frequency in the stations arraylist */
	public static void UnSetStation(Integer index, String freq, String name) {
		Integer pos = new Integer(index.intValue() + 1);
		try {
			HashMap<String, String> item = stations.get(index.intValue());
			item.put(ITEM_KEY, pos.toString());
			item.put(ITEM_VALUE, freq);
			item.put(ITEM_NAME, name);
			stations.set(index.intValue(), item);
		} catch (NullPointerException e) {
			Log.e(TAG, "Tried to add null value");
		}
	}

	/* Get particular station from the stations arraylist */

	public static String GetStation(Integer index) {
		Integer pos = new Integer(index.intValue() + 1);
		try {
			HashMap<String, String> item = stations.get(index.intValue());
			Object freq = item.get(ITEM_VALUE);
			Object name = item.get(ITEM_NAME);
			if (name != null) {
				txtStationName.setText(name.toString());
			} else {
				Log.w(TAG, "There is no name in the HashMap.");
			}

			if (freq != null) {
				return freq.toString();
			} else {
				Log.w(TAG, "There is no key in the HashMap.");
			}
		} catch (NullPointerException e) {
			Log.e(TAG, "Tried to add null value");
		}
		return null;
	}

	/*
	 * Handler for all the FM related events. The events will be handled only
	 * when we are in the FM application main menu
	 */

	private Handler mHandler = new Handler() {

		public void handleMessage(Message msg) {

			switch (msg.what) {

			/*
			 * After FM is enabled dismiss the progress dialog and display the
			 * FM main screen. Set the default volume.
			 */

			case EVENT_FM_ENABLED:

				if (pd != null)
					pd.dismiss();
				setContentView(R.layout.fmrxmain);
				initControls();
				loadDefaultConfiguration();
				imgFmPower.setImageResource(R.drawable.poweron);


				/*
				 * Setting the default band when FM app is started for
				 * the first time or after reentering the Fm app
				 */
				if (sdefaultSettingOn == false) {
					/* Set the default band */
					mStatus = sFmReceiver.setBand(sBand);
					if (mStatus == false) {
						showAlert(getParent(), "FmRadio",
								"Not able to setband!!!!");
					} else {

						lastTunedFrequency = (float) lastTunedFrequency * 1000;
						mStatus = sFmReceiver.tune(lastTunedFrequency
								.intValue());
						if (mStatus == false) {
							showAlert(getParent(), "FmRadio",
									"Not able to tune!!!!");
						}
						lastTunedFrequency = lastTunedFrequency / 1000;
					}

				}


				break;

			/* Update the icon on main screen with appropriate mono/stereo icon */
			case EVENT_MONO_STEREO_DISPLAY:

				Integer mode = (Integer) msg.obj;
				if (DBG)
					Log.d(TAG, "enter handleMessage ---mode" + mode.intValue());
				if (mode.intValue() == 0) {
					imgFmMode.setImageResource(R.drawable.fm_stereo);
				} else {
					imgFmMode.setImageResource(R.drawable.fm_mono);
				}
				break;

			case EVENT_FM_DISABLED:

				/*
				 * we have exited the FM App. Set the sdefaultSettingOn flag to
				 * false Save the default configuration in the preference
				 */
				sdefaultSettingOn = false;
				saveDefaultConfiguration();

				mStatus = sFmReceiver.destroy();
				if (mStatus == false) {
					showAlert(getParent(), "FmRadio", "Not able to destroy!!!!");
				}

				finish();
				break;

			case EVENT_SEEK_STARTED:

				Integer freq = (Integer) msg.obj;
				Log.i(TAG, "enter handleMessage ----EVENT_SEEK_STARTED freq"
						+ freq);
				lastTunedFrequency = (float) freq / 1000;
				txtStatusMsg.setText(R.string.playing);
				txtFmRxTunedFreq.setText(lastTunedFrequency.toString());
				// clear the RDS text
				txtRadioText.setText(null);
				// clear the PS text
				txtPsText.setText(null);

				/*
				 * Seek up/down will be completed here. So set the state to
				 * idle, so that user can seek to other frequency.
				 */

				mSeekState = SEEK_REQ_STATE_IDLE;

				break;

			/*
			 * enable audio routing , if the fm app is getting started first
			 * time
			 */
			case EVENT_TUNE_COMPLETE:
				Integer tuneFreq = (Integer) msg.obj;
				if (DBG)
					Log.d(TAG,
							"enter handleMessage ----EVENT_TUNE_COMPLETE tuneFreq"
									+ tuneFreq);
				
				lastTunedFrequency = (float) tuneFreq / 1000;
				txtStatusMsg.setText(R.string.playing);
				txtFmRxTunedFreq.setText(lastTunedFrequency.toString());
				// clear the RDS text
				txtRadioText.setText(null);
				if (DBG)
					Log.d(TAG, "sdefaultSettingOn" + sdefaultSettingOn);

				if (sdefaultSettingOn == false) {
					/*
					 * The default setting for the FMApp are completed here. If
					 * the user selects back and goes out of the FM App, when he
					 * reenters we dont have to do the default configurations
					 */
					sdefaultSettingOn = true;
				}

				// clear the RDS text
				txtRadioText.setText(null);

				// clear the PS text
				txtPsText.setText(null);

				break;

			/* Display the RDS text on UI */
			case EVENT_RDS_TEXT:
				if (FM_SEND_RDS_IN_BYTEARRAY == true) {
					byte[] rdsText = (byte[]) msg.obj;

					for (int i = 0; i < 4; i++)
						Log.i(TAG, "rdsText" + rdsText[i]);
				} else {
					String rds = (String) msg.obj;
					if (DBG)
						Log.d(TAG, "enter handleMessage ----EVENT_RDS_TEXT rds"
								+ rds);
					txtRadioText.setText(rds);
				}
				break;

			/* Display the RDS text on UI */
			case EVENT_PI_CODE:
				String pi = (String) msg.obj;
	
				break;

			/* Display the PS text on UI */
			case EVENT_PS_CHANGED:

				if (FM_SEND_RDS_IN_BYTEARRAY == true) {
					byte[] psName = (byte[]) msg.obj;

					for (int i = 0; i < 4; i++)
						Log.i(TAG, "psName" + psName[i]);
				} else {
					String ps = (String) msg.obj;
					if (DBG)
						Log.d(TAG, "ps  String is " + ps);
					
					txtPsText.setText(ps);
				}

				break;

			case EVENT_COMPLETE_SCAN_DONE:

				int[] channelList = (int[]) msg.obj;
				int noOfChannels = (int) msg.arg2;

				if (DBG) {

					for (int i = 0; i < noOfChannels; i++)

						Log.d(TAG, "channelList" + channelList[i]);
				}

				break;

			case EVENT_FM_ERROR:

				Log.i(TAG, "enter handleMessage ----EVENT_FM_ERROR");
				// showAlert(getParent(), "FmRadio", "Error!!!!");

				LayoutInflater inflater = getLayoutInflater();
				View layout = inflater.inflate(R.layout.toast,
						(ViewGroup) findViewById(R.id.toast_layout));
				TextView text = (TextView) layout.findViewById(R.id.text);
				text.setText("Error in FM Application");

				Toast toast = new Toast(getApplicationContext());
				toast
						.setGravity(Gravity.BOTTOM | Gravity.CENTER_VERTICAL,
								0, 0);
				toast.setDuration(Toast.LENGTH_LONG);
				toast.setView(layout);
				toast.show();
				break;

			}
		}
	};

	/* Display alert dialog */
	public void showAlert(Context context, String title, String msg) {

		new AlertDialog.Builder(context).setTitle(title).setIcon(
				android.R.drawable.ic_dialog_alert).setMessage(msg)
				.setNegativeButton(android.R.string.cancel, null).show();

	}

	private void setRdsConfig() {
		configurationState = CONFIGURATION_STATE_PENDING;
		SharedPreferences fmConfigPreferences = getSharedPreferences(
				"fmConfigPreferences", MODE_PRIVATE);

		// Set Band
		int band = fmConfigPreferences.getInt(BAND, DEFAULT_BAND);
		if (DBG)
			Log.d(TAG, "setRdsConfig()--- band= " + band);
		if (band != sBand) // If Band is same as the one set already donot set
		// it again
		{
			mStatus = sFmReceiver.setBand(band);
			if (mStatus == false) {
				Log.e(TAG, "setRdsConfig()-- setBand ->Erorr");
				showAlert(this, "FmRadio",
						"Cannot  setBand to selected Value!!!!");
			} else {
				sBand = band;
				if (sdefaultSettingOn == true) {
					/* Set the default frequency */
					if (sBand == FM_BAND_EUROPE_US)
						lastTunedFrequency = (float) DEFAULT_FREQ_EUROPE;
					else
						lastTunedFrequency = (float) DEFAULT_FREQ_JAPAN;
				}

				lastTunedFrequency = (float) lastTunedFrequency * 1000;
				mStatus = sFmReceiver.tune(lastTunedFrequency.intValue());
				if (mStatus == false) {
					showAlert(getParent(), "FmRadio", "Not able to tune!!!!");
				}
				lastTunedFrequency = lastTunedFrequency / 1000;
			}

		}

		// Set De-emp Filter
		int deEmp = fmConfigPreferences.getInt(DEEMP, DEFAULT_DEEMP);
		if (mDeEmpFilter != deEmp)// If De-Emp filter is same as the one set
		// already donot set it again
		{
			mStatus = sFmReceiver.setDeEmphasisFilter(deEmp);
			if (DBG)
				Log.d(TAG, "setRdsConfig()--- DeEmp= " + deEmp);

			if (mStatus == false) {
				Log.e(TAG, "setRdsConfig()-- setDeEmphasisFilter ->Erorr");
				showAlert(this, "FmRadio",
						"Cannot set De-Emp Fileter to selected Value!!!!");

			}
			mDeEmpFilter = deEmp;

		}

		// Set Mode
		int mode = fmConfigPreferences.getInt(MODE, DEFAULT_MODE);
		if (mMode != mode)// If Mode is same as the one set already donot set it
		// again
		{
			if (DBG)
				Log.d(TAG, "setRdsConfig()--- Band= "
						+ fmConfigPreferences.getInt(BAND, DEFAULT_BAND));

			mStatus = sFmReceiver.setMonoStereoMode(mode);
			if (mStatus == false) {
				showAlert(this, "FmRadio", "Not able to setmode!!!!");
			} else {
				mMode = mode;
				if (mMode == 0) {
					imgFmMode.setImageResource(R.drawable.fm_stereo);
				} else {
					imgFmMode.setImageResource(R.drawable.fm_mono);
				}
			}

		}

		/** Set channel spacing to the one selected by the user */
		int channelSpace = fmConfigPreferences.getInt(CHANNELSPACE,
				DEFAULT_CHANNELSPACE);
		if (DBG)
			Log.d(TAG, "setChannelSpacing()--- channelSpace= " + channelSpace);
		if (channelSpace != sChannelSpace) // If channelSpace is same as the one
		// set already donot set
		// it again
		{
			mStatus = sFmReceiver.setChannelSpacing(channelSpace);
			if (mStatus == false) {
				Log.e(TAG, "setChannelSpacing()-- setChannelSpacing ->Erorr");
				showAlert(this, "FmRadio",
						"Cannot  setChannelSpacing to selected Value!!!!");
			}
			sChannelSpace = channelSpace;
		}

		// Set Rssi
		int rssiThreshHold = fmConfigPreferences.getInt(RSSI, DEFAULT_RSSI);
		if (DBG)
			Log.d(TAG, "setRssi()-ENTER --- rssiThreshHold= " + rssiThreshHold);
		
		if (mRssi != rssiThreshHold) {

			// Set RSSI if a new value is entered by the user

			mStatus = sFmReceiver.setRssiThreshold(rssiThreshHold);
			if (mStatus == false) {
				showAlert(this, "FmRadio", "Not able to setRssiThreshold!!!!");
			}

			mRssi = rssiThreshHold;
		}

		// set RDS related configuration
		boolean rdsEnable = fmConfigPreferences.getBoolean(RDS, DEFAULT_RDS);
		if (DBG)
			Log.d(TAG, "setRDS()--- rdsEnable= " + rdsEnable);
		if (mRds != rdsEnable) {

			if (rdsEnable) {
				mStatus = sFmReceiver.enableRds();
				if (mStatus == false) {
					Log.e(TAG, "setRDS()-- enableRds() ->Erorr");
					showAlert(this, "FmRadio", "Cannot enable RDS!!!!");
				}

			} else {
				mStatus = sFmReceiver.disableRds();
				if (mStatus == false) {
					Log.e(TAG, "setRDS()-- disableRds() ->Erorr");
					showAlert(this, "FmRadio", "Cannot disable RDS!!!!");
				} else {
					/* clear the PS and RDS text */
					txtPsText.setText(null);
					txtRadioText.setText(null);
				}
			}
			mRds = rdsEnable;
		}

		// setRdssystem
		int rdsSystem = fmConfigPreferences.getInt(RDSSYSTEM,
				DEFAULT_RDS_SYSTEM);
		if (DBG)
			Log.d(TAG, "setRdsSystem()--- rdsSystem= " + rdsSystem);
		
		if (mRdsSystem != rdsSystem) {
			// Set RDS-SYSTEM if a new choice is made by the user

			mStatus = sFmReceiver.setRdsSystem(fmConfigPreferences.getInt(
					RDSSYSTEM, DEFAULT_RDS_SYSTEM));
			if (mStatus == false) {
				Log.e(TAG, " setRdsSystem()-- setRdsSystem ->Erorr");
				showAlert(this, "FmRadio",
						"Cannot set Rds System to selected Value!!!!");
			}
			mRdsSystem = rdsSystem;
		}

		boolean rdsAfSwitch = fmConfigPreferences.getBoolean(RDSAF,
				DEFAULT_RDS_AF);
		int rdsAf = 0;
		rdsAf = rdsAfSwitch ? 1 : 0;
		if (DBG)
			Log.d(TAG, "setRdsAf()--- rdsAfSwitch= " + rdsAf);
		
		if (mRdsAf != rdsAfSwitch) {
			// Set RDS-AF if a new choice is made by the user

			mStatus = sFmReceiver.setRdsAfSwitchMode(rdsAf);
			if (mStatus == false) {
				Log.e(TAG, "setRdsAf()-- setRdsAfSwitchMode(1) ->Erorr");
				showAlert(this, "FmRadio", "Cannot set RDS AF Mode ON!!!!");
			}
			mRdsAf = rdsAfSwitch;
		}

	}

	/* Load the Default values from the preference when the application starts */
	private void loadDefaultConfiguration() {

		SharedPreferences fmConfigPreferences = getSharedPreferences(
				"fmConfigPreferences", MODE_PRIVATE);
		/* Read the stations stored in text file and update the UI */
		readObject();
		if (DBG)
			Log.d(TAG, "stations" + stations);
		
		Iterator iterator = stations.iterator();

		while (iterator.hasNext()) {
			HashMap<String, String> item = (HashMap<String, String>) iterator
					.next();
			Object v = item.get(ITEM_VALUE);
			Object n = item.get(ITEM_NAME);
			Object i = item.get(ITEM_KEY);
			try {
				mIndex = Integer.parseInt(i.toString());
				updateSetStation((mIndex - 1), v.toString(), n.toString());
			} catch (Exception e) {
				Log.e(TAG, "Exception");
				e.printStackTrace();
			}

		}
		sBand = fmConfigPreferences.getInt(BAND, DEFAULT_BAND);
		lastTunedFrequency = fmConfigPreferences.getFloat(FREQUENCY,
				(sBand == FM_BAND_EUROPE_US ? DEFAULT_FREQ_EUROPE
						: DEFAULT_FREQ_JAPAN));

		if (DBG)
			Log.d(TAG, " Load default band " + sBand + "last fre" + lastTunedFrequency);

	}

	/* Save the Default values to the preference when the application exits */
	private void saveDefaultConfiguration() {

		SharedPreferences fmConfigPreferences = getSharedPreferences(
				"fmConfigPreferences", MODE_PRIVATE);
		SharedPreferences.Editor editor = fmConfigPreferences.edit();
		editor.putInt(BAND, sBand);
		editor.putFloat(FREQUENCY, lastTunedFrequency);
		if (DBG)
			Log.d(TAG, " save default band " + sBand + "last fre" + lastTunedFrequency);
		/* Write the stations stored to text file */
		writeObject();
		editor.commit();
	}

	/* Read the Stations from the text file in file system */
	void readObject() {
		String filename = "data/data/com.ti.fmapp/presets.txt";
		FileInputStream fis = null;
		ObjectInputStream in = null;
		Log.i(TAG, "readObject");

		try {
			fis = new FileInputStream(filename);
			in = new ObjectInputStream(fis);
			stations = (ArrayList<HashMap<String, String>>) in.readObject();
			in.close();
		} catch (IOException ex) {
			Log.e(TAG, "IOException");
			ex.printStackTrace();
		} catch (ClassNotFoundException ex) {
			Log.e(TAG, "ClassNotFoundException");
			ex.printStackTrace();
		}
	}

	/* Write the Stations to the text file in file system */
	void writeObject() {
		String filename = "data/data/com.ti.fmapp/presets.txt";
		FileOutputStream fos = null;
		ObjectOutputStream out = null;

		Log.i(TAG, "writeObject");
		try {
			fos = new FileOutputStream(filename);
			out = new ObjectOutputStream(fos);
			out.writeObject(stations);
			out.close();
		} catch (IOException ex) {
			Log.e(TAG, "IOException");
			ex.printStackTrace();
		}

	}

	/* Initialise all the widgets */
	private void initControls() {
		/**
		 * Start FM RX
		 */

		imgFmPower = (ImageView) findViewById(R.id.imgPower);
		imgFmPower.setOnClickListener(this);

		imgFmAudiopath = (ImageView) findViewById(R.id.imgAudiopath);
		imgFmAudiopath.setOnClickListener(this);

		imgFmMode = (ImageView) findViewById(R.id.imgMode);

		imgFmVolume = (ImageView) findViewById(R.id.imgMute);
		imgFmVolume.setOnClickListener(this);

		imgFmSeekUp = (ImageButton) findViewById(R.id.imgseekup);
		imgFmSeekUp.setOnClickListener(this);

		imgFmSeekDown = (ImageButton) findViewById(R.id.imgseekdown);
		imgFmSeekDown.setOnClickListener(this);

		txtFmRxTunedFreq = (TextView) findViewById(R.id.txtRxFreq);
		txtFmRxTunedFreq.setText(lastTunedFrequency.toString());

		txtStatusMsg = (TextView) findViewById(R.id.txtStatusMsg);
		txtRadioText = (TextView) findViewById(R.id.txtRadioText);
		txtPsText = (TextView) findViewById(R.id.txtPsText);

		txtPsText = (TextView) findViewById(R.id.txtPsText);

		txtStationName = (TextView) findViewById(R.id.txtStationName);
		txtStationName.setText(null);

		btnStation1 = (Button) findViewById(R.id.station1);
		btnStation1.setOnClickListener(this);
		btnStation1.setEnabled(false);

		btnStation2 = (Button) findViewById(R.id.station2);
		btnStation2.setOnClickListener(this);
		btnStation2.setEnabled(false);

		btnStation3 = (Button) findViewById(R.id.station3);
		btnStation3.setOnClickListener(this);
		btnStation3.setEnabled(false);

		btnStation4 = (Button) findViewById(R.id.station4);
		btnStation4.setOnClickListener(this);
		btnStation4.setEnabled(false);

		btnStation5 = (Button) findViewById(R.id.station5);
		btnStation5.setOnClickListener(this);
		btnStation5.setEnabled(false);

		btnStation6 = (Button) findViewById(R.id.station6);
		btnStation6.setOnClickListener(this);
		btnStation6.setEnabled(false);

		// Get the notification manager service.
		mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

	}

	/** Adds Delay of 2 seconds */
	private void insertDelayThread() {

		new Thread() {
			public void run() {
				try {
					// Add some delay to make sure all configuration has been
					// completed.
					sleep(1500);
				} catch (Exception e) {
					Log.e(TAG, "InsertDelayThread()-- Exception !!");
				}
				// Dismiss the Dialog
				configPd.dismiss();
			}
		}.start();

	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		switch (requestCode) {
		case (ACTIVITY_TUNE): {
			if (resultCode == Activity.RESULT_OK && data != null) {

				Bundle extras = data.getExtras();
				if (extras != null) {
					Integer freqSet = (int) extras.getFloat(FREQ_VALUE, 0);
					lastTunedFrequency = (float) freqSet / 1000;
					txtFmRxTunedFreq.setText(lastTunedFrequency.toString());
					mStatus = sFmReceiver.tune(freqSet);
					if (mStatus == false) {
						showAlert(this, "FmRadio", "Not able to tune!!!!");
					}
				}
			}
		}
			break;

		case (ACTIVITY_CONFIG): {
			if (resultCode == Activity.RESULT_OK) {
				if (DBG)
					Log.d(TAG, "ActivityFmRdsConfig configurationState "
							+ configurationState);
				setRdsConfig();
				configPd = ProgressDialog.show(this, "Please wait..",
						"Applying new Configuration", true, false);
				// The delay is inserted to make sure all the configurations
				// have been completed.
				insertDelayThread();

			}
		}
			break;

		}
	}

	public boolean onKeyDown(int keyCode, KeyEvent event) {

		switch (keyCode) {
		case KeyEvent.KEYCODE_DPAD_DOWN:

			return true;
		case KeyEvent.KEYCODE_DPAD_UP:

			return true;
		case KeyEvent.KEYCODE_BACK:
			/*
			 * Show a radio notification when the user presses back button and
			 * leaves FM app. The FM radio is still on
			 */
			this.showNotification(R.drawable.radio, R.string.app_rx,
					txtFmRxTunedFreq.getText(), false);
			saveDefaultConfiguration();
			finish();
			return true;
			/* Keys B to L are mapped to different get APIs for Testing */

		case KeyEvent.KEYCODE_B:
			Log.i(TAG, "Testing getTunedFrequency()  returned Tuned Freq = "
					+ sFmReceiver.getTunedFrequency());
			return true;

		case KeyEvent.KEYCODE_C:
			Log.i(TAG, "Testing getRssiThreshold()	returned RSSI thrshld = "
					+ sFmReceiver.getRssiThreshold());
			return true;

		case KeyEvent.KEYCODE_D:
			Log.i(TAG, "Testing getBand() returned Band  = "
					+ sFmReceiver.getBand());
			return true;

		case KeyEvent.KEYCODE_E:
			Log.i(TAG, "Testing getDeEmphasisFilter()	returned De-emp  = "
					+ sFmReceiver.getDeEmphasisFilter());
			return true;

		case KeyEvent.KEYCODE_F:
			Log.i(TAG, "Testing getMonoStereoMode() returned MonoStereo = "
					+ sFmReceiver.getMonoStereoMode());
			return true;

		case KeyEvent.KEYCODE_G:
			Log.i(TAG, "Testing getMuteMode()  returned MuteMode = "
					+ sFmReceiver.getMuteMode());
			return true;

		case KeyEvent.KEYCODE_H:
			Log.i(TAG,
					"Testing getRdsAfSwitchMode()	returned RdsAfSwitchMode = "
							+ sFmReceiver.getRdsAfSwitchMode());
			return true;

		case KeyEvent.KEYCODE_I:
			Log.i(TAG, "Testing getRdsGroupMask() returned RdsGrpMask = "
					+ sFmReceiver.getRdsGroupMask());
			return true;

		case KeyEvent.KEYCODE_J:
			Log.i(TAG, "Testing getRdsSystem() returned Rds System = "
					+ sFmReceiver.getRdsSystem());
			return true;

		case KeyEvent.KEYCODE_K:
			Log.i(TAG,
					"Testing getRfDependentMuteMode()	returned RfDepndtMuteMode = "
							+ sFmReceiver.getRfDependentMuteMode());
			return true;

		case KeyEvent.KEYCODE_L:

			LayoutInflater inflater = getLayoutInflater();
			View layout = inflater.inflate(R.layout.toast,
					(ViewGroup) findViewById(R.id.toast_layout));
			TextView text = (TextView) layout.findViewById(R.id.text);
			text.setText("The current Rssi     " + sFmReceiver.getRssi());

			Toast toast = new Toast(getApplicationContext());
			toast.setGravity(Gravity.BOTTOM | Gravity.CENTER_VERTICAL, 0, 0);
			toast.setDuration(Toast.LENGTH_LONG);
			toast.setView(layout);
			toast.show();

			return true;

		case KeyEvent.KEYCODE_M:
			Log.i(TAG, "Testing isValidChannel()	returned isValidChannel = "
					+ sFmReceiver.isValidChannel());

			return true;

		case KeyEvent.KEYCODE_N:
			Log.i(TAG, "Testing getFwVersion()	returned getFwVersion = "
					+ sFmReceiver.getFwVersion());

			return true;

		case KeyEvent.KEYCODE_O:
			Log.i(TAG,
					"Testing getChannelSpacing()	returned getChannelSpacing = "
							+ sFmReceiver.getChannelSpacing());

			return true;

		case KeyEvent.KEYCODE_P:
			Log.i(TAG, "Testing completescan()");
			sFmReceiver.completeScan();
			return true;

		case KeyEvent.KEYCODE_Q:
			Log.i(TAG,
					"Testing getCompleteScanProgress()	returned scan progress = "
							+ sFmReceiver.getCompleteScanProgress());

			return true;

		case KeyEvent.KEYCODE_R:
			Log.i(TAG, "Testing stopCompleteScan()	returned status = "
					+ sFmReceiver.stopCompleteScan());

			return true;

		case KeyEvent.KEYCODE_S:
			Log.i(TAG,
					"Testing setRfDependentMuteMode()	returned RfDepndtMuteMode = "
							+ sFmReceiver.setRfDependentMuteMode(1));
			return true;

		case KeyEvent.KEYCODE_T:
			Log.i(TAG, "Testing setRdsGroupMask() returned RdsGrpMask = "
					+ sFmReceiver.setRdsGroupMask(16));
			return true;
		case KeyEvent.KEYCODE_X:
			Log.i(TAG, "Testing setRssiThreshold() returned rssiThreshHold = "
					+ sFmReceiver.setRssiThreshold(128));

			return true;

		}

		return false;
	}

	/* Get the stored frequency from the arraylist and tune to that frequency */
	void tuneStationFrequency(String text) {
		try {
			float iFreq = Float.parseFloat(text);
			if (iFreq != 0) {
				lastTunedFrequency = (float) iFreq * 1000;
				if (DBG)
					Log.d(TAG, "lastTunedFrequency" + lastTunedFrequency);
				mStatus = sFmReceiver.tune(lastTunedFrequency.intValue());
				if (mStatus == false) {
					showAlert(getParent(), "FmRadio", "Not able to tune!!!!");
				}
				lastTunedFrequency = lastTunedFrequency / 1000;
			} else {

				new AlertDialog.Builder(this).setIcon(
						android.R.drawable.ic_dialog_alert).setMessage(
						"Enter valid frequency!!").setNegativeButton(
						android.R.string.ok, null).show();

			}
		} catch (NumberFormatException nfe) {
			Log.e(TAG, "nfe");
		}
	}

	public void onClick(View v) {
		int id = v.getId();

		switch (id) {
		case R.id.imgPower:

			/*
			 * The exit from the FM application happens here. FM will be
			 * disabled
			 */
			mStatus = sFmReceiver.disable();

			break;
		case R.id.imgMode:
			break;
		case R.id.imgAudiopath:
			// TODO
			break;
		case R.id.imgMute:
			mStatus = sFmReceiver.setMuteMode(mToggleMute);
			if (mStatus == false) {
				showAlert(this, "FmRadio", "Not able to setmute!!!!");
			} else {
				if (mToggleMute == FM_MUTE) {
					imgFmVolume.setImageResource(R.drawable.fm_volume_mute);
					mToggleMute = FM_UNMUTE;
				} else {
					imgFmVolume.setImageResource(R.drawable.fm_volume);
					mToggleMute = FM_MUTE;
				}
			}

			break;

		case R.id.imgseekdown:
			mDirection = FM_SEEK_DOWN;
			// FM seek down

			if (mSeekState == SEEK_REQ_STATE_IDLE) {
				txtStationName.setText(null); // set the station name to null
				mStatus = sFmReceiver.seek(mDirection);
				if (mStatus == false) {
					showAlert(this, "FmRadio", "Not able to seek down!!!!");
				} else {
					mSeekState = SEEK_REQ_STATE_PENDING;
					txtStatusMsg.setText(R.string.seeking);
				}

			}

			break;
		case R.id.imgseekup:
			mDirection = FM_SEEK_UP;
			// FM seek up
			if (mSeekState == SEEK_REQ_STATE_IDLE) {
				txtStationName.setText(null); // set the station name to null
				mStatus = sFmReceiver.seek(mDirection);
				if (mStatus == false) {
					showAlert(this, "FmRadio", "Not able to seek up!!!!");

				} else {
					mSeekState = SEEK_REQ_STATE_PENDING;
					txtStatusMsg.setText(R.string.seeking);
				}
			}

			break;
		case R.id.station1:
			mStationIndex = 0;
			updateStationDisplay(mStationIndex);

			break;
		case R.id.station2:
			mStationIndex = 1;
			updateStationDisplay(mStationIndex);

			break;
		case R.id.station3:
			mStationIndex = 2;
			updateStationDisplay(mStationIndex);

			break;
		case R.id.station4:
			mStationIndex = 3;
			updateStationDisplay(mStationIndex);
			break;

		case R.id.station5:
			mStationIndex = 4;
			updateStationDisplay(mStationIndex);

			break;
		case R.id.station6:
			mStationIndex = 5;
			updateStationDisplay(mStationIndex);
			break;
		}

	}

	/* Gets the stored frequency and tunes to it. */
	void updateStationDisplay(int index) {
		String tunedFreq = null;
		tunedFreq = GetStation(index);
		tuneStationFrequency(tunedFreq);
		txtFmRxTunedFreq.setText(tunedFreq.toString());

	}

	/* Creates the menu items */
	public boolean onCreateOptionsMenu(Menu menu) {

		super.onCreateOptionsMenu(menu);
		MenuItem item;

		item = menu.add(0, MENU_CONFIGURE, 0, R.string.configure);
		item.setIcon(R.drawable.configure);

		item = menu.add(0, MENU_ABOUT, 0, R.string.about);
		item.setIcon(R.drawable.fm_menu_help);

		item = menu.add(0, MENU_EXIT, 0, R.string.exit);
		item.setIcon(R.drawable.icon);

		item = menu.add(0, MENU_PRESET, 0, R.string.preset);
		item.setIcon(R.drawable.fm_menu_preferences);

		item = menu.add(0, MENU_SETFREQ, 0, R.string.setfreq);
		item.setIcon(R.drawable.fm_menu_manage);

		return true;
	}

	/* Handles item selections */
	public boolean onOptionsItemSelected(MenuItem item) {

		switch (item.getItemId()) {
		case MENU_CONFIGURE:
			/* Start the configuration window */
			Intent irds = new Intent(INTENT_RDS_CONFIG);
			startActivityForResult(irds, ACTIVITY_CONFIG);
			break;
		case MENU_PRESET:
			/* Start the Presets window */
			Intent i = new Intent(INTENT_PRESET);
			startActivity(i);
			break;
		case MENU_EXIT:
			/*
			 * The exit from the FM application happens here. FM will be
			 * disabled
			 */
			mStatus = sFmReceiver.disable();

			break;
		case MENU_ABOUT:
			/* Start the help window */
			Intent iTxHelp = new Intent(INTENT_RXHELP);
			startActivity(iTxHelp);
			break;

		case MENU_SETFREQ:
			/* Start the Manual frequency input window */
			Intent iRxTune = new Intent(INTENT_RXTUNE);
			startActivityForResult(iRxTune, ACTIVITY_TUNE);
			break;

		}
		return super.onOptionsItemSelected(item);
	}

	protected void onSaveInstanceState(Bundle icicle) {
		super.onSaveInstanceState(icicle);
		/* save the fm state into bundle for the activity restart */
		mFmInterrupted = true;
		Bundle fmState = new Bundle();
		fmState.putBoolean(FM_INTERRUPTED_KEY, mFmInterrupted);

		icicle.putBundle(FM_STATE_KEY, fmState);
	}

	public void onStart() {
		super.onStart();
	}

	public void onPause() {
		super.onPause();

		if (pd != null)
			pd.dismiss();

	}

	public void onConfigurationChanged(Configuration newConfig) {
		Log.i(TAG, "onConfigurationChanged");
		super.onConfigurationChanged(newConfig);

	}

	public void onResume() {
		Log.i(TAG, "onResume");
		super.onResume();
		startup();
	}

	public void onDestroy() {
		Log.i(TAG, "onDestroy");
		super.onDestroy();
		/*
		 * Unregistering the receiver , so that we dont handle any FM events
		 * when out of the FM application screen
		 */
		unregisterReceiver(mReceiver);
		sFmReceiver.close();
	}

	// Receives all of the FM intents and dispatches to the proper handler

	private final BroadcastReceiver mReceiver = new BroadcastReceiver() {

		public void onReceive(Context context, Intent intent) {

			String fmAction = intent.getAction();

			if (fmAction.equals(FmReceiverIntent.FM_ENABLED_ACTION)) {

				mHandler.sendMessage(mHandler
						.obtainMessage(EVENT_FM_ENABLED, 0));
			}
			if (fmAction.equals(FmReceiverIntent.FM_DISABLED_ACTION)) {
				mHandler.sendMessage(mHandler.obtainMessage(EVENT_FM_DISABLED,
						0));
			}

			if (fmAction
					.equals(FmReceiverIntent.DISPLAY_MODE_MONO_STEREO_ACTION)) {
		
				Integer modeDisplay = intent.getIntExtra(
						FmReceiverIntent.MODE_MONO_STEREO, 0);
				mHandler.sendMessage(mHandler.obtainMessage(
						EVENT_MONO_STEREO_DISPLAY, modeDisplay));
			}

			if (fmAction.equals(FmReceiverIntent.RDS_TEXT_CHANGED_ACTION)) {
				if (FM_SEND_RDS_IN_BYTEARRAY == true) {
					Bundle extras = intent.getExtras();

					byte[] rdsText = extras.getByteArray(FmReceiverIntent.RDS);
					int status = extras.getInt(FmReceiverIntent.STATUS, 0);

					mHandler.sendMessage(mHandler.obtainMessage(EVENT_RDS_TEXT,
							status, 0, rdsText));
				} else {
					String rdstext = intent
							.getStringExtra(FmReceiverIntent.RADIOTEXT_CONVERTED);
					mHandler.sendMessage(mHandler.obtainMessage(EVENT_RDS_TEXT,
							rdstext));
				}
			}
			if (fmAction.equals(FmReceiverIntent.PI_CODE_CHANGED_ACTION)) {

				Integer pi = intent.getIntExtra(FmReceiverIntent.PI, 0);

				mHandler.sendMessage(mHandler.obtainMessage(EVENT_PI_CODE, pi
						.toString()));
			}

			if (fmAction.equals(FmReceiverIntent.TUNE_COMPLETE_ACTION)) {

				int tuneFreq = intent.getIntExtra(
						FmReceiverIntent.TUNED_FREQUENCY, 0);

				mHandler.sendMessage(mHandler.obtainMessage(
						EVENT_TUNE_COMPLETE, tuneFreq));

			}

			if (fmAction.equals(FmReceiverIntent.SEEK_ACTION)) {
				int freq = intent.getIntExtra(FmReceiverIntent.SEEK_FREQUENCY,
						0);

				mHandler.sendMessage(mHandler.obtainMessage(EVENT_SEEK_STARTED,
						freq));
			}

			if (fmAction.equals(FmReceiverIntent.PS_CHANGED_ACTION)) {
	
				if (FM_SEND_RDS_IN_BYTEARRAY == true) {
					Bundle extras = intent.getExtras();
					byte[] psName = extras.getByteArray(FmReceiverIntent.PS);
					int status = extras.getInt(FmReceiverIntent.STATUS, 0);

					mHandler.sendMessage(mHandler.obtainMessage(
							EVENT_PS_CHANGED, status, 0, psName));
				} else {

					String name = intent
							.getStringExtra(FmReceiverIntent.PS_CONVERTED);

					mHandler.sendMessage(mHandler.obtainMessage(
							EVENT_PS_CHANGED, name));
				}
			}

			if (fmAction.equals(FmReceiverIntent.COMPLETE_SCAN_DONE_ACTION)) {

				Bundle extras = intent.getExtras();

				int[] channelList = extras
						.getIntArray(FmReceiverIntent.SCAN_LIST);

				int noOfChannels = extras.getInt(
						FmReceiverIntent.SCAN_LIST_COUNT, 0);

				int status = extras.getInt(FmReceiverIntent.STATUS, 0);

				for (int i = 0; i < noOfChannels; i++)

					Log.i(TAG, "channelList" + channelList[i]);

				mHandler.sendMessage(mHandler.obtainMessage(
						EVENT_COMPLETE_SCAN_DONE, status, noOfChannels,
						channelList));
			}

			if (fmAction.equals(FmReceiverIntent.FM_ERROR_ACTION)) {
				mHandler.sendMessage(mHandler.obtainMessage(EVENT_FM_ERROR, 0));
			}

		}
	};



}
