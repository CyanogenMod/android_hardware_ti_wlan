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
*   FILE NAME:      mcp_hal_utils.c
*
*   DESCRIPTION:    This file implements the MCP HAL Utils for Linux.
*
*   AUTHOR:         Chen Ganir
*
\*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "mcp_hal_log.h"
#include "mcp_hal_misc.h"

#include "fm_trace.h"

/* SEMC_BEGIN (Including math.h to enable logarithmic volume gain levels) */
#include "math.h"
/* SEMC_END (Including math.h to enable logarithmic volume gain levels) */

/****************************************************************************
 *
 * Public Functions
 *
 ***************************************************************************/
void MCP_HAL_MISC_Srand(McpUint seed)
{
    srand(seed);
}

McpU16 MCP_HAL_MISC_Rand(void)
{
    return (McpU16)(rand() % MCP_U16_MAX);
}

void MCP_HAL_MISC_Assert(const char *expression, const char *file, McpU16 line)
{
    MCP_HAL_LOG_ERROR(file, line, MCP_HAL_LOG_MODULE_TYPE_ASSERT, (expression));
    sleep(5);
    abort();
}

/*Moved the code from SM to the HAL layer*/
McpU16 MCP_HAL_MISC_GetGainValue(McpUint  gain)
{

    /*SEMC_BEGIN(Updating gain steps for log increments) */
    if (gain == 0) {
	 FM_TRACE("getGainValue 0 %d",gain);
        return (McpU16) 0;
    } else {
        /* 29200 (0x7210) is the maximum gain we want
        this corresponds to -6.5 dB this will be compensated
        by the gain values in the analog loop */
        McpU16 maximum_gain = 29200;
        /* 70 is the number of volume steps this implementation
        has at upper level*/
        McpUint volumeSteps = 70;
	McpU16 gain_t ;
	gain_t  =(McpU16)floor(maximum_gain *pow(10, (45.0*gain/((double)volumeSteps) - 45.0)/20.0));
	FM_TRACE("MCP_HAL_MISC_GetGainValue log %d",gain_t);

        return (McpU16)floor(maximum_gain *
                pow(10, (45.0*gain/((double)volumeSteps) - 45.0)/20.0));
    }


    /*SEMC_END(Updating gain steps for log increments) */
}

