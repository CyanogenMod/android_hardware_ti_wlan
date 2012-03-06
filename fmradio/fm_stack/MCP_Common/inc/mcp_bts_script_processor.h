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
*   FILE NAME:      mcp_bts_script_processor.h
*
*   BRIEF:          Generic processor of TI HCI-scripts
*
*   DESCRIPTION:  
*       
*       The script processor is a generic module that is capable of processing an HCI script
*       in "bts" format, and sending it to a TI chip over an HCI transport.
*
*       It has the following capabiltiies:
*       - Multiple clients simultaneosuly (TBD - is this required / possible?).
*       - Per client, it supports downloading of a single script at a time. 
*       - Scripts may be located on a file system or in memory.
*
*       The client of the module activates the script loading by calling MCP_BTS_SP_ExecuteScript and providing
*       callbacks that this module uses to send the script commands over HCI and to notify about execution
*       completion.
*
*       Script Format:
*       ------------

*       |Script Header (32 bytes)|Action 1|Action 2|. . .|Action N|
*
*       Script Header Format:
*       ------------------
*
*       |Magic Number (4 bytes: 0x42535442)|Version Number (4 bytes - 0x00000001)|Reserved (24 bytes)|
*
*       Action Format:
*       ------------
*       |Action Type (2 bytes)|Action Data Size (2 bytes)|Action Data (M bytes, up to MCP_BTS_SP_MAX_SCRIPT_ACTION_DATA_LEN bytes)|
*
*       Action Type may have one of the following values:
*       ------------------------------------------
*       - Send Command
*       - Configure Serial Port Parameters (eg. UART baud rate change)
*       - Delay
*       - Run Script – call execution of another BTS script
*       - Remark
*       - Wait event
*       
*       Supported Script Format:
*       - Script in "bts" format.
*       - The following script actions are SUPPORTED:
*           - Send command
*           - Wait event, where event must be comamnd complete
*           - Delay
*           - Remark
*       - The following scriopt commands are NOT SUPPORTED:
*           - Serial port parameters
*           - Run (nested) script
*           - Any other command that is not listed above. 

*       Generally speaking, the script is a sequence of write operations to the chip. It is NOT a general
*       purpose script with the flexibility allowed by HCI tester scripts. Specifically, the following HCI tester
*       script capabilities are prohibited:
*       - Read values from the chip
*       - Run nested scripts
*       - Use any "programming language" capabilties such as variables, control loops, etc.
*
*
*       Script Processing:
*       ---------------
*       First the script header is read and verified for compliance with the expected format.
*
*       Next, script actions are processed one at a time. Every action is analyzed, its action type
*       is inspected, and handled accordingly.
*
*       Note that a wait command complete event action must follow every send command action,
*       before the next send command action.
*
*       There is no explicit indication for the end of the script. Execution completes when the entire
*       contents of the file were processed. It follows that the file mus contain complete and valid
*       entities, in the order specified above. 
*
*       Tasks
*       -----
*       The script processor runs in the context of its callers.Note that the delay script action causes the
*       caller to block for the duration specified in the action.
*
*       Error Handling:
*       ------------
*       The scripts are expected to fully comply with all of the constraints listed above. Violation of any constraint
*       leads to immediate execution failure. The same is true of command execution results; Failure to execute
*       a script command leads to immediate execution failure. 
*
*       Failrue reason is communicated to the caller when script execution completes
*                   
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __MCP_BTS_SCRIPT_PROCESSOR
#define __MCP_BTS_SCRIPT_PROCESSOR

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_hal_fs.h"


/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * McpBtsSpContext Forward declaration
 */
typedef struct tagMcpBtsSpContext McpBtsSpContext;

/*-------------------------------------------------------------------------------
 * McpBtsSpScriptActionDataLenType typedef
 *
 * Represents the length of a script command action data

 */
typedef McpU16  McpBtsSpScriptActionDataLenType;

/*-------------------------------------------------------------------------------
 * MCP_BTS_SP_MAX_SCRIPT_ACTION_DATA_LEN
 *
 * The maximum supported action data length
 */
#define MCP_BTS_SP_MAX_SCRIPT_ACTION_DATA_LEN   ((McpBtsSpScriptActionDataLenType)300)

/*
 * McpBtsSpStatus typedef
 *
 *  Status code for conveying results between this module and its clients
 */
typedef enum tagMcpBtsSpStatus {
    MCP_BTS_SP_STATUS_SUCCESS           = MCP_HAL_STATUS_SUCCESS,
    MCP_BTS_SP_STATUS_FAILED                = MCP_HAL_STATUS_FAILED,
    MCP_BTS_SP_STATUS_PENDING           = MCP_HAL_STATUS_PENDING,
    MCP_BTS_SP_STATUS_IN_PROGRESS       = MCP_HAL_STATUS_IN_PROGRESS,
    MCP_BTS_SP_STATUS_NO_RESOURCES      = MCP_HAL_STATUS_NO_RESOURCES,
    MCP_BTS_SP_STATUS_INVALID_PARM      = MCP_HAL_STATUS_INVALID_PARM,
    MCP_BTS_SP_STATUS_NOT_SUPPORTED = MCP_HAL_STATUS_NOT_SUPPORTED,
    MCP_BTS_SP_STATUS_TIMEOUT           = MCP_HAL_STATUS_TIMEOUT,
    MCP_BTS_SP_STATUS_INTERNAL_ERROR    = MCP_HAL_STATUS_INTERNAL_ERROR,
    MCP_BTS_SP_STATUS_IMPROPER_STATE    = MCP_HAL_STATUS_IMPROPER_STATE,

    /* The specified script file was not found on the file system in the specified location */
    MCP_BTS_SP_STATUS_FILE_NOT_FOUND    = MCP_HAL_STATUS_OPEN,

    /* Some FFS error occurred while accesing the script file */
    MCP_BTS_SP_STATUS_FFS_ERROR,

    /*
        The processed script is invalid. Exact reason varies.

        This status code is used to notify the caller that script execution 
        failed.

        [ToDo] Decide if we want a detailed error code or just the general "Invalid Script" status.
        Detailed codes may be:
        - invalid magic number
        - action data too long
        - unsupported / unrecognized action type
        - Others?
    */
    MCP_BTS_SP_STATUS_INVALID_SCRIPT,

    /* HCI Command execution failed since HCI transport is not initialized */
    MCP_BTS_SP_STATUS_HCI_INIT_ERROR,

    /* HCI Command execution failed since the HCI command is invalid */
    MCP_BTS_SP_STATUS_INVALID_HCI_CMD,

    /* HCI Command execution failed due to some fatal HCI transport error */
    MCP_BTS_SP_STATUS_HCI_TRANSPORT_ERROR,

    MCP_BTS_SP_STATUS_EXECUTION_ABORTED
} McpBtsSpStatus;

/*
 * McpBtsSpSendHciScriptCmdCb typedef
 *
 *  Prototype of client callback function that this module uses to send
 *  HCI commands over an HCI transport layer.
 *
 *  Parameters:
 *      context [in]            - Handle to the client
 *      hciOpcode [in]      - HCI opcode of command
 *      hciCmdParms [in]        - HCI Command Parameters
 *      hciCmdParmsLen [in]     - Length of hciCmdParms
 *
 *  Returns:
 *      MCP_BTS_SP_STATUS_PENDING
 *      MCP_BTS_SP_STATUS_INVALID_HCI_CMD
 *      MCP_BTS_SP_STATUS_HCI_INIT_ERROR
 *      MCP_BTS_SP_STATUS_INTERNAL_ERROR        
 */
typedef McpBtsSpStatus (*McpBtsSpSendHciScriptCmdCb)(   McpBtsSpContext         *context,
                                                                McpU16              hciOpcode, 
                                                                McpU8               *hciCmdParms, 
                                                                McpUint                 hciCmdParmsLen);

typedef enum tagMcpBtSpFlowControlType {
    MCP_BTS_SP_FLOW_CTRL_TYPE_NO_CHANGE,
    MCP_BTS_SP_FLOW_CTRL_TYPE_NO_FLOW_CONTROL,
    MCP_BTS_SP_FLOW_CTRL_TYPE_HARDWARE_FLOW_CONTROL,
    MCP_BTS_SP_FLOW_CTRL_TYPE_PACKET_WISE,
    MCP_BTS_SP_FLOW_CTRL_TYPE_THREE_WIRE_H5
} McpBtSpFlowControlType;

typedef McpBtsSpStatus (*McpBtsSpSetTranParmsCb)(   McpBtsSpContext             *context,
                                                        McpUint                 speed, 
                                                        McpBtSpFlowControlType  flowCtrlType);

/*
 * McpBtsSpExecuteCompleteCb typedef
 *
 *  Prototype of client callback function that this module calls
 *  to notify the client that the script execution completed.
 *
 *  Parameters:
 *      context [in]    - Handle to the client
 *      status [in]     - completion status. One of:
 *                      MCP_BTS_SP_STATUS_SUCCESS
 *                      MCP_BTS_SP_STATUS_FFS_ERROR
 *                      MCP_BTS_SP_STATUS_INVALID_SCRIPT
 *                      MCP_BTS_SP_STATUS_INVALID_HCI_CMD
 *                      MCP_BTS_SP_STATUS_HCI_TRANSPORT_ERROR
 *                      MCP_BTS_SP_STATUS_INTERNAL_ERROR
 */
typedef void (*McpBtsSpExecuteCompleteCb)(McpBtsSpContext *context, McpBtsSpStatus status);

/*
 * McpBtsSpScriptLocationType typedef
 *
 *  Represents the script location. Options are the FFS and momory
 */
typedef enum tagMcpBtsSpScriptLocationType {
    MCP_BTS_SP_SCRIPT_LOCATION_FS,
    MCP_BTS_SP_SCRIPT_LOCATION_MEMORY
} McpBtsSpScriptLocationType;

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/
 
/*
 * McpBtsSpScriptLocation structure
 *
 *  Structure that contains detailed information about a script to be executed.
 *
 *  Clients should fill this structure and pass it when calling MCP_BTS_SP_ExecuteScript
 */
typedef struct tagMcpBtsSpScriptLocation {
    /* Location Type: FFS or Memoty */
    McpBtsSpScriptLocationType      locationType;
    
    union {
        /* For FFS scripts - full path of script file*/
        McpUtf8 *fullFileName;

        /* For memory */
        struct {
            McpU8   *address;   /* address in memory of script data */
            McpUint size;       /* Size in bytes of memory script */
        } memoryData;
    } locationData;
} McpBtsSpScriptLocation;

typedef struct tagMcpBtsSpExecuteScriptCbData {
    /* Callback function that is used to send HCI commands*/
    McpBtsSpSendHciScriptCmdCb      sendHciCmdCb;

    /* Callback function that is used to set transport parameters */
    McpBtsSpSetTranParmsCb          setTranParmsCb;
        
    /* Callback function that is used to send completion notification */
    McpBtsSpExecuteCompleteCb       execCompleteCb;
} McpBtsSpExecuteScriptCbData;

typedef enum _McpBtsSpProcessingState {
    MCP_BTS_SP_PROCESSING_STATE_NONE,
    MCP_BTS_SP_PROCESSING_STATE_PROCESS_NEXT_ACTION,
    MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_COMMAND_COMPLETE,
    MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_LAST_COMMAND_COMPLETE,
    MCP_BTS_SP_PROCESSING_STATE_DONE
} McpBtsSpProcessingState;

/*
 * McpBtsSpContext structure
 *
 *  Structure that contains information that is used to manage the processing of the script.
 *  Every processed script shall have a separate context.
 *
 *  The structure definition is exposed in the header file (rather than in the source file) since
 *  it is the caller's responsibility to allocate memory for the contexts.
 *
 *  The structure includes script location information but is not using the 
 *  McpBtsSpScriptLocation type. The reason is that McpBtsSpScriptLocation is for clients, whereas the
 *  information required for scripts processing is different.
 */
struct tagMcpBtsSpContext {
    McpBtsSpScriptLocationType      locationType;

    /* The valid filed depends on locationType (the union tag) */
    union {
        McpHalFsFileDesc                fileDesc;
        McpU8                       *memoryPos;
    } locationData;

    McpUint                         scriptSize;
    
    McpBtsSpExecuteScriptCbData     cbData;

    McpBool                         asyncExecution;     /* Indicates if script execution completes asynchronously */

    McpBtsSpProcessingState         processingState;        /* state of the processing state machine */
    McpUint                         scriptProcessingPos;    /* Current position in the script */

    /* Buffer to hold action data - could be allocated on stack but would increase stack size */
    McpU8                           scriptActionData[MCP_BTS_SP_MAX_SCRIPT_ACTION_DATA_LEN];

    McpBool                         abortRequested;

};

/*-------------------------------------------------------------------------------
 * MCP_BTS_SP_ExecuteScript()
 *
 * Brief:  
 *      Executes a script
 *
 * Description:
 *      This function is called to execute a script and notify the caller upon completion.
 *
 *      The caller specifies the location of the script and callbacks for sending HCI commands
 *      and for receiving completion notification. The caller allocates a context structure and
 *      passes it as an argument. The caller must have the context memory valid until execution
 *      completes.
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      scriptLocation [in] - The location of the script
 *      cbData [in] - Callbacks for various operations that are needed for script execution
 *      context [out] - Caller allocated context that stores script processing information
 *      pData [in\out] - User data for the current action.
 *      
 *
 * Returns:
 *      MCP_BTS_SP_STATUS_SUCCESS
 *      MCP_BTS_SP_STATUS_PENDING
 *      MCP_BTS_SP_STATUS_FILE_NOT_FOUND
 *      MCP_BTS_SP_STATUS_NOT_SUPPORTED
 *      MCP_BTS_SP_STATUS_FFS_ERROR
 *      MCP_BTS_SP_STATUS_INVALID_SCRIPT
 */
McpBtsSpStatus MCP_BTS_SP_ExecuteScript(    const McpBtsSpScriptLocation        *scriptLocation, 
                                                    const McpBtsSpExecuteScriptCbData   *cbData,
                                                    McpBtsSpContext                     *context);

McpBtsSpStatus MCP_BTS_SP_AbortScriptExecution(McpBtsSpContext *context);
    
/*-------------------------------------------------------------------------------
 * MCP_BTS_SP_HciCmdCompleted()
 *
 * Brief:  
 *      Function that the caller uses to notify that an HCI Command Complete was received
 *
 * Description:
 *      This function should be called by the client to notify the script processor that a 
 *      "Command Complete" HCI event was recevied for the script that is processed,
 *      and associated with the specified context.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      context [in] - The context that was given in the call 
 *      eventParms [in] - The HCI parameters that were received with the event
 *      eventParmsLen [in] - The length of eventParms
 */
void MCP_BTS_SP_HciCmdCompleted(    McpBtsSpContext     *context,
                                                McpU8           *eventParms,
                                                McpU8           eventParmsLen);

/*-------------------------------------------------------------------------------
 * MCP_BTS_SP_SetTranParmsCompleted()
 *
 * Brief:  
 *      Function that the caller uses to notify that transport parameters setting completed
 *
 * Description: None
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      context [in] - The context that was given in the call 
 */
void MCP_BTS_SP_SetTranParmsCompleted(McpBtsSpContext   *context);

#endif /*__MCP_BTS_SCRIPT_PROCESSOR */

