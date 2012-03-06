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

#ifndef __FMC_TYPES_H
#define __FMC_TYPES_H

#include <pthread.h>
#include "mcp_hal_types.h"

/* -------------------------------------------------------------
 *                  8 Bits Types
 */
typedef McpU8       FMC_U8;
typedef McpS8       FMC_S8;

/* -------------------------------------------------------------
 *                  16 Bits Types
 */
typedef McpU16      FMC_U16;
typedef McpS16      FMC_S16;

/* -------------------------------------------------------------
 *                  32 Bits Types
 */
typedef McpU32      FMC_U32;
typedef McpS16      FMC_S32;


/* -------------------------------------------------------------
 *          Native Integer Types (# of bits irrelevant)
 */
typedef McpInt      FMC_INT;
typedef McpUint     FMC_UINT;

   
/* --------------------------------------------------------------
 *                  Boolean Definitions                          
 */

/* --------------------------------------------------------------
 *                  Boolean Definitions                          
 */
typedef McpBool FMC_BOOL;


#define FMC_TRUE    MCP_TRUE
#define FMC_FALSE   MCP_FALSE

/*
*/
#define FMC_NO_VALUE                                    ((FMC_U32)0xFFFFFFFFUL)


#endif  /* __FMC_TYPES_H */

