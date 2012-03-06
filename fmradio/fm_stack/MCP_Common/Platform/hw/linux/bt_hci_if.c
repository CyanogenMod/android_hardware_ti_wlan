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


#include "mcp_defs.h"
#include "bt_hci_if.h"
#include "mcp_hal_memory.h"
#include "fmc_defs.h"

#include "fmc_log.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMRXSM);

#define _BT_HCI_IF_MAX_NUM_OF_CLIENTS       ((McpUint)30) /* TODO ronen: understand how many we really need */

typedef struct _tagBtHciIf_ClientData {
    /* 
        Allows obtaining the instance object from the client handle.
        This is required since multiple clients (using multiple handles) exist for
        a single instance object
    */
    BtHciIfObj          *containingIfObj;
    
    BtHciIfClientCb     clientCb;
    void                    *pClientUserData;
    McpU8                   hciCmdParms[255];
}_BtHciIf_ClientData;

struct tagBtHciIfObj {
    McpHalChipId            chipId;
    BtHciIfMngrCb           mngrCb;
    _BtHciIf_ClientData clientsData[_BT_HCI_IF_MAX_NUM_OF_CLIENTS];
};

typedef struct _tagBtHciIf_Data {
    BtHciIfObj  ifObjs[MCP_HAL_MAX_NUM_OF_CHIPS];
} _BtHciIf_Data;

MCP_STATIC _BtHciIf_Data    _btHciIf_Data;

MCP_STATIC void _BT_HCI_IF_HandleRmgrInitCompleteEvent(McpHalChipId chipId);
MCP_STATIC void _BT_HCI_IF_HandleRmgrConfigHciCompleteEvent(McpHalChipId chipId);
MCP_STATIC void _BT_HCI_IF_HandleSendRadioCmdCompleteEvent(McpHalChipId chipId);
MCP_STATIC void _BT_HCI_IF_HandleRmgrDeinitCompleteEvent(McpHalChipId chipId);

MCP_STATIC void _BT_HCI_IF_RmgrCb(McpHalChipId chipId);
MCP_STATIC void _BT_HCI_IF_HciCmdCompleteCb(McpHalChipId chipId);

BtHciIfStatus BT_HCI_IF_StaticInit(void)
{
    McpUint chipIdx;
    McpUint clientIdx;
    
    for (chipIdx = 0; chipIdx < MCP_HAL_MAX_NUM_OF_CHIPS; ++chipIdx)
    {   
        _btHciIf_Data.ifObjs[chipIdx].chipId = MCP_HAL_INVALID_CHIP_ID;
        _btHciIf_Data.ifObjs[chipIdx].mngrCb = NULL;

        for (clientIdx = 0; clientIdx < _BT_HCI_IF_MAX_NUM_OF_CLIENTS; ++clientIdx)
        {
            _btHciIf_Data.ifObjs[chipIdx].clientsData[clientIdx].containingIfObj = NULL;        
            _btHciIf_Data.ifObjs[chipIdx].clientsData[clientIdx].clientCb = NULL;       
            _btHciIf_Data.ifObjs[chipIdx].clientsData[clientIdx].pClientUserData = NULL;
        }
    }

    return BT_HCI_IF_STATUS_SUCCESS;
}

BtHciIfStatus BT_HCI_IF_Create(McpHalChipId chipId, BtHciIfMngrCb mngrCb, BtHciIfObj **btHciIfObj)
{
    _btHciIf_Data.ifObjs[chipId].chipId= chipId;
    _btHciIf_Data.ifObjs[chipId].mngrCb = mngrCb;
    
    *btHciIfObj = &_btHciIf_Data.ifObjs[chipId];

    return BT_HCI_IF_STATUS_SUCCESS;
}

BtHciIfStatus BT_HCI_IF_Destroy(BtHciIfObj **btHciIfObj)
{
    *btHciIfObj = 0;

    return BT_HCI_IF_STATUS_SUCCESS;
}

BtHciIfStatus BT_HCI_IF_MngrChangeCb(BtHciIfObj *btHciIfObj, BtHciIfMngrCb newCb, BtHciIfMngrCb *oldCb)
{
    /* If caller wishes to receive the previous cb, return it in the output parameter */
    if (oldCb != NULL)
    {
        *oldCb = btHciIfObj->mngrCb;
    }

    /* Set the new cb */
    btHciIfObj->mngrCb = newCb;

    return BT_HCI_IF_STATUS_SUCCESS;
}

BtHciIfStatus BT_HCI_IF_MngrTransportOn(BtHciIfObj *btHciIfObj)
{
    MCP_FUNC_START("BT_HCI_IF_MngrTransportOn");

#if 0
    status = RMGR_RadioInit(btHciIfObj->chipId, _BT_HCI_IF_RmgrCb);
    MCP_VERIFY_FATAL((status == BT_STATUS_PENDING), BT_HCI_IF_STATUS_INTERNAL_ERROR,
                        ("BT_HCI_IF_MngrTransportOn: RMGR_RadioInit Failed (%s)", pBT_Status(status)));
#endif	

	_BT_HCI_IF_HandleRmgrInitCompleteEvent(btHciIfObj->chipId);

    MCP_FUNC_END();

    return BT_HCI_IF_STATUS_PENDING;
}

BtHciIfStatus BT_HCI_IF_MngrConfigHci(BtHciIfObj *btHciIfObj)
{
    MCP_FUNC_START("BT_HCI_IF_MngrTransportOn");

	/* shouldn't be used yet */
    FMC_ASSERT(0);
    
    MCP_UNUSED_PARAMETER(btHciIfObj);

#if 0
    status = RMGR_ConfigHci();
    MCP_VERIFY_FATAL((status == BT_STATUS_SUCCESS), BT_HCI_IF_STATUS_INTERNAL_ERROR,
                        ("BT_HCI_IF_MngrTransportOn: RMGR_RadioInit Failed (%s)", pBT_Status(status)));
#endif	

	_BT_HCI_IF_HandleRmgrConfigHciCompleteEvent(btHciIfObj->chipId);

    MCP_FUNC_END();

	return BT_HCI_IF_STATUS_PENDING;
}

BtHciIfStatus BT_HCI_IF_MngrSendRadioCommand(   BtHciIfObj      *btHciIfObj,    
                                                            BtHciIfHciOpcode    hciOpcode, 
                                                            McpU8           *hciCmdParms, 
                                                            McpU8           hciCmdParmsLen)
{
    BtHciIfStatus           status;
    _BtHciIf_ClientData *clientData;
  
    MCP_FUNC_START("BT_HCI_IF_MngrSendRadioCommand");

 	/* shouldn't be used yet */
    FMC_ASSERT(0);
	
    clientData = &btHciIfObj->clientsData[btHciIfObj->chipId];

    /* 
        Before HCI is configured, we are sending HCI commands via the low-level HCI_SendRadioCommand() call.
        This is an internal call that was previously issued from within radiomod.c
    */
#if 0
    btStatus = HCI_SendRadioCommand((U16)hciOpcode, hciCmdParmsLen, &(clientData->hciCommand));
    MCP_VERIFY_FATAL((btStatus == BT_STATUS_PENDING), BT_HCI_IF_STATUS_INTERNAL_ERROR, 
                        ("BT_HCI_IF_MngrSendRadioCommand: HCI_SendRadioCommand Failed (%s)", pBT_Status(btStatus)));
#endif
    
    status = BT_HCI_IF_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;
}

BtHciIfStatus   BT_HCI_IF_MngrSetTranParms(BtHciIfObj *btHciIfObj, BtHciIfTranParms *tranParms)
{
    BtHciIfStatus           status;
    
    MCP_FUNC_START("BT_HCI_IF_MngrSetTranParms");

	/* shouldn't be used yet */
    FMC_ASSERT(0);

    MCP_UNUSED_PARAMETER(btHciIfObj);
        
    status = BT_HCI_IF_STATUS_SUCCESS;
    
    MCP_FUNC_END();

    return status;
}

BtHciIfStatus BT_HCI_IF_MngrTransportOff(BtHciIfObj *btHciIfObj)
{
    BtHciIfStatus       status;

    MCP_FUNC_START("BT_HCI_IF_MngrTransportOn");

    MCP_UNUSED_PARAMETER(btHciIfObj);
        
	MCP_FUNC_END();
		
	return BT_HCI_IF_STATUS_SUCCESS;      
}

BtHciIfStatus BT_HCI_IF_RegisterClient(BtHciIfObj *btHciIfObj, BtHciIfClientCb cb, BtHciIfClientHandle *clientHandle)
{
    McpUint clientIdx;
    McpBool found;

    MCP_FUNC_START("BT_HCI_IF_RegisterClient");

    found = MCP_FALSE;
    
    /* Find a vacant client entry */
    for (clientIdx = 0; clientIdx < _BT_HCI_IF_MAX_NUM_OF_CLIENTS; ++clientIdx)
    {
        if (btHciIfObj->clientsData[clientIdx].containingIfObj == NULL)
        {
            found = MCP_TRUE;
            break;
        }
    }

	if (!found) 
	{
        FMC_ASSERT(found == MCP_TRUE);
        return BT_HCI_IF_STATUS_FAILED;
	}

    /* Init found entry */
    btHciIfObj->clientsData[clientIdx].containingIfObj = btHciIfObj;
    btHciIfObj->clientsData[clientIdx].clientCb = cb;

    /* Client handle is a pointer to the selected entry */
    *clientHandle = &btHciIfObj->clientsData[clientIdx];

    MCP_FUNC_END();

    return BT_HCI_IF_STATUS_SUCCESS;  
}

BtHciIfStatus BT_HCI_IF_DeregisterClient(BtHciIfClientHandle *clientHandle)
{
    _BtHciIf_ClientData *clientData;
    
    MCP_FUNC_START("BT_HCI_IF_DeregisterClient");
    
    clientData = *clientHandle;

    clientData->containingIfObj = NULL;
    clientData->clientCb = NULL;
    
    *clientHandle = NULL;

    MCP_FUNC_END();

    return BT_HCI_IF_STATUS_SUCCESS;    
}

BtHciIfStatus BT_HCI_IF_SendHciCommand( BtHciIfClientHandle     clientHandle,   
                                                BtHciIfHciOpcode        hciOpcode, 
                                                McpU8               *hciCmdParms, 
                                                McpU8               hciCmdParmsLen,
                                                McpU8               cmdCompletionEvent,
                                                void *              pUserData)
{
    BtHciIfStatus           status;
    _BtHciIf_ClientData *clientData;
    
    MCP_FUNC_START("BT_HCI_IF_SendHciCommand");

	/* shouldn't be used yet */
    FMC_ASSERT(0);

    status = BT_HCI_IF_STATUS_PENDING;
    
    MCP_FUNC_END();

    return status;
}

void _BT_HCI_IF_HandleRmgrInitCompleteEvent(McpHalChipId chipId)
{
    BtHciIfMngrEvent    btHciIfMngrEvent;
    BtHciIfObj *btHciIfObj;

    btHciIfObj = &_btHciIf_Data.ifObjs[chipId];
 
    btHciIfMngrEvent.reportingObj = btHciIfObj;
    btHciIfMngrEvent.type = BT_HCI_IF_MNGR_EVENT_TRANPORT_ON_COMPLETED;

    btHciIfMngrEvent.status = BT_HCI_IF_STATUS_SUCCESS;
    
    (btHciIfObj->mngrCb)(&btHciIfMngrEvent);
}

void _BT_HCI_IF_HandleRmgrConfigHciCompleteEvent(McpHalChipId chipId)
{
    BtHciIfMngrEvent    btHciIfMngrEvent;
    BtHciIfObj      *btHciIfObj;
    
    btHciIfObj = &_btHciIf_Data.ifObjs[chipId];
    
    btHciIfMngrEvent.reportingObj = btHciIfObj;
    btHciIfMngrEvent.type = BT_HCI_IF_MNGR_EVENT_HCI_CONFIG_COMPLETED;


    btHciIfMngrEvent.status = BT_HCI_IF_STATUS_SUCCESS;

	
    (btHciIfObj->mngrCb)(&btHciIfMngrEvent);
}

void _BT_HCI_IF_HandleRmgrDeinitCompleteEvent(McpHalChipId chipId)
{
    BtHciIfMngrEvent    btHciIfMngrEvent;
    BtHciIfObj *btHciIfObj;

    btHciIfObj = &_btHciIf_Data.ifObjs[chipId];
 
    btHciIfMngrEvent.reportingObj = btHciIfObj;
    btHciIfMngrEvent.type = BT_HCI_IF_MNGR_EVENT_TRANPORT_OFF_COMPLETED;

    btHciIfMngrEvent.status = BT_HCI_IF_STATUS_SUCCESS;
    
    (btHciIfObj->mngrCb)(&btHciIfMngrEvent);
}

