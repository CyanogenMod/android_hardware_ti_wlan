/*
 * testdrv.h
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

#ifndef _MMC_TEST_H_
#define _MMC_TEST_H_

#define TESTDRV_MODULE_NAME "sdio_test"

#ifdef  TESTDRV_CONFIG_MMC_TEST_DEBUG
#define DBG(x...)	printk(x)
#else
#define DBG(x...)	do { } while (0)
#endif

#define DRPW_BASE		         0x00310000
#define REGISTERS_BASE			 0x00300000

#define PARTITION_DOWN_REG_ADDR       REGISTERS_BASE	
#define PARTITION_DOWN_REG_SIZE       0x8800            




#define TXN_FUNC_ID_CTRL         0
#define TXN_FUNC_ID_BT           1
#define TXN_FUNC_ID_WLAN         2

#define TESTDRV_GPIO_INITIAL_VALUE_FOR_OUTPUT   0
#define TESTDRV_MUXMODE_3                       3
#define TESTDRV_TIWLAN_IRQ                     (OMAP_GPIO_IRQ(TESTDRV_GPIO_9))

#define TESTDRV_SDIO_FUNC1_OFFSET           	0x1FFC0  /* address of the partition table */

#define SDIO_TEST_FIRST_VALID_DMA_ADDR			(0x00000008)	/* used for escaping addressing invalid DMA Addresses */
#define SDIO_TEST_NO_OF_TRANSACTIONS			(3)

#define TESTDRV_512_SDIO_BLOCK					(512)
#define TESTDRV_MAX_SDIO_BLOCK					(TESTDRV_512_SDIO_BLOCK /* - 4 */)

#define TESTDRV_MAX_PART_SIZE  					0x1F000 	/* 124k	*/	 

#define TESTDRV_CODE_RAM_SIZE  					0x30000		/* 192K	*/
#define TESTDRV_DATA_RAM_SIZE 					0xC000		/* 48K 	*/
#define TESTDRV_PACKET_RAM_SIZE 				0xD000		/* 52K 	*/

#define TESTDRV_REG_PART_START_ADDR 			0x300000
#define TESTDRV_REG_DOWNLOAD_PART_SIZE 			0x8800 		/* 44k	*/ 	
#define TESTDRV_REG_WORKING_PART_SIZE 			0xB000 		/* 44k	*/ 	

#define TESTDRV_CODE_RAM_PART_START_ADDR 		0		
#define TESTDRV_DATA_RAM_PART_START_ADDR 		0x20000000
#define TESTDRV_PACKET_RAM_PART_START_ADDR 		0x40000

/* Partition Size Left for Memory */
#define TESTDRV_MEM_WORKING_PART_SIZE  			(TESTDRV_MAX_PART_SIZE - TESTDRV_REG_WORKING_PART_SIZE) 	 
#define TESTDRV_MEM_DOWNLOAD_PART_SIZE  		(TESTDRV_MAX_PART_SIZE - TESTDRV_REG_DOWNLOAD_PART_SIZE) 	 

#define TESTDRV_TESTING_DATA_LENGTH 512
#endif /* _MMC_TEST_H_ */

