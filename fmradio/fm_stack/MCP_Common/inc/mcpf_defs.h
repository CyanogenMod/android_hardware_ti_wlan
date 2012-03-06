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

#ifndef __MCPF_DEFS_H
#define __MCPF_DEFS_H

#include "mcp_hal_types.h"


#define INLINE __inline

/* Types Definition */
typedef void * handle_t;	/* abstraction for module/component context */
 
/* Callbacks Prototypes */
typedef void (*cmd_comp_cb)(/*Maital - TBD*/);
typedef void (*mcpf_timer_cb)( handle_t hwnd, McpUint u_Msg, McpUint * idEvent, McpUint u_Time);/*(handle_t hCaller, McpU32 timerid); gpsc_os_timer_expired_res (U8 timerid);*/

/* Enums Definition */
typedef enum 
{
	E_COMPLETE, /* synchronous operation (completed successfully) */
	E_OP_PENDING, /* a-synchronous operation (completion event will follow) */
	E_ERROR,
	E_MEM_ERROR,	/* failure in memory allocation */
	E_FILE_ERROR,	/* failure in file access */
	E_STATE_ERROR,	/* invalid state */		
} mcpf_res_t;


/* Structures Definition */
typedef struct
{
	McpU8			source;	/* for debug purposes (caller driver identifier) */
	McpU8			opcode;	
	void 			*param;
	cmd_comp_cb 	cb;
} mcpf_cmd_t;

typedef void (*cmd_handler_cb)(handle_t hCaller,  mcpf_cmd_t *cmd);


#endif
