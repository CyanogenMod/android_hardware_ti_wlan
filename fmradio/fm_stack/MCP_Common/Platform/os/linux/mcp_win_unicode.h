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
*   FILE NAME:      mcp_win_unicode.h
*
*   BRIEF:          This file defines all definitions and prototypes related to.
*                   the implementation of unicode support the Windows port of the MCP HAL.
*
*   DESCRIPTION:    General
*
*                   A big advantage of the UTF-8 encoding is that it is 100%
*                   compatible with ASCII strings with characters in the range
*                   of 0x00 to 0x7F.
*
*                   When you make use of other character encodings then ASCII or
*                   UTF-8 in your application then a conversion is needed
*                   from and to UTF-8.
*                   A popular encoding scheme for Asian countries is UTF-16.
*
*                   This module does provide conversion routines for UTF-16 to
*                   UTF-8 and the other way around:
*
*                   -	MCP_WIN_Utf16ToUtf8: convert from UTF-16 to UTF-8
*                   -	MCP_WIN_Utf8ToUtf16: convert from UTF-8 to UTF-16
*
*                   UTF-16 encoding does store characters in 2-byte entities and
*                   is therefore endian sensitive. A processor can order its
*                   bytes in memory in 2 different ways:
*                   -	big endian:
*                     A word is stored as MSB,LSB (increasing memory address)
*                     Also called network order.
*                   -	little endian:
*                     A word is stored as LSB,MSB (increasing memory address)
*
*                   The above mentioned conversion routines are endian independent.
*                   It will convert to the native endianity (big- or little-endian)
*                   of the local processor automatically.
*                   This means that the UTF-16 strings in these conversion routines
*                   are stored in native endian format.
*                   When it should be stored in a specific endian format,
*                   the other 2 conversion routines can be used:
*
*                   -	MCP_WIN_Utf16ToUtf8Endian: convert from UTF-16 to UTF-8
*                   -	MCP_WIN_Utf8ToUtf16Endian: convert from UTF-8 to UTF-16
*
*                   These 2 routines do convert to a specific endian format
*                   for UTF-16.
*
*                   A detailed description of the usage of these 4 routines is
*                   found at the prototype of each function in this file.
*                   
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __MCP_WIN_UNICODE_H
#define __MCP_WIN_UNICODE_H

/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "mcp_hal_types.h" 

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/

/* Three types of endianity domains: big, little or native (big or little)*/
typedef enum _McpWinEndianity
{
	btlBigEndian,    /* network order */
	btlLittleEndian,
	btlNativeEndian  /* local endianity, being big or little. */
} McpWinEndianity;

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/
 
/*-------------------------------------------------------------------------------
 * MCP_WIN_Utf16ToUtf8Endian()
 *
 * Brief:  
 *     Converts UTF-16 formatted text to UTF-8 format.
 *
 * Description:
 *     Converts UTF-16 text to UTF-8 format. The endianity of the source text
 *     can be defined.
 *
 *     For use in the Application, the endianity of the source is always native
 *     (local endian format). For simplicity (hiding the endianity parameter) in
 *     these cases, another function prototype (macro) is defined to hide this
 *     parameter. See MCP_WIN_Utf16ToUtf8
 *
 * Type:
 *		 Synchronous
 *
 * Parameters:
 *     tgtText [out] - pointer to the target buffer that will be filled with the UTF-8 data
 *
 *     tgtSize [in] - size of the target buffer in quantities of bytes
 *
 *     srcText [in] - pointer to the UTF16 source text. The UTF-16 is formatted according
 *         the native endian format (endianity of the local system, either big or little).
 *
 *     srcLen [in] - num of words (McpU16 entities) that are needed to be converted from the source,
 *         including the 0-termination (McpU16 word).
 *
 *     endianity [in] - Endianity of the 'srcText'.
 *         The word (2 bytes) can be written in the following formats:
 *         - btlBigEndian: first byte = MSB (network order)
 *         - btlLittleEndian: first byte  = LSB
 *         - btlNativeEndian: the endian format of the local system (big or little)
 *
 * Returns:
 *     Number of bytes filled in the 'tgtText' (including the 0-byte).
 */
McpU16 MCP_WIN_Utf16ToUtf8Endian(McpUtf8 *tgtText,
                          McpU16 tgtSize,
                          McpUtf16 *srcText,
                          McpU16 srcLen,
                          McpWinEndianity endianity);

/*-------------------------------------------------------------------------------
 * MCP_WIN_Utf8ToUtf16Endian()
 *
 * Brief:
 *     Converts UTF-8 formatted text to UTF-16 format.
 *
 * Description:
 *     Converts UTF-8 text to UTF-16 format. The endianity of the target
 *     can be defined.
 *
 *     For use in the Application, the endianity of the target is always native
 *     (local endian format). For simplicity (hiding the endianity parameter) in
 *     these cases, another function prototype (macro) is defined to hide this
 *     parameter. See MCP_WIN_Utf8ToUtf16
 *
 * Type:
 *		 Synchronous
 *
 * Parameters:
 *     tgtText [out] - pointer to the target buffer that will be filled with
 *         the UTF-16 data
 *
 *     tgtSize [in] - size of the target buffer in quantities of words.
 *         (McpU16 entities)
 *
 *     srcText [in] - pointer to the UTF-8 source text, assumes the text
 *         is 0-terminated
 *
 *     endianity [in] - Endianity of the 'tgtText'.
 *         The word (2 bytes) can be written in the following formats:
 *         - btlBigEndian: first byte = MSB (network order)
 *         - btlLittleEndian: first byte  = LSB
 *         - btlNativeEndian: the endian format of the local system (big or little)
 *
 * Returns:
 *     How many bytes were used in the target buffer, including the word used
 *     for 0 termination.
 *     This number should always be even. If it is odd, it is an error.
 */
McpU16 MCP_WIN_Utf8ToUtf16Endian(McpUtf16 *tgtText, 
                          McpU16 tgtSize, 
                          const McpUtf8*srcText,
                          McpWinEndianity endianity);

/*-------------------------------------------------------------------------------
 * MCP_WIN_Utf8ToUtf16()
 *
 * Brief:
 *     Converts UTF-8 formatted text to UTF-16 format (native)
 *
 * Description:
 *     Converts UTF-8 text to UTF-16 format (native format)
 *     It is just a macro around the MCP_WIN_Utf8ToUtf16Endian, hiding the
 *     endianity parameter. This function is typically use by the APP developer
 *     that needs conversions from UTF-8 to UTF-16 in its native format.
 *
 * Type:
 *		 Synchronous
 *
 * Parameters:
 *     tgtText [out] - pointer to the target buffer that will be filled with
 *         the UTF-16 data (in native endian format).
 *
 *     tgtSize [in] - size of the target buffer in quantities of words.
 *         (McpU16 entities)
 *
 *     srcText [in] - pointer to the UTF-8 source text, assumes the text
 *         is 0-terminated
 *
 * Returns:
 *     How many bytes were used in the target buffer, including the word used
 *     for 0 termination.
 *     This number should always be even. If it is odd, it is an error.
 */
#define MCP_WIN_Utf8ToUtf16(tgtText,tgtSize,srcText) MCP_WIN_Utf8ToUtf16Endian(tgtText,tgtSize,srcText,btlNativeEndian)

/*-------------------------------------------------------------------------------
 * MCP_WIN_Utf16ToUtf8()
 *
 * Brief:
 *     Converts UTF-16 formatted text (native) to UTF-8 format
 *
 * Description:
 *     Converts UTF-16 text (native format) to UTF-8
 *     It is just a macro around the MCP_WIN_Utf16ToUtf8Endian, hiding the
 *     endianity parameter. This function is typically use by the APP developer
 *     that needs conversions from UTF-16 in its native format to UTF-8.
 *
 * Type:
 *		 Synchronous
 *
 * Parameters:
 *     tgtText [out] - pointer to the target buffer that will be filled with the UTF-8 data
 *
 *     tgtSize [in] - size of the target buffer in quantities of bytes
 *
 *     srcText [in] - pointer to the UTF16 source text. The UTF-16 is formatted according
 *         the native endian format (endianity of the local system, either big or little).
 *
 *     srcLen [in] - num of words (McpU16 entities) that are needed to be converted from the source,
 *         including the 0-termination (McpU16 word).
 *
 * Returns:
 *     Number of bytes filled in the 'tgtText' (including the 0-byte).
 */
#define MCP_WIN_Utf16ToUtf8(tgtText,tgtSize,srcText,srcLen) MCP_WIN_Utf16ToUtf8Endian(tgtText,tgtSize,srcText,srcLen,btlNativeEndian)

#endif /* __MCP_WIN_UNICODE_H */
