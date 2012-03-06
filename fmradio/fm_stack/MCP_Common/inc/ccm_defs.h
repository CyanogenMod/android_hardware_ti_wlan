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

#ifndef _CCM_DEFS_H
#define _CCM_DEFS_H

#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"
#include "mcp_config.h"

typedef enum
{
	CCM_STATUS_SUCCESS			= MCP_HAL_STATUS_SUCCESS ,
	CCM_STATUS_FAILED				= MCP_HAL_STATUS_FAILED,
	CCM_STATUS_PENDING			= MCP_HAL_STATUS_PENDING,
	CCM_STATUS_NO_RESOURCES		= MCP_HAL_STATUS_NO_RESOURCES,
	CCM_STATUS_IN_PROGRESS		= MCP_HAL_STATUS_IN_PROGRESS,
	CCM_STATUS_INVALID_PARM 		= MCP_HAL_STATUS_INVALID_PARM,
	CCM_STATUS_INTERNAL_ERROR	= MCP_HAL_STATUS_INTERNAL_ERROR,
	CCM_STATUS_IMPROPER_STATE	= MCP_HAL_STATUS_IMPROPER_STATE
} CcmStatus;

#define CCM_IM_STATUS_START 		((McpUint)MCP_HAL_STATUS_OPEN)
#define CCM_PM_STATUS_START 		((McpUint)(MCP_HAL_STATUS_OPEN + 100))
#define CCM_VAC_STATUS_START 		((McpUint)(MCP_HAL_STATUS_OPEN + 200))

#endif

