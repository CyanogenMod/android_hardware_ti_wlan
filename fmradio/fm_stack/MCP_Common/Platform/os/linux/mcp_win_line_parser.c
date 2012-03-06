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
*   FILE NAME:   mcp_win_line_parser.c
*
*   DESCRIPTION: This file contains unicode specific routines.
*
*   AUTHOR:      Udi Ron
*
\*******************************************************************************/


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "mcp_win_line_parser.h"
#include "mcp_hal_string.h"

#define MCP_WIN_LINE_PARSER_DELIMS	" \n"

typedef enum
{
	_MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE,
	_MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM,
	_MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL,
	_MCP_WIN_LINE_PARSER_CHAR_TYPE_END
} _lineParserCharType;

typedef enum
{
	_MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER,
	_MCP_WIN_LINE_PARSER_STATE_IN_TOKEN,
	_MCP_WIN_LINE_PARSER_STATE_START_QUOTED_TOKEN,
	_MCP_WIN_LINE_PARSER_STATE_IN_QUOTED_TOKEN,
	_MCP_WIN_LINE_PARSER_STATE_END_QUOTED_TOKEN
} _lineParserParsingState;

static McpU32 	_lineParserNextArgIndex = 0;
static McpU32 	_lineParserNumOfArgs = 0;
char 	*_lineParserArgs[MCP_WIN_LINE_PARSER_MAX_NUM_OF_ARGUMENTS];
char	_lineParserLine[MCP_WIN_LINE_PARSER_MAX_LINE_LEN + 1];
static McpBool	parsingFailed = MCP_FALSE;

_lineParserCharType GetCharType(char c, const char* delimiters)
{
	if (c == (char)34)
	{
		return _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE;
	}
	else if (strchr(delimiters, c) != NULL)
	{
		return _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM;
	}
	else if (c == '\0')
	{
		return _MCP_WIN_LINE_PARSER_CHAR_TYPE_END;
	}
	else
	{
		return _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL;
	}
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_ParseLine(McpU8 *line, const char* delimiters)
{
	char *pos = NULL;
	char *lineEnd;
	_lineParserParsingState state;
	_lineParserCharType	charType;

	_lineParserNextArgIndex = 0;
	_lineParserNumOfArgs = 0;
	parsingFailed = MCP_TRUE;

	MCP_HAL_STRING_StrCpy(_lineParserLine, (char*)line);

	if (MCP_HAL_STRING_StrLen(_lineParserLine)== 0)
	{
		parsingFailed = MCP_FALSE;
		return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
	}

	pos = _lineParserLine;
	lineEnd = _lineParserLine + MCP_HAL_STRING_StrLen(_lineParserLine);

	state = _MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER;

	while (pos <= lineEnd)
	{
		charType = GetCharType(*pos, delimiters);

		switch (state)
		{
		case _MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER:

			switch (charType)
			{
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE:

				state = _MCP_WIN_LINE_PARSER_STATE_START_QUOTED_TOKEN;

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM:

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL:
				_lineParserArgs[_lineParserNumOfArgs] = pos;
				++_lineParserNumOfArgs;
				state = _MCP_WIN_LINE_PARSER_STATE_IN_TOKEN;
				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_END:

				break;
			};
			
			break;

		case _MCP_WIN_LINE_PARSER_STATE_START_QUOTED_TOKEN:

			switch (charType)
			{
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE:
				_lineParserArgs[_lineParserNumOfArgs] = pos;
				++_lineParserNumOfArgs;
				state = _MCP_WIN_LINE_PARSER_STATE_END_QUOTED_TOKEN;

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM:
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL:
				_lineParserArgs[_lineParserNumOfArgs] = pos;
				++_lineParserNumOfArgs;
				state = _MCP_WIN_LINE_PARSER_STATE_IN_QUOTED_TOKEN;

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_END:
				return MCP_WIN_LINE_PARSER_STATUS_FAILED;

			};
			
			break;

		case _MCP_WIN_LINE_PARSER_STATE_IN_TOKEN:

			switch (charType)
			{
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE:
				return MCP_WIN_LINE_PARSER_STATUS_FAILED;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM:
				*pos = '\0';
				state = _MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER;

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL:

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_END:
				state = _MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER;

				break;
			};
			
			break;

		case _MCP_WIN_LINE_PARSER_STATE_IN_QUOTED_TOKEN:

			switch (charType)
			{
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE:
				state = _MCP_WIN_LINE_PARSER_STATE_END_QUOTED_TOKEN;

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM:

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL:

				break;

			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_END:

				return MCP_WIN_LINE_PARSER_STATUS_FAILED;

			};
			
			break;

		case _MCP_WIN_LINE_PARSER_STATE_END_QUOTED_TOKEN:

			switch (charType)
			{
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_QUOTE:
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_NORMAL:

				return MCP_WIN_LINE_PARSER_STATUS_FAILED;


			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_DELIM:
			case _MCP_WIN_LINE_PARSER_CHAR_TYPE_END:

				*(pos-1) = '\0';
				state = _MCP_WIN_LINE_PARSER_STATE_IN_DELIMITER;

				break;
			};
			
			break;
		};

		++pos;
	}

	parsingFailed = MCP_FALSE;

	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

McpU32 MCP_WIN_LINE_PARSER_GetNumOfArgs(void)
{
	return _lineParserNumOfArgs;
}

McpBool MCP_WIN_LINE_PARSER_AreThereMoreArgs(void)
{
	if ((parsingFailed == MCP_TRUE) || (_lineParserNextArgIndex >= _lineParserNumOfArgs))
	{
		return MCP_FALSE;
	}
	else
	{
		return MCP_TRUE;
	}
}

void MCP_WIN_LINE_PARSER_ToLower(McpU8 *str)
{
	McpU32 i; 
	size_t len = MCP_HAL_STRING_StrLen((char*)str);

	for (i = 0; i < len; ++i)
	{
		str[i] = (McpU8)tolower(str[i]);
	}
}

void MCP_WIN_LINE_PARSER_ToUpper(McpU8 *str)
{
	McpU32 i;
	size_t len = MCP_HAL_STRING_StrLen((char*)str);

	for (i = 0; i < len; ++i)
	{
		str[i] = (McpU8)toupper(str[i]);
	}
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextChar(McpU8 *c)
{
	char tempStr[200];
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextStr((McpU8*)tempStr, 200);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}
	else
	{
		if (MCP_HAL_STRING_StrLen(tempStr) > 1)
		{
			return MCP_WIN_LINE_PARSER_STATUS_ARGUMENT_TOO_LONG;
		}
		else
		{
			*c = tempStr[0];

			return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
		}
	}
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextStr(McpU8 *str, McpU8 len)
{
	if (parsingFailed == MCP_TRUE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	if (MCP_WIN_LINE_PARSER_AreThereMoreArgs() == MCP_FALSE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_NO_MORE_ARGUMENTS;
	}
			
	if (MCP_HAL_STRING_StrLen(_lineParserArgs[_lineParserNextArgIndex]) > len)
	{
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_ARGUMENT_TOO_LONG;
	}

	MCP_HAL_STRING_StrCpy((char*)str, _lineParserArgs[_lineParserNextArgIndex]);

	++_lineParserNextArgIndex;

	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextU8(McpU8 *value, McpBool hex)
{
	McpU32	tempValue = 0;
	
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextU32(&tempValue, hex);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}

	if (tempValue > 0xFF)
	{
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	*value = (McpU8)tempValue;
	
	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextU16(McpU16 *value, McpBool hex)
{
	McpU32	tempValue = 0;
	
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextU32(&tempValue, hex);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}

	if (tempValue > 0xFFFF)
	{
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	*value = (McpU16)tempValue;
	
	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextU32(McpU32 *value, McpBool hex)
{
	int 		num;
	McpS32		signedValue = 0;

	if (parsingFailed ==MCP_TRUE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	if (MCP_WIN_LINE_PARSER_AreThereMoreArgs() == MCP_FALSE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_NO_MORE_ARGUMENTS;
	}

	if (hex == MCP_TRUE)
	{
		num = sscanf(_lineParserArgs[_lineParserNextArgIndex], "%x", &signedValue);
	}
	else
	{
		num = sscanf(_lineParserArgs[_lineParserNextArgIndex], "%d", &signedValue);
	}
	

	if ((num == 0) || (num == EOF))
	{	
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}
	else
	{
		if (signedValue < 0)
		{
			parsingFailed = MCP_TRUE;
			return MCP_WIN_LINE_PARSER_STATUS_FAILED;

		}

		*value = (McpU32)signedValue;
		++_lineParserNextArgIndex;
				
		return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
	}
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextS8(McpS8 *value)
{
	McpS32	tempValue = 0;
	
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextS32(&tempValue);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}

	if ((McpU32)tempValue > 0xFF)
	{
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	*value = (McpS8)tempValue;
	
	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextS16(McpS16 *value)
{
	McpS32	tempValue = 0;
	
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextS32(&tempValue);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}

	if ((McpU32)tempValue > 0xFFFF)
	{
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	*value = (McpS16)tempValue;
	
	return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextS32(McpS32 *value)
{
	int 		num;

	if (parsingFailed ==MCP_TRUE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}

	if (MCP_WIN_LINE_PARSER_AreThereMoreArgs() == MCP_FALSE)
	{
		return MCP_WIN_LINE_PARSER_STATUS_NO_MORE_ARGUMENTS;
	}
	
	if (_lineParserArgs[_lineParserNextArgIndex][0] != '-')
	{
		num = sscanf(_lineParserArgs[_lineParserNextArgIndex], "%d", value);
	}
	else
	{
		num = sscanf(_lineParserArgs[_lineParserNextArgIndex+1], "%d", value);
	}

	if ((num == 0) || (num == EOF))
	{	
		parsingFailed = MCP_TRUE;
		return MCP_WIN_LINE_PARSER_STATUS_FAILED;
	}
	else
	{
		++_lineParserNextArgIndex;
				
		return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
	}
}

MCP_WIN_LINE_PARSER_STATUS MCP_WIN_LINE_PARSER_GetNextBool(McpBool *value)
{
	char tempStr[200];
	MCP_WIN_LINE_PARSER_STATUS status = MCP_WIN_LINE_PARSER_GetNextStr((McpU8*)tempStr, 200);

	if (status != MCP_WIN_LINE_PARSER_STATUS_SUCCESS)
	{
		return status;
	}
	else
	{
		MCP_WIN_LINE_PARSER_ToLower((McpU8*)tempStr);

		if (MCP_HAL_STRING_StrCmp(tempStr, "true") == 0)
		{
			*value = MCP_TRUE;
			return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
		}
		else if (MCP_HAL_STRING_StrCmp(tempStr, "false") == 0)
		{
			*value = MCP_FALSE;
			return MCP_WIN_LINE_PARSER_STATUS_SUCCESS;
		}
		else
		{
			parsingFailed = MCP_TRUE;
			return MCP_WIN_LINE_PARSER_STATUS_FAILED;
		}
	}
}

