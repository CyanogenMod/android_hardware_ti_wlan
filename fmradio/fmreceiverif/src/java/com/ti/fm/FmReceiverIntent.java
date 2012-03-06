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
 *   FILE NAME:      FmReceiverIntent.java
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

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;

/**
 * Manages the FM Reception
 * 
 * @hide
 */
public interface FmReceiverIntent {

	public static final String MODE_MONO_STEREO = "com.ti.fm.rx.intent.MODE_MONO_STEREO";
	public static final String TUNED_FREQUENCY = "com.ti.fm.rx.intent.TUNED_FREQUENCY";
	public static final String SEEK_FREQUENCY = "com.ti.fm.rx.intent.SEEK_FREQUENCY";
	public static final String RDS = "com.ti.fm.rx.intent.RDS";
	public static final String PS = "com.ti.fm.rx.intent.PS";
	public static final String PI = "com.ti.fm.rx.intent.PI";
	public static final String REPERTOIRE = "com.ti.fm.rx.intent.REPERTOIRE";
	public static final String MUTE = "com.ti.fm.rx.intent.MUTE";
	public static final String STATUS = "com.ti.fm.rx.intent.STATUS";
	public static final String SCAN_LIST = "com.ti.fm.rx.intent.SCAN_LIST";
	public static final String SCAN_LIST_COUNT = "com.ti.fm.rx.intent.SCAN_LIST_COUNT";
	public static final String RADIOTEXT_CONVERTED = "com.ti.fm.rx.intent.RADIOTEXT_CONVERTED_VALUE";
	public static final String PS_CONVERTED = "com.ti.fm.rx.intent.PS_CONVERTED_VALUE";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String FM_ENABLED_ACTION = "com.ti.fm.rx.intent.action.FM_ENABLED";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String FM_DISABLED_ACTION = "com.ti.fm.rx.intent.action.FM_DISABLED";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String SEEK_ACTION = "com.ti.fm.rx.intent.action.SEEK_ACTION";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String RDS_TEXT_CHANGED_ACTION = "com.ti.fm.rx.intent.action.RDS_TEXT_CHANGED";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String PS_CHANGED_ACTION = "com.ti.fm.rx.intent.action.PS_CHANGED";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String TUNE_COMPLETE_ACTION = "com.ti.fm.rx.intent.action.TUNE_COMPLETE_ACTION";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String DISPLAY_MODE_MONO_STEREO_ACTION = "com.ti.fm.rx.intent.action.DISPLAY_MODE_MONO_STEREO_ACTION";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String PI_CODE_CHANGED_ACTION = "com.ti.fm.rx.intent.action.PI_CODE_CHANGED_ACTION";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String COMPLETE_SCAN_DONE_ACTION = "com.ti.fm.rx.intent.action.COMPLETE_SCAN_DONE_ACTION";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	@Deprecated
	public static final String FM_SUSPENDED_ACTION = "com.ti.fm.rx.intent.action.FM_SUSPENDED";
	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	@Deprecated
	public static final String FM_RESUMED_ACTION = "com.ti.fm.rx.intent.action.FM_RESUMED";

	@SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
	public static final String FM_ERROR_ACTION = "com.ti.fm.rx.intent.action.FM_ERROR_ACTION";

}
