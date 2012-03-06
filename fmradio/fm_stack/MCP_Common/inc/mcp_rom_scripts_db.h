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
*   FILE NAME:      mcp_rom_scripts_db.h
*
*   BRIEF:          This file defines the interface for Rom Scripts repository.
*
*   DESCRIPTION:
*	
*
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __MCP_ROM_SCRIPTS_DB_H
#define __MCP_ROM_SCRIPTS_DB_H

typedef struct tagMcpRomScripts_Data {
	const char	*fileName;
	McpUint		size;
	const McpU8	*address;
} McpRomScripts_Data;


McpBool MCP_RomScriptsGetMemInitScriptData(	const char 	*scriptFileName,
											McpUint		*scriptSize,
											const McpU8 **scriptAddress);

#endif /* __MCP_ROM_SCRIPTS_DB_H */

