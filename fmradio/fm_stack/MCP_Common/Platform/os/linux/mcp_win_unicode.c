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
*   FILE NAME:   mcp_win_unicode.c
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
#include "mcp_win_unicode.h"
#include "mcp_hal_string.h"
#include "mcp_endian.h"

/******************************************************************************** 
 *
 * Constants 
 *
 *******************************************************************************/
static const int mcpWin_halfShift = 10; /* used for shifting by 10 bits */

static const McpU32 mcpWin_halfBase = 0x0010000UL;
static const McpU32 mcpWin_halfMask = 0x3FFUL;

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char mcpWin_trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const McpU32 mcpWin_offsetsFromUTF8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                                       0x03C82080UL, 0xFA082080UL, 0x82082080UL};

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const McpUtf8 mcpWin_firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/
typedef enum _McpWinfirstByteMark
{
	conversionOK, 		/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted,	/* insuff. room in target for conversion */
	sourceIllegal		  /* source sequence is illegal/malformed */
} McpWinfirstByteMark;

typedef enum _McpWinConversionFlags
{
	strictConversion = 0,
	lenientConversion
} McpWinConversionFlags;

/******************************************************************************** 
 *
 * Macros
 *
 *******************************************************************************/

/* Write a word native McpU16 into the target buffer according to the endianity directive:     */
/*   endianity = native: no translation necessary.                                          */
/*   endianity = big-endian: force to big-endian (network order) storage. First byte = MSB. */
/*   endianity = little-endian: force to little-endian storage. First byte = LSB.           */
/**/
#define MCP_WIN_WRITE_UTF16(endianity, target, nativeWord)            \
  if(endianity==btlNativeEndian) *target=nativeWord;             \
  else if(endianity==btlBigEndian) MCP_ENDIAN_HostToBE16(nativeWord, (McpU8*)target); \
  else MCP_ENDIAN_HostToLE16(nativeWord, (McpU8*)target);

/* Read a word according to the endianity directive into the source in native format:       */
/*   endianity = native: no translation necessary.                                          */
/*   endianity = big-endian: force to big-endian (network order) reading. First byte = MSB. */
/*   endianity = little-endian: force to little-endian reading. First byte = LSB.           */
/**/
#define MCP_WIN_READ_UTF16(endianity, source, nativeWord)            \
  if(endianity==btlNativeEndian) *nativeWord=*source;             \
  else if(endianity==btlBigEndian) *nativeWord=MCP_ENDIAN_BEtoHost16((McpU8*)source); \
  else *nativeWord=MCP_ENDIAN_LEtoHost16((McpU8*)source);
  
/* Some fundamental constants */
#define MCP_WIN_UNI_REPLACEMENT_CHAR ((McpU32) 0x0000FFFD)
#define MCP_WIN_UNI_MAX_BMP ((McpU32) 0x0000FFFF)
#define MCP_WIN_UNI_MAX_UTF16 ((McpU32) 0x0010FFFF)
#define MCP_WIN_UNI_MAX_UTF32 ((McpU32) 0x7FFFFFFF)
#define MCP_WIN_UNI_MAX_LEGAL_UTF32 ((McpU32)0x0010FFFF)

#define MCP_WIN_UNI_SUR_HIGH_START  ((McpU32) 0xD800)
#define MCP_WIN_UNI_SUR_HIGH_END     ((McpU32) 0xDBFF)
#define MCP_WIN_UNI_SUR_LOW_START    ((McpU32) 0xDC00)
#define MCP_WIN_UNI_SUR_LOW_END      ((McpU32) 0xDFFF)

/********************************************************************************
 *
 * Data Structures
 *
 *******************************************************************************/

/********************************************************************************
 *
 * Globals
 *
 *******************************************************************************/

/********************************************************************************
 *
 * Internal function prototypes
 *
 *******************************************************************************/

static McpWinfirstByteMark _MCP_WIN_ConvertUTF8toUTF16(const McpUtf8 **sourceStart,
                                           const McpUtf8 *sourceEnd, 
                                           McpUtf16 **targetStart,
                                           McpUtf16 *targetEnd,
                                           McpWinConversionFlags flags,
                                           McpWinEndianity endianity);

static McpWinfirstByteMark _MCP_WIN_ConvertUTF16toUTF8(const McpUtf16 **sourceStart,
                                           const McpUtf16 *sourceEnd, 
                                           McpUtf8 **targetStart,
                                           McpUtf8 *targetEnd,
                                           McpWinConversionFlags flags,
                                           McpWinEndianity endianity);
		
static McpBool _MCP_WIN_isLegalUTF8Sequence(const McpUtf8 *source,
                                const McpUtf8 *sourceEnd);

static McpBool _MCP_WIN_isLegalUTF8(const McpUtf8 *source,
                        int length);

/********************************************************************************
 *
 * Function definitions
 *
 *******************************************************************************/

static McpWinfirstByteMark _MCP_WIN_ConvertUTF16toUTF8 (const McpUtf16 **sourceStart,
                                            const McpUtf16 *sourceEnd, 
                                            McpUtf8 **targetStart,
                                            McpUtf8 *targetEnd,
                                            McpWinConversionFlags flags,
                                            McpWinEndianity endianity)
{
  McpWinfirstByteMark result = conversionOK;
  const McpUtf16 *source = *sourceStart;
  McpUtf8 *target = *targetStart;

  while (source < sourceEnd)
  {
	  McpU32 ch;
	  McpU16 ch16;
    unsigned short bytesToWrite = 0;
    const McpU32 byteMask = 0xBF;
    const McpU32 byteMark = 0x80; 
    const McpUtf16* oldSource = source; /* In case we have to back up because of target overflow. */

    /* Read next UTF-16 word, according the defined enianity. */
    MCP_WIN_READ_UTF16(endianity, source, &ch16)
    ch = (McpU32) ch16; /* Use 32 bit value for calculations. */
    source++;

	  /* If we have a surrogate pair, convert to McpU32 first. */
	  if (ch >= MCP_WIN_UNI_SUR_HIGH_START && ch <= MCP_WIN_UNI_SUR_HIGH_END)
    {
	    /* If the 16 bits following the high surrogate are in the source buffer... */
	    if (source < sourceEnd)
      {
        McpU32 ch2;
        
        /* Read high surrogate. */
        MCP_WIN_READ_UTF16(endianity, source, &ch16)
        ch2 = (McpU32) ch16; /* Use 32 bit value for calculations. */

		    /* If it's a low surrogate, convert to UTF32. */
		    if (ch2 >= MCP_WIN_UNI_SUR_LOW_START && ch2 <= MCP_WIN_UNI_SUR_LOW_END)
        {
		      ch = ((ch - MCP_WIN_UNI_SUR_HIGH_START) << mcpWin_halfShift) + (ch2 - MCP_WIN_UNI_SUR_LOW_START) + mcpWin_halfBase;
		      ++source;
		    }
        else if (flags == strictConversion)
		    { /* it's an unpaired high surrogate */
		      --source; /* return to the illegal value itself */
		      result = sourceIllegal;
		      break;
		    }
	    }
      else
      { /* We don't have the 16 bits following the high surrogate. */
		    --source; /* return to the high surrogate */
		    result = sourceExhausted;
		    break;
	    }
	  } 
    else if (flags == strictConversion)
    {
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= MCP_WIN_UNI_SUR_LOW_START && ch <= MCP_WIN_UNI_SUR_LOW_END)
      {
		    --source; /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
	    }
	  }

	  /* Figure out how many bytes the result will require */
	  if (ch < (McpU32)0x80) { bytesToWrite = 1; }
	  else if (ch < (McpU32)0x800) {   bytesToWrite = 2; }
	  else if (ch < (McpU32)0x10000) {  bytesToWrite = 3; }
	  else if (ch < (McpU32)0x110000) { bytesToWrite = 4; }
	  else
    {
      bytesToWrite = 3;
      ch = MCP_WIN_UNI_REPLACEMENT_CHAR;
	  }

    target += bytesToWrite;
    if (target > targetEnd)
    {
      source = oldSource; /* Back up source pointer! */
      target -= bytesToWrite; result = targetExhausted;
      break;
    }

	  switch (bytesToWrite)
    { /* note: everything falls through. */
      case 4: *--target = (McpUtf8)((ch | byteMark) & byteMask); ch >>= 6;
      case 3: *--target = (McpUtf8)((ch | byteMark) & byteMask); ch >>= 6;
      case 2: *--target = (McpUtf8)((ch | byteMark) & byteMask); ch >>= 6;
      case 1: *--target =  (McpUtf8)(ch | mcpWin_firstByteMark[bytesToWrite]);
    }

    target += bytesToWrite;
  }

  *sourceStart = source;
  *targetStart = target;

  return result;
}

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = mcpWin_trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns MCP_FALSE.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */
static McpBool _MCP_WIN_isLegalUTF8(const McpUtf8 *source,
                        int length)
{
  McpUtf8 a;
  const McpUtf8 *srcptr = source+length;

  switch (length)
  {
    default: return MCP_FALSE;

    /* Everything else falls through when "MCP_TRUE"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return MCP_FALSE;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return MCP_FALSE;
    case 2: if ((a = (*--srcptr)) > 0xBF) return MCP_FALSE;

    switch (*source)
    {
	    /* no fall-through in this inner switch */
      case 0xE0: if (a < 0xA0) return MCP_FALSE; break;
	    case 0xED: if (a > 0x9F) return MCP_FALSE; break;
	    case 0xF0: if (a < 0x90) return MCP_FALSE; break;
	    case 0xF4: if (a > 0x8F) return MCP_FALSE; break;
	    default:  if (a < 0x80) return MCP_FALSE;
    }

    case 1: if (*source >= 0x80 && *source < 0xC2) return MCP_FALSE;
  }

  if (*source > 0xF4) return MCP_FALSE;
  return MCP_TRUE;
}

/* --------------------------------------------------------------------- */

/*
 * Exported function to return whether a UTF-8 sequence is legal or not.
 * This is not used here; it's just exported.
 */
#ifdef SKIP 
static McpBool _MCP_WIN_isLegalUTF8Sequence(const McpUtf8 *source,
                                const McpUtf8 *sourceEnd)
{
  int length = mcpWin_trailingBytesForUTF8[*source] + 1;

  if (source + length > sourceEnd)
  {
	  return MCP_FALSE;
  }

  return _MCP_WIN_isLegalUTF8(source,length);
}
#endif /* SKIP non used function. */

/* --------------------------------------------------------------------- */

/* _MCP_WIN_ConvertUTF8toUTF16 notes:
 * The interface converts a whole buffer to avoid function-call overhead.
 * Constants have been gathered. Loops & conditionals have been removed as
 * much as possible for efficiency, in favor of drop-through switches.
 * (See "Note A" below for equivalent code.)
 * If your compiler supports it, the "_MCP_WIN_isLegalUTF8" call can be turned
 * into an inline function.
 */
/* ---------------------------------------------------------------------
    Note A.
    The fall-through switches in UTF-8 reading code save a
    temp variable, some decrements & conditionals.  The switches
    are equivalent to the following loop:
    
{
  int tmpBytesToRead = extraBytesToRead+1;
  do
  {
    ch += *source++;
    --tmpBytesToRead;
    if (tmpBytesToRead) ch <<= 6;
  } while (tmpBytesToRead > 0);
}

    In UTF-8 writing code, the switches on "bytesToWrite" are
    similarly unrolled loops.
   --------------------------------------------------------------------- */
static McpWinfirstByteMark _MCP_WIN_ConvertUTF8toUTF16 (const McpUtf8 **sourceStart,
                                            const McpUtf8 *sourceEnd, 
                                            McpUtf16 **targetStart,
                                            McpUtf16 *targetEnd, 
                                            McpWinConversionFlags flags,
                                            McpWinEndianity endianity)
{
  McpWinfirstByteMark result = conversionOK;
  const McpUtf8 *source = *sourceStart;
  McpUtf16 *target = *targetStart;
    
  while (source < sourceEnd)
  {
    McpU32 ch = 0;
    unsigned short extraBytesToRead = mcpWin_trailingBytesForUTF8[*source];

    if (source + extraBytesToRead >= sourceEnd)
    {
      result = sourceExhausted; break;
    }

    /* Do this check whether lenient or strict */
    if (MCP_FALSE == _MCP_WIN_isLegalUTF8(source, extraBytesToRead+1))
    {
      result = sourceIllegal;
      break;
    }

    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (extraBytesToRead)
    {
      case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
      case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
      case 3: ch += *source++; ch <<= 6;
      case 2: ch += *source++; ch <<= 6;
      case 1: ch += *source++; ch <<= 6;
      case 0: ch += *source++;
    }
    
    ch -= mcpWin_offsetsFromUTF8[extraBytesToRead];

    if (target >= targetEnd)
    {
      source -= (extraBytesToRead+1); /* Back up source pointer! */
      result = targetExhausted; break;
    }

    if (ch <= MCP_WIN_UNI_MAX_BMP)
    { /* Target is a character <= 0xFFFF */
	    /* UTF-16 surrogate values are illegal in UTF-32 */
      if (ch >= MCP_WIN_UNI_SUR_HIGH_START && ch <= MCP_WIN_UNI_SUR_LOW_END)
      {
        if (flags == strictConversion)
        {
          source -= (extraBytesToRead+1); /* return to the illegal value itself */
          result = sourceIllegal;
          break;
        }
        else
        {
          MCP_WIN_WRITE_UTF16(endianity,target,(McpU16)MCP_WIN_UNI_REPLACEMENT_CHAR)
          target++;
        }
      }
      else
      {
        MCP_WIN_WRITE_UTF16(endianity,target,(McpU16)ch) /* normal case */      
        target++;
      }
    }
    else if (ch > MCP_WIN_UNI_MAX_UTF16)
    {
      if (flags == strictConversion)
      {
        result = sourceIllegal;
        source -= (extraBytesToRead+1); /* return to the start */
        break; /* Bail out; shouldn't continue */
      }
      else
      {
        MCP_WIN_WRITE_UTF16(endianity,target,(McpU16)MCP_WIN_UNI_REPLACEMENT_CHAR)
        target++;
      }
    }
    else
    {
      /* target is a character in range 0xFFFF - 0x10FFFF. */
      if (target + 1 >= targetEnd)
      {
        source -= (extraBytesToRead+1); /* Back up source pointer! */
        result = targetExhausted; break;
      }

      ch -= mcpWin_halfBase;

      MCP_WIN_WRITE_UTF16(endianity,target,(McpU16)((ch >> mcpWin_halfShift) + MCP_WIN_UNI_SUR_HIGH_START))
      target++;

      MCP_WIN_WRITE_UTF16(endianity,target,(McpU16)((ch & mcpWin_halfMask) + MCP_WIN_UNI_SUR_LOW_START))
      target++;
	  }
  }

  *sourceStart = source;
  *targetStart = target;

  return result;
}


/********************************************************************************
 *
 * Exported function definitions
 *
 *******************************************************************************/


McpU16 MCP_WIN_Utf16ToUtf8Endian(McpUtf8 *tgtText,
                          McpU16 tgtSize,
                          McpUtf16 *srcText,
                          McpU16 srcLen,
                          McpWinEndianity endianity)
{
  McpUtf16 *sourceStart;
  McpUtf8 *targetStart;
  
  sourceStart = srcText;
  targetStart = tgtText;
	
  (void) _MCP_WIN_ConvertUTF16toUTF8((const McpUtf16 **)&sourceStart, (McpUtf16*) &srcText[srcLen], 
          &targetStart, tgtText + tgtSize + 1, strictConversion, endianity);

  /* conversion failed? --> can use normal error code. */
/*  if(res != conversionOK)
  {
    * Possible error codes:                                           *
    *   sourceExhausted:    partial character in source, but hit end  *
    *   targetExhausted:    insuff. room in target for conversion     *
    *   sourceIllegal:      source sequence is illegal/malformed      *
    *   return -1;          ahh.. can use normal error code           *
  }
*/

  /* Return number of bytes filled in the 'tgtText' (including the 0-byte) */
  return (McpU16) (targetStart - tgtText);
}

/* --------------------------------------------------------------------- */

McpU16 MCP_WIN_Utf8ToUtf16Endian(McpUtf16 *tgtText, 
                       McpU16 tgtSize, 
                       const McpUtf8 *srcText,
                       McpWinEndianity endianity)
{
  McpUtf8 *sourceStart;
  McpUtf8 *sourceEnd;
  McpUtf16 *targetStart;
  McpUtf16 *targetEnd;
  McpWinfirstByteMark res;

	/* Point to the first byte which is _out_ of the string */
	/*  because we want the null byte to be converted too   */
  sourceStart = (McpUtf8*)srcText;
  sourceEnd   =  &sourceStart[MCP_HAL_STRING_StrLenUtf8(srcText)+1];

  targetStart = tgtText;
  targetEnd   = targetStart + tgtSize + 1;
	
  res = _MCP_WIN_ConvertUTF8toUTF16((const McpUtf8 **)&sourceStart, sourceEnd, 
        &targetStart, targetEnd, strictConversion, endianity);

  /* conversion failed? --> return error. */
  if(res != conversionOK)
  { 
    /* Other possible error codes:                                     */
    /*   sourceExhausted:    partial character in source, but hit end  */
    /*   targetExhausted:    insuff. room in target for conversion     */
    /*   sourceIllegal:      source sequence is illegal/malformed      */
    return 1;
  }

  /* Return the number of bytes that are written in the 'tgtText', */
  /*  including the 0-termination of 2 bytes.                      */
  return (McpU16) (2*(targetStart - tgtText));
}

