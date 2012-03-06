/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and  
 * limitations under the License.
 */

/*******************************************************************************\
*
*   FILE NAME:      mcp_hal_pm.c
*
*   DESCRIPTION:    This file contains the implementation of the chip power 
*                   management for Android on Zoom2 platform
*
*   AUTHOR:         Chen Ganir
*
\*******************************************************************************/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>

#include "mcp_defs.h"
#include "mcp_hal_pm.h"
#include "mcp_hal_log.h"

/********************************************************************************
 *
 * Internal function prototypes
 *
 *******************************************************************************/
#ifndef EBTIPS_RELEASE
/* Utilities */
static const char *Tran_Id_AsStr(McpHalTranId transport);
static const char *Chip_Id_AsStr(McpHalChipId chip);
#endif /* EBTIPS_RELEASE */

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/


#define MCP_HAL_PM_UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM))

#ifdef EBTIPS_RELEASE
#define MCP_HAL_PM_LOG_DEBUG(msg)
#define MCP_HAL_PM_LOG_FUNCTION(msg)
#else
#define MCP_HAL_PM_LOG_DEBUG(msg)       MCP_HAL_LOG_DEBUG(__FILE__, __LINE__, MCP_HAL_LOG_MODULE_TYPE_PM, msg)
#define MCP_HAL_PM_LOG_FUNCTION(msg)    MCP_HAL_LOG_FUNCTION(__FILE__, __LINE__, MCP_HAL_LOG_MODULE_TYPE_PM, msg)
#endif
#define MCP_HAL_PM_LOG_INFO(msg)        MCP_HAL_LOG_INFO(__FILE__, __LINE__, MCP_HAL_LOG_MODULE_TYPE_PM, msg)
#define MCP_HAL_PM_LOG_ERROR(msg)       MCP_HAL_LOG_ERROR(__FILE__, __LINE__, MCP_HAL_LOG_MODULE_TYPE_PM, msg)
#define MCP_HAL_PM_LOG_FATAL(msg)       MCP_HAL_LOG_FATAL(__FILE__, __LINE__, MCP_HAL_LOG_MODULE_TYPE_PM, msg)

#define UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM));


#define WAKE_LOCK_RELEASED      (0)
#define WAKE_LOCK_ACQUIRED      (1)
#define WAKE_LOCK_FILENAME      ("/sys/power/wake_lock")
#define WAKE_UNLOCK_FILENAME    ("/sys/power/wake_unlock")
#define WAKE_LOCK_ID_LEN        (9)
static int g_wakelock_fd = -1;
static int g_wakeunlock_fd = -1;
static char wake_lock_id[] = "BTIPSD_PM";
static int g_wakelock_state = WAKE_LOCK_RELEASED;

/********************************************************************************
 *
 * Functions
 *
 *******************************************************************************/


McpHalPmStatus MCP_HAL_PM_AcquireWakeLock()
{
    if (g_wakelock_state == WAKE_LOCK_RELEASED)
    {
        if (g_wakelock_fd == -1)
        {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_AcquireWakeLock : Failed to acquire lock : wake_lock file is not opened"));
            return MCP_HAL_PM_STATUS_FAILED;
        }

        if (WAKE_LOCK_ID_LEN != write(g_wakelock_fd, &wake_lock_id, WAKE_LOCK_ID_LEN)) 
        {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_AcquireWakeLock: write failed: %s\n", strerror(errno)));
            return MCP_HAL_PM_STATUS_FAILED;
        }
        g_wakelock_state = WAKE_LOCK_ACQUIRED;
    }
    
    MCP_HAL_PM_LOG_INFO(("MCP_HAL_PM_AcquireWakeLock : Lock=%d",g_wakelock_state));

    return MCP_HAL_PM_STATUS_SUCCESS;
}

McpHalPmStatus MCP_HAL_PM_ReleaseWakeLock()
{
    if (g_wakelock_state == WAKE_LOCK_ACQUIRED)
    {
        if (g_wakeunlock_fd == -1)
        {
            MCP_HAL_PM_LOG_ERROR(("Failed to acquire lock : wake_unlock file is not opened"));
            return MCP_HAL_PM_STATUS_FAILED;
        }

        if (WAKE_LOCK_ID_LEN != write(g_wakeunlock_fd, &wake_lock_id, WAKE_LOCK_ID_LEN)) {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_ReleaseWakeLock: write failed: %s\n", strerror(errno)));
            return MCP_HAL_PM_STATUS_FAILED;
        }
        g_wakelock_state = WAKE_LOCK_RELEASED;
    }
    
    MCP_HAL_PM_LOG_INFO(("MCP_HAL_PM_ReleaseWakeLock : Lock=%d",g_wakelock_state));
    
    return MCP_HAL_PM_STATUS_SUCCESS;
}


/*---------------------------------------------------------------------------
 *            CCM_HAL_PM_Init()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Init chip pm module.
 *
 *  Returns:
 *            MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 */
McpHalPmStatus MCP_HAL_PM_Init(void)
{
    MCP_HAL_PM_LOG_DEBUG(("MCP_HAL_PM_Init is called."));

    if (g_wakelock_fd == -1)
    {
        g_wakelock_fd = open(WAKE_LOCK_FILENAME, O_WRONLY);
        if (g_wakelock_fd == -1) {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Init: open %s failed: %s\n", WAKE_LOCK_FILENAME, strerror(errno)));
            return MCP_HAL_PM_STATUS_FAILED;
        }
    }
    else
    {
        MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Init : Uninitialized wake_lock file descriptor"));
        return MCP_HAL_PM_STATUS_FAILED;
    }

    if (g_wakeunlock_fd == -1)
    {
        g_wakeunlock_fd = open(WAKE_UNLOCK_FILENAME, O_WRONLY);
        if (g_wakeunlock_fd == -1) {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Init: open %s failed: %s\n", WAKE_UNLOCK_FILENAME, strerror(errno)));
            return MCP_HAL_PM_STATUS_FAILED;
        }
    }
    else
    {
        MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Init : Uninitialized wake_lock file descriptor"));
        return MCP_HAL_PM_STATUS_FAILED;
    }

    g_wakelock_state = WAKE_LOCK_RELEASED;
    
    return MCP_HAL_PM_STATUS_SUCCESS;
}
/*---------------------------------------------------------------------------
 *            HAL_PM_Deinit()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Deinit chip pm module.
 * 
 * Returns:
 *            MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 */
McpHalPmStatus MCP_HAL_PM_Deinit(void)
{
    MCP_HAL_PM_LOG_DEBUG(("MCP_HAL_PM_Deinit is called."));

    if (g_wakelock_fd != -1)
    {
        if (-1 == close(g_wakelock_fd))
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Deinit: close wake_lock file failed: %s\n", strerror(errno)));

        g_wakelock_fd = -1;
    }

    
    if (g_wakeunlock_fd != -1)
    {
        if (WAKE_LOCK_ID_LEN != write(g_wakeunlock_fd, &wake_lock_id, WAKE_LOCK_ID_LEN)) {
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Deinit: write failed: %s\n", strerror(errno)));
        }

        g_wakelock_state = WAKE_LOCK_RELEASED;

        if (-1 == close(g_wakeunlock_fd))
            MCP_HAL_PM_LOG_ERROR(("MCP_HAL_PM_Deinit: close wake_unlock file failed: %s\n", strerror(errno)));

        g_wakeunlock_fd = -1;
    }


    return MCP_HAL_PM_STATUS_SUCCESS;
}


/*---------------------------------------------------------------------------
 *            CCM_HAL_PM_RegisterTransport()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Register client on specific transport on specific chip.
 *
 */
McpHalPmStatus MCP_HAL_PM_RegisterTransport(McpHalChipId chip_id,McpHalTranId tran_id,McpHalPmHandler* handler)
{
    UNUSED_PARAMETER(chip_id);
    UNUSED_PARAMETER(tran_id);
    MCP_HAL_PM_UNUSED_PARAMETER(handler);
    MCP_HAL_PM_LOG_DEBUG(("%s on %s is register succesfully.",Tran_Id_AsStr(tran_id),Chip_Id_AsStr(chip_id)));
    return MCP_HAL_PM_STATUS_SUCCESS;
}


/*---------------------------------------------------------------------------
 *            CCM_HAL_PM_Deregister_Transport()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Deregister client on specific transport on specific chip.
 *
 */
McpHalPmStatus MCP_HAL_PM_DeregisterTransport(McpHalChipId chip_id,McpHalTranId tran_id)
{
    UNUSED_PARAMETER(chip_id);
    UNUSED_PARAMETER(tran_id);
    MCP_HAL_PM_LOG_DEBUG(("%s on %s deregister succesfully",Tran_Id_AsStr(tran_id),Chip_Id_AsStr(chip_id)));
    return MCP_HAL_PM_STATUS_SUCCESS;
}


/*---------------------------------------------------------------------------
 *            CCM_HAL_PM_Transport_Wakeup()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Synopsis:  Chip power manager wakes up.
 *
 */

McpHalPmStatus MCP_HAL_PM_TransportWakeup(McpHalChipId chip_id,McpHalTranId tran_id)
{           
    UNUSED_PARAMETER(chip_id);
    UNUSED_PARAMETER(tran_id);
    MCP_HAL_PM_LOG_DEBUG(("%s on %s sleep state is changed to awake.",Tran_Id_AsStr(tran_id),Chip_Id_AsStr(chip_id)));
    
    return MCP_HAL_PM_AcquireWakeLock();
}



/*---------------------------------------------------------------------------
 *            CCM_HAL_PM_Transport_Sleep()
 *---------------------------------------------------------------------------
 *
 * Synopsis:  Chip power manager go to sleep.
 *
 */
McpHalPmStatus MCP_HAL_PM_TransportSleep(McpHalChipId chip_id,McpHalTranId tran_id)
{
    UNUSED_PARAMETER(chip_id);
    UNUSED_PARAMETER(tran_id);
    MCP_HAL_PM_LOG_DEBUG(("%s on %s sleep state is changed to sleep.",Tran_Id_AsStr(tran_id),Chip_Id_AsStr(chip_id)));
    
    return MCP_HAL_PM_ReleaseWakeLock();
}

#ifndef EBTIPS_RELEASE
const char *Tran_Id_AsStr(McpHalTranId transport)
{
    switch (transport)
    {
        case MCP_HAL_TRAN_ID_0:             return "MCP_HAL_TRAN_ID_0";
        case MCP_HAL_TRAN_ID_1:         return "MCP_HAL_TRAN_ID_1";
        case MCP_HAL_TRAN_ID_2:         return "MCP_HAL_TRAN_ID_2";     
        default:                            return "Unknown";
    };
};

const char *Chip_Id_AsStr(McpHalChipId chip)
{
    switch (chip)
    {
        case MCP_HAL_CHIP_ID_0:                    return "MCP_HAL_CHIP_ID_0";
        case MCP_HAL_CHIP_ID_1:              return "MCP_HAL_CHIP_ID_1";
        default:                                 return "Unknown";
    };
};

#endif /* EBTIPS_RELEASE */
