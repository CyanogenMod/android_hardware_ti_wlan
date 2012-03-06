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
*   FILE NAME:      btl_log.h
*
*   DESCRIPTION:    This file defines common macros that should be used for message logging 
*
*   AUTHOR:         Keren Ferdman
*
\*******************************************************************************/

#ifndef __FMC_LOG_H
#define __FMC_LOG_H

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_log.h"
#include "fmc_defs.h"

#undef FMC_LOG_SET_MODULE

#define FMC_LOG_SET_MODULE(x) MCP_HAL_LOG_SET_MODULE((x))


#define FMC_LOG_MODULE_FMCOMMON     MCP_HAL_LOG_MODULE_TYPE_FMCOMMON
#define FMC_LOG_MODULE_FMPOOL       MCP_HAL_LOG_MODULE_TYPE_FMPOOL
#define FMC_LOG_MODULE_FMCORE       MCP_HAL_LOG_MODULE_TYPE_FMCORE
#define FMC_LOG_MODULE_FMOS         MCP_HAL_LOG_MODULE_TYPE_FMOS
#define FMC_LOG_MODULE_FMRX         MCP_HAL_LOG_MODULE_TYPE_FMRX
#define FMC_LOG_MODULE_FMRXSM       MCP_HAL_LOG_MODULE_TYPE_FMRXSM
#define FMC_LOG_MODULE_FMTX         MCP_HAL_LOG_MODULE_TYPE_FMTX
#define FMC_LOG_MODULE_FMTXSM       MCP_HAL_LOG_MODULE_TYPE_FMTXSM
#define FMC_LOG_MODULE_FMUTILS      MCP_HAL_LOG_MODULE_TYPE_FMUTILS



#ifdef FMC_RELEASE
#define FMC_LOG_DEBUG(msg)
#define FMC_LOG_FUNCTION(msg) 	
#else
#define FMC_LOG_DEBUG(msg)		MCP_HAL_LOG_DEBUG(__FILE__, __LINE__, mcpHalLogModuleId, msg)
#define FMC_LOG_FUNCTION(msg) 		MCP_HAL_LOG_FUNCTION(__FILE__, __LINE__, mcpHalLogModuleId, msg)

#endif /* EBTIPS_RELEASE */
#define FMC_LOG_INFO(msg)			MCP_HAL_LOG_INFO(__FILE__, __LINE__, mcpHalLogModuleId, msg)			
#define FMC_LOG_ERROR(msg)			MCP_HAL_LOG_ERROR(__FILE__, __LINE__, mcpHalLogModuleId, msg)		
#define FMC_LOG_FATAL(msg)			MCP_HAL_LOG_FATAL(__FILE__, __LINE__, mcpHalLogModuleId, msg) 
#endif /* __FMC_LOG_H */


