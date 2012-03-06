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

#include "mcp_hci_sequencer.h"
#include "mcp_hal_memory.h"
#include "mcp_defs.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

/* internal prototypes */
void _MCP_HciSeq_callback (BtHciIfClientEvent* pEvent);
BtHciIfStatus _MCP_HciSeq_SendCommand(MCP_HciSeq_Context *pContext);

/*---------------------------------------------------------------------------
 *            MCP_HciSeq_CreateSequence()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Prepares a HCI sequence to be used
 *
 */
void MCP_HciSeq_CreateSequence (MCP_HciSeq_Context *pContext, 
                                BtHciIfObj *hciIfObj, 
                                McpHalCoreId coreId)
{
    BtHciIfStatus   status;

    MCP_FUNC_START ("MCP_HciSeq_CreateSequence");

    /* nullify variables */
    pContext->uSequenceId = 0;
    pContext->uCommandCount = 0;
    pContext->uCurrentCommandIdx = 0;
    pContext->bPendingCommand = MCP_FALSE;
    /* register a HCI IF client */
    pContext->coreId = coreId;
    if(pContext->coreId == MCP_HAL_CORE_ID_BT)
    {
            status = BT_HCI_IF_RegisterClient(hciIfObj, _MCP_HciSeq_callback, &(pContext->handle));
        MCP_VERIFY_FATAL_NO_RETVAR ((BT_HCI_IF_STATUS_SUCCESS == status),
                                    ("MCP_HciSeq_CreateSequence: BT_HCI_IF_RegisterClient returned status %d",
                                     status));
    }

    MCP_FUNC_END ();
}

/*---------------------------------------------------------------------------
 *            MCP_HciSeq_DestroySequence()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  destroys a HCI sequence that is no longer in use
 *
 */
void MCP_HciSeq_DestroySequence (MCP_HciSeq_Context *pContext)
{
    BtHciIfStatus   status;

    MCP_FUNC_START ("MCP_HciSeq_DestroySequence");

    /* de-register HCI IF client */
    status = BT_HCI_IF_DeregisterClient (&(pContext->handle));
    MCP_VERIFY_FATAL_NO_RETVAR ((BT_HCI_IF_STATUS_SUCCESS == status),
                                ("MCP_HciSeq_DestroySequence: BT_HCI_IF_DeregisterClient returned status %d",
                                 status));

    MCP_FUNC_END ();
}

/*---------------------------------------------------------------------------
 *            MCP_HciSeq_RunSequence()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Starts execution of an HCI sequence
 *
 */
 /*FM-VAC*/
BtHciIfStatus MCP_HciSeq_RunSequence (MCP_HciSeq_Context *pContext, 
                                      const McpU32 uCommandCount,
                                      const McpHciSeqCmd *pCommands,
                                      McpBool bCallCbOnlyAfterLastCmd)
{
    /* sanity cehck */
    if (HCI_SEQ_MAX_CMDS_PER_SEQUENCE < uCommandCount)
    {
        return BT_HCI_IF_STATUS_FAILED;
    }
    pContext->bCallCBOnlyForLastCmd = bCallCbOnlyAfterLastCmd;

    /* copy commands */
    MCP_HAL_MEMORY_MemCopy ((McpU8 *)pContext->commandsSequence, 
                            (McpU8 *)pCommands,
                            uCommandCount*sizeof(McpHciSeqCmd));

    /* initialize new sequence */
    pContext->uCommandCount = uCommandCount;
    pContext->uCurrentCommandIdx = 0;
    pContext->uSequenceId++;

    /* if a command is pending */
    if (MCP_TRUE == pContext->bPendingCommand)
    {
        return BT_HCI_IF_STATUS_PENDING;
    }
    else
    {
        /* prepare first command (by calling the CB) */
        pContext->commandsSequence[ pContext->uCurrentCommandIdx ].fCommandPrepCB( 
            &(pContext->command),
            pContext->commandsSequence[ pContext->uCurrentCommandIdx ].pUserData );

        /* execute first command */
        return(_MCP_HciSeq_SendCommand(pContext));
    }
}

/*---------------------------------------------------------------------------
 *            MCP_HciSeq_CancelSequence()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Stops execution of a HCI sequence
 *
 */
void MCP_HciSeq_CancelSequence (MCP_HciSeq_Context *pContext)
{
    /* if a sequence is running */
    if (0 < pContext->uCommandCount)
    {
        /* change sequence ID - to avoid calling the CB */
        pContext->uSequenceId++;

        /* nullify command count and current command index */
        pContext->uCommandCount = 0;
        pContext->uCurrentCommandIdx = 0;
    }
}

/*---------------------------------------------------------------------------
 *            _MCP_HciSeq_callback()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Internal callback used to trigger next command in the sequence
 *
 */
 /*FM-VAC*/
void _MCP_HciSeq_callback (BtHciIfClientEvent* pEvent)
{
    MCP_HciSeq_Context   *pContext = (MCP_HciSeq_Context*)pEvent->pUserData;
    BtHciIfStatus        tStatus = BT_HCI_IF_STATUS_INTERNAL_ERROR;
    McpU32               uSequenceId;

    /* Clear flag of pending command */
    pContext->bPendingCommand = MCP_FALSE;
    
    /* CB is not called if previous sequence was canceled or a new sequence was run! */
    if (pContext->uSeqenceIdOfSentCommand == pContext->uSequenceId)
    {
        /* cancel was not requested - advance to next command */
        pContext->uCurrentCommandIdx++;
        
        /* if original CB exists */
        if (NULL != pContext->command.callback) 
        {
            if((!pContext->bCallCBOnlyForLastCmd)||
                ((pContext->bCallCBOnlyForLastCmd)&&
                    (pContext->uCurrentCommandIdx >= pContext->uCommandCount)))
            {
                /* set the user data in the completion event structure */
                pEvent->pUserData = pContext->command.pUserData;
        
                /* if the sequence if finished, reset counters */
                if (pContext->uCurrentCommandIdx == pContext->uCommandCount)
                {
                    pContext->uCommandCount = 0;
                    pContext->uCurrentCommandIdx = 0;
                }

                /* save current sequence ID */
                uSequenceId = pContext->uSequenceId;

                /* call original CB */
                pContext->command.callback (pEvent);

                /* do not continue processing, if the sequence was cancelled or
                 * a new sequence started in the callback above */
                if (uSequenceId != pContext->uSequenceId)
                {
                    return;
                }
            }
        }   
    }

    /* now check if more commands are available for processing */
    if (pContext->uCurrentCommandIdx < pContext->uCommandCount)
    {
        /* prepare next command (by calling the CB) */
        pContext->commandsSequence[ pContext->uCurrentCommandIdx ].fCommandPrepCB( 
            &(pContext->command), pContext->commandsSequence[ pContext->uCurrentCommandIdx ].pUserData );

        /* execute first command */
        tStatus = _MCP_HciSeq_SendCommand(pContext);

        if (BT_HCI_IF_STATUS_PENDING != tStatus)
        {
            /* an error has occurred - set the user data in the completion event structure */
            pEvent->pUserData = pContext->command.pUserData;

            /* and call the CB */
            pContext->command.callback (pEvent);
        }
    }
}

BtHciIfStatus _MCP_HciSeq_SendCommand(MCP_HciSeq_Context *pContext)
{
    BtHciIfStatus status = BT_HCI_IF_STATUS_INTERNAL_ERROR;

    /* save current sequence ID for the command to be sent */
    pContext->uSeqenceIdOfSentCommand = pContext->uSequenceId;
    
    /* set flag of pending command */
    pContext->bPendingCommand = MCP_TRUE;
    
    /* execute the command */
    if(MCP_HAL_CORE_ID_BT == pContext->coreId)
    {
        status = BT_HCI_IF_SendHciCommand(pContext->handle,  
                                          pContext->command.eHciOpcode, 
                                          pContext->command.pHciCmdParms, 
                                          pContext->command.uhciCmdParmsLen,
                                          pContext->command.uCompletionEvent,
                                          (void*)pContext);       
    }
    else if (MCP_HAL_CORE_ID_FM == pContext->coreId)
    {
        status = FM_TRANSPORT_IF_SendFmVacCommand(pContext->command.pHciCmdParms, 
                                                  pContext->command.uhciCmdParmsLen,
                                                  _MCP_HciSeq_callback,
                                                  (void*)pContext);       
    }

    return (status);
}

