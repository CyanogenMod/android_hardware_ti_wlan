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
*   FILE NAME:      mcp_load_manager.c
*
*   BRIEF:          This file defines the API of the MCP Load Manager
*
*   DESCRIPTION:    General
*                   This file defines the API of the MCP Load Manager
*
*   AUTHOR:         Malovany Ram
*
\*******************************************************************************/

#ifndef __MCP_LOAD_MANAGER_H__
#define __MCP_LOAD_MANAGER_H__

/*******************************************************************************
 *
 * Include files
 *
 ******************************************************************************/
#include "mcp_hal_defs.h"
#include "mcp_bts_script_processor.h"
#include "bt_hci_if.h"

/*******************************************************************************
 *
 * Types
 *
 ******************************************************************************/
/*Load Manager call back fuction*/
typedef void (*TLoadMngrCB)(McpBtsSpStatus status, void *pUserData);


/*******************************************************************************
 *
 * Data Structures
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * Function declarations
 *
 ******************************************************************************/
/*------------------------------------------------------------------------------
 * MCP_LoadMngr_Init()
 *
 * Brief:  
 *      Initialize Load Manager structs.
 *
 * Description:
 *      Initialize Load Manager structs.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      N/A
 */
void MCP_LoadMngr_Init(void);

/*------------------------------------------------------------------------------
 * MCP_LoadMngr_Deinit()
 *
 * Brief:  
 *      Deinitialize Load Manager structs.
 *
 * Description:
 *      Deinitialize Load Manager structs.
 *
 * Type:
 *      N/A
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      N/A
 */
void MCP_LoadMngr_Deinit(void);
/*------------------------------------------------------------------------------
 * MCP_MCP_LoadMngr_Create()
 *
 * Brief:  
 *      Create the load manager state machine and register the BT_IF client.
 *
 * Description:
 *      Create the load manager state machine and register the BT_IF client.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      hciIfObj [in] - pointer to HCI interface object.
 *
 * Returns:
 *      N/A
 */
void MCP_LoadMngr_Create(BtHciIfObj *hciIfObj);

/*-------------------------------------------------------------------------------
 * MCP_LoadMngr_Destroy()
 *
 * Brief:  
 *      Deregister the BT_IF client.
 *
 * Description:
 *      Deregister the BT_IF client.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      N/A
 *
 *
 * Returns:
 *      N/A
 */
void MCP_LoadMngr_Destroy(void);

/*-------------------------------------------------------------------------------
 * MCP_LoadMngr_SetScript()
 *
 * Brief:  
 *      Set the script that need to be loaded.
 *
 * Description:
 *      Set the script that need to be loaded
 *      This function must be call before MCP_LoadMngr_LoadScript.
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      scriptName [in]     - Script file name (not including path)
 *      scriptLocation [in] - Script location (path)
 *
 *
 * Returns:
 *      MCP_HAL_STATUS_SUCCESS - Operation success.
 *      MCP_HAL_STATUS_FAILED - Operation fail.
 */
McpHalStatus MCP_LoadMngr_SetScriptName (const char *scriptName,
                                         const McpUtf8 *scriptLocation);

/*-------------------------------------------------------------------------------
 * MCP_LoadMngr_LoadScript()
 *
 * Brief:  
 *      Load a script.
 *
 * Description:
 *      Load a script according to MCP_LoadMngr_SetScript function.
 *      This function must be call after calling MCP_LoadMngr_SetScript
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      fCB [in] - Client cllback function. 
 *      pUserData [in] - User data for the corrent command.
 *
 * Returns:
 *
 *      MCP_HAL_STATUS_PENDING - The load script operation was started 
 *          successfully.A MCP_BTS_SP_STATUS_SUCCESS event will be received 
 *          when the operation has been succesfully done.
 *          If the operation  failed McpBtsSpStatus other then
 *          MCP_BTS_SP_STATUS_SUCCESS and MCP_BTS_SP_STATUS_EXECUTION_ABORTED
 *          will be sent.
 *
 *      MCP_BTS_SP_STATUS_SUCCESS - The operation was successfuly
 *
 *      MCP_BTS_SP_STATUS_FAILED - The operation failed.
 *
 */

McpHalStatus MCP_LoadMngr_LoadScript(TLoadMngrCB fCB , void *pUserData);

/*------------------------------------------------------------------------------
 * MCP_LoadMngr_StopLoadScript()
 *
 * Brief:  
 *      Abort the current script Load.
 *
 * Description:
 *      Abort the current script Load.
 *
 * Type:
 *      Synchronous / Asynchronous
 *
 * Parameters:
 *      fCB [in] - Client cllback function.
 *      pUserData [in] - User data for the corrent command.
 *
 * Returns:
 *
 *      MCP_HAL_STATUS_PENDING - The load script operation was started 
 *          successfully.A MCP_BTS_SP_STATUS_EXECUTION_ABORTED event will be
 *          received when the operation has been succesfully done.
 *          If the operation  failed  a diffrrent McpBtsSpStatus will be sent.
 *
 *      MCP_BTS_SP_STATUS_FAILED - There is no Client call back function.
 *
 */
McpHalStatus MCP_LoadMngr_StopLoadScript (TLoadMngrCB fCB,void *pUserData);


/*------------------------------------------------------------------------------
 * MCP_LoadMngr_NotifyUnload()
 *
 * Brief:  
 *      Notify the Load manager that the script is unloaded
 *      du to radio off.
 *
 * Description:
 *      Notify the Load manager that the script is unloaded
 *      du to radio off.
 *
 * Type:
 *      Synchronous 
 *
 * Parameters:
 *      N/A
 *
 * Returns:
 *      N/A
 *
 */
void MCP_LoadMngr_NotifyUnload (void);


#endif /* __MCP_LOAD_MANAGER_H__ */

