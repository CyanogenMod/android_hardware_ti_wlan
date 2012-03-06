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
*   FILE NAME:      mcp_hal_endian.h
*
*   BRIEF:          This file defines Endian-related HAL utilities.
*
*   DESCRIPTION:    General
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __MCP_HAL_ENDIAN_H
#define __MCP_HAL_ENDIAN_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_defs.h"


/*---------------------------------------------------------------------------
 * MCP_ENDIAN_BEtoHost16()
 *
 * Brief:
 *	Convert a 16 bits number from Big Endian to Host 
 *
 * Description:  .
 *	Retrieve a 16-bit number from the given buffer. The number is in Big-Endian format.
 *	Converts it to host format and returns it to the caller.
 *
 * Parameters:
 *	
 * Return:
 *	16-bit number in Host's format
 */
McpU16 MCP_ENDIAN_BEtoHost16(const McpU8 *beBuff16);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_BEtoHost32()
 *
 * Brief:
 *	Convert a 32 bits number from Big Endian to Host 
 *
 * Description: 
 *	Retrieve a 32-bit number from the given buffer. The number is in Big-Endian format.
 *	Converts it to host format and returns it to the caller.
 *
 * Parameters:
 *	
 * Return:
 *	32-bit number in Host's format
 */
McpU32 MCP_ENDIAN_BEtoHost32(const McpU8 *beBuff32);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_LEtoHost16()
 *
 * Brief:
 *	Convert a 16 bits number from Little Endian to Host 
 *
 * Description: 
 *	Retrieve a 16-bit number from the given buffer. The number is in Little-Endian format.
 *	Converts it to host format and returns it to the caller.
 *
 * Parameters:
 *	
 * Return:
 *	16-bit number in Host's format
 */
McpU16 MCP_ENDIAN_LEtoHost16(const McpU8 *leBuff16);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_LEtoHost32()
 *
 * Brief:
 *	Convert a 32 bits number from Little Endian to Host 
 *
 * Description:  
 *	Retrieve a 32-bit number from the given buffer. The number is in Little-Endian format.
 *	Converts it to host format and returns it to the caller.
 *
 * Parameters:
 *	
 * Return:
 *	32-bit number in Host's format
 */
McpU32 MCP_ENDIAN_LEtoHost32(const McpU8 *leBuff32);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_HostToLE16()
 *
 * Brief:
 *	Convert a 16 bits number from Host to Little Endian
 *
 * Description: 
 *	Retrieve a 16-bit number from the given host value. The number is in host format.
 *	Converts it to Little-Endian format and stores it in the specified buffer in Little-Endian format.
 *
 * Parameters:
 *	
 * Return:
 *	void
 */
void MCP_ENDIAN_HostToLE16(McpU16 hostValue16, McpU8 *leBuff16);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_HostToLE32()
 *
 * Brief:
 *	Convert a 32 bits number from Host to Little Endian
 *
 * Description: 
 *	Retrieve a 32-bit number from the given host value. The number is in host format.
 *	Converts it to Little-Endian format and stores it in the specified buffer in Little-Endian format.
 *
 * Parameters:
 *	
 * Return:
 *	void
 */
void MCP_ENDIAN_HostToLE32(McpU32 hostValue32, McpU8 *leBuff32);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_HostToBE16()
 *
 * Brief:
 *	Convert a 16 bits number from Host to Little Endian
 *
 * Description: 
 *	Retrieve a 16-bit number from the given host value. The number is in host format.
 *	Converts it to Big-Endian format and stores it in the specified buffer in Big-Endian format.
 *
 * Parameters:
 *	
 * Return:
 *	void
 */
void MCP_ENDIAN_HostToBE16(McpU16 hostValue16, McpU8 *beBuff16);

/*---------------------------------------------------------------------------
 * MCP_ENDIAN_HostToBE32()
 *
 * Brief:
 *	Convert a 32 bits number from Host to Little Endian
 *
 * Description: 
 *	Retrieve a 32-bit number from the given host value. The number is in host format.
 *	Converts it to Big-Endian format and stores it in the specified buffer in Big-Endian format.
 *
 * Parameters:
 *	
 * Return:
 *	void
 */
void MCP_ENDIAN_HostToBE32(McpU32 hostValue32, McpU8 *beBuff32);

#endif	/* __MCP_HAL_ENDIAN_H */


