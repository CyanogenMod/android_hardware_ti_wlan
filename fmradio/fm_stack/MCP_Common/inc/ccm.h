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

#ifndef __CCM_H
#define __CCM_H

#include "mcp_hal_types.h"
#include "mcp_hal_defs.h"

#include "mcp_defs.h"

#include "ccm_defs.h"

#include "ccm_im.h"
#include "ccm_vac.h"
#include "bt_hci_if.h"

typedef struct tagCcmObj CcmObj;

CcmStatus CCM_StaticInit(void);

/*-------------------------------------------------------------------------------
 * CCM_Create()
 *
 * Brief:       
 *  Creates a CCM instance
 *
 * Description:
 *  Creates a new instance of the CCM. 
 *
 *  
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      chipId [in] - Identifies the chip that this instance is managing
 *      this [out] - pointer to the instance data
 *
 * Returns:
 *      CCM_STATUS_SUCCESS - Operation is successful.
 */
CcmStatus CCM_Create(McpHalChipId chipId, CcmObj **thisObj);

CcmStatus CCM_Destroy(CcmObj **thisObj);

CcmImObj *CCM_GetIm(CcmObj *thisObj);

TCCM_VAC_Object *CCM_GetVac(CcmObj *thisObj);

Cal_Config_ID *CCM_GetCAL(CcmObj *thisObj);

BtHciIfObj *CCM_GetBtHciIfObj(CcmObj *thisObj);

#endif  /* __CCM_H */

