/***************************************************************************
**+----------------------------------------------------------------------+**
**|                                ****                                  |**
**|                                ****                                  |**
**|                                ******o***                            |**
**|                          ********_///_****                           |**
**|                           ***** /_//_/ ****                          |**
**|                            ** ** (__/ ****                           |**
**|                                *********                             |**
**|                                 ****                                 |**
**|                                  ***                                 |**
**|                                                                      |**
**|     Copyright (c) 1998 - 2009 Texas Instruments Incorporated         |**
**|                        ALL RIGHTS RESERVED                           |**
**|                                                                      |**
**| Permission is hereby granted to licensees of Texas Instruments       |**
**| Incorporated (TI) products to use this computer program for the sole |**
**| purpose of implementing a licensee product based on TI products.     |**
**| No other rights to reproduce, use, or disseminate this computer      |**
**| program, whether in part or in whole, are granted.                   |**
**|                                                                      |**
**| TI makes no representation or warranties with respect to the         |**
**| performance of this computer program, and specifically disclaims     |**
**| any responsibility for any damages, special or consequential,        |**
**| connected with the use of this program.                              |**
**|                                                                      |**
**+----------------------------------------------------------------------+**
***************************************************************************/
 
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

#include "host_platform.h"
#include "SdioDrvDbg.h"
#include "TxnDefs.h"
#include "SdioAdapter.h"
#include "SdioDrv.h"
#include "bmtrace_api.h"


#ifdef SDIO_1_BIT /* see also in SdioDrv.c */
#define SDIO_BITS_CODE   0x80 /* 1 bits */
#else
#define SDIO_BITS_CODE   0x82 /* 4 bits */
#endif

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

/* For block mode configuration */
#define FN0_FBR2_REG_108                    0x210
#define FN0_FBR2_REG_108_BIT_MASK           0xFFF 


int sdioAdapt_ConnectBus (void *        fCbFunc,
                          void *        hCbArg,
                          unsigned int  uBlkSizeShift,
                          unsigned int  uSdioThreadPriority,
                          unsigned char **pRxDmaBufAddr,
                          unsigned int  *pRxDmaBufLen,
                          unsigned char **pTxDmaBufAddr,
                          unsigned int  *pTxDmaBufLen)
{
	unsigned char  uByte;
    unsigned long  uLong;
    unsigned long  uCount = 0;
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
    iStatus = sdioDrv_ConnectBus (fCbFunc, hCbArg, uBlkSizeShift, uSdioThreadPriority);
	if (iStatus) { return iStatus; }
  
#if 0
	/* Send commands sequence: 0, 5, 3, 7 */
	iStatus = sdioDrv_ExecuteCmd (SD_IO_GO_IDLE_STATE, 0, MMC_RSP_NONE, &uByte, sizeof(uByte));
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SDIO_CMD5, TRY1_VDD_VOLTAGE_WINDOW, MMC_RSP_R4, &uByte, sizeof(uByte));
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SD_IO_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6, &uLong, sizeof(uLong));
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SD_IO_SELECT_CARD, uLong, MMC_RSP_R6, &uByte, sizeof(uByte));
	if (iStatus) { return iStatus; }
#endif
	/* Send commands sequence: 0, 5, 3, 7 */
	printk("sdioAdapt_ConnectBus: Send commands sequence: 0, 5, 3, 7\n");

	/* according to p. 3601 */
	iStatus = sdioDrv_ExecuteCmd (SD_IO_GO_IDLE_STATE, 0, MMC_RSP_NONE, &uByte, sizeof(uByte));

	printk("\nAfter SD_IO_GO_IDLE_STATE, RSP10=%d \n", iStatus );
	if (iStatus) { return iStatus; }

	iStatus = sdioDrv_ExecuteCmd (SDIO_CMD5, TRY1_VDD_VOLTAGE_WINDOW, MMC_RSP_R4, &uByte, sizeof(uByte));
	printk("\nAfter VDD_VOLTAGE_WINDOW, RSP10=%d \n", iStatus );
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SD_IO_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6, &uLong, sizeof(uLong));
	printk("\n After SD_IO_SEND_RELATIVE_ADDR, RSP10=%d \n", iStatus );
	if (iStatus) { return iStatus; }

	printk("\n BENZY: uLong=%lu \n", uLong );

	iStatus = sdioDrv_ExecuteCmd (SD_IO_SELECT_CARD, 0x10000, MMC_RSP_R6, &uByte, sizeof(uByte));
	printk("\nAfter SD_IO_SELECT_CARD, RSP10=%d \n", iStatus );
	if (iStatus) { return iStatus; }



    /* NOTE:
     * =====
     * Each of the following loops is a workaround for a HW bug that will be solved in PG1.1 !!
     * Each write of CMD-52 to function-0 should use it as follows:
     * 1) Write the desired byte using CMD-52
     * 2) Read back the byte using CMD-52
     * 3) Write two dummy bytes to address 0xC8 using CMD-53
     * 4) If the byte read in step 2 is different than the written byte repeat the sequence
     */

#ifdef TNETW1283
#ifdef SDIO_HIGH_SPEED
    PERR("sdioAdapt_ConnectBus: Set HIGH_SPEED bit on register 0x13\n");
    /* CCCR 13 bit EHS(1) */
    iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, 0x13, &uByte, 1, 1);
    PERR2("After r 0x%x, iStatus=%d \n", uByte, iStatus );
    if (iStatus) { return iStatus; }
    
    uByte |= 0x1; /* set bit #1 EHS */
    iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, 0x13, &uByte, 1, 1);
    PERR2("After w 0x%x, iStatus=%d \n", uByte, iStatus );
    if (iStatus) { return iStatus; }

    iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, 0x13, &uByte, 1, 1);
    PERR2("After r 0x%x, iStatus=%d \n", uByte, iStatus );
    if (iStatus) { return iStatus; }
    
    PERR1("After CCCR 0x13, uByte=%d \n", (int)uByte );
#endif
#endif

    /* set device side bus width to 4 bit (for 1 bit write 0x80 instead of 0x82) */
    do
    {
        uByte = SDIO_BITS_CODE;
        iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, CCCR_BUS_INTERFACE_CONTOROL, &uByte, 1, 1);
        if (iStatus) { return iStatus; }

        iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, CCCR_BUS_INTERFACE_CONTOROL, &uByte, 1, 1);
        if (iStatus) { return iStatus; }
        
#ifdef TNETW1283
        PERR1("Skip  w 0xC8, iStatus=%d \n", iStatus );
#else
        iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, 0xC8, &uLong, 2, 1, 1);
        PERR1("After w 0xC8, after HIGH_SPEED iStatus=%d \n", iStatus );
        if (iStatus) { return iStatus; }
#endif
        uCount++;

    } while ((uByte != SDIO_BITS_CODE) && (uCount < MAX_RETRIES));


    uCount = 0;

    /* allow function 2 */
    do
    {
        uByte = 4;
        iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, CCCR_IO_ENABLE, &uByte, 1, 1);
        if (iStatus) { return iStatus; }

        iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, CCCR_IO_ENABLE, &uByte, 1, 1);
        if (iStatus) { return iStatus; }
        
#ifdef TNETW1283
        PERR1("Skip  w 0xC8, after CCCR_IO_ENABLE iStatus=%d \n", iStatus );
#else
        iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, 0xC8, &uLong, 2, 1, 1);
        if (iStatus) { return iStatus; }
#endif
        uCount++;

    } while ((uByte != 4) && (uCount < MAX_RETRIES));


#ifdef SDIO_IN_BAND_INTERRUPT

    uCount = 0;

    do
    {
        uByte = 3;
        iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, CCCR_INT_ENABLE, &uByte, 1, 1);
        if (iStatus) { return iStatus; }

        iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, CCCR_INT_ENABLE, &uByte, 1, 1);
        if (iStatus) { return iStatus; }
        
#ifdef TNETW1283
        PERR1("Skip  w 0xC8, after CCCR_INT_ENABLE iStatus=%d \n", iStatus );
#else
        iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, 0xC8, &uLong, 2, 1, 1);
        if (iStatus) { return iStatus; }
#endif
        uCount++;

    } while ((uByte != 3) && (uCount < MAX_RETRIES));


#endif

    uCount = 0;
    
    /* set block size for SDIO block mode */
    do
    {
        uLong = uBlkSize;
        iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, FN0_FBR2_REG_108, &uLong, 2, 1, 1);
        if (iStatus) { return iStatus; }

        iStatus = sdioDrv_ReadSync (TXN_FUNC_ID_CTRL, FN0_FBR2_REG_108, &uLong, 2, 1, 1);
        if (iStatus) { return iStatus; }
        
#ifdef TNETW1283
        PERR1("Skip  w 0xC8, after FN0_FBR2_REG_108 iStatus=%d \n", iStatus );
#else
        iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, 0xC8, &uLong, 2, 1, 1);
        PERR1("After w 0xC8, uLong=%d \n", (int)uLong );
        if (iStatus) { return iStatus; }
#endif
        uCount++;

    } while (((uLong & FN0_FBR2_REG_108_BIT_MASK) != uBlkSize) && (uCount < MAX_RETRIES));

    
    if (uCount >= MAX_RETRIES)
    {
        /* Failed to write CMD52_WRITE to function 0 */
        return (int)uCount;
    }

	return iStatus;
}


int sdioAdapt_DisconnectBus (void)
{
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

    /* If transction length is below threshold, use Sync methods */
////Benzy - Hack        if (uLength < SYNC_ASYNC_LENGTH_THRESH) 
//    {
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
#if 0 /*Benzy Remove it !!!*/

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

    /* Call read or write bytes Sync method */
    if (bDirection) 
    {
        iStatus = sdioDrv_ReadSyncBytes (uFuncId, uHwAddr, pHostAddr, uLength, bMore);
    }
    else 
    {
        iStatus = sdioDrv_WriteSyncBytes (uFuncId, uHwAddr, pHostAddr, uLength, bMore);
    }

    /* If failed return ERROR, if succeeded return COMPLETE */
    if (iStatus) 
    {
        return TXN_STATUS_ERROR;
    }
    return TXN_STATUS_COMPLETE;
}




