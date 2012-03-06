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
*   FILE NAME:      mcp_hal_log_udp.h
*
*   BRIEF:          This file defines internal structures and data types required
*                   for UDP logging.
*
*   DESCRIPTION:  
*
*   AUTHOR:         Chen Ganir
*
\*******************************************************************************/
#ifndef __MCP_HAL_LOG_UDP_H
#define __MCP_HAL_LOG_UDP_H

#define MCPHAL_LOG_MAX_FILENAME_LENGTH   150
#define MCPHAL_LOG_MAX_MESSAGE_LENGTH    150
#define MCPHAL_LOG_MAX_THREADNAME_LENGTH 20

typedef struct _udp_log_msg_t
{
    McpU32 line; 
	McpHalLogModuleId_e moduleId;
    McpHalLogSeverity severity;
    char fileName[MCPHAL_LOG_MAX_FILENAME_LENGTH];
    char message[MCPHAL_LOG_MAX_MESSAGE_LENGTH];
    char threadName[MCPHAL_LOG_MAX_THREADNAME_LENGTH];
} udp_log_msg_t;

#endif /* __MCP_HAL_LOG_UDP_H */



