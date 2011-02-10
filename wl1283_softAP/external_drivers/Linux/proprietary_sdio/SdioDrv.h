/*
 * SdioDrv.h
 *
 * Copyright (C) 2009 Texas Instruments, Inc. - http://www.ti.com/
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

#ifndef __OMAP4430_SDIODRV_API_H
#define __OMAP4430_SDIODRV_API_H

#include <asm/types.h>
#include <linux/mmc/mmc.h>

/* Card Common Control Registers (CCCR) */

#define CCCR_IO_ENABLE                      0x02
#define CCCR_IO_READY                       0x03
#define CCCR_INT_ENABLE                     0x04
#define CCCR_BUS_INTERFACE_CONTOROL         0x07

/* Pprotocol defined constants */  
         
#define SD_IO_GO_IDLE_STATE		  		    0  
#define SD_IO_SEND_RELATIVE_ADDR	  	    3 
#define SDIO_CMD5			  			    5
#define SD_IO_SELECT_CARD		  		    7 
#define SDIO_CMD52		 	 			    52		
#define SDIO_CMD53		 	 			    53
#define SD_IO_SEND_OP_COND		            SDIO_CMD5  
#define SD_IO_RW_DIRECT			            SDIO_CMD52 
#define SD_IO_RW_EXTENDED		            SDIO_CMD53 
#define SDIO_SHIFT(v,n)                     (v<<n)
#define SDIO_RWFLAG(v)                      (SDIO_SHIFT(v,31))
#define SDIO_FUNCN(v)                       (SDIO_SHIFT(v,28))
#define SDIO_RAWFLAG(v)                     (SDIO_SHIFT(v,27))
#define SDIO_BLKM(v)                        (SDIO_SHIFT(v,27))
#define SDIO_OPCODE(v)                      (SDIO_SHIFT(v,26))
#define SDIO_ADDRREG(v)                     (SDIO_SHIFT(v,9))


#define VDD_VOLTAGE_WINDOW                  0xffffc0
#define FN2_OBI_INV                         0x0002


#define TRY1_VDD_VOLTAGE_WINDOW             0xffff00

/* HSMMC controller bit definitions
 * */
#define OMAP_HSMMC_CMD_NO_RESPONSE          0 << 0
#define OMAP_HSMMC_CMD_LONG_RESPONSE        1 << 0
#define OMAP_HSMMC_CMD_SHORT_RESPONSE       2 << 0

#define MMC_ERR_NONE	                    0
#define MMC_ERR_TIMEOUT	                    1
#define MMC_ERR_BADCRC	                    2
#define MMC_ERR_FIFO	                    3
#define MMC_ERR_FAILED	                    4
#define MMC_ERR_INVALID	                    5

/********************************************************************/
/*	SDIO driver functions prototypes                                */
/********************************************************************/
int sdioDrv_ConnectBus     (void *       fCbFunc, 
                            void *       hCbArg, 
                            unsigned int uBlkSizeShift, 
                            unsigned int uSdioThreadPriority);

int sdioDrv_DisconnectBus  (void);

int sdioDrv_ExecuteCmd     (unsigned int uCmd, 
                            unsigned int uArg, 
                            unsigned int uRespType, 
                            void *       pResponse, 
                            unsigned int uLen);
                           
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

void sdioDrv_register_pm(int (*wlanDrvIf_Start)(void),
						int (*wlanDrvIf_Stop)(void));
void simulate_prob(void);

int sdioDrv_init(void);
void sdioDrv_exit(void);


#endif/* _OMAP4430_SDIODRV_H */
