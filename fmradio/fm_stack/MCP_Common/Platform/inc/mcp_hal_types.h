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
*   FILE NAME:      mcp_hal_types.h
*
*   BRIEF:    		Definitions for basic basic HAL Types.
*
*   DESCRIPTION:	This file defines the BASIC hal types. These would be used
*					as base types for upper layers
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __MCP_HAL_TYPES_H
#define __MCP_HAL_TYPES_H

/* -------------------------------------------------------------
 *					Platform-Depndent Part							
 *																
 * 		SET THE VALUES OF THE FOLLOWING PRE-PROCESSOR 			
 *		DEFINITIONS 	TO THE VALUES THAT APPLY TO THE 				
 *		TARGET PLATFORM											
 *																
 */

/* Size of type (char) in the target platform, in bytes	*/
#define MCP_CHAR_SIZE		(1)

/* Size of type (short) in the target platform, in bytes */
#define MCP_SHORT_SIZE 	(2)

/* Size of type (long) in the target platform, in bytes	*/
#define MCP_LONG_SIZE 		(4)

/* Size of type (int) in the target platform, in bytes	*/
#define MCP_INT_SIZE 		(4)

/* -------------------------------------------------------------
 *					8 Bits Types
 */
#if MCP_CHAR_SIZE == 1

typedef unsigned char	 	McpU8;
typedef signed char 		McpS8;

#elif MCP_SHORT_SIZE == 1

typedef unsigned short 	McpU8;
typedef          short 		McpS8;

#elif MCP_INT_SIZE == 1

typedef unsigned int 		McpU8;
typedef          int 		McpS8;

#else

#error Unable to define 8-bits basic types!

#endif

/* -------------------------------------------------------------
 *					16 Bits Types
 */
#if MCP_SHORT_SIZE == 2

typedef unsigned short 	McpU16;
typedef          short 		McpS16;

#elif MCP_INT_SIZE == 2

typedef unsigned int 		McpU16;
typedef          int 		McpS16;

#else

#error Unable to define 16-bits basic types!

#endif

/* -------------------------------------------------------------
 *					32 Bits Types
 */
#if MCP_LONG_SIZE == 4

typedef unsigned long 	McpU32;
typedef          long 	McpS32;

#elif MCP_INT_SIZE == 4

typedef unsigned int 	McpU32;
typedef          int 	McpS32;

#else

#error Unable to define 32-bits basic types!

#endif

/* -------------------------------------------------------------
 *			Native Integer Types (# of bits irrelevant)
 */
typedef int			McpInt;
typedef unsigned int	McpUint;


/* -------------------------------------------------------------
 *					UTF8,16 types
 */
typedef McpU8 	McpUtf8;
typedef McpU16 	McpUtf16;
	
/* --------------------------------------------------------------
 *					Boolean Definitions							 
 */
typedef McpInt McpBool;

#define MCP_TRUE  (1 == 1)
#define MCP_FALSE (0==1) 

/* --------------------------------------------------------------
 *					Null Definition							 
 */
#ifndef NULL
#define NULL    0
#endif

/* -------------------------------------------------------------
 *					LIMITS						
 */
 
#define	MCP_U8_MAX			((McpU8)0xFF)
#define	MCP_U16_MAX			((McpU16)0xFFFF)
#define	MCP_U32_MAX			((McpU32)0xFFFFFFFF)

#if MCP_INT_SIZE == 4

#define MCP_UINT_MAX			(MCP_U32_MAX)

#elif MCP_INT_SIZE == 2

#define MCP_UINT_MAX			(MCP_U16_MAX)

#endif


#endif /* __MCP_HAL_TYPES_H */

