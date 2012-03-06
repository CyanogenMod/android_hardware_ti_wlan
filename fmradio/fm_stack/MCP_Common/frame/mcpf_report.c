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

/** \file report.c
 *  \brief report module implementation
 *
 *  \see report.h
 */

#include "mcpf_report.h"
#include "mcp_hal_os.h"
#include "mcp_defs.h"

/************************************************************************
 *                        report_create                                 *
 ************************************************************************/
handle_t report_Create (handle_t hOs)
{
    TReport *pReport;

    pReport = os_memoryAlloc(hOs, sizeof(TReport));
    if (!pReport)
    {
        return NULL;
    }

    pReport->hOs = hOs;

    os_memoryZero(hOs, pReport->aSeverityTable, sizeof(pReport->aSeverityTable));
    os_memoryZero(hOs, pReport->aModuleTable, sizeof(pReport->aModuleTable));

    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[NAVC_MODULE_LOG]), "NAVC  ",       sizeof("NAVC  "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[BT_MODULE_LOG]),   "BT       ",    sizeof("BT       "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[FM_MODULE_LOG]),   "FM  ",         sizeof("FM  "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[MCPF_MODULE_LOG]), "MCPF      ",   sizeof("MCPF      "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[TRANS_MODULE_LOG]), "TRANS   ",    sizeof("TRANS   "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[QUEUE_MODULE_LOG]), "QUEUE      ", sizeof("QUEUE      "));
    os_memoryCopy(hOs, (void *)(pReport->aModuleDesc[REPORT_MODULE_LOG]), "REPORT     ", sizeof("REPORT     "));
        
    return (handle_t)pReport;
}

/************************************************************************
 *                        report_SetDefaults                            *
 ************************************************************************/
mcpf_res_t report_SetDefaults (handle_t hReport, TReportInitParams *pInitParams)
{
    report_SetReportModuleTable (hReport, (McpU8 *)pInitParams->aModuleTable);
    report_SetReportSeverityTable (hReport, (McpU8 *)pInitParams->aSeverityTable);
    
    return E_COMPLETE;
}

/************************************************************************
 *                        report_unLoad                                 *
 ************************************************************************/
mcpf_res_t report_Unload (handle_t hReport)
{
    TReport *pReport = (TReport *)hReport;

    os_memoryFree(pReport->hOs, pReport, sizeof(TReport));
    return E_COMPLETE;
}


mcpf_res_t report_SetReportModule(handle_t hReport, McpU8 module_index)
{
    ((TReport *)hReport)->aModuleTable[module_index] = 1;

    return E_COMPLETE;
}


mcpf_res_t report_ClearReportModule(handle_t hReport, McpU8 module_index)
{
    ((TReport *)hReport)->aModuleTable[module_index] = 0;

    return E_COMPLETE;
}


mcpf_res_t report_GetReportModuleTable(handle_t hReport, McpU8 *pModules)
{
    McpU8 index;

    os_memoryCopy(((TReport *)hReport)->hOs, 
                  (void *)pModules, 
                  (void *)(((TReport *)hReport)->aModuleTable), 
                  sizeof(((TReport *)hReport)->aModuleTable));

    for (index = 0; index < sizeof(((TReport *)hReport)->aModuleTable); index++)
    {
        pModules[index] += '0';
    }

    return E_COMPLETE;
}


mcpf_res_t report_SetReportModuleTable(handle_t hReport, McpU8 *pModules)
{
    McpU8 index;

    for (index = 0; index < sizeof(((TReport *)hReport)->aModuleTable); index++)
    {
        pModules[index] -= '0';
    }

    os_memoryCopy(((TReport *)hReport)->hOs, 
                  (void *)(((TReport *)hReport)->aModuleTable), 
                  (void *)pModules, 
                  sizeof(((TReport *)hReport)->aModuleTable));

    return E_COMPLETE;
}


mcpf_res_t report_GetReportSeverityTable(handle_t hReport, McpU8 *pSeverities)
{
    McpU8 index;

    os_memoryCopy (((TReport *)hReport)->hOs, 
                   (void *)pSeverities, 
                   (void *)(((TReport *)hReport)->aSeverityTable), 
                   sizeof(((TReport *)hReport)->aSeverityTable));

    for (index = 0; index < sizeof(((TReport *)hReport)->aSeverityTable); index++)
    {
        pSeverities[index] += '0';
    }

    return E_COMPLETE;
}


mcpf_res_t report_SetReportSeverityTable(handle_t hReport, McpU8 *pSeverities)
{
    McpU8 index;

    for (index = 0; index < sizeof(((TReport *)hReport)->aSeverityTable); index++)
    {
        pSeverities[index] -= '0';
    }

    os_memoryCopy(((TReport *)hReport)->hOs, 
                  (void *)(((TReport *)hReport)->aSeverityTable), 
                  (void *)pSeverities, 
                  sizeof(((TReport *)hReport)->aSeverityTable));

    return E_COMPLETE;
}

/************************************************************************
 *                        hex_to_string                                 *
 ************************************************************************/
mcpf_res_t report_Dump (McpU8 *pBuffer, char *pString, McpU32 size)
{
    McpU32 index;
    McpU8  temp_nibble;

    /* Go over pBuffer and convert it to chars */ 
    for (index = 0; index < size; index++)
    {
        /* First nibble */
        temp_nibble = (McpU8)(pBuffer[index] & 0x0F);
        if (temp_nibble <= 9)
        {
            pString[(index << 1) + 1] = (char)(temp_nibble + '0');
        }
        else
        {
            pString[(index << 1) + 1] = (char)(temp_nibble - 10 + 'A');
        }

        /* Second nibble */
        temp_nibble = (McpU8)((pBuffer[index] & 0xF0) >> 4);
        if (temp_nibble <= 9)
        {
            pString[(index << 1)] = (char)(temp_nibble + '0');
        }
        else
        {
            pString[(index << 1)] = (char)(temp_nibble - 10 + 'A');
        }
    }

    /* Put string terminator */
    pString[(size * 2)] = 0;

    return E_COMPLETE;
}


/* HEX DUMP for BDs !!! Debug code only !!! */
mcpf_res_t report_PrintDump (McpU8 *pData, McpU32 datalen)
{
#ifdef _DEBUG
    McpS32  dbuflen=0;
    McpU32 j;
    char   dbuf[50];
    static const char hexdigits[16] = "0123456789ABCDEF";

    for(j=0; j < datalen;)
    {
        /* Add a byte to the line*/
        dbuf[dbuflen] =  hexdigits[(pData[j] >> 4)&0x0f];
        dbuf[dbuflen+1] = hexdigits[pData[j] & 0x0f];
        dbuf[dbuflen+2] = ' ';
        dbuf[dbuflen+3] = '\0';
        dbuflen += 3;
        j++;
        if((j % 16) == 0)
        {
            /* Dump a line every 16 hex digits*/
            MCPF_OS_REPORT(("%04.4x  %s\n", j-16, dbuf));
            dbuflen = 0;
        }
    }
    /* Flush if something has left in the line*/
    if(dbuflen)
        MCPF_OS_REPORT(("%04.4x  %s\n", j & 0xfff0, dbuf));
#else
    MCP_UNUSED_PARAMETER(pData);
    MCP_UNUSED_PARAMETER(datalen);
#endif
    return E_COMPLETE;
}

