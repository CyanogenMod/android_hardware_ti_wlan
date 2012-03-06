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
 *   FILE NAME:      FmRxService.java
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

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import com.ti.fm.IFmReceiver;
import com.ti.server.StubFmRxService;

public final class FmRxService extends Service {

	/** TAG for log. */
	private static final String TAG = "FmRxService";

	/**
	 * IBinder of this service.
	 */
	private final IFmReceiver.Stub mBinder = new StubFmRxService();

	/**
	 * Called by the system when the service is first created. Do not call this
	 * method directly.
	 */
	@Override
	public void onCreate() {
		super.onCreate();
		((StubFmRxService) mBinder).init(this);
	}

	/**
	 * Called by the system to notify a Service that it is no longer used and is
	 * being removed. The service should clean up an resources it holds (threads
	 * registered receivers, etc) at this point. Upon return, there will be no
	 * more calls in to this Service object and it is effectively dead.
	 */
	@Override
	public void onDestroy() {
		super.onDestroy();
	}

	/**
	 * If this service is binded, this method is called.
	 * 
	 * @param arg
	 *            Intent used when service is called.
	 * @return IBinder.
	 */
	@Override
	public IBinder onBind(final Intent arg) {
		return mBinder;
	}
}
