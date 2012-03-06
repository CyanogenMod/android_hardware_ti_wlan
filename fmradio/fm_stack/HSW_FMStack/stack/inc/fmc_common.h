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

/*******************************************************************************\
*
*   FILE NAME:      fmc_common.h
*
*   BRIEF:          This file defines common FM types, definitions, and functions.
*
*   DESCRIPTION:    
*
*	FM RX and FM TX each have their own specific files with specific definitions. This file contains
*	definitions that are common to both FM RX & TX.
*			
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __FMC_COMMON_H
#define __FMC_COMMON_H

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_defs.h"
/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
 
/*-------------------------------------------------------------------------------
 * FmcLen Type
 *
 * 	Represents a length of an entity
 */
typedef FMC_UINT FmcLenType;

#define FM_RDS_PS_MAX_MSG_LEN					((FmcLenType)8)
#define FM_RDS_RT_A_MAX_MSG_LEN					((FmcLenType)32)
#define FM_RDS_RT_B_MAX_MSG_LEN					((FmcLenType)64)
#define FM_RDS_RAW_MAX_MSG_LEN					((FmcLenType)255)

/*-------------------------------------------------------------------------------
 * FmcAppHandle Type
 *
 */
typedef struct _FmcAppHandle 	FmcAppHandle;


/*-------------------------------------------------------------------------------
 * FmcBand Type
 *
 */
typedef  FMC_UINT  FmcBand;

#define FMC_BAND_EUROPE_US						((FmcBand)(0))
#define FMC_BAND_JAPAN								((FmcBand)(1))


/*-------------------------------------------------------------------------------
 * FmcMuteMode Type
 *
 */
typedef  FMC_UINT	 FmcMuteMode;

#define 	FMC_MUTE									((FmcMuteMode)(0))
#define	FMC_NOT_MUTE								((FmcMuteMode)(1))
#define	FMC_ATTENUATE								((FmcMuteMode)(2))

/*-------------------------------------------------------------------------------
 * FmcFreq Type
 *
 *	Represents a receiver / transmitter tuning frequency
 */
typedef  FMC_UINT  FmcFreq;

#define FMC_UNDEFINED_FREQ						((FmcFreq)0xFFFFFFFF)

/* Europe / US band limits */
#define FMC_FIRST_FREQ_US_EUROPE_KHZ				((FmcFreq)87500)
#define FMC_LAST_FREQ_US_EUROPE_KHZ 			((FmcFreq)108000)

/* Japan band limits */
#define FMC_FIRST_FREQ_JAPAN_KHZ					((FmcFreq)76000)
#define FMC_LAST_FREQ_JAPAN_KHZ 				((FmcFreq)90000)

/*-------------------------------------------------------------------------------
 * FmcChannelSpacing Type
 *
 * 	Represents the resolution of the tuner, in kHz.
 */
typedef FMC_UINT FmcChannelSpacing;

#define FMC_CHANNEL_SPACING_50_KHZ				((FmcChannelSpacing)(1))
#define FMC_CHANNEL_SPACING_100_KHZ				((FmcChannelSpacing)(2))
#define FMC_CHANNEL_SPACING_200_KHZ				((FmcChannelSpacing)(4))

/*-------------------------------------------------------------------------------
 * FmcEmphasisFilter type
 *
 *	Represents a (Pre / De) emphasis mode for the receiver / transmitter
 *
 */
typedef FMC_UINT FmcEmphasisFilter;

#define FMC_EMPHASIS_FILTER_NONE 					((FmcEmphasisFilter)0)
#define FMC_EMPHASIS_FILTER_50_USEC 				((FmcEmphasisFilter)1)
#define FMC_EMPHASIS_FILTER_75_USEC 				((FmcEmphasisFilter)2)

/*-------------------------------------------------------------------------------
 * FmcAudioRouteMask type
 *
 */
typedef FMC_U32 FmcAudioRouteMask;

#define FMC_AUDIO_ROUTE_MASK_I2S 				((FmcAudioRouteMask)0x00000001)
#define FMC_AUDIO_ROUTE_MASK_ANALOG						((FmcAudioRouteMask)0x00000002)

#define FMC_AUDIO_ROUTE_MASK_NONE					((FmcAudioRouteMask)0x0)
#define FMC_AUDIO_ROUTE_MASK_ALL						((FmcAudioRouteMask)0x00000003)

/*-------------------------------------------------------------------------------
 * FmcRdsRdbs Type
 *
 * 	Represents the RDS system type (RDS / RBDS)
 */
typedef FMC_UINT FmcRdsSystem;

#define FM_RDS_SYSTEM_RDS						((FmcRdsSystem)(0))
#define FM_RDS_SYSTEM_RBDS					((FmcRdsSystem)(1))

/*-------------------------------------------------------------------------------
 * FmcRdsGroupTypeMask Type
 *
 *	Represents an RDS group type & version
 *
 *	There are 15 groups, each group has 2 versions: A and B
 *
 *	The values are bit masks
 */
typedef FMC_U32 FmcRdsGroupTypeMask;

#define FM_RDS_GROUP_TYPE_MASK_0A						((FmcRdsGroupTypeMask)0x00000001)
#define FM_RDS_GROUP_TYPE_MASK_0B						((FmcRdsGroupTypeMask)0x00000002)
#define FM_RDS_GROUP_TYPE_MASK_1A						((FmcRdsGroupTypeMask)0x00000004)
#define FM_RDS_GROUP_TYPE_MASK_1B						((FmcRdsGroupTypeMask)0x00000008)
#define FM_RDS_GROUP_TYPE_MASK_2A						((FmcRdsGroupTypeMask)0x00000010)
#define FM_RDS_GROUP_TYPE_MASK_2B						((FmcRdsGroupTypeMask)0x00000020)
#define FM_RDS_GROUP_TYPE_MASK_3A						((FmcRdsGroupTypeMask)0x00000040)
#define FM_RDS_GROUP_TYPE_MASK_3B						((FmcRdsGroupTypeMask)0x00000080)
#define FM_RDS_GROUP_TYPE_MASK_4A						((FmcRdsGroupTypeMask)0x00000100)
#define FM_RDS_GROUP_TYPE_MASK_4B						((FmcRdsGroupTypeMask)0x00000200)
#define FM_RDS_GROUP_TYPE_MASK_5A						((FmcRdsGroupTypeMask)0x00000400)
#define FM_RDS_GROUP_TYPE_MASK_5B						((FmcRdsGroupTypeMask)0x00000800)
#define FM_RDS_GROUP_TYPE_MASK_6A						((FmcRdsGroupTypeMask)0x00001000)
#define FM_RDS_GROUP_TYPE_MASK_6B						((FmcRdsGroupTypeMask)0x00002000)
#define FM_RDS_GROUP_TYPE_MASK_7A						((FmcRdsGroupTypeMask)0x00004000)
#define FM_RDS_GROUP_TYPE_MASK_7B						((FmcRdsGroupTypeMask)0x00008000)
#define FM_RDS_GROUP_TYPE_MASK_8A						((FmcRdsGroupTypeMask)0x00010000)
#define FM_RDS_GROUP_TYPE_MASK_8B						((FmcRdsGroupTypeMask)0x00020000)
#define FM_RDS_GROUP_TYPE_MASK_9A						((FmcRdsGroupTypeMask)0x00040000)
#define FM_RDS_GROUP_TYPE_MASK_9B						((FmcRdsGroupTypeMask)0x00080000)
#define FM_RDS_GROUP_TYPE_MASK_10A						((FmcRdsGroupTypeMask)0x00100000)
#define FM_RDS_GROUP_TYPE_MASK_10B						((FmcRdsGroupTypeMask)0x00200000)
#define FM_RDS_GROUP_TYPE_MASK_11A						((FmcRdsGroupTypeMask)0x00400000)
#define FM_RDS_GROUP_TYPE_MASK_11B						((FmcRdsGroupTypeMask)0x00800000)
#define FM_RDS_GROUP_TYPE_MASK_12A						((FmcRdsGroupTypeMask)0x01000000)
#define FM_RDS_GROUP_TYPE_MASK_12B						((FmcRdsGroupTypeMask)0x02000000)
#define FM_RDS_GROUP_TYPE_MASK_13A						((FmcRdsGroupTypeMask)0x04000000)
#define FM_RDS_GROUP_TYPE_MASK_13B						((FmcRdsGroupTypeMask)0x08000000)
#define FM_RDS_GROUP_TYPE_MASK_14A						((FmcRdsGroupTypeMask)0x10000000)
#define FM_RDS_GROUP_TYPE_MASK_14B						((FmcRdsGroupTypeMask)0x20000000)
#define FM_RDS_GROUP_TYPE_MASK_15A						((FmcRdsGroupTypeMask)0x40000000)
#define FM_RDS_GROUP_TYPE_MASK_15B						((FmcRdsGroupTypeMask)0x80000000)

#define FM_RDS_GROUP_TYPE_MASK_NONE						((FmcRdsGroupTypeMask)0x0)
#define FM_RDS_GROUP_TYPE_MASK_ALL						((FmcRdsGroupTypeMask)0xFFFFFFFF)

/*-------------------------------------------------------------------------------
 * FmcRdsRtMsgType Type
 *
 * 	Represents the type of RDS RT message: A or B
 *	The values comply with the values in RDS_CONFIG_DATA_SET
 *	
 *	In the Menual mode the firmware itself will decide if A or B type will 
 *	be transmittes depending on the length of the text.
 *
 *	Worning: Don't change the values - They are competible to the FW definitions
 */
typedef FMC_UINT FmcRdsRtMsgType;

#define FMC_RDS_RT_MSG_TYPE_AUTO					((FmcRdsRtMsgType)FMC_FW_TX_RDS_TEXT_TYPE_RT_AUTO)
#define FMC_RDS_RT_MSG_TYPE_A						((FmcRdsRtMsgType)FMC_FW_TX_RDS_TEXT_TYPE_RT_A)
#define FMC_RDS_RT_MSG_TYPE_B						((FmcRdsRtMsgType)FMC_FW_TX_RDS_TEXT_TYPE_RT_B)


/*-------------------------------------------------------------------------------
 * FmcAfCode Type
 *
 * 	RDS Alternate Frequency Code
 */
typedef  FMC_UINT  FmcAfCode;

/* Special AF code that indicates that no valid AF value exists */
#define FMC_AF_CODE_NO_AF_AVAILABLE				((FmcAfCode)224)

/*-------------------------------------------------------------------------------
 * FmcRdsPtyCode Type
 *
 * 	RDS Program Type Code
 */
typedef  FMC_UINT  FmcRdsPtyCode;

#define FMC_RDS_PTY_CODE_NO_PROGRAM_UNDEFINED		((FmcRdsPtyCode)0)
#define FMC_RDS_PTY_CODE_MAX_VALUE		((FmcRdsPtyCode)31)
/*-------------------------------------------------------------------------------
 * FmcRdsPiCode Type
 *
  * 	RDS Program Indication Code
*/
typedef  FMC_U16  FmcRdsPiCode;

/*-------------------------------------------------------------------------------
 * FmcRdsRepertoire Type
 *
 * 	RDS Repertoire used for text data encoding and decoding
 */
typedef  FMC_UINT  FmcRdsRepertoire;

#define FMC_RDS_REPERTOIRE_G0_CODE_TABLE			((FmcRdsRepertoire)0)
#define FMC_RDS_REPERTOIRE_G1_CODE_TABLE			((FmcRdsRepertoire)1)
#define FMC_RDS_REPERTOIRE_G2_CODE_TABLE			((FmcRdsRepertoire)2)

/*-------------------------------------------------------------------------------
 * FmcRdsPsDispayScrollSizeMode Type
 *
 * 	
*/
typedef  FMC_UINT	 FmcRdsPsDispayScrollSize;

/*-------------------------------------------------------------------------------
 * FmcRdsPsDispayScrollSizeMode Type
 *
 * 	
*/
typedef  FMC_UINT	 FmcRdsPsScrollSpeed;

#define FMC_RDS_SCROLL_SPEED_DEFUALT			((FmcRdsPsScrollSpeed)3)
/*-------------------------------------------------------------------------------
 * FmcRdsPsDisplayMode Type
 *
 * 	Flag that indicates whether PS text should be scrolled or not
*/
typedef  FMC_UINT	 FmcRdsPsDisplayMode;

#define FMC_RDS_PS_DISPLAY_MODE_STATIC						((FmcRdsPsDisplayMode)0)
#define FMC_RDS_PS_DISPLAY_MODE_SCROLL							((FmcRdsPsDisplayMode)1)

/*-------------------------------------------------------------------------------
 * FmcRdsTaCode Type
 *
 *	RDS Traffic Announcement Code (Used in conjunction with TP)
*/
typedef  FMC_UINT	 FmcRdsTaCode;

#define FMC_RDS_TA_OFF								((FmcRdsTaCode)0)
#define FMC_RDS_TA_ON								((FmcRdsTaCode)1)

/*-------------------------------------------------------------------------------
 * FmcRdsTpCode Type
 *
 *	RDS Traffic Program Code (Used in conjunction with TA)
*/
typedef  FMC_UINT	 FmcRdsTpCode;

#define FMC_RDS_TP_OFF								((FmcRdsTpCode)0)
#define FMC_RDS_TP_ON								((FmcRdsTpCode)1)

/*-------------------------------------------------------------------------------
 * FmcRdsMusicSpeechFlag Type
 *
 * 	RDS flag that indicates whether the broadcast program is of Music or Speech
*/
typedef  FMC_UINT	 FmcRdsMusicSpeechFlag;

#define FMC_RDS_SPEECH								((FmcRdsMusicSpeechFlag)0)
#define FMC_RDS_MUSIC								((FmcRdsMusicSpeechFlag)1)

/*-------------------------------------------------------------------------------
 * FmcRdsExtendedCountryCode Type
 *
 * 	RDS flag that indicates whether the broadcast program is of Music or Speech
*/
typedef  FMC_UINT	 FmcRdsExtendedCountryCode;

#define FMC_RDS_ECC_NONE							((FmcRdsExtendedCountryCode)0)


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/
/*[ToDo Zvi ]Check what are these functions*/
/*-------------------------------------------------------------------------------
 * FMC_RegisterApp()
 *
 *	Brief:  
 *		Allocates a unique application handle for a new FM application.
 *
 *	Description:
 *		This function may be called by a new applicaton before any other BTL 
 *		API function, except FMC_Init. Alternatively, the default application 
 *		may be used by supplying a null appplication handle when creating
 *		a new FM Rx or TX context.
 *
 *		The allocated handle should be supplied in subsequent FM API calls.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		appHandle [out] - allocated application handle.
 *
 * Returns:
 *		FMC_STATUS_SUCCESS - Application handle was allocated successfully.
 *
 *		FMC_STATUS_FAILED -  Failed to allocate application handle.
 */
FmcStatus FMC_RegisterApp(FmcAppHandle **appHandle);

/*-------------------------------------------------------------------------------
 * FMC_DeRegisterApp()
 *
 *	Brief:  
 *		Releases an application handle (previously allocated with FMC_RegisterApp).
 *
 *	Description:
 *		An application should call this function when it completes using FM RX and TX services.
 *
 *		Upon completion, the application handle is set to null in order to prevent 
 *		the application from an illegal attempt to keep using it.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		appHandle [in/out] - application handle.
 *
 * Returns:
 *		FMc_STATUS_SUCCESS - Application handle was deallocated successfully.
 *
 *		FMC_STATUS_FAILED -  Failed to deallocate application handle.
 */
FmcStatus FMC_DeRegisterApp(FmcAppHandle **appHandle);

#endif /* ifndef __FMC_COMMON_H*/

