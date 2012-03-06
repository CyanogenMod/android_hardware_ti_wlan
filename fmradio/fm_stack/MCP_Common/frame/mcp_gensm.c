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
*   FILE NAME:      mcp_gensm.c
*
*   BRIEF:          This file defines the implementation of the general state machine
*
*   DESCRIPTION:    General
*             		This file defines the implementation of the general state machine
*
*   AUTHOR:            Malovany Ram
*
\*******************************************************************************/


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "mcp_gensm.h"
#include "mcp_defs.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

/********************************************************************************
 *
 * Function definitions
 *
 *******************************************************************************/


void MCP_GenSM_Create (MCP_TGenSM *pGenSM , 
							McpU32 uStateNum, 
							McpU32	uEventNum, 
							MCP_TGenSM_actionCell *pMatrix,
							McpU32 uInitialState,
							Mcp_TEnumTString pEventTString,
							Mcp_TEnumTString pStateTString)
{
    /* set initial values for state machine */
    pGenSM->uStateNum       = uStateNum;
    pGenSM->uEventNum       = uEventNum;
    pGenSM->tMatrix         = pMatrix;
    pGenSM->uCurrentState   = uInitialState;
    pGenSM->bEventPending   = MCP_FALSE;
    pGenSM->bInAction       = MCP_FALSE;
    pGenSM->pEventTString = pEventTString;
    pGenSM->pStateTString= pStateTString;

}


void MCP_GenSM_Destroy (MCP_TGenSM *pGenSM)
{
		/*Currently this function is NULL for future purpose */
		MCP_UNUSED_PARAMETER(pGenSM);
}

void MCP_GenSM_Event (MCP_TGenSM *pGenSM, McpU32 uEvent, void *pData)
{

    McpU32           uCurrentState;
    MCP_TGenSM_actionCell   *pCell;

    /* mark that an event is pending */
    pGenSM->bEventPending = MCP_TRUE;

    /* save event and data */
    pGenSM->uEvent = uEvent;
    pGenSM->pData = pData;

    /* if an event is currently executing, return (new event will be handled when current event is done)*/
    if (MCP_TRUE == pGenSM->bInAction)
    {		       
				MCP_LOG_INFO(("Event is already in process ! delaying execution of event: %s \n",
								pGenSM->pEventTString(pGenSM->uEvent)));
        return;
    }

    /* execute events, until none is pending */
    while (MCP_TRUE == pGenSM->bEventPending)
    {
        /* get the cell pointer for the current state and event */
        pCell = &(pGenSM->tMatrix[ (pGenSM->uCurrentState * pGenSM->uEventNum) + pGenSM->uEvent ]);

	/* print state transition information */
	MCP_LOG_INFO(("Transition from State:%s, Event:%s -->  State : %s\n",
 					pGenSM->pStateTString(pGenSM->uCurrentState),
					pGenSM->pEventTString(pGenSM->uEvent),
					pGenSM->pStateTString(pCell->uNextState)));
        
        /* mark that event execution is in place */
        pGenSM->bInAction = MCP_TRUE;

        /* mark that pending event is being handled */
        pGenSM->bEventPending = MCP_FALSE;
        
        /* keep current state */
        uCurrentState = pGenSM->uCurrentState;
	/*Save previous State - For debug info */
	 pGenSM->uPreviousState= pGenSM->uCurrentState;
        /* update current state */
        pGenSM->uCurrentState = pCell->uNextState;

        /* run transition function */
        (*(pCell->fAction)) (pGenSM->pData);

        /* mark that event execution is complete */
        pGenSM->bInAction = MCP_FALSE;
    }
}


McpU32 MCP_GenSM_GetCurrentState (MCP_TGenSM *pGenSM)
{
     return pGenSM->uCurrentState;
}
