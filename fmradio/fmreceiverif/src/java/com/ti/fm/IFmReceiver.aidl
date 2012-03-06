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
*   FILE NAME:      IFmReceiver.aidl
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

/**
 * System private API for FM Receiver service
 *
 * {@hide}
 */
interface IFmReceiver {
    boolean create();
    boolean enable();
    boolean disable();
    boolean isEnabled() ;
    int getFMState();
    boolean setBand(int band);
    int getBand();
    boolean setChannelSpacing(int channelSpace);
    int getChannelSpacing();
    boolean setMonoStereoMode(int mode);
    int getMonoStereoMode();
    boolean setMuteMode(int muteMode);
    int getMuteMode();
    boolean setRfDependentMuteMode(int rfMuteMode);
    int getRfDependentMuteMode();
    boolean setRssiThreshold(int threshhold);
    int getRssiThreshold();
    boolean setDeEmphasisFilter(int filter);
    int getDeEmphasisFilter();
    boolean tune(int freq);
    int getTunedFrequency();
    boolean seek(int direction);
    boolean stopSeek();
    int getRssi();
    int getRdsSystem();
    boolean setRdsSystem(int system);
    boolean enableRds();
    boolean disableRds();
    boolean setRdsGroupMask(int mask);
    long getRdsGroupMask();
    boolean setRdsAfSwitchMode(int mode);
    int getRdsAfSwitchMode();
    boolean changeAudioTarget(int mask,int digitalConfig);
    boolean changeDigitalTargetConfiguration(int digitalConfig);
    boolean enableAudioRouting();
    boolean disableAudioRouting();    
    boolean destroy();
    boolean isValidChannel();
    boolean completeScan();  
    double getFwVersion();        
    int getCompleteScanProgress();  
    int stopCompleteScan();

}
