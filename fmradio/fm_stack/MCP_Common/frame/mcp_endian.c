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
#include "mcp_endian.h"

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
McpU16 MCP_ENDIAN_BEtoHost16(const McpU8 *beBuff16)
{
    return (McpU16)( ((McpU16) *beBuff16 << 8) | ((McpU16) *(beBuff16+1)) ); 
}

McpU32 MCP_ENDIAN_BEtoHost32(const McpU8 *beBuff32)
{
    return (McpU32)( ((McpU32) *beBuff32 << 24)   | \
                  ((McpU32) *(beBuff32+1) << 16) | \
                  ((McpU32) *(beBuff32+2) << 8)  | \
                  ((McpU32) *(beBuff32+3)) );
}

McpU16 MCP_ENDIAN_LEtoHost16(const McpU8 *leBuff16)
{
    return (McpU16)( ((McpU16) *(leBuff16+1) << 8) | ((McpU16) *leBuff16) ); 
}

McpU32 MCP_ENDIAN_LEtoHost32(const McpU8 *leBuff32)
{
    return (McpU32)( ((McpU32) *(leBuff32+3) << 24)   | \
                  ((McpU32) *(leBuff32+2) << 16) | \
                  ((McpU32) *(leBuff32+1) << 8)  | \
                  ((McpU32) *(leBuff32)) );
}

void MCP_ENDIAN_HostToLE16(McpU16 hostValue16, McpU8 *leBuff16)
{
   leBuff16[1] = (McpU8)(hostValue16>>8);
   leBuff16[0] = (McpU8)hostValue16;
}

void MCP_ENDIAN_HostToLE32(McpU32 hostValue32, McpU8 *leBuff32)
{
   leBuff32[3] = (McpU8) (hostValue32>>24);
   leBuff32[2] = (McpU8) (hostValue32>>16);
   leBuff32[1] = (McpU8) (hostValue32>>8);
   leBuff32[0] = (McpU8) hostValue32;
}

void MCP_ENDIAN_HostToBE16(McpU16 hostValue16, McpU8 *beBuff16)
{
   beBuff16[0] = (McpU8)(hostValue16>>8);
   beBuff16[1] = (McpU8)hostValue16;
}

void MCP_ENDIAN_HostToBE32(McpU32 hostValue32, McpU8 *beBuff32)
{
   beBuff32[0] = (McpU8) (hostValue32>>24);
   beBuff32[1] = (McpU8) (hostValue32>>16);
   beBuff32[2] = (McpU8) (hostValue32>>8);
   beBuff32[3] = (McpU8) hostValue32;
}





