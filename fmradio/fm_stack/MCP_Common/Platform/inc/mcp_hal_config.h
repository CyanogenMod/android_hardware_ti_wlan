/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010 Sony Ericsson Mobile Communications AB.
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
*   FILE NAME:      mcp_hal_config.h
*
*   BRIEF:          MCP HAL Configuration Parameters
*
*   DESCRIPTION:    
*
*   The constants in this file configure the MCP HAL layer for a specific
*   platform and project.
*
*   Some constants are numeric, and others indicate whether a feature is enabled
*   (defined as MCP_HAL_CONFIG_ENABLED) or disabled (defined as
*     MCP_HAL_CONFIG_DISABLED).
*
*   The values in this specific file are tailored for a Windows distribution.
*   To change a constant, simply change its value in this file and recompile the
*   entire BTIPS package.
*
*   AUTHOR:         Udi Ron
*
*******************************************************************************/

#ifndef __MCP_HAL_CONFIG_H
#define __MCP_HAL_CONFIG_H

#include "mcp_hal_types.h"


/*------------------------------------------------------------------------------
 * Common
 *
 *     Represents common configuration parameters.
 */

/*
 *	Indicates that a feature is enabled
 */
#define MCP_HAL_CONFIG_ENABLED									(MCP_TRUE)

/*
 *	Indicates that a feature is disabled
 */
#define MCP_HAL_CONFIG_DISABLED									(MCP_FALSE)

/*
 * Defines the number of chips that are in actual use in the system.
 *
 * Values may be 1 or 2
 */
#define MCP_HAL_CONFIG_NUM_OF_CHIPS								((McpUint)1)

/*
 *  The maximum number of bytes in one UFF-8 character
 */
#define MCP_HAL_CONFIG_MAX_BYTES_IN_UTF8_CHAR                   (2)

/*------------------------------------------------------------------------------
 * FS
 *
 *     Represents configuration parameters for FS module.
 */

/*
*   Specific the absolute path of the folder where MCP scripts should be located
*/
#if defined(ANDROID)
  #define MCP_HAL_CONFIG_FS_SCRIPT_FOLDER		                                ("/etc/firmware/")
#elif defined(SDP3430)
 #error "Build is for Android"
 #define MCP_HAL_CONFIG_FS_SCRIPT_FOLDER		                                ("/lib/firmware/")
#endif

/*
 *  The maximum length of a file system path, including file name component
 *  (in characters) and 0-terminator
*/
#define MCP_HAL_CONFIG_FS_MAX_PATH_LEN_CHARS                    (256)

/*
 *  The maximum name of the file name part on the local file system
 *  (in characters) and 0-terminator
*/
#define MCP_HAL_CONFIG_FS_MAX_FILE_NAME_LEN_CHARS               (256)

/*
 *  The maximum number of directories that may be opened simultaneously via the
 *  CP_HAL_FS_OpenDir()
*/
#define MCP_HAL_CONFIG_FS_MAX_NUM_OF_OPEN_DIRS				    (10)
/*
*   The folder separator character of files on the file system
 *	[ToDo] - What to do when the path is non-ASCII?
*/
#define MCP_HAL_CONFIG_FS_PATH_DELIMITER                        ('/')

/*
 *  Define if the file system is case sensitive
*/
#define MCP_HAL_CONFIG_FS_CASE_SENSITIVE                        (MCP_TRUE)

/*------------------------------------------------------------------------------
 * OS
 *
 *     Represents configuration parameters for OS module.
 */
                            
/* 	
 *	The total number of semaphores that are in use in the system
 *	
 *	BT: 5
 *	FM: 1 
 *
 *	[ToDo] - Mechainsm to adapt the number to the actual clients and their needs
 */
#define MCP_HAL_CONFIG_OS_MAX_NUM_OF_SEMAPHORES                 (6)

#define MCP_HAL_OS_MAX_ENTITY_NAME_LEN							(20)

#define LOG_FILE                                                "/sqlite_stmt_journals/btips_log.txt"

#define CORE_DUMP_LOCATION                                      "/btips/"

#endif /* __MCP_HAL_CONFIG_CONFIG_H */

