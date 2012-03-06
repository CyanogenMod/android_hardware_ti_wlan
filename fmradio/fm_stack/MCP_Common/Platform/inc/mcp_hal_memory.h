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
*   FILE NAME:      mcp_hal_memory.h
*
*   BRIEF:          This file defines Memory-related HAL utilities.
*
*   DESCRIPTION:    General
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __MCP_HAL_MEMORY_H
#define __MCP_HAL_MEMORY_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_defs.h"

/*-------------------------------------------------------------------------------
 * MCP_HAL_MEMORY_MemCopy()
 *
 * Brief:  
 *		Called by the stack to copy memory from one buffer to another.
 *
 * Description:
 *		Called by the stack to copy memory from one buffer to another.
 *     
 *		This function's implementation could use the ANSI memcpy function.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		dest [out] - Destination buffer for data.
 *
 *		source [in] - Source buffer for data. "dest" and "source" must not overlap.
 *
 *		numBytes [in] - Number of bytes to copy from "source" to "dest".
 *
 * Returns:
 *		void.
 * 
 */
void MCP_HAL_MEMORY_MemCopy(void *dest, const void *source, McpU32 numBytes);


/*-------------------------------------------------------------------------------
 * MCP_HAL_MEMORY_MemCmp()
 *
 * Brief:  
 *      Compare characters in two buffers.
 *
 * Description:
 *      Called by the stack to compare the bytes in two different buffers.
 *      If the buffers lengths or contents differ, this function returns FALSE.
 *
 *      This function's implementation could use the ANSI memcmp
 *      routine as shown:
 *
 *      return (len1 != len2) ? FALSE : (0 == memcmp(buffer1, buffer2, len2));
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *      buffer1 [in] - First buffer to compare.
 *
 *      len1 [in] - Length of first buffer to compare.
 *
 *      buffer2 [in] - Second buffer to compare.
 *
 *      len2 [in] - Length of second buffer to compare.
 *
 * Returns:
 *      TRUE - The lengths and contents of both buffers match exactly.
 *
 *      FALSE - Either the lengths or the contents of the buffers do not match.
 */
McpBool MCP_HAL_MEMORY_MemCmp(const void *buffer1, McpU16 len1, const void *buffer2, McpU16 len2);


/*-------------------------------------------------------------------------------
 * MCP_HAL_MEMORY_MemSet()
 *
 * Brief: 
 *      Sets buffers to a specified character.
 *
 * Description:
 *     Fills the destination buffer with the specified byte.
 *
 *     This function's implementation could use the ANSI memset
 *     function.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *     dest [out] - Buffer to fill.
 *
 *     byte [in] - Byte to fill with.
 *
 *     len [in] - Length of the destination buffer.
 *
 * Returns:
 *		void.
 */
void MCP_HAL_MEMORY_MemSet(void *dest, McpU8 byte, McpU32 len);


#endif	/* __MCP_HAL_MEMORY_H */

