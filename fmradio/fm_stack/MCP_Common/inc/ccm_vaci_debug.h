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
*   FILE NAME:      ccm_vaci_debug.h
*
*   BRIEF:          This file include debug prototypes for the Connectivity Chip
*                   Manager (CCM) Voice and Audio Control (VAC) component.
*                  
*
*   DESCRIPTION:    This file includes mainly enum-string conversion functions
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/
#ifndef __CCM_VACI_DEBUG_H__
#define __CCM_VACI_DEBUG_H__

#include "ccm_vac.h"

/*-------------------------------------------------------------------------------
 * _CCM_VAC_DebugResourceStr()
 *
 * Brief:  
 *      Returns a string describing a resource
 *      
 * Description:
 *      Returns a string describing a resource for debug purposes
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      eResource - the resource index
 *
 * Returns:
 *      A char pointer to the descriptive string
 */
const char *_CCM_VAC_DebugResourceStr(ECAL_Resource eResource);

/*-------------------------------------------------------------------------------
 * _CCM_VAC_DebugOperationStr()
 *
 * Brief:  
 *      Returns a string describing an operation
 *      
 * Description:
 *      Returns a string describing an operation for debug purposes
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      eOperation - the operation index
 *
 * Returns:
 *      A char pointer to the descriptive string
 */
const char *_CCM_VAC_DebugOperationStr(ECAL_Operation eOperation);

#endif /* __CCM_VACI_DEBUG_H__ */
