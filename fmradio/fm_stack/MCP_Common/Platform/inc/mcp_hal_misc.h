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
*   FILE NAME:      mcp_hal_misc.h
*
*   BRIEF:          This file defines Miscellaneous HAL utilities that are not
*                   part of any specific functionality (e.g, strings or memory).
*
*   DESCRIPTION:    General
*                   
*   AUTHOR:         Udi Ron
*
\******************************************************************************/

#ifndef __MCP_HAL_MISC_H
#define __MCP_HAL_MISC_H


/*******************************************************************************
 *
 * Include files
 *
 ******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_defs.h"


/*------------------------------------------------------------------------------
 * MCP_HAL_MISC_Srand()
 *
 * Brief:  
 *    Sets a random starting point
 *
 * Description:
 *	The function sets the starting point for generating a series of pseudorandom
 *  integers. To reinitialize the generator, use 1 as the seed argument. Any
 *  other value for seed sets the generator to a random starting point.
 *  MCP_HAL_MISC_Rand() retrieves the pseudorandom numbers that are generated. 
 *	Calling MCP_HAL_MISC_Rand() before any call to srand generates the same
 *  sequence as calling MCP_HAL_MISC_Srand() with seed passed as 1.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		seed [in] - Seed for random-number generation
 *
 * Returns:
 *     void
 */
void MCP_HAL_MISC_Srand(McpUint seed);

/*------------------------------------------------------------------------------
 * MCP_HAL_MISC_Rand()
 *
 * Brief:  
 *     Generates a pseudorandom number
 *
 * Description:
 *    Returns a pseudorandom integer in the range 0 to 65535. 
 *
 *	Use the MCP_HAL_MISC_Srand() function to seed the pseudorandom-number
 *  generator before calling rand.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		void.
 *
 * Returns:
 *     Returns a pseudorandom number, as described above.
 */
McpU16 MCP_HAL_MISC_Rand(void);


/*---------------------------------------------------------------------------
 * MCP_HAL_MISC_Assert()
 *
 * Brief: 
 *     Called by the stack to indicate that an assertion failed. 
 *
 * Description: 
 *     Called by the stack to indicate that an assertion failed. MCP_HAL_MISC_Assert
 *     should display the failed expression and the file and line number
 *     where the expression occurred.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *     expression [in] - A string containing the failed expression.
 *
 *     file [in] - A string containing the file in which the expression occurred.
 *
 *     line [in] - The line number that tested the expression.
 *
 * Returns:
 *		void.
 */
void MCP_HAL_MISC_Assert(const char *expression, const char *file, McpU16 line);

#endif	/* __MCP_HAL_MISC_H */

