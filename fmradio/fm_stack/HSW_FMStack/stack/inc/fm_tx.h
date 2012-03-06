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
*   FILE NAME:      fm_tx.h
*
*   BRIEF:          This file defines the API of the FM TX stack.
*
*   DESCRIPTION:    General
*
*   TBD: 
*   1. Add some overview of the API
*   2. List of funtions, divided into groups, with their short descriptions.
*   3. Add API sequences for Common use-cases:
*       1.1. Enable + Tune + Power On
*       1.2. RDS Transmission
*       1.3 TBD - Others
*                   
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/

#ifndef __FM_TX_H
#define __FM_TX_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_common.h"
#include "ccm_audio_types.h"

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
/*
*   FM Tx can have only one source :
*   -Analog
*   -Digital :
*           - I2S
*           - PCM
*/
typedef enum 
{
    FM_TX_AUDIO_SOURCE_I2SH = CAL_RESOURCE_I2SH,
    FM_TX_AUDIO_SOURCE_PCMH = CAL_RESOURCE_PCMH,
    FM_TX_AUDIO_SOURCE_FM_ANALOG = CAL_RESOURCE_FM_ANALOG,
} FmTxAudioSource;

/*-------------------------------------------------------------------------------
 * FmTxContext structure
 *
 *     Each application must create a context before starting to use the FM TX 
 */
typedef struct _FmTxContext FmTxContext;

/*-------------------------------------------------------------------------------
 * FmTxEvent type
 *
 *  FmTxEvent Struct Forward Declaration
 */
typedef struct _FmTxEvent FmTxEvent;

/*-------------------------------------------------------------------------------
 * FmCallBack type
 *
 *     A function of this type is called to indicate FM TX events.
 */
typedef void (*FmTxCallBack)(const FmTxEvent *event);

/*-------------------------------------------------------------------------------
 * FmTxPowerLevel Type
 *
 */
typedef  FMC_UINT    FmTxPowerLevel;

#define     FM_TX_POWER_LEVEL_MIN                   ((FmTxPowerLevel)0)
#define     FM_TX_POWER_LEVEL_MAX                   ((FmTxPowerLevel)31)

/*-------------------------------------------------------------------------------
 * FmTxRdsTransmissionMode Type
 *
 * TBD
 */
typedef  FMC_UINT  FmTxRdsTransmissionMode;

#define FMC_RDS_TRANSMISSION_MANUAL             ((FmTxRdsTransmissionMode)0)
#define FMC_RDS_TRANSMISSION_AUTOMATIC          ((FmTxRdsTransmissionMode)1)


/*-------------------------------------------------------------------------------
 * FmTxRdsTransmittedGroupsMask type
 *
 *    Indicates which RDS fields are transmitted simultaneously.
 *
 *  It is a bit mask. A set bit indicates the associated field is transmitted. It is given as an argument 
 *  in the FM_TX_SetRdsTransmittedGroupsMask() call.
 *
 *  Currently, the following fields are controlled:
 *  1. Program Service Name (PS)
 *  2. Radio Text (RT)
 *
 *  At the moment, only one of them (PS or RT) may be transmitted simultaneosly
 */
typedef FMC_U32 FmTxRdsTransmittedGroupsMask;

/* RDS Program Service Name */
#define FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK       ((FmTxRdsTransmittedGroupsMask)(0x00000001))

/* RDS Radio Text  */
#define FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK       ((FmTxRdsTransmittedGroupsMask)(0x00000002))

/* RDS Radio Text  */
#define FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK      ((FmTxRdsTransmittedGroupsMask)(0x00000004))

/*-------------------------------------------------------------------------------
 * FmTxEventType type
 *
 *  Contains the type of the event that the application receives from the FM TX module
 *
 */
typedef FMC_UINT FmTxEventType;

/* Event type for completed commands */
#define FM_TX_EVENT_CMD_DONE                    ((FmTxEventType)0)

/*-------------------------------------------------------------------------------
 * FmTxMonoStereoMode Type
 *
 *  Mono / Stereo FM reception or transmission
 */
typedef  FMC_UINT  FmTxMonoStereoMode;

#define FM_TX_MONO_MODE                             ((FmTxMonoStereoMode)(0))
#define FM_TX_STEREO_MODE                       ((FmTxMonoStereoMode)(1))

/*-------------------------------------------------------------------------------
 * FmTxCmdType type
 *
 *  Contains the type of the completed command for which the event is sent to the application
 *
 */
typedef FMC_UINT FmTxCmdType;

#define FM_TX_CMD_DESTROY                           ((FmTxCmdType)0)    /* Destroy command */
#define FM_TX_CMD_ENABLE                            ((FmTxCmdType)1)    /* Enable command */
#define FM_TX_CMD_DISABLE                           ((FmTxCmdType)2)    /* Disable command */
#define FM_TX_CMD_SET_INTERRUPT_MASK                ((FmTxCmdType)5)    /* Set Interrupt mask register */
#define FM_TX_CMD_GET_INTERRUPT_SATUS               ((FmTxCmdType)6)    /* Get Interrupt status register */


#define FM_TX_CMD_START_TRANSMISSION                ((FmTxCmdType)7)    /* Start Transmission command */
#define FM_TX_CMD_STOP_TRANSMISSION             ((FmTxCmdType)8)    /* Stop Transmission command */
#define FM_TX_CMD_TUNE                              ((FmTxCmdType)9)    /* Get Tuned Frequency command */
#define FM_TX_CMD_GET_TUNED_FREQUENCY           ((FmTxCmdType)10)   /* Get Tuned Frequency command */
#define FM_TX_CMD_SET_MONO_STEREO_MODE          ((FmTxCmdType)11)   /* Set Mono/Stereo command */
#define FM_TX_CMD_GET_MONO_STEREO_MODE          ((FmTxCmdType)12)   /* Get Mono/Stereo command */
#define FM_TX_CMD_SET_POWER_LEVEL               ((FmTxCmdType)13)   /* Set Power Level command */
#define FM_TX_CMD_GET_POWER_LEVEL               ((FmTxCmdType)14)   /* Get Power Level command */
#define FM_TX_CMD_SET_MUTE_MODE                 ((FmTxCmdType)15)   /* Set Mute Mode command */
#define FM_TX_CMD_GET_MUTE_MODE                 ((FmTxCmdType)16)   /* Set Mute Mode command */
#define FM_TX_CMD_ENABLE_RDS                        ((FmTxCmdType)18)   /* Enable RDS command */
#define FM_TX_CMD_DISABLE_RDS                       ((FmTxCmdType)19)   /* Disable RDS command */
#define FM_TX_CMD_SET_RDS_TRANSMISSION_MODE ((FmTxCmdType)20)   /* Set RDS Transmission Mode command */
#define FM_TX_CMD_GET_RDS_TRANSMISSION_MODE ((FmTxCmdType)21)   /* Get RDS Transmission Mode command */
#define FM_TX_CMD_SET_RDS_AF_CODE               ((FmTxCmdType)22)   /* Set RDS AF Mode command */
#define FM_TX_CMD_GET_RDS_AF_CODE               ((FmTxCmdType)23)   /* Get RDS AF Mode command */
#define FM_TX_CMD_SET_RDS_PI_CODE               ((FmTxCmdType)26)   /* Set RDS PI Code command */
#define FM_TX_CMD_GET_RDS_PI_CODE               ((FmTxCmdType)27)   /* Get RDS PI Code command */
#define FM_TX_CMD_SET_RDS_PTY_CODE              ((FmTxCmdType)28)   /* Set RDS PTY Code command */
#define FM_TX_CMD_GET_RDS_PTY_CODE              ((FmTxCmdType)29)   /* Set RDS PTY Code command */
#define FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE       ((FmTxCmdType)30)   /* Set RDS Text Repertoire command */
#define FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE       ((FmTxCmdType)31)   /* Set RDS Text Repertoire command */
#define FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE       ((FmTxCmdType)32)   /* Set RDS Text Scroll Mode command */
#define FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE       ((FmTxCmdType)33)   /* Set RDS Text Scroll Mode command */
#define FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED      ((FmTxCmdType)34)   /* Set RDS Text Scroll Speed command */
#define FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED      ((FmTxCmdType)35)   /* Set RDS Text Scroll Speed command */
#define FM_TX_CMD_SET_RDS_TEXT_PS_MSG           ((FmTxCmdType)38)   /* Set RDS Text PS Message command */
#define FM_TX_CMD_GET_RDS_TEXT_PS_MSG           ((FmTxCmdType)39)   /* Set RDS Text PS Message command */
#define FM_TX_CMD_SET_RDS_TEXT_RT_MSG           ((FmTxCmdType)40)   /* Set RDS Text RT Message command */
#define FM_TX_CMD_GET_RDS_TEXT_RT_MSG           ((FmTxCmdType)41)   /* Set RDS Text RT Message command */
#define FM_TX_CMD_SET_RDS_TRANSMITTED_MASK      ((FmTxCmdType)42)   /* Set RDS Transmitted Mask command */
#define FM_TX_CMD_GET_RDS_TRANSMITTED_MASK      ((FmTxCmdType)43)   /* Get RDS Transmitted Mask command */
#define FM_TX_CMD_SET_RDS_TRAFFIC_CODES         ((FmTxCmdType)44)   /* Set RDS Traffic Codes command */
#define FM_TX_CMD_GET_RDS_TRAFFIC_CODES         ((FmTxCmdType)45)   /* Get RDS Traffic Codes command */
#define FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG ((FmTxCmdType)46)   /* Set RDS Music Speech command */
#define FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG ((FmTxCmdType)47)   /* Get RDS Music Speech command */
#define FM_TX_CMD_SET_PRE_EMPHASIS_FILTER       ((FmTxCmdType)50)   /* Set Pre-Emphasis Filter command */
#define FM_TX_CMD_GET_PRE_EMPHASIS_FILTER       ((FmTxCmdType)51)   /* Get Pre-Emphasis Filter command */
#define FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE     ((FmTxCmdType)52)   /* Set ECC command */
#define FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE     ((FmTxCmdType)53)   /* Get ECC command */
#define FM_TX_CMD_WRITE_RDS_RAW_DATA            ((FmTxCmdType)54)   /* Write Raw RDS command */
#define FM_TX_CMD_READ_RDS_RAW_DATA             ((FmTxCmdType)55)   /* Read Raw RDS command */
#define FM_TX_CMD_CHANGE_AUDIO_SOURCE           ((FmTxCmdType)56)   /* Write Raw RDS command */
#define FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION            ((FmTxCmdType)57)   /* Read Raw RDS command */

#define FM_TX_CMD_INIT            ((FmTxCmdType)58)   /* */
#define FM_TX_CMD_DEINIT            ((FmTxCmdType)59)   /* */



#define FM_TX_LAST_CMD_TYPE                     (FM_TX_CMD_DEINIT)
#define FM_TX_INVALID_CMD_TYPE                  (FM_TX_LAST_CMD_TYPE + 1)

/*-------------------------------------------------------------------------------
 * FmTxStatus type
 *
 *  FM TX Status Codes
 *
 */
typedef FMC_UINT FmTxStatus;

/*
    Status Codes that are common to RX & TX

    Their values must equal those of the corresponding common code
*/
#define FM_TX_STATUS_SUCCESS                            ((FmTxStatus)FMC_STATUS_SUCCESS)
#define FM_TX_STATUS_FAILED                         ((FmTxStatus)FMC_STATUS_FAILED)
#define FM_TX_STATUS_PENDING                            ((FmTxStatus)FMC_STATUS_PENDING)
#define FM_TX_STATUS_INVALID_PARM                   ((FmTxStatus)FMC_STATUS_INVALID_PARM)
#define FM_TX_STATUS_IN_PROGRESS                        ((FmTxStatus)FMC_STATUS_IN_PROGRESS)
#define FM_TX_STATUS_NOT_APPLICABLE                 ((FmTxStatus)FMC_STATUS_NOT_APPLICABLE)
#define FM_TX_STATUS_NOT_SUPPORTED                  ((FmTxStatus)FMC_STATUS_NOT_SUPPORTED)
#define FM_TX_STATUS_INTERNAL_ERROR                 ((FmTxStatus)FMC_STATUS_INTERNAL_ERROR)
#define FM_TX_STATUS_TRANSPORT_INIT_ERR             ((FmTxStatus)FMC_STATUS_TRANSPORT_INIT_ERR)
#define FM_TX_STATUS_HARDWARE_ERR                   ((FmTxStatus)FMC_STATUS_HARDWARE_ERR)
#define FM_TX_STATUS_NO_VALUE_AVAILABLE             ((FmTxStatus)FMC_STATUS_NO_VALUE_AVAILABLE)
#define FM_TX_STATUS_CONTEXT_DOESNT_EXIST           ((FmTxStatus)FMC_STATUS_CONTEXT_DOESNT_EXIST)
#define FM_TX_STATUS_CONTEXT_NOT_DESTROYED          ((FmTxStatus)FMC_STATUS_CONTEXT_NOT_DESTROYED)
#define FM_TX_STATUS_CONTEXT_NOT_ENABLED            ((FmTxStatus)FMC_STATUS_CONTEXT_NOT_ENABLED)
#define FM_TX_STATUS_CONTEXT_NOT_DISABLED           ((FmTxStatus)FMC_STATUS_CONTEXT_NOT_DISABLED)
#define FM_TX_STATUS_NOT_DE_INITIALIZED             ((FmTxStatus)FMC_STATUS_NOT_DE_INITIALIZED)
#define FM_TX_STATUS_NOT_INITIALIZED                    ((FmTxStatus)FMC_STATUS_NOT_INITIALIZED)
#define FM_TX_STATUS_TOO_MANY_PENDING_CMDS          ((FmTxStatus)FMC_STATUS_TOO_MANY_PENDING_CMDS)
#define FM_TX_STATUS_DISABLING_IN_PROGRESS          ((FmTxStatus)FMC_STATUS_DISABLING_IN_PROGRESS)
#define FM_TX_STATUS_SCRIPT_EXEC_FAILED             ((FmcStatus)FMC_STATUS_SCRIPT_EXEC_FAILED)
#define FM_TX_STATUS_PROCESS_TIMEOUT_FAILURE        ((FmcStatus)FMC_STATUS_PROCESS_TIMEOUT_FAILURE)

#define FM_TX_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES ((FmcStatus)FMC_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)

/*
    TX-specific codes. They start from FMC_FIRST_FM_TX_STATUS_CODE to avoid conflict
    with the common codes range
*/
#define FM_TX_STATUS_RDS_NOT_ENABLED                    ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 1))
#define FM_TX_STATUS_TRANSMITTER_NOT_TUNED          ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 2))
#define FM_TX_STATUS_TRANSMISSION_IS_NOT_ON         ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 3))
#define FM_TX_STATUS_FM_RX_ALREADY_ENABLED          ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 4))
#define FM_TX_STATUS_AUTO_MODE_NOT_ON               ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 5))
#define FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON           ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 6))
#define FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON     ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 7))
#define FM_TX_STATUS_CONFLICTING_RDS_CMD_IN_PROGRESS    ((FmTxStatus)(FMC_FIRST_FM_TX_STATUS_CODE + 8))

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FmTxEvent structure
 *
 *     Represents an FM TX event.
 */
struct _FmTxEvent 
{
    /* The context for which the event is intended */
    FmTxContext     *context;
    
    /* Defines the event that caused the callback */
    FmTxEventType   eventType;

    /* 
        The status of this event. In case a status is not applicable 
        this field will equal FM_TX_STATUS_NOT_APPLICABLE
    */
    FmTxStatus  status;

    union {
        /*
            The cmd_done structure is valid when eventType == FM_TX_EVENT_CMD_DONE
        */
        struct {
        
            /* The type of the command that completed execution */
            FmTxCmdType  cmdType;

            union {
                /* 
                    Valid when cmdType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG or
                    FM_TX_CMD_GET_RDS_TRAFFIC_CODES
                */
                struct {
                    FmcRdsTaCode    taCode;
                    FmcRdsTpCode    tpCode;
                } trafficCodes;
                /* 
                    Valid when cmdType==    FM_TX_CMD_WRITE_RDS_RAW_DATA or
                    FM_TX_CMD_READ_RDS_RAW_DATA
                */
                struct 
                {
                    /* The length of the PS message */
                    FMC_UINT            len;
                
                    /* The radio text */
                    FMC_U8              *msg;
                } psMsg;
                

                /*
                    Valid when cmdType== FM_TX_CMD_SET_RDS_TEXT_RT_MSG or 
                    FM_TX_CMD_GET_RDS_TEXT_RT_MSG
                */
                struct 
                {
                    FmcRdsRtMsgType     rtMsgType;
                    
                    /* The length of the PS message */
                    FMC_UINT            len;
                
                    /* The radio text */
                    FMC_U8              *msg;
                } rtMsg;

                /* 
                    Valid when cmdType== FM_TX_CMD_WRITE_RDS_RAW_DATA or 
                    FM_TX_CMD_READ_RDS_RAW_DATA
                */
                struct 
                {
                    /* The length of the Raw RDS data */
                    FMC_UINT    len;
                
                    /* A pointer to the RDS data - must be copied to an application buffer */
                    FMC_U8      *data;
                    } rawRds;     
                struct
                {
                    TCCM_VAC_UnavailResourceList *ptUnavailResources;
                    
                }audioOperation;
                /* 
                    Potentially valid for all other values of cmdType.
                    
                    The value should be cast to a type that is appropriate to
                    the specific command.
                    
                    When no value is available, value==FMC_NO_VALUE
                */
                FMC_U32     value;
            } v;
        } cmdDone;
    } p;    
} ;

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * FM_TX_Init()
 *
 * Brief:  
 *      Initializes FM TX module.
 *
 * Description:
 *    It is usually called at system startup.
 *
 *    This function must be called by the system before any other FM TX function.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      void.
 *
 * Returns:
 *      FMC_STATUS_SUCCESS - FM was initialized successfully.
 *
 *      FMC_STATUS_FAILED -  FM failed initialization.
 */
FmcStatus FM_TX_Init(void);

/*-------------------------------------------------------------------------------
 * FM_Deinit()
 *
 * Brief:  
 *      Deinitializes FM module.
 *
 * Description:
 *      After calling this function, no FM function (RX or TX) can be called.
 *
 * Caveats:
 *      Both FM RX & TX must be detroyed before calling this function.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      void
 *
 * Returns:
 *      N/A (void)
 */
FmcStatus FM_TX_Deinit(void);


/*-------------------------------------------------------------------------------
 * FM_TX_Create()
 *
 * Brief:  
 *      Allocates a unique FM TX context.
 *
 * Description:
 *      This function must be called before any other FM TX function.
 *
 *      The allocated context should be supplied in subsequent FM TX calls.
 *      The caller must also provide a callback function, which will be called 
 *      on FM TX events.
 *
 *      Both FM RX & TX Contexts may exist concurrently. However, only one at a time may be enabled 
 *      (see FM_TX_Enable for details)
 *
 *      Currently, only a single context may be created
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      appHandle [in] - application handle, can be a null pointer (use default application).
 *
 *      fmCallback [in] - all FM TX events will be sent to this callback.
 *      
 *      fmContext [out] - allocated FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_SUCCESS - Operation completed successfully.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_ALREADY_EXISTS - FM Tx Context already created
 *
 *      FM_TX_STATUS_FM_NOT_INITIALIZED - FM Not initialized. Please call FM_Init() first
 */
FmTxStatus FM_TX_Create(FmcAppHandle *appHandle, const FmTxCallBack fmCallback, FmTxContext **fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_Destroy()
 *
 * Brief:  
 *      Releases the FM TX context (previously allocated with FM_TX_Create).
 *
 * Description:
 *      An application should call this function when it completes using FM TX services,
 *      and the FM TX is disabled.
 *
 *
 *
 * Generated Events:
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      fmContext [in/out] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_SUCCESS - Operation completed successfully.
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_DOESNT_EXIST - FM Tx Context wasn't created
 */
FmTxStatus FM_TX_Destroy(FmTxContext **fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_Enable()
 *
 * Brief:  
 *      Enable FM TX
 *
 * Description:
 *      After calling this function, FM TX is enabled and ready for configurations.
 *
 *      This procedure will initialize the FM TX Module in the chip. This process is comrised
 *      of the following steps:
 *      1. Power on the chip and send the chip init script (only if BT radio is not on at the moment)
 *      2. Power on the FM module in the chip
 *      3. Send the FM Init Script
 *      4. Apply default values that are defined in fm_config.h
 *
 *      The application may call any FM TX function only after successfuly completion
 *      of all the steps listed above.
 *
 *      If, for any reason, the application decides to abort the enabling process, it should call
 *      FM_TX_Disable() any time. In that case, an event TX enable will be recieved with status 
 *      FM_TX_STATUS_DISABLING_IN_PROGRESS.
 *
 * Caveats:
 *      EITHER FM RX or FM TX MAY BE ENABLED CONCURRENTLY, BUT NOT BOTH
 *
 *      When the transmitter is enabled, it is not transmitting. To actually
 *      start transmission, FM_TX_StartTransmission() must be called.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_ENABLE
 *
 * Type:
 *      Synchronous/Asynchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_SUCCESS - Operation completed successfully (already enabled).
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_FM_RX_ALREADY_ENABLED - FM RX already enabled, only one a time allowed
 * 
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress 
 *
 *      FM_TX_STATUS_CONTEXT_ALREADY_ENABLED - The context is already enabled. Please disable it first.
 */
FmTxStatus FM_TX_Enable(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_Disable()
 *
 * Brief:  
 *      Disable FM TX
 *
 * Description:
 *      
 *      Disables the FM TX.
 *      Any command in Progress or pending in the commands queue will be processed synchronously, and 
 *      event for this command will be returned with status Disabeling.
 *
 *      If this function is called while an FM_TX_Enable() is in progress, it will abort the enabling process.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE,  command type==FM_TX_CMD_DISABLE
 *
 * Type:
 *      Asynchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 * 
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress 
 */
FmTxStatus FM_TX_Disable(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_Tune()
 *
 * Brief:  
 *      Tune the transmitter to the specified frequency
 *
 * Description:
 *      Tune the transmitter to the specified frequency.
 *
 * Default Values: 
 *      NO DEFAULT VALUE AVAILABLE
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_TUNE
 *
 * Type:
 *      Synchronous/Asynchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *      freq [in] - the frequency to set in KHz.
 *
 *                  The value must match the configured band and channel spacing.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_RX_STATUS_TOO_MANY_PENDING_OPERATIONS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_Tune(FmTxContext *fmContext, FmcFreq freq);

/*-------------------------------------------------------------------------------
 * FM_TX_GetTunedFrequency()
 *
 * Brief:  
 *      Returns the currently tuned frequency.
 *
 * Description:
 *      Returns the currently tuned frequency.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_TUNED_FREQUENCY
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TRANSMITTER_NOT_TUNED - Frequency was never set (first call FM_TX_Tune())
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetTunedFrequency(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_StartTransmission()
 *
 * Brief:  
 *      Starts FM TX Transmission
 *
 * Description:
 *      After succesfuly completion of this function, FM TX is transmitting on the tuned frequency.
 * *
 *      If transmission is already on, the function will return immediately with status FM_TX_STATUS_SUCCESS.
 *
 * Caveats:
 *      The transmitter must be tuned before calling this function (via FM_TX_Tune()).
 *
 * Default Values: 
 *      Stopped
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_START_TRANSMISSION
 *
 * Type:
 *      Synchronous/Asynchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_SUCCESS - Operation completed successfully (transmission already On).
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_TRANSMITTER_NOT_TUNED - Tune the transmitter before calling this function
 */
FmTxStatus FM_TX_StartTransmission(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_StopTransmission()
 *
 * Brief:  
 *      Stops FM TX transmitter
 *
 * Description:
 *      Stops transmitter operation while retaining all FM TX configurations intact
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_STOP_TRANSMISSION
 *
 * Type:
 *      Synchronous/Asynchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TRANSMISSION_IS_NOT_ON - Transmission is not on at the moment
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_StopTransmission(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetMonoStereoMode()
 *
 * Brief:  
 *      Set the Mono/Stereo mode.
 *
 * Description:
 *      Set the FM TX to Mono or Stereo mode.
 *
 * Default Values: 
 *      Stereo
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_MONO_STEREO_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *      mode [in] - The mode to set: FM_STEREO_MODE or FM_MONO_MODE.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_SetMonoStereoMode(FmTxContext *fmContext, FmTxMonoStereoMode mode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetMonoStereoMode()
 *
 * Brief: 
 *      Returns the current Mono/Stereo mode.
 *
 * Description:
 *      Returns the current Mono/Stereo mode.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_MONO_STEREO_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetMonoStereoMode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetPreEmphasisFilter()
 *
 * Brief: 
 *      Selects the transmitter's pre-emphasis filter
 *
 * Description:
 *      The FM transmitting station uses a pre-emphasis filter to provide better S/N ratio, 
 *      and the receiver uses a de-emphasis filter corresponding to the transmitter to restore 
 *      the original audio. This varies per country. Therefore the filter has to configure the 
 *      appropriate filter accordingly.
 *
 * Generated Events:
 *      1. Event type==FM_TX_EVENT_CMD_DONE, with command type == FM_TX_CMD_SET_PRE_EMPHASIS_FILTER
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      filter [in] - the de-emphasis filter to use
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */  
FmTxStatus FM_TX_SetPreEmphasisFilter(FmTxContext *fmContext, FmcEmphasisFilter filter);

/*-------------------------------------------------------------------------------
 * FM_TX_GetPreEmphasisFilter()
 *
 * Brief:  
 *      Returns the current pre-emphasis filter
 *
 * Description:
 *      Returns the current pre-emphasis filter
 *
 * Generated Events:
 *      1. Event type==FM_TX_EVENT_CMD_DONE, with command type == FM_TX_CMD_GET_PRE_EMPHASIS_FILTER
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmRxContext [in] - FM RX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetPreEmphasisFilter(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetPowerLevel()
 *
 * Brief:  
 *      Set the output power level
 *
 * Description:
 *      Set the output power level, 0-31 (0 is strongest, 31 is weakest)
 *
 * Default Values: 
 *      4
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_POWER_LEVEL
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *      level [in] - the level to set: 0 (strongest) - 31 (weakest).
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_SetPowerLevel(FmTxContext *fmContext, FmTxPowerLevel level);

/*-------------------------------------------------------------------------------
 * FM_TX_GetPowerLevel()
 *
 * Brief:  
 *      Returns the current output level.
 *
 * Description:
 *      Returns the current output level
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_POWER_LEVEL
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetPowerLevel(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetMuteMode()
 *
 * Brief:  
 *      Set the mute mode.
 *
 * Description:
 *      Sets the radio mute mode - mute, unmute
 *
 * Default Values: 
 *      Not Muted
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_MUTE_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *      mode [in] - the mode to set.
 *                  Can get the modes:FMC_MUTE or FMC_NOT_MUTE only.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_SetMuteMode(FmTxContext *fmContext, FmcMuteMode mode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetMuteMode()
 *
 * Brief:  
 *      Returns the current mute mode.
 *
 * Description:
 *      Returns the current mute mode
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_MUTE_MODE
           With value FMC_MUTE or FMC_NOT_MUTE.
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetMuteMode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_EnableRds()
 *
 * Brief:  
 *      Enables RDS transmission.
 *
 * Description:
 *      Starts RDS data transmission. RDS data will be transmitted only when the transmitter is on
 *
 * Default Values: 
 *      RDS Is disabled
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_ENABLE_RDS
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_EnableRds(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_DisableRds()
 *
 * Brief:  
 *      Disables RDS transmission.
 *
 * Description:
 *      Stops RDS data transmission.
 *
 *      All RDS configurations are retained.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_DISABLE_RDS
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_DisableRds(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTransmissionMode()
 *
 * Brief:  
 *      Sets the RDS ransmission mode (automatic / manual)
 *
 * Description:
 *      The RDS Must be disabled when this function is called.
 *      In the automatic mode the FM TX user configures individual RDS information fields
 *      via the FM TX API. The FM TX module in the chip processes the individual fields and
 *      forms the RDS data accordingly. 
 *
 *      In the manual mode of operation, the FM TX API user builds the RDS data (except for the 
 *      check-words that are part of every block in the RDS group). It calls FM_TX_SetRdsRawData()
 *      to provide FM TX with the RDS data for transmission. 
 *
 *      The individual RDS fields retain their values when switching from automatic to manual mode.
 *
 * Default Values: 
 *      Automatic
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_SET_RDS_TRANSMISSION_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 *      mode [in] - the RDS transmission mode to set.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - The requested transmission mode is not supported
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_SetRdsTransmissionMode(FmTxContext *fmContext, FmTxRdsTransmissionMode mode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTransmissionMode()
 *
 * Brief:  
 *      Returns the current RDS mode.
 *
 * Description:
 *      Returns the current RDS mode
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_GET_RDS_TRANSMISSION_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 */
FmTxStatus FM_TX_GetRdsTransmissionMode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsAfCode()
 *
 * Brief: 
 *      Sets the AF code.       
 *
 * Description:
 *      Sets the transmitted Alternate Frequency (AF) code. 
 *      
 *      In case the caller doesn't want to transmit any valid AF code,
 *      a special value (FMC_AF_CODE_NO_AF_AVAILABLE) should be specified in the call.
 *
 *      The values range from 1 to 204. In the following code list, each code represents an
 *      AF carrier frequency:
 *
 *          1       87.6 MHZ
 *          2       87.7 MHZ
 *          :       :
 *          :       :
 *          204         107.9 MHZ
 *
 *
 * Default Values: 
 *      FMC_AF_NO_AF_AVAILABLE
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_AF_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      afCode [in] - AF Code to transmit. 1 - 204, or FMC_AF_CODE_NO_AF_AVAILABLE
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsAfCode(FmTxContext *fmContext, FmcAfCode afCode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsAfCode()
 *
 * Brief:  
 *      Returns the current AF code.
 *
 * Description:
 *      Returns the current AF code
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_AF_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsAfCode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsPiCode()
 *
 * Brief: 
 *      Sets the PI code.       
 *
 * Description:
 *      The Program-identification (PI) Code consists of a code enabling the receiver 
 *      to distinguish between countries, areas in which the same program is transmitted, 
 *      and the identification of the program itself. The code is not intended for direct 
 *      display and is assigned to each individual radio program, to enable it to be distinguished 
 *      from all other programs. One important application of this information would be to enable 
 *      the receiver to search automatically for an alternative frequency in case of bad reception of
 *      the program to which the receiver is tuned; the criteria for the change-over to the new 
 *      frequency would be the presence of a better signal having the same Program Identification code.
 *
 * Default Values: 
 *      FM_CONFIG_TX_DEFAULT_RDS_PI_CODE (defined in fm_config.h)
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_PI_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      piCode [in] - PI Code to transmit
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsPiCode(FmTxContext *fmContext, FmcRdsPiCode piCode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsPiCode()
 *
 * Brief:  
 *      Returns the current PI code.
 *
 * Description:
 *      Returns the current PI code
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_PI_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsPiCode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsPtyCode()
 *
 * Brief: 
 *      Sets the PTY code.      
 *
 * Description:
 *      The Program-type (PTY) code is an identification number to be transmitted 
 *      with each program item and which is intended to specify the current Program 
 *      Type within 31 possibilities. This code could be used for search tuning. The code will,
 *      moreover, enable suitable receivers and recorders to be pre-set to respond only to 
 *      program items of the desired type. The last number, i.e. 31, is reserved for an alarm 
 *      identification which is intended to switch on the audio signal when a receiver is
 *      operated in a waiting reception mode.
 *
 * Default Values: 
 *      0 (Undefined Program Type)
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_PTY_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      ptyCode [in] - PTY Code to transmit (1 - 31)
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsPtyCode(FmTxContext *fmContext, FmcRdsPtyCode ptyCode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsPtyCode()
 *
 * Brief:  
 *      Returns the current PTY code.
 *
 * Description:
 *      Returns the current PTY code
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_PTY_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsPtyCode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTextRepertoire()
 *
 * Brief: 
 *      Sets the Text Repertoire.       
 *
 * Description:
 *      3 Code tables were defined (G0, G1, G2) for the displayed 8-bit text characters relating 
 *      to the Program Service name (PS), and Radiotext (RT).
 *
 *      This function selects which table (text repertoire) to use for these fields.
 *
 * Default Values: 
 *      G0 Code Table (basic code table)
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      repertoire [in] - Reperoire to use
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsTextRepertoire(FmTxContext *fmContext, FmcRdsRepertoire repertoire);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTextRepertoire()
 *
 * Brief:  
 *      Returns the current text repertoire.
 *
 * Description:
 *      Returns the current text repertoire
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsTextRepertoire(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsPsDisplayMode()
 *
 * Brief: 
 *      Sets the PS Text Scroll Mode
 *
 * Description:
 *      Configures whether or not to scroll the PS text when transmitting PS text. This call has no affect
 *      on RT text.
 *
 * Default Values: 
 *      Static
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      scrollMode [in] - Scroll mode (on / off) to use
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsPsDisplayMode(FmTxContext *fmContext, FmcRdsPsDisplayMode scrollMode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsPsDisplayMode()
 *
 * Brief:  
 *      Returns the current PS text scroll mode.
 *
 * Description:
 *      Returns the current PS text scroll mode
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsPsDisplayMode(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsPsScrollSpeed()
 *
 * Brief: 
 *      Sets the PS Text Scroll Parameters
 *
 * Description:
 *      Configures scroll parameters that are used when PS text is transmitted and PS scrolling is turned on.
 *
 *      The parameters include:
 *      --------------------
 *      1. Speed: The number of times that one set of PS characters needs to be repeated 
 *                  before moving ahead by one character scrol
 *      2. Receiver's display size: number of characters available on receiver's display
 *
 * Default Valuess:
 *      Speed:      3
 *      Display Size:   8
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      speed [in] - Scroll speed (1 - 255)
 *
 *      displaySize [in] - Receiver's display size (1 - 8)
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */

FmTxStatus FM_TX_SetRdsPsScrollSpeed(FmTxContext        *fmContext, 
                                                FmcRdsPsScrollSpeed         speed);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsPsScrollSpeed()
 *
 * Brief:  
 *      Returns the current text scroll speed.
 *
 * Description:
 *      Returns the current text scroll speed
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsPsScrollSpeed(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTextPsMsg()
 *
 * Brief: 
 *      Sets the PS Text.       
 *
 * Description:
 *      The Program-service name (PS) text is the label of the program service consisting of 
 *      not more than eight alphanumeric characters coded in accordance with the configured
 *      text repertoire (see FM_TX_SetRdsTextRepertoire() for details), which is displayed by 
 *      RDS receivers in order to inform the listener what program service is being broadcast 
 *      by the station to which the receiver is tuned. An example for a name is "Radio 21". 
 *      The Program Service name is not intended to be used for automatic search tuning and 
 *      must not be used for giving sequential information. 
 *
 * Caveats:
 *      The message will be transmitted only when PS transmission is enabled 
 *          via the FM_TX_SetRdsTransmittedGroupsMask() call.
 *
 * Default Values: 
 *      FM_CONFIG_TX_DEFAULT_RDS_PS_MSG (defined in fm_config.h)
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      msg [in] - PS Text
 *
 *      len [in] - Length of text (1 - 8)
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsTextPsMsg(FmTxContext *fmContext, const FMC_U8 *msg, FMC_UINT len);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTextPsMsg()
 *
 * Brief:  
 *      Returns the current PS Text.
 *
 * Description:
 *      Returns the current PS Text
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TEXT_PS_MSG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsTextPsMsg(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTextRtMsg()
 *
 * Brief: 
 *      Sets the RDS RT Data to be sent.        
 *
 * Description:
 *      The Radiotext (RT) is a text that may be displayed on RDS receivers equipped
 *      with suitable display facilities. The message is coded in accordance with the 
 *      configured text repertoire (see FM_TX_SetRdsTextRepertoire() for details).
 *
 *      RT messages are broadcast in type 2 groups. There are 2 variants to this group: A and B.
 *      The difference is the number of characters that may be broadcast in a single group. Type A
 *      has twice the number of characters than type B.
 *
 * Caveats:
 *      The message will be transmitted only when RT transmission is enabled 
 *          via the FM_TX_SetRdsTransmittedGroupsMask() call.
 *
 * Default Values: 
 *      FM_CONFIG_TX_DEFAULT_RDS_RT_MSG (defined in fm_config.h)
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TEXT_RT_MSG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      msgType [in] - RT Type (A or B or Auto )
 *
 *      msg [in] - The text to display.
 *
 *      len [in] - The length of the text message:
 *              For Type A: 1 - 64 bytes
 *              For Type B: 1 - 32 bytes
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsTextRtMsg(   FmTxContext             *fmContext, 
                                            FmcRdsRtMsgType     msgType, 
                                            const FMC_U8        *msg,
                                            FMC_UINT            len);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTextRtMsg()
 *
 * Brief:  
 *      Returns the current RT Text.
 *
 * Description:
 *      Returns the current RT Text
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TEXT_RT_MSG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsTextRtMsg(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTransmittedGroupsMask()
 *
 * Brief: 
 *      Sets which fields will be transmitted in the RDS stream     
 *
 * Description:
 *      Currently, the following fields may be controlled via this function:
 *      1. PS (Program Service Name)
 *      2. RT (Radio Text)
 *      3.ECC(Extended Country Code)
 *
 * Caveats:
 *      All three can be transmitted simultaneously
 *
 * Default Values:
 *      Transmit PS Text
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TRANSMITTED_MASK
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      fieldsMask [in] - The mask specifying which fields to transmit.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - The requested value or operation is not supported
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsTransmittedGroupsMask(   FmTxContext                     *fmContext, 
                                                            FmTxRdsTransmittedGroupsMask    fieldsMask);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTransmittedGroupsMask()
 *
 * Brief:  
 *      Returns the current transmitted fields mask.
 *
 * Description:
 *      Returns the current transmitted fields mask
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TRANSMITTED_MASK
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsTransmittedGroupsMask(FmTxContext *fmContext);


/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsTrafficCodes()
 *
 * Brief: 
 *      Sets transmitted traffic codes (TP, TA)
 *
 * Description:
 *      There are 2 traffic codes: Traffic Program code (TP), and Traffic Announcement code (TA). 
 *
 *      Their coding is described below:
 *  
 *      TP      TA          Applications
 *      --      --          ----------
 *      Off         Off             This program does not carry traffic announcements nor does it refer,
 *                          via EON, to a program that does.
 *
 *      Off     On          This program carries EON information about another program which
 *                          gives traffic information.
 *
 *      On      Off         This program carries traffic announcements but none are being
 *                          broadcast at present and may also carry EON information about other
 *                          traffic announcements.
 *
 *      On      On          A traffic announcement is being broadcast on this program at present.
 *
 *
 * Caveats:
 *      Currently, the combination TP=Off, TA=On is not supported.
 *
 * Default Valuess:
 *      TP: Off
 *      TA: Off
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_TRAFFIC_CODES
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      taCode [in] - TA Code.
 *
 *      tpCode [in] - TP Code.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - The requested value or operation is not supported
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsTrafficCodes(FmTxContext         *fmContext, 
                                            FmcRdsTaCode    taCode,
                                            FmcRdsTpCode    tpCode);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsTrafficCodes()
 *
 * Brief:  
 *      Returns the current traffic codes.
 *
 * Description:
 *      Returns the current traffic codes
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_TRAFFIC_CODES
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsTrafficCodes(FmTxContext *fmContext);

/*-------------------------------------------------------------------------------
 * FM_TX_SetRdsMusicSpeechFlag()
 *
 * Brief: 
 *      Sets the music/speech flag      
 *
 * Description:
 *      The music/speech flag is is a two-state signal to provide information on whether 
 *      music or speech is being broadcast. The signal would permit receivers to be equipped 
 *      with two separate volume controls, one for music and one for speech, so that the listener could
 *      adjust the balance between them to suit his individual listening habits.
 *
 * Default Values: 
 *      Music
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      musicSpeechFlag [in] - The flag's value.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_SetRdsMusicSpeechFlag(FmTxContext *fmContext, FmcRdsMusicSpeechFlag    musicSpeechFlag);

/*-------------------------------------------------------------------------------
 * FM_TX_GetRdsMusicSpeechFlag()
 *
 * Brief:  
 *      Returns the music/speech flag
 *
 * Description:
 *      Returns the music/speech flag
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already waiting
 *                                                      execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when in automatic mode.
 */
FmTxStatus FM_TX_GetRdsMusicSpeechFlag(FmTxContext *fmContext);

/*------------------------------------------------------------------------------
 * FM_TX_SetRdsECC()
 *
 * Brief: 
 *
 *
 * Description:
 *
 * Default Values:
 *
 * Generated Events:
 *      1. eventType == FM_TX_EVENT_CMD_DONE
 *         with commandType == FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      countryCode [in] - Extended Country Code to transmit. Must be in range
 *          of 8 bits (0-255).
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be
 *          sent to the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid
 *          parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already
 *          waiting execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when
 *          in automatic mode.
 */
FmTxStatus FM_TX_SetRdsECC(FmTxContext *fmContext,
                           FmcRdsExtendedCountryCode countryCode);

/*------------------------------------------------------------------------------
 * FM_TX_GetRdsECC()
 *
 * Brief:  
 *
 *
 * Description:
 *
 *
 * Generated Events:
 *      1. eventType == FM_TX_EVENT_CMD_DONE
 *         with commandType == FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be
 *          sent to the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called  with an invalid
 *          parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled
 *
 *      FM_TX_STATUS_TOO_MANY_PENDING_CMDS - Too many operations are already
 *          waiting execution in operations queue.
 *
 *      FM_TX_STATUS_RDS_AUTO_MODE_NOT_ON - Operation can only be performed when
 *          in automatic mode.
 */
FmTxStatus FM_TX_GetRdsECC(FmTxContext *fmContext);

/*------------------------------------------------------------------------------
 * FM_TX_WriteRdsRawData()
 *
 * Brief: 
 *      Writes the raw RDS data .       
 *
 * Description:
 *      Provides a raw RDS data buffer to be transmitted. The buffer should contain RDS data ready for transmission, 
 *      except for the check-words.
 *
 * Caveats:
 *      This function is applicable only when the RDS is operating in manual mode (FMC_RDS_TRANSMISSION_MANUAL).
 *      The mode is set via the FM_TX_SetRdsTransmissionMode() API call.
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_WRITE_RDS_RAW_DATA
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      rdsRawData [in] - The data buffer.
 *
 *      len [in] - The length of the data buffer (1 - 256)
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled, or RDS mode is not manual.
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - Manual mode is not supported
 *
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress
 *
 *      FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON - Operation can only be performed when in manual mode.
 */
FmTxStatus FM_TX_WriteRdsRawData(FmTxContext *fmContext, const FMC_U8 *rdsRawData, FMC_UINT len);

/*-------------------------------------------------------------------------------
 * FM_TX_ReadRdsRawData()
 *
 * Brief:  
 *      Returns the current RDS Raw Data.
 *
 * Description:
 *      Returns the current RDS Raw Data.
 *
 * Caveats:
 *      This function is applicable only when the RDS is operating in manual mode (FMC_RDS_TRANSMISSION_MANUAL).
 *      The mode is set via the FM_TX_SetRdsTransmissionMode() API call.
 *
 * Default Values: 
 *      NO DEFAULT VALUE AVAILABLE
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_READ_RDS_RAW_DATA
 *
 * Type:
 *      Asynchronous/Synchronous
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == FM_TX_CMD_GET_RDS_DI_CODES
 *
 * Parameters:
 *      fmContext [in] - FM TX context.
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled, or RDS mode is not manual.
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - Manual mode is not supported
 *
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress
 *
 *      FM_TX_STATUS_RDS_MANUAL_MODE_NOT_ON - Operation can only be performed when in manual mode.
 *
 *      FM_TX_STATUS_NO_VALUE_AVAILABLE - RDS Raw data haven't been set (call FM_TX_WriteRdsRawData first)
 */
FmTxStatus FM_TX_ReadRdsRawData(FmTxContext *fmContext);
/*-------------------------------------------------------------------------------
 * FM_TX_ChangeAudioSource()
 *
 * Brief: 
 *      bbb.        
 *
 * Description:
 *
 *
 * Caveats:
 *      bbb
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == bbb
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      txSource [in] - bbb
 *
 *      digitalConfig [in] - bbb 
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled, or RDS mode is not manual.
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - Manual mode is not supported
 *
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress
 *
 */

FmTxStatus FM_TX_ChangeAudioSource(FmTxContext *fmContext, FmTxAudioSource txSource, ECAL_SampleFrequency eSampleFreq);
/*-------------------------------------------------------------------------------
 * FM_RX_ChangeDigitalSourceConfiguration()
 *
 * Brief: 
 *      bbb.        
 *
 * Description:
 *
 *
 * Caveats:
 *      bbb
 *
 * Generated Events:
 *      1. eventType==FM_TX_EVENT_CMD_DONE, with commandType == bbb
 *
 * Type:
 *      Asynchronous/Synchronous
 *
 * Parameters:
 *      fmContext [in] - FM context.
 *
 *      digitalConfig [in] - bbb 
 *
 * Returns:
 *      FM_TX_STATUS_PENDING - Operation started successfully, an event will be sent to
 *                              the application upon completion.
 *
 *      FM_TX_STATUS_INVALID_PARM - The function was called with an invalid parameter
 *
 *      FM_TX_STATUS_CONTEXT_NOT_ENABLED - The context is not enabled, or RDS mode is not manual.
 *
 *      FM_TX_STATUS_NOT_SUPPORTED - Manual mode is not supported
 *
 *      FM_TX_STATUS_IN_PROGRESS - This function is already in progress
 *
 */
FmTxStatus FM_TX_ChangeDigitalSourceConfiguration(FmTxContext *fmContext, ECAL_SampleFrequency eSampleFreq);
#endif /* ifndef __FM_TX_H  */

