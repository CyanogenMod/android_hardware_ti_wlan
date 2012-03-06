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

#include "mcp_hal_string.h"
#include "mcp_rom_scripts_db.h"
#include "mcp_defs.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

extern const McpRomScripts_Data mcpRomScripts_Data[];
extern const McpUint mcpRomScripts_NumOfScripts;

McpBool MCP_RomScriptsGetMemInitScriptData(	const char 	*scriptFileName,
											McpUint		*scriptSize,
											const McpU8 **scriptAddress)
{
	McpUint	idx;
	McpBool	scriptFound;

	scriptFound = MCP_FALSE;
	*scriptSize = 0;
	*scriptAddress = NULL;
	
	for (idx = 0; idx < mcpRomScripts_NumOfScripts; ++idx)
	{
		if (MCP_HAL_STRING_StriCmp(scriptFileName, mcpRomScripts_Data[idx].fileName) == 0)
		{
			scriptFound = MCP_TRUE;
			*scriptSize = mcpRomScripts_Data[idx].size;
			*scriptAddress = mcpRomScripts_Data[idx].address;
		}
	}

	MCP_LOG_INFO(("MCP_RomScriptsGetMemInitScriptData: %s file \"%s\" in ROM!", 
		((scriptFound == MCP_TRUE) ? ("found") : ("failed to find")),
		scriptFileName));

	return scriptFound;
}


