/*
 * SdioDrv.c
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

/** \file   SdioDrv.c
 *  \brief  The OMAP3430 Linux SDIO driver (platform and OS dependent)
 *
 * The lower SDIO driver (BSP) for OMAP3430 on Linux OS.
 * Provides all SDIO commands and read/write operation methods.
 *
 *  \see    SdioDrv.h
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <mach/io.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/delay.h>

#include "SdioDrvDbg.h"
#include "SdioDrv.h"


typedef struct OMAP3430_sdiodrv
{
	void          (*BusTxnCB)(void* BusTxnHandle, int status);
	void*         BusTxnHandle;
	unsigned int  uBlkSize;
	unsigned int  uBlkSizeShift;
	void          *async_buffer;
	unsigned int  async_length;
	int           async_status;
	struct device *dev;
	void		(*notify_sdio_ready)(void);
	int			sdio_host_claim_ref;
} OMAP3430_sdiodrv_t;

int g_sdio_debug_level = SDIO_DEBUGLEVEL_ERR;

static OMAP3430_sdiodrv_t g_drv;
static struct sdio_func *tiwlan_func[1 + SDIO_TOTAL_FUNCS];

void sdioDrv_ClaimHost(unsigned int uFunc)
{
    if (g_drv.sdio_host_claim_ref)
        return;

/* currently only wlan sdio function is supported */
    BUG_ON(uFunc != SDIO_WLAN_FUNC);
    BUG_ON(tiwlan_func[uFunc] == NULL);

    g_drv.sdio_host_claim_ref = 1;

    sdio_claim_host(tiwlan_func[uFunc]);
}

void sdioDrv_ReleaseHost(unsigned int uFunc)
{
    if (!g_drv.sdio_host_claim_ref)
        return;

    /* currently only wlan sdio function is supported */
    BUG_ON(uFunc != SDIO_WLAN_FUNC);
    BUG_ON(tiwlan_func[uFunc] == NULL);

    g_drv.sdio_host_claim_ref = 0;

    sdio_release_host(tiwlan_func[uFunc]);
}

int sdioDrv_ConnectBus (void *       fCbFunc, 
                        void *       hCbArg, 
                        unsigned int uBlkSizeShift, 
                        unsigned int uSdioThreadPriority)
                       
{
    printk("%s\n", __FUNCTION__);

    g_drv.BusTxnCB      = fCbFunc;
    g_drv.BusTxnHandle  = hCbArg;
    g_drv.uBlkSizeShift = uBlkSizeShift;
    g_drv.uBlkSize      = 1 << uBlkSizeShift;

    sdioDrv_ClaimHost(SDIO_WLAN_FUNC);

    return 0;
}

int sdioDrv_DisconnectBus (void)
{
    printk("%s\n", __FUNCTION__);

    sdioDrv_ReleaseHost(SDIO_WLAN_FUNC);

    return 0;
}

static int generic_read_bytes(unsigned int uFunc, unsigned int uHwAddr,
				unsigned char *pData, unsigned int uLen,
				unsigned int bIncAddr, unsigned int bMore)
{
	unsigned int i;
	int ret;

	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d bIncAddr %d\n", __func__, uFunc, uHwAddr, (unsigned int)pData, uLen, bIncAddr);

	BUG_ON(uFunc != SDIO_CTRL_FUNC && uFunc != SDIO_WLAN_FUNC);
	
	for (i = 0; i < uLen; i++) {
		if (uFunc == 0)
			*pData = sdio_f0_readb(tiwlan_func[uFunc], uHwAddr, &ret);
		else
			*pData = sdio_readb(tiwlan_func[uFunc], uHwAddr, &ret);

		if (0 != ret) {
			printk(KERN_ERR "%s: function %d sdio error: %d\n", __func__, uFunc, ret);
			return -1;
		}

		pData++;
		if (bIncAddr)
			uHwAddr++;
	}

	return 0;
}

static int generic_write_bytes(unsigned int uFunc, unsigned int uHwAddr,
								unsigned char *pData, unsigned int uLen,
								unsigned int bIncAddr, unsigned int bMore)
{
	unsigned int i;
	int ret;

	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d\n", __func__, uFunc, uHwAddr, (unsigned int) pData, uLen);

	BUG_ON(uFunc != SDIO_CTRL_FUNC && uFunc != SDIO_WLAN_FUNC);

	for (i = 0; i < uLen; i++) {
		if (uFunc == 0)
			sdio_f0_writeb(tiwlan_func[uFunc], *pData, uHwAddr, &ret);
		else
			sdio_writeb(tiwlan_func[uFunc], *pData, uHwAddr, &ret);

		if (0 != ret) {
			printk(KERN_ERR "%s: function %d sdio error: %d\n", __func__, uFunc, ret);
			return -1;
		}

		pData++;
		if (bIncAddr)
			uHwAddr++;
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
	int ret;

	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d bIncAddr %d\n", __func__, uFunc, uHwAddr, (unsigned int)pData, uLen, bIncAddr);

	/* If request is either for sdio function 0 or not a multiple of 4 (OMAP DMA limit)
	    then we have to use CMD 52's */
	if (uFunc == SDIO_CTRL_FUNC || uLen % 4 != 0) {
		ret = generic_read_bytes(uFunc, uHwAddr, pData, uLen, bIncAddr, bMore);
	}
	else {
		if (bIncAddr)
			ret = sdio_memcpy_fromio(tiwlan_func[uFunc], pData, uHwAddr, uLen);
		else
			ret = sdio_readsb(tiwlan_func[uFunc], pData, uHwAddr, uLen);
	}

	if (ret) {
		printk(KERN_ERR "%s: sdio error: %d\n", __func__, ret);
		return -1;
	}

	return 0;
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
	PERR("%s not yet supported!\n", __func__);

	return -1;
}


/*--------------------------------------------------------------------------------------*/

int sdioDrv_WriteSync (unsigned int uFunc, 
                       unsigned int uHwAddr, 
                       void *       pData, 
                       unsigned int uLen,
                       unsigned int bIncAddr,
                       unsigned int bMore)
{
	int ret;

	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d bIncAddr %d\n", __func__, uFunc, uHwAddr, (unsigned int)pData, uLen, bIncAddr);

	/* If request is either for sdio function 0 or not a multiple of 4 (OMAP DMA limit)
	    then we have to use CMD 52's */
	if (uFunc == SDIO_CTRL_FUNC || uLen % 4 != 0)
	{
		ret = generic_write_bytes(uFunc, uHwAddr, pData, uLen, bIncAddr, bMore);
	}
	else
	{
	if (bIncAddr)
		ret = sdio_memcpy_toio(tiwlan_func[uFunc], uHwAddr, pData, uLen);
	else
		ret = sdio_writesb(tiwlan_func[uFunc], uHwAddr, pData, uLen);
	}
     
	if (ret) {
		printk(KERN_ERR "%s: sdio error: %d\n", __func__, ret);
		return -1;
	}

	return 0;
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
	PERR("%s not yet supported!\n", __func__);

	return -1;
}

/*--------------------------------------------------------------------------------------*/

int sdioDrv_ReadSyncBytes (unsigned int  uFunc, 
                           unsigned int  uHwAddr, 
                           unsigned char *pData, 
                           unsigned int  uLen, 
                           unsigned int  bMore)
{
	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d\n", __func__, uFunc, uHwAddr, (unsigned int)pData, uLen);

	return generic_read_bytes(uFunc, uHwAddr, pData, uLen, 1, bMore);
}

/*--------------------------------------------------------------------------------------*/

int sdioDrv_WriteSyncBytes (unsigned int  uFunc, 
                            unsigned int  uHwAddr, 
                            unsigned char *pData, 
                            unsigned int  uLen, 
                            unsigned int  bMore)
{
	PDEBUG("%s: uFunc %d uHwAddr %d pData %x uLen %d\n", __func__, uFunc, uHwAddr, (unsigned int) pData, uLen);

	return generic_write_bytes(uFunc, uHwAddr, pData, uLen, 1, bMore);
}

static void tiwlan_sdio_irq(struct sdio_func *func)
{
	PDEBUG("%s:\n", __func__);
}

int sdioDrv_DisableFunction(unsigned int uFunc)
{
	PDEBUG("%s: func %d\n", __func__, uFunc);

	/* currently only wlan sdio function is supported */
	BUG_ON(uFunc != SDIO_WLAN_FUNC);
	BUG_ON(tiwlan_func[uFunc] == NULL);
	
	return sdio_disable_func(tiwlan_func[uFunc]);
}

int sdioDrv_EnableFunction(unsigned int uFunc)
{
	PDEBUG("%s: func %d\n", __func__, uFunc);

	/* currently only wlan sdio function is supported */
	BUG_ON(uFunc != SDIO_WLAN_FUNC);
	BUG_ON(tiwlan_func[uFunc] == NULL);
	
	return sdio_enable_func(tiwlan_func[uFunc]);
}
int sdioDrv_EnableInterrupt(unsigned int uFunc)
{
	PDEBUG("%s: func %d\n", __func__, uFunc);

	/* currently only wlan sdio function is supported */
	BUG_ON(uFunc != SDIO_WLAN_FUNC);
	BUG_ON(tiwlan_func[uFunc] == NULL);

	return sdio_claim_irq(tiwlan_func[uFunc], tiwlan_sdio_irq);
}

int sdioDrv_DisableInterrupt(unsigned int uFunc)
{
	PDEBUG("%s: func %d\n", __func__, uFunc);

	/* currently only wlan sdio function is supported */
	BUG_ON(uFunc != SDIO_WLAN_FUNC);
	BUG_ON(tiwlan_func[uFunc] == NULL);

	return sdio_release_irq(tiwlan_func[uFunc]);
}

int sdioDrv_SetBlockSize(unsigned int uFunc, unsigned int blksz)
{
	PDEBUG("%s: func %d\n", __func__, uFunc);

	/* currently only wlan sdio function is supported */
	BUG_ON(uFunc != SDIO_WLAN_FUNC);
	BUG_ON(tiwlan_func[uFunc] == NULL);

	return sdio_set_block_size(tiwlan_func[uFunc], blksz);
}
static int tiwlan_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	PDEBUG("TIWLAN: probed with vendor 0x%x, device 0x%x, class 0x%x\n",
		func->vendor, func->device, func->class);

	if (func->vendor != SDIO_VENDOR_ID_TI ||
		func->device != SDIO_DEVICE_ID_TI_WL12xx ||
		func->class != SDIO_CLASS_WLAN)
		return -ENODEV;

	printk(KERN_INFO "TIWLAN: Found TI/WLAN SDIO controller (vendor 0x%x, device 0x%x, class 0x%x)\n",
           func->vendor, func->device, func->class);

	tiwlan_func[SDIO_WLAN_FUNC] = func;
	tiwlan_func[SDIO_CTRL_FUNC] = func;

	return 0;
}

static void tiwlan_sdio_remove(struct sdio_func *func)
{
	PDEBUG("%s\n", __func__);

	tiwlan_func[SDIO_WLAN_FUNC] = NULL;
	tiwlan_func[SDIO_CTRL_FUNC] = NULL;
}

static const struct sdio_device_id tiwl12xx_devices[] = {
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_WLAN) },
       {}
};
MODULE_DEVICE_TABLE(sdio, tiwl12xx_devices);

static struct sdio_driver tiwlan_sdio_drv = {
    .probe          = tiwlan_sdio_probe,
    .remove         = tiwlan_sdio_remove,
    .name           = "sdio_tiwlan",
    .id_table       = tiwl12xx_devices,
};

int sdioDrv_init(void)
{
	int ret;

	PDEBUG("%s: Debug mode\n", __func__);

	memset(&g_drv, 0, sizeof(g_drv));

	ret = sdio_register_driver(&tiwlan_sdio_drv);

	if (ret < 0) {
		printk(KERN_ERR "sdioDrv_init: sdio register failed: %d\n", ret);
		goto out;
	}

	printk(KERN_INFO "TI WiLink 1271 SDIO: Driver loaded\n");

out:
	return ret;
}

void sdioDrv_exit(void)
{
	sdio_unregister_driver(&tiwlan_sdio_drv);
	printk(KERN_INFO "TI WiLink 1271 SDIO Driver unloaded\n");
}

#if 0
module_init(sdioDrv_init);
module_exit(sdioDrv_exit);
module_param(g_sdio_debug_level, int, SDIO_DEBUGLEVEL_ERR);
MODULE_PARM_DESC(g_sdio_debug_level, "TIWLAN SDIO debug level");
EXPORT_SYMBOL(g_sdio_debug_level);
EXPORT_SYMBOL(sdioDrv_ConnectBus);
EXPORT_SYMBOL(sdioDrv_DisconnectBus);
EXPORT_SYMBOL(sdioDrv_ReadSync);
EXPORT_SYMBOL(sdioDrv_WriteSync);
EXPORT_SYMBOL(sdioDrv_ReadAsync);
EXPORT_SYMBOL(sdioDrv_WriteAsync);
EXPORT_SYMBOL(sdioDrv_ReadSyncBytes);
EXPORT_SYMBOL(sdioDrv_WriteSyncBytes);
EXPORT_SYMBOL(sdioDrv_EnableFunction);
EXPORT_SYMBOL(sdioDrv_EnableInterrupt);
EXPORT_SYMBOL(sdioDrv_DisableFunction);
EXPORT_SYMBOL(sdioDrv_DisableInterrupt);
EXPORT_SYMBOL(sdioDrv_SetBlockSize);
EXPORT_SYMBOL(sdioDrv_Register_Notification);
MODULE_DESCRIPTION("TI WLAN 1271 SDIO interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS(SDIO_DRIVER_NAME);
MODULE_AUTHOR("Texas Instruments");
#endif

