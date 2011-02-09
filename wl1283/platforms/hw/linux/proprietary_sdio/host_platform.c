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
#include "tidef.h"
#include <linux/kernel.h>
#include <asm/io.h>
#include <mach/tc.h>
#include <linux/delay.h>

#include "host_platform.h"
#include "ioctl_init.h"
#include "WlanDrvIf.h"

#include "Device1273.h"
#include "SdioDrv.h"


#define OS_API_MEM_ADRR  	0x0000000
#define OS_API_REG_ADRR  	0x300000

#define SDIO_ATTEMPT_LONGER_DELAY_LINUX  150

/*--------------------------------------------------------------------------------------*/

static void pad_config(unsigned long pad_addr, u32 andmask, u32 ormask)
{
	int val;
	u32 *addr;

	addr = (u32 *) ioremap(pad_addr, 4);
	if (!addr) {
		printk(KERN_ERR "OMAP3430_pad_config: ioremap failed with addr %lx\n", pad_addr);
		return;
	}

	val =  __raw_readl(addr);
	val &= andmask;
	val |= ormask;
	__raw_writel(val, addr);

	iounmap(addr);
}

static int OMAP3430_TNETW_Power(int power_on)
{
	if (power_on) {
		gpio_set_value(PMENA_GPIO, 1);
	} else {
		gpio_set_value(PMENA_GPIO, 0);
	}

	return 0;    
}

/*-----------------------------------------------------------------------------

Routine Name:

        hPlatform_hardResetTnetw

Routine Description:

        set the GPIO to low after awaking the TNET from ELP.

Arguments:

        OsContext - our adapter context.


Return Value:

        None

-----------------------------------------------------------------------------*/

int hPlatform_hardResetTnetw(void)
{
  int err;

    /* Turn power OFF*/
  if ((err = OMAP3430_TNETW_Power(0)) == 0)
  {
    mdelay(500);
    /* Turn power ON*/
    err = OMAP3430_TNETW_Power(1);
    mdelay(50);
  }
  return err;

} /* hPlatform_hardResetTnetw() */

/* Turn device power off */
int hPlatform_DevicePowerOff (void)
{
    int err;
    
    err = OMAP3430_TNETW_Power(0);
    
    mdelay(10);
    
    return err;
}

/*--------------------------------------------------------------------------------------*/

/* Turn device power off according to a given delay */
int hPlatform_DevicePowerOffSetLongerDelay(void)
{
    int err;
    
    err = OMAP3430_TNETW_Power(0);
    
    mdelay(SDIO_ATTEMPT_LONGER_DELAY_LINUX);
    
    return err;
}

/* Turn device power on */
int hPlatform_DevicePowerOn (void)
{
    int err;

    err = OMAP3430_TNETW_Power(1);

    /* New Power Up Sequence */
    mdelay(15);
    err = OMAP3430_TNETW_Power(0);
    mdelay(1);

    err = OMAP3430_TNETW_Power(1);

    /* Should not be changed, 50 msec cause failures */
    mdelay(70);

    return err;
}

/*--------------------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------

Routine Name:

        InitInterrupt

Routine Description:

        this function init the interrupt to the Wlan ISR routine.

Arguments:

        tnet_drv - Golbal Tnet driver pointer.


Return Value:

        status

-----------------------------------------------------------------------------*/

int hPlatform_initInterrupt(void *tnet_drv, void* handle_add ) 
{
	TWlanDrvIfObj *drv = tnet_drv;
    int rc;

	if (drv->irq == 0 || handle_add == NULL)
	{
	  print_err("hPlatform_initInterrupt() bad param drv->irq=%d handle_add=0x%x !!!\n",drv->irq,(int)handle_add);
	  return -EINVAL;
	}
	printk("hPlatform_initInterrupt: call gpio_request %d\n", IRQ_GPIO);
	if (gpio_request(IRQ_GPIO, NULL) != 0) 
    {
	    print_err("hPlatform_initInterrupt() gpio_request() FAILED !!\n");
		return -EINVAL;
	}
	gpio_direction_input(IRQ_GPIO);
#ifdef USE_IRQ_ACTIVE_HIGH
	printk("hPlatform_initInterrupt: call request_irq IRQF_TRIGGER_RISING\n");
	if ((rc = request_irq(drv->irq, handle_add, IRQF_TRIGGER_RISING, drv->netdev->name, drv)))
#else
	printk("hPlatform_initInterrupt: call request_irq IRQF_TRIGGER_FALLING\n");
	if ((rc = request_irq(drv->irq, handle_add, IRQF_TRIGGER_FALLING, drv->netdev->name, drv)))
#endif
	{
	    print_err("TIWLAN: Failed to register interrupt handler\n");
		gpio_free(IRQ_GPIO);
		return rc;
	}

	if (gpio_request(PMENA_GPIO, NULL) != 0)
	{
		printk(KERN_ERR "%s:OMAP2430_TNETW_Power() omap_request_gpio FAILED\n",__FILE__);
	    gpio_free(IRQ_GPIO);
		return -EINVAL;
	};
	gpio_direction_output(PMENA_GPIO, 0);

	printk("hPlatform_initInterrupt: calling simulate_prob function\n");
	simulate_prob();

	return rc;

} /* hPlatform_initInterrupt() */

/*--------------------------------------------------------------------------------------*/

void hPlatform_freeInterrupt(void) 
{
	gpio_free(IRQ_GPIO);
}

void hPlatform_Wlan_Hardware_DeInit(void)
{
  gpio_free(PMENA_GPIO);

}

