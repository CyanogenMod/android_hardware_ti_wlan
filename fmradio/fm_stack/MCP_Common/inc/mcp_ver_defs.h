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
*   FILE NAME:      mcp_ver_defs.h
*
*   DESCRIPTION:    This file defines common macros that should be used for message logging, 
*					and exception checking, handling and reporting
*
*					In addition, it contains miscellaneous other related definitions.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/


#ifndef __MCP_VER_DEFS_H
#define __MCP_VER_DEFS_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_hal_log.h"
#include "mcp_hal_misc.h"

#if (MCP_HAL_LOG_REPORT_API_FUNCTION_ENTRY_EXIT == MCP_HAL_CONFIG_ENABLED)

#define MCP_LOG_FUNCTION_ENTRY				MCP_LOG_FUNCTION(("Entered %s", mcpDbgFuncName))								
#define MCP_LOG_FUNCTION_EXIT					MCP_LOG_FUNCTION(("Exiting %s", mcpDbgFuncName))											
#define MCP_DEFINE_FUNC_NAME(funcName)	const char* mcpDbgFuncName = funcName

#else

#define MCP_LOG_FUNCTION_ENTRY
#define MCP_DEFINE_FUNC_NAME(funcName)
#define MCP_LOG_FUNCTION_EXIT

#endif

#define MCP_FUNC_START(funcName)			\
	MCP_DEFINE_FUNC_NAME(funcName);	\
	MCP_LOG_FUNCTION_ENTRY

#define MCP_FUNC_END()						\
	goto CLEANUP;							\
	CLEANUP:								\
	MCP_LOG_FUNCTION_EXIT

#define MCP_ASSERT(condition)	\
		MCP_HAL_MISC_Assert(#condition, __FILE__, (McpU16)__LINE__)

#define MCP_VERIFY_ERR_NORET(condition, msg)	\
		if ((condition) == 0)						\
		{										\
			MCP_LOG_ERROR(msg);				\
		}
		
#define MCP_VERIFY_FATAL_NORET(condition, msg)		\
		if ((condition) == 0)							\
		{											\
			MCP_LOG_FATAL(msg);					\
			MCP_ASSERT(condition);					\
		}

#define MCP_VERIFY_ERR_NO_RETVAR(condition, msg)	\
		if ((condition) == 0)						\
		{										\
			MCP_LOG_ERROR(msg);				\
			goto CLEANUP;						\
		}
		
#define MCP_VERIFY_FATAL_NO_RETVAR(condition, msg)		\
		if ((condition) == 0)							\
		{											\
			MCP_LOG_FATAL(msg);					\
			MCP_ASSERT(condition);					\
			goto CLEANUP;							\
		}

#define MCP_ERR_NORET(msg)							\
			MCP_LOG_ERROR(msg);
		
#define MCP_FATAL_NORET(msg)						\
			MCP_LOG_FATAL(msg);					\
			MCP_ASSERT(0);

#define MCP_RET_SET_RETVAR(setRetVarExp)			\
	(setRetVarExp);									\
	goto CLEANUP
	
#define MCP_VERIFY_ERR_SET_RETVAR(condition, setRetVarExp, msg)		\
		if ((condition) == 0)											\
		{															\
			MCP_LOG_ERROR(msg);									\
			(setRetVarExp);											\
			goto CLEANUP;											\
		}			

#define MCP_VERIFY_FATAL_SET_RETVAR(condition, setRetVarExp, msg)	\
		if ((condition) == 0)						\
		{										\
			MCP_LOG_FATAL(msg);				\
			(setRetVarExp);						\
			MCP_ASSERT(condition);				\
			goto CLEANUP;						\
		}

#define MCP_ERR_SET_RETVAR(setRetVarExp, msg)		\
		MCP_LOG_ERROR(msg);						\
		(setRetVarExp);								\
		goto CLEANUP;

#define MCP_FATAL_SET_RETVAR(setRetVarExp, msg)	\
			MCP_LOG_FATAL(msg);					\
			(setRetVarExp);							\
			MCP_ASSERT(0);							\
			goto CLEANUP;


#define MCP_RET(returnCode)								\
		MCP_RET_SET_RETVAR(status = returnCode)

#define MCP_RET_NO_RETVAR()							\
		MCP_RET_SET_RETVAR(status = status)

#define MCP_VERIFY_ERR(condition, returnCode, msg)		\
		MCP_VERIFY_ERR_SET_RETVAR(condition, (status = (returnCode)), msg)

 #define MCP_VERIFY_FATAL(condition, returnCode, msg)	\
		MCP_VERIFY_FATAL_SET_RETVAR(condition, (status = (returnCode)),  msg)

#define MCP_ERR(returnCode, msg)		\
		MCP_ERR_SET_RETVAR((status = (returnCode)), msg)

 #define MCP_FATAL(returnCode, msg)	\
		MCP_FATAL_SET_RETVAR((status = (returnCode)),  msg)

#define MCP_ERR_NO_RETVAR(msg)					\
			MCP_LOG_ERROR(msg);				\
			goto CLEANUP;

#define MCP_FATAL_NO_RETVAR(msg)				\
			MCP_LOG_FATAL(msg);				\
			MCP_ASSERT(0);						\
			goto CLEANUP;

#endif /* __MCP_VER_DEFS_H */


