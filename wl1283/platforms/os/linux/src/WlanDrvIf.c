/*
 * WlanDrvIf.c
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


/*
 * src/wlandrvif.c
 *
 * Kernel level portion of eSTA DK Linux module driver
 *
 */


/** \file   WlanDrvIf.c 
 *  \brief  The OS-Dependent interfaces of the WLAN driver with external applications:
 *          - Configuration utilities (including download, configuration and activation)
 *          - Network Stack (Tx and Rx)
 *          - Interrupts
 *          - Events to external applications
 *  
 *  \see    WlanDrvIf.h, Wext.c
 */


#include <net/sock.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/wireless.h>


#include "WlanDrvIf.h"
#include "osApi.h"
#include "host_platform.h"
#include "context.h"
#include "CmdHndlr.h"
#include "WlanDrvWext.h"
#include "DrvMain.h"
#include "txDataQueue_Api.h"
#include "txMgmtQueue_Api.h"
#include "TWDriver.h"
#include "Ethernet.h"
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include "pwrState_Types.h"

#ifdef TI_DBG
#include "tracebuf_api.h"
#endif
#include "bmtrace_api.h"
#ifdef STACK_PROFILE
#include "stack_profile.h"
#endif

#include "SdioDrv.h"

/* save driver handle just for module cleanup */
static TWlanDrvIfObj *pDrvStaticHandle;

#define OS_SPECIFIC_RAM_ALLOC_LIMIT			(0xFFFFFFFF)	/* assume OS never reach that limit */


MODULE_DESCRIPTION("TI WLAN Embedded Station Driver");
MODULE_LICENSE("Dual BSD/GPL");


int wlanDrvIf_Stop (struct net_device *dev);
int wlanDrvIf_Open(struct net_device *dev);
int wlanDrvIf_Release(struct net_device *dev);
int wlanDrvIf_LoadFiles (TWlanDrvIfObj *drv, TLoaderFilesData *pInitFiles);
static int wlanDrvIf_Suspend(TI_HANDLE hWlanDrvIf);
static int wlanDrvIf_Resume(TI_HANDLE hWlanDrvIf);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 31))
static int wlanDrvIf_Xmit(struct sk_buff *skb, struct net_device *dev);
static int wlanDrvIf_XmitDummy(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *wlanDrvIf_NetGetStat(struct net_device *dev);

static struct net_device_ops tiwlan_ops_pri = {
	.ndo_open = wlanDrvIf_Open,
	.ndo_stop = wlanDrvIf_Release,
	.ndo_get_stats = wlanDrvIf_NetGetStat,
	.ndo_do_ioctl = NULL,
	.ndo_start_xmit = wlanDrvIf_Xmit,
};

static struct net_device_ops tiwlan_ops_dummy = {
	.ndo_open = wlanDrvIf_Open,
	.ndo_stop = wlanDrvIf_Release,
	.ndo_get_stats = wlanDrvIf_NetGetStat,
	.ndo_do_ioctl = NULL,
	.ndo_start_xmit = wlanDrvIf_XmitDummy,
};
#endif

/*
 * \brief	execute a private command FROM KERNEL SPACE
 *
 * \context	EXTERNAL (not driver)
 *
 * \param	tPrivCmd	command to execute
 * 						NOTE: the in/out buffers of the command MUST be in KERNEL space
 *
 * \return	TI_OK upon success; TI_NOK upon failure
 */
static int wlanDrvIf_KExecPrivCmd(TI_HANDLE hWlanDrvIf, ti_private_cmd_t tPrivCmd)
{
	int rc;
	TWlanDrvIfObj *this = (TWlanDrvIfObj *) hWlanDrvIf;

	/* if command is async, or has output buffer - set as a GET command
	 * (commands from user-space have this field set in IPC_STA_Private_Send()) */
	tPrivCmd.flags = (tPrivCmd.out_buffer || IS_PARAM_ASYNC(tPrivCmd.cmd)) ? PRIVATE_CMD_GET_FLAG : PRIVATE_CMD_SET_FLAG;

	/* Call the Cmd module with the given user paramters */
	rc = cmdHndlr_InsertCommand(this->tCommon.hCmdHndlr, SIOCIWFIRSTPRIV, 0,
			NULL /* not used by SIOCIWFIRSTPRIV */, 0,
			NULL /* not used by SIOCIWFIRSTPRIV */, 0, (TI_UINT32*)&tPrivCmd, NULL);

	return rc;
}

/** 
 * \fn     wlanDrvIf_Xmit
 * \brief  Packets transmission
 * 
 * The network stack calls this function in order to transmit a packet
 *     through the WLAN interface.
 * The packet is inserted to the drver Tx queues and its handling is continued
 *     after switching to the driver context.
 *
 * \note   
 * \param  skb - The Linux packet buffer structure
 * \param  dev - The driver network-interface handle
 * \return 0 (= OK)
 * \sa     
 */ 
static int wlanDrvIf_Xmit (struct sk_buff *skb, struct net_device *dev)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)NETDEV_GET_PRIVATE(dev);
	TTxCtrlBlk *  pPktCtrlBlk;
    int status;

    CL_TRACE_START_L1();

    os_profile (drv, 0, 0);
	drv->stats.tx_packets++;
	drv->stats.tx_bytes += skb->len;

	/* Allocate a TxCtrlBlk for the Tx packet and save timestamp, length and packet handle */
    pPktCtrlBlk = TWD_txCtrlBlk_Alloc (drv->tCommon.hTWD);

    if ( NULL == pPktCtrlBlk )
    {
        os_printf (" wlanDrvIf_Xmit() :  pPktCtrlBlk returned as NULL from TWD_txCtrlBlk_Alloc()" );
        return 1;
    }

    pPktCtrlBlk->tTxDescriptor.startTime    = os_timeStampMs(drv); /* remove use of skb->tstamp.off_usec */
    pPktCtrlBlk->tTxDescriptor.length       = skb->len;
    pPktCtrlBlk->tTxPktParams.pInputPkt     = skb;

	/* Point the first BDL buffer to the Ethernet header, and the second buffer to the rest of the packet */
	pPktCtrlBlk->tTxnStruct.aBuf[0] = skb->data;
	pPktCtrlBlk->tTxnStruct.aLen[0] = ETHERNET_HDR_LEN;
	pPktCtrlBlk->tTxnStruct.aBuf[1] = skb->data + ETHERNET_HDR_LEN;
	pPktCtrlBlk->tTxnStruct.aLen[1] = (TI_UINT16)skb->len - ETHERNET_HDR_LEN;
	pPktCtrlBlk->tTxnStruct.aLen[2] = 0;

    /* Send the packet to the driver for transmission. */
    status = txDataQ_InsertPacket (drv->tCommon.hTxDataQ, pPktCtrlBlk,(TI_UINT8)skb->priority);

	/* If failed (queue full or driver not running), drop the packet. */
    if (status != TI_OK)
    {
        drv->stats.tx_errors++;
    }
    os_profile (drv, 1, 0);

    CL_TRACE_END_L1("tiwlan_drv.ko", "OS", "TX", "");

    return 0;
}
/*--------------------------------------------------------------------------------------*/
/** 
 * \fn     wlanDrvIf_FreeTxPacket
 * \brief  Free the OS Tx packet
 * 
 * Free the OS Tx packet after driver processing is finished.
 *
 * \note   
 * \param  hOs          - The OAL object handle
 * \param  pPktCtrlBlk  - The packet CtrlBlk
 * \param  eStatus      - The packet transmission status (OK/NOK)
 * \return void
 * \sa     
 */ 
/*--------------------------------------------------------------------------------------*/

void wlanDrvIf_FreeTxPacket (TI_HANDLE hOs, TTxCtrlBlk *pPktCtrlBlk, TI_STATUS eStatus)
{
	dev_kfree_skb((struct sk_buff *)pPktCtrlBlk->tTxPktParams.pInputPkt);
}

/** 
 * \fn     wlanDrvIf_XmitDummy
 * \brief  Dummy transmission handler
 * 
 * This function is registered at the network stack interface as the packets-transmission
 *     handler (replacing wlanDrvIf_Xmit) when the driver is not operational.
 * Using this dummy handler is more efficient then checking the driver state for every 
 *     packet transmission.
 *
 * \note   
 * \param  skb - The Linux packet buffer structure
 * \param  dev - The driver network-interface handle
 * \return error 
 * \sa     wlanDrvIf_Xmit
 */ 
static int wlanDrvIf_XmitDummy (struct sk_buff *skb, struct net_device *dev)
{
    /* Just return error. The driver is not running (network stack frees the packet) */
    return -ENODEV;
}


/** 
 * \fn     wlanDrvIf_NetGetStat
 * \brief  Provides driver statistics
 * 
 * Provides driver Tx and Rx statistics to network stack.
 *
 * \note   
 * \param  dev - The driver network-interface handle
 * \return The statistics pointer 
 * \sa     
 */ 
static struct net_device_stats *wlanDrvIf_NetGetStat (struct net_device *dev)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)NETDEV_GET_PRIVATE(dev);
    ti_dprintf (TIWLAN_LOG_OTHER, "wlanDrvIf_NetGetStat()\n");
    
    return &drv->stats;
}


/** 
 * \fn     wlanDrvIf_UpdateDriverState
 * \brief  Update the driver state
 * 
 * The DrvMain uses this function to update the OAL with the driver steady state
 *     that is relevant for the driver users.
 * 
 * \note   
 * \param  hOs          - The driver object handle
 * \param  eDriverState - The new driver state
 * \return void
 * \sa     
 */ 
void wlanDrvIf_UpdateDriverState (TI_HANDLE hOs, EDriverSteadyState eDriverState)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hOs;

    ti_dprintf(TIWLAN_LOG_OTHER, "wlanDrvIf_UpdateDriverState(): State = %d\n", eDriverState);

    /* Save the new state */
    drv->tCommon.eDriverState = eDriverState;

    /* If the new state is not RUNNING, replace the Tx handler to a dummy one. */
    if (eDriverState != DRV_STATE_RUNNING)
    {
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 31))
		drv->netdev->hard_start_xmit = wlanDrvIf_XmitDummy;
#else
		drv->netdev->netdev_ops = &tiwlan_ops_dummy;
#endif
    }
    else
    {
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 31))
	drv->netdev->hard_start_xmit = wlanDrvIf_Xmit;
#else
	drv->netdev->netdev_ops = &tiwlan_ops_pri;
#endif
    }
}


/** 
 * \fn     wlanDrvIf_HandleInterrupt
 * \brief  The WLAN interrupt handler
 * 
 * The WLAN driver interrupt handler called in the interrupt context.
 * The actual handling is done in the driver's context after switching to the workqueue.
 * 
 * \note   
 * \param  irq      - The interrupt type
 * \param  hDrv     - The driver object handle
 * \param  cpu_regs - The CPU registers
 * \return IRQ_HANDLED
 * \sa     
 */ 
irqreturn_t wlanDrvIf_HandleInterrupt (int irq, void *hDrv, struct pt_regs *cpu_regs)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hDrv;

#ifdef OMAP_LEVEL_INT
    disable_irq_nosync(drv->irq);
#endif

    TWD_InterruptRequest (drv->tCommon.hTWD);

    return IRQ_HANDLED;
}


/** 
 * \fn     PollIrqHandler
 * \brief  WLAN IRQ polling handler - for debug!
 * 
 * A debug option to catch the WLAN events in polling instead of interrupts.
 * A timer calls this function periodically to check the interrupt status register.
 * 
 * \note   
 * \param  parm - The driver object handle
 * \return void
 * \sa     
 */ 
#ifdef PRIODIC_INTERRUPT
static void wlanDrvIf_PollIrqHandler (TI_HANDLE parm)
{
   TWlanDrvIfObj *drv = (TWlanDrvIfObj *)parm;

   wlanDrvIf_HandleInterrupt (0, drv, NULL);
   os_periodicIntrTimerStart (drv);
}
#endif


/** 
 * \fn     wlanDrvIf_DriverTask
 * \brief  The driver task
 * 
 * This is the driver's task, where most of its job is done.
 * External contexts just save required information and schedule the driver's
 *     task to continue the handling.
 * See more information in the context engine module (context.c).
 *
 * \note   
 * \param  hDrv - The driver object handle
 * \return void
 * \sa
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
static void wlanDrvIf_DriverTask (void *hDrv)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hDrv;
#else
static void wlanDrvIf_DriverTask(struct work_struct *work)
{
#ifdef STACK_PROFILE
    register unsigned long sp asm ("sp");
    unsigned long local_sp = sp;
#endif
    TWlanDrvIfObj *drv = container_of(work, TWlanDrvIfObj, tWork);
#endif

#ifdef STACK_PROFILE
    unsigned long curr1, base1;
    unsigned long curr2, base2;
    static unsigned long maximum_stack = 0;
    #endif

    os_profile (drv, 0, 0);

    #ifdef STACK_PROFILE
    curr1 = check_stack_start(&base1, local_sp + 4, 0);
    #endif

    /* Call the driver main task */
    context_DriverTask (drv->tCommon.hContext);

    os_profile (drv, 1, 0);

    /* First prevent suspend for 1 sec if requested (unless suspending - otherwise
     * this will abort the suspend), and then remove the current prevention */
    if (!drv->bSuspendInProgress)
    {
	os_wake_lock_timeout(drv);
    }

	os_wake_unlock(drv);

    #ifdef STACK_PROFILE
    curr2 = check_stack_stop(&base2, 0);
    if (base2 == base1)
    {
       /* if the current measurement is bigger then the maximum store it and print*/
        if ((curr1 - curr2) > maximum_stack)
        {
            printk("STACK PROFILER GOT THE LOCAL MAXIMMUM!!!! \n");
            printk("current operation stack use=%lu \n",(curr1 - curr2));
            printk("total stack use=%lu \n",8192 - curr2 + base2);
            printk("total stack usage=%lu percent \n",100 * (8192 - curr2 + base2) / 8192);
            maximum_stack = curr1 - curr2;
       }
    }
    #endif

    os_profile (drv, 1, 0);
}


/** 
 * \fn     wlanDrvIf_LoadFiles
 * \brief  Load init files from loader
 * 
 * This function is called from the loader context right after the driver
 *     is created (in IDLE state).
 * It copies the following files to the driver's memory:
 *     - Ini-File - The driver's default parameters values
 *     - NVS-File - The NVS data for FW usage
 *     - FW-Image - The FW program image
 *
 * \note   
 * \param  drv - The driver object handle
 * \return void
 * \sa     wlanDrvIf_GetFile
 */ 
int wlanDrvIf_LoadFiles (TWlanDrvIfObj *drv, TLoaderFilesData *pInitFiles)
{
	if (!pInitFiles)
    {
        ti_dprintf (TIWLAN_LOG_ERROR, "No Init Files!\n");
        return -EINVAL;
    }

    if (drv->tCommon.eDriverState != DRV_STATE_IDLE)
    {
        ti_dprintf (TIWLAN_LOG_ERROR, "Trying to load files not in IDLE state!\n");
        return -EINVAL;
    }

    if (pInitFiles->uIniFileLength) 
    {
        drv->tCommon.tIniFile.uSize = pInitFiles->uIniFileLength;
        drv->tCommon.tIniFile.pImage = kmalloc (pInitFiles->uIniFileLength, GFP_KERNEL);
        #ifdef TI_MEM_ALLOC_TRACE        
          os_printf ("MTT:%s:%d ::kmalloc(%lu, %x) : %lu\n", __FUNCTION__, __LINE__, pInitFiles->uIniFileLength, GFP_KERNEL, pInitFiles->uIniFileLength);
        #endif
        if (!drv->tCommon.tIniFile.pImage)
        {
            ti_dprintf (TIWLAN_LOG_ERROR, "Cannot allocate buffer for Ini-File!\n");
            return -ENOMEM;
        }
        os_memoryCopyFromUser (drv, drv->tCommon.tIniFile.pImage,
                &pInitFiles->data[pInitFiles->uNvsFileLength + pInitFiles->uFwFileLength],
                drv->tCommon.tIniFile.uSize);
    }

    if (pInitFiles->uNvsFileLength)
    {
        drv->tCommon.tNvsImage.uSize = pInitFiles->uNvsFileLength;
        drv->tCommon.tNvsImage.pImage = kmalloc (drv->tCommon.tNvsImage.uSize, GFP_KERNEL);
        #ifdef TI_MEM_ALLOC_TRACE        
          os_printf ("MTT:%s:%d ::kmalloc(%lu, %x) : %lu\n", 
              __FUNCTION__, __LINE__, drv->tCommon.tNvsImage.uSize, GFP_KERNEL, drv->tCommon.tNvsImage.uSize);
        #endif
        if (!drv->tCommon.tNvsImage.pImage)
        {
            ti_dprintf (TIWLAN_LOG_ERROR, "Cannot allocate buffer for NVS image\n");
            return -ENOMEM;
        }
        os_memoryCopyFromUser (drv, drv->tCommon.tNvsImage.pImage, &pInitFiles->data[0], drv->tCommon.tNvsImage.uSize );
    }
    
    drv->tCommon.tFwImage.uSize = pInitFiles->uFwFileLength;
    if (!drv->tCommon.tFwImage.uSize)
    {
        ti_dprintf (TIWLAN_LOG_ERROR, "No firmware image\n");
        return -EINVAL;
    }
    drv->tCommon.tFwImage.pImage = os_memoryAlloc (drv, drv->tCommon.tFwImage.uSize);
    #ifdef TI_MEM_ALLOC_TRACE        
      os_printf ("MTT:%s:%d ::kmalloc(%lu, %x) : %lu\n", 
          __FUNCTION__, __LINE__, drv->tCommon.tFwImage.uSize, GFP_KERNEL, drv->tCommon.tFwImage.uSize);
    #endif
    if (!drv->tCommon.tFwImage.pImage)
    {
        ti_dprintf(TIWLAN_LOG_ERROR, "Cannot allocate buffer for firmware image\n");
        return -ENOMEM;
    }
    os_memoryCopyFromUser (drv, drv->tCommon.tFwImage.pImage,
            &pInitFiles->data[pInitFiles->uNvsFileLength],
            drv->tCommon.tFwImage.uSize);

    ti_dprintf(TIWLAN_LOG_OTHER, "--------- Eeeprom=%p(%lu), Firmware=%p(%lu), IniFile=%p(%lu)\n", 
        drv->tCommon.tNvsImage.pImage, drv->tCommon.tNvsImage.uSize, 
        drv->tCommon.tFwImage.pImage,  drv->tCommon.tFwImage.uSize,
        drv->tCommon.tIniFile.pImage,  drv->tCommon.tIniFile.uSize);


    /* Move the driver to FullyOn power-state */
    wlanDrvIf_KExecPrivCmd(drv, (ti_private_cmd_t){
	.cmd = PWR_STATE_PWR_ON_PARAM,
	.in_buffer = NULL,
	.out_buffer = NULL});
    
    return 0;
}


/** 
 * \fn     wlanDrvIf_GetFile
 * \brief  Provides access to a requested init file
 * 
 * Provide the requested file information and call the requester callback.
 * Note that in Linux the files were previously loaded to driver memory 
 *     by the loader (see wlanDrvIf_LoadFiles).
 *
 * \note   
 * \param  hOs       - The driver object handle
 * \param  pFileInfo - The requested file's properties
 * \return TI_OK
 * \sa     wlanDrvIf_LoadFiles
 */ 
int wlanDrvIf_GetFile (TI_HANDLE hOs, TFileInfo *pFileInfo)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hOs;

	if (drv == NULL || pFileInfo == NULL) {
		ti_dprintf(TIWLAN_LOG_ERROR, "wlanDrv_GetFile: ERROR: Null File Handler, Exiting");
		return TI_NOK;
	}

    /* Future option for getting the FW image part by part */ 
    pFileInfo->hOsFileDesc = NULL;

    /* Fill the file's location and size in the file's info structure */
    switch (pFileInfo->eFileType) 
    {
    case FILE_TYPE_INI: 
        pFileInfo->pBuffer = (TI_UINT8 *)drv->tCommon.tIniFile.pImage; 
        pFileInfo->uLength = drv->tCommon.tIniFile.uSize; 
        break;
    case FILE_TYPE_NVS:     
        pFileInfo->pBuffer = (TI_UINT8 *)drv->tCommon.tNvsImage.pImage; 
        pFileInfo->uLength = drv->tCommon.tNvsImage.uSize; 
        break;
    case FILE_TYPE_FW:
        if (drv->tCommon.tFwImage.pImage == NULL)
        {	
            ti_dprintf(TIWLAN_LOG_ERROR, "wlanDrv_GetFile: ERROR: no Firmware image, exiting\n");
            return TI_NOK;
        }
		pFileInfo->pBuffer = (TI_UINT8 *)drv->tCommon.tFwImage.pImage; 
		pFileInfo->bLast		= TI_FALSE;
		pFileInfo->uLength	= 0;
		pFileInfo->uOffset 				= 0;
		pFileInfo->uChunkBytesLeft 		= 0;
		pFileInfo->uChunksLeft 			= BYTE_SWAP_LONG( *((TI_UINT32*)(pFileInfo->pBuffer)) );
		/* check uChunksLeft's Validity */
		if (( pFileInfo->uChunksLeft == 0 ) || ( pFileInfo->uChunksLeft > MAX_CHUNKS_IN_FILE ))
		{
			ti_dprintf (TIWLAN_LOG_ERROR, "wlanDrvIf_GetFile() Read Invalid Chunks Left: %d!\n",pFileInfo->uChunksLeft);
			return TI_NOK;
		}
		pFileInfo->pBuffer += DRV_ADDRESS_SIZE;
		/* FALL THROUGH */
	case FILE_TYPE_FW_NEXT:
		/* check dec. validity */
		if ( pFileInfo->uChunkBytesLeft >= pFileInfo->uLength )
		{
			pFileInfo->uChunkBytesLeft 		-= pFileInfo->uLength;
		}
		/* invalid Dec. */
		else
		{
			ti_dprintf (TIWLAN_LOG_ERROR, "wlanDrvIf_GetFile() No. of Bytes Left < File Length\n");
			return TI_NOK;
		}
		pFileInfo->pBuffer 	+= pFileInfo->uLength; 

		/* Finished reading all Previous Chunk */
		if ( pFileInfo->uChunkBytesLeft == 0 )
		{
			/* check dec. validity */
			if ( pFileInfo->uChunksLeft > 0 )
			{
				pFileInfo->uChunksLeft--;
			}
			/* invalid Dec. */
			else
			{
				ti_dprintf (TIWLAN_LOG_ERROR, "No. of Bytes Left = 0 and Chunks Left <= 0\n");
				return TI_NOK;
			}
			/* read Chunk's address */
			pFileInfo->uAddress = BYTE_SWAP_LONG( *((TI_UINT32*)(pFileInfo->pBuffer)) );
			pFileInfo->pBuffer += DRV_ADDRESS_SIZE;
			/* read Portion's length */
			pFileInfo->uChunkBytesLeft = BYTE_SWAP_LONG( *((TI_UINT32*)(pFileInfo->pBuffer)) );
			pFileInfo->pBuffer += DRV_ADDRESS_SIZE;
		}
		/* Reading Chunk is NOT complete */
		else
		{
			pFileInfo->uAddress += pFileInfo->uLength;
		}
		
		if ( pFileInfo->uChunkBytesLeft < OS_SPECIFIC_RAM_ALLOC_LIMIT )
		{
			pFileInfo->uLength = pFileInfo->uChunkBytesLeft;
		}
		else
		{
			pFileInfo->uLength = OS_SPECIFIC_RAM_ALLOC_LIMIT;
		}

		/* If last chunk to download */
		if (( pFileInfo->uChunksLeft == 0 ) && 
			( pFileInfo->uLength == pFileInfo->uChunkBytesLeft ))
		{
			pFileInfo->bLast = TI_TRUE;
		}

        break;
    }

    /* Call the requester callback */
    if (pFileInfo->fCbFunc)
    {
        pFileInfo->fCbFunc (pFileInfo->hCbHndl);
    }

    return TI_OK;
}


/** 
 * \fn     wlanDrvIf_SetMacAddress
 * \brief  Set STA MAC address
 * 
 * Called by DrvMain from init process.
 * Copies STA MAC address to the network interface structure.
 *
 * \note   
 * \param  hOs      - The driver object handle
 * \param  pMacAddr - The STA MAC address
 * \return TI_OK
 * \sa     wlanDrvIf_LoadFiles
 */ 
void wlanDrvIf_SetMacAddress (TI_HANDLE hOs, TI_UINT8 *pMacAddr)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hOs;

    /* Copy STA MAC address to the network interface structure */
    MAC_COPY (drv->netdev->dev_addr, pMacAddr);
}

int wlanDrvIf_Open (struct net_device *dev)
{
    /* Enable network interface queue */
    netif_start_queue (dev);

    return 0;
}

int wlanDrvIf_Release (struct net_device *dev)
{
    /* Disable network interface queue */
    netif_stop_queue (dev);

    return 0;
}

/** 
 * \fn     wlanDrvIf_SetupNetif
 * \brief  Setup driver network interface
 * 
 * Called in driver creation process.
 * Setup driver network interface.
 *
 * \note   
 * \param  drv - The driver object handle
 * \return 0 - OK, else - failure
 * \sa     
 */ 
static int wlanDrvIf_SetupNetif (TWlanDrvIfObj *drv)
{
   struct net_device *dev;
   int res;

   /* Allocate network interface structure for the driver */
   dev = alloc_etherdev (0);
   if (dev == NULL)
   {
      ti_dprintf (TIWLAN_LOG_ERROR, "alloc_etherdev() failed\n");
      return -ENOMEM;
   }

   /* Setup the network interface */
   ether_setup (dev);

   NETDEV_SET_PRIVATE(dev,drv);
   drv->netdev = dev;
   drv->netdev->addr_len = MAC_ADDR_LEN;
   strcpy (dev->name, TIWLAN_DRV_IF_NAME);
   netif_carrier_off (dev);
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 31))
/* the following is required on at least BSP 23.8 and higher.
    Without it, the Open function of the driver will not be called
    when trying to 'ifconfig up' the interface */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
	dev->validate_addr = NULL;
#endif
   dev->open = wlanDrvIf_Open;
   dev->stop = wlanDrvIf_Release;
   dev->hard_start_xmit = wlanDrvIf_XmitDummy;
   dev->get_stats = wlanDrvIf_NetGetStat;
   dev->do_ioctl = NULL;
#else
	dev->netdev_ops = &tiwlan_ops_dummy;
#endif
	dev->tx_queue_len = 100;

   /* Initialize Wireless Extensions interface (WEXT) */
   wlanDrvWext_Init (dev);

   res = register_netdev (dev);
   if (res != 0)
   {
      ti_dprintf (TIWLAN_LOG_ERROR, "register_netdev() failed : %d\n", res);
      kfree (dev);
      return res;
   }

   /* Setup power-management callbacks */
   hPlatform_SetupPm(wlanDrvIf_Suspend, wlanDrvIf_Resume, pDrvStaticHandle);

/*
On the latest Kernel there is no more support for the below macro.
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
   SET_MODULE_OWNER (dev);
#endif
   return 0;
}

/** 
 * \fn     wlanDrvIf_CommandDone
 * \brief  Free current command semaphore.
 * 
 * This routine is called whenever a command has finished executing and Free current command semaphore.
 *
 * \note   
 * \param  hOs           - The driver object handle
 * \param  pSignalObject - handle to complete mechanism per OS
 * \param  CmdResp_p     - respond structure (TCmdRespUnion) for OSE OS only 
 * \return 0 - OK, else - failure
 * \sa     wlanDrvIf_Destroy
 */ 
void wlanDrvIf_CommandDone (TI_HANDLE hOs, void *pSignalObject, TI_UINT8 *CmdResp_p)
{
    /* Free semaphore */
    os_SignalObjectSet (hOs, pSignalObject);
}


/** 
 * \fn     wlanDrvIf_Create
 * \brief  Create the driver instance
 * 
 * Allocate driver object.
 * Initialize driver OS resources (IRQ, workqueue, events socket)
 * Setup driver network interface.
 * Create and link all driver modules.
 *
 * \note   
 * \param  void
 * \return 0 - OK, else - failure
 * \sa     wlanDrvIf_Destroy
 */ 
static int wlanDrvIf_Create (void)
{
    TWlanDrvIfObj *drv;
    int rc;

    /* Allocate driver's structure */
    drv = kmalloc (sizeof(TWlanDrvIfObj), GFP_KERNEL);
    if (!drv)
    {
        return -ENOMEM;
    }

    drv->bSuspendInProgress = TI_FALSE;

#ifdef TI_DBG
	tb_init(TB_OPTION_NONE);
#endif
    pDrvStaticHandle = drv;  /* save for module destroy */
    #ifdef TI_MEM_ALLOC_TRACE
      os_printf ("MTT:%s:%d ::kmalloc(%lu, %x) : %lu\n", __FUNCTION__, __LINE__, sizeof(TWlanDrvIfObj), GFP_KERNEL, sizeof(TWlanDrvIfObj));
    #endif
    memset (drv, 0, sizeof(TWlanDrvIfObj));


    drv->tCommon.eDriverState = DRV_STATE_IDLE;

	drv->tiwlan_wq = create_freezeable_workqueue(DRIVERWQ_NAME);
	if (!drv->tiwlan_wq)
    {
		ti_dprintf (TIWLAN_LOG_ERROR, "wlanDrvIf_Create(): Failed to create workQ!\n");
		rc = -EINVAL;
		goto drv_create_end_1;
    }

	drv->wl_packet = 0;
	drv->wl_count = 0;
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init (&drv->wl_wifi, WAKE_LOCK_SUSPEND, "wifi_wake");
	wake_lock_init (&drv->wl_rxwake, WAKE_LOCK_SUSPEND, "wifi_rx_wake");
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	INIT_WORK(&drv->tWork, wlanDrvIf_DriverTask, (void *)drv);
#else
	INIT_WORK(&drv->tWork, wlanDrvIf_DriverTask);
#endif
    spin_lock_init (&drv->lock);

    /* Setup driver network interface. */
    rc = wlanDrvIf_SetupNetif (drv);
    if (rc)
    {
		goto drv_create_end_2;
    }


    /* Create the events socket interface */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
	drv->wl_sock = netlink_kernel_create( NETLINK_USERSOCK, 0, NULL, THIS_MODULE );
#else
	drv->wl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, 0, NULL, NULL, THIS_MODULE );
#endif
    if (drv->wl_sock == NULL)
    {
        ti_dprintf (TIWLAN_LOG_ERROR, "netlink_kernel_create() failed !\n");
		rc = -EINVAL;
		goto drv_create_end_3;
    }

    /* Create all driver modules and link their handles */
	rc = drvMain_Create (drv,
                    &drv->tCommon.hDrvMain, 
                    &drv->tCommon.hCmdHndlr, 
                    &drv->tCommon.hContext, 
                    &drv->tCommon.hTxDataQ,
                    &drv->tCommon.hTxMgmtQ,
					&drv->tCommon.hTxCtrl,
                    &drv->tCommon.hTWD,
					&drv->tCommon.hEvHandler,
                    &drv->tCommon.hCmdDispatch,
                    &drv->tCommon.hReport,
                    &drv->tCommon.hPwrState);
	if (rc != TI_OK) {
		ti_dprintf (TIWLAN_LOG_ERROR, "%s: Failed to dvrMain_Create!\n", __func__);
		rc = -EINVAL;
		goto drv_create_end_4;
	}
    /* 
     *  Initialize interrupts (or polling mode for debug):
     */
#ifdef PRIODIC_INTERRUPT
    /* Debug mode: Polling (the timer is started by HwInit process) */
    drv->hPollTimer = os_timerCreate ((TI_HANDLE)drv, wlanDrvIf_PollIrqHandler, (TI_HANDLE)drv);
#else 
    /* Normal mode: Interrupts (the default mode) */
    rc = hPlatform_initInterrupt (drv, (void*)wlanDrvIf_HandleInterrupt);
    if (rc)
    {
        ti_dprintf (TIWLAN_LOG_ERROR, "wlanDrvIf_Create(): Failed to register interrupt handler!\n");
		goto drv_create_end_5;
    }
#endif  /* PRIODIC_INTERRUPT */

   return 0;
drv_create_end_5:
	/* Destroy all driver modules */
	if (drv->tCommon.hDrvMain)
    {
		drvMain_Destroy (drv->tCommon.hDrvMain);
	}

drv_create_end_4:
	if (drv->wl_sock)
    {
		sock_release (drv->wl_sock->sk_socket);
	}

drv_create_end_3:
	/* Release the driver network interface */
	if (drv->netdev)
    {
		unregister_netdev (drv->netdev);
		free_netdev (drv->netdev);
	}

drv_create_end_2:
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy (&drv->wl_wifi);
	wake_lock_destroy (&drv->wl_rxwake);
#endif
	if (drv->tiwlan_wq)
    {
		destroy_workqueue(drv->tiwlan_wq);
    }

drv_create_end_1:
	kfree (drv);
	printk ("%s: Fail, rc = %d\n", __func__, rc);
	return rc;
}


/** 
 * \fn     wlanDrvIf_Destroy
 * \brief  Destroy the driver instance
 * 
 * Destroy all driver modules.
 * Release driver OS resources (IRQ, workqueue, events socket)
 * Release driver network interface.
 * Free init files memory.
 * Free driver object.
 *
 * \note   
 * \param  drv - The driver object handle
 * \return void
 * \sa     wlanDrvIf_Create
 */ 
static void wlanDrvIf_Destroy (TWlanDrvIfObj *drv)
{
    if(!drv)
    {
        return;
    }

    /* Move the driver to Off power-state */
    wlanDrvIf_KExecPrivCmd(drv, (ti_private_cmd_t){
		.cmd = PWR_STATE_PWR_OFF_PARAM,
		.in_buffer = NULL,
		.out_buffer = NULL});
	if (drv->tiwlan_wq)
    {
		cancel_work_sync (&drv->tWork);
		flush_workqueue(drv->tiwlan_wq);
	}

    /* Release the driver network interface and stop driver */
    if (drv->netdev)
    {
        netif_stop_queue  (drv->netdev);
        if (drv->tCommon.eDriverState != DRV_STATE_IDLE)
        {
        	wlanDrvIf_Release(drv->netdev);
        }
        unregister_netdev (drv->netdev);
        free_netdev (drv->netdev);
    }

	/* Destroy all driver modules */
    if (drv->tCommon.hDrvMain)
    {
        drvMain_Destroy (drv->tCommon.hDrvMain);
    }

    /* close the ipc_kernel socket*/
    if (drv && drv->wl_sock) 
    {
        sock_release (drv->wl_sock->sk_socket);
    }

    /* Release the driver interrupt (or polling timer) */
#ifdef PRIODIC_INTERRUPT
    os_timerDestroy (drv, drv->hPollTimer);
#else
    if (drv->irq)
    {
        hPlatform_freeInterrupt(drv);
    }
#endif
	if (drv->tiwlan_wq)
    {
		destroy_workqueue(drv->tiwlan_wq);
    }

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy (&drv->wl_wifi);
	wake_lock_destroy (&drv->wl_rxwake);
#endif

    /*
     *  Free init files memory
     */
    if (drv->tCommon.tFwImage.pImage)
    {
        os_memoryFree (drv, drv->tCommon.tFwImage.pImage, drv->tCommon.tFwImage.uSize);
        #ifdef TI_MEM_ALLOC_TRACE        
          os_printf ("MTT:%s:%d ::kfree(0x%p) : %d\n", 
              __FUNCTION__, __LINE__, drv->tCommon.tFwImage.uSize, -drv->tCommon.tFwImage.uSize);
        #endif
    }
    if (drv->tCommon.tNvsImage.pImage)
    {
        kfree (drv->tCommon.tNvsImage.pImage);
        #ifdef TI_MEM_ALLOC_TRACE        
          os_printf ("MTT:%s:%d ::kfree(0x%p) : %d\n", 
              __FUNCTION__, __LINE__, drv->tCommon.tNvsImage.uSize, -drv->tCommon.tNvsImage.uSize);
        #endif
    }
    if (drv->tCommon.tIniFile.pImage)
    {
        kfree (drv->tCommon.tIniFile.pImage);
        #ifdef TI_MEM_ALLOC_TRACE        
          os_printf ("MTT:%s:%d ::kfree(0x%p) : %d\n", 
              __FUNCTION__, __LINE__, drv->tCommon.tIniFile.uSize, -drv->tCommon.tIniFile.uSize);
        #endif
    }

    /* Free the driver object */
#ifdef TI_DBG
	tb_destroy();
#endif
    kfree (drv);
}


/** 
 * \fn     wlanDrvIf_ModuleInit  &  wlanDrvIf_ModuleExit
 * \brief  Linux Init/Exit functions
 * 
 * The driver Linux Init/Exit functions (insmod/rmmod) 
 *
 * \note   
 * \param  void
 * \return Init: 0 - OK, else - failure.   Exit: void
 * \sa     wlanDrvIf_Create, wlanDrvIf_Destroy
 */ 

static int __init wlanDrvIf_ModuleInit (void)
{
    printk(KERN_INFO "TIWLAN: driver init\n");
	sdioDrv_init();
    return wlanDrvIf_Create ();
}

static void __exit wlanDrvIf_ModuleExit (void)
{
    wlanDrvIf_Destroy (pDrvStaticHandle);
	sdioDrv_exit();
    printk (KERN_INFO "TI WLAN: driver unloaded\n");
}


/**
 * \fn     wlanDrvIf_StopTx
 * \brief  block Tx thread until wlanDrvIf_ResumeTx called .
 *
 * This routine is called whenever we need to stop the network stack to send us pakets since one of our Q's is full.
 *
 * \note
 * \param  hOs           - The driver object handle
* \return
 * \sa     wlanDrvIf_StopTx
 */
void wlanDrvIf_StopTx (TI_HANDLE hOs)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hOs;

    netif_stop_queue (drv->netdev);
}

/**
 * \fn     wlanDrvIf_ResumeTx
 * \brief  Resume Tx thread .
 *
 * This routine is called whenever we need to resume the network stack to send us pakets since our Q's are empty.
 *
 * \note
 * \param  hOs           - The driver object handle
 * \return
 * \sa     wlanDrvIf_ResumeTx
 */
void wlanDrvIf_ResumeTx (TI_HANDLE hOs)
{
    TWlanDrvIfObj *drv = (TWlanDrvIfObj *)hOs;

    netif_wake_queue (drv->netdev);
}

/*
 * \brief	suspend the driver
 *
 * 			This callback is invoked by the kernel when suspending the system. In this stage all the
 * 			applications are already frozen so no commands are expected; and the SDIO is still active.
 *
 * \context	called in EXTERNAL context, in KERNEL space
 */
static int wlanDrvIf_Suspend(TI_HANDLE hWlanDrvIf)
{
	TWlanDrvIfObj *pWlanDrvIf = (TWlanDrvIfObj*) hWlanDrvIf;
	TPwrState     *pPwrState  = (TPwrState*) pWlanDrvIf->tCommon.hPwrState;
	TI_STATUS      rc         = TI_OK;

	pWlanDrvIf->bSuspendInProgress = TI_TRUE;

	/*
	 * setup the suspend/resume actions.
	 *
	 * Low On: the station remains connected to the AP, and enters
	 *         Long Doze PS mode - wakes up every N DTIMs (N configured
	 *         in INI). Scan and measurements are stopped.
	 *         HW is in ELP.
	 *
	 * Sleep:  if connected, the station is disconnected from AP. Scan
	 *         and measurements are stopped.
	 *         HW is in ELP.
	 *
	 * Off:    disconnected, HW is off.
	 *
	 * On:     driver is returned to either Standby mode (if it was
	 *         disconnected) or FullOn mode (if was in Low On).
	 */
	if (pWlanDrvIf->tCommon.eDriverState == DRV_STATE_RUNNING)
	{
		switch (pPwrState->tConfig.eSuspendType)
		{
		case PWRSTATE_SUSPEND_NONE:
			pWlanDrvIf->eSuspendCmd = LAST_CMD; /* do nothing */
			pWlanDrvIf->eResumeCmd  = LAST_CMD; /* do nothing */
			break;
		case PWRSTATE_SUSPEND_LOWON:
			pWlanDrvIf->eSuspendCmd = PWR_STATE_DOZE_PARAM;
			pWlanDrvIf->eResumeCmd  = PWR_STATE_PWR_ON_PARAM;
			break;
		case PWRSTATE_SUSPEND_SLEEP:
			pWlanDrvIf->eSuspendCmd = PWR_STATE_SLEEP_PARAM;
			pWlanDrvIf->eResumeCmd  = PWR_STATE_PWR_ON_PARAM;
			break;
		case PWRSTATE_SUSPEND_OFF:
			pWlanDrvIf->eSuspendCmd = PWR_STATE_PWR_OFF_PARAM;
			pWlanDrvIf->eResumeCmd  = PWR_STATE_PWR_ON_PARAM;
			break;
		default:
			pWlanDrvIf->eSuspendCmd = LAST_CMD;
			pWlanDrvIf->eResumeCmd  = LAST_CMD;
			break;
		}
	}
	else
	{
		pWlanDrvIf->eSuspendCmd = pWlanDrvIf->eResumeCmd = LAST_CMD;
	}

	/*
	 * issue the suspend command
	 */
	if (pWlanDrvIf->eSuspendCmd != LAST_CMD)
	{
	    rc = wlanDrvIf_KExecPrivCmd(pWlanDrvIf, (ti_private_cmd_t){
		.cmd = pWlanDrvIf->eSuspendCmd,
		.in_buffer = NULL,
		.out_buffer = NULL});
	}

	/*
	 * if failed to suspend, prevent the kernel from suspending again
	 * for 1 second, to allow the supplicant to react to any events
	 * from the driver
	 */
	if (rc != TI_OK)
	{
		pWlanDrvIf->bSuspendInProgress = TI_FALSE;

		os_wake_lock_timeout_enable(pWlanDrvIf);
		os_wake_lock_timeout(pWlanDrvIf);
	}

	/* on error, kernel will retry to suspend until it succeeds */
	return rc == TI_OK ? 0 : -1;
}

/*
 * \brief	resume the driver
 *
 * \context	called in EXTERNAL context, in KERNEL space
 */
static int wlanDrvIf_Resume(TI_HANDLE hWlanDrvIf)
{
	TWlanDrvIfObj *pWlanDrvIf = (TWlanDrvIfObj*) hWlanDrvIf;
	TI_STATUS      rc         = TI_OK;

	/* issue resume command */
	if (pWlanDrvIf->eResumeCmd != LAST_CMD)
	{
		rc = wlanDrvIf_KExecPrivCmd(pWlanDrvIf, (ti_private_cmd_t){
			.cmd = pWlanDrvIf->eResumeCmd,
			.in_buffer = NULL,
			.out_buffer = NULL});
	}

	pWlanDrvIf->bSuspendInProgress = TI_FALSE;

	return rc == TI_OK ? 0 : -1;
}

/**
 * \fn		wlanDrvIf_IsIoctlEnabled
 *
 * \brief	indicates whether the driver currently accepts the specified IOCTL
 *
 * \param	uIoctl	the IOCTL to check
 * \return	TI_TRUE if the driver can accept uIoctl now, TI_FALSE otherwise
 */
TI_BOOL wlanDrvIf_IsIoctlEnabled(TI_HANDLE hWlanDrvIf, TI_UINT32 uIoctl)
{
	TWlanDrvIfObj *pWlanDrvIf = (TWlanDrvIfObj*) hWlanDrvIf;
	TI_BOOL        bEnabled = TI_FALSE;

	switch (pWlanDrvIf->tCommon.eDriverState)
	{
	case DRV_STATE_RUNNING:
		bEnabled = TI_TRUE;
		break;
	case DRV_STATE_FAILED:
	case DRV_STATE_STOPING:
	case DRV_STATE_STOPPED:
	case DRV_STATE_IDLE:
		/* to allow DRIVER_INIT_PARAM, DRIVER_STATUS and DRIVER_START command. see wlanDrvIf_IsCmdEnabled() */
		bEnabled = (uIoctl==SIOCIWFIRSTPRIV);
		break;
	}

	if (!bEnabled)
	{
		printk(KERN_WARNING "TIWLAN: ioctl 0x%x is disabled in state %d\n", uIoctl, pWlanDrvIf->tCommon.eDriverState);
	}

	return bEnabled;
}

/**
 * \fn		wlanDrvIf_IsCmdEnabled
 *
 * \brief	Indicates whether the driver currently accepts the specified private-command.
 * 			Used to filter commands from user applications
 *
 * \param	uCmd	the private-command to check
 * \return	TI_TRUE if the driver can accept eCmd now, TI_FALSE otherwise
 */
int wlanDrvIf_IsCmdEnabled(TI_HANDLE hWlanDrvIf, TI_UINT32 uCmd)
{
	TWlanDrvIfObj *pWlanDrvIf = (TWlanDrvIfObj*) hWlanDrvIf;
	int cmdStatus = CMD_DONOTHING;

	/* PwrState commands are never allowed from user application */
	switch (uCmd)
	{
	case PWR_STATE_PWR_ON_PARAM:
	case PWR_STATE_PWR_OFF_PARAM:
	case PWR_STATE_SLEEP_PARAM:
	case PWR_STATE_DOZE_PARAM:
		printk(KERN_WARNING "TIWLAN: cmd 0x%x is not allowed\n", uCmd);
		return CMD_DISABLED;
	}

	/* other commands are enabled/disabled depending on the driver-state */
	switch (pWlanDrvIf->tCommon.eDriverState)
	{
	case DRV_STATE_RUNNING:
		if(uCmd == DRIVER_START_PARAM)
			cmdStatus = CMD_DONOTHING;
		else
			cmdStatus = CMD_ENABLED;
		break;
	case DRV_STATE_FAILED:
	case DRV_STATE_STOPING:
		if(uCmd == DRIVER_STATUS_PARAM)
			cmdStatus = CMD_ENABLED;
		else
			cmdStatus = CMD_DISABLED;
		break;
	case DRV_STATE_STOPPED:
		if((uCmd == DRIVER_START_PARAM)||(uCmd == DRIVER_STATUS_PARAM))
			cmdStatus = CMD_ENABLED;
		else 
			cmdStatus = CMD_DISABLED;
		break;
	case DRV_STATE_IDLE:
		if((uCmd == DRIVER_INIT_PARAM)||(uCmd == DRIVER_STATUS_PARAM))
			cmdStatus = CMD_ENABLED;
		else 
			cmdStatus = CMD_DISABLED;
		break;
	}

	if (cmdStatus == CMD_DISABLED)
	{
		printk(KERN_WARNING "TIWLAN: cmd 0x%x is disabled in state %d\n", uCmd, pWlanDrvIf->tCommon.eDriverState);
	}

	return cmdStatus;
}

module_init (wlanDrvIf_ModuleInit);
module_exit (wlanDrvIf_ModuleExit);
