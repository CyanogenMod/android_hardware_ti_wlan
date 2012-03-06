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
*   FILE NAME:      fmc_debug.h
*
*   BRIEF:          This file defines debug utilites
*
*   DESCRIPTION:    
*
*	The file contains utilities that are used only for debug purposes.
*
*	To avoid coupling between different modules, only basic types are used in the prototypes.
*
*   AUTHOR:   Udi Ron
*
\*******************************************************************************/
#ifndef __FMC_DEBUG_H
#define __FMC_DEBUG_H

#include "fmc_types.h"

/* Utilities for conversion  of values to strings for readable logging */

/*
	Converting an FmcStatus to a string
*/
const char *FMC_DEBUG_FmcStatusStr(FMC_UINT status);

/*
	Converting an FmRxStatus to a string
*/
const char *FMC_DEBUG_FmRxStatusStr(FMC_UINT status);

/*
	Converting an FmTxStatus to a string
*/
const char *FMC_DEBUG_FmTxStatusStr(FMC_UINT status);

/*
	Converting an FmTxCmdType to a string
*/
const char *FMC_DEBUG_FmTxCmdStr(FMC_UINT cmdType);
/*
	Converting an FmRxCmdType to a string
*/
const char *FMC_DEBUG_FmRxCmdStr(FMC_UINT cmdType);

/*
	Converting an FmcBand to a string
*/
const char *FMC_DEBUG_BandStr(FMC_UINT band);

#endif /* ifndef __FMC_DEBUG_H */

