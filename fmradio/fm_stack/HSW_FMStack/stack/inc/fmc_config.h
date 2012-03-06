/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
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
/*******************************************************************************\
*
*   FILE NAME:      fmc_config.h
*
*   BRIEF:          FM Configuration Parameters
*
*   DESCRIPTION:
*
*     The constants in this file configure the FM for a specific platform and project.
*   To change a constant, simply change its value in this file and recompile the entire FM Stack.
*
*     Some constants are numeric, and others indicate whether a feature
*     is enabled. 
*
*   The values in this specific file are tailored for a Windows distribution. To change a constant, 
*   simply change its value in this file and recompile the entire FM Stack.
*
*   AUTHOR:   Udi Ron
*
\******************************************************************************/

#ifndef __FMC_CONFIG_H
#define __FMC_CONFIG_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_config.h"

/********************************************************************************
 *
 * Constants
 *
 *******************************************************************************/
/*
*   Indicates that a feature is enabled
*/
#define FMC_CONFIG_ENABLED                                          (1)

/*
*   Indicates that a feature is disabled
*/
#define FMC_CONFIG_DISABLED                                         (0)

/* 
 * Defines whether the FM stcak is enabled or disabled
 */
#define FMC_CONFIG_FM_STACK                                         FMC_CONFIG_ENABLED


/*-------------------------------------------------------------------------------
 * Common
 *
 *     Represents common configuration parameters.
 */
/*
*   Defines the full path to the location (in the file system) of the init script files of the FM.
*/
#define FMC_CONFIG_SCRIPT_FILES_FULL_PATH_LOCATION                              (MCP_HAL_CONFIG_FS_SCRIPT_FOLDER)


#define FMC_CONFIG_SCRIPT_FILES_FMC_INIT_NAME                                           ("fmc_init")

#define FMC_CONFIG_SCRIPT_FILES_FM_RX_INIT_NAME                                             ("fm_rx_init")

#define FMC_CONFIG_SCRIPT_FILES_FM_TX_INIT_NAME                                             ("fm_tx_init")

/*
*   Defines the name of the single FM mutex that is shared between RX & TX
*/

#define FMC_CONFIG_FM_MUTEX_NAME                                ("FMC_SEM")


/*
*   Defines the name of the single FM task that is shared between RX & TX
*/
#define FMC_CONFIG_FM_TASK_NAME                                 ("FMC")

/*
    Defines the maximum number of commands that may be pedning their execution in
    the RX or TX queue

    [ToDo]: Specify the memory size per unit
*/
#define FMC_CONFIG_MAX_NUM_OF_PENDING_CMDS                  (20)

/*
*   Bit 0: INTx polarity: 0 = low, 1 = high
    Bit 1: When polarity high: 0 = high, 1 = Hi-Z
*/
#define FMC_CONFIG_CMN_INTX_CONFIG                  (0)
/*-------------------------------------------------------------------------------
 * FM RX
 *
 *     Represents configuration parameters for FMS module.
 */

/*
*   define the time from the moment the FM stack switches to the AF before it allows another switch. This is necessary to avoid
*   excessive changes between the primary and alternate frequencies.
*/
#define FM_CONFIG_RX_AF_TIMER_MS                                (30000)

/*
*   Must wait at least 20msec before starting to send commands to the FM.
*/
#define FMC_CONFIG_WAKEUP_TIMEOUT_MS                     (40)

/*
*   
*   Set mono or stereo mode. 
*   If stereo mode is set, but no stereo is detected then mono mode will be chosen.
*/
#define FMC_CONFIG_RX_MOST_MODE                 (0)
/*
*   Set stereo blending mode
*/
#define FMC_CONFIG_RX_MOST_BLEND                    (1)
/*
*   
*   Choose de-emphasis filter.
*   The FM transmitting station uses a pre-emphasis filter to provide better S/N ratio, 
*   and the receiver uses a de-emphasis filter corresponding to the transmitter
*   to restore the original audio.  This varies per country.  
*   Therefore the host has to know the country it is in and set the BRF6350 for the correct de-emphasis.
*/
#if 0  // Duplicate definition. To be Removed
#define FMC_CONFIG_RX_DEMPH_MODE                    (FM_RX_EMPHASIS_FILTER_50_USEC)
#endif
#define FMC_CONFIG_RX_DEMPH_MODE                    (FMC_EMPHASIS_FILTER_50_USEC)
/*
*   Set the signal strength level that once reached will stop the auto search process
*/
#ifdef SEMC_CONFIG_RX_SEARCH_LVL
#define FMC_CONFIG_RX_SEARCH_LVL                    SEMC_CONFIG_RX_SEARCH_LVL
#else
#define FMC_CONFIG_RX_SEARCH_LVL                    (7)
#endif
/*
*   Selects Europe/US band or Japan.
*/
#define FMC_CONFIG_RX_BAND                  (FMC_BAND_EUROPE_US)
/*
*   Set mute mode.
*/
#define FMC_CONFIG_RX_MUTE_STATUS                   (FMC_FW_RX_MUTE_UNMUTE_MODE)
/*
*   This command sets the number of RDS tuples  that will be stored before an interrupt to the host is set.
*/
#define FMC_CONFIG_RX_RDS_MEM                   (FMC_FW_RX_RDS_THRESHOLD)
/*
*   Set PI mask. 
*/
#define FMC_CONFIG_RX_RDS_PI_MASK                   (0)
/*
*   This value is compared to the PI code when performing PI match.
*/
#define FMC_CONFIG_RX_RDS_PI                    (0)
/*
*   Set RDS operation modes.
*/
#define FMC_CONFIG_RX_RDS_SYSTEM                    (FM_RDS_SYSTEM_RDS)
/*
*   
*   Set gain level of the audio left & right channels. 
*   Default value is set to produce -1 dB of the audio full scale when modulating 1 KHz tone at ?F = 105 KHz.
*/
#define FMC_CONFIG_RX_VOLUME                    (0x78b8)
/*
*   Enable/disable I2S module and analog audio. 
*   The outputs are also enabled/disabled according to this setting.  
*   This command should be given after the I2S_MODE_CONFIG_SET 
*/
#define FMC_CONFIG_RX_AUDIO_ENABLE                  (FMC_FW_RX_FM_AUDIO_ENABLE_I2S)

/*-------------------------------------------------------------------------------
 * FM TX Constants
 *
 *     Represents configuration parameters for FMS module.
 */

/*
    The default transmitted RDS PI code.

    Currently the default is set to:
    
    Country Code (bits 12-15):                          UK (0xC)
    
    Program type in terms of area coverage (bits 8-11):     Local (Local program transmitted via a 
                                                            single transmitter only during 
                                                            the whole transmitting time).

    Program reference number (bits 0-7):                    Not Assigned
*/
#define FMC_CONFIG_TX_DEFAULT_RDS_PI_CODE                   ((FmcRdsPiCode)0x0000)

/*
    The default transmitted RDS group type (PS or/and RT )
*/
#define FMC_CONFIG_TX_DEFAULT_RDS_TRANSMITTED_FIELDS_MASK   (FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK)

/*
    The default transmitted PS text.
*/
#define FMC_CONFIG_TX_DEFAULT_RDS_PS_MSG                    ("TX DEMO")

/*
    The default transmitted RT Type (A or B).
*/
#define FMC_CONFIG_TX_DEFAULT_RDS_RT_TYPE                   (FMC_RDS_RT_MSG_TYPE_AUTO)

/*
    The default transmitted RT text.
*/
#define FMC_CONFIG_TX_DEFAULT_RDS_RT_MSG                    ("Default Radio Text")

#define FMC_CONFIG_TX_DEFUALT_POWER_LEVEL                   (4)

/**/
#define FMC_CONFIG_TX_DEFUALT_REPERTOIRE_CODE_TABLE     (FMC_RDS_REPERTOIRE_G0_CODE_TABLE)

/**/
#define FMC_CONFIG_TX_DEFUALT_STEREO_MONO_MODE          (FM_TX_STEREO_MODE)

/**/
#define FMC_CONFIG_TX_DEFUALT_MUSIC_SPEECH_FLAG         (FMC_RDS_MUSIC)

/**/
#define FMC_CONFIG_TX_DEFUALT_PRE_EMPHASIS_FILTER           (FMC_EMPHASIS_FILTER_50_USEC)

/**/
#define FMC_CONFIG_TX_DEFUALT_MUTE_MODE         (FMC_NOT_MUTE)

/**/
#define FMC_CONFIG_TX_DEFUALT_RDS_TEXT_SCROLL_MODE          (FMC_RDS_PS_DISPLAY_MODE_STATIC)

/**/
#define FMC_CONFIG_TX_DEFUALT_RDS_PS_DISPLAY_SPEED          (FMC_RDS_SCROLL_SPEED_DEFUALT)



#endif /* __FMC_CONFIG_H */

