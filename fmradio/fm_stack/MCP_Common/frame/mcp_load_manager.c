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
*   BRIEF:          This file implements the load manager API and SM
*
*   DESCRIPTION:    This file implements the load manager API and SM
*
*   AUTHOR:         Malovany Ram
*
\*******************************************************************************/

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_gensm.h"
#include "mcp_hal_string.h"
#include "mcp_load_manager.h"
#include "mcp_ver_defs.h"
#include "mcp_defs.h"
#include "mcp_bts_script_processor.h"
#include "mcp_rom_scripts_db.h"
#include "mcp_unicode.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

/********************************************************************************
 *
 * Constants 
 *
 *******************************************************************************/

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

/*States in the State Machine */
typedef enum _EMCP_LM_STATE
{
    MCP_LM_STATE_UNLOADED = 0,    
    MCP_LM_STATE_LOADING,
    MCP_LM_STATE_ABORTING,    
    MCP_LM_STATE_LOADED,
    MCP_LM_STATE_MAX_NUM,
    MCP_LM_STATE_INVALID
} EMCP_LM_STATE;

/*Events to the State Machine */
typedef enum _EMCP_LM_EVENT
{
    MCP_LM_EVENT_LOAD=0,
    MCP_LM_EVENT_ABORT,
    MCP_LM_EVENT_ABORT_DONE,
    MCP_LM_EVENT_UNLOAD_DONE,
    MCP_LM_EVENT_LOAD_DONE,
    MCP_LM_EVENT_LOAD_FAIL,
    MCP_LM_EVENT_MAX_NUM,
    MCP_LM_EVENT_INVALID
} EMCP_LM_EVENT;

/********************************************************************************
 *
 * Internal Functions
 *
 *******************************************************************************/
MCP_STATIC void MCP_LM_Nothing(void *pUserData);
MCP_STATIC void MCP_LM_Load(void *pUserData);
MCP_STATIC void MCP_LM_Load_Pending(void *pUserData);
MCP_STATIC void MCP_LM_Load_Success(void *pUserData);
MCP_STATIC void MCP_LM_Abort(void *pUserData);
MCP_STATIC void MCP_LM_ActionUnexpected(void *pUserData);
MCP_STATIC const char *MCP_LM_StateToString(EMCP_LM_STATE eState);
MCP_STATIC const char *MCP_LM_EventToString(EMCP_LM_EVENT eEvent);

/********************************************************************************
 *
 * Functions
 *
 *******************************************************************************/
void MCP_LoadMngr_Hci_Cmds_callback(BtHciIfClientEvent *event);
void MCP_LM_ExcutionCompleteCB(McpBtsSpContext *context, McpBtsSpStatus status);

McpBtsSpStatus MCP_LM_SendHciCmdCB(   McpBtsSpContext         *context,
                                      McpU16              hciOpcode, 
                                      McpU8               *hciCmdParms, 
                                      McpUint                 hciCmdParmsLen);

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/

/*Static init for the Action cell acorrding to the state machine */
MCP_STATIC MCP_TGenSM_actionCell tSmMatrix[ MCP_LM_STATE_MAX_NUM ][ MCP_LM_EVENT_MAX_NUM ] = 
{
    { /* MCP_LM_STATE_UNLOADED */
        { MCP_LM_STATE_LOADING, MCP_LM_Load},                  /* MCP_LM_EVENT_LOAD */
        { MCP_LM_STATE_UNLOADED, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_ABORT */
        { MCP_LM_STATE_UNLOADED, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_ABORT_DONE */
        { MCP_LM_STATE_UNLOADED, MCP_LM_Nothing},              /* MCP_LM_EVENT_UNLOAD_DONE */ 
        { MCP_LM_STATE_UNLOADED, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_LOAD_DONE */
        { MCP_LM_STATE_UNLOADED, MCP_LM_ActionUnexpected}      /* MCP_LM_EVENT_LOAD_FAIL */        
    },
    { /* MCP_LM_STATE_LOADING */
        { MCP_LM_STATE_LOADING, MCP_LM_Load_Pending},          /* MCP_LM_EVENT_LOAD */
        { MCP_LM_STATE_ABORTING, MCP_LM_Abort},                /* MCP_LM_EVENT_ABORT */
        { MCP_LM_STATE_LOADING, MCP_LM_ActionUnexpected},      /* MCP_LM_EVENT_ABORT_DONE */
        { MCP_LM_STATE_LOADING, MCP_LM_ActionUnexpected},      /* MCP_LM_EVENT_UNLOAD_DONE */ 
        { MCP_LM_STATE_LOADED, MCP_LM_Nothing},                /* MCP_LM_EVENT_LOAD_DONE */
        { MCP_LM_STATE_UNLOADED, MCP_LM_Nothing}               /* MCP_LM_EVENT_LOAD_FAIL */
    },
    { /* MCP_LM_STATE_ABORTING */
        { MCP_LM_STATE_ABORTING, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_LOAD */
        { MCP_LM_STATE_ABORTING, MCP_LM_Nothing},              /* MCP_LM_EVENT_ABORT */
        { MCP_LM_STATE_UNLOADED, MCP_LM_Nothing},              /* MCP_LM_EVENT_ABORT_DONE */
        { MCP_LM_STATE_ABORTING, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_UNLOAD_DONE */
        { MCP_LM_STATE_ABORTING, MCP_LM_ActionUnexpected},     /* MCP_LM_EVENT_LOAD_DONE */
        { MCP_LM_STATE_ABORTING, MCP_LM_ActionUnexpected}      /* MCP_LM_EVENT_LOAD_FAIL */
    },
    { /* MCP_LM_STATE_LOADED */
        { MCP_LM_STATE_LOADED, MCP_LM_Load_Success},           /* MCP_LM_EVENT_LOAD */
        { MCP_LM_STATE_LOADED, MCP_LM_ActionUnexpected},       /* MCP_LM_EVENT_ABORT */
        { MCP_LM_STATE_LOADED, MCP_LM_ActionUnexpected},       /* MCP_LM_EVENT_ABORT_DONE */
        { MCP_LM_STATE_UNLOADED, MCP_LM_Nothing},              /* MCP_LM_EVENT_UNLOAD_DONE */
        { MCP_LM_STATE_LOADED, MCP_LM_ActionUnexpected},       /* MCP_LM_EVENT_LOAD_DONE */
        { MCP_LM_STATE_LOADED, MCP_LM_ActionUnexpected}        /* MCP_LM_EVENT_LOAD_FAIL */
    }
};

/********************************************************************************
 *
 * Globals
 *
 *******************************************************************************/

/* Load Status for the state machine */
McpHalStatus iLoadSmstatus;
  
/* Script name and location (directory) */
char *pScriptName;
McpUtf8 *pScriptLocation;

/* Context that stores script processing information */
McpBtsSpContext context;

/* Callbacks for various operations that are needed for script execution */
McpBtsSpExecuteScriptCbData tExcuteCallbacks;

/* General state machine data */
MCP_TGenSM tDataGenSm;

/* Handle for every client for the BT_HCI_IF */
BtHciIfClientHandle handle;

/* Client call back function*/   
TLoadMngrCB fCb;

/* Client data for the current command*/
void *pUserData; 

/********************************************************************************
*
* Function definitions
*
*******************************************************************************/


void MCP_LoadMngr_Init(void)
{
    MCP_FUNC_START("MCP_LoadMngr_Init");

    /* Initialize Load State Machine status */
    iLoadSmstatus = MCP_HAL_STATUS_FAILED;

    /* Init the execution complete CB function */
    tExcuteCallbacks.execCompleteCb = MCP_LM_ExcutionCompleteCB;
    tExcuteCallbacks.sendHciCmdCb = MCP_LM_SendHciCmdCB;

    /* Init the HCI cmd and the Params CB functions to NULL */   
    tExcuteCallbacks.setTranParmsCb = NULL;

    MCP_FUNC_END();
}

void MCP_LoadMngr_Deinit(void)
{
    MCP_FUNC_START("MCP_LoadMngr_Deinit");
    
    /* Currently Nothing */  
    MCP_FUNC_END();
}

void MCP_LoadMngr_Create(BtHciIfObj *hciIfObj)
{
    BtHciIfStatus   status;
    MCP_FUNC_START("MCP_LoadMngr_Create");

    /* Create State Machine */
    MCP_GenSM_Create(&(tDataGenSm),
                     MCP_LM_STATE_MAX_NUM,
                     MCP_LM_EVENT_MAX_NUM,
                     &(tSmMatrix[0][0]),
                     MCP_LM_STATE_UNLOADED,
                     (Mcp_TEnumTString)MCP_LM_EventToString,
                     (Mcp_TEnumTString)MCP_LM_StateToString);

    /* Register a HCI IF client */
    status = BT_HCI_IF_RegisterClient(hciIfObj, MCP_LoadMngr_Hci_Cmds_callback, &(handle));
    MCP_VERIFY_FATAL_NO_RETVAR ((BT_HCI_IF_STATUS_SUCCESS == status),
                                ("MCP_LoadMngr_Create: BT_HCI_IF_RegisterClient returned status %d",
                                 status));

    MCP_FUNC_END();
}

void MCP_LoadMngr_Destroy(void)
{
    MCP_FUNC_START("MCP_LoadMngr_Destroy");

    /* Destroy State Machine */
    MCP_GenSM_Destroy(&(tDataGenSm));

    /* De-register IF client*/
    BT_HCI_IF_DeregisterClient(&(handle));

    MCP_FUNC_END();
}


McpHalStatus MCP_LoadMngr_SetScriptName(const char *scriptName,
                                        const McpUtf8 *scriptLocation)
{
    MCP_FUNC_START("MCP_LoadMngr_SetScriptName");

    /* set the script file name and location */
    pScriptName = (char *)scriptName;
    pScriptLocation = (McpUtf8 *)scriptLocation;

    MCP_FUNC_END();

    return MCP_HAL_STATUS_SUCCESS;
}


McpHalStatus MCP_LoadMngr_LoadScript(TLoadMngrCB fCB, void *pUserData)
{
    McpHalStatus    status;

    MCP_FUNC_START("MCP_LoadMngr_LoadScript");

    /* Check for preliminary conditions that there is a valid CB */
    MCP_VERIFY_ERR ((NULL != fCB), MCP_HAL_STATUS_FAILED,
                    ("MCP_LoadMngr_LoadScript: NULL CB!"));

    /* Update CB and user data */
    fCb = fCB;
    pUserData = pUserData;   

    /* Send MCP_LM_EVENT_LOAD event to the SM to start the loading process */
    MCP_GenSM_Event (&tDataGenSm, MCP_LM_EVENT_LOAD, NULL);

	/* 
	 * copy the SM status to 'status', to avoid compilation warning and 
	 * allow VERIFY_ERR above to return correct status
	 */
	status = iLoadSmstatus;

    MCP_FUNC_END();

    return status;
}

McpHalStatus MCP_LoadMngr_StopLoadScript(TLoadMngrCB fCB, void *pUserData)
{
    McpHalStatus    status;

    MCP_FUNC_START("MCP_LoadMngr_StopLoadScript");

    /* check for preliminary conditions that there is a CB */
    MCP_VERIFY_ERR ((NULL != fCB), MCP_HAL_STATUS_FAILED,
                    ("MCP_LoadMngr_LoadScript: NULL CB!"));

    /* Update CB and user data */
    fCb = fCB;
    pUserData = (void*)pUserData;    

    /*Send an MCP_LM_EVENT_ABORT event to the SM*/
    MCP_GenSM_Event (&tDataGenSm, MCP_LM_EVENT_ABORT, NULL);

	/* 
	 * copy the SM status to 'status', to avoid compilation warning and 
	 * allow VERIFY_ERR above to return correct status
	 */
	status = iLoadSmstatus;

    MCP_FUNC_END();

    return status;
}


void MCP_LoadMngr_NotifyUnload ()
{

    MCP_FUNC_START ("MCP_LoadMngr_NotifyUnload");

    /* Send an MCP_LM_EVENT_UNLOAD_DONE event to the SM */
    MCP_GenSM_Event (&tDataGenSm, MCP_LM_EVENT_UNLOAD_DONE, NULL);

    MCP_FUNC_END();
}

MCP_STATIC void MCP_LM_Nothing (void *pUserData)
{   
    MCP_FUNC_START("MCP_LM_Nothing");

    MCP_UNUSED_PARAMETER(pUserData);

    MCP_FUNC_END();
}

MCP_STATIC void MCP_LM_Load (void *pUserData)
{
    McpBtsSpStatus status;
    McpBtsSpScriptLocation tScriptLocation;
    McpBool memInitScriptFound;
    McpUtf8 scriptFullFileName[MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS *
                               MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR];

    MCP_FUNC_START ("MCP_LM_Load");

    MCP_UNUSED_PARAMETER (pUserData);

    /* First try to load the script from FS */
    tScriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_FS;
    MCP_StrCpyUtf8(scriptFullFileName, (const McpUtf8 *)pScriptLocation);
    MCP_StrCatUtf8(scriptFullFileName, (const McpUtf8 *)pScriptName);
    tScriptLocation.locationData.fullFileName = scriptFullFileName;

    /* Execute the requested script*/
    status = MCP_BTS_SP_ExecuteScript (&tScriptLocation, &tExcuteCallbacks, &context);

    if (status == MCP_BTS_SP_STATUS_PENDING)
    {
        /* Update the State Machine status */
        iLoadSmstatus = MCP_HAL_STATUS_PENDING;
    }
    else if (status == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Update the State Machine status */
        iLoadSmstatus = MCP_HAL_STATUS_SUCCESS;    
    }
    else if (MCP_BTS_SP_STATUS_FILE_NOT_FOUND == status)
    {
        /* FS Script Not Found - Use default hard-coded memory script instead */
        tScriptLocation.locationType = MCP_BTS_SP_SCRIPT_LOCATION_MEMORY;

        /* Check if memory has a script for this version and load its parameters if found */
        memInitScriptFound =  MCP_RomScriptsGetMemInitScriptData(
                                    pScriptName, 
                                    &tScriptLocation.locationData.memoryData.size,
                                    (const McpU8**)&tScriptLocation.locationData.memoryData.address);

        /* verify the script was found in memory */
        MCP_VERIFY_FATAL_NO_RETVAR ((memInitScriptFound == MCP_TRUE),
                          ("MCP_LM_Load: can't find script %s on FS and memory", pScriptName));

        /* execute the memory script */
        status = MCP_BTS_SP_ExecuteScript(&tScriptLocation, &tExcuteCallbacks, &context);

        if (status == MCP_BTS_SP_STATUS_PENDING)
        {
            iLoadSmstatus = MCP_HAL_STATUS_PENDING;
        }
        else if (status == MCP_BTS_SP_STATUS_SUCCESS)
        {
            iLoadSmstatus = MCP_HAL_STATUS_SUCCESS;
        }
        else
        {
            iLoadSmstatus = MCP_HAL_STATUS_FAILED;
        }
    }
    else
    {
        /* Update the State Machine status */
        iLoadSmstatus = MCP_HAL_STATUS_FAILED;
    }

    MCP_FUNC_END();
}


MCP_STATIC void MCP_LM_Load_Pending(void *pUserData)
{       
    MCP_FUNC_START("MCP_LM_Load_Pending");

    MCP_UNUSED_PARAMETER (pUserData);

    /* Update Load status return flag */
    iLoadSmstatus = MCP_HAL_STATUS_PENDING;

    MCP_FUNC_END();
}   

MCP_STATIC void MCP_LM_Load_Success(void *pUserData)
{   

    MCP_FUNC_START("MCP_LM_Load_Success");

    MCP_UNUSED_PARAMETER(pUserData);

    iLoadSmstatus=MCP_BTS_SP_STATUS_SUCCESS;

    MCP_FUNC_END();
}   


MCP_STATIC void MCP_LM_Abort(void *pUserData)
{
    McpBtsSpStatus  status;

    MCP_FUNC_START("MCP_LM_Abort");

    MCP_UNUSED_PARAMETER(pUserData);

    /* Abort script execution */
    status = MCP_BTS_SP_AbortScriptExecution(&(context));

    /* Set the return status according to script processor reply */
    switch (status)
    {
        case MCP_BTS_SP_STATUS_SUCCESS:
        case MCP_BTS_SP_STATUS_EXECUTION_ABORTED:
            iLoadSmstatus = MCP_HAL_STATUS_SUCCESS;
            break;

        case MCP_BTS_SP_STATUS_PENDING:
            iLoadSmstatus = MCP_HAL_STATUS_PENDING;
            break;

        default:
            iLoadSmstatus = MCP_HAL_STATUS_FAILED;
            break;
    }

    MCP_FUNC_END();
}

MCP_STATIC void MCP_LM_ActionUnexpected(void *pUserData)
{

    MCP_FUNC_START("MCP_LM_ActionUnexpected");

    MCP_UNUSED_PARAMETER(pUserData);

    MCP_FUNC_END();
}



/*------------------------------------------------------------------------------
 *       Call back function for the script processor to send an HCI command
 *------------------------------------------------------------------------------
 */
McpBtsSpStatus MCP_LM_SendHciCmdCB (McpBtsSpContext *context,
                                    McpU16 hciOpcode, 
                                    McpU8 *hciCmdParms, 
                                    McpUint hciCmdParmsLen)
{
    McpBtsSpStatus          status = MCP_BTS_SP_STATUS_PENDING;
    BtHciIfStatus           btHciIfStatus;

    MCP_FUNC_START("MCP_LM_SendHciCmdCB");

    MCP_UNUSED_PARAMETER(context);

    btHciIfStatus = BT_HCI_IF_SendHciCommand(handle,   
                                              (BtHciIfHciOpcode)hciOpcode, 
                                             hciCmdParms, 
                                             (McpU8)hciCmdParmsLen,                                          
                                             BT_HCI_IF_HCI_EVENT_COMMAND_COMPLETE,
                                             NULL);

    MCP_VERIFY_ERR((btHciIfStatus == BT_HCI_IF_STATUS_PENDING), MCP_BTS_SP_STATUS_FAILED,
                   ("MCP_LM_SendHciCmdCB: BT_HCI_IF_SendHciCommand Failed"));

    MCP_FUNC_END();

    return status;
}

/*------------------------------------------------------------------------------
 *            Callback function for script execution complete
 *------------------------------------------------------------------------------
 */
void MCP_LM_ExcutionCompleteCB(McpBtsSpContext *context, McpBtsSpStatus status)
{
    MCP_FUNC_START("MCP_LM_ExcutionCompleteCB");

    MCP_UNUSED_PARAMETER(context);

    if (status == MCP_BTS_SP_STATUS_EXECUTION_ABORTED)
    {
        /* Abort was completed successfully sent anMCP_LM_EVENT_ABORT_DONE to the SM */
        MCP_GenSM_Event (&tDataGenSm, MCP_LM_EVENT_ABORT_DONE, NULL);
    }

    else if (status == MCP_BTS_SP_STATUS_SUCCESS)
    {
        /* Load script was successfully sent an event to SM that the load is done */
        MCP_GenSM_Event (&tDataGenSm, MCP_LM_EVENT_LOAD_DONE, NULL);
    }
    else
    {
        /* Load script fail send an event to SM */
        MCP_GenSM_Event(&tDataGenSm, MCP_LM_EVENT_LOAD_FAIL, NULL);
    }

    fCb (status, pUserData);

    MCP_FUNC_END();
}

/*------------------------------------------------------------------------------
 *        Call back function for the script processor for every HCI command 
 *------------------------------------------------------------------------------
 */

void MCP_LoadMngr_Hci_Cmds_callback(BtHciIfClientEvent *event)
{

    MCP_FUNC_START("MCP_LoadMngr_Hci_Cmds_callback");

    MCP_UNUSED_PARAMETER (event);

    /* The command is finished, proceed to the next command */
    MCP_BTS_SP_HciCmdCompleted (&context, NULL, NULL);

    MCP_FUNC_END();
}

/*------------------------------------------------------------------------------
 *            MCP_LM_StateToString()
 *------------------------------------------------------------------------------
 *
 * Synopsis:  Convert enum State to string
 *
 */

MCP_STATIC const char *MCP_LM_StateToString(EMCP_LM_STATE eState)
{
    switch (eState)
    {
    case MCP_LM_STATE_UNLOADED:
        return "MCP_LM_STATE_UNLOADED";
    case MCP_LM_STATE_LOADING:
        return "MCP_LM_STATE_LOADING";
    case MCP_LM_STATE_ABORTING:
        return "MCP_LM_STATE_ABORTING";
    case MCP_LM_STATE_LOADED:
        return "MCP_LM_STATE_LOADED";              
    default:
        return "UNKNOWN";
    }   
}

/*------------------------------------------------------------------------------
 *            MCP_LM_EventToString()
 *------------------------------------------------------------------------------
 *
 * Synopsis:  Convert enum Event to string
 *
 */
MCP_STATIC const char *MCP_LM_EventToString(EMCP_LM_EVENT eEvent)
{
    switch (eEvent)
    {
    case MCP_LM_EVENT_LOAD:
        return "MCP_LM_EVENT_LOAD";
    case MCP_LM_EVENT_ABORT:
        return "MCP_LM_EVENT_ABORT";
    case MCP_LM_EVENT_ABORT_DONE:
        return "MCP_LM_EVENT_ABORT_DONE";
    case MCP_LM_EVENT_UNLOAD_DONE:
        return "MCP_LM_EVENT_UNLOAD_DONE";
    case MCP_LM_EVENT_LOAD_DONE:
        return "MCP_LM_EVENT_LOAD_DONE";
    case MCP_LM_EVENT_LOAD_FAIL:
        return "MCP_LM_EVENT_LOAD_FAIL";    
    default:
        return "UNKNOWN";
    }   
}

