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
#include "mcp_hal_os.h"
#include "mcp_hal_string.h"
#include "mcp_hal_misc.h"
#include "mcp_hal_memory.h"
#include "mcp_defs.h"
#include "mcp_endian.h"
#include "mcp_bts_script_processor.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

/*
    The value 0x42535442 stands for (in ASCII) BTSB
    which is Bluetooth Script Binary
*/

/* Define The maximum representation of McpUint in ASCII*/
#define MAX_MCP_UINT_SIZE_IN_ASCII    10
MCP_STATIC  const char	invalidMcpempStr[] = "formatMsg is too long";

/* Global variable for all the debug functions bellow */
#define MAX_MCP_TMP_STR_SIZE    27
MCP_STATIC  char	tempStr[MAX_MCP_TMP_STR_SIZE+1];



#define MCP_BTS_SP_FILE_HEADER_MAGIC                0x42535442

typedef McpU16 McpBtsSpScriptActionType;

#define MCP_BTS_SP_SCRIPT_ACTION_SEND_COMMAND               ((McpBtsSpScriptActionType)1)
#define MCP_BTS_SP_SCRIPT_ACTION_WAIT_FOR_COMMAND_COMPLETE  ((McpBtsSpScriptActionType)2)
#define MCP_BTS_SP_SCRIPT_ACTION_SERIAL_PORT_PARMS          ((McpBtsSpScriptActionType)3)
#define MCP_BTS_SP_SCRIPT_ACTION_SLEEP                      ((McpBtsSpScriptActionType)4)
#define MCP_BTS_SP_SCRIPT_ACTION_RUN_SCRIPT                 ((McpBtsSpScriptActionType)5)
#define MCP_BTS_SP_SCRIPT_ACTION_REMARK                     ((McpBtsSpScriptActionType)6)

typedef McpU8   McpBtsSpHciEventStatus;

#define MCP_BTS_SP_HCI_EVENT_STATUS_SUCCESS                 ((McpBtsSpHciEventStatus)0x0)

#define MCP_BTS_SP_HCI_EVENT_STATUS_OFFSET                  ((McpBtsSpHciEventStatus)3)

/*
    
*/
typedef struct _MCP_BTS_SP_ScriptHeader 
{
    McpU32 magicNumber;
    McpU32 version;
    McpU8 reserved[24];
} MCP_BTS_SP_ScriptHeader;

typedef struct _MCP_BTS_SP_ScriptActionHeader 
{
    McpBtsSpScriptActionType actionType;
    McpU16 actionDataLen;
} MCP_BTS_SP_ScriptActionHeader;

typedef struct _MCP_BTS_SP_ActionSerialPortParameters
{
    McpU32 baudRate;
    McpU32 flowControl;
} MCP_BTS_SP_ActionSerialPortParameters;

typedef struct _MCP_BTS_SP_ActionSleep
{
    unsigned long nDurationInMillisec; /*  in milliseconds */
} MCP_BTS_SP_ActionSleep;

typedef enum {
    MCP_BTS_SP_PROCESSING_EVENT_START,
    MCP_BTS_SP_PROCESSING_EVENT_COMMAND_COMPLETE,
    MCP_BTS_SP_PROCESSING_EVENT_SET_TRAN_PARMS_COMPLETE
} McpBtsSpProcessingEvent;

static McpBtsSpStatus _MCP_BTS_SP_VerifyMagicNumber(McpBtsSpContext *context);

static McpBtsSpStatus _MCP_BTS_SP_ProcessScriptCommands(McpBtsSpContext *context,
                                                        McpBtsSpProcessingEvent processingEvent,
                                                        void *eventData);

static McpBtsSpStatus _MCP_BTS_SP_ProcessNextScriptAction(McpBtsSpContext *context,
                                                          McpBool *moreActions);

McpBtsSpStatus _MCP_BTS_SP_GetNextAction(McpBtsSpContext *context, 
                                         McpU16 *actionDataActualLen, 
                                         McpBtsSpScriptActionType *actionType,
                                         McpU8 *actionData, 
                                         McpU16 actionDataMaxLen, 
                                         McpBool *moreActions);

static void _MCP_BTS_SP_CompleteExecution(McpBtsSpContext *context,
                                          McpBtsSpStatus executionStatus);

static const char *_MCP_BTS_SP_DebugStatusStr(McpBtsSpStatus status);

static McpBtsSpStatus _MCP_BTS_SP_OpenScript(McpBtsSpContext *context,
                                             const McpBtsSpScriptLocation *scriptLocation);
static McpBtsSpStatus _MCP_BTS_SP_ReadActionHeader(McpBtsSpContext *context,
                                                   MCP_BTS_SP_ScriptActionHeader *actionHeader);
static McpBtsSpStatus _MCP_BTS_SP_ReadScript(McpBtsSpContext *context,
                                             void *buf,
                                             McpUint len);
static McpBtsSpStatus _MCP_BTS_SP_CloseScript(McpBtsSpContext *context);

static McpBtsSpStatus _MCP_BTS_SP_FsOpenScript(McpBtsSpContext *context,
                                               const McpUtf8 *fullFileName);
static McpBtsSpStatus _MCP_BTS_SP_FsReadScript(McpBtsSpContext *context,
                                               void *buf,
                                               McpUint len);
static McpBtsSpStatus _MCP_BTS_SP_FsCloseScript(McpBtsSpContext *context);
static McpBtsSpStatus _MCP_BTS_SP_MemOpenScript(McpBtsSpContext *context, 
                                                McpU8 *address,
                                                McpUint size);
static McpBtsSpStatus _MCP_BTS_SP_MemReadScript(McpBtsSpContext *context,
                                                void *buf,
                                                McpUint len);
static McpBtsSpStatus _MCP_BTS_SP_MemCloseScript(McpBtsSpContext *context);


/*
    This function starts the script execution

    It may return synchronously in the following cases:
    1. It detects an error at this phase; Or
    2. The script doesn't contain any HCI command or delay (e.g., only comments)

    Otherwise, the function returns MCP_BTS_SP_STATUS_PENDING and completion will be notified
    via execCompleteCb
*/
McpBtsSpStatus MCP_BTS_SP_ExecuteScript(const McpBtsSpScriptLocation *scriptLocation, 
                                        const McpBtsSpExecuteScriptCbData *cbData,
                                        McpBtsSpContext *context)
{
    McpBtsSpStatus status;
    
    /* 
        Initialize processing management fields. 

        Applicable fields are initialized from arguments. Others are initialized using default values
    */
    context->locationType = scriptLocation->locationType;
    context->locationData.fileDesc = MCP_HAL_FS_INVALID_FILE_DESC;
    context->scriptSize = 0;
    context->cbData = *cbData;
    context->asyncExecution = MCP_FALSE;
    context->processingState = MCP_BTS_SP_PROCESSING_STATE_NONE;
    context->scriptProcessingPos = 0;
    context->abortRequested = MCP_FALSE;
    
    /* Open the script file */
    status = _MCP_BTS_SP_OpenScript(context, scriptLocation);
    
    if (status != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_ERROR(("MCP_BTS_SP_ExecuteScript: _MCP_BTS_SP_OpenScript Failed (%s), Exiting",
                       _MCP_BTS_SP_DebugStatusStr(status)));
        _MCP_BTS_SP_CompleteExecution(context, status);
        return status;
    }

    /* Verify that the magic number i the script header is valid */
    status = _MCP_BTS_SP_VerifyMagicNumber(context);
    
    if (status != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_ERROR(("MCP_BTS_SP_ExecuteScript: _MCP_BTS_SP_VerifyMagicNumber Failed (%s), Exiting",
                       _MCP_BTS_SP_DebugStatusStr(status)));
        _MCP_BTS_SP_CompleteExecution(context, status);
        return status;
    }

    /* Start processing script actions */
    status = _MCP_BTS_SP_ProcessScriptCommands(context,
                                               MCP_BTS_SP_PROCESSING_EVENT_START,
                                               NULL);

    if (status == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Execution completed successfully */
        _MCP_BTS_SP_CompleteExecution(context, MCP_BTS_SP_STATUS_SUCCESS);
        
    }
    else if (status == MCP_BTS_SP_STATUS_PENDING)
    {
        /* Do Nothing, Wait for command complete event to continue processing */
    }
    else
    {
        /* Commands processing failed, abort and notify failure */
        _MCP_BTS_SP_CompleteExecution(context, status); 
    }
    
    return status;
}

McpBtsSpStatus MCP_BTS_SP_AbortScriptExecution(McpBtsSpContext *context)
{
    context->abortRequested = MCP_TRUE;

    return MCP_BTS_SP_STATUS_PENDING;
}

McpBtsSpStatus _MCP_BTS_SP_VerifyMagicNumber(McpBtsSpContext *context)
{
    McpBtsSpStatus spStatus;
    MCP_BTS_SP_ScriptHeader header;

    /* Read script header */
    spStatus = _MCP_BTS_SP_ReadScript(context, &header, sizeof(header));

    if (spStatus != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_ERROR(("_MCP_BTS_SP_VerifyMagicNumber: Failed getting script header (%s)",
                       _MCP_BTS_SP_DebugStatusStr(spStatus)));
        return spStatus;
    }

    /* 
        check magic number

        [ToDo] Consider possible Endianity problems
    */
    if (header.magicNumber != MCP_BTS_SP_FILE_HEADER_MAGIC)
    {
        MCP_LOG_ERROR(("_MCP_BTS_SP_VerifyMagicNumber: Invalid Magic Number (%x), Expected (%x)", 
                       header.magicNumber,
                       MCP_BTS_SP_FILE_HEADER_MAGIC));
        return MCP_BTS_SP_STATUS_INVALID_SCRIPT;
    }

    return MCP_BTS_SP_STATUS_SUCCESS;
}

/*
    The main state machine of the script processor
*/
McpBtsSpStatus _MCP_BTS_SP_ProcessScriptCommands(McpBtsSpContext *context,
                                                 McpBtsSpProcessingEvent processingEvent,
                                                 void *eventData)
{
    McpBtsSpStatus spStatus = MCP_BTS_SP_STATUS_SUCCESS;
    McpBool keepProcessing;

    MCP_UNUSED_PARAMETER(eventData);
    
    MCP_LOG_INFO(("_MCP_BTS_SP_ProcessScriptCommands: Processing Event: %d", processingEvent));

    keepProcessing = MCP_TRUE;
    
    while ((keepProcessing == MCP_TRUE) && (context->abortRequested == MCP_FALSE))
    {
        keepProcessing = MCP_FALSE;

        MCP_LOG_INFO(("_MCP_BTS_SP_ProcessScriptCommands: State = %d", context->processingState));
        
        switch (context->processingState)
        {
            case MCP_BTS_SP_PROCESSING_STATE_NONE:

                if (processingEvent == MCP_BTS_SP_PROCESSING_EVENT_START)
                {
                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_PROCESS_NEXT_ACTION;             
                }
                else
                {
                    MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessScriptCommands: Unexpected Processing Event (%d)",
                                   processingEvent));

                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                    spStatus = MCP_BTS_SP_STATUS_INTERNAL_ERROR;
                }

                keepProcessing = MCP_TRUE;
                
            break;

            case MCP_BTS_SP_PROCESSING_STATE_PROCESS_NEXT_ACTION:
            {
                McpBool moreActions;
                
                /* Process next script action as long as we do not have to wait for a command complete event */
                do {
                    spStatus = _MCP_BTS_SP_ProcessNextScriptAction(context, &moreActions);
                } while ((spStatus == MCP_BTS_SP_STATUS_SUCCESS) && (moreActions == MCP_TRUE));

                /* [ToDo] - Handle the case of the last actio in the script being a "Send Command" (illegal) */
                if (spStatus == MCP_BTS_SP_STATUS_PENDING)
                {
                    /* The first time we wait for a command complete event we will record that fact */
                    context->asyncExecution = MCP_TRUE;

                    /* */
                    if (moreActions == MCP_TRUE)
                    {
                        context->processingState = MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_COMMAND_COMPLETE;
                    }
                    else
                    {
                        context->processingState = MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_LAST_COMMAND_COMPLETE;
                    }
                }
                else
                {
                    /* 
                        We get here in 2 cases:
                        1. script processing error
                        2. Last script action was not a "Send Command" action

                        In that case we terminate. spStatus stores the completion status
                    */
                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                    keepProcessing = MCP_TRUE;
                }
            }
            break;
            
            case MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_COMMAND_COMPLETE:

                if (processingEvent == MCP_BTS_SP_PROCESSING_EVENT_COMMAND_COMPLETE)
                {
                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_PROCESS_NEXT_ACTION; 
                }
                else
                {
                    MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessScriptCommands: Unexpected Processing Event (%d)",
                                   processingEvent));

                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                    spStatus = MCP_BTS_SP_STATUS_INTERNAL_ERROR;
                }
                
                keepProcessing = MCP_TRUE;
                
            break;

            case MCP_BTS_SP_PROCESSING_STATE_WAIT_FOR_LAST_COMMAND_COMPLETE:
                
                if (processingEvent == MCP_BTS_SP_PROCESSING_EVENT_COMMAND_COMPLETE)
                {
                    spStatus = MCP_BTS_SP_STATUS_SUCCESS;
                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                }
                else
                {
                    MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessScriptCommands: Unexpected Processing Event (%d)",
                                   processingEvent));

                    context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                    spStatus = MCP_BTS_SP_STATUS_INTERNAL_ERROR;
                }

                keepProcessing = MCP_TRUE;
                
            break;
            
            case MCP_BTS_SP_PROCESSING_STATE_DONE:

                MCP_LOG_INFO(("_MCP_BTS_SP_ProcessScriptCommands: Completed (%s)",
                              _MCP_BTS_SP_DebugStatusStr(spStatus)));
                
                _MCP_BTS_SP_CompleteExecution(context, spStatus);

            break;
            
            default:
                MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessScriptCommands: Invalid Processing State (%s)",
                               context->processingState));

                context->processingState = MCP_BTS_SP_PROCESSING_STATE_DONE;
                spStatus = MCP_BTS_SP_STATUS_INTERNAL_ERROR;
                keepProcessing = MCP_TRUE;
        }
    }

    if (context->abortRequested == MCP_TRUE)
    {
        _MCP_BTS_SP_CompleteExecution(context, MCP_BTS_SP_STATUS_EXECUTION_ABORTED);
    }
    
    return spStatus;
}

McpBtsSpStatus _MCP_BTS_SP_ProcessNextScriptAction(McpBtsSpContext *context, McpBool *moreActions)
{
    McpBtsSpStatus spStatus;
    McpBtsSpScriptActionDataLenType actionDataActualLen;
    McpBtsSpScriptActionType actionType;

    /* 
        [ToDo] - Check the need for the mopreCommand and its usage
        This condition should be true when the script contains no actions (just a header) => 
        the test may be moved elsewhere, and tested only once, after reading the header
    */
    if (context->scriptProcessingPos >= context->scriptSize)
    {
        *moreActions = MCP_FALSE;
        return MCP_BTS_SP_STATUS_SUCCESS;
    }

    /* Read next action from script and parse it */
    spStatus = _MCP_BTS_SP_GetNextAction(context, 
                                         &actionDataActualLen,
                                         &actionType,
                                         context->scriptActionData, 
                                         MCP_BTS_SP_MAX_SCRIPT_ACTION_DATA_LEN,
                                         moreActions);

    if (spStatus != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessNextScriptAction: _MCP_BTS_SP_GetNextAction Failed (%s)", 
                       _MCP_BTS_SP_DebugStatusStr(spStatus)));
        return spStatus;
    }

    /* Handle actions according to their type */
    /* [ToDo] Detect the case where 2 commands are sent without waiting for an event in-between */
    switch (actionType)
    {
        case MCP_BTS_SP_SCRIPT_ACTION_SEND_COMMAND:
        {
            /* Prepare Send HCI Command parameters */
            /* [ToDo] Define HCI command structure / define symbolic constants for offsets */
            McpU8   hciParmLen = context->scriptActionData[3];
            McpU16  hciOpcode;
            McpU8   *hciParms = &context->scriptActionData[4];
            /*[ToDo Zvi] We should use utils general function here */
            hciOpcode = (McpU16)( ((McpU16) *(&context->scriptActionData[1]+1) << 8) | ((McpU16) *&context->scriptActionData[1]) ); 
                        
            /* Send the HCI command via the client's supplied callback function */
            spStatus = (context->cbData.sendHciCmdCb)(  context,
                                                hciOpcode,
                                                hciParms,
                                                hciParmLen);
            
            /* 
                We return pending only when waiting for a command complete. 
                Otherwise, we should be called again by the caller of this function
            */
            if (spStatus == MCP_BTS_SP_STATUS_PENDING)
            {
                spStatus = MCP_BTS_SP_STATUS_SUCCESS;
            }
            else
            {
                /* [ToDo] - FATAL may be too severe as an uncoditional handling of errors */
                MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessNextScriptAction: Failed sending HCI Command via the callback (%d)", 
                               _MCP_BTS_SP_DebugStatusStr(spStatus)));
            }
        }
        break;

        case MCP_BTS_SP_SCRIPT_ACTION_WAIT_FOR_COMMAND_COMPLETE:
            /* Nothing more to do, wait for client to call MCP_BTS_SP_HciCmdCompleted to continue processing */
            spStatus = MCP_BTS_SP_STATUS_PENDING;
            break;

        /* Invalid action for Software scripts */
        case MCP_BTS_SP_SCRIPT_ACTION_SERIAL_PORT_PARMS:
        {
            MCP_BTS_SP_ActionSerialPortParameters *pParams = (MCP_BTS_SP_ActionSerialPortParameters *)&context->scriptActionData[0];

            spStatus = (context->cbData.setTranParmsCb)(context, pParams->baudRate, pParams->flowControl);

            if (spStatus != MCP_BTS_SP_STATUS_SUCCESS)
            {
                /* [ToDo] - FATAL may be too severe as an uncoditional handling of errors */
                MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessNextScriptAction: Setting Speed Callback Failed (%d)", 
                               _MCP_BTS_SP_DebugStatusStr(spStatus)));
            }
        }   
        break;
        
        /* Invalid action for Software scripts */
        case MCP_BTS_SP_SCRIPT_ACTION_RUN_SCRIPT:
            MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessNextScriptAction: Run Script action Disallowed in BTS File"));
            spStatus = MCP_BTS_SP_STATUS_INVALID_SCRIPT;
            break;

        case MCP_BTS_SP_SCRIPT_ACTION_SLEEP:
        {
            /* Delay action - sleep for the specified amount of time */
            MCP_BTS_SP_ActionSleep *pParams = (MCP_BTS_SP_ActionSleep*)&context->scriptActionData[0];
            MCP_HAL_OS_Sleep(pParams->nDurationInMillisec);
        }
        break;
        
        case MCP_BTS_SP_SCRIPT_ACTION_REMARK:
            /* Nothing to do */
            spStatus = MCP_BTS_SP_STATUS_SUCCESS;
            break;

        /* Invalid action type detected */      
        default:
            MCP_LOG_FATAL(("_MCP_BTS_SP_ProcessNextScriptAction: Invalid Action (%d) in BTS File", actionType));
            spStatus = MCP_BTS_SP_STATUS_INVALID_SCRIPT;
            break;
            
    }

    return spStatus;
}

/*
    Reads the next action from the script file, verify correctness and return its individual elements
*/
McpBtsSpStatus _MCP_BTS_SP_GetNextAction(McpBtsSpContext *context, 
                                         McpU16 *actionDataActualLen, 
                                         McpBtsSpScriptActionType *actionType,
                                         McpU8 *actionData, 
                                         McpU16 actionDataMaxLen,
                                         McpBool *moreActions)
{
    McpBtsSpStatus spStatus;
    MCP_BTS_SP_ScriptActionHeader actionHeader;

    /* Read the action header */
    spStatus = _MCP_BTS_SP_ReadActionHeader(context, &actionHeader);

    if (spStatus != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_FATAL(("_MCP_BTS_SP_GetNextAction: Failed reading action header (%s)",
                       _MCP_BTS_SP_DebugStatusStr(spStatus)));
        return spStatus;
    }

    /*
        Parse action header. The format is:

        [ToDo] How is the binary data represented? Do we need to handle endianity?
    */
      *actionType = actionHeader.actionType;
    *actionDataActualLen = actionHeader.actionDataLen;

    /* Verify that actual action data len doesn't exceed the action data buffer max size */
    if (*actionDataActualLen > actionDataMaxLen)
    {       
        MCP_LOG_FATAL(("_MCP_BTS_SP_GetNextAction: Action Data is too long (%d), Max: %d.", 
                            *actionDataActualLen, actionDataMaxLen));
        return MCP_BTS_SP_STATUS_INVALID_SCRIPT;
    }

    /* Read the action data */

    spStatus = _MCP_BTS_SP_ReadScript(context, actionData, *actionDataActualLen);

    if (spStatus != MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_FATAL(("_MCP_BTS_SP_GetNextAction: Failed reading action Data (%s)",
                       _MCP_BTS_SP_DebugStatusStr(spStatus)));
        return spStatus;
    }

    /* If we completed reading the entire script it means there are no more actions (we just read the last one) */
    if (context->scriptProcessingPos >= context->scriptSize)
    {
        *moreActions = MCP_FALSE;
    }
    else
    {
        *moreActions = MCP_TRUE;
    }

    return spStatus;
}

/*
    This function is called to complete script execution from all possible execution paths.
*/
void _MCP_BTS_SP_CompleteExecution(McpBtsSpContext *context, McpBtsSpStatus executionStatus)
{
    if (executionStatus == MCP_BTS_SP_STATUS_SUCCESS)
    {
        MCP_LOG_INFO(("_MCP_BTS_SP_CompleteExecution: SCRIPT EXECUTION SUCCESSFULLY COMPLETED"));
    }
    else if (executionStatus == MCP_BTS_SP_STATUS_EXECUTION_ABORTED)
    {
        MCP_LOG_INFO(("_MCP_BTS_SP_CompleteExecution: SCRIPT EXECUTION ABORTED"));
    }
    else
    {
        MCP_LOG_FATAL(("_MCP_BTS_SP_CompleteExecution: SCRIPT EXECUTION FAILED (%s)", 
                            _MCP_BTS_SP_DebugStatusStr(executionStatus)));
    }

    /* Close script file ([ToDo] - Memory scripts ) */
    _MCP_BTS_SP_CloseScript(context);

    /* If execution completed asynchronously, notify the callback on execution completion status */
    if (context->asyncExecution == MCP_TRUE)
    {
        (context->cbData.execCompleteCb)(context, executionStatus);
    }
}

void MCP_BTS_SP_HciCmdCompleted(McpBtsSpContext *context, 
                                McpU8 *eventParms,
                                McpU8 eventParmsLen)
{
    MCP_UNUSED_PARAMETER(eventParms);
    MCP_UNUSED_PARAMETER(eventParmsLen);
    
/* 
    [ToDo] The eventParms currentl doens't include the HCI opcode, event status, etc. Only the HCI return parms 

    Need to see how to handle this in FMC_Transport
*/
#if 0   
    McpBtsSpStatus spStatus;

    /* Initialize to an invalid event status (in case there is no status available in the event parameters) */
    McpBtsSpHciEventStatus  eventStatus = (McpBtsSpHciEventStatus)0xFF;

    /* Extract event status from parms */
    if (eventParmsLen >= MCP_BTS_SP_HCI_EVENT_STATUS_OFFSET)
    {
        eventStatus = eventParms[MCP_BTS_SP_HCI_EVENT_STATUS_OFFSET];
    }
    
    if (eventType != MCP_BTS_SP_HCI_EVENT_COMMAND_COMPLETE)
    {
        MCP_LOG_FATAL(("MCP_BTS_SP_HciCmdCompleted: Unexpected HCI Event (%d), Status: %d",
                       eventType,
                       eventStatus));   
        _MCP_BTS_SP_CompleteExecution(context,
                                      MCP_BTS_SP_STATUS_EXECUTION_FAILED_UNEXPECTED_HCI_EVENT);
        return;
    }

    /* Now we know we have the expected command complete event */

    /* verify that the command completed successfully */
    if (eventStatus != MCP_BTS_SP_HCI_EVENT_STATUS_SUCCESS)
    {
        MCP_LOG_FATAL(("MCP_BTS_SP_HciCmdCompleted: Command completed unsuccessfully (%d)",
                       eventStatus));  
        _MCP_BTS_SP_CompleteExecution(context,
                                      MCP_BTS_SP_STATUS_EXECUTION_FAILED_COMMAND_FAILED);
        return;
    }
    
#endif

    /* [ToDo] - Pass the command complete status as the event data (instead of the 3rd argument) */
    _MCP_BTS_SP_ProcessScriptCommands(context, MCP_BTS_SP_PROCESSING_EVENT_COMMAND_COMPLETE, NULL);
}

void MCP_BTS_SP_SetTranParmsCompleted(McpBtsSpContext   *context)
{
    _MCP_BTS_SP_ProcessScriptCommands(context, MCP_BTS_SP_PROCESSING_EVENT_SET_TRAN_PARMS_COMPLETE, NULL);
}

McpBtsSpStatus _MCP_BTS_SP_OpenScript(McpBtsSpContext *context, const McpBtsSpScriptLocation *scriptLocation)
{
    if (scriptLocation->locationType == MCP_BTS_SP_SCRIPT_LOCATION_FS)
    {
        return _MCP_BTS_SP_FsOpenScript(context, scriptLocation->locationData.fullFileName);
    }
    else
    {
        return _MCP_BTS_SP_MemOpenScript(context, 
                                         scriptLocation->locationData.memoryData.address, 
                                         scriptLocation->locationData.memoryData.size);
    }
}

McpBtsSpStatus _MCP_BTS_SP_ReadActionHeader(McpBtsSpContext *context,
                                            MCP_BTS_SP_ScriptActionHeader *actionHeader)
{
    McpBtsSpStatus status;
    
    if (context->locationType == MCP_BTS_SP_SCRIPT_LOCATION_FS)
    {
        status = _MCP_BTS_SP_FsReadScript(context,
                                          actionHeader,
                                          sizeof(MCP_BTS_SP_ScriptActionHeader));
    }
    else
    {
        _MCP_BTS_SP_MemReadScript(context, actionHeader, sizeof(MCP_BTS_SP_ScriptActionHeader));

        actionHeader->actionType = MCP_ENDIAN_LEtoHost16((const McpU8*)&actionHeader->actionType);
        actionHeader->actionDataLen = MCP_ENDIAN_LEtoHost16((const McpU8*)&actionHeader->actionDataLen);

        status = MCP_BTS_SP_STATUS_SUCCESS;
    }

    return status;
}

McpBtsSpStatus _MCP_BTS_SP_ReadScript(McpBtsSpContext *context, void *buf, McpUint len)
{
    if (context->locationType == MCP_BTS_SP_SCRIPT_LOCATION_FS)
    {
        return _MCP_BTS_SP_FsReadScript(context, buf, len);
    }
    else
    {
        return _MCP_BTS_SP_MemReadScript(context, buf, len);
    }
}

McpBtsSpStatus _MCP_BTS_SP_CloseScript(McpBtsSpContext *context)
{
    if (context->locationType == MCP_BTS_SP_SCRIPT_LOCATION_FS)
    {
        return _MCP_BTS_SP_FsCloseScript(context);
    }
    else
    {
        return _MCP_BTS_SP_MemCloseScript(context);
    }
}

/*
    Opens the script file and obtains its size
*/
McpBtsSpStatus _MCP_BTS_SP_FsOpenScript(McpBtsSpContext *context,
                                        const McpUtf8 *fullFileName)
{
    McpHalFsStatus fsStatus;
    McpHalFsStat fsStat;

    /* Get file information */
    fsStatus = MCP_HAL_FS_Stat(fullFileName, &fsStat);

    if (fsStatus != MCP_HAL_FS_STATUS_SUCCESS)
    {

        /* Failed to get file information - report and return proper error code */
        MCP_LOG_ERROR(("_MCP_BTS_SP_FsOpenScript: MCP_HAL_FS_Stat Failed (%d)",
                       fsStatus));

        if (fsStatus == MCP_HAL_FS_STATUS_ERROR_NOTFOUND)
        {
            return MCP_BTS_SP_STATUS_FILE_NOT_FOUND;
        }
        else
        {
            return MCP_BTS_SP_STATUS_FFS_ERROR;
        }
    }

    /* Record the file size*/
    context->scriptSize = fsStat.size;
    
    /* Open the script file for reading as a binary file */
    fsStatus = MCP_HAL_FS_Open(fullFileName,  
                               (MCP_HAL_FS_O_RDONLY | MCP_HAL_FS_O_BINARY), 
                               &context->locationData.fileDesc);

    if (fsStatus != MCP_HAL_FS_STATUS_SUCCESS)
    {
        MCP_LOG_ERROR(("_MCP_BTS_SP_FsOpenScript: MCP_HAL_FS_Open Failed (%d)",
                       fsStatus));
        return MCP_BTS_SP_STATUS_FFS_ERROR;
    }

    MCP_LOG_INFO(("_MCP_BTS_SP_FsOpenScript: Successfully opened %s",
                  (char *)fullFileName));       

    return MCP_BTS_SP_STATUS_SUCCESS;
}

/*
    Read data from a file script

    The function reads len bytes from the script. The buf argument must be at least of
    size len. In addition, it must be possible to successfully read len bytes from the file. Otherwise, 
    the function fails and returns an appropriate error code.
*/
McpBtsSpStatus _MCP_BTS_SP_FsReadScript(McpBtsSpContext *context, void *buf, McpUint len)
{
    McpHalFsStatus  fsStatus;
    McpU32  numRead;

    /* Read up to len bytes from the file */
    fsStatus = MCP_HAL_FS_Read(context->locationData.fileDesc,  buf, (McpU32)len, &numRead);
    
    if (fsStatus != MCP_HAL_FS_STATUS_SUCCESS)
    {
        MCP_LOG_ERROR(("_MCP_BTS_SP_FsReadScript: MCP_HAL_FS_Read Failed (%d)", fsStatus));
        return MCP_BTS_SP_STATUS_FFS_ERROR;
    }

    /* Verify that we indeed read len bytes and not less */
    if (numRead != len)
    {
        MCP_LOG_ERROR(("_MCP_BTS_SP_FsReadScript: Expected to read %d bytes, read %d", len, numRead));
        return MCP_BTS_SP_STATUS_FFS_ERROR;
    }

    /* Update position in file */
    context->scriptProcessingPos += numRead;
    
    return MCP_BTS_SP_STATUS_SUCCESS;
}

/*
    Closes the script file
*/
McpBtsSpStatus _MCP_BTS_SP_FsCloseScript(McpBtsSpContext *context)
{
    McpHalFsStatus  fsStatus;
    
    if (context->locationData.fileDesc != MCP_HAL_FS_INVALID_FILE_DESC)
    {
        fsStatus = MCP_HAL_FS_Close(context->locationData.fileDesc);

        context->locationData.fileDesc = MCP_HAL_FS_INVALID_FILE_DESC;
            
        if (fsStatus != MCP_HAL_FS_STATUS_SUCCESS)
        {
            MCP_LOG_ERROR(("_MCP_BTS_SP_FsOpenScriptFile: MCP_HAL_FS_Open Failed (%d)",
                           fsStatus));
            return MCP_BTS_SP_STATUS_FFS_ERROR;
        }
    }

    return MCP_BTS_SP_STATUS_SUCCESS;
}

McpBtsSpStatus _MCP_BTS_SP_MemOpenScript(McpBtsSpContext *context, 
                                         McpU8 *address,
                                         McpUint size)
{
    context->locationData.memoryPos = address;
    
    /* Record the script size*/
    context->scriptSize = size;

    return MCP_BTS_SP_STATUS_SUCCESS;
}

McpBtsSpStatus _MCP_BTS_SP_MemReadScript(McpBtsSpContext *context, void *buf, McpUint len)
{
    MCP_HAL_MEMORY_MemCopy(buf, context->locationData.memoryPos, len);

    context->locationData.memoryPos += len;

    /* Update position in file */
    context->scriptProcessingPos += len;

    return MCP_BTS_SP_STATUS_SUCCESS;
}

McpBtsSpStatus _MCP_BTS_SP_MemCloseScript(McpBtsSpContext *context)
{
    MCP_UNUSED_PARAMETER(context);
    
    return MCP_BTS_SP_STATUS_SUCCESS;
}

/*
    Utility function that formats a number as a string

    [ToDo] - Duplicate of a function in FMC_Utils - Consider making an infrastructure utility function
*/
const char *_MCP_BTS_SP_FormatNumber(const char *formatMsg, McpUint number, char *tempStr,McpUint maxTempStrSize)
{
    McpU16 formatMsgLen;
    formatMsgLen=MCP_HAL_STRING_StrLen(formatMsg);

    /*Verfiy that the tempStr allocation is enough  */
    if ((McpU16)(formatMsgLen+MAX_MCP_UINT_SIZE_IN_ASCII)>maxTempStrSize)
        {            
            return invalidMcpempStr;
        }    

    MCP_HAL_STRING_Sprintf(tempStr, formatMsg, number);

    return tempStr;
}

const char *_MCP_BTS_SP_DebugStatusStr(McpBtsSpStatus status)
{
  

    switch (status)
    {
        case MCP_BTS_SP_STATUS_SUCCESS:             return "SUCCESS";
        case MCP_BTS_SP_STATUS_FAILED:              return "FAILED";
        case MCP_BTS_SP_STATUS_PENDING:             return "PENDING";
        case MCP_BTS_SP_STATUS_INVALID_PARM:        return "INVALID_PARM";
        case MCP_BTS_SP_STATUS_INTERNAL_ERROR:      return "INTERNAL_ERROR";
        case MCP_BTS_SP_STATUS_NOT_SUPPORTED:       return "NOT_SUPPORTED";
        case MCP_BTS_SP_STATUS_FILE_NOT_FOUND:      return "FILE_NOT_FOUND";
        case MCP_BTS_SP_STATUS_FFS_ERROR:           return "FFS_ERROR";
        case MCP_BTS_SP_STATUS_INVALID_SCRIPT:      return "INVALID_SCRIPT";
        case MCP_BTS_SP_STATUS_HCI_INIT_ERROR:      return "HCI_INIT_ERROR";
        case MCP_BTS_SP_STATUS_INVALID_HCI_CMD:     return "INVALID_HCI_CMD";
        case MCP_BTS_SP_STATUS_HCI_TRANSPORT_ERROR: return "HCI_TRANSPORT_ERROR";
        default:                                    return _MCP_BTS_SP_FormatNumber("INVALID Status:%x",
                                                                                    status,
                                                                                    tempStr,
                                                                                    MAX_MCP_TMP_STR_SIZE);
    }
}


