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
*   FILE NAME:      mcp_hci_sequencer.h
*
*   BRIEF:          Generic HCI sequence executer
*
*   DESCRIPTION:  
*       
*       The HCI sequencer is able to send a sequnce of HCI commands. It saves 
*       the user the burden of handling the sequnce by itself, waiting for each
*       command complete and triggering the next command.
*
*       It has the following capabiltiies:
*       - Multiple clients simultaneosuly
*       - gracefull abort the HCI sequence during execution
*
*       The HCI sequencer is activated by calling TI_HciSeq_RunSequence. The 
*       caller should supply a sequence of HCI commands preparation functions, 
*       which will be called in order to fill in command paramters in the 
*       command token.
*       The user may cancel the execution of a sequence by calling 
*       TI_HciSeq_CancelSequence. The HCI sequencer will wait for the current 
*       command to complete execution before starting the next sequence. If a 
*       new sequence needs to be started immediatly, the user may call 
*       TI_HciSeq_RunSequence directly, which will have the same effect.
*
*       Tasks
*       -----
*       The HCI sequencer runs in the context of its callers. 
*
*       Error Handling:
*       ------------
*       Command execution results are supplied to the originator using the 
*       supllied callback, if any. The originator is responsible to examine the
*       specific result for any erroneous replies (as this can vary between 
*       commands). If an error is encountered, the originator may continue or 
*       cancel the rest of the sequence. Note that a callback for the last
*       command must be provided to indicate sequence completion.
*                   
*   AUTHOR:   Ronen kalish
*
\*******************************************************************************/
#ifndef __MCP_HCI_SEQUENCER__
#define __MCP_HCI_SEQUENCER__

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"
#include "bt_hci_if.h"
#include "fm_transport_if.h"
/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
#define HCI_SEQ_MAX_CMDS_PER_SEQUENCE      (5)

/*-------------------------------------------------------------------------------
 * McpHciSeqCmdToken
 *
 * holds a sequence command.
 */
typedef struct _McpHciSeqCmdToken
{       
    BtHciIfClientCb             callback;       
    BtHciIfHciOpcode            eHciOpcode;
    McpU8                       *pHciCmdParms;
    McpU8                       uhciCmdParmsLen;
    McpU8                       uCompletionEvent;
    void                        *pUserData;
} McpHciSeqCmdToken;

/* comamnd preparetion callback - used to prepare next command in sequence */
typedef void (*McpHciSeqPrepCB)(McpHciSeqCmdToken *pToken, void *pUserData);

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/
/*-------------------------------------------------------------------------------
 * McpHciSeqCmd
 *
 * holds a sequence command.
 */
typedef struct _McpHciSeqCmd
{
    McpHciSeqPrepCB         fCommandPrepCB; /* called to prepare the command before sending */
    void                    *pUserData;     /* user data supplied to the above CB */
} McpHciSeqCmd;

/*-------------------------------------------------------------------------------
 * MCP_HciSeq_Context
 *
 * holds a sequence context. More than one context may be used simultaneously.
 * The same context can be recycled to run several sequences, but only one sequence at
 * a time.
 */
typedef struct _MCP_HciSeq_Context 
{   
    McpHciSeqCmdToken       command;                /* the actual command token */
    BtHciIfClientHandle     handle; /*Handle for every client for the BT_HCI_IF */
    McpHciSeqCmd            commandsSequence[ HCI_SEQ_MAX_CMDS_PER_SEQUENCE ]; /* the commands in the sequence */
    McpU32                  uCommandCount;          /* the number of commands in the above sequence */
    McpU32                  uCurrentCommandIdx;     /* the index of the current command executing */
    McpU32                  uSequenceId;             /* the currently running sequence ID */
    McpU32                  uSeqenceIdOfSentCommand; /* and its mirror for currently sent command */
    McpHalCoreId            coreId;
    McpBool                 bCallCBOnlyForLastCmd;
    McpBool                 bPendingCommand;
} MCP_HciSeq_Context;

/********************************************************************************
 *
 * Function prototypes
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_HciSeq_CreateSequence()
 *
 * Brief:  
 *      Creates an HCI sequence object
 *
 * Description:
 *      This function prepares the context object it receives for use.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pContext [in] - Caller allocated context that stores sequence 
 *                      information
 *      hciIfObj [in] - BT HCI IF object to use as BT tarnsport
 *      coreId   [in] - the core using the sequencer (designates which 
 *                      transport will be used)
 *
 * Returns:
 *      N/A
 */
void MCP_HciSeq_CreateSequence (MCP_HciSeq_Context *pContext, 
                                BtHciIfObj *hciIfObj, 
                                McpHalCoreId coreId);

/*-------------------------------------------------------------------------------
 * MCP_HciSeq_DestroySequence()
 *
 * Brief:  
 *      Destroys an HCI sequence object
 *
 * Description:
 *      This function clean up a HCI sequence object
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pContext [in] - Caller allocated context that stores sequence information
 *
 * Returns:
 *      N/A
 */
void MCP_HciSeq_DestroySequence (MCP_HciSeq_Context *pContext);

/*-------------------------------------------------------------------------------
 * MCP_HciSeq_RunSequence()
 *
 * Brief:  
 *      Runs a sequence of HCI commands
 *
 * Description:
 *      This function is called to execute a sequence of HCI commands. The caller 
 *      must have the context memory valid until execution completes.
 *      The callbacks specified in the command sequence (if any) will be called
 *      respectively for the command they were specified for. A callback must be
 *      specified for the last command, to indicate sequence complete.
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      pContext [in] - Caller allocated context that stores sequence information
 *      uCommandCount [in] - Number of commands in the sequence
 *      pCommands [in] - actual commands in the sequence
 *      bCallCbOnlyAfterLastCmd [in] - if true the callback will be called only at the end of the seq
 *
 * Returns:
 *      BT_HCI_IF_STATUS_FAILED - TODO ronenk: expand this
 *      BT_HCI_IF_STATUS_PENDING
 */
BtHciIfStatus MCP_HciSeq_RunSequence (MCP_HciSeq_Context *pContext, 
                                const McpU32 uCommandCount,
                                const McpHciSeqCmd *pCommands,
                                McpBool bCallCbOnlyAfterLastCmd);

/*-------------------------------------------------------------------------------
 * MCP_HciSeq_CancelSequence()
 *
 * Brief:  
 *      Cancels a running sequence of HCI commands
 *
 * Description:
 *      This function gracefully cancels a running sequence of HCI commands. 
 *      The caller supplies the original context structure.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pContext [in] - Caller allocated context that stores sequence information
 *
 * Returns:
 *      N/A
 */
void MCP_HciSeq_CancelSequence (MCP_HciSeq_Context *pContext);

#endif /* __MCP_HCI_SEQUENCER__ */

