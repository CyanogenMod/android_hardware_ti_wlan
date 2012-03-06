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
/******************************************************************************\
*
*   FILE NAME:      mcp_utils.c
*
*   DESCRIPTION:    This file implements generic utilities.
*
*   AUTHOR:         Vladimir Abram
*
\******************************************************************************/
#include "mcp_hal_types.h"
#include "mcp_utils.h"


/*******************************************************************************
 *
 * Public Functions
 *
 ******************************************************************************/

McpU32 MCP_UTILS_AtoU32(const char *string)
{
	int sign = 1;
	int counter = 0;
	int number = 0;
	int tmp;
	int length;
	const char  *cp = string;

	if (0 == string)
    {
		return 0;
    }

	if ('-' == string[0])
    {
        sign = -1;
        counter = 1;
    }

	while (*cp != 0) cp++;

	length = (McpU32)(cp - string);

	for(; counter<=length; counter++)
	{
		if ((string[counter] >= '0') && (string[counter] <= '9'))
		{
			tmp = (string[counter] - '0');		
			number = number * 10 + tmp;   
		}
	}

	number *= sign;

	return (McpU32)(number);
}

