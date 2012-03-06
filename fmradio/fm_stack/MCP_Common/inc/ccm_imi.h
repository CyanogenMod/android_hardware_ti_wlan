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


#ifndef __CCM_IMI_H
#define __CCM_IMI_H

#include "ccm_im.h"
#include "bt_hci_if.h"

typedef void (*CcmChipOnNotificationCb)(void *userData,
                                        McpU16 projectType,
                                        McpU16 versionMajor,
                                        McpU16 versionMinor);

CcmImStatus CCM_IM_StaticInit(void);

CcmImStatus CCM_IM_Create(McpHalChipId chipId, CcmImObj **thisObj, CcmChipOnNotificationCb, void *userData);
CcmImStatus CCM_IM_Destroy(CcmImObj **thisObj);


/*-------------------------------------------------------------------------------
 * CCM_IMI_GetBtHciIfObj()
 *
 * Brief:       
 *  Returns the contained instance of he BT_HCI_If
 *
 * Description:
 *  Returns the contained instance of he BT_HCI_If
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      this [in / out] - pointer to the instance data. Set to null on exit
 *
 * Returns:
 *      Pointer to the contained instance
 */
BtHciIfObj *CCM_IMI_GetBtHciIfObj(CcmImObj *thisObj);

#endif  /* __CCM_IMI_H */

