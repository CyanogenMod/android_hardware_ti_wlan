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
*   FILE NAME:      lineparser.h
*
*   BRIEF:          This file defines the API of the line parser.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/
#ifndef __MCP_LINUX_LINE_PARSER_H_
#define __MCP_LINUX_LINE_PARSER_H_

#include "mcp_hal_types.h"

#define MCP_LINUX_LINE_PARSER_MAX_NUM_OF_ARGUMENTS		10
#define MCP_LINUX_LINE_PARSER_MAX_LINE_LEN					200
#define MCP_LINUX_LINE_PARSER_MAX_MODULE_NAME_LEN		10
#define MCP_LINUX_LINE_PARSER_MAX_STR_LEN			    		80

typedef enum
{
	MCP_LINUX_LINE_PARSER_STATUS_SUCCESS,
	MCP_LINUX_LINE_PARSER_STATUS_FAILED,
	MCP_LINUX_LINE_PARSER_STATUS_ARGUMENT_TOO_LONG,
	MCP_LINUX_LINE_PARSER_STATUS_NO_MORE_ARGUMENTS
} MCP_LINUX_LINE_PARSER_STATUS;

MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_ParseLine(McpU8 *line, const char* delimiters);
McpU32 MCP_LINUX_LINE_PARSER_GetNumOfArgs(void);
McpBool MCP_LINUX_LINE_PARSER_AreThereMoreArgs(void);

void MCP_LINUX_LINE_PARSER_ToLower(McpU8 *str);

MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextChar(McpU8 *c);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextStr(McpU8 *str, McpU8 len);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextU8(McpU8 *value, McpBool hex);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextU16(McpU16 *value, McpBool hex);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextU32(McpU32 *value, McpBool hex);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextS8(McpS8 *value);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextS16(McpS16 *value);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextS32(McpS32 *value);
MCP_LINUX_LINE_PARSER_STATUS MCP_LINUX_LINE_PARSER_GetNextBool(McpBool *value);

#endif	/* __MCP_LINUX_LINE_PARSER_H_ */

