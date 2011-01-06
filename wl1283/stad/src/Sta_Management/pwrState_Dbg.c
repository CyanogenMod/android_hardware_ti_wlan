/*
 * pwrState_Dbg.c
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


/** \file pwrState_Dbg.c
 *
 *  \brief Power State Module debug functions.

 *  \date 03-Nov-2010
 */

#include "tidef.h"
#include "osApi.h"
#include "timer.h"
#include "report.h"
#include "TWDriver.h"

#include "pwrState_Types.h"


#define DBG_FUNC_PRINT_HELP		0
#define DBG_FUNC_PRINT_OBJECT	1
#define DBG_FUNC_PRINT_STATS	2
#define DBG_FUNC_CLEAR_STATS	3

static void printBinDump(TI_UINT8* buf, TI_UINT32 len)
{
	static char* nibbleToStr[] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
			                      "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111",};
	TI_UINT32 offset = 0;
	TI_UINT8  nibble;

	for (offset = 0; offset < len; offset++)
	{
		nibble = (buf[offset] & 0xF0) >> 4;
		WLAN_OS_REPORT((nibbleToStr[nibble]));
		WLAN_OS_REPORT((" "));

		nibble = (buf[offset] & 0x0F);
		WLAN_OS_REPORT((nibbleToStr[nibble]));
		WLAN_OS_REPORT((" "));
	}
}

static void pwrState_dbpPrintFunctions(void)
{
	WLAN_OS_REPORT(("   pwrState Debug Functions   \n"));
	WLAN_OS_REPORT(("------------------------------\n"));
	WLAN_OS_REPORT(("2400 - Print the pwrState debug help\n"));
	WLAN_OS_REPORT(("2401 - Print the pwrState object\n"));
	WLAN_OS_REPORT(("2402 - Print the pwrState statistics\n"));
	WLAN_OS_REPORT(("2403 - Reset the pwrState statistics\n"));
}

static void pwrState_dbgPrintObject(TI_HANDLE hPwrState)
{
	TPwrState *this = (TPwrState*)hPwrState;

	WLAN_OS_REPORT(("pwrState:\n"));
	WLAN_OS_REPORT(("  eSuspendType = %d\n", this->tConfig.eSuspendType));
	WLAN_OS_REPORT(("  uSuspendNDTIM = %d\n", this->tConfig.uSuspendNDTIM));
	WLAN_OS_REPORT(("  eStandbyNextState = %d\n", this->tConfig.eStandbyNextState));
	WLAN_OS_REPORT(("  eSuspendFilterUsage = 0x%x\n", this->tConfig.eSuspendFilterUsage));
	WLAN_OS_REPORT(("  uDozeTimeout = %d\n", this->tConfig.uDozeTimeout));

	WLAN_OS_REPORT(("  tSuspendRxFilterValue = {.offset=%d, .mask=", this->tConfig.tSuspendRxFilterValue.offset));
	printBinDump(this->tConfig.tSuspendRxFilterValue.mask, this->tConfig.tSuspendRxFilterValue.maskLength);
	WLAN_OS_REPORT((", .pattern="));
	printBinDump(this->tConfig.tSuspendRxFilterValue.pattern, this->tConfig.tSuspendRxFilterValue.patternLength);
	WLAN_OS_REPORT(("}\n"));

	WLAN_OS_REPORT(("  tTwdSuspendConfig.uCmdMboxTimeout = %d\n", this->tConfig.tTwdSuspendConfig.uCmdMboxTimeout));

	WLAN_OS_REPORT(("  fCurrentState = %pF\n", this->fCurrentState));
	WLAN_OS_REPORT(("  tCurrentTransition.fCompleteCb = %pF\n", this->tCurrentTransition.fCompleteCb));
	WLAN_OS_REPORT(("  tCurrentTransition.iCompleteStatus = %d\n", this->tCurrentTransition.iCompleteStatus));
	WLAN_OS_REPORT(("  tCurrentTransition.bContinueToOff = %d\n", this->tCurrentTransition.bContinueToOff));
}

static void pwrState_dbgPrintStats(TI_HANDLE hPwrState)
{
	WLAN_OS_REPORT(("Not yet implemented!\n"));
}

static void pwrState_dbgClearStats(TI_HANDLE hPwrState)
{
	WLAN_OS_REPORT(("Not yet implemented!\n"));
}

/**
 * \fn     smeDebugFunction
 *
 * \brief  Main pwrState debug function
 *
 * \param  hPwrState
 * \param  funcType	the specific debug function
 * \param  pParam	parameters for the debug function
 *
 */
void pwrState_DebugFunction(TI_HANDLE hPwrState, TI_UINT32 uFuncType, void *pParam)
{
	switch (uFuncType)
	{
	case DBG_FUNC_PRINT_HELP:
		pwrState_dbpPrintFunctions();
		break;

	case DBG_FUNC_PRINT_OBJECT:
		pwrState_dbgPrintObject( hPwrState );
		break;

	case DBG_FUNC_PRINT_STATS:
		pwrState_dbgPrintStats( hPwrState );
		break;

	case DBG_FUNC_CLEAR_STATS:
		pwrState_dbgClearStats( hPwrState );
		break;

	default:
		WLAN_OS_REPORT(("%s: invalid function type (%d)\n", __func__, uFuncType));
		break;
	}
}

