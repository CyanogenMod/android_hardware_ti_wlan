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

#ifndef __CCM_IMI_COMMON_H
#define __CCM_IMI_COMMON_H

#include "mcp_hal_types.h"
#include "mcp_config.h"
#include "mcp_defs.h"

#include "ccm_defs.h"
#include "ccm_config.h"

#include "ccm_imi.h"

typedef McpUint _CcmImStatus;

#define _CCM_IM_STATUS_SUCCESS			((_CcmImStatus)CCM_IM_STATUS_SUCCESS)
#define _CCM_IM_STATUS_FAILED				((_CcmImStatus)CCM_IM_STATUS_FAILED)
#define _CCM_IM_STATUS_PENDING			((_CcmImStatus)CCM_IM_STATUS_PENDING)
#define _CCM_IM_STATUS_NO_RESOURCES		((_CcmImStatus)CCM_IM_STATUS_NO_RESOURCES)
#define _CCM_IM_STATUS_IN_PROGRESS		((_CcmImStatus)CCM_IM_STATUS_IN_PROGRESS)
#define _CCM_IM_STATUS_INVALID_PARM		((_CcmImStatus)CCM_IM_STATUS_INVALID_PARM)
#define _CCM_IM_STATUS_INTERNAL_ERROR	((_CcmImStatus)CCM_IM_STATUS_INTERNAL_ERROR)
#define _CCM_IM_STATUS_IMPROPER_STATE	((_CcmImStatus)CCM_IM_STATUS_IMPROPER_STATE)

#define _CCM_IM_FIRST_INTERNAL_STATUS		((_CcmImStatus)1000)

#define _CCM_IM_STATUS_NULL_SM_ENTRY		((_CcmImStatus)_CCM_IM_FIRST_INTERNAL_STATUS)

#define _CCM_IM_STATUS_INVALID_BT_HCI_VERSION_INFO		((_CcmImStatus)_CCM_IM_FIRST_INTERNAL_STATUS + 1)
#define _CCM_IM_STATUS_TRAN_ON_ABORTED					((_CcmImStatus)_CCM_IM_FIRST_INTERNAL_STATUS + 2)

/*
	Index of core (0 - 2) in State Machine
*/
typedef  McpUint _CcmIm_SmStackIdx;

#define _CCM_IM_NUM_OF_SM_STACK_IDXS	((_CcmIm_SmStackIdx)CCM_IM_MAX_NUM_OF_STACKS)
#define _CCM_IM_INVALID_SM_STACK_IDX		((_CcmIm_SmStackIdx)CCM_IM_INVALID_STACK_ID)

#endif

