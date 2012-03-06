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


/** Include Filess **/
#include "mcpf_api.h"
#include "mcp_hal_os.h"

/** Macro definitions **/
#define _MCPF_MAIN_UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM))

/** MCPF APIs - Initialization (TODO In Second Phase)  **/
handle_t 	mcpf_create()
{
	return NULL;
}

handle_t 	mcpf_destroy()
{
	return NULL;
}

handle_t	drvf_open ( handle_t	hMcpf, 
						handle_t	hCaller,
						McpU8 		num_of_prio_queues,
						McpU32      num_incoming_messages)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(hCaller);
	_MCPF_MAIN_UNUSED_PARAMETER(num_of_prio_queues);
	_MCPF_MAIN_UNUSED_PARAMETER(num_incoming_messages);
	return NULL;
}

mcpf_res_t	drvf_close (handle_t 	  hDrvf)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hDrvf);
	return E_COMPLETE;
}

mcpf_res_t	drvf_register (	handle_t 		hDrvf, 
							handle_t 		hCaller,
							McpU8 		  	dest, 
							cmd_handler_cb  	cb)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hDrvf);
	_MCPF_MAIN_UNUSED_PARAMETER(hCaller);
	_MCPF_MAIN_UNUSED_PARAMETER(dest);
	_MCPF_MAIN_UNUSED_PARAMETER(cb);
	return E_COMPLETE;
}

mcpf_res_t	drvf_unregister(handle_t 		hDrvf, 
							handle_t 		hCaller,
							McpU8 			dest, 
							cmd_handler_cb  cb)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hDrvf);
	_MCPF_MAIN_UNUSED_PARAMETER(hCaller);
	_MCPF_MAIN_UNUSED_PARAMETER(dest);
	_MCPF_MAIN_UNUSED_PARAMETER(cb);
	return E_COMPLETE;
}

mcpf_res_t	mcpf_handle_msg(handle_t 	*hMcpf, 
							McpU8 	  	 dest, 	  // target driver identifier  
							mcpf_cmd_t 	*cmd)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(dest);
	_MCPF_MAIN_UNUSED_PARAMETER(cmd);
	return E_COMPLETE;
}

mcpf_res_t	mcpf_enqueue_msg(handle_t 	*hMcpf, 
							 McpU8 		dest, 	  // target driver identifier  
							 McpU8		priority,	
							 mcpf_cmd_t 	*cmd)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(dest);
	_MCPF_MAIN_UNUSED_PARAMETER(priority);
	_MCPF_MAIN_UNUSED_PARAMETER(cmd);
	return E_COMPLETE;
}

mcpf_res_t	drvf_msgq_enable(handle_t 	hDrvf, 
							 McpU8		priority)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hDrvf);
	_MCPF_MAIN_UNUSED_PARAMETER(priority);
	return E_COMPLETE;
}
	
mcpf_res_t	drvf_msgq_disable(handle_t 	hDrvf, 
							  McpU8		priority)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hDrvf);
	_MCPF_MAIN_UNUSED_PARAMETER(priority);
	return E_COMPLETE;
}



/** MCPF APIs - OS Related **/

/* Memory Alloc */
McpS8	  	*mcpf_mem_alloc (handle_t 	hMcpf, McpU16 length)
{
	return os_memoryAlloc(hMcpf, length);
}

void  mcpf_mem_free  (handle_t hMcpf, McpU8 *buf)
{ 
	os_memoryFree(hMcpf, buf, sizeof(buf));
}


/* Memory Pools Management (TODO In Second Phase) */
void 		*mcpf_memory_pool_create(handle_t hMcpf, McpU16 element_size, McpU16 element_num)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(element_size);
	_MCPF_MAIN_UNUSED_PARAMETER(element_num);
	return NULL;
}

mcpf_res_t  mcpf_memory_pool_destroy(handle_t hMcpf,  void *mpool)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(mpool);
	return E_COMPLETE;
}

McpS8  		*mcpf_mem_alloc_from_pool(handle_t hMcpf, void *mpool)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(mpool);
	return NULL;
}

mcpf_res_t  mcpf_mem_free_from_pool (handle_t  hMcpf, void *mpool, McpS8 *buf)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(mpool);
	_MCPF_MAIN_UNUSED_PARAMETER(buf);
	return E_COMPLETE;
}


/* Critical Section */
mcpf_res_t mcpf_smphr_CreateSemaphore(handle_t hMcpf, const char *semaphoreName, handle_t *semaphoreHandle)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	return MCP_HAL_OS_CreateSemaphore(semaphoreName, semaphoreHandle);
}

mcpf_res_t mcpf_smphr_DestroySemaphore(handle_t hMcpf, handle_t *semaphoreHandle)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	return MCP_HAL_OS_DestroySemaphore(semaphoreHandle);
}

mcpf_res_t mcpf_smphr_LockSemaphore(handle_t hMcpf, handle_t semaphoreHandle,McpUint timeout)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	return MCP_HAL_OS_LockSemaphore(semaphoreHandle, timeout);
}

mcpf_res_t mcpf_smphr_UnlockSemaphore(handle_t hMcpf, handle_t semaphoreHandle)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	return MCP_HAL_OS_UnlockSemaphore(semaphoreHandle);
}


/* Timers */
McpUint mcpf_os_timer_start(handle_t hMcpf,	McpU8 timerid, McpU32 expiry_time,
							mcpf_timer_cb cb, handle_t hCaller)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	_MCPF_MAIN_UNUSED_PARAMETER(timerid);
	_MCPF_MAIN_UNUSED_PARAMETER(hCaller);
	return os_timerStart(expiry_time, cb);
}

McpBool mcpf_os_timer_stop(handle_t hMcpf,	McpU8 timerid)
{
	_MCPF_MAIN_UNUSED_PARAMETER(hMcpf);
	return os_timerStop(timerid);
}

/* Files (TODO In Second Phase) */
FILE		*mcpf_file_open(McpU8 *FileName, McpU8 *Flags)
{
	_MCPF_MAIN_UNUSED_PARAMETER(FileName);
	_MCPF_MAIN_UNUSED_PARAMETER(Flags);
	return NULL;
}

mcpf_res_t	mcpf_file_close(FILE *fd)
{
	_MCPF_MAIN_UNUSED_PARAMETER(fd);
	return E_COMPLETE;
}

McpS32			mcpf_file_read(FILE *fd, McpU8 *Mem, McpU16 num_bytes)
{
	_MCPF_MAIN_UNUSED_PARAMETER(fd);
	_MCPF_MAIN_UNUSED_PARAMETER(Mem);
	_MCPF_MAIN_UNUSED_PARAMETER(num_bytes);
	return 0;
}

McpS32			mcpf_file_write(FILE *fd, McpU8 *Mem, McpU16 num_bytes)
{
	_MCPF_MAIN_UNUSED_PARAMETER(fd);
	_MCPF_MAIN_UNUSED_PARAMETER(Mem);
	_MCPF_MAIN_UNUSED_PARAMETER(num_bytes);
	return 0;
}

mcpf_res_t	mcpf_file_empty(FILE *fd)
{
	_MCPF_MAIN_UNUSED_PARAMETER(fd);
	return E_COMPLETE;
}


/* Miscellaneous (TODO In Second Phase) */



/** MCPF APIs -  Data Path Related **/

