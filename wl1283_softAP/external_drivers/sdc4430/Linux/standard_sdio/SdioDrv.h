/*
 * SdioDrv.h
 *
 * Copyright (C) 2009 Texas Instruments, Inc. - http://www.ti.com/
 *
 * Contributed by Ohad Ben-Cohen <ohad@bencohen.org>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __OMAP3430_SDIODRV_API_H
#define __OMAP3430_SDIODRV_API_H

#include <asm/types.h>
#include <linux/mmc/mmc.h>

#define TIWLAN_MMC_MAX_DMA			(8192)
#define SDIO_DRIVER_NAME 			"TIWLAN_SDIO"
#define SDIO_TOTAL_FUNCS			(2)
#define SDIO_WLAN_FUNC				(2)
#define SDIO_BT_FUNC				(1)
#define SDIO_CTRL_FUNC				(0)

/* Card Common Control Registers (CCCR) */

#define CCCR_SDIO_REVISION                  0x00
#define CCCR_SD_SPECIFICATION_REVISION      0x01
#define CCCR_IO_ENABLE                      0x02
#define CCCR_IO_READY                       0x03
#define CCCR_INT_ENABLE                     0x04
#define CCCR_INT_PENDING                    0x05
#define CCCR_IO_ABORT                       0x06
#define CCCR_BUS_INTERFACE_CONTOROL         0x07
#define CCCR_CARD_CAPABILITY	            0x08
#define CCCR_COMMON_CIS_POINTER             0x09 /*0x09-0x0B*/
#define CCCR_FNO_BLOCK_SIZE	                0x10 /*0x10-0x11*/
#define FN0_CCCR_REG_32                     0x64

/* Protocol defined constants */
         
#define SD_IO_GO_IDLE_STATE		  		    0  
#define SD_IO_SEND_RELATIVE_ADDR	  	    3 
#define SDIO_CMD5			  			    5
#define SD_IO_SELECT_CARD		  		    7 

#define VDD_VOLTAGE_WINDOW                  0xffffc0

/********************************************************************/
/*	SDIO driver functions prototypes                                */
/********************************************************************/
int sdioDrv_ConnectBus     (void *       fCbFunc,
                            void *       hCbArg,
                            unsigned int uBlkSizeShift,
                            unsigned int uSdioThreadPriority);

int sdioDrv_DisconnectBus  (void);

int sdioDrv_ReadSync       (unsigned int uFunc, 
                            unsigned int uHwAddr, 
                            void *       pData, 
                            unsigned int uLen, 
                            unsigned int bIncAddr,
                            unsigned int bMore);

int sdioDrv_ReadAsync      (unsigned int uFunc, 
                            unsigned int uHwAddr, 
                            void *       pData, 
                            unsigned int uLen, 
                            unsigned int bBlkMode,
                            unsigned int bIncAddr,
                            unsigned int bMore);

int sdioDrv_WriteSync      (unsigned int uFunc, 
                            unsigned int uHwAddr, 
                            void *       pData, 
                            unsigned int uLen,
                            unsigned int bIncAddr,
                            unsigned int bMore);

int sdioDrv_WriteAsync     (unsigned int uFunc, 
                            unsigned int uHwAddr, 
                            void *       pData, 
                            unsigned int uLen, 
                            unsigned int bBlkMode,
                            unsigned int bIncAddr,
                            unsigned int bMore);

int sdioDrv_ReadSyncBytes  (unsigned int  uFunc, 
                            unsigned int  uHwAddr, 
                            unsigned char *pData, 
                            unsigned int  uLen, 
                            unsigned int  bMore);
                           
int sdioDrv_WriteSyncBytes (unsigned int  uFunc, 
                            unsigned int  uHwAddr, 
                            unsigned char *pData, 
                            unsigned int  uLen, 
                            unsigned int  bMore);

int sdioDrv_EnableFunction(unsigned int uFunc);
int sdioDrv_EnableInterrupt(unsigned int uFunc);
int sdioDrv_DisableFunction(unsigned int uFunc);
int sdioDrv_DisableInterrupt(unsigned int uFunc);
int sdioDrv_SetBlockSize(unsigned int uFunc, unsigned int blksz);
void sdioDrv_Register_Notification(void (*notify_sdio_ready)(void));
void sdioDrv_ReleaseHost(unsigned int uFunc);
void sdioDrv_ClaimHost(unsigned int uFunc);

int sdioDrv_init(void);
void sdioDrv_exit(void);

#endif/* _OMAP3430_SDIODRV_H */
