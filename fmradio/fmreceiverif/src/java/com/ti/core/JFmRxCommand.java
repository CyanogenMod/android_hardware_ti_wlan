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
 *   FILE NAME:      JFmRxCommand.java
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

public class JFmRxCommand {

	public static final int CMD_ENABLE = 0; /* Enable command */
	public static final int CMD_DISABLE = 1; /* Disable command */
	public static final int CMD_SET_BAND = 2;/* Set Band command */
	public static final int CMD_GET_BAND = 3; /* Get Band command */
	public static final int CMD_SET_MONO_STEREO_MODE = 4; /*
														 * Set Mono/Stereo
														 * command
														 */
	public static final int CMD_GET_MONO_STEREO_MODE = 5;/*
														 * Get Mono/Stereo
														 * command
														 */
	public static final int CMD_SET_MUTE_MODE = 6;/* Set Mute mode command */
	public static final int CMD_GET_MUTE_MODE = 7;/* Get Mute mode command */
	public static final int CMD_SET_RF_DEPENDENT_MUTE_MODE = 8;/*
																 * Set
																 * RF-Dependent
																 * Mute Mode
																 * command
																 */
	public static final int CMD_GET_RF_DEPENDENT_MUTE_MODE = 9;/*
																 * Get
																 * RF-Dependent
																 * Mute Mode
																 * command
																 */
	public static final int CMD_SET_RSSI_THRESHOLD = 10; /*
														 * Set RSSI Threshold
														 * command
														 */
	public static final int CMD_GET_RSSI_THRESHOLD = 11; /*
														 * Get RSSI Threshold
														 * command
														 */
	public static final int CMD_SET_DEEMPHASIS_FILTER = 12; /*
															 * Set De-Emphassi
															 * Filter command
															 */
	public static final int CMD_GET_DEEMPHASIS_FILTER = 13; /*
															 * Get De-Emphassi
															 * Filter command
															 */
	public static final int CMD_SET_VOLUME = 14; /* Set Volume command */
	public static final int CMD_GET_VOLUME = 15; /* Get Volume command */
	public static final int CMD_TUNE = 16; /* Tune command */
	public static final int CMD_GET_TUNED_FREQUENCY = 17; /*
														 * Get Tuned Frequency
														 * command
														 */
	public static final int CMD_SEEK = 18; /* Seek command */
	public static final int CMD_STOP_SEEK = 19; /* Stop Seek command */
	public static final int CMD_GET_RSSI = 20; /* Get RSSI command */
	public static final int CMD_ENABLE_RDS = 21; /* Enable RDS command */
	public static final int CMD_DISABLE_RDS = 22; /* Disable RDS command */
	public static final int CMD_SET_RDS_SYSTEM = 23; /* Set RDS System command */
	public static final int CMD_GET_RDS_SYSTEM = 24; /* Get RDS System command */
	public static final int CMD_SET_RDS_GROUP_MASK = 25; /*
														 * Set RDS groups to be
														 * recieved
														 */
	public static final int CMD_GET_RDS_GROUP_MASK = 26; /*
														 * Get RDS groups to be
														 * recieved
														 */
	public static final int CMD_SET_RDS_AF_SWITCH_MODE = 27; /*
															 * Set AF Switch
															 * Mode command
															 */
	public static final int CMD_GET_RDS_AF_SWITCH_MODE = 28; /*
															 * Get AF Switch
															 * Mode command
															 */

	public static final int CMD_ENABLE_AUDIO = 29; /* Set Audio Routing command */
	public static final int CMD_DISABLE_AUDIO = 30; /* Get Audio Routing command */
	public static final int CMD_DESTROY = 31; /* Destroy command */

	public static final int CMD_CHANGE_AUDIO_TARGET = 32; /*
														 * Change the audio
														 * target
														 */
	public static final int CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION = 33; /*
																		 * Change
																		 * the
																		 * digital
																		 * target
																		 * configuration
																		 */

	public static final int INIT_ASYNC = 34;
	public static final int CMD_INIT = 35;
	public static final int CMD_DEINIT = 36;
	public static final int CMD_SET_CHANNEL_SPACING = 37;
	public static final int CMD_GET_CHANNEL_SPACING = 38;
	public static final int CMD_GET_FW_VERSION = 39;
	public static final int CMD_IS_CHANNEL_VALID = 40;
	public static final int CMD_COMPLETE_SCAN = 41;
	public static final int CMD_COMPLETE_SCAN_PROGRESS = 42;
	public static final int CMD_STOP_COMPLETE_SCAN = 43;

	public static final int LAST_API_CMD = 43;
	public static final int CMD_NONE = 255;

	private final int value;

	private JFmRxCommand(int value) {
		this.value = value;
	}

	public Integer getValue() {
		return value;
	}
}
