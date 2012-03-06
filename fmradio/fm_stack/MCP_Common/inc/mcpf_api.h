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

#ifndef __MCPF_API_H
#define __MCPF_API_H

#include <stdio.h>

#include "mcpf_defs.h"
#include "mcp_endian.h"
#include "mcpf_queue.h"


/** MCPF APIs - Initialization (TODO In Second Phase)  **/
handle_t    mcpf_create();

handle_t    mcpf_destroy();

handle_t    drvf_open ( handle_t    hMcpf, 
                        handle_t    hCaller,
                        McpU8       num_of_prio_queues,
                        McpU32      num_incoming_messages);

mcpf_res_t  drvf_close (handle_t      hDrvf);

mcpf_res_t  drvf_register ( handle_t        hDrvf, 
                            handle_t        hCaller,
                            McpU8           dest, 
                            cmd_handler_cb      cb);  

mcpf_res_t  drvf_unregister(handle_t        hDrvf, 
                            handle_t        hCaller,
                            McpU8           dest, 
                            cmd_handler_cb  cb);

mcpf_res_t  mcpf_handle_msg(handle_t    *hMcpf, 
                            McpU8        dest,    // target driver identifier  
                            mcpf_cmd_t  *cmd);

mcpf_res_t  mcpf_enqueue_msg(handle_t   *hMcpf, 
                             McpU8      dest,     // target driver identifier  
                             McpU8      priority,   
                             mcpf_cmd_t     *cmd);  

mcpf_res_t  drvf_msgq_enable(handle_t   hDrvf, 
                             McpU8      priority);
    
mcpf_res_t  drvf_msgq_disable(handle_t  hDrvf, 
                              McpU8     priority);


/** Utils **/
/* Queue Handling */
#define mcpf_que_Create  que_Create

#define mcpf_que_Destroy que_Destroy

#define mcpf_que_Enqueue que_Enqueue

#define mcpf_que_Dequeue que_Dequeue

#define mcpf_que_Requeue que_Requeue

#define mcpf_que_Size    que_Size

/* Little/Big endianess conversion (LEtoHost16/32…) */
#define mcpf_endian_BEtoHost16 MCP_ENDIAN_BEtoHost16

#define mcpf_endian_BEtoHost32 MCP_ENDIAN_BEtoHost32

#define mcpf_endian_BEtoHost326 MCP_ENDIAN_BEtoHost32

#define mcpf_endian_LEtoHost32 MCP_ENDIAN_LEtoHost32

#define mcpf_endian_HostToLE16 MCP_ENDIAN_HostToLE16

#define mcpf_endian_HostToLE32 MCP_ENDIAN_HostToLE32

#define mcpf_endian_HostToBE16 MCP_ENDIAN_HostToBE16

#define mcpf_endian_HostToBE32 MCP_ENDIAN_HostToBE32


/** MCPF APIs - OS Related **/

/* Memory Alloc */
McpS8       *mcpf_mem_alloc (handle_t   hMcpf, McpU16 length); // gpsc_os_malloc_ind (U16 mem_size);

void  mcpf_mem_free  (handle_t hMcpf, McpU8 *buf); // gpsc_os_free_ind (U8 *Mem);


/* Memory Pools Management (TODO In Second Phase) */
void        *mcpf_memory_pool_create(handle_t hMcpf, McpU16 element_size, McpU16 element_num);

mcpf_res_t  mcpf_memory_pool_destroy(handle_t hMcpf,  void *mpool);

McpS8       *mcpf_mem_alloc_from_pool(handle_t hMcpf, void *mpool); 

mcpf_res_t  mcpf_mem_free_from_pool (handle_t  hMcpf, void *mpool, McpS8 *buf);


/* Critical Section */
mcpf_res_t mcpf_smphr_CreateSemaphore(handle_t hMcpf, const char *semaphoreName, handle_t *semaphoreHandle); 

mcpf_res_t mcpf_smphr_DestroySemaphore(handle_t hMcpf, handle_t *semaphoreHandle); 

mcpf_res_t mcpf_smphr_LockSemaphore(handle_t hMcpf, handle_t semaphoreHandle,McpUint timeout);

mcpf_res_t mcpf_smphr_UnlockSemaphore(handle_t hMcpf, handle_t semaphoreHandle);


/* Timers */
McpUint     mcpf_os_timer_start(handle_t        hMcpf,
                                    McpU8           timerid,
                                    McpU32          expiry_time,
                                    mcpf_timer_cb   cb, 
                                    handle_t        hCaller); 

McpBool     mcpf_os_timer_stop(handle_t         hMcpf,
                                McpU8           timerid); 


/* Logging - Macros are defined in mcpf_report.h */
/* Replace these functions:
    void gpsc_os_diagnostic_ind (T_GPSC_diag_level diag_level, S8 *diag_string);

    void gpsc_os_fatal_error_ind (T_GPSC_result result);
*/


/* Files (TODO In Second Phase) */
FILE        *mcpf_file_open(McpU8 *FileName, McpU8 *Flags);

mcpf_res_t  mcpf_file_close(FILE *fd);

McpS32          mcpf_file_read(FILE *fd, McpU8 *Mem, McpU16 num_bytes);

McpS32          mcpf_file_write(FILE *fd, McpU8 *Mem, McpU16 num_bytes);

mcpf_res_t  mcpf_file_empty(FILE *fd);
/* Replace these functions:
    T_GPSC_file_result  gpsc_os_file_read_ind   (U8 fileid, U8 *Mem, U16 num_bytes);

    T_GPSC_file_result  gpsc_os_file_write_ind  (U8 fileid, U8 *Mem, U16 num_bytes);
  
    T_GPSC_file_result  gpsc_os_file_delete_ind (U8 fileid);
    
    S32                 gpsc_os_patch_open_ind  (void);
      
    S32                 gpsc_os_patch_read_ind  (U8 *Mem, U16 num_bytes);
        
    void                gpsc_os_patch_close_ind (void);
*/


/* Miscellaneous (TODO In Second Phase) */
/* Replace these functions:
    U32 gpsc_os_get_current_time_ind (void);
*/


/** MCPF APIs -  Data Path Related **/
/* TODO in Shared transport implementation */


#endif
