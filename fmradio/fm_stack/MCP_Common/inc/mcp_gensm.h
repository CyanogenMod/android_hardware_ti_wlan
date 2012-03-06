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
*   BRIEF:          This file defines the API of the general state machine
*
*   DESCRIPTION:    General
*             		This file defines the API of the general state machine
*
*   AUTHOR:            Malovany Ram
*
\*******************************************************************************/

#ifndef __MCP_GENSM_H__
#define __MCP_GENSM_H__

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "mcp_hal_defs.h"


/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

/* action function type definition */
typedef void (*MCP_TGenSM_action) (void *pData);
typedef const char * (*Mcp_TEnumTString)( McpInt status);

/* State/Event cell */
typedef  struct _MCP_TGenSM_actionCell
{
    McpU32       uNextState; 	/**< next state in transition */
    MCP_TGenSM_action   fAction;    /**< action function */
} MCP_TGenSM_actionCell;

/* 
 * matrix type 
 * Although the state-machine matrix is actually a two-dimensional array, it is treated as a single 
 * dimension array, since the size of each dimeansion is only known in run-time
 */
typedef MCP_TGenSM_actionCell *TGenSM_matrix;


/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/
typedef struct MCP_TGenSM
{
    TGenSM_matrix   tMatrix;       	/**< next state/action matrix */
    McpU32	uStateNum;         		/**< Number of states in the matrix */
    McpU32       uEventNum;       	/**< Number of events in the matrix */
    McpU32       uCurrentState;     /**< Current state */
    McpU32       uPreviousState;     /**< previous state */
    McpU32       uEvent;            /**< Last event sent */
    void            *pData;        	/**< Last event data */
    McpBool	bEventPending;     		/**< Event pending indicator */
    McpBool         bInAction;     	/**< Evenet execution indicator */
    Mcp_TEnumTString         pEventTString;     	/**< Event to String function */
    Mcp_TEnumTString         pStateTString;     	/**< State to String function */

} MCP_TGenSM;


/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/


/*-------------------------------------------------------------------------------
 * MCP_GenSM_Create()
 *
 * Brief:  
 *      Create a SM machine with deafult values.
 *
 * Description:
 *      Create a SM machine with deafult values.
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pGenSM [in] - hanlde to the generic state machine object.
 *      uStateNum [in] - number of states in the state machine.
 *      EventNum [in] - number of events in the state machine.
 *      pMatrix [in] -  pointer to the event/actions matrix.
 *      uInitialState [in] - Initial state.
 *
 * Returns:
 *		N/A
 */

void MCP_GenSM_Create (MCP_TGenSM *pGenSM , 
							McpU32 uStateNum, 
							McpU32	uEventNum, 
							MCP_TGenSM_actionCell *pMatrix,
							McpU32 uInitialState,
							Mcp_TEnumTString pEventTString,
							Mcp_TEnumTString pStateTString);

/*-------------------------------------------------------------------------------
 * MCP_GenSM_Destroy()
 *
 * Brief:  
 *		Currently nothing.
 *
 * Description:
 *		N/A
 *
 * Type:
 *		N/A
 *
 * Parameters:
 *		N/A
 *
 * Returns:
 *		N/A
 */

void MCP_GenSM_Destroy (MCP_TGenSM *pGenSM);


/*-------------------------------------------------------------------------------
 * MCP_GenSM_Event()
 *
 * Brief:  
 *      Enter an event to the state machine.
 *
 * Description:
 *      Enter an event to the state machine.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pGenSM [in] - hanlde to the generic state machine object.
 *      uEvent [in] - event to enter the state machine.
 *      pData [in] - Client data for the current event.
 *
 * Returns:
 *		N/A
 */

void MCP_GenSM_Event (MCP_TGenSM *pGenSM, McpU32 uEvent, void *pData);

/*-------------------------------------------------------------------------------
 * MCP_GenSM_GetCurrentState()
 *
 * Brief:  
 *      Get the current state machine.
 *
 * Description:
 *      Get the current state machine.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pGenSM [in] - hanlde to the generic state machine object.
 *
 * Returns:
 *		N/A
 */
McpU32 MCP_GenSM_GetCurrentState (MCP_TGenSM *pGenSM);

#endif /* __MCP_GENSM_H__ */

