/*
 * testdrv.c
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
*

/** \file testdrv.c
*/

/* includes */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mmc/card.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include "testdrv.h"
#include "SdioDrv.h"
#include "host_platform.h"

int sdioAdapt_ConnectBus(void *fCbFunc, void *hCbArg, unsigned int  uBlkSizeShift, unsigned int  uSdioThreadPriority);

static unsigned int mem_partition_size = TESTDRV_MEM_DOWNLOAD_PART_SIZE;
static unsigned int reg_partition_size = TESTDRV_REG_DOWNLOAD_PART_SIZE;

typedef enum 
{
	SdioSync= 0,
	SdioAsync= 1,
	SdioSyncMax
} SdioSyncMode_e;

typedef enum 
{
	SDIO_BLOCK_NONE= 0,
	SDIO_BLOCK_MODE= 1
} ESdioBlockMode;

#define TESTDRV_READ_TEST    0x00000001
#define TESTDRV_WRITE_TEST   0x00000002
#define TESTDRV_COMPARE_TEST 0x00000004
#define TESTDRV_FUNC0_TEST        0x00000008
#define TESTDRV_FUNC0_READ_TEST   0x00000010

#define DO_ON_ERROR(RETURN_VALUE, STRING) \
	printk(STRING); \
	if (RETURN_VALUE!=0) \
	{ \
		printk(" FAILED !!!\n"); \
		return ret; \
	} \
	else \
	{ \
		printk(" PASSED\n" ); \
	} \

struct _g_sdio
{
	void*		      SDIO_handle;
	struct semaphore  sem;
	int               status;
} g_sdio;

#define SDIO_TEST_ALIGNMENT				4
#define SDIO_TEST_R_W_BUFS_NUM			10
#define SDIO_TEST_R_W_BUF_LEN			512
#define SDIO_TEST_MAX_R_W_BUF_ARRAY		(SDIO_TEST_R_W_BUF_LEN*SDIO_TEST_R_W_BUFS_NUM)

unsigned long	      g_last_read=0;
int                   g_Quiet=1;
char	              *read_buf=NULL;
char	              *read_buf_array[SDIO_TEST_R_W_BUFS_NUM];
char	              *write_buf=NULL;
char	              *write_buf_array[SDIO_TEST_R_W_BUFS_NUM];
char	              complete_test_read_buf[TESTDRV_MAX_SDIO_BLOCK];
extern int            g_sdio_debug_level;

struct parts
{
	unsigned long mem_size;
	unsigned long mem_off;
	unsigned long reg_size;
	unsigned long reg_off;
}g_parts;




/*--------------------------------------------------------------------------------------*/
/* 
 * Copy from SdioAdaptor.c 
 */

#ifdef SDIO_1_BIT /* see also in SdioDrv.c */
#define SDIO_BITS_CODE   0x80 /* 1 bits */
#else
#define SDIO_BITS_CODE   0x82 /* 4 bits */
#endif

#define PERR(format, args... ) printk(format , ##args)
#define PERR1 PERR
#define PERR2 PERR


#undef IRQ_GPIO
#undef PMENA_GPIO
#undef TNETW_IRQ

#define IRQ_GPIO                        53
#define PMENA_GPIO                      54
#define TNETW_IRQ                       (OMAP_GPIO_IRQ(IRQ_GPIO))



/************************************************************************
 * Defines
 ************************************************************************/
#define SYNC_ASYNC_LENGTH_THRESH	360   /* If Txn length above threshold use Async mode */
#define MAX_RETRIES                 10

/* For block mode configuration */
#define FN0_FBR2_REG_108                    0x210
#define FN0_FBR2_REG_108_BIT_MASK           0xFFF 
#define SDIODRV_MAX_LOOPS 50000

void dumpreg(void)
{
	printk(KERN_ERR "\n MMCHS_HL_REV      for mmc5 = %x  ", omap_readl( 0x480D5000 ));
	printk(KERN_ERR "\n MMCHS_HL_HWINFO   for mmc5 = %x  ", omap_readl( 0x480D5004 ));
	printk(KERN_ERR "\n MMCHS_HL_SYSCONFIG for mmc5 = %x ", omap_readl( 0x480D5010 ));
	printk(KERN_ERR "\n MMCHS_SYSCONFIG   for mmc5 = %x  ", omap_readl( 0x480D5110 ));
	printk(KERN_ERR "\n MMCHS_SYSSTATUS   for mmc5 = %x  ", omap_readl( 0x480D5114 ));
	printk(KERN_ERR "\n MMCHS_CSRE	      for mmc5 = %x  ", omap_readl( 0x480D5124 ));
	printk(KERN_ERR "\n MMCHS_SYSTEST     for mmc5 = %x  ", omap_readl( 0x480D5128 ));
	printk(KERN_ERR "\n MMCHS_CON         for mmc5 = %x  ", omap_readl( 0x480D512C ));
	printk(KERN_ERR "\n MMCHS_PWCNT       for mmc5 = %x  ", omap_readl( 0x480D5130 ));
	printk(KERN_ERR "\n MMCHS_BLK         for mmc5 = %x  ", omap_readl( 0x480D5204 ));
	printk(KERN_ERR "\n MMCHS_ARG         for mmc5 = %x  ", omap_readl( 0x480D5208 ));
	printk(KERN_ERR "\n MMCHS_CMD         for mmc5 = %x  ", omap_readl( 0x480D520C ));
	printk(KERN_ERR "\n MMCHS_RSP10       for mmc5 = %x  ", omap_readl( 0x480D5210 ));
	printk(KERN_ERR "\n MMCHS_RSP32       for mmc5 = %x  ", omap_readl( 0x480D5214 ));
	printk(KERN_ERR "\n MMCHS_RSP54       for mmc5 = %x  ", omap_readl( 0x480D5218 ));
	printk(KERN_ERR "\n MMCHS_RSP76       for mmc5 = %x  ", omap_readl( 0x480D521C ));
	printk(KERN_ERR "\n MMCHS_DATA        for mmc5 = %x  ", omap_readl( 0x480D5220 ));
	printk(KERN_ERR "\n MMCHS_PSTATE      for mmc5 = %x  ", omap_readl( 0x480D5224 ));
	printk(KERN_ERR "\n MMCHS_HCTL        for mmc5 = %x  ", omap_readl( 0x480D5228 ));
	printk(KERN_ERR "\n MMCHS_SYSCTL      for mmc5 = %x  ", omap_readl( 0x480D522C ));
	printk(KERN_ERR "\n MMCHS_STAT        for mmc5 = %x  ", omap_readl( 0x480D5230));
	printk(KERN_ERR "\n MMCHS_IE          for mmc5 = %x  ", omap_readl( 0x480D5234 ));
	printk(KERN_ERR "\n MMCHS_ISE         for mmc5 = %x  ", omap_readl( 0x480D5238 ));
	printk(KERN_ERR "\n MMCHS_AC12        for mmc5 = %x  ", omap_readl( 0x480D523C ));
	printk(KERN_ERR "\n MMCHS_CAPA        for mmc5 = %x  ", omap_readl( 0x480D5240 ));
	printk(KERN_ERR "\n MMCHS_CUR_CAPA    for mmc5 = %x  ", omap_readl( 0x480D5248 ));
	printk(KERN_ERR "\n MMCHS_FE          for mmc5 = %x  ", omap_readl( 0x480D5250 ));
	printk(KERN_ERR "\n MMCHS_REV         for mmc5 = %x\n", omap_readl( 0x480D52FC ));

	printk("\n start padconfiguration");
	printk(KERN_ERR "WLAN_IRQ:          0x4A100078=%x\n",   omap_readl( 0x4A100078 ));
	printk(KERN_ERR "WLAN_EN:           0x4A10007C=%x\n",   omap_readl( 0x4A10007C ));
	printk(KERN_ERR "SDIO_CLK+SDIO_CMD: 0x4A100148=%x\n",   omap_readl( 0x4A100148 ));
	printk(KERN_ERR "SDIO_D0+SDIO_D1:   0x4A10014C=%x\n",   omap_readl( 0x4A10014C ));
	printk(KERN_ERR "SDIO_D2+D3:        0x4A100150=%x\n",   omap_readl( 0x4A100150 ));
}


int sdioAdapt_ConnectBus (void *        fCbFunc,
                          void *        hCbArg,
                          unsigned int  uBlkSizeShift,
                          unsigned int  uSdioThreadPriority)
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

    /* Init SDIO driver and HW */
    iStatus = sdioDrv_ConnectBus (fCbFunc, hCbArg, uBlkSizeShift, uSdioThreadPriority);
     PERR1("After sdioDrv_ConnectBus, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }

  
    /* Send commands sequence: 0, 5, 3, 7 */
    PERR("sdioAdapt_ConnectBus: Send commands sequence: 0, 5, 3, 7\n");

	/* according to p. 3601 */
	iStatus = sdioDrv_ExecuteCmd (SD_IO_GO_IDLE_STATE, 0, MMC_RSP_NONE, &uByte, sizeof(uByte));

	PERR1("\nAfter SD_IO_GO_IDLE_STATE, RSP_NONE=%d \n", iStatus );
	if (iStatus) { return iStatus; }

	iStatus = sdioDrv_ExecuteCmd (SDIO_CMD5, TRY1_VDD_VOLTAGE_WINDOW, MMC_RSP_R4, &uByte, sizeof(uByte));
    PERR1("\nAfter VDD_VOLTAGE_WINDOW, RSP_R4=%d \n", iStatus );

	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SD_IO_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6, &uLong, sizeof(uLong));
    PERR1("\n After SD_IO_SEND_RELATIVE_ADDR, RSP_R6=%d \n", iStatus );
	if (iStatus) { return iStatus; }

	PERR1("\n SD_IO_SEND_RELATIVE_ADDR returned: uLong=%lu \n", uLong );

	iStatus = sdioDrv_ExecuteCmd (SD_IO_SELECT_CARD, 0x10000, MMC_RSP_R6, &uByte, sizeof(uByte));
    PERR1("\nAfter SD_IO_SELECT_CARD, RSP_R6=%d \n", iStatus );
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


    /* set device side bus width to 4 bit (for 1 bit write 0x80 instead of 0x82) */
    do
    {
        PERR("sdioAdapt_ConnectBus: Set bus width to %x bit on register 0xd2\n", (unsigned int)uByte);
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

    PERR2("After CCCR_BUS_INTERFACE_CONTOROL, uCount=%lu, value = 0x%x\n", uCount,  uByte);

    uCount = 0;

    /* allow function 2 */
    do
    {
		uByte = 4;
        iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, CCCR_IO_ENABLE, &uByte, 1, 1);
        if (iStatus) { return iStatus; }

		uByte = 0;
		iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, CCCR_IO_ENABLE, &uByte, 1, 1);
        if (iStatus) { 
				PERR2("BUG BUG BUG to READ from CCCR_IO_ENABLE value = 0x%x\n", (unsigned int)uByte);
				return iStatus; }

		uCount++;
    } while ((uByte != 4) && (uCount < MAX_RETRIES));

    PERR2("After CCCR_IO_ENABLE, uCount=%d, value = 0x%x\n", (int)uCount,  (unsigned int)uByte);


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

    PERR1("After CCCR_INT_ENABLE, uCount=%d \n", uCount );

#endif

    uCount = 0;
    do
    {
        uLong = uBlkSize;
		PERR1("sdioAdapt_ConnectBus: Before sdioDrv_WriteSync to FN0_FBR2_REG_108, uLong=%lu\n", uLong);
		iStatus = sdioDrv_WriteSync (TXN_FUNC_ID_CTRL, FN0_FBR2_REG_108, &uLong, 2, 1, 1);
        PERR1("After w FN0_FBR2_REG_108, uLong=%lu, iStatus=%d \n", uLong, iStatus );
        if (iStatus) { return iStatus; }

        iStatus = sdioDrv_ReadSync (TXN_FUNC_ID_CTRL, FN0_FBR2_REG_108, &uLong, 2, 1, 1);
        PERR1("After r FN0_FBR2_REG_108, uLong=%lu, iStatus=%d \n", uLong, iStatus);
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

    PERR2("After FN0_FBR2_REG_108, uCount=%d, value = 0x%x\n", (int)uCount,  (unsigned int)uLong);

    
    if (uCount >= MAX_RETRIES)
    {
        /* Failed to write CMD52_WRITE to function 0 */
        return (int)uCount;
    }
	return iStatus;
}

int sdioAdapt_ConnectBus_0537only (void *        fCbFunc,
                          void *        hCbArg,
                          unsigned int  uBlkSizeShift,
                          unsigned int  uSdioThreadPriority)
{
	unsigned char  uByte=0;
    unsigned long  uLong=0;
    //unsigned long  uCount = 0;
    unsigned int   uBlkSize = 1 << uBlkSizeShift;
	int            iStatus;

    if (uBlkSize < SYNC_ASYNC_LENGTH_THRESH) 
    {
        PERR1("%s(): Block-Size should be bigger than SYNC_ASYNC_LENGTH_THRESH!!\n", __FUNCTION__ );
    }

    /* Init SDIO driver and HW */
    iStatus = sdioDrv_ConnectBus (fCbFunc, hCbArg, uBlkSizeShift, uSdioThreadPriority);
     PERR1("After sdioDrv_ConnectBus, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }

  
    /* Send commands sequence: 0, 5, 3, 7 */
    PERR("sdioAdapt_ConnectBus_0537only: Send commands sequence: 0, 5, 3, 7\n");
	/* according to p. 3601 */
	iStatus = sdioDrv_ExecuteCmd (SD_IO_GO_IDLE_STATE, 0, MMC_RSP_NONE, &uByte, sizeof(uByte));
    PERR1("After SD_IO_GO_IDLE_STATE, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SDIO_CMD5, TRY1_VDD_VOLTAGE_WINDOW, MMC_RSP_R4, &uByte, sizeof(uByte));
    PERR1("After VDD_VOLTAGE_WINDOW, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }
	iStatus = sdioDrv_ExecuteCmd (SD_IO_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6, &uLong, sizeof(uLong));
    PERR1("After SD_IO_SEND_RELATIVE_ADDR, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }
    iStatus = sdioDrv_ExecuteCmd (SD_IO_SELECT_CARD, 0x10000, MMC_RSP_R6, &uByte, sizeof(uByte));
    PERR1("After SD_IO_SELECT_CARD, iStatus=%d \n", iStatus );
	if (iStatus) { return iStatus; }

    return iStatus;
}


/* **************************** */
/*--------------------------------------------------------------------------------------*/
inline int set_partition(int clientID, 
						 unsigned long mem_partition_size,
						 unsigned long mem_partition_offset,
						 unsigned long reg_partition_size,
						 unsigned long reg_partition_offset)
{
	int status;
	char byteData[4];
	g_parts.mem_size 	= mem_partition_size;
	g_parts.mem_off 	= mem_partition_offset;
	g_parts.reg_size 	= reg_partition_size;
	g_parts.reg_off 	= reg_partition_offset;
	/* 	printk("entering %s(%lx,%lx,%lx,%lx)\n",__FUNCTION__, v1, v2, v3, v4); */
	/* set mem partition size */
	byteData[0] = g_parts.mem_size & 0xff;
	byteData[1] = (g_parts.mem_size>>8) & 0xff;
	byteData[2] = (g_parts.mem_size>>16) & 0xff;
	byteData[3] = (g_parts.mem_size>>24) & 0xff;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET,   &byteData[0], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+1, &byteData[1], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+2, &byteData[2], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+3, &byteData[3], 1, 0);
	if(status)return status;

	/* set mem partition offset */
	byteData[0] = g_parts.mem_off & 0xff;
	byteData[1] = (g_parts.mem_off>>8) & 0xff;
	byteData[2] = (g_parts.mem_off>>16) & 0xff;
	byteData[3] = (g_parts.mem_off>>24) & 0xff;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+4, &byteData[0], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+5, &byteData[1], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+6, &byteData[2], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+7, &byteData[3], 1, 0);
	if(status)return status;

	/* set reg partition size */
	byteData[0] = g_parts.reg_size & 0xff;
	byteData[1] = (g_parts.reg_size >> 8) & 0xff;
	byteData[2] = (g_parts.reg_size>>16) & 0xff;
	byteData[3] = (g_parts.reg_size>>24) & 0xff;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+8, &byteData[0], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+9, &byteData[1], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+10, &byteData[2], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+11, &byteData[3], 1, 0);
	if(status)return status;

	/* set reg partition offset */
	byteData[0] = g_parts.reg_off & 0xff;
	byteData[1] = (g_parts.reg_off>>8) & 0xff;
	byteData[2] = (g_parts.reg_off>>16) & 0xff;
	byteData[3] = (g_parts.reg_off>>24) & 0xff;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+12, &byteData[0], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+13, &byteData[1], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+14, &byteData[2], 1, 0);
	if(status)return status;
	status = sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, TESTDRV_SDIO_FUNC1_OFFSET+15, &byteData[3], 1, 0);
	if(status)return status;

	/* printk("exiting %s\n",__FUNCTION__); */
	return status;

} /* set_partition */

/*--------------------------------------------------------------------------------------*/


static int OMAP_TNETW_Power(int power_on)

{
  printk("%s: (-%s-)\n", __FUNCTION__, power_on ? "ON" : "OFF");
  
  if (power_on)
  {
	  gpio_set_value(PMENA_GPIO, 1);
  }
  else
  {
	  gpio_set_value(PMENA_GPIO, 0);
  }

  return 0;
    
} /* OMAP_TNETW_Power() */

/*--------------------------------------------------------------------------------------*/

static int OMAP_TNETW_HardReset(void)
{
	printk("%s: start\n",__FUNCTION__);
	OMAP_TNETW_Power(0);
	mdelay(50);
	OMAP_TNETW_Power(1);
	mdelay(50);
	printk("%s: end.\n",__FUNCTION__);
  return 0;

}


/*--------------------------------------------------------------------------------------*/
/* 
 * Copy from host_platform.c 
 */

static void pad_config(unsigned long pad_addr, u32 andmask, u32 ormask)
{
	int val;
	u32 *addr;

	addr = (u32 *) ioremap(pad_addr, 4);
	if (!addr) {
		printk(KERN_ERR "OMAP_pad_config: ioremap failed with addr %lx\n", pad_addr);
		return;
	}

	val =  __raw_readl(addr);
	val &= andmask;
	val |= ormask;
	__raw_writel(val, addr);

	iounmap(addr);
}

int hPlatform_Wlan_Hardware_Init(void)
{
	/*WLAN_IRQ - 53*/
	/* Should set (x is don't change):  xxxx xxx1 xxx0 0011 xxxx xxxx xxxx xxxx */
    pad_config( 0x4A100078, 0xFFECFFFF, 0x00030000);
    /*WLAN_EN - 54*/
   /* Should set (x is don't change):  xxxx xxxx xxxx xxxx xxxx xxxx xxx0 1x11 */
	pad_config(0x4A10007C, 0xFFFFFFEF, 0x0000000B);
	/* Should set (x is don't change):  xxxx xxx1 xxx1 1000 xxxx xxxx xxx0 0000 */
    /*SDIO_CLK + SDIO_CMD*/
	pad_config(0x4A100148, 0xFFF8FFE0, 0x01180000);
	/* SDIO_D0 + SDIO_D1*/
	/* Should set (x is don't change):  xxxx xxxx xxx1 1000 xxxx xxxx xxx1 1000 */
    pad_config(0x4A10014C, 0xFFF8FFF8, 0x00180018);
    /* SDIO_D2 / D3*/
	/* Should set (x is don't change):  xxxx xxxx xxx1 1000 xxxx xxxx xxx1 1000 */
    pad_config(0x4A100150, 0xFFF8FFF8, 0x00180018);
	return 0;
}


/*--------------------------------------------------------------------------------------*/
/*======================================================================================*/
/*--------------------------------------------------------------------------------------*/
/* This function allocates memory for all write buffers, and fill them with non-zero characters */
static int init_write_buf(void)
{
	int i, z;

	if ((write_buf = kmalloc(SDIO_TEST_MAX_R_W_BUF_ARRAY, __GFP_DMA)) == NULL)
	{
		kfree(write_buf);
		return -ENOMEM;
	}

	for(i=0 ; i<SDIO_TEST_R_W_BUFS_NUM ; i++)
	{
		/* fill write buffers with non-sero content */
		write_buf_array[i] = write_buf+i*SDIO_TEST_R_W_BUF_LEN;
		for (z=0; z<SDIO_TEST_R_W_BUF_LEN; z++)
			write_buf_array[i][z] = z%250 + 1;
	}

	return 0;
}

/* This function allocates memory for all write buffers, and fill them with zeros */
static int init_read_buf(void)
{
	int i;

	if ((read_buf = kmalloc(SDIO_TEST_MAX_R_W_BUF_ARRAY, __GFP_DMA)) == NULL)
	{
		kfree(read_buf);
		return -ENOMEM;
	}

	for(i=0 ; i<SDIO_TEST_R_W_BUFS_NUM ; i++)
	{
		/* zero read buffers */
		read_buf_array[i] = read_buf+i*SDIO_TEST_R_W_BUF_LEN;
		memset(read_buf_array[i],0,SDIO_TEST_R_W_BUF_LEN);
	}

	return 0;
}

/* This function allocates memory for all write & read buffers, 
	fills the read buffers with zeros and the write buffers with non-zeros characters */
static int init_buffers(void)
{
	if (init_write_buf() != 0)
	{
		printk("init_buffers: init_write_buf failed\n");
		return (-1);
	}
		
	if (init_read_buf() != 0)
	{
		printk("init_buffers: init_read_buf failed\n");
		return (-1);
	}

	return(0);

} /* init_buffers() */

/* This function zeros all read buffers */
static void zero_read_buf(void)
{
	int i;

	if ( read_buf == NULL )
	{
		printk("zero_read_buf: read_buf not allocated\n");
		return;
	}
	for(i=0 ; i<SDIO_TEST_R_W_BUFS_NUM ; i++)
	{
		read_buf_array[i] = read_buf+i*SDIO_TEST_R_W_BUF_LEN;
		memset(read_buf_array[i],0,SDIO_TEST_R_W_BUF_LEN);
	}

} /* zero_buffers() */

/* This function fills all write buffers with non-zero characters */
static void fill_write_buf(void)
{
	int i,z;

	if ( write_buf == NULL )
	{
		printk("fill_write_buf: write_buf not allocated\n");
		return;
	}
	for(i=0 ; i<SDIO_TEST_R_W_BUFS_NUM ; i++)
	{
		write_buf_array[i] = write_buf+i*SDIO_TEST_R_W_BUF_LEN;
		memset(write_buf_array[i],0,SDIO_TEST_R_W_BUF_LEN);
		for (z=0; z<SDIO_TEST_R_W_BUF_LEN; z++)
			write_buf_array[i][z] = z%250 + 1;
	}
} /* zero_buffers() */

/*--------------------------------------------------------------------------------------*/

static int sync_write_test(int number_of_transactions, int block_len)
{
	int i;
	int ret = 0;
	for(i=0;i<number_of_transactions;i++)
	{	
		printk("sync_write_test: i=%d\n", i);
	    ret = sdioDrv_WriteSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR, write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM], block_len, 1, 1);
        if(ret)
		{
			return -ENODEV;
		}
	}

	return ret;

} /* sync_write_test() */

/*--------------------------------------------------------------------------------------*/

static int sdio_test_write_async(unsigned long addr, char *buf, int length)
{
	int ret = sdioDrv_WriteAsync(TXN_FUNC_ID_WLAN, addr, buf, length, SDIO_BLOCK_NONE, 1, 1);

	if (down_interruptible(&g_sdio.sem))
	{
		printk(KERN_ERR "sdio_test_write_async() - FAILED !\n");
		ret = -ENODEV;
	}
	if (ret == 0)
	{
		ret = g_sdio.status;
	}

	return ret;

} /* sdio_test_write_async() */

/*--------------------------------------------------------------------------------------*/

static int async_write_test(int number_of_transactions, int block_len)
{
	int i;
	int ret = 0;
	for(i=0;i<number_of_transactions;i++)
	{
		printk("async_write_test: i=%d\n", i);
		ret = sdio_test_write_async(SDIO_TEST_FIRST_VALID_DMA_ADDR, write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM], block_len);
		if(ret){
			return -ENODEV;
		}
	}

	return ret;

} /* async_write_test() */

/*--------------------------------------------------------------------------------------*/

static int sync_read_test(int number_of_transactions, int block_len)
{
	int i;
	int ret = 0;
	for(i=0;i<number_of_transactions;i++){
		printk("sync_read_test: i=%d\n", i);
		ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR,read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM], block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
	}

	return ret;

} /* sync_read_test() */

/*--------------------------------------------------------------------------------------*/
/* This test reads aligned/non-aligned block in asynch way.
	originally the function read only aligned blocks in asynch way */
static int sdio_test_read_async(unsigned long addr, char *buf, int length)
{
	int ret = sdioDrv_ReadAsync(TXN_FUNC_ID_WLAN, addr, buf, length, SDIO_BLOCK_NONE, 1, 1);

	if (down_interruptible(&g_sdio.sem))
	{
		printk(KERN_ERR "sdio_test_read_async() - FAILED !\n");
		ret = -ENODEV;
	}
	if (ret == 0)
	{
		ret = g_sdio.status;
	}

	return ret;

} /* sdio_test_read_async() */

/*--------------------------------------------------------------------------------------*/

static int async_read_test(int number_of_transaction, int block_len)
{
	int i;
	int ret = 0;
	for(i=0;i<number_of_transaction;i++)
	{
		printk("async_read_test: i=%d\n", i);
		ret = sdio_test_read_async(SDIO_TEST_FIRST_VALID_DMA_ADDR, read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM], block_len);
		if(ret)
		{
			return -ENODEV;
		}
	}

	return ret;

} /* async_read_test() */

/*--------------------------------------------------------------------------------------*/

static int sync_compare_test(int number_of_transactions, int block_len)
{
	int i,j,ret = 0,*my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		ret = sdioDrv_WriteSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR, my_write_buf, block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
		ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR, my_read_buf, block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
		if (memcmp(my_write_buf,my_read_buf,block_len))
		{
		  printk("\nERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)my_write_buf,(int)my_read_buf,i);
		  for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
		  {
		    if ((*my_read_buf) != (*my_write_buf))
		    printk("  R[%d] = 0x%x, W[%d] = 0x%x\n", j, *my_read_buf, j, *my_write_buf);
		  }
		  return -1;
		}
	}

	return ret;

} /* sync_compare_test() */

static int sync_compare_test_no_write(int number_of_transactions, int block_len)
{
	int i,j,ret = 0,*my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[0];  //[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[0]; //[i%SDIO_TEST_R_W_BUFS_NUM];
		//ret = sdioDrv_WriteSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR, my_write_buf, block_len, 1, 1);
		//if(ret){
		//	return -ENODEV;
		//}
		ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, SDIO_TEST_FIRST_VALID_DMA_ADDR, my_read_buf, block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
		if (memcmp(my_write_buf,my_read_buf,block_len))
		{
		  printk("\nERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)my_write_buf,(int)my_read_buf,i);
		  for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
		  {
		    if ((*my_read_buf) != (*my_write_buf))
		        printk("  R[%d] = 0x%x, W[%d] = 0x%x\n", j, *my_read_buf, j, *my_write_buf);
		  }
		  return -1;
		}
	}

	return ret;

} /* sync_compare_test_no_write() */

static int sync_func0_test(int number_of_transactions, int block_len)
{
	int i,j,ret = 0,*my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		*my_write_buf = i;
		ret = sdioDrv_WriteSync(TXN_FUNC_ID_CTRL, 0xC0, my_write_buf, block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
		ret = sdioDrv_ReadSync(TXN_FUNC_ID_CTRL, 0xC0, my_read_buf, block_len, 1, 1);
		if(ret){
			return -ENODEV;
		}
		if (memcmp(my_write_buf,my_read_buf,block_len))
		{
		  printk("\nERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)my_write_buf,(int)my_read_buf,i);
		  for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
		  {
		    printk("  R[%d] = 0x%x, W[%d] = 0x%x\n", j, *my_read_buf, j, *my_write_buf);
		  }
		  return -1;
		}
	}

	return ret;

} /* sync_func0_test() */

static int sync_func0_read_test(int number_of_transactions, int block_len)
{
    int ret = 0;

    memset(write_buf_array[0], 0, SDIO_TEST_R_W_BUF_LEN);
    *(write_buf_array[0]) = 0x55;
    *(write_buf_array[0] + 1) = 0xaa;

    ret = sdioDrv_ReadSync(TXN_FUNC_ID_CTRL, 0xC0, read_buf_array[0], 4, 1, 1);
    if (ret) { return -ENODEV; }

    ret = sdioDrv_WriteSync(TXN_FUNC_ID_CTRL, 0xC0, write_buf_array[0], 2, 1, 1);
    if (ret) { return -ENODEV; }

    ret = sdioDrv_ReadSync(TXN_FUNC_ID_CTRL, 0xC0, read_buf_array[0], 4, 1, 1);
    if (ret) { return -ENODEV; }

    printk("C0 = 0x%x, C1 = 0x%x, C2 = 0x%x, C3 = 0x%x, \n", 
           *(read_buf_array[0]), *(read_buf_array[0]+1), *(read_buf_array[0]+2), *(read_buf_array[0]+3));

	return ret;

} /* sync_func0_test() */
static int sync_gen_compare_test(int number_of_transactions, unsigned int address, int block_len)
{
	int i,j;
	int ret_write 	= 0;
	int ret_read 	= 0;
	int *my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];

		ret_write = sdioDrv_WriteSync(TXN_FUNC_ID_WLAN, address, my_write_buf, block_len, 1, 1);
		if(ret_write)
		{
			printk("sync_gen_compare_test: sdioDrv_WriteSync failed\n");
			return -ENODEV;
		}
		ret_read = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, address, my_read_buf, block_len, 1, 1);
		if(ret_read)
		{
			printk("sync_gen_compare_test: sdioDrv_ReadSync failed\n");
			return -ENODEV;
		}

		/* If both read & write passed */
		if ((ret_write == 0) && (ret_read == 0))
		{
			if (memcmp(my_write_buf,my_read_buf,block_len))
			{
				printk("\nERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)my_write_buf,(int)my_read_buf,i);
				for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
				{
					printk("Addr = 0x%X,	R[%d] = 0x%x, W[%d] = 0x%x\n", address, j, *my_read_buf, j, *my_write_buf);
				}
				return -1;
			}
		}
	}

	return 0;

} /* sync_gen_compare_test() */
/*--------------------------------------------------------------------------------------*/

static int async_compare_test(int number_of_transactions, int block_len)
{
	int i,j,ret = 0,*my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		sdio_test_write_async(SDIO_TEST_FIRST_VALID_DMA_ADDR, (char*)my_write_buf, block_len);
		if(ret)
		{
			return -ENODEV;
		}
		ret =  sdio_test_read_async(SDIO_TEST_FIRST_VALID_DMA_ADDR,(char*)my_read_buf, block_len);
		if(ret)
		{
			return -ENODEV;
		}
		if (memcmp(my_write_buf,my_read_buf,block_len))
		{
		  printk("\nERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)my_write_buf,(int)my_read_buf,i);
		  for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
		  {
		    printk("  R[%d] = 0x%x, W[%d] = 0x%x\n", j, *my_read_buf, j, *my_write_buf);
		  }
		  return -1;
		}
	}

	return ret;
} /* async_compare_test() */

static int async_gen_compare_test(int number_of_transactions, unsigned int address, int block_len)
{
	int i,j;
	int ret_write 	= 0;
	int ret_read 	= 0;
	int *my_read_buf,*my_write_buf;

	for(i=0;i<number_of_transactions;i++)
	{
		my_read_buf  = (int *)read_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];
		my_write_buf = (int *)write_buf_array[i%SDIO_TEST_R_W_BUFS_NUM];

		ret_write = sdio_test_write_async(address, (char*)my_write_buf, block_len);
		if(ret_write)
		{
			printk("sync_gen_compare_test: sdioDrv_WriteSync failed\n");
			return -ENODEV;
		}
		ret_read =  sdio_test_read_async(address, (char*)my_read_buf, block_len);
		if(ret_read)
		{
			printk("sync_gen_compare_test: sdioDrv_ReadSync failed\n");
			return -ENODEV;
		}
		/* If both read & write passed */
		if ((ret_write == 0) && (ret_read == 0))
		{
			if (memcmp(my_write_buf,my_read_buf,block_len))
			{
				printk("\nasync_gen_compare_test, ERROR: Write(0x%x)/Read(0x%x) (%d) buffers are not identical!\n",(int)write_buf,(int)read_buf,i);
				for (j=0; j < block_len/4; j++,my_read_buf++,my_write_buf++)
				{
					printk("  R[%d] = 0x%x, W[%d] = 0x%x\n", j, *my_read_buf, j, *my_write_buf);
				}
				return -1;
			}
		}
	}

	return 0;

} /* async_gen_compare_test() */

/*--------------------------------------------------------------------------------------*/

static int perform_test(char* string, int (*test_func)(int, int), int number_of_transactions, int block_len)
{
	unsigned long exectime,basetime,endtime;
	int ret;

	printk("%s", string);
	basetime = jiffies;
	ret = test_func(number_of_transactions, block_len);
	endtime = jiffies;
	exectime = endtime>=basetime? endtime-basetime : 0xffffffff-basetime+endtime;
	exectime = jiffies_to_msecs(exectime);
	printk(": %d*%d bytes took %lu [msecs] ", number_of_transactions, block_len, exectime);
	if (exectime!=0)
		printk("=> %d [Mbps]\n", (int)((number_of_transactions*8*block_len)/(exectime*1000)));
	else
		printk("\n");

	return ret;

} /* perform_test() */

/*--------------------------------------------------------------------------------------*/

static int do_test_sync(unsigned long tests)
{
	int ret=0;

	if (tests & TESTDRV_READ_TEST)
	{
	  ret = perform_test("Starting sync read test", sync_read_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "sync read test");
	}
	if (tests & TESTDRV_WRITE_TEST)
	{
	  ret = perform_test("Starting sync write test", sync_write_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "sync write test");
	}
	if (tests & TESTDRV_COMPARE_TEST)
	{
	  ret = perform_test("Starting sync wr/rd compare test", sync_compare_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "sync wr/rd compare test");
	}
	if (tests & TESTDRV_FUNC0_TEST)
	{
	  ret = perform_test("Starting sync func-0 wr/rd compare test", sync_func0_test, 10000, 4);
	  DO_ON_ERROR(ret, "sync func-0 wr/rd compare test");
	}
	if (tests & TESTDRV_FUNC0_READ_TEST)
	{
	  ret = perform_test("Starting sync func-0 read test", sync_func0_read_test, 0, 0);
	  DO_ON_ERROR(ret, "sync func-0 read test");
	}

	return ret;

} /* do_test_sync() */

/*--------------------------------------------------------------------------------------*/

static int do_test_async(unsigned long tests)
{
	int ret=0;

	if (tests & TESTDRV_READ_TEST)
	{
	  ret = perform_test("Starting A-sync read test", async_read_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync read test");
	}
	if (tests & TESTDRV_WRITE_TEST)
	{
	  ret = perform_test("Starting A-sync write test", async_write_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync write test");
	}
	if (tests & TESTDRV_COMPARE_TEST)
	{
	  ret = perform_test("Starting A-sync wr/rd compare test", async_compare_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync wr/rd compare test");
	}

	return ret;

} /* do_test_async() */

static int do_test_async_data_mem(unsigned long tests)
{
	int ret=0;
	int status = 0;

	/* set Partition to Data Memory */
	status = set_partition(0,TESTDRV_MEM_DOWNLOAD_PART_SIZE,TESTDRV_DATA_RAM_PART_START_ADDR,TESTDRV_REG_DOWNLOAD_PART_SIZE,TESTDRV_REG_PART_START_ADDR);

	{
	  ret = perform_test("Starting A-sync Data Partition read test", async_read_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync read test");
	}
	{
	  ret = perform_test("Starting A-sync Data Partition write test", async_write_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync write test");
	}
	{
	  ret = perform_test("Starting A-sync Data Partition wr/rd compare test", async_compare_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync wr/rd compare test");
	}

	return ret;

} /* do_test_async_data_mem() */

static int do_test_async_code_mem(unsigned long tests)
{
	int ret=0;
	int status = 0;

	/* set Partition to Data Memory */
	status = set_partition(0,TESTDRV_MEM_DOWNLOAD_PART_SIZE,TESTDRV_CODE_RAM_PART_START_ADDR,TESTDRV_REG_DOWNLOAD_PART_SIZE,TESTDRV_REG_PART_START_ADDR);

	{
	  ret = perform_test("Starting A-sync Code Partition read test", async_read_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync read test");
	}
	{
	  ret = perform_test("Starting A-sync Code Partition write test", async_write_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync write test");
	}
	{
	  ret = perform_test("Starting A-sync Code Partition wr/rd compare test", async_compare_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync wr/rd compare test");
	}

	return ret;

} /* do_test_async_code_mem() */

static int do_test_async_packet_mem(unsigned long tests)
{
	int ret=0;
	int status = 0;

	/* set Partition to Data Memory */
	status = set_partition(0,TESTDRV_MEM_WORKING_PART_SIZE,TESTDRV_PACKET_RAM_PART_START_ADDR,TESTDRV_REG_WORKING_PART_SIZE,TESTDRV_REG_PART_START_ADDR);

	{
	  ret = perform_test("Starting A-sync Packet Partition read test", async_read_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync read test");
	}
	{
	  ret = perform_test("Starting A-sync Packet Partition write test", async_write_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync write test");
	}
	{
	  ret = perform_test("Starting A-sync Packet Partition wr/rd compare test", async_compare_test, 10000, TESTDRV_TESTING_DATA_LENGTH);
	  DO_ON_ERROR(ret, "A-sync wr/rd compare test");
	}

	return ret;

} /* do_test_async_packet_mem() */

static void sdio_test_is_alive(void)
{
	int ChipID = 0;

	sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, 0x16800 + 0x5674, &(ChipID), 4, 1, 1);
	printk("Read device ID via ReadSync: 0x%x\n", ChipID);


} /* sdio_test_is_alive() */


/*--------------------------------------------------------------------------------------*/

static unsigned long sdio_test_get_base(char target)
{

	unsigned long base;

	switch (target)
	{
		case 'r':		/* WLAN Registers */
		case 'R':
			base = mem_partition_size;
			break;

		case 'm':		/* WLAN Memory */
		case 'M':
			base = 0;
			break;

		default:
			base = 0;
			break;
	}

	return base;

} /* sdio_test_get_base() */

/*--------------------------------------------------------------------------------------*/

static void sdio_test_read(const char* buffer, unsigned long base, SdioSyncMode_e SyncMode, ESdioBlockMode eBlockMode)
{
	unsigned long addr, length, quiet,j;
	u8 *buf;

    if (base == mem_partition_size) /* register */
	{
	  sscanf(buffer, "%X", (unsigned int *)&addr);
	  length = sizeof(unsigned long);
	  quiet  = g_Quiet;
	}
	else
	{
	  sscanf(buffer, "%X %d", (unsigned int *)&addr, (unsigned int *)&length);
	  printk("Read %s from 0x%x length %d\n", (SyncMode == SdioSync) ? "sync" : "A-sync", (unsigned int)(addr), (unsigned int)length);	  
	  quiet =  g_Quiet;
	}	  

    addr  = addr + base;

    if (( buf = kmalloc(length*2,__GFP_DMA)) == NULL)
	{
	  printk(" sdio_test_read() kmalloc(%d) FAILED !!!\n",(unsigned int)length);
	  return;
	}

	if (SyncMode == SdioSync)
	{
		printk("%s: %d - I'm trying to read in Sync", __FUNCTION__,__LINE__);
		quiet = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, addr, buf, length, 1, 1);
	}
	else
	{
		printk("%s: %d - I'm trying to read in A-sync", __FUNCTION__,__LINE__);
		quiet = sdioDrv_ReadAsync(TXN_FUNC_ID_WLAN, addr, buf, length, eBlockMode, 1, 1);
	}

	if (!quiet)
	{
	  printk("The value at 0x%x is ", (unsigned int)(addr-base));
	  for (j=0; j < length/4; j++)
	  {
	    printk("0x%08x ", (unsigned int)*(unsigned int *)(buf+j*sizeof(unsigned int)));
	  }
	  printk("\n");
	}
	g_last_read = (unsigned int)*(unsigned int *)(buf);
	kfree(buf);

} /* sdio_test_read() */


static void sdio_test_read_byte(const char* buffer, unsigned long base)
{
	unsigned long addr, length, quiet,j;
	u8 *buf;

    if (base == mem_partition_size) /* register */
	{
	  sscanf(buffer, "%X", (unsigned int *)&addr);
	  length = sizeof(unsigned char);
	  quiet  = g_Quiet;
	}
	else
	{
	  sscanf(buffer, "%X %d", (unsigned int *)&addr, (unsigned int *)&length);
	  printk("Read from 0x%x length %d\n", (unsigned int)(addr), (unsigned int)length);	  
	  quiet =  g_Quiet;
	}	  

    addr  = addr + base;

    if (( buf = kmalloc(length*2,__GFP_DMA)) == NULL)
	{
	  printk(" sdio_test_read() kmalloc(%d) FAILED !!!\n",(unsigned int)length);
	  return;
	}

    for (j=0; j < length; j++)
	{
		quiet = sdioDrv_ReadSyncBytes(TXN_FUNC_ID_WLAN, addr + j, (buf + j), 1, 0);
	}

	if (!quiet)
	{
	  printk("The value at 0x%x is ", (unsigned int)(addr-base));
	  for (j=0; j < length; j++)
	  {
	    printk("0x%02x ", (unsigned int)*(buf+j));
	  }
	  printk("\n");
	}
	g_last_read = (unsigned int)*(unsigned int *)(buf);
	kfree(buf);

} /* sdio_test_read_byte() */


/*--------------------------------------------------------------------------------------*/
#define BUFLEN 1024

static void sdio_test_write(const char* buffer, unsigned int base, SdioSyncMode_e SyncMode, ESdioBlockMode eBlockMode)
{
	unsigned long  addr, buflen=4, j;
	u8 *value;
    unsigned long len;

	if (( value = kmalloc(BUFLEN*2,__GFP_DMA)) == NULL)
	{
	  printk("sdio_test_write: sdio_test_write() kmalloc(%d) FAILED !!!\n",(unsigned int)BUFLEN);
	  return;
	}
	sscanf(buffer, "%X %X %d", (unsigned int *)&addr, (unsigned int *)value, (unsigned int *)&len);

	if (!g_Quiet)
	{
		printk("sdio_test_write: Writing %X to %X  len %d\n", (unsigned int)*(unsigned int *)value, (unsigned int)addr,(unsigned int)len);
	}
	addr = addr + base;

	if (base != mem_partition_size) /* memory */
	{
	  buflen = len;

	  for (j=0; j < buflen*2-1 ; j++)
	  {
	    value[j+1] = value[0]+j+1;
	  }
	}
	if (SyncMode == SdioSync)
	{
	  	sdioDrv_WriteSync(TXN_FUNC_ID_WLAN, addr, (u8*)value, buflen, 1, 1);
	}
	else
	{
		sdioDrv_WriteAsync(TXN_FUNC_ID_WLAN, addr, (u8*)value, buflen, eBlockMode, 1, 1);
	}
	kfree(value);

} /* sdio_test_write() */

/* 
 * Special e commands
 * ------------------
 */
static int sdio_test_e(const char* buffer, char e_cmd, SdioSyncMode_e SyncMode, ESdioBlockMode eBlockMode)
{
    int itarations = 1;
    int len = 512;
    int ret=0;

    switch (e_cmd)
    {
        case '1':
            sscanf(buffer, "%d %d", (unsigned int *)&len, (unsigned int *)&itarations);
            printk("sdio_test_e e_cmd=%d: Call wr/rd/compare test len=%d, iter=%d\n", (unsigned int)e_cmd, len, itarations);
            ret = perform_test("Starting wr/rd/compare test", sync_compare_test, itarations, len);
            DO_ON_ERROR(ret, "Result wr/rd/compare test");
            break;

        case '2':
            sscanf(buffer, "%d %d", (unsigned int *)&len, (unsigned int *)&itarations);
            printk("sdio_test_e e_cmd=%d: Call    rd/compare test len=%d, iter=%d\n", (unsigned int)e_cmd, len, itarations);
            ret = perform_test("Starting    rd/compare test", sync_compare_test_no_write, itarations, len);
            DO_ON_ERROR(ret, "Result    rd/compare test");
            break;

        default:
            printk("e command usage:\n");
            printk("  e1 <len> <iterations> : call wr/rd/compare test\n");
            printk("  e2 <len> <iterations> : call    rd/compare test\n");

			break;
	}
	return ret;
} /* sdio_test_e() */

#if 0
/*
 * omap accesss
 */
static void sdio_access_omap(const char* buffer, char omap_cmd, SdioSyncMode_e SyncMode, ESdioBlockMode eBlockMode)
{
    unsigned long  addr;
    unsigned long  value, param;

    switch (omap_cmd)
    {
        case 'r':
            sscanf(buffer, "%X", (unsigned int *)&addr);
            value = omap_readl(addr);
            printk("sdio_access_omap: read omap register addr 0x%u = 0x%x\n", (uint)addr, (uint)value);
            break;

        case 'w':
            sscanf(buffer, "%X %X", (unsigned int *)&addr, (unsigned int *)&value);
            printk("sdio_access_omap: write omap register addr 0x%x = 0x%x\n", (uint)addr, (uint)value);
            omap_writel(value, addr);
            break;

        case 'c':
            value = omap_readl(MMCHS_SYSCONFIG);

            sscanf(buffer, "%d", (unsigned int *)&param);
            if (param == 0) /* not continueus clock - set AUTOIDLE bit 0 */
            {
                printk("sdio_access_omap: not continueus clock - set AUTOIDLE bit 0 in SYSCONFIG register\n");
                value |= 1;
            }
            else /* continueus clock - clear AUTOIDLE bit 0 */
            {
                printk("sdio_access_omap: continueus clock - clear AUTOIDLE bit 0 in SYSCONFIG register\n");
                value &= ~1;
            }
            printk("sdio_access_omap: write omap register SYSCONFIG addr 0x%x = 0x%x\n", (uint)addr, (uint)value);
            omap_writel(value, addr);
            break;

        case 'g':
            sscanf(buffer, "%d %d", (unsigned int *)&value, (unsigned int *)&param);
            printk("sdio_access_omap: set GPIO line %d, value %d\n", (int)value, (int)param);
			if (gpio_request(value, NULL) != 0) 
            
	        {
                printk("sdio_access_omap: ERROR on gpio_request %d\n", (int)value);
		        return;
	        };
			gpio_direction_output(value, TESTDRV_GPIO_INITIAL_VALUE_FOR_OUTPUT);
			gpio_set_value(value, param);
            gpio_free(value);
            break;

        case 'd':
            sscanf(buffer, "%d", (unsigned int *)&value);
            printk("sdio_access_omap: delay %d msec\n", (int)value);
            mdelay(value);
            break;

        default:
            printk("sdio_access_omap: cmd not supported\n");
			break;
	}
	return;
} /* sdio_access_omap() */
#endif
/*--------------------------------------------------------------------------------------*/
static void sdio_test_write_byte(const char* buffer, unsigned int base)
{
	unsigned long  addr, buflen=1, j;
	u8 *value;
    unsigned long len;


	if (( value = kmalloc(BUFLEN*2,__GFP_DMA)) == NULL)
	{
	  printk("sdio_test_write_byte: sdio_test_write() kmalloc(%d) FAILED !!!\n",(unsigned int)BUFLEN);
	  return;
	}

	sscanf(buffer, "%X %X %d", (unsigned int *)&addr, (unsigned int *)value, (unsigned int *)&len);

	if (!g_Quiet)
	{
		printk("sdio_test_write_byte: Writing %X to %X  len %d\n", (unsigned int)*(unsigned int *)value, (unsigned int)addr,*(unsigned int *)len);
	}
	addr = addr + base;

	if (base != mem_partition_size) /* memory */
	{
	  buflen = len;

	  for (j=0; j < buflen-1 ; j++)
	  {
	    value[j+1] = value[0]+j+1;
	  }
	}

    for (j=0; j < buflen; j++)
	{
	  	sdioDrv_WriteSyncBytes(TXN_FUNC_ID_WLAN, addr + j, value + j, 1, 0);
	}
	kfree(value);

} /* sdio_test_write_byte() */

/*--------------------------------------------------------------------------------------*/

static void sdio_test_reset(void)
{
    int err;

	disable_irq (TNETW_IRQ);
	err = OMAP_TNETW_HardReset(); 
	enable_irq (TNETW_IRQ);

	return;

} /* sdio_test_reset() */

/*--------------------------------------------------------------------------------------*/

void is_alive_test(void)
{
    unsigned long base;
	char   *cmd = "rr 5674";
	
	base = sdio_test_get_base(cmd[1]);
	sdio_test_read(&cmd[2], base, SdioSync, SDIO_BLOCK_NONE); 

} /* is_alive_test() */

/*--------------------------------------------------------------------------------------*/

void sdio_test_CB(void* Handle, int status)
{
  /*    DEBUG(LEVEL1,"enter\n"); */
	g_sdio.status = status;
	up(&g_sdio.sem);
	
} /* sdio_test_CB() */

/*--------------------------------------------------------------------------------------*/

static void  sdio_test_interrupt(void)
{
    unsigned long base;
    char   *cmd = "wr 4f4 1";

	base = sdio_test_get_base(cmd[1]);
	sdio_test_write(&cmd[2], base, SdioSync, SDIO_BLOCK_NONE); 

} /* sdio_test_interrupt() */

/*--------------------------------------------------------------------------------------*/

static void  sdio_test_init(void)
{
	int                 ChipID = 0, status;
    unsigned long       base;
    char                *cmd;
	int 				rc=0;

	rc = sdioAdapt_ConnectBus (sdio_test_CB, NULL, 9, 90);
	if (rc != 0) {
		printk("sdio_test_init ERROR: rc =%d\n", rc);

		return;
	}

	/* Set Registers Partition */
	printk("In sdio_test_init: going to perform set_partition. mem size = 0x%X, mem addr =  0x%X, reg size = 0x%X, reg addr = 0x%X\n",mem_partition_size,TESTDRV_CODE_RAM_PART_START_ADDR,reg_partition_size,TESTDRV_REG_PART_START_ADDR);

	printk("Running set_partition status = "); 
	status = set_partition(0,mem_partition_size,TESTDRV_CODE_RAM_PART_START_ADDR,reg_partition_size,TESTDRV_REG_PART_START_ADDR);
	printk("%d %s\n", status, status ? "NOT OK" : "OK"); 
	
	sdioDrv_ReadSyncBytes(TXN_FUNC_ID_WLAN, 0x16800 + 0x5674, (unsigned char *)&(ChipID), 1, 0);
	sdioDrv_ReadSyncBytes(TXN_FUNC_ID_WLAN, 0x16800 + 0x5675, (unsigned char *)&(ChipID)+1, 1, 0);
	sdioDrv_ReadSyncBytes(TXN_FUNC_ID_WLAN, 0x16800 + 0x5676, (unsigned char *)&(ChipID)+2, 1, 0);
	sdioDrv_ReadSyncBytes(TXN_FUNC_ID_WLAN, 0x16800 + 0x5677, (unsigned char *)&(ChipID)+3, 1, 0);

    printk("Ya Hoo we just Read device ID via ReadByte: 0x%x\n", ChipID);

#if 0 /*Benzy: Generate Interrupt*/
	/* HI_CFG - host interrupt active high */
	cmd = "wr 808 b8";
	base = sdio_test_get_base(cmd[1]);
	sdio_test_write(&cmd[2], base, SdioSync, SDIO_BLOCK_NONE); 
	/* HINT_MASK - unmask all */
	cmd = "wr 4dc FFFFFFFE";
	sdio_test_write(&cmd[2], base, SdioSync, SDIO_BLOCK_NONE); 
#endif
#ifdef TNETW1283
#ifdef SDIO_HIGH_SPEED
	{
		int            		iStatus;
		unsigned char  		uByte;

		uByte = 0;
		PERR("sdioAdapt_ConnectBus: Set HIGH_SPEED bit on register 0x13\n");
		/* CCCR 13 bit EHS(1) */
		iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, 0xc0, &uByte, 1, 1);
		PERR2("After r 0x%x, iStatus=%d \n", uByte, iStatus );
		if (iStatus) { return; }
	
		uByte |= 0x1; /* set bit #1 EHS */
		iStatus = sdioDrv_WriteSyncBytes (TXN_FUNC_ID_CTRL, 0xc0, &uByte, 1, 1);
		PERR2("After w 0x%x, iStatus=%d \n", uByte, iStatus );
		if (iStatus) { return; }
	
		iStatus = sdioDrv_ReadSyncBytes (TXN_FUNC_ID_CTRL, 0xc0, &uByte, 1, 1);
		PERR2("After r 0x%x, iStatus=%d \n", uByte, iStatus );
		if (iStatus) { return; }
	
		PERR1("After CCCR 0xc0, uByte=%d \n", (int)uByte );
	
		/*enable High speed on OMAP side*/
		OMAP_HSMMC_WRITE( HCTL, OMAP_HSMMC_READ(HCTL) | 0x4);
	
		host_set_default_sdio_clk(1); /*2=24Mhz 96==1Mhz 240 = 400KHz*/
	}
#endif
#endif

} /* sdio_test_init() */

static void  sdio_test_init_connect_bus(void)
{
	printk("Connect bus only\n");
	sdioAdapt_ConnectBus_0537only (sdio_test_CB, NULL, 9, 90);

} /* sdio_test_init() */

/*--------------------------------------------------------------------------------------*/

static void usage(void)
{
  printk("\nSDIO Test proc usage: \n");
  printk("----------------------- \n\n");
  printk("    rr\\a   addr              - read register \\async\n");
  printk("    rm\\a\\b addr length       - read memory \\async \\block mode, length (align to 4) in bytes, block mode just in async \n");
  printk("    wr\\a   addr value        - write register \\async\n");
  printk("    wm\\a\\b addr value length - write memory and increment value \\async \\block mode, length (align to 4) in bytes, block mode just in async\n\n");

  printk("    brr   addr              - read register byte (cmd 52)\n");
  printk("    brm   addr length       - read memory byte (cmd 52), length in bytes\n");
  printk("    bwr   addr value        - write register byte (cmd 52)\n");
  printk("    bwm   addr value length - write memory byte and increment value (cmd 52), length in bytes\n\n");

  printk("    sr | sw | sc            - sync  read/write/compare test\n");
  printk("    ar | aw | ac            - async read/write/compare test\n\n");

  printk("    cs\\i                    - sync SDIO complete test \\Infinite\n");
  printk("    ca\\i                    - async SDIO complete test \\Infinite\n");
  printk("    ct\\i                    - sync & async SDIO complete test \\Infinite  \n\n");
 
  printk("    er    addr              - error read memory,  running a second CMD53 immediately after the first one(for abort transaction test)\n");
  printk("    ew    addr value        - error write memory, running a second CMD53 immediately after the first one(for abort transaction test)\n\n");

  printk("    na\\i                    - async SDIO non-sligned complete test \\Infinite\n");
  printk("    ns\\i                    - sync SDIO non-sligned complete test \\Infinite \n");
  printk("    nt\\i                    - sync & async SDIO non-sligned complete test \\Infinite\n\n");

  printk("    h                       - help\n");
  printk("    t                       - TENETW reset\n");
  printk("    i                       - init\n");
  printk("    y                       - init only cmd 0, 5, 3, 7\n");
  printk("    z     divider           - SDIO CLK=(Main clk / divider( 9bit)).  Main clk = 96M\n");

  printk("    or    addr              - read OMAP register\n");
  printk("    ow    addr value        - write OMAP register\n");
  printk("    oc    param             - set continueus clock (1) or not (0) - AUTOIDLE bit 0 in SYSCLOCK register\n");
  printk("    og    gpio_line value   - set GPIO line\n");
  printk("    od    delay_msec        - delay in msec\n");

  printk("    e1    len count         - write/read/compare memory with variable length and number of iterations\n");
  printk("    e2    len count         -       read/compare memory with variable length and number of iterations\n");

  printk("    p     memOffset memSize regOffset regSize - set partitions\n\n");

  printk("    fr    function addr       - read register byte (cmd 52) from function 0\\1\\2\n");
  printk("    fw    function addr value - write register byte (cmd 52) to function 0\\1\\2\n\n");

  printk("    md                      - w|r|c  to Data Memory async\n");
  printk("    mc                      - w|r|c  to Code Memory async\n");
  printk("    mp                      - w|r|c  to Packet Memory async\n");

  printk("    l                       - keep alive test\n");

  printk("    q                       - 0: disable quite mode, 1: enable quite mode\n");


} /* usage */

/*--------------------------------------------------------------------------------------*/
/* The following fuctions are for SDIO Memory Complete Test */
/*--------------------------------------------------------------------------------------*/

/* This function calculates how many iterations have to be made in order to go over
	all full blocks of size block_zise (not partiol blocks) in memory of size mem_zise */
static int sdio_test_calc_iterations_no(int mem_zise, int block_zise)
{
	int iteration_no = 1;

	if ( mem_zise > block_zise )
	{
		if (( mem_zise % block_zise ) == 0 )
		{
			iteration_no = 0;
		}

		iteration_no += mem_zise / block_zise;
	}

	return(iteration_no);

} /* sdio_test_calc_iterations_no */


/* Perform Write - Read - Compare Test of i blocks */
static int sdio_test_compare_blocks(unsigned int mem_test_size, unsigned int block_size, SdioSyncMode_e SyncMode)
{
	int block_remain_size 	= 0;
	unsigned long addr 		= 0;
	int blocks_no			= 0;
	int ret 				= 0;
	int j;


	/* check parameters */
	if (( block_size > TESTDRV_MAX_SDIO_BLOCK ) || ( SyncMode >= SdioSyncMax ))
	{
		printk("ERROR:	sdio_test_compare_blocks: Invalid Parameter\n");
		return (-1);
	}

	blocks_no = sdio_test_calc_iterations_no (mem_test_size,block_size);
	if ( blocks_no <= 0 )
	{
		printk("ERROR:	sdio_test_compare_blocks: blocks_no < 1\n");
		return(-1);
	}

	for ( j = 0 ; j < (blocks_no-1) ; j++ )
	{
		addr = j*block_size;

		if (SyncMode == SdioSync)
		{
			if ( addr < SDIO_TEST_FIRST_VALID_DMA_ADDR )
			{
				if ( block_size > SDIO_TEST_FIRST_VALID_DMA_ADDR )
				{
					ret = sync_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, SDIO_TEST_FIRST_VALID_DMA_ADDR, block_size-SDIO_TEST_FIRST_VALID_DMA_ADDR);
				}
			}
			else
			{
				ret = sync_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, addr, block_size);
			}				
		}
		else
		{
			if ( addr < SDIO_TEST_FIRST_VALID_DMA_ADDR )
			{
				if ( block_size > SDIO_TEST_FIRST_VALID_DMA_ADDR )
				{
					ret = async_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, SDIO_TEST_FIRST_VALID_DMA_ADDR, block_size-SDIO_TEST_FIRST_VALID_DMA_ADDR);
				}
			}
			else
			{
				ret = async_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, addr, block_size);
			}				
		}
	}

	/* Test Last Block - if Exists */
	/*printk("In sdio_test_compare_blocks: test last block\n"); */

	block_remain_size = mem_test_size % block_size;
	if ( block_remain_size > 0 )
	{
		addr = (blocks_no-1)*block_size;

		/* write Last block to Mem */
		if (SyncMode == SdioSync)
		{
			if ( addr < SDIO_TEST_FIRST_VALID_DMA_ADDR )
			{
				if ( (block_size-SDIO_TEST_FIRST_VALID_DMA_ADDR) > 0 )
				{
					ret = sync_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, SDIO_TEST_FIRST_VALID_DMA_ADDR, block_remain_size-SDIO_TEST_FIRST_VALID_DMA_ADDR);
				}
			}
			else
			{
				ret = sync_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, addr, block_remain_size);
			}				
		}
		else
		{
			if ( addr < SDIO_TEST_FIRST_VALID_DMA_ADDR )
			{
				if ( (block_size-SDIO_TEST_FIRST_VALID_DMA_ADDR) > 0 )
				{
					ret = async_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, SDIO_TEST_FIRST_VALID_DMA_ADDR, block_size-SDIO_TEST_FIRST_VALID_DMA_ADDR);
				}
			}
			else
			{
				ret = async_gen_compare_test(SDIO_TEST_NO_OF_TRANSACTIONS, addr, block_remain_size);
			}				
		}
	}

	return (0);

} /* sdio_test_compare_blocks */

/* Perform Read Test of i blocks */
static int sdio_test_read_blocks(unsigned int mem_test_size, unsigned int block_size, SdioSyncMode_e SyncMode)
{
	int block_remain_size 	= 0;
	int addr 				= 0;
	int j;
	int blocks_no			= 0;
	int ret 				= 0;

	/* check parameters */
	if (( block_size > TESTDRV_MAX_SDIO_BLOCK ) ||
		( SyncMode >= SdioSyncMax ))
	{
		printk("ERROR:	sdio_test_read_blocks: Invalid Parameter\n");
		return (-1);
	}

	blocks_no = sdio_test_calc_iterations_no (mem_test_size,block_size);
	if ( blocks_no <= 0 )
	{
		printk("ERROR:	sdio_test_read_blocks: blocks_no < 1\n");
		return(-1);
	}
	for ( j = 0 ; j < (blocks_no-1) ; j++ )
	{
		addr = j*block_size;

		/* Read block from Mem */
		if (SyncMode == SdioSync)
		{
			ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, addr, (u8*)complete_test_read_buf, block_size, 1, 1);
		}
		else
		{
			ret = sdio_test_read_async(addr,(char*)complete_test_read_buf,block_size);
		}
		if ( ret != 0 )
		{
			/* Report Error, but continue the Test */ 
			printk("ERROR:	sdio_test_read_blocks: ERROR - Read buffer failed!\n");
		}

	} /* for ( j = 0 ; j < (blocks_no-1) ; j++ ) */

	/* Test Last Block - if Exists */
	block_remain_size = mem_test_size % block_size;
	if ( block_remain_size > 0 )
	{
		addr = (blocks_no-1)*block_size;

		/* Read block from Mem */
		if (SyncMode == SdioSync)
		{
			ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN, addr, (u8*)complete_test_read_buf, block_remain_size, 1, 1);
		}
		else
		{
			ret = sdio_test_read_async(addr,(char*)complete_test_read_buf,block_size);
		}
		if ( ret != 0 )
		{
			/* Report Error, but continue the Test */ 
			printk("ERROR:	sdio_test_read_blocks: ERROR - ****Last**** Read buffer failed!\n");
		}
	}

	return (0);

} /* sdio_test_read_blocks */

/* Perform Write - Read - Compare Test */
static int sdio_test_complete_mem(unsigned int mem_test_start,
								  unsigned int mem_test_size, 
								  unsigned int block_test_size, 
								  SdioSyncMode_e SyncMode)
{
	int i,status;
	int mem_remain_size = 0;
	int iteration_no	= 0;


	/* check parameters */
	if (( mem_test_size == 0 ) ||
		( block_test_size > TESTDRV_MAX_SDIO_BLOCK ) ||
		( SyncMode >= SdioSyncMax ))
	{
		printk("ERROR:	sdio_test_complete_mem: invalid Parameter\n");
		return (-1);
	}

	/* Calculate Number of Iterations. Each Iteration Is At Most the size of one memory Pratition */
	iteration_no = sdio_test_calc_iterations_no(mem_test_size,mem_partition_size);
	if ( iteration_no <= 0 )
	{
		printk("ERROR:	sdio_test_complete_mem: iteration_no < 1\n");
		return (-1);
	}
	/* Go over all Ram Memory */
	for ( i = 0 ; i < (iteration_no-1) ; i++)
	{
		/* Set Memory Partition at the Start of each Iteration */
		status = set_partition(0,mem_partition_size,(mem_test_start + i*mem_partition_size),reg_partition_size,TESTDRV_REG_PART_START_ADDR);
		if ( status )
		{
			printk("ERROR:	sdio_test_complete_mem: Set Partition to addr 0x%X on Iteration %d failed\n",mem_test_start + mem_test_size - mem_remain_size,i);
			return (-1);
		}

		sdio_test_compare_blocks(mem_partition_size,block_test_size,SyncMode);
	}
	/* Go over Last Interation */
	mem_remain_size = mem_test_size - mem_partition_size*(iteration_no-1);
	if ( mem_remain_size > 0 )
	{
		/* Set Memory Partition at the Start of each Iteration */
		status = set_partition(0,mem_remain_size,(mem_test_start + mem_test_size - mem_remain_size),reg_partition_size,TESTDRV_REG_PART_START_ADDR);
		if ( status )
		{
			printk("ERROR:	sdio_test_complete_mem: Set Partition to addr 0x%X on Iteration %d failed\n",mem_test_start + mem_test_size - mem_remain_size,i);
			return (-1);
		}
		return (sdio_test_compare_blocks(mem_remain_size,block_test_size,SyncMode));
	}

	return(0);

} /* sdio_test_complete_mem */

/* Perform Read from Regs Test */
static int sdio_test_complete_reg(unsigned int mem_test_size, unsigned int block_test_size, SdioSyncMode_e SyncMode)
{
	int i;
	int mem_remain_size = 0;
	int iteration_no	= 0;

	/* check parameters */
	if (( mem_test_size == 0 ) ||
		( block_test_size > TESTDRV_MAX_SDIO_BLOCK ) ||
		( SyncMode >= SdioSyncMax ))
	{
		printk("ERROR:	sdio_test_complete_reg: invalid Parameter\n");
		return (-1);
	}

	iteration_no = sdio_test_calc_iterations_no(mem_test_size,reg_partition_size);
	if ( iteration_no <= 0 )
	{
		printk("ERROR:	sdio_test_complete_reg: iteration_no < 1\n");
		return (-1);
	}
	/* Go over all Ram Memory */
	for ( i = 0 ; i < (iteration_no-1) ; i++)
	{
		sdio_test_read_blocks(mem_test_size,block_test_size,SyncMode);
	}
	/* Go over Last Interation */
	mem_remain_size = mem_test_size - mem_test_size*(iteration_no-1);
	if ( mem_remain_size > 0 )
	{
		return (sdio_test_read_blocks(mem_remain_size,block_test_size,SyncMode));
	}

	return 0; /* ??? */

} /* sdio_test_complete_reg */


/*	This is a complete Test of SDIO. 
	The Test checks all the Memory Partition (Read/Write/Compare) 
	and all the Register Partition (Read) */
static void sdio_test_complete(unsigned int block_size, SdioSyncMode_e SyncMode)
{
	/* check parameters */
	if (( SdioAsync >= SdioSyncMax) || ( block_size > TESTDRV_MAX_SDIO_BLOCK ))
	{
		printk("sdio_test_complete: invalid Parameter\n");
		return;
	}

	mem_partition_size = TESTDRV_MEM_DOWNLOAD_PART_SIZE;
	reg_partition_size = TESTDRV_REG_DOWNLOAD_PART_SIZE;

	/*--------------------------*/
	/* Test Memory RAM		 	*/
	/*--------------------------*/
	/* Test Memory RAM: 
	Go over all Memory Partitions and for each addrs Perform 3 Times W & R with compare */

	/*--------------------------*/
	/* Test Code Ram Partition	*/
	/*--------------------------*/
	if (sdio_test_complete_mem(TESTDRV_CODE_RAM_PART_START_ADDR,TESTDRV_CODE_RAM_SIZE,block_size,SyncMode) != 0)
	{
		printk("ERROR:	Test Code Ram Failed\n");
	}
	else
	{
		printk("Test Code Ram Succeed\n");
	}

	/*--------------------------*/
	/* Test Data Ram Partition	*/
	/*--------------------------*/
	if (sdio_test_complete_mem(TESTDRV_DATA_RAM_PART_START_ADDR,TESTDRV_DATA_RAM_SIZE,block_size,SyncMode) != 0)
	{
		printk("ERROR:	Test Data Ram Failed\n");
	}
	else
	{
		printk("Test Data Ram Succeed\n");
	}

	/*--------------------------*/
	/* Test Packet Ram Partition*/
	/*--------------------------*/
	mem_partition_size = TESTDRV_MEM_WORKING_PART_SIZE;
	reg_partition_size = TESTDRV_REG_WORKING_PART_SIZE;
	if (sdio_test_complete_mem(TESTDRV_PACKET_RAM_PART_START_ADDR,TESTDRV_PACKET_RAM_SIZE,block_size,SyncMode) != 0)
	{
		printk("ERROR:	Test Packet Ram Failed\n");
	}
	else
	{
		printk("Test Packet Ram Succeed\n");
	}

	/*--------------------------*/
	/* Test Registers Partition */
	/*--------------------------*/
	if (sdio_test_complete_reg(reg_partition_size,block_size,SyncMode) !=  0)
	{
		printk("ERROR:	Test Registers Ram Failed\n");
	}
	else
	{
		printk("Test Registers Ram Succeed\n");
	}

} /* sdio_test_complete() */

/* This Test perform sdio_test_complete on differnt block sizes - in async and once only */
static void sdio_test_once_async_complete(void)
{
	printk("Perform Async Complete SDIO\n");
	printk("************	SDIO Complete Async Test, 4 byte block	************\n");
	sdio_test_complete(4, SdioAsync);
	printk("************	SDIO Complete Async Test, 64 byte block	************\n");
	sdio_test_complete(64, SdioAsync);
	printk("************	SDIO Complete Async Test, 128 byte block	************\n");
	sdio_test_complete(128, SdioAsync);
	printk("************	SDIO Complete Async Test, 256 byte block	************\n");
	sdio_test_complete(256, SdioAsync);
	printk("************	SDIO Complete Async Test,384 byte block	************\n");
	sdio_test_complete(384, SdioAsync);
	printk("************	SDIO Complete Async Test, 512 byte block	************\n");
	sdio_test_complete(TESTDRV_512_SDIO_BLOCK, SdioAsync);	/* 0 equivalent to 512 bytes block */
}

/* This Test perform sdio_test_complete on differnt block sizes - in async and for ever */
static void sdio_test_infinit_async_complete(void)
{
	/* Loop Forever */
	while(1)
	{  	
		sdio_test_once_async_complete();	
	}
}

/* This Test perform sdio_test_complete on differnt block sizes - in sync and once only */
static void sdio_test_once_sync_complete(void)
{
	printk("Perform Synch Complete SDIO\n");
	printk("************	SDIO Complete Test, 4 byte block	************\n");
	sdio_test_complete(4, SdioSync);
	printk("************	SDIO Complete Sync Test, 64 byte block	************\n");
	sdio_test_complete(64, SdioSync);
	printk("************	SDIO Complete Sync Test, 128 byte block	************\n");
	sdio_test_complete(128, SdioSync);
	printk("************	SDIO Complete Sync Test, 256 byte block	************\n");
	sdio_test_complete(256, SdioSync);
	printk("************	SDIO Complete Sync Test,384 byte block	************\n");
	sdio_test_complete(384, SdioSync);
	printk("************	SDIO Complete Sync Test, 512 byte block	************\n");
	sdio_test_complete(TESTDRV_512_SDIO_BLOCK, SdioSync);	/* 0 equivalent to 512 bytes block */
}

/* This Test perform sdio_test_complete on differnt block sizes - in sync and for ever */
static void sdio_test_infinit_sync_complete(void)
{
	/* Loop Forever */
	while(1)
	{  	
		sdio_test_once_sync_complete();	
	}
}

/* This Test perform sdio_test_complete on differnt block sizes - in sync & async and once only */
static void sdio_test_once_complete(void)
{
	sdio_test_once_async_complete();

	sdio_test_once_sync_complete();
}

/* This Test perform sdio_test_complete on differnt block sizes - in sync & async and forever */
static void sdio_test_infinit_complete(void)
{
	while(1)
	{
		sdio_test_once_complete();	
	}
}

/*--------------------------------------------------------------------------------------*/
/* The following functions are for test which checks reading from not-aligned addresses */
/*--------------------------------------------------------------------------------------*/

/* Perform aligned Write of i blocks */
static int sdio_test_write_non_aligned_blocks(unsigned int block_size, 
											  SdioSyncMode_e SyncMode)
{
	unsigned long addr 		= 0;
	int blocks_no			= 0;
	int ret 				= 0;
    int j,i;


	/* check parameters */
	if (( block_size > TESTDRV_MAX_SDIO_BLOCK ) || 
		( block_size%SDIO_TEST_ALIGNMENT != 0) ||
		( SyncMode >= SdioSyncMax ) ||
		( SDIO_TEST_MAX_R_W_BUF_ARRAY+2*block_size > mem_partition_size ))
	{
		printk("ERROR:	sdio_test_write_non_aligned_blocks: Invalid Parameter\n");
		return (-1);
	}
	if ( SDIO_TEST_R_W_BUF_LEN%block_size != 0 )
	{
		printk("ERROR:	sdio_test_write_non_aligned_blocks: block_size not aligned\n");
		return (-1);
	}

	fill_write_buf();
	blocks_no = SDIO_TEST_R_W_BUF_LEN / block_size;

	for (j = 0 ; j < SDIO_TEST_R_W_BUFS_NUM ; j++)
	{
		for (i = 0 ; i < blocks_no ; i++)
		{
			char* temp_write_buf = write_buf_array[j] + i*block_size;
			/* address in memory: will not begine in 0x0 */
			addr = j*SDIO_TEST_R_W_BUF_LEN + (i+1)*block_size;

			/* write Synch */
			if (SyncMode == SdioSync)
			{
				/* write non-sligned block */
				ret = sdioDrv_WriteSync(TXN_FUNC_ID_WLAN,
										addr, 
										temp_write_buf, 
										block_size, 1, 1);
				if(ret)
				{
					printk("sdio_test_write_non_aligned_blocks: sdioDrv_WriteSync failed\n");
					return -ENODEV;
				}
			}				
			/* write A-Synch */
			else
			{
				ret = sdio_test_write_async(addr, 
											temp_write_buf, 
											block_size);
				if(ret)
				{
					printk("sdio_test_write_non_aligned_blocks: sdio_test_write_async failed\n");
					return -ENODEV;
				}
			}
		}
	}

	return (0);

} /* sdio_test_write_non_aligned_blocks */

/* Perform non-aligned Read (and Compere to Wrote values) of i blocks */
static int sdio_test_read_compare_non_aligned_blocks(unsigned int block_size, 
													 SdioSyncMode_e SyncMode, 
													 int alignDec)
{
	unsigned long addr 		= 0;
	int blocks_no			= 0;
	int ret 				= 0;
	int j,i,k;


	/* check parameters */
	if (( block_size > TESTDRV_MAX_SDIO_BLOCK ) || 
		( block_size%SDIO_TEST_ALIGNMENT != 0) ||
		( SyncMode >= SdioSyncMax ) ||
		( SDIO_TEST_MAX_R_W_BUF_ARRAY+2*block_size > mem_partition_size ))
	{
		printk("ERROR:	sdio_test_read_compare_non_aligned_blocks: Invalid Parameter\n");
		return (-1);
	}
	if ( SDIO_TEST_R_W_BUF_LEN%block_size != 0 )
	{
		printk("ERROR:	sdio_test_read_compare_non_aligned_blocks: block_size not aligned\n");
		return (-1);
	}

	zero_read_buf();
	blocks_no = SDIO_TEST_R_W_BUF_LEN / block_size;

	for (j = 0 ; j < SDIO_TEST_R_W_BUFS_NUM ; j++)
	{
		for (i = 0 ; i < blocks_no ; i++)
		{
			char* temp_read_buf = read_buf_array[j] + i*block_size;
			/* address in memory: will not begine in 0x0 and will be not-aligned */
			addr = j*SDIO_TEST_R_W_BUF_LEN + (i+1)*block_size + alignDec;

			/* read Synch */
			if (SyncMode == SdioSync)
			{
				/* write non-sligned block */
				ret = sdioDrv_ReadSync(TXN_FUNC_ID_WLAN,
									   addr, 
									   temp_read_buf, 
									   block_size - alignDec, 1, 1);
				if(ret)
				{
					printk("sdio_test_read_non_aligned_blocks: sdioDrv_ReadSync failed\n");
					return -ENODEV;
				}
			}				
			/* read A-Synch */
			else
			{
				ret = sdio_test_read_async(addr, 
										   temp_read_buf, 
										   block_size - alignDec);
				if(ret)
				{
					printk("sdio_test_read_compare_non_aligned_blocks: sdio_test_read_async failed\n");
					return -ENODEV;
				}
			}
		}
	}

	/* check read buffers */
	for (j = 0 ; j < SDIO_TEST_R_W_BUFS_NUM ; j++)
	{
		for ( i = 0 ; i < blocks_no ; i++)
		{
			char* temp_read_buf_ptr = read_buf_array[j] + i*block_size;
			char* temp_write_buf_ptr = write_buf_array[j] + i*block_size + alignDec;

			for ( k = 0 ; k < (block_size - alignDec) ; k++ )
			{
				if ( temp_read_buf_ptr[k] != temp_write_buf_ptr[k])
				{
					printk("sdio_test_read_compare_non_aligned_blocks: read & write mismatch!\n");
					printk("index = %d	;	read: 0x%X	;	write: 0x%X\n",k,temp_read_buf_ptr[k], temp_write_buf_ptr[k]);
					return (-ENODEV);
				}
			}
			for ( k = block_size - alignDec ; k < block_size ; k++ )
			{
				if ( temp_read_buf_ptr[k] != 0 )
				{
					printk("read buf supposed to be 0, but its not!\n");
					printk("read buf: 0x%X\n",temp_read_buf_ptr[k]);
					return (-ENODEV);
				}
			}
		}
	}

	return (0);

} /* sdio_test_read_compare_non_aligned_blocks */

/* This function performs test of reading from not-aligned addresses once 
	(sync/async mode is passed to the function) */
static int sdio_test_once_not_aligned(unsigned int block_size, 
									  SdioSyncMode_e SyncMode,
									  int alignDec)
{
	int ret = 0;

	ret = sdio_test_write_non_aligned_blocks(block_size, SyncMode);
	if ( ret != 0 )
	{
		printk("sdio_test_once_not_aligned: sdio_test_write_non_aligned_blocks failed\n");
		return(ret);
	}

	return (sdio_test_read_compare_non_aligned_blocks(block_size, SyncMode, alignDec));
}


/*	This is a complete Test of reading from Not-Aligned addresses. 
	This function performs test of reading from not-aligned addresses - from all different Memory Sections
	(DATA/CODE/PACKET),(sync/async mode is passed to the function) */
static void sdio_test_non_aligned_complete(unsigned int block_size, 
										   SdioSyncMode_e SyncMode,
										   int alignDec)
{
	int status = 0;

	/* check parameters */
	if (( SdioAsync >= SdioSyncMax) || 
		( block_size > TESTDRV_MAX_SDIO_BLOCK ) ||
		( alignDec < 1 ) || 
		( alignDec >= SDIO_TEST_ALIGNMENT ))
	{
		printk("sdio_test_non_aligned_complete: invalid Parameter\n");
		return;
	}


	mem_partition_size = TESTDRV_MEM_DOWNLOAD_PART_SIZE;
	reg_partition_size = TESTDRV_REG_DOWNLOAD_PART_SIZE;

	/*--------------------------*/
	/* Test Memory RAM		 	*/
	/*--------------------------*/
	/* Test Memory RAM: 
	Go over all Memory Partitions and for each addrs Perform 3 Times W & R with compare */

	/*--------------------------*/
	/* Test Code Ram Partition	*/
	/*--------------------------*/
	/* Set Memory Partition */
	status = set_partition(0,
					   mem_partition_size,
					   TESTDRV_CODE_RAM_PART_START_ADDR,
					   reg_partition_size,
					   TESTDRV_REG_PART_START_ADDR);
	if ( status )
	{
		printk("ERROR:	sdio_test_non_aligned_complete: set_partition failed\n");
		return;
	}
	if (sdio_test_once_not_aligned(block_size,SyncMode,alignDec) != 0)
	{
		printk("ERROR:	Test Code Ram not aligned Failed\n");
	}
	else
	{
		printk("Test Code Ram not aligned Succeed\n");
	}

	/*--------------------------*/
	/* Test Data Ram Partition	*/
	/*--------------------------*/
	/* Set Memory Partition */
	status = set_partition(0,
					   mem_partition_size,
					   TESTDRV_DATA_RAM_PART_START_ADDR,
					   reg_partition_size,
					   TESTDRV_REG_PART_START_ADDR);
	if (sdio_test_once_not_aligned(block_size,SyncMode,alignDec) != 0)
	{
		printk("ERROR:	Test Data Ram not aligned Failed\n");
	}
	else
	{
		printk("Test Data Ram not aligned Succeed\n");
	}

	/*--------------------------*/
	/* Test Packet Ram Partition*/
	/*--------------------------*/
	mem_partition_size = TESTDRV_MEM_WORKING_PART_SIZE;
	reg_partition_size = TESTDRV_REG_WORKING_PART_SIZE;
	/* Set Memory Partition */
	status = set_partition(0,
					   mem_partition_size,
					   TESTDRV_PACKET_RAM_SIZE,
					   reg_partition_size,
					   TESTDRV_REG_PART_START_ADDR);
	if ( status )
	{
		printk("ERROR:	sdio_test_non_aligned_complete: set_partition failed\n");
		return;
	}
	if (sdio_test_once_not_aligned(block_size,SyncMode,alignDec) != 0)
	{
		printk("ERROR:	Test Packet Ram not aligned Failed\n");
	}
	else
	{
		printk("Test Packet Ram not aligned Succeed\n");
	}

} /* sdio_test_non_aligned_complete() */

/*	This function tests reading non-sligned blocks in sync way, only once */
static void sdio_test_once_not_aligned_sync_complete(void)
{
	printk("************	SDIO not_aligned Complete Sync Test, 16 byte block	************\n");
	sdio_test_non_aligned_complete(16, SdioSync,2);
	printk("************	SDIO not_aligned Complete Sync Test, 64 byte block	************\n");
	sdio_test_non_aligned_complete(64, SdioSync,2);
	printk("************	SDIO not_aligned Complete Sync Test, 128 byte block	************\n");
	sdio_test_non_aligned_complete(128, SdioSync,2);
	printk("************	SDIO not_aligned Complete Sync Test, 256 byte block	************\n");
	sdio_test_non_aligned_complete(256, SdioSync,2);
	printk("************	SDIO not_aligned Complete Sync Test, 512 byte block	************\n");
	sdio_test_non_aligned_complete(512, SdioSync,2);	/* 0 equivalent to 512 bytes block */
}

/*	This function tests reading non-sligned blocks in async way, only once */
static void sdio_test_once_not_aligned_async_complete(void)
{
	printk("************	SDIO not_aligned Complete Async Test, 16 byte block	************\n");
	sdio_test_non_aligned_complete(16, SdioAsync,2);
	printk("************	SDIO not_aligned Complete Async Test, 64 byte block	************\n");
	sdio_test_non_aligned_complete(64, SdioAsync,2);
	printk("************	SDIO not_aligned Complete Async Test, 128 byte block	************\n");
	sdio_test_non_aligned_complete(128, SdioAsync,2);
	printk("************	SDIO not_aligned Complete Async Test, 256 byte block	************\n");
	sdio_test_non_aligned_complete(256, SdioAsync,2);
	printk("************	SDIO not_aligned Complete Async Test, 512 byte block	************\n");
	sdio_test_non_aligned_complete(512, SdioAsync,2);	/* 0 equivalent to 512 bytes block */
}

/*	This function tests reading non-sligned blocks in sync way, for ever */
static void sdio_test_infinit_not_aligned_sync_complete(void)
{
	while(1)
	{
		sdio_test_once_not_aligned_sync_complete();
	}
}

/*	This function tests reading non-sligned blocks in async way, for ever */
static void sdio_test_infinit_not_aligned_async_complete(void)
{
	while(1)
	{
		sdio_test_once_not_aligned_async_complete();
	}
}

/*	This function tests reading non-sligned blocks in sync & azync way, only once */
static void sdio_test_once_not_aligned_complete(void)
{
	sdio_test_once_not_aligned_sync_complete();
	sdio_test_once_not_aligned_async_complete();
}

/*	This function tests reading non-sligned blocks in sync & azync way, for ever */
static void sdio_test_infinit_not_aligned_complete(void)
{
	while(1)
	{
		sdio_test_once_not_aligned_sync_complete();
		sdio_test_once_not_aligned_async_complete();
	}
}
/*--------------------------------------------------------------------------------------*/

/*	This function receive and parse user requests */
static int sdio_test_write_proc(struct file *file, 
								const char *buffer,
								unsigned long count, 
								void *data)
{
	int flag = 0;
    unsigned long base;
    int j;
	SdioSyncMode_e SyncMode;
    ESdioBlockMode eBlockMode = SDIO_BLOCK_NONE;
	
	if (buffer[2] == 'a' || buffer[2] == 'A')
	{
		SyncMode = SdioAsync;
		j = 3;
	}
	else
	{
		SyncMode = SdioSync;
		j = 2;
	}

	if (buffer[3] == 'b' || buffer[3] == 'B')
	{
		eBlockMode = SDIO_BLOCK_MODE;
		j = 4;
	}
	else
	{
		eBlockMode = SDIO_BLOCK_NONE;
	}

    switch (buffer[0])
    {
    case 'z':
    case 'Z':
        {
            unsigned int divider;

            sscanf((buffer + 1), "%d", (unsigned int *)&divider);

            /* 
             * SDIO CLK = (Main clk / (divider( 9bit) << 6)) | 5 
             * Main clk = 96M. 
             * |5 in order to set Clock enable 
             */
            divider = (divider << 6) | 5;

            /* write to omap SYSCTL register */
            omap_writel( divider, MMCHS_SYSCTL);
        }
        break;
    case 'f':
    case 'F':
        {
            unsigned int Address, data, function;
            unsigned char value;

            if (buffer[1] == 'w' || buffer[1] == 'W')
            {
                sscanf((buffer + 2), "%d %X %X", (unsigned int *)&function, (unsigned int *)&Address, (unsigned int *)&data);
				sdioDrv_WriteSyncBytes (function, Address, (char *)&data, 1, 0);
            }
            else
            {
                sscanf((buffer + 2), "%d %X", (unsigned int *)&function, (unsigned int *)&Address);
				sdioDrv_ReadSyncBytes (function, Address, (char *)&data, 1, 0);
				value = (unsigned char)data;
				printk("\nfunction %d address to write: %x. data to write %x. value %x\n",function, Address,data,value);
				sdioDrv_WriteSyncBytes (function, Address,  &value, 1, 0);
                printk("The value at 0x%x is 0x%02x\n", Address, value);
            }

        }
        break;
    case 'p':
    case 'P':
        {
            unsigned int memOffset, memSize, regOffset, regSize;
            int status;

            sscanf((buffer + 1), "%X %X %X %X", (unsigned int *)&memOffset, (unsigned int *)&memSize, (unsigned int *)&regOffset,(unsigned int *)&regSize);
            /* Set Registers Partition */
            printk("perform set partitions. mem addr =  0x%X, mem size = 0x%X, reg addr = 0x%X,  reg size = 0x%X,\n",memOffset,memSize,regOffset,regSize);
            printk("Running load_vals status = "); 
            status = set_partition (0,memSize, memOffset, regSize, regOffset);
            printk("%d %s\n", status, status ? "NOT OK" : "OK");
            mem_partition_size = memSize;
            reg_partition_size = regSize;
        }
        break;

    case 'b':
    case 'B':
        if (buffer[1] == 'r' || buffer[1] == 'R')
        {
            base = sdio_test_get_base(buffer[2]);
            sdio_test_read_byte(&buffer[3], base); 
        }
        if (buffer[1] == 'w' || buffer[1] == 'W')
        {
            base = sdio_test_get_base(buffer[2]);
            sdio_test_write_byte(&buffer[3], base); 
        }
        break;

	case 'm':
	case 'M':
		if (buffer[1] == 'd' || buffer[1] == 'D')
		{
			printk("In switch (buffer[0]): calling do_test_async_data_mem\n");
			do_test_async_data_mem(flag);
		}
		if (buffer[1] == 'c' || buffer[1] == 'C')
		{
			printk("In switch (buffer[0]): calling do_test_async_code_mem\n");
			do_test_async_code_mem(flag);
		}
		if (buffer[1] == 'p' || buffer[1] == 'P')
		{
			printk("In switch (buffer[0]): calling do_test_async_packet_mem\n");
			do_test_async_packet_mem(flag);
		}
		break;

	case 'r':
	case 'R':
		base = sdio_test_get_base(buffer[1]);
		sdio_test_read(&buffer[j], base, SdioSync, SDIO_BLOCK_NONE); 
		break;

	case 'w':
	case 'W':
		base = sdio_test_get_base(buffer[1]);
		sdio_test_write(&buffer[j], base, SyncMode, eBlockMode); 
		break;

    case 'e':
	case 'E':
		sdio_test_e(&buffer[j], buffer[1], SyncMode, eBlockMode); 
		break;

#if 0 
    case 'o':
	case 'O':
		sdio_access_omap(&buffer[j], buffer[1], SyncMode, eBlockMode); 
		break;
#endif
	case 'o':
	case 'O':
		dumpreg();
		break;

	case 'q':
	case 'Q':
		if (g_Quiet)
			g_Quiet = 0;
		else
			g_Quiet = 1;
		printk("Changed Quiet mode to %d\n",g_Quiet);
		break;

	case 's':
	case 'S':
		switch (buffer[1])
		{
		case 'r':
		case 'R':
			do_test_sync(TESTDRV_READ_TEST);
			break;

		case 'w':
		case 'W':
			do_test_sync(TESTDRV_WRITE_TEST);
			break;

		case 'c':
		case 'C':
			do_test_sync(TESTDRV_COMPARE_TEST);
			break;
		case 'f':
		case 'F':
			do_test_sync(TESTDRV_FUNC0_TEST);
			break;

		case 'a':
		case 'A':
			do_test_sync(TESTDRV_FUNC0_READ_TEST);
			break;
		default:
			do_test_sync(TESTDRV_READ_TEST | TESTDRV_WRITE_TEST | TESTDRV_COMPARE_TEST | TESTDRV_FUNC0_TEST);
			break;
		} /* switch (buffer[1]) */  		
		break;

	case 'a':
	case 'A':
		switch (buffer[1])
		{
		case 'r':
		case 'R':
			do_test_async(TESTDRV_READ_TEST);
			break;

		case 'w':
		case 'W':
			do_test_async(TESTDRV_WRITE_TEST);
			break;

		case 'c':
		case 'C':
			do_test_async(TESTDRV_COMPARE_TEST);
			break;

		default:
			do_test_async(TESTDRV_READ_TEST | TESTDRV_WRITE_TEST | TESTDRV_COMPARE_TEST);
			break;

		} /* switch (buffer[1]) */
		break;

	case 'i':
	case 'I':
		sdio_test_init();
		break;

	case 'y':
	case 'Y':
           printk(KERN_ERR "\n MMCHS_SYSCONFIG   for mmc5 = %x  ", omap_readl( 0x4A100148 ));

		sdio_test_init_connect_bus();
		break;

	case 'l':
	case 'L':
		sdio_test_is_alive();
		break;

	case 't':
	case 'T':
		sdio_test_reset();
		break;

    case 'u':
    case 'U':
        sdio_test_interrupt();
        break;

	/* Complete Memory SDIO Test */
	case 'c':
	case 'C':
		/* Total Complete memory Test */
		if (buffer[1] == 't' || buffer[1] == 'T')
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				/* Loop Forever */
				sdio_test_infinit_complete();
			}
			else
			{
				sdio_test_once_complete();
			}
		}
		/* A-Sync Complete memory Test */
		else if (buffer[1] == 'a' || buffer[1] == 'A')
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				sdio_test_infinit_async_complete();
			}
			else /* test is not infinit */
			{
				sdio_test_once_async_complete();
			}
		}
		/* Sync Complete memory Test */
		else
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				sdio_test_infinit_sync_complete();
			}
			else /* test is not infinit */
			{
				sdio_test_once_sync_complete();
			}
		}
		break;

	/* Not-aligned Complete Memory SDIO Test */
	case 'n':
	case 'N':

		/* Total Not-aligned Complete memory Test */
		if (buffer[1] == 't' || buffer[1] == 'T')
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				sdio_test_infinit_not_aligned_complete();
			}
			else
			{
				sdio_test_once_not_aligned_complete();
			}
		}
		/* A-Sync Not-aligned Complete memory Test */
		else if (buffer[1] == 'a' || buffer[1] == 'A')
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				sdio_test_infinit_not_aligned_async_complete();
			}
			else
			{
				sdio_test_once_not_aligned_async_complete();
			}
		}
		/* Sync Not-aligned Complete memory Test */
		else
		{
			/*check if Infinit test */
			if (buffer[2] == 'i' || buffer[2] == 'I')
			{
				sdio_test_infinit_not_aligned_sync_complete();
			}
			else
			{
				sdio_test_once_not_aligned_sync_complete();
			}
		}

		break;

	default:
		usage();
		break;
    };

    return count;

} /* sdio_test_write_proc() */

/*--------------------------------------------------------------------------------------*/

static int sdio_test_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len=0;

   len = sprintf(page, "%x\n",(unsigned int)g_last_read);
   *eof = 1;

   if (!g_Quiet)
   {
	 printk("Read_proc %X\n", (unsigned int)g_last_read);
   }

   return len;

} /* sdio_test_read_proc() */

/*--------------------------------------------------------------------------------------*/

static irqreturn_t tiwlan_interrupt (int irq, void *not_used)
{
    unsigned long base;
    char   *cmd = "wr 4f0 1";

	base = sdio_test_get_base(cmd[1]);
	sdio_test_write(&cmd[2], base, SdioSync, SDIO_BLOCK_NONE); 

	if (!g_Quiet)
	{
	    printk("tiwlan_interrupt() handled\n");
	}

    return IRQ_HANDLED;

} /* tiwlan_interrupt() */

/*--------------------------------------------------------------------------------------*/

static int  __init sdio_test_module_init(void)
{
	struct proc_dir_entry *mmc_api_file;
	int err;

	if ((mmc_api_file = create_proc_entry(TESTDRV_MODULE_NAME, 0666, NULL)) == NULL)
	{
		printk("Could not create /proc entry\n");
		return -ENOMEM;
	}

	mmc_api_file->data       = NULL;
	mmc_api_file->read_proc  = (read_proc_t*)sdio_test_read_proc;
	mmc_api_file->write_proc = (write_proc_t*)sdio_test_write_proc;
	err = hPlatform_Wlan_Hardware_Init(); 

	if (gpio_request(PMENA_GPIO, NULL) != 0) 	{
	  printk(KERN_ERR "%s:%s FAILED to do to get gpio_request(PMENA_GPIO)\n",__FILE__,__FUNCTION__);
	  return -EINVAL;
	}
	gpio_direction_output(PMENA_GPIO, 0);
 
	if (gpio_request(IRQ_GPIO, NULL) != 0) 
	{
		printk(KERN_ERR "%s:%s FAILED to do to get gpio_request(IRQ_GPIO)\n",__FILE__,__FUNCTION__);
		return -EINVAL;
	};
    gpio_direction_input(IRQ_GPIO);

	sdio_test_reset();
	memset(&g_sdio,0,sizeof(g_sdio));
	sema_init(&g_sdio.sem, 0);
    
	if (request_irq (TNETW_IRQ, tiwlan_interrupt, IRQF_TRIGGER_FALLING, TESTDRV_MODULE_NAME, &g_sdio))
	{
	    printk("sdio_test_module_init() Failed to register interrupt handler!!\n");
	}

	simulate_prob();
	printk("sdio_test_module_init: OK\n");

	return init_buffers();
	
} /*  sdio_test_module_init() */

/*--------------------------------------------------------------------------------------*/

static void __exit sdio_test_module_exit(void)
{
    sdioDrv_DisconnectBus();

	if (write_buf)
	{
		kfree(write_buf);
	}
	if (read_buf)
	{
		kfree(read_buf);
	}
	free_irq(TNETW_IRQ, &g_sdio);
	gpio_free(IRQ_GPIO);
	gpio_free(PMENA_GPIO);
	remove_proc_entry("sdio_test", NULL);

} /* sdio_test_exit() */

/*--------------------------------------------------------------------------------------*/

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("OMAP MMC Test");
MODULE_LICENSE("GPL");

module_init(sdio_test_module_init);
module_exit(sdio_test_module_exit);

