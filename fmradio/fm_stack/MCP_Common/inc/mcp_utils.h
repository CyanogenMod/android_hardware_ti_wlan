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
/******************************************************************************\
*
*   FILE NAME:      mcp_utils.h
*
*   BRIEF:          This file defines utilities that are not part of any
*                   specific functionality (e.g, strings or memory).
*
*   DESCRIPTION:    General
*                   
*   AUTHOR:         Vladimir Abram
*
\*******************************************************************************/

#ifndef __MCP_UTILS_H
#define __MCP_UTILS_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h"


/*------------------------------------------------------------------------------
 * MCP_UTILS_AtoU32()
 *
 * Brief: 
 *    Convert strings to double  (same as ANSI C atoi)
 *
 * Description: 
 *    Convert strings to double  (same as ANSI C atoi)
 * 	  The MCP_UTILS_AtoU32 function converts a character string to an integer
 *    value.
 *    The function do not recognize decimal points or exponents.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *     string [in] - String to be converted which has the following form:
 *
 *    [whitespace][sign][digits]
 *
 *	  Where whitespace consists of space and/or tab characters, which are
 *    ignored;
 *    sign is either plus (+) or minus (–); and digits are one or more decimal
 *    digits. 
 *
 * Returns:
 *	  A U32 value produced by interpreting the input characters as a number.
 *    The return value is 0 if the input cannot be converted to a value of that
 *    type. 
 *	  The return value is undefined in case of overflow.
 */
McpU32 MCP_UTILS_AtoU32(const char *string);


#endif	/* __MCP_UTILS_H */

