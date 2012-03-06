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
*   FILE NAME:      mcp_hal_log.h
*
*   BRIEF:          This file defines the API of the MCP HAL log MACROs.
*
*   DESCRIPTION:  The MCP HAL LOG API implements platform dependent logging
*                       MACROs which should be used for logging messages by 
*                   different layers, such as the transport, stack, BTL and BTHAL.
*                   The following 5 Macros mast be implemented according to the platform:
*                   MCP_HAL_LOG_DEBUG
*                   MCP_HAL_LOG_INFO
*                   MCP_HAL_LOG_ERROR
*                   MCP_HAL_LOG_FATAL
*
*   AUTHOR:         Udi Ron 
*
\*******************************************************************************/
#ifndef __MCP_HAL_LOG_H
#define __MCP_HAL_LOG_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"

/*---------------------------------------------------------------------------
 * McpHalLogSeverity type
 *
 *     Represents possible types of log messages severity
 */
typedef McpU8 McpHalLogSeverity;

#define MCP_HAL_LOG_SEVERITY_FUNCTION       ((McpU8)10)
#define MCP_HAL_LOG_SEVERITY_DEBUG          ((McpU8)9)
#define MCP_HAL_LOG_SEVERITY_INFO           ((McpU8)8)
#define MCP_HAL_LOG_SEVERITY_ERROR          ((McpU8)7)
#define MCP_HAL_LOG_SEVERITY_FATAL          ((McpU8)6)

/*
 *   Macro for use with the McpHalLogLevelMask array entry for each
 *   Module in the system
 */
#define MCP_HAL_LOG_GET_SEVERITY_FLAG(x)    (1<<((x)-MCP_HAL_LOG_SEVERITY_FATAL))

/*
 *  Default mask allowing all messages to be printed
 */
#define MCP_HAL_LOG_DEFALT_DEBUG_LEVEL_MASK (MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_FUNCTION) |\
                                            MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_DEBUG) |\
                                            MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_INFO) |\
                                            MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_ERROR) |\
                                            MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_FATAL))

/*
 *  Invalid logging mask
 */
#define MCP_HAL_LOG_INVALID_DEBUG_MASK      0xFF

/*
 *  Size of the module mask array
 */
#define MCP_HAL_LOG_MASK_ARRAY_SIZE         (MCP_HAL_LOG_MODULE_TYPE_LAST)

/*
 *  Maximum size of module name
 */
#define MAX_MODULE_NAME_LENGTH 25


/*
 *  Module ID's
 */
typedef enum {
    /* Stack and HAL's */
    MCP_HAL_LOG_MODULE_TYPE_UNKNOWN = 0,
    MCP_HAL_LOG_MODULE_TYPE_BMG ,
    MCP_HAL_LOG_MODULE_TYPE_SPP ,
    MCP_HAL_LOG_MODULE_TYPE_OPPC    ,
    MCP_HAL_LOG_MODULE_TYPE_OPPS    ,
    MCP_HAL_LOG_MODULE_TYPE_BPPSND,
    MCP_HAL_LOG_MODULE_TYPE_PBAPS   ,
    MCP_HAL_LOG_MODULE_TYPE_PAN ,
    MCP_HAL_LOG_MODULE_TYPE_AVRCPTG,
    MCP_HAL_LOG_MODULE_TYPE_FTPS    ,
    MCP_HAL_LOG_MODULE_TYPE_FTPC    ,
    MCP_HAL_LOG_MODULE_TYPE_VG  ,
    MCP_HAL_LOG_MODULE_TYPE_AG  ,
    MCP_HAL_LOG_MODULE_TYPE_RFCOMM,
    MCP_HAL_LOG_MODULE_TYPE_A2DP    ,
    MCP_HAL_LOG_MODULE_TYPE_HID ,
    MCP_HAL_LOG_MODULE_TYPE_MDG ,
    MCP_HAL_LOG_MODULE_TYPE_BIPINT,
    MCP_HAL_LOG_MODULE_TYPE_BIPRSP,
    MCP_HAL_LOG_MODULE_TYPE_SAPS    ,
    MCP_HAL_LOG_MODULE_TYPE_COMMON,
    MCP_HAL_LOG_MODULE_TYPE_L2CAP   ,
    MCP_HAL_LOG_MODULE_TYPE_OPP ,
    MCP_HAL_LOG_MODULE_TYPE_FTP ,
    MCP_HAL_LOG_MODULE_TYPE_PM  ,
    MCP_HAL_LOG_MODULE_TYPE_BTDRV   ,
    MCP_HAL_LOG_MODULE_TYPE_UNICODE,
    MCP_HAL_LOG_MODULE_TYPE_BSC ,
    MCP_HAL_LOG_MODULE_TYPE_BTSTACK,
    MCP_HAL_LOG_MODULE_TYPE_FMCOMMON,
    MCP_HAL_LOG_MODULE_TYPE_FMPOOL,
    MCP_HAL_LOG_MODULE_TYPE_FMCORE,
    MCP_HAL_LOG_MODULE_TYPE_FMOS,
    MCP_HAL_LOG_MODULE_TYPE_FMUTILS,
    MCP_HAL_LOG_MODULE_TYPE_FMRX,
    MCP_HAL_LOG_MODULE_TYPE_FMRXSM,
    MCP_HAL_LOG_MODULE_TYPE_FMTX,
    MCP_HAL_LOG_MODULE_TYPE_FMTXSM,
    MCP_HAL_LOG_MODULE_TYPE_CCM,
    MCP_HAL_LOG_MODULE_TYPE_CCM_IM,
    MCP_HAL_LOG_MODULE_TYPE_CCM_VAC,
    MCP_HAL_LOG_MODULE_TYPE_CCM_CAL,
    MCP_HAL_LOG_MODULE_TYPE_FRAME,
    MCP_HAL_LOG_MODULE_TYPE_FS  ,
    MCP_HAL_LOG_MODULE_TYPE_MODEM   ,
    MCP_HAL_LOG_MODULE_TYPE_OS  ,
    MCP_HAL_LOG_MODULE_TYPE_SCRIPT_PROCESSOR,
    MCP_HAL_LOG_MODULE_TYPE_ASSERT,
    MCP_HAL_LOG_MODULE_TYPE_HAL_FS,
    MCP_HAL_LOG_MODULE_TYPE_BTHAL_MM,

    /* BTBUS, Client and server */
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_MAIN_EVENTS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_MAIN,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_TIMEOUTS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_UTIL,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_MAIN,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_A2DP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_AG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_AVRCP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_BMG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_COMMON,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_FMRX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_FMTX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_FTPC,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_FTPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_HALMM,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_HALPB,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_HIDH,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_MDG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_OPPC,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_OPPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_PBAPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_XXX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_CLIENT_SPP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_MAIN,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_A2DP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_AG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_AVRCP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_BMG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_COMMON,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_FMRX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_FMTX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_FTPC,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_FTPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_HALMM,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_HALPB,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_HIDH,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_MDG,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_OPPC,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_OPPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_PBAPS,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_XXX,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_SERVER_SPP,
    MCP_HAL_LOG_MODULE_TYPE_BTBUS_UTILS,
   
    /* Predefined user placeholders */
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_001,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_002,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_003,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_004,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_005,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_006,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_007,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_008,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_009,
    MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_010,
    MCP_HAL_LOG_MODULE_TYPE_LAST = MCP_HAL_LOG_MODULE_TYPE_USER_DEFINED_010
} McpHalLogModuleId_e;

/*
 *
 */
typedef McpU8 McpHalLogLevelMask ;

/*
 *  
 */
typedef struct _McpHalLogModule_t
{
    McpHalLogLevelMask logLevelMask;
    char name[MAX_MODULE_NAME_LENGTH];
}McpHalLogModule_t;

/*
 *
 */
const char* MCP_HAL_LOG_SetModuleName(McpHalLogModuleId_e moduleId,const char* name);

/*
 *
 */
const char* MCP_HAL_LOG_GetModuleName(McpHalLogModuleId_e moduleId);

/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_SET_MODULE
 *
 *      Basic module declaration macro
 */
#define MCP_HAL_LOG_SET_MODULE(moduleId) static const McpHalLogModuleId_e mcpHalLogModuleId = (McpHalLogModuleId_e)(moduleId)




#define MCP_HAL_LOG_SET_MODULE_LOGLEVEL(moduleId,level) MCP_HAL_LOG_Modules[moduleId].logLevelMask=level;

#define MCP_HAL_LOG_GET_MODULE_LOGLEVEL(moduleId) MCP_HAL_LOG_Modules[moduleId].logLevelMask;

/*
 *  Module Array
 */
extern McpHalLogModule_t MCP_HAL_LOG_Modules[];

/*-------------------------------------------------------------------------------
 * Platform dependent functions
 */
char *MCP_HAL_LOG_FormatMsg(const char *format, ...);

void MCP_HAL_LOG_LogMsg(const char*     fileName, 
                            McpU32          line, 
                            McpHalLogModuleId_e moduleId, 
                            McpHalLogSeverity   severity,
                            const char      *msg);

void MCP_HAL_LOG_Init(void);

void MCP_HAL_LOG_Deinit(void);

void MCP_HAL_LOG_SetThreadName(const char* name);

#ifdef EBTIPS_RELEASE
#define GENERAL_RELEASE
#endif

#ifdef FMC_RELEASE
#define GENERAL_RELEASE
#endif


#ifdef GENERAL_RELEASE
#define MCP_HAL_LOG_REPORT_API_FUNCTION_ENTRY_EXIT                      (MCP_HAL_CONFIG_DISABLED)
#else
#define MCP_HAL_LOG_REPORT_API_FUNCTION_ENTRY_EXIT                      (MCP_HAL_CONFIG_ENABLED)
#endif

#define MCP_HAL_LOG_FORMAT_MSG(msg) MCP_HAL_LOG_FormatMsg msg


/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_FUNCTION
 *
 *      Defines trace message in function level, meaning it is used when entering
 *      and exiting a function.
 *      It should not be used in release build.
 *      This MACRO is not used when EBTIPS_RELEASE or FMC_RELEASE is enabled.
 */
#define MCP_HAL_LOG_FUNCTION(file, line, moduleId, msg)\
    ( (MCP_HAL_LOG_Modules[moduleId].logLevelMask & MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_FUNCTION))\
    ? ((void)MCP_HAL_LOG_LogMsg( file,line,moduleId,MCP_HAL_LOG_SEVERITY_FUNCTION,MCP_HAL_LOG_FORMAT_MSG(msg)))\
    : (void)0 )

/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_DEBUG
 *
 *      Defines trace message in debug level, which should not be used in release build.
 *      This MACRO is not used when EBTIPS_RELEASE or FMC_RELEASE is enabled.
 */
#define MCP_HAL_LOG_DEBUG(file, line, moduleId, msg)\
    ( (MCP_HAL_LOG_Modules[moduleId].logLevelMask & MCP_HAL_LOG_GET_SEVERITY_FLAG(MCP_HAL_LOG_SEVERITY_DEBUG))\
    ? ((void)MCP_HAL_LOG_LogMsg( file,line,moduleId,MCP_HAL_LOG_SEVERITY_DEBUG,MCP_HAL_LOG_FORMAT_MSG(msg)))\
    : (void)0 )

/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_INFO
 *
 *      Defines trace message in info level.
 */
#define MCP_HAL_LOG_INFO(file, line, moduleId, msg) ((void)MCP_HAL_LOG_LogMsg( file,line,moduleId,MCP_HAL_LOG_SEVERITY_INFO,MCP_HAL_LOG_FORMAT_MSG(msg)))

/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_ERROR
 *
 *      Defines trace message in info level.
 */
#define MCP_HAL_LOG_ERROR(file, line, moduleId, msg) ((void)MCP_HAL_LOG_LogMsg( file,line,moduleId,MCP_HAL_LOG_SEVERITY_ERROR,MCP_HAL_LOG_FORMAT_MSG(msg)))

/*-------------------------------------------------------------------------------
 * MCP_HAL_LOG_FATAL
 *
 *      Defines trace message in fatal level.
 */
#define MCP_HAL_LOG_FATAL(file, line, moduleId, msg) ((void)MCP_HAL_LOG_LogMsg( file,line,moduleId,MCP_HAL_LOG_SEVERITY_FATAL,MCP_HAL_LOG_FORMAT_MSG(msg)))
    
    
#endif /* __MCP_HAL_LOG_H */


