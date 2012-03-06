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
*   FILE NAME:      mcp_hal_utils.c
*
*   DESCRIPTION:    This file implements the MCP HAL Utils for Windows.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/
#include <memory.h>
#include <string.h>

#include "mcp_hal_memory.h"

/****************************************************************************
 *
 * Local Prototypes
 *
 ***************************************************************************/

/****************************************************************************
 *
 * Public Functions
 *
 ***************************************************************************/


void MCP_HAL_MEMORY_MemCopy(void *dest, const void *source, McpU32 numBytes)
{
    memcpy(dest, source, numBytes);
}

McpBool MCP_HAL_MEMORY_MemCmp(const void *buffer1, McpU16 len1, const void *buffer2, McpU16 len2)
{
    if (len1 != len2) {
        return MCP_FALSE;
    }

    return(0 == memcmp(buffer1, buffer2, len1));
}

void MCP_HAL_MEMORY_MemSet(void *dest, McpU8 byte, McpU32 len)
{
    memset(dest, byte, len);
}

