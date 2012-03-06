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
*   FILE NAME:      mcp_config_reader.h
*
*   BRIEF:          Init file reader
*
*   DESCRIPTION:    The init file reader utility handles ini configuration
*                   files and their memory replacement within a single object.
*                   It allows the file parser to receive one line at a time, 
*                   encapsulating file handling and differences between file
*                   and memory.
*
*   AUTHOR:         Ronen Kalish
*
\******************************************************************************/
#ifndef __MCP_CONFIG_READER_H__
#define __MCP_CONFIG_READER_H__

#include "mcp_hal_types.h"
#include "mcp_hal_fs.h"

/*-------------------------------------------------------------------------------
 * McpConfigParser type
 *
 *     The MCP config reader object
 */
typedef struct _McpConfigReader
{
    McpHalFsFileDesc        tFile;
    McpU32                  uFileSize;
    McpU32                  uFileBytesRead;
    McpU8                   *pMemory;
} McpConfigReader;

/*-------------------------------------------------------------------------------
 * MCP_CONFIG_READER_Open()
 *
 * Brief:  
 *      Opens a new configuration reader object, either opens a file or memory
 *      location
 *      
 * Description:
 *      This function will initialize a config reader object, and attempt to open 
 *      the supplied ini file. If it fails it will attempt to open the supplied
 *      memory ini alternative. 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pConfigParser   [in]    - The memory for the config reader object
 *      pFileName       [in]    - the file to open (NULL if no file exist)
 *      pMemeConfig     [in]    - The alternative memory configuration (NULL if 
 *                                it shouldn't be used).
 *
 * Returns:
 *      TRUE        - Configuration opened successfully
 *      FALSE       - Failed to open configuration object
 */
McpBool MCP_CONFIG_READER_Open (McpConfigReader *pConfigReader, 
                                McpUtf8 *pFileName, 
                                McpU8 *pMemConfig);
/*-------------------------------------------------------------------------------
 * MCP_CONFIG_READER_Close()
 *
 * Brief:  
 *		Closes the file indicated by pConfigReader->tFile if exists.
 *		
 * Description:
 *		Closes the file indicated by pConfigReader->tFile if exists.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		pConfigParser	[in]	- The memory for the config reader object
 */
McpBool MCP_CONFIG_READER_Close (McpConfigReader *pConfigReader);

/*-------------------------------------------------------------------------------
 * MCP_CONFIG_READER_getNextLine()
 *
 * Brief:  
 *      Retrieves the next line in a configuration storage object.
 *      
 * Description:
 *      Retrieves the next line in a configuration storage object.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      pConfigParser   [in]    - The memory for the config reader object
 *      pFileName       [in]    - the file to open (NULL if no file exist)
 *      pMemeConfig     [in]    - The alternative memory configuration (NULL if 
 *                                it shouldn't be used).
 *
 * Returns:
 *      TRUE        - Configuration opened successfully
 *      FALSE       - Failed to open configuration object
 */
McpBool MCP_CONFIG_READER_getNextLine (McpConfigReader * pConfigReader,
                                       McpU8* pLine);

#endif /* __MCP_CONFIG_READER_H__ */
