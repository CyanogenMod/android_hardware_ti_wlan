/*
 * version.c
 *
 * Copyright(c) 1998 - 2010 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/** \file report.c
 *  \brief report module implementation
 *
 *  \see version.h
 */
#include "version.h"
#include "osApi.h"

#define MAX_NUMBER_LENGTH_IN_VERSION 60

#define SW_FW_API_VERSION_INDEX 2
#define SW_INI_API_VERSION_INDEX 3

TI_UINT16 version_atoi(TI_UINT8* str);

TI_UINT16 version_findSubVersion(TI_UINT8 *str, TI_UINT16 len, TI_UINT8 delim, TI_UINT8 indexToFind)
{
    TI_UINT16 strPos = 0;
    TI_UINT16 currStringInArrayPos = 0;
    TI_UINT8 tempBuff[MAX_NUMBER_LENGTH_IN_VERSION] = {0};

    TI_UINT8 currentDelimCount = 0;

    while ( (strPos < len) && (currentDelimCount <= indexToFind))
    {
        if (str[strPos] == delim)
        {
            currentDelimCount++;
        }
        else
        {
            if (currentDelimCount == indexToFind)
            {
                tempBuff[currStringInArrayPos] = str[strPos];
                currStringInArrayPos++;
            }
        }
        strPos++;
    }

    return version_atoi(tempBuff);
}

TI_UINT16 version_strlen(TI_UINT8* str)
{
    TI_UINT16 ret = 0;

    while (str[ret])
    {
        ret++;
    }

    return ret;
}

TI_UINT16 version_atoi(TI_UINT8* str)
{
    TI_UINT16 ret = 0;
    TI_INT16 i = 0;
    TI_UINT16 j = 1;

    for (i = 0; str[i] ; i++, j*=10)
    {
        ret += (str[i]-'0')*j;
    }

    return ret;
}

TI_BOOL version_fwDriverMatch(TI_UINT8* fwVersion, TI_UINT16* pFwVersionOutput)
{
    TI_UINT16   fwVersionInt = 0;
    TI_UINT16   drvVersionInt = 0;

    fwVersionInt  = version_findSubVersion(fwVersion, version_strlen(fwVersion), '.', SW_FW_API_VERSION_INDEX);
    drvVersionInt = version_findSubVersion(SW_VERSION_STR, version_strlen(SW_VERSION_STR), '.', SW_FW_API_VERSION_INDEX);

    *pFwVersionOutput = fwVersionInt;

    return (fwVersionInt == drvVersionInt ? TI_TRUE : TI_FALSE);
}

TI_BOOL version_IniDriverMatch(TI_UINT16 iniVersion)
{
    TI_UINT16   drvVersionInt =  version_findSubVersion(SW_VERSION_STR, version_strlen(SW_VERSION_STR), '.', SW_INI_API_VERSION_INDEX);

    return (drvVersionInt == iniVersion ? TI_TRUE : TI_FALSE);
}
