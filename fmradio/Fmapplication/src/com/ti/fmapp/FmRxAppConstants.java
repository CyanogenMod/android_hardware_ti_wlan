/*
 * TI's FM
 *
 * Copyright 2001-2010 Texas Instruments, Inc. - http://www.ti.com/
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
 *   FILE NAME:      FmRxAppConstants.java
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

public interface FmRxAppConstants {
	/********************************************
	 * Message Code
	 ********************************************/
	public static final int EVENT_FM_ENABLED = 1;
	public static final int EVENT_FM_DISABLED = 2;
	public static final int EVENT_MONO_STEREO_CHANGE = 3;
	public static final int EVENT_SEEK_STARTED = 4;
	public static final int EVENT_VOLUME_CHANGE = 5;
	public static final int EVENT_TUNE_COMPLETE = 6;
	public static final int EVENT_MUTE_CHANGE = 7;
	public static final int EVENT_SEEK_STOPPED = 8;
	public static final int EVENT_RDS_TEXT = 9;
	public static final int EVENT_BAND_CHANGE = 10;
	public static final int EVENT_MONO_STEREO_DISPLAY = 11;
	public static final int EVENT_FM_RDSCONFIG = 12;
	public static final int EVENT_ENABLE_RDS = 13;
	public static final int EVENT_SET_RDS_SYSTEM = 14;
	public static final int EVENT_SET_RDS_AF = 15;
	public static final int EVENT_DISABLE_RDS = 16;
	public static final int EVENT_SET_DEEMP_FILTER = 17;
	public static final int EVENT_PS_CHANGED = 18;
	public static final int EVENT_SET_RSSI_THRESHHOLD = 19;
	public static final int EVENT_SET_RF_DEPENDENT_MUTE = 20;
	public static final int EVENT_MASTER_VOLUME_CHANGED = 21;
	public static final int EVENT_FM_PAUSE = 22;
	public static final int EVENT_FM_RESUME = 23;
	public static final int EVENT_SET_CHANNELSPACE = 24;
	public static final int EVENT_COMPLETE_SCAN_DONE = 25;
	public static final int EVENT_COMPLETE_SCAN_STOP = 26;
	public static final int EVENT_PI_CODE = 27;
	public static final int EVENT_FM_ERROR = 28;
	/* Volume range */

	public static final int MIN_VOLUME = 0;
	public static final int MAX_VOLUME = 70;
	public static final int GAIN_STEP = 1;

	/* default values */
	public static final int DEF_VOLUME = 10;
	public static final float DEFAULT_FREQ_EUROPE = (float) 87500 / 1000;
	public static final float DEFAULT_FREQ_JAPAN = (float) 76000 / 1000;
	public static final int DEFAULT_BAND = 0; // EuropeUS
	public static final int DEFAULT_MODE = 0; // Stereo
	public static final boolean DEFAULT_RDS = true;
	public static final int DEFAULT_DEEMP = 0;
	public static final int DEFAULT_RDS_SYSTEM = 0;
	public static final boolean DEFAULT_RDS_AF = false;
	public static final int DEFAULT_RSSI = 7;
	public static final int DEFAULT_CHANNELSPACE = 2;

	/* Actvity result index */

	public static final int ACTIVITY_TUNE = 1;
	public static final int ACTIVITY_CONFIG = 2;
	public static final String FREQ_VALUE = "FREQUENCY";

	/* Rssi range */

	public static final int RSSI_MIN = 1;
	public static final int RSSI_MAX = 127;

	/* Preset list display items */

	public static final String ITEM_KEY = "key";
	public static final String ITEM_VALUE = "value";
	public static final String ITEM_NAME = "name";

	/* seek states */
	public static final boolean SEEK_REQ_STATE_IDLE = true;
	public static final boolean SEEK_REQ_STATE_PENDING = false;

	/* Preference save keys */

	public static final String BAND = "BAND";
	public static final String VOLUME = "VOLUME";
	public static final String FREQUENCY = "FREQUENCY";
	public static final String MODE = "MODE";
	public static final String RDS = "RDS";
	public static final String RDSSYSTEM = "RDSSYSTEM";
	public static final String DEEMP = "DEEMP";
	public static final String RDSAF = "RDSAF";
	public static final String RSSI = "RSSI";
	public static final String RSSI_STRING = "RSSI_STRING";
	public static final String DEF_RSSI_STRING = "7";
	public static final String CHANNELSPACE = "CHANNELSPACE";

	/* Configuration states */

	public static final int CONFIGURATION_STATE_IDLE = 1;
	public static final int CONFIGURATION_STATE_PENDING = 2;

	/* Initial values */
	public static final int INITIAL_VAL = 5;
	public static final int INITIAL_RSSI = 0;

	/* Seek constants */

	public static final int FM_SEEK_UP = 1;
	public static final int FM_SEEK_DOWN = 0;

	/* Mute constants */

	public static final int FM_MUTE = 0;
	public static final int FM_UNMUTE = 1;
	public static final int FM_ATT = 2;

	/* Activity Intenets */
	public static final String INTENT_RDS_CONFIG = "android.intent.action.RDS_CONFIG";
	public static final String INTENT_PRESET = "android.intent.action.PRESET";
	public static final String INTENT_RXHELP = "android.intent.action.START_RXHELP";
	public static final String INTENT_RXTUNE = "android.intent.action.START_RXFREQ";

}
