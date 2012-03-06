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


#ifndef __MCP_CONFIG_H
#define __MCP_CONFIG_H

#include "mcp_hal_types.h"
#include "mcp_hal_config.h"

/*
*   Indicates that a feature is enabled
*/
#define MCP_CONFIG_ENABLED                                          (MCP_HAL_CONFIG_ENABLED)

/*
*   Indicates that a feature is disabled
*/
#define MCP_CONFIG_DISABLED                                         (MCP_HAL_CONFIG_DISABLED)

/*-------------------------------------------------------------------------------
 * MCP_CONFIG_USE_STATIC_KEYWORD
 * 
 *  Determines how program symbols using the MCP_STATIC will be declared.
 *
 * Values:
 *  MCP_CONFIG_ENABLED: The symbols will be declared as static (using the c 'static' keyword)
 *  MCP_CONFIG_DISABLED: The symbols will have global scope
*/
#define     MCP_CONFIG_USE_STATIC_KEYWORD                               (MCP_CONFIG_ENABLED)

#define MCP_CONFIG_BT_HCI_IF_MAX_NUM_OF_REGISTERED_CLIENTS      ((McpUint)2)

/*-------------------------------------------------------------------------------
 * MCP config parser Module
 *
 *     Represents configuration parameters for MCP config parser - 
 *          affects the amount of memory reserved for configuration
 */
#define     MCP_CONFIG_PASRER_MAX_SECTION_LEN                   15
#define     MCP_CONFIG_PASRER_MAX_KEY_NAME_LEN                  40
#define     MCP_CONFIG_PASRER_MAX_KEY_VALUE_LEN                 35
#define     MCP_CONFIG_PARSER_SECTION_NUMBER                    7
#define     MCP_CONFIG_PARSER_KEY_NUMBER                        83


#endif



