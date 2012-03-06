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
*   FILE NAME:      fm_rx.h
*
*   BRIEF:          This file defines the API of the FM Rx stack.
*
*   DESCRIPTION:    General
*
*					The fms_api layer defines the procedures used by the FM applications.
*                   
*   AUTHOR:   Udi Ron/Zvi Schneider
*
\*******************************************************************************/


#ifndef __FM_RX_H
#define __FM_RX_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_utils.h"
#include "fmc_common.h"
#include "ccm_audio_types.h"
/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
/*

*/
typedef enum 
{
	FM_RX_AUDIO_TARGET_I2SH ,
	FM_RX_AUDIO_TARGET_PCMH ,
	FM_RX_AUDIO_TARGET_FM_OVER_SCO,
	FM_RX_AUDIO_TARGET_FM_OVER_A3DP,
	FM_RX_AUDIO_TARGET_FM_ANALOG ,
	FM_RX_AUDIO_TARGET_INVALID 
} FmRx_Resource;
/*
	Mask of possible FM RX Resources
*/
typedef enum 
{
	FM_RX_TARGET_MASK_INVALID = 0,
	FM_RX_TARGET_MASK_I2SH			= 1 << FM_RX_AUDIO_TARGET_I2SH,
	FM_RX_TARGET_MASK_PCMH			= 1 << FM_RX_AUDIO_TARGET_PCMH,
	FM_RX_TARGET_MASK_FM_OVER_SCO = 1<< FM_RX_AUDIO_TARGET_FM_OVER_SCO,
	FM_RX_TARGET_MASK_FM_OVER_A3DP = 1<< FM_RX_AUDIO_TARGET_FM_OVER_A3DP,
	FM_RX_TARGET_MASK_FM_ANALOG 	= 1 << FM_RX_AUDIO_TARGET_FM_ANALOG,
} FmRxAudioTargetMask;

/* 
	Forward Declaration
*/
typedef struct _FmRxEvent FmRxEvent;

/*-------------------------------------------------------------------------------
 * FmRxContext structure
 *
 *     Each application must create a context before starting to use the FM 
 */
typedef struct _FmRxContext FmRxContext;

/*-------------------------------------------------------------------------------
 * FmRxCallBack type
 *
 *     A function of this type is called to indicate FM events.
 */
typedef  void (*FmRxCallBack)(const FmRxEvent *event);

/********************************************************************************
 *
 * Events sent to the application
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FmRxEventType structure
 *
 */
typedef FMC_UINT FmRxEventType;

/* 
	This event will be sent whenever an API command to the FM module 
	in the chip completed
*/
#define FM_RX_EVENT_CMD_DONE						((FmRxEventType)0)

/* 
	This event will be sent when the receiver changes the 
	reception mode from mono to stereo or vice-versa.
	
	Note that the receiver automatically switches to mono when the TBD too high, and
	back to stereo when it is low enough. This occurs even when the application
	configured the mono/stereo mode to stereo via the FM_RX_SetMonoStereoMode() function
*/
#define FM_RX_EVENT_MONO_STEREO_MODE_CHANGED	((FmRxEventType)1)

/* 
	This event will be sent when the RDS PI Code has changed.

	This event will also be sent whenever RDS reception starts after
	being disabled (either explicitly or after enabling FM RX)

	"p.piChangedData" is valid. 
	"p.piChangedData.pi" contains the new PI Code 

*/
#define FM_RX_EVENT_PI_CODE_CHANGED				((FmRxEventType)2)

/*
	This event indicates that an AF switch process started

	The current AF list will be traversed and an attempt will be made
	to switch to the frequencies in the list one by one. 
*/
#define FM_RX_EVENT_AF_SWITCH_START				((FmRxEventType)3)

/* 
	This event is sent when an attempt to switch to the next 
	frequency in the AF list failed.

	"p.afSwitchData" is valid. 
	"p.afSwitchData.afFreq" is set to	the attempted AF frequency
	"p.afSwitchData.tunedFreq" is set to the original tuned frequency.

 */
#define FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED	((FmRxEventType)4)

/* 
	The AF switch process completed. The status field indicates the 
	results of the switch and may be:
	
	FM_RX_STATUS_SUCCESS: 
		The switch succeeded. "p.afSwitchData" is valid. 
		"p.afSwitchData.afFreq" is set to the newly tuned frequency.
		"p.afSwitchData.tunedFreq" is set to the original tuned frequency.

	FM_RX_STATUS_AF_SWITCH_FAILED_LIST_EXHAUSTED: 
		No frequency in the list complied as a valid AF the attempted AF frequency
*/
#define FM_RX_EVENT_AF_SWITCH_COMPLETE			((FmRxEventType)5)

/*
	This event that the list of available alternate frequencies has changed.
	"p.afListData" is valid.
	"p.afListData.afListSize" indicates the size of the new list
	"p.afListData.afList" points to the list of frequency values
*/
#define FM_RX_EVENT_AF_LIST_CHANGED				((FmRxEventType)6)

/*
	Program Service name changed event

	"p.psData" is valid.
	"p.psData.frequency" contains The frequency that is currently tuned
	"p.psData.name" points to the PS name string
    "p.psData.repertoire" Contains RDS Repertoire used for text data encoding and decoding.    
*/
#define FM_RX_EVENT_PS_CHANGED					((FmRxEventType)7)

/*
	A RDS radio-text message was received

	"p.radioText" is valid.
	"p.radioText.len" contains the length of the message
	"p.radioText.msg" points to the RT message string
*/
#define FM_RX_EVENT_RADIO_TEXT					((FmRxEventType)8)

/*
	This event indicates that a requested RDS group data was received

	"p.rawRdsGroupData" is valid.
	"p.rawRdsGroupData.type" contains the type of the group
	"p.rawRdsGroupData.data" points to the 8 bytes of group data (2 bytes per block)
*/
#define FM_RX_EVENT_RAW_RDS						((FmRxEventType)9)	/* Raw RDS event */

/*
	This event indicates that a requested xxx was received

	"p.xxx" is valid.
	"p.xxx.xxx" 
	"p.xxx.xxx" 
*/
#define FM_RX_EVENT_AUDIO_PATH_CHANGED 					((FmRxEventType)10)	/*  */

/* 
	This event will be sent when the RDS PTY Code has changed.

	This event will also be sent whenever RDS reception starts after
	being disabled (either explicitly or after enabling FM RX)

	"p.ptyCode" contains the new PTY code. 

*/
#define FM_RX_EVENT_PTY_CODE_CHANGED 			((FmRxEventType)11)

/* 
	This event will be sent whenever the Complete Scan  end's.

        "p.completeScanData"
        "p.completeScanData.numOfChannels" - contains the number of channels that was found in the Scan.
        "p.completeScanData.channelsData" - Contains the Freq in MHz for each channel.

*/

#define FM_RX_EVENT_COMPLETE_SCAN_DONE			((FmRxEventType)12)



/********************************************************************************
 *
 * FM commands definitions
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FmRxCmdType structure
 *
 */
typedef FMC_UINT FmRxCmdType;

#define FM_RX_CMD_ENABLE							((FmRxCmdType)0)	/* Enable command */
#define FM_RX_CMD_DISABLE							((FmRxCmdType)1)	/* Disable command */
#define FM_RX_CMD_SET_BAND						((FmRxCmdType)2)	/* Set Band command */
#define FM_RX_CMD_GET_BAND						((FmRxCmdType)3)	/* Get Band command */
#define FM_RX_CMD_SET_MONO_STEREO_MODE			((FmRxCmdType)4)	/* Set Mono/Stereo command */
#define FM_RX_CMD_GET_MONO_STEREO_MODE			((FmRxCmdType)5)	/* Get Mono/Stereo command */
#define FM_RX_CMD_SET_MUTE_MODE					((FmRxCmdType)6)	/* Set Mute mode command */
#define FM_RX_CMD_GET_MUTE_MODE					((FmRxCmdType)7)	/* Get Mute mode command */
#define FM_RX_CMD_SET_RF_DEPENDENT_MUTE_MODE	((FmRxCmdType)8)	/* Set RF-Dependent Mute Mode command */
#define FM_RX_CMD_GET_RF_DEPENDENT_MUTE_MODE	((FmRxCmdType)9)	/* Get RF-Dependent Mute Mode command */
#define FM_RX_CMD_SET_RSSI_THRESHOLD				((FmRxCmdType)10)	/* Set RSSI Threshold command */
#define FM_RX_CMD_GET_RSSI_THRESHOLD				((FmRxCmdType)11)	/* Get RSSI Threshold command */
#define FM_RX_CMD_SET_DEEMPHASIS_FILTER			((FmRxCmdType)12)	/* Set De-Emphassi Filter command */
#define FM_RX_CMD_GET_DEEMPHASIS_FILTER			((FmRxCmdType)13)	/* Get De-Emphassi Filter command */
#define FM_RX_CMD_SET_VOLUME						((FmRxCmdType)14)	/* Set Volume command */
#define FM_RX_CMD_GET_VOLUME						((FmRxCmdType)15)	/* Get Volume command */
#define FM_RX_CMD_TUNE								((FmRxCmdType)16)	/* Tune command */
#define FM_RX_CMD_GET_TUNED_FREQUENCY			((FmRxCmdType)17)	/* Get Tuned Frequency command */
#define FM_RX_CMD_SEEK								((FmRxCmdType)18)	/* Seek command */
#define FM_RX_CMD_STOP_SEEK						((FmRxCmdType)19)	/* Stop Seek command */
#define FM_RX_CMD_GET_RSSI						((FmRxCmdType)20)	/* Get RSSI command */
#define FM_RX_CMD_ENABLE_RDS						((FmRxCmdType)21)	/* Enable RDS command */
#define FM_RX_CMD_DISABLE_RDS						((FmRxCmdType)22)	/* Disable RDS command */
#define FM_RX_CMD_SET_RDS_SYSTEM					((FmRxCmdType)23)	/* Set RDS System command */
#define FM_RX_CMD_GET_RDS_SYSTEM					((FmRxCmdType)24)	/* Get RDS System command */
#define FM_RX_CMD_SET_RDS_GROUP_MASK			((FmRxCmdType)25)	/* Set RDS groups to be recieved */
#define FM_RX_CMD_GET_RDS_GROUP_MASK			((FmTxCmdType)26)	/*  Get RDS groups to be recieved*/
#define FM_RX_CMD_SET_RDS_AF_SWITCH_MODE		((FmTxCmdType)27)	/* Set AF Switch Mode command */
#define FM_RX_CMD_GET_RDS_AF_SWITCH_MODE		((FmRxCmdType)28)	/* Get AF Switch Mode command */

#define FM_RX_CMD_ENABLE_AUDIO				((FmRxCmdType)29)	/* Set Audio Routing command */
#define FM_RX_CMD_DISABLE_AUDIO 				((FmRxCmdType)30)	/* Get Audio Routing command */
#define FM_RX_CMD_DESTROY							((FmRxCmdType)31)	/* Destroy command */

#define FM_RX_CMD_CHANGE_AUDIO_TARGET					((FmRxCmdType)32)	/* Change the audio target*/
#define FM_RX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION	((FmRxCmdType)33)	/* Change the digital target configuration*/

#define FM_RX_INIT_ASYNC                              	((FmRxCmdType)34)	/* */

#define FM_RX_CMD_INIT                              	((FmRxCmdType)35)	/* */
#define FM_RX_CMD_DEINIT                              	((FmRxCmdType)36)	/* */
#define FM_RX_CMD_SET_CHANNEL_SPACING                              	((FmRxCmdType)37)	/* */
#define FM_RX_CMD_GET_CHANNEL_SPACING                              	((FmRxCmdType)38)	/* */
#define FM_RX_CMD_GET_FW_VERSION                            	((FmRxCmdType)39)	/*Gets the FW version */
#define FM_RX_CMD_IS_CHANNEL_VALID                            	((FmRxCmdType)40)	/*Verify that the tuned channel is valid*/
#define FM_RX_CMD_COMPLETE_SCAN                            	((FmRxCmdType)41)	/*Perfrom Complete Scan on the selected Band*/
#define FM_RX_CMD_COMPLETE_SCAN_PROGRESS                            	((FmRxCmdType)42)
#define FM_RX_CMD_STOP_COMPLETE_SCAN                            	((FmRxCmdType)43)

#define FM_RX_LAST_API_CMD						(FM_RX_CMD_STOP_COMPLETE_SCAN)
#define FM_RX_CMD_NONE					0xFFFFFFFF
/*-------------------------------------------------------------------------------
 * FmRxStatus type
 *
 *	TBD
 *
 */
typedef FMC_UINT FmRxStatus;

#define FM_RX_STATUS_SUCCESS							((FmRxStatus)FMC_STATUS_SUCCESS) 
#define FM_RX_STATUS_FAILED							((FmRxStatus)FMC_STATUS_FAILED)
#define FM_RX_STATUS_PENDING							((FmRxStatus)FMC_STATUS_PENDING)
#define FM_RX_STATUS_INVALID_PARM					((FmRxStatus)FMC_STATUS_INVALID_PARM)
#define FM_RX_STATUS_IN_PROGRESS						((FmRxStatus)FMC_STATUS_IN_PROGRESS)
#define FM_RX_STATUS_NOT_APPLICABLE					((FmRxStatus)FMC_STATUS_NOT_APPLICABLE)
#define FM_RX_STATUS_NOT_SUPPORTED					((FmRxStatus)FMC_STATUS_NOT_SUPPORTED)
#define FM_RX_STATUS_INTERNAL_ERROR					((FmRxStatus)FMC_STATUS_INTERNAL_ERROR)
#define FM_RX_STATUS_TRANSPORT_INIT_ERR				((FmRxStatus)FMC_STATUS_TRANSPORT_INIT_ERR)
#define FM_RX_STATUS_HARDWARE_ERR					((FmRxStatus)FMC_STATUS_HARDWARE_ERR)
#define FM_RX_STATUS_NO_VALUE_AVAILABLE				((FmRxStatus)FMC_STATUS_NO_VALUE_AVAILABLE)
#define FM_RX_STATUS_CONTEXT_DOESNT_EXIST			((FmRxStatus)FMC_STATUS_CONTEXT_DOESNT_EXIST)
#define FM_RX_STATUS_CONTEXT_NOT_DESTROYED			((FmRxStatus)FMC_STATUS_CONTEXT_NOT_DESTROYED)
#define FM_RX_STATUS_CONTEXT_NOT_ENABLED			((FmRxStatus)FMC_STATUS_CONTEXT_NOT_ENABLED)
#define FM_RX_STATUS_CONTEXT_NOT_DISABLED			((FmTxStatus)FMC_STATUS_CONTEXT_NOT_DISABLED)
#define FM_RX_STATUS_NOT_DE_INITIALIZED				((FmRxStatus)FMC_STATUS_NOT_DE_INITIALIZED)
#define FM_RX_STATUS_NOT_INITIALIZED					((FmRxStatus)FMC_STATUS_NOT_INITIALIZED)
#define FM_RX_STATUS_TOO_MANY_PENDING_CMDS			((FmTxStatus)FMC_STATUS_TOO_MANY_PENDING_CMDS)
#define FM_RX_STATUS_DISABLING_IN_PROGRESS			((FmRxStatus)FMC_STATUS_DISABLING_IN_PROGRESS)
#define FM_RX_STATUS_SCRIPT_EXEC_FAILED				((FmcStatus)FMC_STATUS_SCRIPT_EXEC_FAILED)

#define FM_RX_STATUS_FAILED_BT_NOT_INITIALIZED		((FmcStatus)FMC_STATUS_FAILED_BT_NOT_INITIALIZED)
#define FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES ((FmcStatus)FMC_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)

#define FM_RX_STATUS_FM_TX_ALREADY_ENABLED			((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 1)
#define FM_RX_STATUS_SEEK_IN_PROGRESS				((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 2)
#define FM_RX_STATUS_SEEK_IS_NOT_IN_PROGRESS		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 3)
#define FM_RX_STATUS_AF_IN_PROGRESS					((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 4)
#define FM_RX_STATUS_RDS_IS_NOT_ENABLED				((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 5)
#define FM_RX_STATUS_SEEK_REACHED_BAND_LIMIT		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 6)

#define FM_RX_STATUS_SEEK_STOPPED					((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 7)
#define FM_RX_STATUS_SEEK_SUCCESS					((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 8)
#define FM_RX_STATUS_STOP_SEEK						((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 9)	/* Internal use */
#define FM_RX_STATUS_PENDING_UPDATE_CMD_PARAMS	((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 10)
#define FM_RX_STATUS_FAILED_ALREADY_PENDING 		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 11)
#define FM_RX_STATUS_INVALID_TYPE						((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 12)
#define FM_RX_STATUS_CMD_TYPE_WAS_ALREADY_ALLOCATED		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 13)
#define FM_RX_STATUS_AF_SWITCH_FAILED_LIST_EXHAUSTED		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 14)
#define FM_RX_STATUS_COMPLETE_SCAN_IS_NOT_IN_PROGRESS		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 15)
#define FM_RX_STATUS_COMPLETE_SCAN_STOPPED		((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE + 16)  

#define FM_RX_NO_VALUE									((FmRxStatus)FMC_FIRST_FM_RX_STATUS_CODE)

/*-------------------------------------------------------------------------------
 * FmRxRfDependentMuteMode type
 *
 *	TBD
 *
 */
typedef FMC_UINT FmRxRfDependentMuteMode;

#define FM_RX_RF_DEPENDENT_MUTE_ON					((FmRxRfDependentMuteMode)1)
#define FM_RX_RF_DEPENDENT_MUTE_OFF					((FmRxRfDependentMuteMode)0)

/*-------------------------------------------------------------------------------
 * FmRxSeekDirection type
 *
 *	TBD
 *
 */
typedef FMC_UINT FmRxSeekDirection;

#define FM_RX_SEEK_DIRECTION_DOWN					((FmRxSeekDirection)0)
#define FM_RX_SEEK_DIRECTION_UP						((FmRxSeekDirection)1)	

/*-------------------------------------------------------------------------------
 * FmRxRdsAfMode type
 *
 *	TBD
 *
 */
typedef FMC_UINT FmRxRdsAfSwitchMode;

#define FM_RX_RDS_AF_SWITCH_MODE_ON					((FmRxRdsAfSwitchMode)1)
#define FM_RX_RDS_AF_SWITCH_MODE_OFF				((FmRxRdsAfSwitchMode)0)


/*-------------------------------------------------------------------------------
 * FmcEmphasisFilter type
 *
 *	Represents a ( De) emphasis mode for the receiver
 *
 */
#if 0  // Duplicate definition. To be Removed 
typedef FMC_UINT FmRxEmphasisFilter;

#define FM_RX_EMPHASIS_FILTER_50_USEC 				((FmRxEmphasisFilter)0)
#define FM_RX_EMPHASIS_FILTER_75_USEC 				((FmRxEmphasisFilter)1)
#endif 

/*-------------------------------------------------------------------------------
 * FmRxMonoStereoMode Type
 *
 *	Mono / Stereo FM reception or transmission
 */
typedef  FMC_UINT  FmRxMonoStereoMode;

#define FM_RX_MONO_MODE								((FmRxMonoStereoMode)(1))
#define FM_RX_STEREO_MODE 						((FmRxMonoStereoMode)(0))

/*-------------------------------------------------------------------------------
 * FmRxAudioPath type
 *
 *	   Represents FM Rx audio path types.
 */
typedef FMC_U8 FmRxAudioPath;

 /*****************************************************************
 *
 * Rx audio path routing definitions 
 *
 ****************************************************************/

#define FM_RX_AUDIO_PATH_OFF						(0) /* Audio is switched off */
#define FM_RX_AUDIO_PATH_HEADSET					(1) /* Audio is routed to wired headset */
#define FM_RX_AUDIO_PATH_HANDSET					(2) /* Audio is routed to handset */



/*-------------------------------------------------------------------------------
 * Complete Scan MAX Num of Channels
 *
 */
#define FM_RX_COMPLETE_SCAN_MAX_NUM					(128)



/*-------------------------------------------------------------------------------
 * FmRxErrorCallBack type
 *
 *     A function of this type is called to indicate FM Errors.
 */
typedef  void (*FmRxErrorCallBack)(const FmRxStatus status);


/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FmRxEvent structure
 *
 *     Represents FM event.
 */
struct _FmRxEvent {

        /* The context for which the event is intended */
        FmRxContext     *context;

	/* Defines the event that caused the callback */
	FmRxEventType	eventType;

	/* The status of this command */
	FmRxStatus		status;

	union {
		struct {
			/* The command done */
			FmRxCmdType cmd;

			/* When no value is available (usually for Set commands), value==FMC_NO_VALUE */
			FMC_U32 value;     
	        } cmdDone;
		struct{
			/* The command done */
			FmRxCmdType cmd;

			/*in case of failure due to unavailible resources will indicate the requested (by the fm) operation*/
			ECAL_Operation operation;
			/*in case of failure due to unavailible resources will indicate resources and operations need to be freed*/
			TCCM_VAC_UnavailResourceList *ptUnavailResources;
		}audioCmdDone;
		
		struct {
			/* The new mode - Stereo/Mono */
			FmRxMonoStereoMode mode;
		} monoStereoMode;

		struct {
			/* The current PI */
			FmcRdsPiCode pi;
		} piChangedData;
		struct {
			/* The current PI */
			FmcRdsPtyCode pty;
		} ptyChangedData;
		struct {
			/* The current PI */
			FmcRdsPiCode pi;
			
			/* The size of the new AF list */
			FMC_UINT 	afListSize;
			
			/* The new AF list */
			FmcFreq 		*afList;
	        } afListData;
		
		struct {
			/* The current PI */
			FmcRdsPiCode pi;
			
			/* The frequency before the jump */
			FmcFreq tunedFreq;
			
			/* The frequency after the jump */
			FmcFreq afFreq;
	        } afSwitchData;
		
        	struct {
			/* The frequency that is currently tuned */
			FmcFreq frequency;
			
			/* The new ps Name */
			FMC_U8 *name;		

			FmcRdsRepertoire repertoire;
	        } psData;
			
		struct {
			/* 
				Indicates whether to reset the display before displaying the
				new RT text, or replace existing characters with the corresponding
				characters from the new text.
			 */
			FMC_BOOL resetDisplay;
			
			/* The length of the radio text */
			FMC_UINT len;
			
			/* The radio text */
			FMC_U8 *msg;

			/* indicate the start index of the subsequence of chars that should be replaced/set*/
			FMC_U8 startIndex;

			FmcRdsRepertoire repertoire;
		} radioTextData;
		
		struct {
			/*the bit in the mask that indicates the group type by mask*/
			FmcRdsGroupTypeMask        groupBitInMask;
			/* data of all 4 blocks packed together (2 bytes per block) */
			FMC_U8                       	groupData[8];     
		} rawRdsGroupData;

        struct {
            /*Number of channels that was found in the Scan*/
            FMC_U8 numOfChannels;
            /*Holds the Frequency data for each channel */
            FmcFreq channelsData[FM_RX_COMPLETE_SCAN_MAX_NUM];

		} completeScanData;
    } p;    

} ;


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

 /*-------------------------------------------------------------------------------
 * FM_RX_Init()
 *
 * Brief:  
 *		Initializes FM RX module.
 *
 * Description:
 *    It is usually called at system startup.
 *
 *    This function must be called by the system before any other FM RX function.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void.
 *
 * Returns:
 *		FMC_STATUS_SUCCESS - FM was initialized successfully.
 *
 *		FMC_STATUS_FAILED -  FM failed initialization.
 */
FmRxStatus FM_RX_Init(const FmRxErrorCallBack fmErrorCallback);

 /*-------------------------------------------------------------------------------
 * FM_RX_Init_Async()
 *
 * Brief:  
 *		Initializes FM RX module.
 *
 * Description:
 *    It is usually called at system startup.
 *
 *    This function must be called by the system before any other FM RX function.
 *
 * Type:
 *		Synchronous\Asynchronous
 *  
 * Generated Events:
 *		eventType=FM_RX_EVENT_CMD_DONE, with commandType == FM_RX_INIT_ASYNC
 * 		The only feilds that are valid at the event:
 *
 * 		eventType
 *		status
 *		p.cmdDone.cmd 	
 *
 * Parameters:
  *		fmInitCallback [in] - The FM RX Init event will be sent to this callback.
 *
 * Returns:
 *		FMC_STATUS_SUCCESS   -   FM was initialized successfully.
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FMC_STATUS_FAILED -  FM failed initialization.
 *
 */
FmRxStatus FM_RX_Init_Async(const FmRxCallBack fmInitCallback,const FmRxErrorCallBack fmErrorCallback);

/*-------------------------------------------------------------------------------
 * FM_Deinit()
 *
 * Brief:  
 *		Deinitializes FM RX module.
 *
 * Description:
 *		After calling this function, no FM function (RX or TX) can be called.
 *
 * Caveats:
 *		Both FM RX & TX must be detroyed before calling this function.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void
 *
 * Returns:
 *		N/A (void)
 */
FmRxStatus FM_RX_Deinit(void);

/*-------------------------------------------------------------------------------
 * FM_RX_Create()
 *
 * Brief:  
 *		Allocates a unique FM RX context.
 *
 * Description:
 *		This function must be called before any other FM RX function.
 *
 *		The allocated context should be supplied in subsequent FM RX calls.
 *		The caller must also provide a callback function, which will be called 
 *		on FM RX events.
 *
 *		Both FM RX & TX Contexts may exist concurrently. However, only one at a time may be enabled 
 *		(see FM_RX_Enable() for details)
 *
 *		Currently, only a single context may be created
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		appHandle [in] - application handle, can be a null pointer (use default application).
 *
 *		fmCallback [in] - all FM RX events will be sent to this callback.
 *		
 *		fmContext [out] - allocated FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_SUCCESS - Operation completed successfully.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_ALREADY_EXISTS - FM RX Context already created
 *
 *		FM_RX_STATUS_FM_NOT_INITIALIZED - FM Not initialized. Please call FM_Init() first
 */
FmRxStatus FM_RX_Create(FmcAppHandle *appHandle, const FmRxCallBack fmCallback, FmRxContext **fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_Destroy()
 *
 * Brief:  
 *		Releases the FM RX context (previously allocated with FM_RX_Create()).
 *
 * Description:
 *		An application should call this function when it completes using FM RX services.
 *
 *		Upon completion, the FM RX context is set to null in order to prevent 
 *		the application from an illegal attempt to keep using it.
 *
 *		This function perfomrs an orderly destruction of the context. This means that:
 *		1. If the FM RX context is enabled it is first disabled
 *
 * Generated Events:
 *		1. If the context is disabled, FM_RX_EVENT_CMD_DONE, with commandType == FM_RX_CMD_DISABLE; And
 *		2. Another event FM_RX_EVENT_CMD_DONE, with commandType == FM_RX_CMD_DESTROY
 *
 * Type:
 *		Synchronous/Asynchronous
 *
 * Parameters:
 *		fmContext [in/out] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_SUCCESS - Operation completed successfully.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 * 
 *		FM_RX_STATUS_IN_PROGRESS - This function is already in progress 
 */
FmRxStatus FM_RX_Destroy(FmRxContext **fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_Enable()
 *
 * Brief:  
 *		Enable FM RX
 *
 * Description:
 *		After calling this function, FM RX is enabled and ready for configurations.
 *
 *		This procedure will initialize the FM RX Module in the chip. This process is comrised
 *		of the following steps:
 *		1. Power on the chip and send the chip init script (only if BT radio is not on at the moment)
 *		2. Power on the FM module in the chip
 *		3. Send the FM Init Script
 *		4. Apply default values that are defined in fm_config.h
 *		5. Apply default values in the FM RX configuraion script (if such a file exists)
 *
 *		The application may call any FM RX function only after successfuly completion
 *		of all the steps listed above.
 *
 *		If, for any reason, the application decides to abort the enabling process, it should call
 *		FM_RX_Disable() any time. In that case, a single event will be sent to the application
 *		when disabling completes (there will NOT be an event for enabling)
 *
 *		EITHER FM RX or FM TX MAY BE ENABLED CONCURRENTLY, BUT NOT BOTH
 *
 * Caveats:
 *		When the receiver is enabled, it is not locked on any frequency. To actually
 *		start reception, either FM_RX_Tune() or FM_RX_Seek() must be called.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_ENABLE
 *
 * Type:
 *		Synchronous/Asynchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_FM_TX_ALREADY_ENABLED - FM TX already enabled, only one a time allowed
 * 
 *		FM_RX_STATUS_IN_PROGRESS - This function is already in progress 
 *
 *		FM_RX_STATUS_CONTEXT_ALREADY_ENABLED - The context is already enabled. Please disable it first.
 */
FmRxStatus FM_RX_Enable(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_Disable()
 *
 * Brief:  
 *		Disable FM RX
 *
 * Description:
 *		Performs an ordely disabling of the FM RX module.
 * 
 *		FM RX settings will be lost and must be re-configured after the next FM RX power on (via FM_RX_Enable()).
 *
 *		If this function is called while an FM_RX_Enable() is in progress, it will abort the enabling process.
 *
 * Generated Events:
 *		1. If the function is asynchronous (aborts enabling process), an event
 *			with event type==FM_RX_EVENT_CMD_DONE,  command type==FM_RX_CMD_DISABLE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_SUCCESS - Operation completed successfully.
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 * 
 *		FM_RX_STATUS_IN_PROGRESS - This function is already in progress 
 */
FmRxStatus FM_RX_Disable(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetBand()
 *
 * Brief:  
 *		Set the radio band.
 *
 * Description:
 *		Choose between Europe/US band (87.5-108MHz) and Japan band (76-90MHz).
 *
 * Default Values: 
 *		Europe/US
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_BAND
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		band [in] - 	the band to set
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled 
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetBand(FmRxContext *fmContext, FmcBand band);

/*-------------------------------------------------------------------------------
 * FM_RX_GetBand()
 *
 * Brief:  
 *		Returns the current band.
 *
 * Description:
 *		Retrieves the currently set band
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_BAND
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetBand(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetMonoStereoMode()
 *
 * Brief:  
 *		Set the Mono/Stereo mode.
 *
 * Description:
 *		Set the FM RX to Mono or Stereo mode. If stereo mode is set, but no stereo is detected then mono mode will be chosen.
 *		If mono will be set then mono output will be forced.When change will be detected(in stereo mode) MONO <-> STEREO 
 *		FM_RX_EVENT_MONO_STEREO_MODE_CHANGED event will be recieved.
 *
 * Default Values: 
 *		Stereo
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_MONO_STEREO_MODE
 *		2. Event type==FM_RX_EVENT_MONO_STEREO_MODE_CHANGED, with command type == FM_RX_CMD_NONE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		mode [in] - The mode to set: FM_STEREO_MODE or FM_MONO_MODE.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetMonoStereoMode(FmRxContext *fmContext, FmRxMonoStereoMode mode);

/*-------------------------------------------------------------------------------
 * FM_RX_GetMonoStereoMode()
 *
 * Brief: 
 *		Returns the current Mono/Stereo mode.
 *
 * Description:
 *		Returns the current Mono/Stereo mode.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_MONO_STEREO_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetMonoStereoMode(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetMuteMode()
 *
 * Brief:  
 *		Set the mute mode.
 *
 * Description:
 *		Sets the radio mute mode:
 *		mute: Audio is muted completely
 *		attenuate: Audio is attenuated
 *		not muted: Audio volume is set according to the configured volume level
 *
 *		The volume level is unaffected by this function.
 *
 * Default Values: 
 *		Not Muted
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_MUTE_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		mode [in] - the mode to set
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetMuteMode(FmRxContext *fmContext, FmcMuteMode mode);

/*-------------------------------------------------------------------------------
 * FM_RX_GetMuteMode()
 *
 * Brief:  
 *		Returns the current mute mode.
 *
 * Description:
 *		Returns the current mute mode
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_MUTE_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetMuteMode(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetRfDependentMute()
 *
 * Brief:  
 *		Enable/Disable the RF dependent mute feature.
 *
 * Description:
 *		This function turns on or off the RF-Dependent Mute mode. Turning it on causes
 *		the audio to be attenuated according to the RSSI. The lower the RSSI the bigger
 *		the attenuation.
 *
 *		This function doesn't affect the mute mode set via FM_RX_SetMuteMode()
 *
 * Default Values: 
 *		Turned Off
 
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_RF_DEPENDENT_MUTE_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 *		mode [in] - The mute mode
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetRfDependentMuteMode(FmRxContext *fmContext, FmRxRfDependentMuteMode mode);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRfDependentMute()
 *
 * Brief:  
 *		Returns the current RF-Dependent mute mode.
 *
 * Description:
 *		Returns the current RF-Dependent mute mode
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RF_DEPENDENT_MUTE_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetRfDependentMute(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetRssiThreshold()
 *
 * Brief:  
 *		Sets the RSSI tuning threshold
 *
 * Description:
 *		Received Signal Strength Indication (RSSI) is a measurement of the power present in a 
 *		received radio signal. RSSI levels range from 1 (no signal) to 127 (maximum power). 
 *
 *		This function sets the RSSI threshold level. Valid threshold values are from (1X1.5051 dBuV) - (127X1.5051 dBuV)
 *
 * 		The threshold is used in the following processes:
 *		1. When seeking a station (FM_RX_Seek()). The received RSSI must be >= threshold for 
 *			a frequency to be considered as a valid station.
 *		2. In the AF process. Switching to an AF is triggered when the RSSI of the tuned frequency
 *			falls below the RSSI threshold.
 *
 * Default Values: 
 *		7X1.5051 dBuV
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_RSSI_THRESHOLD
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 *		threshold [in] - The threshold level should be Bigger then (0).
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetRssiThreshold(FmRxContext *fmContext, FMC_INT threshold);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRssiThreshold()
 *
 * Brief:  
 *		Returns the current RSSI threshold.
 *
 * Description:
 *		Returns the current RSSI threshold
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RSSI_THRESHOLD
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetRssiThreshold(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetDeEmphasisFilter()
 *
 * Brief: 
 *		Selects the receiver's de-emphasis filter
 *
 * Description:
 *		The FM transmitting station uses a pre-emphasis filter to provide better S/N ratio, 
 *		and the receiver uses a de-emphasis filter corresponding to the transmitter to restore 
 *		the original audio. This varies per country. Therefore the filter has to configure the 
 *		appropriate filter accordingly.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_DEEMPHASIS_FILTER
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * 		filter [in] - the de-emphasis filter to use
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_NOT_SUPPORTED - The requested filter value is not supported
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetDeEmphasisFilter(FmRxContext *fmContext, FmcEmphasisFilter filter);

/*-------------------------------------------------------------------------------
 * FM_RX_GetDeEmphasisFilter()
 *
 * Brief:  
 *		Returns the current de-emphasis filter
 *
 * Description:
 *		Returns the current de-emphasis filter
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_DEEMPHASIS_FILTER
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetDeEmphasisFilter(FmRxContext *fmContext);


/*-------------------------------------------------------------------------------
 * FM_RX_SetVolume()
 *
 * Brief:  
 *		Set the gain level of the audio left & right channels
 *
 * Description:
 *		Set the gain level of the audio left & right channels, 0 - 70 (0 is no gain, 70 is maximum gain)
 *
 * Default Values: 
 *		TBD
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_VOLUME
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		level [in] - the level to set: 0 (weakest) - 70 (loudest).
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetVolume(FmRxContext *fmContext, FMC_UINT level);

/*-------------------------------------------------------------------------------
 * FM_RX_GetVolume()
 *
 * Brief:  
 *		Returns the current volume level.
 *
 * Description:
 *		Returns the current volume level
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_VOLUME
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetVolume(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_Tune()
 *
 * Brief:  
 *		Tune the receiver to the specified frequency
 *
 * Description:
 *		Tune the receiver to the specified frequency. This is useful when setting the receiver
 *		to a preset station. The receiver will be tuned to the specified frequency regardless
 *		of any signal present.
 *
 *		Calling this function stops other frequency related processes in progress (seek, switch to AF)
 *		In that case, the application will receive a single event, reporting the tune result. The application
 *		will NOT recreive any event regarding the process that is being stopped.
 *
 * Default Values: 
 *		NO DEFAULT VALUE AVAILABLE
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_TUNE
 *
 * Type:
 *		Synchronous/Asynchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		freq [in] - the frequency to set in KHz.
 *
 *					The value must match the configured band.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_Tune(FmRxContext *fmContext, FmcFreq freq);


/*-------------------------------------------------------------------------------
 * FM_RX_IsValidChannel()
 *
 * Brief:  
 *		Is the channel valid.
 *
 * Description:
 *		
 *           Verify that the tune channel is valid.Acorrding to 2 parameters:
 *              1.Channel that was set via the API FM_RX_Tune()
 *              2. RSSI search level that was set via the API FM_RX_SetRssiThreshold
 *                 Deafult RSSI value is : 7.                  
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_IS_CHANNEL_VALID
 *
 * CMD_DONE Value:
 *
 *                     1 = Valid.
 *                     0 = Not Valid.
 *
 *
 * Type:
 *		Synchronous/Asynchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_IsValidChannel(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_GetTunedFrequency()
 *
 * Brief:  
 *		Returns the currently tuned frequency.
 *
 * Description:
 *		Returns the currently tuned frequency.
 *
 * Caveats:
 *		TBD: What frequency to return when AF switch / seek are executing or pending?
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_TUNED_FREQUENCY
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_NO_VALUE_AVAILABLE - Frequency was never set (first call FM_RX_Tune())
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetTunedFrequency(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_Seek()
 *
 * Brief: 
 *		Seeks the next good station or stop the current seek.
 *
 * Description:
 *		Seeks up/down for a frequency with an RSSI that is higher than the configured
 *		RSSI threshold level (set via FM_RX_SetRssiThreshold()).
 *
 *		New frequencies are scanned starting from the currently tuned frequency, and
 *		progressing according to the specified direction, in steps of 100 kHz.
 *
 *		When this function is called for the first time after enabling FM RX (via
 *		FM_RX_Enable()), seeking will begin from the beginning of the band limit.
 *
 *		The seek command stops when one of the following conditions occur:
 *		1. A frequency with compliant RSSI was found. That frequency will be the new
 *			tuned frequency.
 *		2. The band limit was reached. The band limit will be the new tuned frequency.
 *		3. FM_RX_StopSeek() was called. The last scanned frequency will be the new
 *			tuned frequency.
 *		4. TBD: FM_RX_Tune() was called. The new tuned frequency will be the one specified
 *			in the FM_RX_Tune() call.
 *		
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SEEK, and
 *			a. If the seek completed successfully, the status will be FM_RX_STATUS_SUCCESS, and
 *				value== newly tuned frequency
 *			b. If the seek stopped because the band limit was reached, the status will be
 *				FM_RX_STATUS_SEEK_REACHED_BAND_LIMIT, and value==band limit frequency.
 *
 *		If the seek stopped due to a FM_RX_StopSeek() command that was issued, a single
 *		event will be issued as described in the FM_RX_StopSeek() description.
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 *		direction [in] - The seek direction: FM_RX_DIR_DOWN or FM_RX_DIR_UP.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_Seek(FmRxContext *fmContext, FmRxSeekDirection direction);

/*-------------------------------------------------------------------------------
 * FM_RX_StopSeek()
 *
 * Brief: 
 *		Stops a seek in progress
 *
 * Description:
 *		Stops a seek that was started via FM_RX_Seek()
 *
 *		The new tuned frequency will be the last scanned frequency. The new frequency
 *		will be indicated to the application via the generated event.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_STOP_SEEK. 
 *			command value == newly tuned frequency (last scanned frequency)
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_SEEK_IS_NOT_IN_PROGRESS - No seek currently in progress
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_StopSeek(FmRxContext *fmContext);


/*-------------------------------------------------------------------------------
 * FM_RX_CompleteScan()
 *
 * Brief: 
 *		Perfrom a complete Scan on the Band.
 *
 * Description:
 *		
 *		Perfrom a complete Scan on the Band and return a list of frequnecies that are valid acorrding to the RSSI.
 *
 * Generated Events:
 *		Event type==FM_RX_EVENT_COMPLETE_SCAN_DONE.
 *		
 *
 * Relevant Data:
 *
 *       "p.completeScanData"
 *       "p.completeScanData.numOfChannels" - contains the number of channels that was found in the Scan.
 *       "p.completeScanData.channelsData" - Contains the Freq in MHz for each channel.
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_CompleteScan(FmRxContext *fmContext);


/*-------------------------------------------------------------------------------
 * FM_RX_GetCompleteScanProgress()
 *
 * Brief: 
 *		Get the progress from the Complte Scan.
 *
 * Description:
 *		
 *		Get the progress from the Complte Scan.This function will be valid only after calling the FM_RX_CompleteScan()
 *      When calling this function without calling the FM_RX_CompleteScan() it will return FM_RX_STATUS_COMPLETE_SCAN_IS_NOT_IN_PROGRESS.
 *
 * Generated Events:
 *		Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_COMPLETE_SCAN_PROGRESS. 
 *		command value = US band if you read back 87500 MHz it would be 0% and 108000 MHz would be 100%. 
 *		                           JAPAN band if you read back 76000 MHz it would be 0% and 90000 MHz would be 100%. 
 *		
 *
 * Relevant Data:
 *
 *       "p.cmdDone"
 *       "p.cmdDone.cmd" - FM_RX_CMD_COMPLETE_SCAN_PROGRESS cmd type.
 *       "p.cmdDone.value" - Contains Freq Progress in MHz .
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *
 *
 *		FM_RX_STATUS_COMPLETE_SCAN_IS_NOT_IN_PROGRESS - No Complete Scan currently in progress
 *
 */
FmRxStatus FM_RX_GetCompleteScanProgress(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_StopCompleteScan()
 *
 * Brief: 
 *		Stop the Complte Scan.
 *
 * Description:
 *		
 *		Stop the Complte Scan.This function will be valid only after calling the FM_RX_CompleteScan()
 *      When calling this function without calling the FM_RX_CompleteScan() it will return FM_RX_STATUS_COMPLETE_SCAN_IS_NOT_IN_PROGRESS.
 *
 * Generated Events:
 *		Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_STATUS_COMPLETE_SCAN_STOPPED. 
 *		command value = Frequency in MHz pirior to the FM_RX_CompleteScan() request.
 *
 * Relevant Data:
 *
 *       "p.cmdDone"
 *       "p.cmdDone.cmd" - FM_RX_STATUS_COMPLETE_SCAN_STOPPED cmd type.
 *       "p.cmdDone.value" - Contains Freq in MHz that pirior to the Complete Scan.
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_COMPLETE_SCAN_IS_NOT_IN_PROGRESS - No Complete Scan currently in progress
 *
 */

FmRxStatus FM_RX_StopCompleteScan(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRssi()
 *
 * Brief:  
 *		Returns the current RSSI.
 *
 * Description:
 *		Returns the current RSSI of the tuned frequency.
 *
 *		RSSI values range from -128 (no signal) to 127 (maximum power)
 *
 *		TBD: What to do in case an AF search / Seek is in progress?
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RSSI
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_NO_VALUE_AVAILABLE - Frequency was never set (first call FM_RX_Tune())
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetRssi(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_EnableRds()
 *
 * Brief:  
 *		Enables RDS reception.
 *
 * Description:
 *		Starts RDS data reception. RDS data reception will be active when the receiver is actually tuned to some frequency.
 *		Only when RDS enabled , RDS field (PI,AF,PTY,PS,RT,repertoire,...)will be parsed.
 *
 * Default Values: 
 *		RDS Is disabled
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_ENABLE_RDS
 *		2. Event type==FM_RX_EVENT_PI_CODE_CHANGED, with command type == FM_RX_CMD_NONE
 *		3. Event type==FM_RX_EVENT_AF_LIST_CHANGED, with command type == FM_RX_CMD_NONE
 *		4. Event type==FM_RX_EVENT_PS_CHANGED, with command type == FM_RX_CMD_NONE
 *		5. Event type==FM_RX_EVENT_RADIO_TEXT, with command type == FM_RX_CMD_NONE
 *		6. Event type==FM_RX_EVENT_RAW_RDS, with command type == FM_RX_CMD_NONE
 *		7. Event type==FM_RX_EVENT_PTY_CODE_CHANGED, with command type == FM_RX_CMD_NONE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_EnableRds(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_DisableRds()
 *
 * Brief:  
 *		Disables RDS reception.
 *
 * Description:
 *		Stops RDS data reception.
 *
 *		All RDS configurations are retained.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_DISABLE_RDS
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_DisableRds(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetRdsSystem()
 *
 * Brief: 
 *		Choose the RDS mode - RDS/RBDS
 *
 * Description:
 *		RDS is the system used in Europe.  RBDS is a new system used in the US (although most stations 
 *		in the US transmit RDS, a few have started to transmit RBDS).  RBDS includes all RDS specifications, 
 *		with some additional information.
 *
 * Default Values: 
 *		RDS 
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_RDS_SYSTEM
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * 		rdsSystem [in] - The RDS system to use.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetRdsSystem(FmRxContext *fmContext, FmcRdsSystem	rdsSystem);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRdsSystem()
 *
 * Brief:  
 *		Returns the current RDS System in use.
 *
 * Description:
 *		Returns the current RDS System in use
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RDS_SYSTEM
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetRdsSystem(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetRdsGroupMask()
 *
 * Brief: 
 *		Selects which RDS gorups to report in a raw RDS event
 *
 * Description:
 *		The application will be called back whenever RDS data of one of the requested types is received.
 *
 *		To disable these notifications, FM_RDS_GROUP_TYPE_MASK_NONE should be specified.
 *		To receive an event for any group, FM_RDS_GROUP_TYPE_MASK_ALL should be specified.
 *
 * Default Values: 
 *		ALL (FM_RDS_GROUP_TYPE_MASK_ALL)
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_RDS_GROUP_MASK
 *
 * Type:
 *		Asynchronous\Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * 		groupMask [in] - Group mask
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 */
FmRxStatus FM_RX_SetRdsGroupMask(FmRxContext *fmContext, FmcRdsGroupTypeMask groupMask);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRdsGroupMask()
 *
 * Brief: 
 *		Returns the current RDS group mask
 *
 * Description:
 *		Returns the current RDS group mask
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RDS_GROUP_MASK
 *
 * Type:
 *		Asynchronous\Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 * 		groupMask [out] - The current group mask
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 */
FmRxStatus FM_RX_GetRdsGroupMask(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_SetRdsAFSwitchMode()
 *
 * Brief: 
 *		Sets the RDS AF feature On or Off
 *
 * Description:
 *		Turns the 'Alternative Frequency' (AF) automatic switching feature of the RDS on or off. 
 *
 *		When RDS is enabled, the receiver obtains, via RDS, a list of one or more alternate
 *		frequencies published by the currently tuned station (if available).
 *		
 *		When AF switching is turned on, and the RSSI falls beneath the configured RSSI threshold level
 *		(configured via FM_RX_SetRssiThreshold()), an attempt will be made to switch to
 *		the first frequency on the list, then to the second, and so on. The process stops
 *		when one of the following conditions are met:
 *		1. The attempt succeeded. In that case the newly tuned frequency is the alternate frequency.
 *		2. The list is exhausted. In that case, the tuned frequency is unaltered.
 *
 *		An attempt to switch is successful if all of the following conditions are met:
 *		1. The RSSI of the alternate frequency is greater than or equal to the RSSI threshold level.
 *		2. The RDS PI (Program Indicator) of the alternate frequency matches that of the tuned
 *			frequency.
 *
 *		TBD: How to handle seek and tune commands that are issued while AF is in progress?
 *
 * Caveats:
 *		AF automatic switching can operate only when RDS reception is enabled (via FM_RX_EnableRds()) 
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_RDS_AF_SWITCH_MODE
 *		2. Event type==FM_RX_EVENT_AF_SWITCH_COMPLETE, with command type == FM_RX_CMD_NONE
 *		3. Event type==FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED, with command type == FM_RX_CMD_NONE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM context.
 *
 *		mode [in] - AF feature On or Off
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetRdsAfSwitchMode(FmRxContext *fmContext, FmRxRdsAfSwitchMode mode);

/*-------------------------------------------------------------------------------
 * FM_RX_GetRdsAFSwitchMode()
 *
 * Brief:  
 *		Returns the current RDS AF Mode.
 *
 * Description:
 *		Returns the current RDS AF Mode
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_RDS_AF_SWITCH_MODE
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetRdsAfSwitchMode(FmRxContext *fmContext);


/*-------------------------------------------------------------------------------
 * FM_RX_SetChannelSpacing()
 *
 * Brief:  
 *		Sets the Seek channel spacing interval.
 *
 * Description:
 *		When performing seek operation there is in interval spacing between 
 *		the stations that are searched.
 *
 * Default Value: 
 *		FMC_CHANNEL_SPACING_100_KHZ.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_SET_CHANNEL_SPACING
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *		channelSpacing [in] - Channel spacing.
 *
 * Returns:
 * 
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_SetChannelSpacing(FmRxContext *fmContext,FmcChannelSpacing channelSpacing);

/*-------------------------------------------------------------------------------
 * FM_RX_GetChannelSpacing()
 *
 * Brief:  
 *		Gets the Seek channel spacing interval.
 *
 * Description:
 *		Gets the Seek channel spacing interval.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_CHANNEL_SPACING
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 * 
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetChannelSpacing(FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_GetFwVersion()
 *
 * Brief:  
 *		Gets the FW version.
 *
 * Description:
 *		Gets the FW version.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_GET_FW_VERSION
 *
 * CMD_DONE Value in Hex:
 *                                When read in decimal, the digit in
 *                                 thousands place indicated ROM version,
 *                                 digits in hundreds and tens place indicate
 *                                 Patch version and the digit in units place
 *                                 indicates Minor Derivative
 *             
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_GetFwVersion (FmRxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_RX_ChangeAudioTarget()
 *
 * Brief:  
 *		Changes the Audio Targets.
 *
 * Description:
 *		Changes the Audio Targets. This command can be called both when Audio enabled and when it is disabled.
 *		
 *		If Audio Routing is disabled it associates the FM operation with the resources requested in the VAC - 
 *		and the it should succeed if the resource was defined in the optional resources of the FM RX opperation 
 *		(it will not occupy the resources).
 *		
 *		If Audio Routing is enabled  it associates the FM operation with the resources requested in the VAC -
 *		this operation will occupy the resources and therefore can fail if the resources are unavailble, In this  
 *		case the unavailible resource list will be returned in the FM_RX_EVENT_CMD_DONE event with the 
 *		status code FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES.
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_CHANGE_AUDIO_TARGET
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 *		rxTagets[in] - The requested Audio Targets Mask.
 *
 *		digitalConfig[in] - The digital configuration (will be used only if digital target requested in the rxTagets).
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 *		FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES - Resources requested by operation unavailible.
 */
FmRxStatus FM_RX_ChangeAudioTarget (FmRxContext *fmContext, FmRxAudioTargetMask rxTagets, ECAL_SampleFrequency digitalConfig);
/*-------------------------------------------------------------------------------
 * FM_RX_ChangeDigitalTargetConfiguration()
 *
 * Brief:  
 *		Change the digital audio configuration. 
 *
 * Description:
 *
 *		Change the digital audio configuration. This function should be called only if a 
 *		digital audio target was set (by defualt or by a call to FM_RX_ChangeAudioTarget).
 *		The digital configuration will apply on every digital target set (if few will be possible in the future). 
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *		digitalConfig[in] - sample frequency for the digital resources.
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_ChangeDigitalTargetConfiguration(FmRxContext *fmContext, ECAL_SampleFrequency digitalConfig);
/*-------------------------------------------------------------------------------
 * FM_RX_EnableAudioRouting()
 *
 * Brief:  
 *		Enable the Audio routing.
 *
 * Description:
 *		
 *		Enables the audio routing according to the VAC configuration. if 
 *		FM_RX_ChangeAudioTarget()/FM_RX_ChangeDigitalTargetConfiguration() was called the routing will 
 *		be according to the configurations done by these calles.
 *		
 *		This command will start the VAC operations (and occupy the resources). if digital target was selected to be fm over Sco/A3dp 
 *		
 *		
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_ENABLE_AUDIO
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *		
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 *		FM_RX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES - Resources requested by operation unavailible.
 */
FmRxStatus FM_RX_EnableAudioRouting (FmRxContext *fmContext);
/*-------------------------------------------------------------------------------
 * FM_RX_DisableAudioRouting()
 *
 * Brief:  
 *		Disables the audio routing.
 *
 * Description:
 *		Disables the audio routing. This command will also Stop the VAC operations (and realese the resources).
 *
 * Generated Events:
 *		1. Event type==FM_RX_EVENT_CMD_DONE, with command type == FM_RX_CMD_DISABLE_AUDIO
 *
 * Type:
 *		Asynchronous/Synchronous
 *
 * Parameters:
 *		fmContext [in] - FM RX context.
 *
 * Returns:
 *		FM_RX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *								the application upon completion.
 *
 *		FM_RX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *		FM_RX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *		FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *														execution in operations queue.
 */
FmRxStatus FM_RX_DisableAudioRouting (FmRxContext *fmContext);

#endif /* ifndef __FM_RX_H */

