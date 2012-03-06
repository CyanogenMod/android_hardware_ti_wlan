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

#ifndef __MCP_DEFS_H
#define __MCP_DEFS_H

#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_log.h"
#include "mcp_ver_defs.h"

/*-------------------------------------------------------------------------------
 * MCP_STATIC Macro
 * 
 * This macro should be used instead of the 'static' keyword.
 *
 * The reason is that static symbols are usually not part of the debugger's symbol table, 
 * thus preventing source-level debugging of static functions and variables.
 *
 * In addition, they hamper footprint calculations since they do not show up in the map
 * as global symbols do.
 *
 */
#if (MCP_CONFIG_USE_STATIC_KEYWORD == MCP_CONFIG_ENABLED)

#define MCP_STATIC		static

#else

#define MCP_STATIC		/* Nothing */

#endif
		

/*
 *	McpStackType type
 *
 *	A logical identifier of a core stack type
*/
typedef enum tagMcpStackType {
	MCP_STACK_TYPE_BT	= 0,
	MCP_STACK_TYPE_FM_RX,
	MCP_STACK_TYPE_FM_TX,
	MCP_STACK_TYPE_GPS,

	MCP_NUM_OF_STACK_TYPES,
	MCP_INVALID_STACK_TYPE
} McpStackType;

/*---------------------------------------------------------------------------
 *
 * Used to define unused function parameters. Some compilers warn if this
 * is not done.
 */
#define MCP_UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM))

#define MCP_LOG_DEBUG(msg)		MCP_HAL_LOG_DEBUG(__FILE__, __LINE__, mcpHalLogModuleId, msg)
#define MCP_LOG_FUNCTION(msg) 		MCP_HAL_LOG_FUNCTION(__FILE__, __LINE__,mcpHalLogModuleId, msg)
#define MCP_LOG_INFO(msg)			MCP_HAL_LOG_INFO(__FILE__, __LINE__,mcpHalLogModuleId, msg)				
#define MCP_LOG_ERROR(msg)			MCP_HAL_LOG_ERROR(__FILE__, __LINE__, mcpHalLogModuleId, msg)			
#define MCP_LOG_FATAL(msg)			MCP_HAL_LOG_FATAL(__FILE__, __LINE__, mcpHalLogModuleId, msg)	

#endif

