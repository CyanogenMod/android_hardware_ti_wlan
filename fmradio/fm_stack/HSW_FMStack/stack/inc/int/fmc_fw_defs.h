/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
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


#ifndef __FMC_FW_DEFS_H
#define __FMC_FW_DEFS_H

#include "fmc_types.h"

/*
	2 Bytes API operation opcode
*/
typedef FMC_U8 	FmcFwOpcode;

/*
	2 bytes value for read / write API operations 
*/
typedef FMC_U16 FmcFwCmdParmsValue;

typedef FMC_U16 FmcFwIntMask;

/* [ToDo] Add all available opcodes and values */

/***********************************************************
	Common FM commands opcodes
************************************************************/
#define FMC_FW_OPCODE_CMN_FLAG_GET						((FmcFwOpcode)0x03)
#define FMC_FW_OPCODE_CMN_INT_MASK_SET_GET				((FmcFwOpcode)0x1a)
#define FMC_FW_OPCODE_CMN_I2S_CLOCK_CONFIG_SET_GET	((FmcFwOpcode)0x1e)
#define FMC_FW_OPCODE_CMN_I2S_MODE_CONFIG_SET_GET	((FmcFwOpcode)0x1f)
#define FMC_FW_OPCODE_CMN_INTX_CONFIG_SET_GET			((FmcFwOpcode)0x21)
#define FMC_FW_OPCODE_CMN_PULL_EN_SET_GET				((FmcFwOpcode)0x22)
#define FMC_FW_OPCODE_CMN_SWITCH_2_FREF_SET			((FmcFwOpcode)0x24)
#define FMC_FW_OPCODE_CMN_FREQ_DRIFT_REPORT_SET		((FmcFwOpcode)0x24)
#define FMC_FW_OPCODE_CMN_PCE_GET						((FmcFwOpcode)0x28)
#define FMC_FW_OPCODE_CMN_FIRM_VER_GET					((FmcFwOpcode)0x29)
#define FMC_FW_OPCODE_CMN_ASIC_VER_GET					((FmcFwOpcode)0x2a)
#define FMC_FW_OPCODE_CMN_ASIC_ID_GET					((FmcFwOpcode)0x2b)
#define FMC_FW_OPCODE_CMN_MAN_ID_GET					((FmcFwOpcode)0x2c)
#define FMC_FW_OPCODE_CMN_HARDWARE_REG_SET_GET		((FmcFwOpcode)0x64)
#define FMC_FW_OPCODE_CMN_CODE_DOWNLOAD				((FmcFwOpcode)0x65)
#define FMC_FW_OPCODE_CMN_RESET							((FmcFwOpcode)0x66)

/***********************************************************
	FM RX commands opcodes
************************************************************/
#define FMC_FW_OPCODE_RX_STEREO_GET					((FmcFwOpcode)0x00)
#define FMC_FW_OPCODE_RX_RSSI_LEVEL_GET				((FmcFwOpcode)0x01)
#define FMC_FW_OPCODE_RX_IF_COUNT_GET				((FmcFwOpcode)0x02)
#define FMC_FW_OPCODE_RX_RDS_SYNC_GET				((FmcFwOpcode)0x04)
#define FMC_FW_OPCODE_RX_RDS_DATA_GET				((FmcFwOpcode)0x05)
#define FMC_FW_OPCODE_RX_FREQ_SET_GET				((FmcFwOpcode)0x0a)
#define FMC_FW_OPCODE_RX_AF_FREQ_SET_GET			((FmcFwOpcode)0x0b)
#define FMC_FW_OPCODE_RX_MOST_MODE_SET_GET		((FmcFwOpcode)0x0c)
#define FMC_FW_OPCODE_RX_MOST_BLEND_SET_GET		((FmcFwOpcode)0x0d)
#define FMC_FW_OPCODE_RX_DEMPH_MODE_SET_GET		((FmcFwOpcode)0x0e)
#define FMC_FW_OPCODE_RX_SEARCH_LVL_SET_GET		((FmcFwOpcode)0x0f)
#define FMC_FW_OPCODE_RX_BAND_SET_GET				((FmcFwOpcode)0x10)
#define FMC_FW_OPCODE_RX_MUTE_STATUS_SET_GET		((FmcFwOpcode)0x11)
#define FMC_FW_OPCODE_RX_RDS_PAUSE_LVL_SET_GET		((FmcFwOpcode)0x12)
#define FMC_FW_OPCODE_RX_RDS_PAUSE_DUR_SET_GET	((FmcFwOpcode)0x13)
#define FMC_FW_OPCODE_RX_RDS_MEM_SET_GET			((FmcFwOpcode)0x14)
#define FMC_FW_OPCODE_RX_RDS_BLK_B_SET_GET			((FmcFwOpcode)0x15)
#define FMC_FW_OPCODE_RX_RDS_MSK_B_SET_GET			((FmcFwOpcode)0x16)
#define FMC_FW_OPCODE_RX_RDS_PI_MASK_SET_GET		((FmcFwOpcode)0x17)
#define FMC_FW_OPCODE_RX_RDS_PI_SET_GET				((FmcFwOpcode)0x18)
#define FMC_FW_OPCODE_RX_RDS_SYSTEM_SET_GET		((FmcFwOpcode)0x19)
#define FMC_FW_OPCODE_RX_SEARCH_DIR_SET_GET		((FmcFwOpcode)0x1b)
#define FMC_FW_OPCODE_RX_VOLUME_SET_GET			((FmcFwOpcode)0x1c)
#define FMC_FW_OPCODE_RX_AUDIO_ENABLE_SET_GET		((FmcFwOpcode)0x1d)
#define FMC_FW_OPCODE_RX_POWER_SET_GET				((FmcFwOpcode)0x20)
#define FMC_FW_OPCODE_RX_HILO_SET_GET				((FmcFwOpcode)0x23)
#define FMC_FW_OPCODE_RX_TUNER_MODE_SET			((FmcFwOpcode)0x2d)
#define FMC_FW_OPCODE_RX_STOP_SEARCH				((FmcFwOpcode)0x2e)
#define FMC_FW_OPCODE_RX_RDS_CNTRL_SET				((FmcFwOpcode)0x2f)
#define FMC_FW_OPCODE_RX_CHANNEL_SPACING_SET_GET		((FmcFwOpcode)0x38)
#define FMC_FW_OPCODE_RX_CHANNEL				((FmcFwOpcode)0x7b)

/***********************************************************
	FM TX commands opcodes
************************************************************/
#define FMC_FW_OPCODE_TX_CHANL_SET_GET							((FmcFwOpcode)0x37)
#define FMC_FW_OPCODE_TX_CHANL_BW_SET_GET						((FmcFwOpcode)0x38)
#define FMC_FW_OPCODE_TX_POWER_LEVEL_SET_GET					((FmcFwOpcode)0x3b)
#define FMC_FW_OPCODE_TX_AUDIO_IO_SET							((FmcFwOpcode)0x3f)
#define FMC_FW_OPCODE_TX_PREMPH_SET_GET						((FmcFwOpcode)0x40)
#define FMC_FW_OPCODE_TX_MONO_SET_GET							((FmcFwOpcode)0x42)
#define FMC_FW_OPCODE_TX_PI_CODE_SET_GET						((FmcFwOpcode)0x44)
#define FMC_FW_OPCODE_TX_RDS_ECC_SET_GET						((FmcFwOpcode)0x45)
#define FMC_FW_OPCODE_TX_RDS_PTY_CODE_SET_GET					((FmcFwOpcode)0x46)
#define FMC_FW_OPCODE_TX_RDS_AF_SET_GET						((FmcFwOpcode)0x47)
#define FMC_FW_OPCODE_TX_RDS_PS_DISPLAY_MODE_SET_GET			((FmcFwOpcode)0x4a)
#define FMC_FW_OPCODE_TX_RDS_REPERTOIRE_SET_GET				((FmcFwOpcode)0x4d)
#define FMC_FW_OPCODE_TX_RDS_TA_SET_GET						((FmcFwOpcode)0x4e)
#define FMC_FW_OPCODE_TX_RDS_TP_SET_GET						       ((FmcFwOpcode)0x4f)
#define FMC_FW_OPCODE_TX_RDS_DI_CODES_SET_GET					((FmcFwOpcode)0x50)
#define FMC_FW_OPCODE_TX_RDS_MUSIC_SPEECH_FLAG_SET_GET		((FmcFwOpcode)0x51)
#define FMC_FW_OPCODE_TX_RDS_PS_SCROLL_SPEED_SET_GET			((FmcFwOpcode)0x52)
#define FMC_FW_OPCODE_TX_POWER_ENB_SET							((FmcFwOpcode)0x5a)
#define FMC_FW_OPCODE_TX_POWER_UP_DOWN_SET					((FmcFwOpcode)0x5b)
#define FMC_FW_OPCODE_TX_MUTE_MODE_SET_GET					((FmcFwOpcode)0x5c)
#define FMC_FW_OPCODE_TX_RDS_DATA_ENB_SET_GET					((FmcFwOpcode)0x5e)
#define FMC_FW_OPCODE_TX_RDS_CONFIG_DATA_SET					((FmcFwOpcode)0x62)
#define FMC_FW_OPCODE_TX_RDS_DATA_SET							((FmcFwOpcode)0x63)

#define FMC_FW_MASK_FR		((FmcFwIntMask)0x0001)	/* Tuning Operation Ended						*/
#define FMC_FW_MASK_BL		((FmcFwIntMask)0x0002)	/* Band limit was reached during search			*/
#define FMC_FW_MASK_RDS		((FmcFwIntMask)0x0004)	/* RDS data threshold reached in FIFO buffer	*/
#define FMC_FW_MASK_BBLK		((FmcFwIntMask)0x0008)	/* RDS B block match condition occurred			*/
#define FMC_FW_MASK_LSYNC		((FmcFwIntMask)0x0010)	/* RDS sync was lost							*/
#define FMC_FW_MASK_LEV		((FmcFwIntMask)0x0020)	/* RSSI level has fallen below the threshold configured by SEARCH_LVL_SET	*/
#define FMC_FW_MASK_IFFR		((FmcFwIntMask)0x0040)	/* Received signal frequency is out of range	*/
#define FMC_FW_MASK_PI			((FmcFwIntMask)0x0080)	/* RDS PI match occurred						*/
#define FMC_FW_MASK_PD		((FmcFwIntMask)0x0100)	/* Audio pause detect occurred					*/
#define FMC_FW_MASK_STIC		((FmcFwIntMask)0x0200)	/* Stereo indication changed					*/
#define FMC_FW_MASK_MAL		((FmcFwIntMask)0x0400)	/* Hardware malfunction							*/
#define FMC_FW_MASK_POW_ENB		((FmcFwIntMask)0x0800)	/* Tx Power Enable/Disable							*/
#define FMC_FW_MASK_INVALID_PARAM	((FmcFwIntMask)0x1000)
/*
	Global FM Core Power Values
*/
#define FMC_FW_FM_CORE_POWER_DOWN	((FMC_U8)0)
#define FMC_FW_FM_CORE_POWER_UP		((FMC_U8)1)

/*
	bbb
*/
#define FMC_FW_RX_TUNER_MODE_STOP_SEARCH      ((FmcFwCmdParmsValue)0x00)
#define FMC_FW_RX_TUNER_MODE_PRESET_MODE	((FmcFwCmdParmsValue)0x01)
#define FMC_FW_RX_TUNER_MODE_AUTO_SEARCH_MODE	((FmcFwCmdParmsValue)0x02)
#define FMC_FW_RX_TUNER_MODE_ALTER_FREQ_JUMP	((FmcFwCmdParmsValue)0x03)
#define FMC_FW_RX_TUNER_MODE_COMPLETE_SCAN	((FmcFwCmdParmsValue)0x05)
/*
	bbb
*/
#define FMC_FW_RX_MUTE_UNMUTE_MODE				((FmcFwCmdParmsValue)0x00)
#define FMC_FW_RX_MUTE_RF_DEP_MODE				((FmcFwCmdParmsValue)0x01)
#define FMC_FW_RX_MUTE_AC_MUTE_MODE				((FmcFwCmdParmsValue)0x02)
#define FMC_FW_RX_MUTE_HARD_MUTE_LEFT_MODE		((FmcFwCmdParmsValue)0x04)
#define FMC_FW_RX_MUTE_HARD_MUTE_RIGHT_MODE		((FmcFwCmdParmsValue)0x08)
#define FMC_FW_RX_MUTE_SOFT_MUTE_FORCE_MODE		((FmcFwCmdParmsValue)0x10)
/*
	bbb
*/
#define FMC_FW_RX_POWER_SET_FM_ON_RDS_OFF 	((FmcFwCmdParmsValue)0x01)
#define FMC_FW_RX_POWER_SET_FM_AND_RDS_ON 	((FmcFwCmdParmsValue)0x03)
#define FMC_FW_RX_POWER_SET_FM_AND_RDS_OFF	((FmcFwCmdParmsValue)0x00)
/*
	bbb
*/
#define FMC_FW_RX_FM_POWER_MODE_DISABLE		((FmcFwCmdParmsValue)0)
#define FMC_FW_RX_FM_POWER_MODE_ENABLE		((FmcFwCmdParmsValue)1)
/*
	bbb
*/
#define FMC_FW_RX_RDS_FLUSH_FIFO				((FmcFwCmdParmsValue)1)
#define FMC_FW_RX_RDS_THRESHOLD				((FmcFwCmdParmsValue)64)
#define FMC_FW_RX_RDS_THRESHOLD_MAX			((FmcFwCmdParmsValue)85)
/*
	bbb
*/
#define FMC_FW_RX_FM_AUDIO_ENABLE_I2S 	((FmcFwCmdParmsValue)0x01)
#define FMC_FW_RX_FM_AUDIO_ENABLE_ANALOG		((FmcFwCmdParmsValue)0x02)
#define FMC_FW_RX_FM_AUDIO_ENABLE_I2S_AND_ANALOG	((FmcFwCmdParmsValue)0x03)
#define FMC_FW_RX_FM_AUDIO_ENABLE_DISABLE		((FmcFwCmdParmsValue)0x00)

/*

*/
#define FMC_FW_RX_FM_VOLUMN_MIN 					((FmcFwCmdParmsValue)0x00)
#define FMC_FW_RX_FM_VOLUMN_MAX 					((FmcFwCmdParmsValue)0xf170)
#define FMC_FW_RX_FM_VOLUMN_INITIAL_VALUE		((FmcFwCmdParmsValue)0x78b8)
#define FMC_FW_RX_FM_GAIN_STEP 					((FmcFwCmdParmsValue)0x370)

/*
	Channel BW Values
*/
#define FMC_FW_TX_CHANNEL_BW_50_KHZ	((FmcFwCmdParmsValue)1)
#define FMC_FW_TX_CHANNEL_BW_100_KHZ	((FmcFwCmdParmsValue)2)
#define FMC_FW_TX_CHANNEL_BW_200_KHZ	((FmcFwCmdParmsValue)4)

/*
	Audio IO Set Values
*/
#define FMC_FW_TX_AUDIO_IO_SET_ANALOG	((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_AUDIO_IO_SET_I2S		((FmcFwCmdParmsValue)1)

/*
	PUPD Values
*/
#define FMC_FW_TX_POWER_DOWN	((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_POWER_UP		((FmcFwCmdParmsValue)1)

/*
	POWER ENABLE Values
*/
#define FMC_FW_TX_POWER_DISABLE	((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_POWER_ENABLE	((FmcFwCmdParmsValue)1)

/*
	TX MUTE Values
*/
#define FMC_FW_TX_UNMUTE			((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_MUTE			((FmcFwCmdParmsValue)1)

/*
	TX RDS DATA ENABLE Values
*/
#define FMC_FW_TX_RDS_ENABLE_STOP	((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_RDS_ENABLE_START	((FmcFwCmdParmsValue)1)

/*
	TX RDS TEXT TYPE Values
*/
#define FMC_FW_TX_RDS_TEXT_TYPE_PS		((FmcFwCmdParmsValue)1)
#define FMC_FW_TX_RDS_TEXT_TYPE_RT_AUTO		((FmcFwCmdParmsValue)2)
#define FMC_FW_TX_RDS_TEXT_TYPE_RT_A		((FmcFwCmdParmsValue)3)
#define FMC_FW_TX_RDS_TEXT_TYPE_RT_B		((FmcFwCmdParmsValue)4)

/*
	TX RDS PS SCROLL Mode Values
*/
#define FMC_FW_TX_RDS_PS_DISPLAY_MODE_SCROLL_OFF		((FmcFwCmdParmsValue)0)
#define FMC_FW_TX_RDS_PS_DISPLAY_MODE_SCROLL_ON		((FmcFwCmdParmsValue)1)

/*
	Maximum length of data that may be sent in a single RDS data set command
	Once FM FW team removes internal limitations, HCI limitations (much
	longer) may apply.

	In case a longer RDS data should be sent to the chip, it is divided into
	multiple chunks, each chunk being up to FMC_FW_TX_MAX_RDS_DATA_SET_LEN
	bytes long
*/
#define FMC_FW_TX_MAX_RDS_DATA_SET_LEN	((FMC_UINT)30)
/*
	Defines the max length of data that can be written to FM Hardware register
*/
#define FMC_FW_WRITE_HARDWARE_REG_MAX_DATA_LEN ((FMC_UINT)HCI_CMD_PARM_LEN)
#endif /* ifndef __FMC_FW_DEFS_H */

