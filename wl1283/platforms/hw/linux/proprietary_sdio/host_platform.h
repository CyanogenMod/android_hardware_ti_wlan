/*
 * host_platform.h
 *
 * Copyright(c) 1998 - 2009 Texas Instruments. All rights reserved.      
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
/*--------------------------------------------------------------------------
 Module:      host_platform_sdio.h

 Purpose:     This module defines unified interface to the host platform specific
              sources and services.

--------------------------------------------------------------------------*/

#ifndef __HOST_PLATFORM_SDIO__H__
#define __HOST_PLATFORM_SDIO__H__


#include <mach/mmc.h>
#include <mach/io.h>


/*OMAP4 MMC5 delerations:

OMAP4_MMC5_BASE
A1_4430_MMC5_CLK,
C1_4430_MMC5_CMD,
D0_4430_MMC5_DAT0,
D1_4430_MMC5_DAT1,
D2_4430_MMC5_DAT2,
D3_4430_MMC5_DAT3,

OMAP4_MMC_REG_OFFSET
HSMMC5
*/
#ifdef OMAP3_DEFINES
#define CONTROL_PADCONF_MMC3_DAT0		0x480025E4    /* mmc3_dat0, mmc3_dat1 */
#define CONTROL_PADCONF_MMC3_DAT2		0x480025E8    /* mmc3_dat2 */
#define CONTROL_PADCONF_MMC3_DAT3		0x480025E0    /* mmc3_dat3 */
#define CONTROL_PADCONF_MCBSP1_CLKX		0x48002184   /* WLAN_IRQ */
#define MMCHS_SYSCONFIG_OMAP3_MMC3 		0x480AD010
#define MMCHS_SYSCTL_OMAP3_MMC2			0x480b412c
#define MMCHS_SYSCTL_OMAP3_MMC3			0x480AD12C
#define MMCHS_SYSCONFIG					MMCHS_SYSCONFIG_OMAP3_MMC3
#define MMCHS_SYSCTL					MMCHS_SYSCTL_OMAP3_MMC3
#else
#define MMC_PADCONF_CLK                 A1_4430_MMC5_CLK
#define MMC_PADCONF_CMD                 C1_4430_MMC5_CMD
#define MMC_PADCONF_DAT0                D0_4430_MMC5_DAT0
#define MMC_PADCONF_DAT1                D0_4430_MMC5_DAT1
#define MMC_PADCONF_DAT2                D0_4430_MMC5_DAT2
#define MMC_PADCONF_DAT3                D0_4430_MMC5_DAT3
#define MMCHS_SYSCONFIG_OMAP4_MMC5      0x480D5110
#define MMCHS_SYSCTL_OMAP4_MMC5			0x480D522C
#define MMCHS_SYSCONFIG					MMCHS_SYSCONFIG_OMAP4_MMC5
#define MMCHS_SYSCTL					MMCHS_SYSCTL_OMAP4_MMC5

#endif /*OMAP3_DEFINES*/

#define TIWLAN_MMC_CONTROLLER_BASE_ADDR  OMAP4_MMC5_BASE /*0x480d5000*/
//#define OMAP_MMC_REGS_BASE               IO_ADDRESS(TIWLAN_MMC_CONTROLLER_BASE_ADDR)

#define OMAP_MMC_REGS_BASE               ioremap(OMAP4_MMC5_BASE + OMAP4_MMC_REG_OFFSET, SZ_4K)
/*
 *  HSMMC Host Controller Registers (taekn from file: kernel/drivers/mmc/host/omap_hsmmc.c)
 */
/* OMAP HSMMC Host Controller Registers */

#define OMAP_HSMMC_SYSCONFIG    0x0010
#define OMAP_HSMMC_CON      0x002C
#define OMAP_HSMMC_BLK      0x0104
#define OMAP_HSMMC_ARG      0x0108
#define OMAP_HSMMC_CMD      0x010C
#define OMAP_HSMMC_RSP10    0x0110
#define OMAP_HSMMC_RSP32    0x0114
#define OMAP_HSMMC_RSP54    0x0118
#define OMAP_HSMMC_RSP76    0x011C
#define OMAP_HSMMC_DATA     0x0120
#define OMAP_HSMMC_HCTL     0x0128
#define OMAP_HSMMC_SYSCTL   0x012C
#define OMAP_HSMMC_STAT     0x0130
#define OMAP_HSMMC_IE       0x0134
#define OMAP_HSMMC_ISE      0x0138
#define OMAP_HSMMC_CAPA     0x0140
#define VS18            (1 << 26)
#define VS30            (1 << 25)
#define SDVS18          (0x5 << 9)
#define SDVS30          (0x6 << 9)
#define SDVS33          (0x7 << 9)
#define SDVS_MASK       0x00000E00
#define SDVSCLR         0xFFFFF1FF
#define SDVSDET         0x00000400
#define AUTOIDLE        0x1
#define SDBP            (1 << 8)
#define DTO         0xe
#define ICE         0x1
#define ICS         0x2
#define CEN         (1 << 2)
#define CLKD_MASK       0x0000FFC0
#define CLKD_SHIFT      6
#define DTO_MASK        0x000F0000
#define DTO_SHIFT       16
#define INT_EN_MASK     0x307F0033
#define INIT_STREAM     (1 << 1)
#define DP_SELECT       (1 << 21)
#define DDIR            (1 << 4)
#define DMA_EN          0x1
#define MSBS            (1 << 5)
#define BCE         (1 << 1)
#define FOUR_BIT        (1 << 1)
#define DW8         (1 << 5)
#define CC          0x1
#define TC          0x02
#define OD          0x1
#define ERR         (1 << 15)
#define CMD_TIMEOUT     (1 << 16)
#define DATA_TIMEOUT        (1 << 20)
#define CMD_CRC         (1 << 17)
#define DATA_CRC        (1 << 21)
#define CARD_ERR        (1 << 28)
#define STAT_CLEAR      0xFFFFFFFF
#define INIT_STREAM_CMD     0x00000000
#define DUAL_VOLT_OCR_BIT   7
#define SRC         (1 << 25)
#define SRD         (1 << 26)

/* END OF OMAP kernel file copy*/


/*added by benzy from TRM*/

#define OMAP_HSMMC_PSTATE              0x0224
#define BRE                            (1 << 11)
#define BWE                            (1 << 10)
#define SRA                            (1 << 24)

/*END of - added by benzy from TRM*/
#define IE_EN_MASK                     0x317F0137


/* 1283 definitions*/
#define IRQ_GPIO                        53
#define PMENA_GPIO                      54
#define TNETW_IRQ                       (OMAP_GPIO_IRQ(IRQ_GPIO))


/**
 * \typedef TI_HANDLE
 * \brief Handle type - Pointer to void
 */

int 
hPlatform_initInterrupt(
	void* tnet_drv,
	void* handle_add
	);

void hPlatform_freeInterrupt(void);

int  hPlatform_hardResetTnetw(void);
int  hPlatform_Wlan_Hardware_Init(void);
void hPlatform_Wlan_Hardware_DeInit(void);
int  hPlatform_DevicePowerOff(void);
int  hPlatform_DevicePowerOffSetLongerDelay(void);
int  hPlatform_DevicePowerOn(void);
#endif /* __HOST_PLATFORM_SDIO__H__ */
