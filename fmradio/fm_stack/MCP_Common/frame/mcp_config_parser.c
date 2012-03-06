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
*   FILE NAME:      mcp_config_parser.c
*
*   BRIEF:          This file defines the implementation of the MCP configuration
*                   parser
*
*   DESCRIPTION:    The MCP configuration parser is a utility to parse
*                   ini files.
*
*   AUTHOR:         Ronen Kalish
*
\*******************************************************************************/

#include "mcp_config_parser.h"
#include "mcp_defs.h"
#include "mcp_hal_string.h"
#include "mcp_utils.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

#define     MCP_CONFIG_PARSE_LINE_LENGTH    50 /* TODO ronen: change this to include section or key lengths */

/* forward declarations */
McpBool _MCP_CONFIG_PARSER_IsEmptyLine (McpU8 *pLine);
McpBool _MCP_CONFIG_PARSER_IsSection (McpU8 *pLine);
McpBool _MCP_CONFIG_PARSER_IsWhiteSpace (McpU8 pChar);
void _MCP_CONFIG_PARSER_GetSectionName (McpU8 *pLine, McpU8 *pName);
void _MCP_CONFIG_PARSER_GetKeyName (McpU8 *pLine, McpU8 *pName);
void _MCP_CONFIG_PARSER_GetKeyValue (McpU8 *pLine, McpU8 *pValue);

McpConfigParserStatus MCP_CONFIG_PARSER_Open (McpUtf8 *pFileName, 
                                              McpU8 *pMemConfig, 
                                              McpConfigParser *pConfigParser)
{
    McpU8                   pLine[ MCP_CONFIG_PARSE_LINE_LENGTH ];
    McpConfigParserStatus   status = MCP_CONFIG_PARSER_STATUS_SUCCESS;
    McpBool                 opStatus = MCP_TRUE;
    McpConfigReader         tConfigReader;


    MCP_FUNC_START ("MCP_CONFIG_PARSER_Open");

    MCP_LOG_INFO (("MCP_CONFIG_PARSER_Open: Attempting to open file %s",
        (NULL == pFileName ? (McpU8 *)"NULL": pFileName)));

    /* open the file or memory location */
    opStatus = MCP_CONFIG_READER_Open (&tConfigReader, pFileName, pMemConfig);
    MCP_VERIFY_ERR ((MCP_TRUE == opStatus),
                    MCP_CONFIG_PARSER_STATUS_FAILURE_FILE_ERROR,
                    ("MCP_CONFIG_PARSER_Open: unable to open configuration storage"));

    /* initialize config parser object data */
    pConfigParser->uKeysNum = 0;
    pConfigParser->uSectionNum = 0;

    do
    {
        /* read next line */
        opStatus = MCP_CONFIG_READER_getNextLine (&tConfigReader, pLine);

        /* if reading succeeded */
        if (MCP_TRUE == opStatus)
        {
            /* if this is an empty line */
            if (MCP_TRUE == _MCP_CONFIG_PARSER_IsEmptyLine (pLine))
            {
                continue;
            }
            /* if this is a new section */
            else if (MCP_TRUE == _MCP_CONFIG_PARSER_IsSection (pLine))
            {
                /* Verify we do not overflow the sections buffer */
                MCP_VERIFY_ERR ((MCP_CONFIG_PARSER_SECTION_NUMBER > pConfigParser->uSectionNum),
                                MCP_CONFIG_PARSER_STATUS_FAILURE_FILE_ERROR,
                                ("MCP_CONFIG_PARSER_Open config file contains too many sections"));
            
                /* start a new section */
                pConfigParser->tSections[ pConfigParser->uSectionNum ].uStartingKeyIndex = 
                    pConfigParser->uKeysNum;
                pConfigParser->tSections[ pConfigParser->uSectionNum ].uLastFoundKeyIndex = 
                    pConfigParser->uKeysNum;
                pConfigParser->tSections[ pConfigParser->uSectionNum ].uNumberOfKeys = 0;
                /* store new section name */
                _MCP_CONFIG_PARSER_GetSectionName (pLine, pConfigParser->tSections[ pConfigParser->uSectionNum ].pSectionName);
                /* increase number of sections */
                pConfigParser->uSectionNum++;


            }
            /* it is a key */
            else
            {
                /* verify a section exists */
                MCP_VERIFY_ERR ((0 < pConfigParser->uSectionNum),
                                MCP_CONFIG_PARSER_STATUS_FAILURE_FILE_ERROR,
                                ("MCP_CONFIG_PARSER_Open: Key not within section boundry"));


                /* Verify we do not overflow the keys buffer */
                MCP_VERIFY_ERR ((MCP_CONFIG_PARSER_KEY_NUMBER > pConfigParser->uKeysNum),
                                MCP_CONFIG_PARSER_STATUS_FAILURE_FILE_ERROR,
                                ("MCP_CONFIG_PARSER_Open config file contains too many keys"));
                                
                /* parse key */
                _MCP_CONFIG_PARSER_GetKeyName (pLine, pConfigParser->pKeyNames[ pConfigParser->uKeysNum ]);
                _MCP_CONFIG_PARSER_GetKeyValue (pLine, pConfigParser->pKeyValues[ pConfigParser->uKeysNum ]);
                /* add key to current section */
                pConfigParser->tSections[ pConfigParser->uSectionNum - 1 ].uNumberOfKeys++;
                /* increase number of keys */
                pConfigParser->uKeysNum++;

                
            }
        } 
    } while (MCP_TRUE == opStatus); /* continue to the next line while reading succeeds */

  	opStatus = MCP_CONFIG_READER_Close(&tConfigReader);
	MCP_VERIFY_ERR ((MCP_TRUE == opStatus),
					MCP_CONFIG_PARSER_STATUS_FAILURE_FILE_ERROR,
					("MCP_CONFIG_PARSER_Open: unable to close configuration storage"));
    MCP_FUNC_END ();

    return status;
}

McpConfigParserStatus MCP_CONFIG_PARSER_GetAsciiKey (McpConfigParser *pConfigParser,
                                                     McpU8 *pSectionName,
                                                     McpU8 *pKeyName,
                                                     McpU8 **pKeyAsciiValue)
{
    McpU32      uSectionIndex, uKeyIndex;

    MCP_FUNC_START ("MCP_CONFIG_PARSER_GetAsciiKey");

    MCP_LOG_INFO (("MCP_CONFIG_PARSER_GetAsciiKey: looking for key %s in section %s",
                   pKeyName, pSectionName));

    /* first find the section */
    uSectionIndex = 0;
    while ((uSectionIndex < pConfigParser->uSectionNum) && 
           (0 != MCP_HAL_STRING_StriCmp ((const char *)pSectionName, 
                                         (const char *)pConfigParser->tSections[ uSectionIndex ].pSectionName)))
    {
        uSectionIndex++;
    }

    /* if section was found */
    if (uSectionIndex < pConfigParser->uSectionNum)
    {
        /* find the key */
        /* start from the last found key, to allow reading more than one key with the same name */
        uKeyIndex = pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex;
        while ((uKeyIndex < (pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex + 
                             pConfigParser->tSections[ uSectionIndex ].uNumberOfKeys)) &&
               (0 != MCP_HAL_STRING_StriCmp ((const char *)pKeyName, 
                                             (const char *)pConfigParser->pKeyNames[ uKeyIndex ])))
        {
            uKeyIndex++;
        }

        /* if key was found */
        if (uKeyIndex < (pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex + 
                         pConfigParser->tSections[ uSectionIndex ].uNumberOfKeys))
        {
            /* keep last key found */
            if ((uKeyIndex + 1) >= (pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex + 
                                    pConfigParser->tSections[ uSectionIndex ].uNumberOfKeys))
            {
                /* last found key was the last key in the section - update to the first key */
                pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex = 
                    pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex;
            }
            else
            {
                /* update to the next key */
                pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex = uKeyIndex + 1;
            }

            /* set key value */
            *pKeyAsciiValue = pConfigParser->pKeyValues[ uKeyIndex ];
            return MCP_CONFIG_PARSER_STATUS_SUCCESS;
        }

        /* now continue from the first key */
        uKeyIndex = pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex;
        while ((uKeyIndex < pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex) &&
               (0 == MCP_HAL_STRING_StriCmp ((const char *)pKeyName, 
                                             (const char *)pConfigParser->pKeyNames[ uKeyIndex ])))
        {
            uKeyIndex++;
        }

        /* if key was found */
        if (uKeyIndex < pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex)
        {
            /* keep last key found */
            if ((uKeyIndex + 1) >= (pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex + 
                                    pConfigParser->tSections[ uSectionIndex ].uNumberOfKeys))
            {
                /* last found key was the last key in the section - update to the first key */
                pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex = 
                    pConfigParser->tSections[ uSectionIndex ].uStartingKeyIndex;
            }
            else
            {
                /* update to the next key */
                pConfigParser->tSections[ uSectionIndex ].uLastFoundKeyIndex = uKeyIndex + 1;
            }

            /* set key value */
            *pKeyAsciiValue = pConfigParser->pKeyValues[ uKeyIndex ];
            return MCP_CONFIG_PARSER_STATUS_SUCCESS;
        }
    }

    MCP_FUNC_END ();

    /* key or section were not found */
    *pKeyAsciiValue = NULL;
    return MCP_CONFIG_PARSER_STATUS_FAILURE_KEY_DOESNT_EXIST;
}

McpConfigParserStatus MCP_CONFIG_PARSER_GetIntegerKey (McpConfigParser *pConfigParser,
                                                       McpU8 *pSectionName,
                                                       McpU8 *pKeyName,
                                                       McpS32 *pKeyIntValue)
{
    McpU8       *pKeyAsciiValue;

    MCP_FUNC_START ("MCP_CONFIG_PARSER_GetIntegerKey");

    MCP_LOG_INFO (("MCP_CONFIG_PARSER_GetIntegerKey: looking for key %s in section %s",
                   pKeyName, pSectionName));

    /* search for the ASCII value of this key */
    if (MCP_CONFIG_PARSER_STATUS_SUCCESS == 
        MCP_CONFIG_PARSER_GetAsciiKey (pConfigParser, pSectionName, pKeyName, &pKeyAsciiValue))
    {
        /* key was found */
        *pKeyIntValue = MCP_UTILS_AtoU32((const char *)pKeyAsciiValue);
        return MCP_CONFIG_PARSER_STATUS_SUCCESS;
    }

    /* key or section were not found */
    *pKeyIntValue = 0;
    return MCP_CONFIG_PARSER_STATUS_FAILURE_KEY_DOESNT_EXIST;
}

McpBool _MCP_CONFIG_PARSER_IsEmptyLine (McpU8 *pLine)
{
    McpU32      uIndex, uLen;

    uLen = MCP_HAL_STRING_StrLen ((const char *)pLine);

    /* trim starting white spaces */
    uIndex = 0;
    while ((uIndex < uLen) && (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uIndex ])))
    {
        uIndex++;
    }

    if (uIndex == uLen)
    {
        return MCP_TRUE;
    }
    else
    {
        return MCP_FALSE;
    }
}

McpBool _MCP_CONFIG_PARSER_IsSection (McpU8 *pLine)
{
    McpU32      uIndex, uLen;

    uLen = MCP_HAL_STRING_StrLen ((const char *)pLine);

    /* trim starting white spaces */
    uIndex = 0;
    while ((uIndex < uLen) && (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uIndex ])))
    {
        uIndex++;
    }

    /* check if first character is an opening square bracket */
    if ('[' != pLine[ uIndex ])
    {
        return MCP_FALSE;
    }

    /* trim ending white spaces */
    uIndex = uLen - 1;
    while ((uIndex > 0) && (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uIndex ])))
    {
        uIndex--;
    }

    /* check if last character is a closing square bracket */
    if (']' != pLine[ uIndex ])
    {
        return MCP_FALSE;
    }

    return MCP_TRUE;
}

McpBool _MCP_CONFIG_PARSER_IsWhiteSpace (McpU8 pChar)
{
    /* spaces and tabs only count as whitespace */
    if ((' ' == pChar) || ('\t' == pChar))
    {
        return MCP_TRUE;
    }

    return MCP_FALSE;
}

void _MCP_CONFIG_PARSER_GetSectionName (McpU8* pLine, McpU8* pName)
{
    McpU32 uStartIndex, uCloseIndex, uIndex;

    /* find starting index: trim heading white spaces */
    uStartIndex = 0;
    uCloseIndex = MCP_HAL_STRING_StrLen ((const char *)pLine) - 1;
    while ((uStartIndex <= uCloseIndex) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uStartIndex ])))
    {
        uStartIndex++;
    }

    /* than opening square bracket */
    uStartIndex++;

    /* and again trim white space */
    while ((uStartIndex <= uCloseIndex) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uStartIndex ])))
    {
        uStartIndex++;
    }

    /* find finishing index: trim trailing white spaces */
    while ((uCloseIndex > 0) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uCloseIndex ])))
    {
        uCloseIndex--;
    }

    /* than closing square bracket */
    uCloseIndex--;

    /* and again trim white space */
    while ((uCloseIndex > 0) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uCloseIndex ])))
    {
        uCloseIndex--;
    }

    /* now copy from starting index to finishing index */
    uIndex = uStartIndex;
    while (uIndex <= uCloseIndex)
    {
        pName[ uIndex - uStartIndex ] = pLine[ uIndex ];
        uIndex++;
    }
    pName[ uIndex - uStartIndex ] = '\0';
}

void _MCP_CONFIG_PARSER_GetKeyName (McpU8 *pLine, McpU8 *pName)
{
    McpU32 uStartIndex, uCloseIndex, uIndex;

    /* find starting index: trim heading white spaces */
    uStartIndex = 0;
    uCloseIndex = MCP_HAL_STRING_StrLen ((const char *)pLine) - 1;
    while ((uStartIndex <= uCloseIndex) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uStartIndex ])))
    {
        uStartIndex++;
    }

    /* find finishing index: find equation sign */
    while ((uCloseIndex > 0) && ('=' != pLine[ uCloseIndex ]))
    {
        uCloseIndex--;
    }
    uCloseIndex--;

    /* and than trim trailing white spaces */
    while ((uCloseIndex > 0) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uCloseIndex ])))
    {
        uCloseIndex--;
    }

    /* now copy key name */
    uIndex = uStartIndex;
    while (uIndex <= uCloseIndex)
    {
        pName[ uIndex - uStartIndex ] = pLine[ uIndex ];
        uIndex++;
    }
    pName[ uIndex - uStartIndex ] = '\0';
}

void _MCP_CONFIG_PARSER_GetKeyValue (McpU8 *pLine, McpU8 *pValue)
{
    McpU32 uStartIndex, uCloseIndex, uIndex;

    uStartIndex = 0;
    uCloseIndex = MCP_HAL_STRING_StrLen ((const char *)pLine) - 1;

    /* find finishing index: find equation sign */
    while ((uStartIndex <= uCloseIndex) && ('=' != pLine[ uStartIndex ]))
    {
        uStartIndex++;
    }
    uStartIndex++;

    /* and than trim leading white spaces */
    while ((uStartIndex <= uCloseIndex) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uStartIndex ])))
    {
        uStartIndex++;
    }

    /* find finishing index: trim trailing white spaces */
    while ((uCloseIndex > 0) && 
           (MCP_TRUE == _MCP_CONFIG_PARSER_IsWhiteSpace (pLine[ uCloseIndex ])))
    {
        uCloseIndex--;
    }

    /* now copy key name */
    uIndex = uStartIndex;
    while (uIndex <= uCloseIndex)
    {
        pValue[ uIndex - uStartIndex ] = pLine[ uIndex ];
        uIndex++;
    }
    pValue[ uIndex - uStartIndex ] = '\0';
}

