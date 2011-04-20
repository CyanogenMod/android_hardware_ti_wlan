/*
 * SdioAdapter.c
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

 
/** \file   SdioAdapter.c 
 *  \brief  The SDIO driver adapter. Platform dependent. 
 * 
 * An adaptation layer between the lower SDIO driver (in BSP) and the upper SdioBusDrv.
 * Used for issuing all SDIO transaction types towards the lower SDIO-driver.
 * Makes the decision whether to use Sync or Async transaction, and reflects it to the caller
 *     by the return value and calling its callback in case of Async.
 *  
 *  \see    SdioAdapter.h, SdioDrv.c & h
 */

#include <linux/mmc/host.h>
#include <linux/slab.h>

#include "SdioDrvDbg.h"
#include "TxnDefs.h"
#include "SdioAdapter.h"
#include "SdioDrv.h"
#include "bmtrace_api.h"
static unsigned char *pDmaBufAddr = 0;

int g_ssd_debug_level=4;

/************************************************************************
 * Defines
 ************************************************************************/
/* Sync/Async Threshold */
#ifdef FULL_ASYNC_MODE
#define SYNC_ASYNC_LENGTH_THRESH	0     /* Use Async for all transactions */
#else
#define SYNC_ASYNC_LENGTH_THRESH	360   /* Use Async for transactions longer than this threshold (in bytes) */
#endif

#define MAX_RETRIES                 10
#define MAX_BUS_TXN_SIZE            8192  /* Max bus transaction size in bytes (for the DMA buffer allocation) */
int sdioAdapt_ConnectBus (void *        fCbFunc,
                          void *        hCbArg,
                          unsigned int  uBlkSizeShift,
                          unsigned int  uSdioThreadPriority,
                          unsigned char **pRxDmaBufAddr,
                          unsigned int  *pRxDmaBufLen,
                          unsigned char **pTxDmaBufAddr,
                          unsigned int  *pTxDmaBufLen)
{
    unsigned int   uBlkSize = 1 << uBlkSizeShift;
	int            iStatus;

    if (uBlkSize < SYNC_ASYNC_LENGTH_THRESH) 
    {
        PERR1("%s(): Block-Size should be bigger than SYNC_ASYNC_LENGTH_THRESH!!\n", __FUNCTION__ );
    }

    /* Allocate a DMA-able buffer and provide it to the upper layer to be used for all read and write transactions */
    if (pDmaBufAddr == 0) /* allocate only once (in case this function is called multiple times) */
    {
        pDmaBufAddr = kmalloc (MAX_BUS_TXN_SIZE, GFP_ATOMIC | GFP_DMA);
        if (pDmaBufAddr == 0) { return -1; }
    }
    *pRxDmaBufAddr = *pTxDmaBufAddr = pDmaBufAddr;
    *pRxDmaBufLen  = *pTxDmaBufLen  = MAX_BUS_TXN_SIZE;
    /* Init SDIO driver and HW */
    iStatus = sdioDrv_ConnectBus (fCbFunc, hCbArg, uBlkSizeShift,uSdioThreadPriority);
    if (iStatus) { return iStatus; }

    sdioDrv_ClaimHost(SDIO_WLAN_FUNC);
    iStatus = sdioDrv_EnableFunction(TXN_FUNC_ID_WLAN);
    if (iStatus) {
        sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);
        return iStatus;
    }

#ifdef SDIO_IN_BAND_INTERRUPT
    iStatus = sdioDrv_EnableInterrupt(TXN_FUNC_ID_WLAN);
    if (iStatus) { return iStatus; }
#endif

    iStatus = sdioDrv_SetBlockSize(TXN_FUNC_ID_WLAN, uBlkSize);
    if (iStatus) {
        sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);
        return iStatus;
    }

    sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);

    return iStatus;
}


int sdioAdapt_DisconnectBus (void)
{
#ifdef SDIO_IN_BAND_INTERRUPT
	sdioDrv_DisableInterrupt(TXN_FUNC_ID_WLAN);
#endif

    sdioDrv_ClaimHost(SDIO_WLAN_FUNC);
	sdioDrv_DisableFunction(TXN_FUNC_ID_WLAN);
    sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);
	if (pDmaBufAddr) 
    {
        kfree (pDmaBufAddr);
        pDmaBufAddr = 0;
    }

    return sdioDrv_DisconnectBus ();
}

ETxnStatus sdioAdapt_Transact (unsigned int  uFuncId,
                               unsigned int  uHwAddr,
                               void *        pHostAddr,
                               unsigned int  uLength,
                               unsigned int  bDirection,
                               unsigned int  bBlkMode,
                               unsigned int  bFixedAddr,
                               unsigned int  bMore)
{
    int iStatus;

	/* currently only SYNC I/O is supported */
    /* If transction length is below threshold, use Sync methods */
#if 0
    if (uLength < SYNC_ASYNC_LENGTH_THRESH) 
    {
#endif
        /* Call read or write Sync method */
        if (bDirection) 
        {
            CL_TRACE_START_L2();
            iStatus = sdioDrv_ReadSync (uFuncId, uHwAddr, pHostAddr, uLength, bFixedAddr, bMore);
            CL_TRACE_END_L2("tiwlan_drv.ko", "INHERIT", "SDIO", ".ReadSync");
        }
        else 
        {
            CL_TRACE_START_L2();
            iStatus = sdioDrv_WriteSync (uFuncId, uHwAddr, pHostAddr, uLength, bFixedAddr, bMore);
            CL_TRACE_END_L2("tiwlan_drv.ko", "INHERIT", "SDIO", ".WriteSync");
        }

        /* If failed return ERROR, if succeeded return COMPLETE */
        if (iStatus) 
        {
            return TXN_STATUS_ERROR;
        }
        return TXN_STATUS_COMPLETE;
#if 0
    }

    /* If transction length is above threshold, use Async methods */
    else 
    {
        /* Call read or write Async method */
        if (bDirection) 
        {
            CL_TRACE_START_L2();
            iStatus = sdioDrv_ReadAsync (uFuncId, uHwAddr, pHostAddr, uLength, bBlkMode, bFixedAddr, bMore);
            CL_TRACE_END_L2("tiwlan_drv.ko", "INHERIT", "SDIO", ".ReadAsync");
        }
        else 
        {
            CL_TRACE_START_L2();
            iStatus = sdioDrv_WriteAsync (uFuncId, uHwAddr, pHostAddr, uLength, bBlkMode, bFixedAddr, bMore);
            CL_TRACE_END_L2("tiwlan_drv.ko", "INHERIT", "SDIO", ".WriteAsync");
        }

        /* If failed return ERROR, if succeeded return PENDING */
		if (iStatus) 
        {
            return TXN_STATUS_ERROR;
        }
        return TXN_STATUS_PENDING;
    }
#endif
}

ETxnStatus sdioAdapt_TransactBytes (unsigned int  uFuncId,
                                    unsigned int  uHwAddr,
                                    void *        pHostAddr,
                                    unsigned int  uLength,
                                    unsigned int  bDirection,
                                    unsigned int  bMore)
{
    int iStatus;

    if (bMore ==1)
    {
        sdioDrv_cancel_inact_timer();
        sdioDrv_ClaimHost(SDIO_WLAN_FUNC);
    }
    /* Call read or write bytes Sync method */
    if (bDirection) 
    {
        iStatus = sdioDrv_ReadSyncBytes (uFuncId, uHwAddr, pHostAddr, uLength, bMore);
    }
    else 
    {
        iStatus = sdioDrv_WriteSyncBytes (uFuncId, uHwAddr, pHostAddr, uLength, bMore);
    }
    if (bMore ==0)
    {
        sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);
        sdioDrv_start_inact_timer();
    }

    /* If failed return ERROR, if succeeded return COMPLETE */
    if (iStatus) 
    {
        return TXN_STATUS_ERROR;
    }
    return TXN_STATUS_COMPLETE;
}

