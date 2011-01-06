/*
 * SdioDrv.c
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

/** \file SdioDrv.c
*/



#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <mach/dma.h>
#include "host_platform.h"
#include "SdioDrvDbg.h"
#include "SdioDrv.h"


#define OMAP_MMC_MASTER_CLOCK          96000000
#define MMC_TIMEOUT_MS                 20


/* 
 * MMC Host controller read/write API's.
 */
#define OMAP_HSMMC_READ_OFFSET(offset) (__raw_readl((OMAP_MMC_REGS_BASE) + (offset)))
#define OMAP_HSMMC_READ(reg)           (__raw_readl((OMAP_MMC_REGS_BASE) + OMAP_HSMMC_##reg))
#define OMAP_HSMMC_WRITE(reg, val)     (__raw_writel((val), (OMAP_MMC_REGS_BASE) + OMAP_HSMMC_##reg))

#define OMAP_HSMMC_SEND_COMMAND(cmd, arg) do \
{ \
	OMAP_HSMMC_WRITE(ARG, arg); \
	OMAP_HSMMC_WRITE(CMD, cmd); \
} while (0)

#define OMAP_HSMMC_CMD52_WRITE     ((SD_IO_RW_DIRECT    << 24) | (OMAP_HSMMC_CMD_SHORT_RESPONSE << 16))
#define OMAP_HSMMC_CMD52_READ      (((SD_IO_RW_DIRECT   << 24) | (OMAP_HSMMC_CMD_SHORT_RESPONSE << 16)) | DDIR)
#define OMAP_HSMMC_CMD53_WRITE     (((SD_IO_RW_EXTENDED << 24) | (OMAP_HSMMC_CMD_SHORT_RESPONSE << 16)) | DP_SELECT)
#define OMAP_HSMMC_CMD53_READ      (((SD_IO_RW_EXTENDED << 24) | (OMAP_HSMMC_CMD_SHORT_RESPONSE << 16)) | DP_SELECT | DDIR)
#define OMAP_HSMMC_CMD53_READ_DMA  (OMAP_HSMMC_CMD53_READ  | DMA_EN)
#define OMAP_HSMMC_CMD53_WRITE_DMA (OMAP_HSMMC_CMD53_WRITE | DMA_EN)

/* Macros to build commands 52 and 53 in format according to SDIO spec */
#define SDIO_CMD52_READ(v1,v2,v3,v4)        (SDIO_RWFLAG(v1)|SDIO_FUNCN(v2)|SDIO_RAWFLAG(v3)| SDIO_ADDRREG(v4))
#define SDIO_CMD52_WRITE(v1,v2,v3,v4,v5)    (SDIO_RWFLAG(v1)|SDIO_FUNCN(v2)|SDIO_RAWFLAG(v3)| SDIO_ADDRREG(v4)|(v5))
#define SDIO_CMD53_READ(v1,v2,v3,v4,v5,v6)  (SDIO_RWFLAG(v1)|SDIO_FUNCN(v2)|SDIO_BLKM(v3)| SDIO_OPCODE(v4)|SDIO_ADDRREG(v5)|(v6&0x1ff))
#define SDIO_CMD53_WRITE(v1,v2,v3,v4,v5,v6) (SDIO_RWFLAG(v1)|SDIO_FUNCN(v2)|SDIO_BLKM(v3)| SDIO_OPCODE(v4)|SDIO_ADDRREG(v5)|(v6&0x1ff))

#define SDIODRV_MAX_LOOPS 50000

#define VMMC2_DEV_GRP		0x2B
#define VMMC2_DEDICATED		0x2E
#define VSEL_S2_18V		0x05
#define LDO_CLR			0x00
#define VSEL_S2_CLR		0x40
#define GPIO_0_BIT_POS		1 << 0
#define GPIO_1_BIT_POS		1 << 1
#define VSIM_DEV_GRP		0x37
#define VSIM_DEDICATED		0x3A
#define TWL4030_MODULE_PM_RECIEVER	0x13

typedef struct OMAP3430_sdiodrv
{
	struct clk    *fclk, *iclk, *dbclk;
	int           dma_tx_channel;
	int           dma_rx_channel;
	int		irq;
	void          (*BusTxnCB)(void* BusTxnHandle, int status);
	void*         BusTxnHandle;
	unsigned int  uBlkSize;
	unsigned int  uBlkSizeShift;
	int           async_status;
	int (*wlanDrvIf_pm_resume)(void);
	int (*wlanDrvIf_pm_suspend)(void);
	struct device *dev;
	dma_addr_t dma_read_addr;
	size_t dma_read_size;
	dma_addr_t dma_write_addr;
	size_t dma_write_size;
} OMAP3430_sdiodrv_t;

#define SDIO_DRIVER_NAME 			"TIWLAN_SDIO"

module_param(g_sdio_debug_level, int, 0644);
MODULE_PARM_DESC(g_sdio_debug_level, "debug level");
int g_sdio_debug_level = SDIO_DEBUGLEVEL_ERR;
EXPORT_SYMBOL( g_sdio_debug_level);

OMAP3430_sdiodrv_t g_drv;

struct work_struct sdiodrv_work;

static int sdiodrv_dma_on = 0;
static int sdiodrv_irq_requested = 0;
static int sdiodrv_iclk_ena = 0;
static int sdiodrv_fclk_ena = 0;
static int sdiodrv_iclk_got = 0;
static int sdiodrv_fclk_got = 0;

int OMAP3430_mmc_reset(void);
static u32 sdiodrv_poll_status(u32 reg_offset, u32 stat, unsigned int msecs);

static void host_set_default_sdio_clk(unsigned int divider)
{
	/* 
	 * SDIO CLK = (Main clk / (divider( 9bit) << 6)) | 5 
	 * Main clk = 96M. 
	 * |5 in order to set Clock enable 
	 */
	divider = (divider << 6) | 5;
	
	/* write to omap SYSCTL register */
	omap_writel( divider, MMCHS_SYSCTL);
}


void simulate_prob(void)
{
	u32 status;

	OMAP3430_mmc_reset();
	//obc - init sequence p. 3600,3617
	/* 1.8V */

	OMAP_HSMMC_WRITE(CAPA,		OMAP_HSMMC_READ(CAPA) | VS18);

	OMAP_HSMMC_WRITE(CAPA,		OMAP_HSMMC_READ(CAPA) & (0x4 << 24) );


	printk("%s: %d: BEFORE HCTL is now set to: %x\n",__FUNCTION__,__LINE__, OMAP_HSMMC_READ(HCTL));
    /* set bus power to 1.8v*/
	OMAP_HSMMC_WRITE(HCTL,		OMAP_HSMMC_READ(HCTL) | SDVS18);
	/* turn bus power on */
	OMAP_HSMMC_WRITE(HCTL,		OMAP_HSMMC_READ(HCTL) | SDBP);

	printk("%s: %d: AFTER CAPA is now set to: %x\n",__FUNCTION__,__LINE__, OMAP_HSMMC_READ(CAPA));
	printk("%s: %d: AFTER HCTL is now set to: %x\n",__FUNCTION__,__LINE__, OMAP_HSMMC_READ(HCTL));

	/* clock gating */
	OMAP_HSMMC_WRITE(SYSCONFIG, OMAP_HSMMC_READ(SYSCONFIG) | AUTOIDLE);

	/* interrupts */
	OMAP_HSMMC_WRITE(ISE,		0);
	OMAP_HSMMC_WRITE(IE,		IE_EN_MASK);
	host_set_default_sdio_clk(2); /*2=24Mhz 48==1Mhz 240=400KHz*/

#ifdef SDIO_1_BIT /* see also in SdioAdapter.c */
	PDEBUG("%s() setting %d data lines\n",__FUNCTION__, 1); 
#else
	PDEBUG("%s() setting %d data lines\n",__FUNCTION__, 4);
	OMAP_HSMMC_WRITE(HCTL, OMAP_HSMMC_READ(HCTL) | (1 << 1));//DTW 4 bits - p. 3650
#endif
	
	/* send the init sequence. 80 clocks of synchronization in the SDIO */
	//doesn't match p. 3601,3617 - obc
	OMAP_HSMMC_WRITE( CON, OMAP_HSMMC_READ(CON) | INIT_STREAM);
	OMAP_HSMMC_SEND_COMMAND( 0, 0);
	status = sdiodrv_poll_status(OMAP_HSMMC_STAT, CC, MMC_TIMEOUT_MS);
	if (!(status & CC)) {
		PERR("sdioDrv_InitHw() SDIO Command error status = 0x%x\n", status);
	}
	OMAP_HSMMC_WRITE(CON, OMAP_HSMMC_READ(CON) & ~INIT_STREAM);
}



void sdiodrv_task(struct work_struct *unused)
{
	PDEBUG("sdiodrv_tasklet()\n");

	if (g_drv.dma_read_addr != 0) {
		dma_unmap_single(g_drv.dev, g_drv.dma_read_addr, g_drv.dma_read_size, DMA_FROM_DEVICE);
		g_drv.dma_read_addr = 0;
		g_drv.dma_read_size = 0;
	}
	
	if (g_drv.dma_write_addr != 0) {
		dma_unmap_single(g_drv.dev, g_drv.dma_write_addr, g_drv.dma_write_size, DMA_TO_DEVICE);
		g_drv.dma_write_addr = 0;
		g_drv.dma_write_size = 0;
	}

	if (g_drv.BusTxnCB != NULL) {
		g_drv.BusTxnCB(g_drv.BusTxnHandle, g_drv.async_status);
	}
}

irqreturn_t sdiodrv_irq(int irq, void *drv)
{
	int status;

	PDEBUG("sdiodrv_irq()\n");

	status = OMAP_HSMMC_READ(STAT);
	OMAP_HSMMC_WRITE(ISE, 0);
	g_drv.async_status = status & (ERR);
	if (g_drv.async_status) {
		PERR("sdiodrv_irq: ERROR in STAT = 0x%x\n", status);
	}
	schedule_work(&sdiodrv_work);

	return IRQ_HANDLED;
}

void sdiodrv_dma_read_cb(int lch, u16 ch_status, void *data)
{
	PDEBUG("sdiodrv_dma_read_cb() channel=%d status=0x%x\n", lch, (int)ch_status);

	g_drv.async_status = ch_status & (1 << 7);
	
	schedule_work(&sdiodrv_work);
}

int sdiodrv_dma_init(void)
{
	int rc;

	rc = omap_request_dma(OMAP44XX_DMA_MMC5_DMA_TX, "SDIO WRITE", NULL /* sdiodrv_dma_write_cb */, &g_drv, &g_drv.dma_tx_channel);
	if (rc != 0) {
		PERR("sdiodrv_dma_init() omap_request_dma(OMAP34XX_DMA_MMC2_TX) FAILED\n");
		goto out;
	}

	rc = omap_request_dma(OMAP44XX_DMA_MMC5_DMA_RX, "SDIO READ", sdiodrv_dma_read_cb, &g_drv, &g_drv.dma_rx_channel);
	if (rc != 0) {
		PERR("sdiodrv_dma_init() omap_request_dma(OMAP24XX_DMA_MMC2_RX) FAILED\n");
		goto freetx;
	}

	omap_set_dma_src_params(g_drv.dma_rx_channel,
  							0,			// src_port is only for OMAP1
  							OMAP_DMA_AMODE_CONSTANT,
  							(TIWLAN_MMC_CONTROLLER_BASE_ADDR) + OMAP_HSMMC_DATA, 0, 0);
  
	omap_set_dma_dest_params(g_drv.dma_tx_channel,
							0,			// dest_port is only for OMAP1
  	  						OMAP_DMA_AMODE_CONSTANT,
  	  						(TIWLAN_MMC_CONTROLLER_BASE_ADDR) + OMAP_HSMMC_DATA, 0, 0);
  
	return 0;

freerx:
	omap_free_dma(g_drv.dma_rx_channel);	

freetx:
	omap_free_dma(g_drv.dma_tx_channel);
out:
	return rc;
}

void sdiodrv_dma_shutdown(void)
{
  omap_free_dma(g_drv.dma_tx_channel);
  omap_free_dma(g_drv.dma_rx_channel);
} /* sdiodrv_dma_shutdown() */

static u32 sdiodrv_poll_status(u32 reg_offset, u32 stat, unsigned int msecs)
{
    u32 status=0, loops=0;

	do
    {
	    status=OMAP_HSMMC_READ_OFFSET(reg_offset);
	    if(( status & stat))
		{
		  break;
		}
	} while (loops++ < SDIODRV_MAX_LOOPS);

	return status;
} /* sdiodrv_poll_status */


//cmd flow p. 3609 obc
static int sdiodrv_send_command(u32 cmdreg, u32 cmdarg)
{
    OMAP_HSMMC_WRITE(STAT, STAT_CLEAR);
	OMAP_HSMMC_SEND_COMMAND(cmdreg, cmdarg);
	return sdiodrv_poll_status(OMAP_HSMMC_STAT, CC, MMC_TIMEOUT_MS);

} /* sdiodrv_send_command() */

/*
 *  Disable clock to the card
 */
#if 0
static void OMAP3430_mmc_stop_clock(void)
{
	OMAP_HSMMC_WRITE(SYSCTL, OMAP_HSMMC_READ(SYSCTL) & ~CEN);
	if ((OMAP_HSMMC_READ(SYSCTL) & CEN) != 0x0)
    {
		PERR("MMC clock not stoped, clock freq can not be altered\n");
    }
} /* OMAP3430_mmc_stop_clock */
#endif

/*
 *  Reset the SD system
 */
int OMAP3430_mmc_reset(void)
{
    int status, loops=0;
	//p. 3598 - need to set SOFTRESET to 0x1 0bc
	OMAP_HSMMC_WRITE(SYSCTL, OMAP_HSMMC_READ(SYSCTL) | SRA);
	while ((status = OMAP_HSMMC_READ(SYSCTL) &  SRA) && loops++ < SDIODRV_MAX_LOOPS);
	if (status & SRA)
	{
	    PERR("OMAP3430_mmc_reset() MMC reset FAILED!! status=0x%x\n",status);
	}

	return status;

} /* OMAP3430_mmc_reset */


#if 0
static void OMAP3430_mmc_set_clock(unsigned int clock, OMAP3430_sdiodrv_t *host)
{
	u16           dsor = 0;
	unsigned long regVal;
	int           status;

	PDEBUG("OMAP3430_mmc_set_clock(%d)\n",clock);
	if (clock) {
		/* Enable MMC_SD_CLK */
		dsor = OMAP_MMC_MASTER_CLOCK / clock;
		if (dsor < 1) {
			dsor = 1;
		}
		if (OMAP_MMC_MASTER_CLOCK / dsor > clock) {
			dsor++;
		}
		if (dsor > 250) {
			dsor = 250;
		}
	}
	OMAP3430_mmc_stop_clock();
	regVal = OMAP_HSMMC_READ(SYSCTL);
	regVal = regVal & ~(CLKD_MASK);//p. 3652
	regVal = regVal | (dsor << 6);
	regVal = regVal | (DTO << 16);//data timeout
	OMAP_HSMMC_WRITE(SYSCTL, regVal);
	OMAP_HSMMC_WRITE(SYSCTL, OMAP_HSMMC_READ(SYSCTL) | ICE);//internal clock enable. obc not mentioned in the spec
	/* 
     * wait till the the clock is satble (ICS) bit is set
	 */
	status  = sdiodrv_poll_status(OMAP_HSMMC_SYSCTL, ICS, MMC_TIMEOUT_MS);
	if(!(status & ICS)) {
	    PERR("OMAP3430_mmc_set_clock() clock not stable!! status=0x%x\n",status);
	}
	/* 
	 * Enable clock to the card
	 */
	OMAP_HSMMC_WRITE(SYSCTL, OMAP_HSMMC_READ(SYSCTL) | CEN);

} /* OMAP3430_mmc_set_clock() */
#endif

static void sdiodrv_free_resources(void)
{
	if (sdiodrv_iclk_ena) {
		clk_disable(g_drv.iclk);
		sdiodrv_iclk_ena = 0;
	}
	
	if (sdiodrv_fclk_ena) {
		clk_disable(g_drv.fclk);
		sdiodrv_fclk_ena = 0;
	}

	if (sdiodrv_fclk_got) {
		clk_put(g_drv.fclk);
		sdiodrv_fclk_got = 0;
	}

	if (sdiodrv_iclk_got) {
		clk_put(g_drv.iclk);
		sdiodrv_iclk_got = 0;
	}

	if (sdiodrv_irq_requested) {
		free_irq(g_drv.irq, &g_drv);
		sdiodrv_irq_requested = 0;
	}
}

int sdioDrv_InitHw(void)
{
	return 0;
} /* sdiodrv_init */

void sdiodrv_shutdown(void)
{
	PDEBUG("entering %s()\n" , __FUNCTION__ );

	sdiodrv_free_resources();

	PDEBUG("exiting %s\n", __FUNCTION__);
} /* sdiodrv_shutdown() */

static int sdiodrv_send_data_xfer_commad(u32 cmd, u32 cmdarg, int length, u32 buffer_enable_status, unsigned int bBlkMode)
{
    int status;

	//printk("%s() writing CMD %u ARG %u\n",__FUNCTION__, cmd, cmdarg);

    /* block mode (TI_FALSE=0)*/
	if(bBlkMode) {

		PDEBUG("%s: %d g_drv.uBlkSize=%x, length=%x\n", __FUNCTION__,__LINE__, g_drv.uBlkSize, length);
        /* 
         * Bits 31:16 of BLK reg: NBLK Blocks count for current transfer.
         *                        in case of Block MOde the lenght is treated here as number of blocks 
         *                        (and not as a length).
         * Bits 11:0 of BLK reg: BLEN Transfer Block Size. in case of block mode set that field to block size. 
         */
        OMAP_HSMMC_WRITE(BLK, (length << 16) | (g_drv.uBlkSize << 0));



        /*
         * In CMD reg:
         * BCE: Block Count Enable
         * MSBS: Multi/Single block select
         */
        cmd |= MSBS | BCE ;
	} else
		{
        OMAP_HSMMC_WRITE(BLK, length);
		}


	status = sdiodrv_send_command(cmd, cmdarg);
	if(!(status & CC)) {
	    PERR("sdiodrv_send_data_xfer_commad() SDIO Command error! STAT = 0x%x\n", status);
	    return 0;
	}
	PDEBUG("%s() length = %d(%dw) BLK = 0x%x\n",
		   __FUNCTION__, length,((length + 3) >> 2), OMAP_HSMMC_READ(BLK));

	if (buffer_enable_status == BWE){

		return (OMAP_HSMMC_READ(STAT)| BWE);
	}
	
	else{
		return sdiodrv_poll_status(OMAP_HSMMC_PSTATE, buffer_enable_status, MMC_TIMEOUT_MS);
	}
		

} /* sdiodrv_send_data_xfer_commad() */

int sdiodrv_data_xfer_sync(u32 cmd, u32 cmdarg, void *data, int length, u32 buffer_enable_status)
{
    u32 buf_start, buf_end, data32;
	int status;

    status = sdiodrv_send_data_xfer_commad(cmd, cmdarg, length, buffer_enable_status, 0);
/*BENZY - BWE / BRE,  might not be good HW issues?*/
/*
	if (buffer_enable_status != BWE) 
	{
		if(!(status & buffer_enable_status)) 
		{
			PERR("sdiodrv_data_xfer_sync() buffer disabled! length = %d BLK = 0x%x PSTATE = 0x%x\n", 
				   length, OMAP_HSMMC_READ(BLK), status);
			return -1;
		}
	}else{
		printk("sdiodrv_data_xfer_sync: Benzy3: just jumped over BWE check (need to investigate)\n");
	}

	PERR("sdiodrv_data_xfer_sync: skipping BWE check\n");
*/
	buf_end = (u32)data+(u32)length;

	//obc need to check BRE/BWE every time, see p. 3605
	/*
	 * Read loop 
	 */
	if (buffer_enable_status == BRE)
	{
	  if (((u32)data & 3) == 0) /* 4 bytes aligned */
	  {
		for (buf_start = (u32)data; (u32)data < buf_end; data += sizeof(unsigned long))
		{
		  *((unsigned long*)(data)) = OMAP_HSMMC_READ(DATA);
		}
	  }
	  else                      /* 2 bytes aligned */
	  {
		for (buf_start = (u32)data; (u32)data < buf_end; data += sizeof(unsigned long))
		{
		  data32 = OMAP_HSMMC_READ(DATA);
		  *((unsigned short *)data)     = (unsigned short)data32;
		  *((unsigned short *)data + 1) = (unsigned short)(data32 >> 16);
		}
	  }
	}
	/*
	 * Write loop 
	 */
	else
	{
	  if (((u32)data & 3) == 0) /* 4 bytes aligned */
	  {
		for (buf_start = (u32)data; (u32)data < buf_end; data += sizeof(unsigned long))
		{
		  OMAP_HSMMC_WRITE(DATA,*((unsigned long*)(data)));
		}
	  }
	  else                      /* 2 bytes aligned */
	  {
		for (buf_start = (u32)data; (u32)data < buf_end; data += sizeof(unsigned long))
		{
		  OMAP_HSMMC_WRITE(DATA,*((unsigned short*)data) | *((unsigned short*)data+1) << 16 );
		}

	  }
	}
	status  = sdiodrv_poll_status(OMAP_HSMMC_STAT, TC, MMC_TIMEOUT_MS);
	if(!(status & TC)) 
    {
	    PERR("sdiodrv_data_xfer_sync() transfer error! STAT = 0x%x\n", status);
	    return -1;
	}
    return 0;

} /* sdiodrv_data_xfer_sync() */

/*--------------------------------------------------------------------------------------*/

/********************************************************************/
/*	SDIO driver interface functions                                 */
/********************************************************************/

/*--------------------------------------------------------------------------------------*/

int sdioDrv_ConnectBus (void *        fCbFunc, 
                        void *        hCbArg, 
                        unsigned int  uBlkSizeShift, 
                        unsigned int  uSdioThreadPriority)
{
    g_drv.BusTxnCB      = fCbFunc;
    g_drv.BusTxnHandle  = hCbArg;
    g_drv.uBlkSizeShift = uBlkSizeShift;  
    g_drv.uBlkSize      = 1 << uBlkSizeShift;

    INIT_WORK(&sdiodrv_work, sdiodrv_task);

    return sdioDrv_InitHw ();
}


/*--------------------------------------------------------------------------------------*/

int sdioDrv_DisconnectBus (void)
{
    return 0;
}

//p.3609 cmd flow
int sdioDrv_ExecuteCmd (unsigned int uCmd, 
                        unsigned int uArg, 
                        unsigned int uRespType, 
                        void *       pResponse, 
                        unsigned int uLen)
{
	unsigned int uCmdReg   = 0;
	unsigned int uStatus   = 0;
	unsigned int uResponse = 0;

	PDEBUG("sdioDrv_ExecuteCmd() starting cmd %02x arg %08x\n", (int)uCmd, (int)uArg);

	uCmdReg = (uCmd << 24) | (uRespType << 16) ;

	uStatus = sdiodrv_send_command(uCmdReg, uArg);

    if (!(uStatus & CC)) 
    {
	    PERR("sdioDrv_ExecuteCmd() SDIO Command error CC = 0x%x\n", uStatus);
	    return -1;
	}
    if ((uLen > 0) && (uLen <= 4))
	{
	    uResponse = OMAP_HSMMC_READ(RSP10);
		memcpy (pResponse, (char *)&uResponse, uLen);
		PDEBUG("sdioDrv_ExecuteCmd() RSP10 = 0x%x\n", uResponse);
	}
    return 0;
}

/*--------------------------------------------------------------------------------------*/

int sdioDrv_ReadSync (unsigned int uFunc, 
                      unsigned int uHwAddr, 
                      void *       pData, 
                      unsigned int uLen,
                      unsigned int bIncAddr,
                      unsigned int bMore)
{
	unsigned int uCmdArg;
	int          iStatus;

	uCmdArg = SDIO_CMD53_READ(0, uFunc, 0, bIncAddr, uHwAddr, uLen);

	iStatus = sdiodrv_data_xfer_sync(OMAP_HSMMC_CMD53_READ, uCmdArg, pData, uLen, BRE);
	if (iStatus != 0)
	{
        PERR("sdioDrv_ReadSync() FAILED!!\n");
	}

	return iStatus;
}

/*--------------------------------------------------------------------------------------*/
int sdioDrv_ReadAsync (unsigned int uFunc, 
                       unsigned int uHwAddr, 
                       void *       pData, 
                       unsigned int uLen, 
                       unsigned int bBlkMode,
                       unsigned int bIncAddr,
                       unsigned int bMore)
{
	int          iStatus;
	unsigned int uCmdArg;
    unsigned int uNumBlks;
    unsigned int uDmaBlockCount;
    unsigned int uNumOfElem;
	dma_addr_t dma_bus_address;

	//printk(KERN_INFO "in sdioDrv_ReadAsync\n");
	
    if (bBlkMode)
    {
        /* For block mode use number of blocks instead of length in bytes */
        uNumBlks = uLen >> g_drv.uBlkSizeShift;
        uDmaBlockCount = uNumBlks;
        /* due to the DMA config to 32Bit per element (OMAP_DMA_DATA_TYPE_S32) the division is by 4 */ 
        uNumOfElem = g_drv.uBlkSize >> 2;
    }
    else
    {	
        uNumBlks = uLen;
        uDmaBlockCount = 1;
        uNumOfElem = (uLen + 3) >> 2;
    }

    uCmdArg = SDIO_CMD53_READ(0, uFunc, bBlkMode, bIncAddr, uHwAddr, uNumBlks);

    iStatus = sdiodrv_send_data_xfer_commad(OMAP_HSMMC_CMD53_READ_DMA, uCmdArg, uNumBlks, BRE, bBlkMode);
/*
    if (!(iStatus & BRE)) 
    {
        PERR("sdioDrv_ReadAsync() buffer disabled! length = %d BLK = 0x%x PSTATE = 0x%x, BlkMode = %d\n", 
              uLen, OMAP_HSMMC_READ(BLK), iStatus, bBlkMode);
        return -1;
    }

	PERR("sdioDrv_ReadAsync: skipping BRE check\n");
*/
	PDEBUG("sdiodrv_read_async() dma_ch=%d \n",g_drv.dma_rx_channel);

	dma_bus_address = dma_map_single(g_drv.dev, pData, uLen, DMA_FROM_DEVICE);
	if (!dma_bus_address) {
		PERR("sdioDrv_ReadAsync: dma_map_single failed\n");
		return -1;
	}		

	if (g_drv.dma_read_addr != 0) {
		printk(KERN_ERR "sdioDrv_ReadAsync: previous DMA op is not finished!\n");
		BUG();
	}
	
	g_drv.dma_read_addr = dma_bus_address;
	g_drv.dma_read_size = uLen;

	omap_set_dma_dest_params    (g_drv.dma_rx_channel,
									0,			// dest_port is only for OMAP1
									OMAP_DMA_AMODE_POST_INC,
									dma_bus_address,
									0, 0);

	omap_set_dma_transfer_params(g_drv.dma_rx_channel, OMAP_DMA_DATA_TYPE_S32, uNumOfElem , uDmaBlockCount , OMAP_DMA_SYNC_FRAME, OMAP34XX_DMA_MMC3_RX, OMAP_DMA_SRC_SYNC);

	omap_start_dma(g_drv.dma_rx_channel);

    /* Continued at sdiodrv_irq() after DMA transfer is finished */
	return 0;
}


/*--------------------------------------------------------------------------------------*/

int sdioDrv_WriteSync (unsigned int uFunc, 
                       unsigned int uHwAddr, 
                       void *       pData, 
                       unsigned int uLen,
                       unsigned int bIncAddr,
                       unsigned int bMore)
{
	unsigned int uCmdArg;
	int          iStatus;

    uCmdArg = SDIO_CMD53_WRITE(1, uFunc, 0, bIncAddr, uHwAddr, uLen);
	iStatus = sdiodrv_data_xfer_sync(OMAP_HSMMC_CMD53_WRITE, uCmdArg, pData, uLen, BWE);
	if (iStatus != 0)
	{
        PERR("sdioDrv_WriteSync() FAILED!!\n");
	}

	return iStatus;
}

/*--------------------------------------------------------------------------------------*/
int sdioDrv_WriteAsync (unsigned int uFunc, 
                        unsigned int uHwAddr, 
                        void *       pData, 
                        unsigned int uLen, 
                        unsigned int bBlkMode,
                        unsigned int bIncAddr,
                        unsigned int bMore)
{
	int          iStatus;
	unsigned int uCmdArg;
    unsigned int uNumBlks;
    unsigned int uDmaBlockCount;
    unsigned int uNumOfElem;
	dma_addr_t dma_bus_address;

    if (bBlkMode)
    {
        /* For block mode use number of blocks instead of length in bytes */
        uNumBlks = uLen >> g_drv.uBlkSizeShift;
        uDmaBlockCount = uNumBlks;
        /* due to the DMA config to 32Bit per element (OMAP_DMA_DATA_TYPE_S32) the division is by 4 */ 
        uNumOfElem = g_drv.uBlkSize >> 2;
    }
    else
    {	
        uNumBlks = uLen;
        uDmaBlockCount = 1;
        uNumOfElem = (uLen + 3) >> 2;
    }

    uCmdArg = SDIO_CMD53_WRITE(1, uFunc, bBlkMode, bIncAddr, uHwAddr, uNumBlks);

    iStatus = sdiodrv_send_data_xfer_commad(OMAP_HSMMC_CMD53_WRITE_DMA, uCmdArg, uNumBlks, BWE, bBlkMode);
/*    if (!(iStatus & BWE)) 
    {
        PERR("sdioDrv_WriteAsync() buffer disabled! length = %d, BLK = 0x%x, Status = 0x%x\n", 
             uLen, OMAP_HSMMC_READ(BLK), iStatus);
        return -1;
    }

	PERR("sdioDrv_WriteAsync: skipping BWE check\n");
*/
	OMAP_HSMMC_WRITE(ISE, TC);

	dma_bus_address = dma_map_single(g_drv.dev, pData, uLen, DMA_TO_DEVICE);
	if (!dma_bus_address) {
		PERR("sdioDrv_WriteAsync: dma_map_single failed\n");
		return -1;
	}

	if (g_drv.dma_write_addr != 0) {
		PERR("sdioDrv_WriteAsync: previous DMA op is not finished!\n");
		BUG();
	}
	
	g_drv.dma_write_addr = dma_bus_address;
	g_drv.dma_write_size = uLen;

	omap_set_dma_src_params     (g_drv.dma_tx_channel,
									0,			// src_port is only for OMAP1
									OMAP_DMA_AMODE_POST_INC,
									dma_bus_address,
									0, 0);

	omap_set_dma_transfer_params(g_drv.dma_tx_channel, OMAP_DMA_DATA_TYPE_S32, uNumOfElem, uDmaBlockCount, OMAP_DMA_SYNC_FRAME, OMAP34XX_DMA_MMC3_TX, OMAP_DMA_DST_SYNC);

	omap_start_dma(g_drv.dma_tx_channel);

    /* Continued at sdiodrv_irq() after DMA transfer is finished */
	return 0;
}

/*--------------------------------------------------------------------------------------*/

int sdioDrv_ReadSyncBytes (unsigned int  uFunc, 
                           unsigned int  uHwAddr, 
                           unsigned char *pData, 
                           unsigned int  uLen, 
                           unsigned int  bMore)
{
	unsigned int uCmdArg;
	unsigned int i;
	int          iStatus;

    for (i = 0; i < uLen; i++) 
    {
        uCmdArg = SDIO_CMD52_READ(0, uFunc, 0, uHwAddr);

        iStatus = sdiodrv_send_command(OMAP_HSMMC_CMD52_READ, uCmdArg);

        if (!(iStatus & CC)) 
        {
            PERR("sdioDrv_ReadSyncBytes() SDIO Command error status = 0x%x\n", iStatus);
            return -1;
        }
        else
        {
            *pData = (unsigned char)(OMAP_HSMMC_READ(RSP10));
        }

		uHwAddr++;
        pData++;
    }

    return 0;
}

/*--------------------------------------------------------------------------------------*/

int sdioDrv_WriteSyncBytes (unsigned int  uFunc, 
                            unsigned int  uHwAddr, 
                            unsigned char *pData, 
                            unsigned int  uLen, 
                            unsigned int  bMore)
{
	unsigned int uCmdArg;
	unsigned int i;
	int          iStatus;

    for (i = 0; i < uLen; i++) 
    {
        uCmdArg = SDIO_CMD52_WRITE(1, uFunc, 0, uHwAddr, *pData);

        iStatus = sdiodrv_send_command(OMAP_HSMMC_CMD52_WRITE, uCmdArg);

        if (!(iStatus & CC)) 
        {
            PERR("sdioDrv_WriteSyncBytes() SDIO Command error status = 0x%x\n", iStatus);
            return -1;
        }

		uHwAddr++;
        pData++;
    }

    return 0;
}

static int sdioDrv_probe(struct platform_device *pdev)
{
	int rc = 0;
#if 0
	u32 status;
#ifdef SDIO_1_BIT /* see also in SdioAdapter.c */
	unsigned long clock_rate = 6000000;
#else
	unsigned long clock_rate = 24000000;
#endif

	printk("TIWLAN SDIO probe WAS CALLED: initializing mmc%d device\n", pdev->id + 1);

	/* remember device struct for future DMA operations */
	g_drv.dev = &pdev->dev;
	g_drv.irq = platform_get_irq(pdev, 0);
	if (g_drv.irq < 0)
		return -ENXIO;
	
	rc= request_irq(g_drv.irq /* OMAP_MMC_IRQ */, sdiodrv_irq, 0, SDIO_DRIVER_NAME, &g_drv);
	if (rc != 0) {
		PERR("sdioDrv_InitHw() -	request_irq FAILED!!\n");
		return rc;
	}
	sdiodrv_irq_requested = 1;
	
	g_drv.fclk = clk_get(&pdev->dev, "mmchs_fck");
	if (IS_ERR(g_drv.fclk)) {
		rc = PTR_ERR(g_drv.fclk);
		PERR("clk_get(fclk) FAILED !!!\n");
		goto err;
	}
	sdiodrv_fclk_got = 1;

	g_drv.iclk	= clk_get(&pdev->dev, "mmchs_ick");
	if (IS_ERR(g_drv.iclk)) {
		rc = PTR_ERR(g_drv.iclk);
		PERR("clk_get(iclk) FAILED !!!\n");
		goto err;
	}
	sdiodrv_iclk_got = 1;

	rc = clk_enable(g_drv.iclk);
	if (rc) {
		PERR("clk_enable(iclk) FAILED !!!\n");
		goto err;
	}
	sdiodrv_iclk_ena = 1;

	rc = clk_enable(g_drv.fclk);
	if (rc) {
		PERR("clk_enable(fclk) FAILED !!!\n");
		goto err;
	}
	sdiodrv_fclk_ena = 1;

	OMAP3430_mmc_reset();

	//obc - init sequence p. 3600,3617
	/* 1.8V */
	OMAP_HSMMC_WRITE(CAPA,		OMAP_HSMMC_READ(CAPA) | VS18);

	OMAP_HSMMC_WRITE(HCTL,		OMAP_HSMMC_READ(HCTL) | SDVS18);
/* bus power ON */
	OMAP_HSMMC_WRITE(HCTL,		OMAP_HSMMC_READ(HCTL) | SDBP);
/* set bus power to 1.8v */
	OMAP_HSMMC_WRITE(HCTL,		OMAP_HSMMC_READ(HCTL) | SDVS18);

	/* clock gating */
	OMAP_HSMMC_WRITE(SYSCONFIG, OMAP_HSMMC_READ(SYSCONFIG) | AUTOIDLE);

	/* interrupts */
	OMAP_HSMMC_WRITE(ISE,		0);
	OMAP_HSMMC_WRITE(IE,		IE_EN_MASK);

	//p. 3601 suggests moving to the end
	OMAP3430_mmc_set_clock(clock_rate, &g_drv);
	printk("SDIO clock Configuration is now set to %dMhz\n",(int)clock_rate/1000000);

	/* Bus width */
#ifdef SDIO_1_BIT /* see also in SdioAdapter.c */
	PDEBUG("%s() setting %d data lines\n",__FUNCTION__, 1);
//	OMAP_HSMMC_WRITE(HCTL, OMAP_HSMMC_READ(HCTL) & (1 << 0));
#else
	PDEBUG("%s() setting %d data lines\n",__FUNCTION__, 4);
	OMAP_HSMMC_WRITE(HCTL, OMAP_HSMMC_READ(HCTL) | (1 << 1));//DTW 4 bits - p. 3650
#endif
	
	/* send the init sequence. 80 clocks of synchronization in the SDIO */
	//doesn't match p. 3601,3617 - obc
	OMAP_HSMMC_WRITE( CON, OMAP_HSMMC_READ(CON) | INIT_STREAM);
	OMAP_HSMMC_SEND_COMMAND( 0, 0);
	status = sdiodrv_poll_status(OMAP_HSMMC_STAT, CC, MMC_TIMEOUT_MS);
	if (!(status & CC)) {
		PERR("sdioDrv_InitHw() SDIO Command error status = 0x%x\n", status);
		rc = -1;
		goto err;
	}
	OMAP_HSMMC_WRITE(CON, OMAP_HSMMC_READ(CON) & ~INIT_STREAM);
	return 0;
err:
	sdiodrv_free_resources();
#endif
	PERR("*********** ERROR sdioDrv_probe() Function was called\n");
	return rc;
}

static int sdioDrv_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "sdioDrv_remove: calling sdiodrv_shutdown\n");
	
	sdiodrv_shutdown();
	
	return 0;
}

#ifdef CONFIG_PM
static int sdioDrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rc = 0;
	
	/* Tell WLAN driver to suspend, if a suspension function has been registered */
	if (g_drv.wlanDrvIf_pm_suspend) {
		printk(KERN_INFO "TISDIO: Asking TIWLAN to suspend\n");
		rc = g_drv.wlanDrvIf_pm_suspend();
		if (rc != 0)
			return rc;
	}

	printk(KERN_INFO "TISDIO: sdioDrv is suspending\n");

	sdiodrv_shutdown();

	return rc;
}

/* Routine to resume the MMC device */
static int sdioDrv_resume(struct platform_device *pdev)
{
	int rc;
	
	printk(KERN_INFO "TISDIO: sdioDrv is resuming\n");
	
	rc = sdioDrv_probe(pdev);
	if (rc != 0) {
		printk(KERN_ERR "TISDIO: resume error\n");
		return rc;
	}

	if (g_drv.wlanDrvIf_pm_resume) {
		printk(KERN_INFO "TISDIO: Asking TIWLAN to resume\n");
		return(g_drv.wlanDrvIf_pm_resume());
	}
	else
		return 0;
}
#else
#define sdioDrv_suspend	NULL
#define sdioDrv_resume	NULL
#endif

static struct platform_driver sdioDrv_struct = {
	.probe		= sdioDrv_probe,
	.remove		= sdioDrv_remove,
	.suspend	= sdioDrv_suspend,
	.resume		= sdioDrv_resume,
	.driver		= {
		.name = SDIO_DRIVER_NAME,
	},
};

void sdioDrv_register_pm(int (*wlanDrvIf_Start)(void),
						int (*wlanDrvIf_Stop)(void))
{
	g_drv.wlanDrvIf_pm_resume = wlanDrvIf_Start;
	g_drv.wlanDrvIf_pm_suspend = wlanDrvIf_Stop;
}

static int __init sdioDrv_init(void)
{
	int rc;
	PDEBUG("entering %s()\n" , __FUNCTION__ );
	memset(&g_drv, 0, sizeof(g_drv));

	printk(KERN_INFO "TIWLAN SDIO init\n");

	rc = sdiodrv_dma_init();
	if (rc != 0) {
		PERR("sdiodrv_init() - sdiodrv_dma_init FAILED!!\n");
		free_irq(IRQ_GPIO, &g_drv);
		return rc;
	}
	sdiodrv_dma_on = 1;

	/* Register the sdio driver */
	return platform_driver_register(&sdioDrv_struct);
}

static void __exit sdioDrv_exit(void)
{
	if (sdiodrv_dma_on) {
		sdiodrv_dma_shutdown();
		sdiodrv_dma_on = 0;
	}

	/* Unregister sdio driver */
	platform_driver_unregister(&sdioDrv_struct);
}

module_init(sdioDrv_init);
module_exit(sdioDrv_exit);

EXPORT_SYMBOL(sdioDrv_ConnectBus);
EXPORT_SYMBOL(sdioDrv_DisconnectBus);
EXPORT_SYMBOL(sdioDrv_ExecuteCmd);
EXPORT_SYMBOL(sdioDrv_ReadSync);
EXPORT_SYMBOL(sdioDrv_WriteSync);
EXPORT_SYMBOL(sdioDrv_ReadAsync);
EXPORT_SYMBOL(sdioDrv_WriteAsync);
EXPORT_SYMBOL(sdioDrv_ReadSyncBytes);
EXPORT_SYMBOL(sdioDrv_WriteSyncBytes);
EXPORT_SYMBOL(sdioDrv_register_pm);
EXPORT_SYMBOL(simulate_prob);


MODULE_DESCRIPTION("TI WLAN SDIO driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS(SDIO_DRIVER_NAME);
MODULE_AUTHOR("Texas Instruments Inc");

