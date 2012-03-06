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
*   FILE NAME:      bt_hci_if.h
*
*   BRIEF:          This file defines the interface of the BT HCI abstraction layer
*
*   DESCRIPTION:    
*
*       The BT HCI abstraction layer is a layer that provides BT HCI functionality independent
*       of a specific BT HCI layer implementation.
*
*       The layer provides 2 major functionalities:
*       1. Initialization: HCI, BT transport and BT radio initialization.
*       2. Post-initialization: Sending BT HCI commands and receving BT HVI events.
*
*           This layer assumes that there is a single managing entity that is responsible
*       for the initialization operations. Once initialization is complete, multiple clients
*       may send BT HCI commands and receive corresponding BT HCI events via this layer.
*
*       TBD: Add support for unsolicited HCI events (e.g., FM "Interrupts")
*
*       The layer supports multiple BT HCI transports, one per TI chip.
*
*       Initialization
*       ----------
*       Initialization consists of the following steps:
*       1. Initialization of the HCI and transport layers. This includes sending HCI reset to the chip.
*           (BT_HCI_IF_MngrTransportOn)
*       2. Sending radio-specific initialization commands (init script) 
*           (BT_HCI_IF_MngrSendRadioCommand / BT_HCI_IF_MngrSetTranParms).
*       3. Completing HCI initialization - HCI flow control (BT_HCI_IF_MngrConfigHci)
*
*       De-initialization
*       -------------
*       De-initilaization requires a single call (BT_HCI_IF_MngrTransportOff)
*
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __BT_HCI_IF_H
#define __BT_HCI_IF_H

#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"

/*---------------------------------------------------------------------------
 * BtHciIfHciOpcode type
 *
 *     An HCI opcode that is part of an HCI command.
 *
 *  These opcodes are passed in the calls to BT_HCI_IF_MngrSendRadioCommand and
 *  BT_HCI_IF_SendHciCommand.
 */
typedef enum tagBtHciIfHciOpcode {
    BT_HCI_IF_HCI_CMD_READ_LOCAL_VERSION                            = 0x1001,
    BT_HCI_IF_HCI_CMD_VS_WRITE_CODEC_CONFIGURE                      = 0xfd06,
    BT_HCI_IF_HCI_CMD_READ_MODIFY_WRITE_HARDWARE_REGISTER           = 0xfd09,
    BT_HCI_IF_HCI_CMD_WRITE_TO_FM                                   = 0xfd35,
    BT_HCI_IF_HCI_CMD_VS_WRITE_I2C_REGISTER_ENHANCED                = 0xfd36,
    BT_HCI_IF_HCI_CMD_FM_OVER_BT_6350                               = 0xfd61,
    BT_HCI_IF_HCI_CMD_SET_NARROW_BAND_VOICE_PATH                    = 0xfd94,
    BT_HCI_IF_HCI_CMD_VS_BT_IP1_1_SET_FM_AUDIO_PATH                 = 0xfd95,
    /*  Assisted A2DP (A3DP) vendor-specifc HCI commands */
    BT_HCI_IF_HCI_CMD_VS_A3DP_AVPR_ENABLE                           = 0xfd92,
    BT_HCI_IF_HCI_CMD_VS_A3DP_OPEN_STREAM                           = 0xfd8c,
    BT_HCI_IF_HCI_CMD_VS_A3DP_CLOSE_STREAM                          = 0xfd8d,
    BT_HCI_IF_HCI_CMD_VS_A3DP_CODEC_CONFIGURATION                   = 0xfd8e,
    BT_HCI_IF_HCI_CMD_VS_A3DP_MULTI_SNK_CONFIGURATION               = 0xfd96,
    BT_HCI_IF_HCI_CMD_VS_A3DP_START_STREAM                          = 0xfd8f,
    BT_HCI_IF_HCI_CMD_VS_A3DP_STOP_STREAM                           = 0xfd90,
} BtHciIfHciOpcode;

/*---------------------------------------------------------------------------
 * BtHciIfHciEventType type
 *
 *     The type of HCI event that is received from the HCI layer.
 */
typedef enum tagBtHciIfHciEventType {
    BT_HCI_IF_HCI_EVENT_INQUIRY_COMPLETE =      0x01,
    BT_HCI_IF_HCI_EVENT_INQUIRY_RESULT =        0x02,
    BT_HCI_IF_HCI_EVENT_CONNECT_COMPLETE =      0x03,
    BT_HCI_IF_HCI_EVENT_CONNECT_REQUEST =       0x04,
    BT_HCI_IF_HCI_EVENT_DISCONNECT_COMPLETE =       0x05,
    BT_HCI_IF_HCI_EVENT_AUTH_COMPLETE =     0x06,
    BT_HCI_IF_HCI_EVENT_REMOTE_NAME_REQ_COMPLETE =      0x07,
    BT_HCI_IF_HCI_EVENT_ENCRYPT_CHNG =      0x08,
    BT_HCI_IF_HCI_EVENT_CHNG_CONN_LINK_KEY_COMPLETE =       0x09,
    BT_HCI_IF_HCI_EVENT_MASTER_LINK_KEY_COMPLETE =      0x0a,
    BT_HCI_IF_HCI_EVENT_READ_REMOTE_FEATURES_COMPLETE =     0x0b,
    BT_HCI_IF_HCI_EVENT_READ_REMOTE_VERSION_COMPLETE =      0x0c,
    BT_HCI_IF_HCI_EVENT_QOS_SETUP_COMPLETE =        0x0d,
    BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE =      0x0e,
    BT_HCI_IF_HCI_EVENT_COMMAND_STATUS =        0x0f,
    BT_HCI_IF_HCI_EVENT_HARDWARE_ERROR =        0x10,
    BT_HCI_IF_HCI_EVENT_FLUSH_OCCURRED =        0x11,
    BT_HCI_IF_HCI_EVENT_ROLE_CHANGE =       0x12,
    BT_HCI_IF_HCI_EVENT_NUM_COMPLETED_PACKETS =     0x13,
    BT_HCI_IF_HCI_EVENT_MODE_CHNG =     0x14,
    BT_HCI_IF_HCI_EVENT_RETURN_LINK_KEYS =      0x15,
    BT_HCI_IF_HCI_EVENT_PIN_CODE_REQ =      0x16,
    BT_HCI_IF_HCI_EVENT_LINK_KEY_REQ =      0x17,
    BT_HCI_IF_HCI_EVENT_LINK_KEY_NOTIFY =       0x18,
    BT_HCI_IF_HCI_EVENT_LOOPBACK_COMMAND =      0x19,
    BT_HCI_IF_HCI_EVENT_DATA_BUFFER_OVERFLOW =      0x1a,
    BT_HCI_IF_HCI_EVENT_MAX_SLOTS_CHNG =        0x1b,
    BT_HCI_IF_HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE =        0x1c,
    BT_HCI_IF_HCI_EVENT_CONN_PACKET_TYPE_CHNG =     0x1d,
    BT_HCI_IF_HCI_EVENT_QOS_VIOLATION =     0x1e,
    BT_HCI_IF_HCI_EVENT_PAGE_SCAN_MODE_CHANGE =     0x1f,
    BT_HCI_IF_HCI_EVENT_PAGE_SCAN_REPETITION_MODE =     0x20,
    BT_HCI_IF_HCI_EVENT_FLOW_SPECIFICATION_COMPLETE =       0x21,
    BT_HCI_IF_HCI_EVENT_INQUIRY_RESULT_WITH_RSSI =      0x22,
    BT_HCI_IF_HCI_EVENT_READ_REMOTE_EXT_FEAT_COMPLETE =     0x23,
    BT_HCI_IF_HCI_EVENT_FIXED_ADDRESS =     0x24,
    BT_HCI_IF_HCI_EVENT_ALIAS_ADDRESS =     0x25,
    BT_HCI_IF_HCI_EVENT_GENERATE_ALIAS_REQ =        0x26,
    BT_HCI_IF_HCI_EVENT_ACTIVE_ADDRESS =        0x27,
    BT_HCI_IF_HCI_EVENT_ALLOW_PRIVATE_PAIRING =     0x28,
    BT_HCI_IF_HCI_EVENT_ALIAS_ADDRESS_REQ =     0x29,
    BT_HCI_IF_HCI_EVENT_ALIAS_NOT_RECOGNIZED =      0x2a,
    BT_HCI_IF_HCI_EVENT_FIXED_ADDRESS_ATTEMPT =     0x2b,
    BT_HCI_IF_HCI_EVENT_SYNC_CONNECT_COMPLETE =     0x2c,
    BT_HCI_IF_HCI_EVENT_SYNC_CONN_CHANGED =     0x2d,
    BT_HCI_IF_HCI_EVENT_HCI_SNIFF_SUBRATING =       0x2e,
    BT_HCI_IF_HCI_EVENT_EXTENDED_INQUIRY_RESULT =       0x2f,
    BT_HCI_IF_HCI_EVENT_ENCRYPT_KEY_REFRESH_COMPLETE =      0x30,
    BT_HCI_IF_HCI_EVENT_IO_CAPABILITY_REQUEST =     0x31,
    BT_HCI_IF_HCI_EVENT_IO_CAPABILITY_RESPONSE =        0x32,
    BT_HCI_IF_HCI_EVENT_USER_CONFIRMATION_REQUEST =     0x33,
    BT_HCI_IF_HCI_EVENT_USER_PASSKEY_REQUEST =      0x34,
    BT_HCI_IF_HCI_EVENT_REMOTE_OOB_DATA_REQUEST =       0x35,
    BT_HCI_IF_HCI_EVENT_SIMPLE_PAIRING_COMPLETE =       0x36,
    BT_HCI_IF_HCI_EVENT_LINK_SUPERV_TIMEOUT_CHANGED =       0x38,
    BT_HCI_IF_HCI_EVENT_ENHANCED_FLUSH_COMPLETE =       0x39,
    BT_HCI_IF_HCI_EVENT_USER_PASSKEY_NOTIFICATION =     0x3a,
    BT_HCI_IF_HCI_EVENT_KEYPRESS_NOTIFICATION =     0x3b,
    BT_HCI_IF_HCI_EVENT_REMOTE_HOST_SUPPORTED_FEATURES =        0x3c,

    BT_HCI_IF_HCI_EVENT_FM_EVENT                            =       0xF0,
    BT_HCI_IF_HCI_EVENT_BLUETOOTH_LOGO                  =                 0xFE,
    BT_HCI_IF_HCI_EVENT_VENDOR_SPECIFIC                 =           0xFF
} BtHciIfHciEventType;

/*---------------------------------------------------------------------------
 * BtHciIfHciEventStatus type
 *
 *     The first parameter in an HCI event often contains a "status" value.
 *     0x00 means pending or success, according to the event type, but
 *     other values provide a specific reason for the failure. These
 *     values are listed below.
 */
typedef enum tagBtHciIfHciEventStatus {
    BT_HCI_IF_HCI_EVENT_STATUS_SUCCESS =        0x00,
    BT_HCI_IF_HCI_EVENT_STATUS_UNKNOWN_HCI_CMD =        0x01,
    BT_HCI_IF_HCI_EVENT_STATUS_NO_CONNECTION =      0x02,
    BT_HCI_IF_HCI_EVENT_STATUS_HARDWARE_FAILURE =       0x03,
    BT_HCI_IF_HCI_EVENT_STATUS_PAGE_TIMEOUT =       0x04,
    BT_HCI_IF_HCI_EVENT_STATUS_AUTH_FAILURE =       0x05,
    BT_HCI_IF_HCI_EVENT_STATUS_KEY_MISSING =        0x06,
    BT_HCI_IF_HCI_EVENT_STATUS_MEMORY_FULL =        0x07,
    BT_HCI_IF_HCI_EVENT_STATUS_CONN_TIMEOUT =       0x08,
    BT_HCI_IF_HCI_EVENT_STATUS_MAX_NUM_CONNS =      0x09,
    BT_HCI_IF_HCI_EVENT_STATUS_MAX_SCO_CONNS =      0x0a,
    BT_HCI_IF_HCI_EVENT_STATUS_ACL_ALREADY_EXISTS =     0x0b,
    BT_HCI_IF_HCI_EVENT_STATUS_CMD_DISALLOWED =     0x0c,
    BT_HCI_IF_HCI_EVENT_STATUS_HOST_REJ_NO_RESOURCES =      0x0d,
    BT_HCI_IF_HCI_EVENT_STATUS_HOST_REJ_SECURITY =      0x0e,
    BT_HCI_IF_HCI_EVENT_STATUS_HOST_REJ_PERSONAL_DEV =      0x0f,
    BT_HCI_IF_HCI_EVENT_STATUS_HOST_TIMEOUT =       0x10,
    BT_HCI_IF_HCI_EVENT_STATUS_UNSUPP_FEATUR_PARM_VAL =     0x11,
    BT_HCI_IF_HCI_EVENT_STATUS_INVAL_HCI_PARM_VAL =     0x12,
    BT_HCI_IF_HCI_EVENT_STATUS_CONN_TERM_USER_REQ =     0x13,
    BT_HCI_IF_HCI_EVENT_STATUS_CONN_TERM_LOW_RESOURCES =        0x14,
    BT_HCI_IF_HCI_EVENT_STATUS_CONN_TERM_POWER_OFF =        0x15,
    BT_HCI_IF_HCI_EVENT_STATUS_CONN_TERM_LOCAL_HOST =       0x16,
    BT_HCI_IF_HCI_EVENT_STATUS_REPEATED_ATTEMPTS =      0x17,
    BT_HCI_IF_HCI_EVENT_STATUS_PAIRING_DISALLOWED =     0x18,
    BT_HCI_IF_HCI_EVENT_STATUS_UNKNOWN_LMP_PDU =        0x19,
    BT_HCI_IF_HCI_EVENT_STATUS_UNSUPP_REMOTE_FEATURE =      0x1a,
    BT_HCI_IF_HCI_EVENT_STATUS_SCO_OFFSET_REJECTED =        0x1b,
    BT_HCI_IF_HCI_EVENT_STATUS_SCO_INTERVAL_REJECTED =      0x1c,
    BT_HCI_IF_HCI_EVENT_STATUS_SCO_AIR_MODE_REJECTED =      0x1d,
    BT_HCI_IF_HCI_EVENT_STATUS_INVALID_LMP_PARM =       0x1e,
    BT_HCI_IF_HCI_EVENT_STATUS_UNSPECIFIED_ERROR =      0x1f,
    BT_HCI_IF_HCI_EVENT_STATUS_UNSUPP_LMP_PARM =        0x20,
    BT_HCI_IF_HCI_EVENT_STATUS_ROLE_CHANGE_DISALLOWED =     0x21,
    BT_HCI_IF_HCI_EVENT_STATUS_LMP_RESPONSE_TIMEDOUT =      0x22,
    BT_HCI_IF_HCI_EVENT_STATUS_LMP_ERR_TRANSACT_COLL =      0x23,
    BT_HCI_IF_HCI_EVENT_STATUS_LMP_PDU_DISALLOWED =     0x24,
    BT_HCI_IF_HCI_EVENT_STATUS_ENCRYPTN_MODE_UNACCEPT =     0x25,
    BT_HCI_IF_HCI_EVENT_STATUS_UNIT_KEY_USED =      0x26,
    BT_HCI_IF_HCI_EVENT_STATUS_QOS_NOT_SUPPORTED =      0x27,
    BT_HCI_IF_HCI_EVENT_STATUS_INSTANT_PASSED =     0x28,
    BT_HCI_IF_HCI_EVENT_STATUS_PAIRING_W_UNIT_KEY_UNSUPP =      0x29,
    BT_HCI_IF_HCI_EVENT_STATUS_DIFFERENT_TRANSACTION_COLLISION =        0x2a,
    BT_HCI_IF_HCI_EVENT_STATUS_INSUFF_RESOURCES_FOR_SCATTER_MODE =      0x2b,
    BT_HCI_IF_HCI_EVENT_STATUS_QOS_UNACCEPTABLE_PARAMETER =     0x2c,
    BT_HCI_IF_HCI_EVENT_STATUS_QOS_REJECTED =       0x2d,
    BT_HCI_IF_HCI_EVENT_STATUS_CHANNEL_CLASSIF_NOT_SUPPORTED =      0x2e,
    BT_HCI_IF_HCI_EVENT_STATUS_INSUFFICIENT_SECURITY =      0x2f,
    BT_HCI_IF_HCI_EVENT_STATUS_PARAMETER_OUT_OF_MANDATORY_RANGE =       0x30,
    BT_HCI_IF_HCI_EVENT_STATUS_SCATTER_MODE_NO_LONGER_REQUIRED =        0x31,
    BT_HCI_IF_HCI_EVENT_STATUS_ROLE_SWITCH_PENDING =        0x32,
    BT_HCI_IF_HCI_EVENT_STATUS_SCATTER_MODE_PARM_CHNG_PENDING =     0x33,
    BT_HCI_IF_HCI_EVENT_STATUS_RESERVED_SLOT_VIOLATION =        0x34,
    BT_HCI_IF_HCI_EVENT_STATUS_SWITCH_FAILED =      0x35,
    BT_HCI_IF_HCI_EVENT_STATUS_EXTENDED_INQ_RESP_TOO_LARGE =        0x36,
    BT_HCI_IF_HCI_EVENT_STATUS_SECURE_SIMPLE_PAIR_NOT_SUPPORTED =       0x37,
} BtHciIfHciEventStatus;

/*---------------------------------------------------------------------------
 * BtHciIfObj type
 *
 *  Represents an instance of this layer
 */
typedef struct tagBtHciIfObj BtHciIfObj;

/*---------------------------------------------------------------------------
 * BtHciIfTranParms type
 *
 *  Used to store transport-specific configuration parameters that are set during initialization
 */
typedef struct tagBtHciIfTranParms {
    McpUint tranType;
    
    union {
        struct tagUartParms {
            McpU32      speed;
            McpUint     flowControl;
        } uartParms;
    } parms;
} BtHciIfTranParms;

/*---------------------------------------------------------------------------
 * BtHciIfStatus type
 *
 *  Status codes that are used to indicate operations results
 */
typedef enum tagBtHciIfStatus
{
    BT_HCI_IF_STATUS_SUCCESS            = MCP_HAL_STATUS_SUCCESS,
    BT_HCI_IF_STATUS_FAILED         = MCP_HAL_STATUS_FAILED,
    BT_HCI_IF_STATUS_PENDING            = MCP_HAL_STATUS_PENDING,
    BT_HCI_IF_STATUS_NO_RESOURCES   = MCP_HAL_STATUS_NO_RESOURCES,
    BT_HCI_IF_STATUS_INTERNAL_ERROR     = MCP_HAL_STATUS_INTERNAL_ERROR,
    BT_HCI_IF_STATUS_IMPROPER_STATE = MCP_HAL_STATUS_IMPROPER_STATE
} BtHciIfStatus;

/*---------------------------------------------------------------------------
 * BtHciIfMngrEventType type
 *
 *  Type of event that the layer sends its manager
 */
typedef enum tagBtHciIfMngrEventType {
    BT_HCI_IF_MNGR_EVENT_TRANPORT_ON_COMPLETED,
    BT_HCI_IF_MNGR_EVENT_SET_TRANS_PARMS_COMPLETED,
    BT_HCI_IF_MNGR_EVENT_SEND_RADIO_CMD_COMPLETED,
    BT_HCI_IF_MNGR_EVENT_HCI_CONFIG_COMPLETED,
    BT_HCI_IF_MNGR_EVENT_TRANPORT_OFF_COMPLETED 
} BtHciIfMngrEventType;

/*---------------------------------------------------------------------------
 * BtHciIfHciEventData type
 *
 *  HCI data that was received in an HCI event that is to be forwarded 
 *  to a layer user (manager or client)
 */
typedef struct tagBtHciIfHciEventData {
    BtHciIfHciEventType    eventType;   /* Event ending HCI operation */
    McpU8               parmLen;            /* Length of HCI event parameters */
    McpU8               *parms;             /* HCI event parameters */
} BtHciIfHciEventData;

/*---------------------------------------------------------------------------
 * BtHciIfMngrEvent type
 *
 *  The data that is passed in an event to a manager 
 */
typedef struct tagBtHciIfMngrEvent {
    BtHciIfObj          *reportingObj;
    BtHciIfMngrEventType    type;
    BtHciIfStatus           status;

    union {
        /* Applicable when type == BT_HCI_IF_MNGR_EVENT_SEND_RADIO_CMD_COMPLETED */
        BtHciIfHciEventData hciEventData;
    } data;
} BtHciIfMngrEvent;

/*---------------------------------------------------------------------------
 * BtHciIfClientEventType type
 *
 *  Type of event that the layer sends a client
 */
typedef enum tagBtHciIfClientEventType {
    BT_HCI_IF_CLIENT_EVENT_HCI_CMD_COMPLETE
} BtHciIfClientEventType;

/*---------------------------------------------------------------------------
 * BtHciIfMngrEvent type
 *
 *  The data that is passed in an event to a client 
 */
typedef struct tagBtHciIfClientEvent {
    BtHciIfClientEventType  type;
    BtHciIfStatus           status;

     /* This field is used to attach user information to HCI command  */
    void            *pUserData;

    union {
        /* Applicable when type == BT_HCI_IF_CLIENT_EVENT_HCI_CMD_COMPLETE */
        BtHciIfHciEventData hciEventData;
    } data;
} BtHciIfClientEvent;

/*---------------------------------------------------------------------------
 * BtHciIfMngrCb type
 *
 *  Function prototype for a manager callback function
 */
typedef void (*BtHciIfMngrCb)(BtHciIfMngrEvent *event);

/*---------------------------------------------------------------------------
 * BtHciIfClientHandle type
 *
 *  A handle that uniquely identifies a client in the interaction with this layer
 */
typedef void* BtHciIfClientHandle;

/*---------------------------------------------------------------------------
 * BtHciIfClientCb type
 *
 *  Function prototype for a client callback function
 */
typedef void (*BtHciIfClientCb)(BtHciIfClientEvent *event);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_StaticInit()
 *
 * Brief:  
 *      Performs static initializations (for all possible instances)
 *
 * Description:
 *
 * Type:
 *      Synchronous
 *
 * Parameters:  void
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 */
 BtHciIfStatus BT_HCI_IF_StaticInit(void);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_Create()
 *
 * Brief:  
 *      Creates an instance of the layer
 *
 * Description:
 *      The function creates a new instance of this layer. There should be a separate instance per
 *      chip with a BT HCI layer.
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId [in] - The id of the chip associated with the instance
 *
 *      mngrCb [in] - Manager callback function
 *
 *      btHciIfObj [out] - Instance object. To be used in subsequent calls.
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 */
BtHciIfStatus BT_HCI_IF_Create(McpHalChipId chipId, BtHciIfMngrCb mngrCb, BtHciIfObj **btHciIfObj);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_Destroy()
 *
 * Brief:  
 *      Destroys an instance of the layer
 *
 * Description:
 *      The function destroys an existing instance of this layer.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      btHciIfObj [in / out] - On entry, the identifier of the existing object. Zeroed in exit to prevent the caller
 *                          from using an invalid pointer.
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 */
BtHciIfStatus BT_HCI_IF_Destroy(BtHciIfObj **btHciIfObj);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrChangeCb()
 *
 * Brief:  
 *      Modifies the manager's callback
 *
 * Description:
 *      The function allows the manager's callback function to be modified. This could be useful
 *      when one entity wishes to delegate managarial responsibility to another entity. The current
 *      callback function is returned to the caller, allowing restoration of the original callback
 *      at a later time.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 */
BtHciIfStatus BT_HCI_IF_MngrChangeCb(BtHciIfObj *btHciIfObj, BtHciIfMngrCb newCb, BtHciIfMngrCb *oldCb);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrTransportOn()
 *
 * Brief:  
 *      Turns on the BT transport.
 *
 * Description:
 *      The function turns on the BT transport. Upon completion, the caller shall receive
 *      a BT_HCI_IF_MNGR_EVENT_TRANPORT_ON_COMPLETED callback notification.
 *
 *      When the transport completes processing this request, the following is true:
 *      - HCI communication channel with the BT chip is operational (without HCI flow control) 
 *      - HCI reset was already issued and acknowledged
 *      
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 * Returns:
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus BT_HCI_IF_MngrTransportOn(BtHciIfObj *btHciIfObj);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrSendRadioCommand()
 *
 * Brief:  
 *      Sends a radio command during transport on
 *
 * Description:
 *      Only the interface manager should call this function.
 *
 *      Commands must be issued one at a time. Commands are sent without HCI flow control
 *      (it is not initialized yet).
 *
 *      Upon completion, the manager shall receive a BT_HCI_IF_MNGR_EVENT_SEND_RADIO_CMD_COMPLETED
 *      notification callback
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 *      hciOpCode [in] - HCI Command Opcode
 *
 *      hciCmdParms [in] - HCI Command parameters. NULL if command has none.
 *
 *      hciCmdParmsLen [in] - Length of hciCmdParms. 0 if hciCmdParms is NULL
 *
 * Returns:
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus BT_HCI_IF_MngrSendRadioCommand(   BtHciIfObj      *btHciIfObj,    
                                                            BtHciIfHciOpcode    hciOpcode, 
                                                            McpU8           *hciCmdParms, 
                                                            McpU8           hciCmdParmsLen);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrSetTranParms()
 *
 * Brief:  
 *      Sets the transport-specific communications parameters 
 *
 * Description:
 *      This function may be used by the interface manager to change the default
 *      transport parameters. The specific parameters depened on the physical transport
 *      in use (e.g., UART).
 *
 *      Upon completion, the manager shall receive a BT_HCI_IF_MNGR_EVENT_SET_TRANS_PARMS_COMPLETED
 *      notification callback
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 *      tranParms [in] - Transport parameters.
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus   BT_HCI_IF_MngrSetTranParms(BtHciIfObj *btHciIfObj, BtHciIfTranParms *tranParms);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrConfigHci()
 *
 * Brief:  
 *      Completes HCI configuration (HCI flow control)
 *
 * Description:
 *      The interface manager should call this function to complete the initialization
 *      of the BT transport. The function configures BT HCI flow control and makes the
 *      transport fully operational. Once completed, the client interface may be used 
 *      by various clients to send BT HCI commands using BT_HCI_IF_SendHciCommand().
 *
 *      Upon completion, the manager shall receive a BT_HCI_IF_MNGR_EVENT_HCI_CONFIG_COMPLETED
 *      notification callback
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus BT_HCI_IF_MngrConfigHci(BtHciIfObj *btHciIfObj);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_MngrTransportOff()
 *
 * Brief:  
 *      Turns off the BT transport.
 *
 * Description:
 *      The function turns off the BT transport. Upon completion, the caller shall receive
 *      a BT_HCI_IF_MNGR_EVENT_TRANPORT_OFF_COMPLETED callback notification.
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 * Returns:
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus BT_HCI_IF_MngrTransportOff(BtHciIfObj *btHciIfObj);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_RegisterClient()
 *
 * Brief:  
 *      Registers a client that wishes to send HCI commands
 *
 * Description:
 *      The function registers a new client with the BT interface. The client receives
 *      a handle that it should use in subsequent calls.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      btHciIfObj [in] - Instance object
 *
 *      cb [in] - Client callback function
 *
 *      clientHandle [out] - Client handle
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_NO_RESOURCES - Maximum number of clients reached. 
 */
BtHciIfStatus BT_HCI_IF_RegisterClient(BtHciIfObj *btHciIfObj, BtHciIfClientCb cb, BtHciIfClientHandle *clientHandle);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_DeregisterClient()
 *
 * Brief:  
 *      De-Registers a client
 *
 * Description:
 *      The function de-registers a registered client from the BT interface. 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      clientHandle [in] - Client handle
 *
 * Returns:
 *      BT_HCI_IF_STATUS_SUCCESS - Operation completed successfully.
 */
BtHciIfStatus BT_HCI_IF_DeregisterClient(BtHciIfClientHandle *clientHandle);

/*-------------------------------------------------------------------------------
 * BT_HCI_IF_SendHciCommand()
 *
 * Brief:  
 *      Sends an HCI command
 *
 * Description:
 *      Every registered client may issue a SINGLE command at a time. 
 *  
 *      Commands are sent via the same BT HCI command channel that the BT stack uses (if applicable).
 *
 *      Upon completion, the client shall receive a BT_HCI_IF_CLIENT_EVENT_HCI_CMD_COMPLETE
 *      notification callback
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      clientHandle [in] - Client handle
 *
 *      hciOpCode [in] - HCI Command Opcode
 *
 *      hciCmdParms [in] - HCI Command parameters. NULL if command has none.
 *
 *      hciCmdParmsLen [in] - Length of hciCmdParms. 0 if hciCmdParms is NULL
 *
 *      eventType [in] - The event that is expected to signal completion of the operation. 
 *                      The most common event is "Command Complete". See BtHciIfHciEventType
 *                      enumeration above for a list of events. 
 *                      The "Command Status" event is always checked. If a "Command Status" event is
 *                      received with an error then the operation is considered complete.
 *                      If a "Command Status" is received without an error then the command
 *                          will finish when the event matches eventType.

 *
 * Returns:
 *      BT_HCI_IF_STATUS_PENDING - Operation completed successfully.
 *
 *      BT_HCI_IF_STATUS_INTERNAL_ERROR - A fatal error has occurred
 */
BtHciIfStatus BT_HCI_IF_SendHciCommand( BtHciIfClientHandle     clientHandle,   
                                                BtHciIfHciOpcode        hciOpcode, 
                                                McpU8               *hciCmdParms, 
                                                McpU8               hciCmdParmsLen,
                                                McpU8               eventType,
                                                void *              pUserData);

#endif

