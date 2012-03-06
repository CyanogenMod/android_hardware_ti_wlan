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

#ifndef __CCM_HAL_PWR_UP_DWN_H
#define __CCM_HAL_PWR_UP_DWN_H

#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"

typedef enum tagCcmHalPwrUpDwnStatus
{
	CCM_HAL_PWR_UP_DWN_STATUS_SUCCESS		= MCP_HAL_STATUS_SUCCESS ,
	CCM_HAL_PWR_UP_DWN_STATUS_FAILED			= MCP_HAL_STATUS_FAILED,
	CCM_HAL_PWR_UP_DWN_STATUS_INVALID_PARM	= MCP_HAL_STATUS_INVALID_PARM,
	CCM_HAL_PWR_UP_DWN_STATUS_INTERNAL_ERROR	= MCP_HAL_STATUS_INTERNAL_ERROR
} CcmHalPwrUpDwnStatus;

CcmHalPwrUpDwnStatus CCM_HAL_PWR_UP_DWN_Init(void);

CcmHalPwrUpDwnStatus CCM_HAL_PWR_UP_DWN_Reset(McpHalChipId	chipId, McpHalCoreId coreId);
CcmHalPwrUpDwnStatus CCM_HAL_PWR_UP_DWN_Shutdown(McpHalChipId chipId, McpHalCoreId coreId);


#endif	/* __CCM_HAL_PWR_UP_DWN_H*/

