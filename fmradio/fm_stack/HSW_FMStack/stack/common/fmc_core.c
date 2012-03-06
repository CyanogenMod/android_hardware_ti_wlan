/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright 2010 Sony Ericsson Mobile Communications AB.
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

#define obc 0

#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fmc_types.h"
#include "fmc_config.h"
#include "fmc_defs.h"
#include "fmc_core.h"
#include "fmc_log.h"
#if obc
#include "ccm_im.h"
#endif
#include "fmc_utils.h"
#include "fm_trace.h"
#include "fmc_os.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMCORE);

/* [ToDo] Make sure there is a single process at a time handled by the transport layer */

#if FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED


typedef void (*_FmcTransportCommandCompleteCb)( FmcStatus   status,
                                                FMC_U8      *data,
                                                FMC_UINT    dataLen);

typedef struct {
    FmcCoreEventCb      clientCb;

    /*
        cmdParms stores a local copy of the command parameters. The parameters are copied
        inside the transport function, to avoid caller's and transport's memory management coupling.
        
        The command parms must be stored in a valid location until the command is acknowledged.
    */
    FMC_U8                  cmdParms[FMC_CORE_MAX_PARMS_LEN];
    FMC_UINT                parmsLen;

    /* 
        holds client cb event data that will be sent in response to the current process that is handled
        by the transport layer
    */
    FmcCoreEvent        event;

    FMC_BOOL            fmInterruptRecieved;
#if obc
    CcmObj                  *ccmObj;
    CCM_IM_StackHandle      ccmImStackHandle;
#endif
} _FmcCoreData;

static _FmcCoreData _fmcTransportData;

FMC_STATIC void _FMC_CORE_CcmImCallback(CcmImEvent *event);

FMC_STATIC FmcStatus _FMC_CORE_SendAnyWriteCommand( FmcFwOpcode fmOpcode,
                                                                        FMC_U8*     fmCmdParms,
                                                                        FMC_UINT    len);

FMC_STATIC void _FMC_CORE_CmdCompleteCb(    FmcStatus   status,
                                                        FMC_U8      *data,
                                                        FMC_UINT    dataLen);

FMC_STATIC FmcStatus _FMC_CORE_HCI_RegisterUnregisterGeneralEvent(FMC_BOOL registerInt);

FMC_STATIC void _FMC_CORE_interruptCb(  FmcStatus   status,
                                                    
                                                    FmcCoreEventType type);

CCM_IM_StackHandle _FMC_CORE_GetCcmImStackHandle(void);

static void _FMC_CORE_InterruptCb(FmcOsEvent evtMask);

extern uint16_t g_current_request_opcode;
extern pthread_mutex_t g_current_request_opcode_guard;

FmcStatus FMC_CORE_Init(void)
{
    FmcStatus       status = FMC_STATUS_SUCCESS;
#if obc
    CcmStatus       ccmStatus;
    CcmImObj        *ccmImObj;
#endif

    FMC_FUNC_START("FMC_CORE_Init");

    /* [ToDo] - Protect against mutliple initializations / handle them correctly if allowed*/
    _fmcTransportData.clientCb = NULL;

#if obc
    ccmStatus = CCM_StaticInit();
    FMC_VERIFY_FATAL_NO_RETVAR((ccmStatus == CCM_IM_STATUS_SUCCESS),  ("CCM_StaticInit Failed (%d)", ccmStatus));

    ccmStatus = CCM_Create(MCP_HAL_CHIP_ID_0, &_fmcTransportData.ccmObj);
    FMC_VERIFY_FATAL_NO_RETVAR((ccmStatus == CCM_IM_STATUS_SUCCESS),  ("CCM_Create Failed (%d)", ccmStatus));

    ccmImObj = CCM_GetIm(_fmcTransportData.ccmObj);
    FMC_VERIFY_FATAL_NO_RETVAR((ccmImObj != NULL),  ("CCM_GetIm Returned a Null CCM IM Obj"));

    /* 
        The transport layer is the layer that interacts with the CCM Init Manager on behalf of the FM stack.
        Therefore, the transport layer registers an internal callback to receive CCM IM 
        notifications and process them.
    */  
    ccmStatus = CCM_IM_RegisterStack(ccmImObj, CCM_IM_STACK_ID_FM, _FMC_CORE_CcmImCallback, &_fmcTransportData.ccmImStackHandle);
    FMC_VERIFY_FATAL_NO_RETVAR((ccmStatus == CCM_IM_STATUS_SUCCESS),  ("CCM_IM_RegisterStack Failed (%d)", ccmStatus));
#endif

    /* open the socket to the stack */
    fm_open_cmd_socket(0);

    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_CORE_Deinit(void)
{
    FmcStatus       status = FMC_STATUS_SUCCESS;
#if obc
    CcmStatus       ccmStatus;
#endif

    FMC_FUNC_START("FMC_CORE_Deinit");

#if obc
    ccmStatus = CCM_IM_DeregisterStack(&_fmcTransportData.ccmImStackHandle);
    FMC_VERIFY_FATAL_NO_RETVAR((ccmStatus == CCM_IM_STATUS_SUCCESS),  ("CCM_IM_DeregisterStack Failed (%d)", ccmStatus));

    ccmStatus = CCM_Destroy(&_fmcTransportData.ccmObj);
    FMC_VERIFY_FATAL_NO_RETVAR((ccmStatus == CCM_IM_STATUS_SUCCESS),  ("CCM_Destroy Failed (%d)", ccmStatus));
#endif
        
    fm_close_cmd_socket();

    FMC_FUNC_END();
    
    return status;
}

FmcStatus FMC_CORE_SetCallback(FmcCoreEventCb eventCb)
{
	FmcStatus status;

	FMC_FUNC_START("FMC_CORE_SetCallback");
	
	/* Verify that only a single client at a time requests notifications */
	if(eventCb != NULL)
	{
		status = FMC_CORE_RegisterUnregisterIntCallback(FMC_TRUE);
	   
		FMC_VERIFY_FATAL((_fmcTransportData.clientCb == NULL), FMC_STATUS_INTERNAL_ERROR,
			("FMC_CORE_SetCallback: Callback already set"));

	}
	else
	{
   
		status = FMC_CORE_RegisterUnregisterIntCallback(FMC_FALSE);
	}
	_fmcTransportData.clientCb = eventCb;

	status = FMC_STATUS_SUCCESS;
	
	FMC_FUNC_END();
	
	return status;
} 

FmcStatus FMC_CORE_TransportOn(void)
{
    FmcStatus       status = FMC_STATUS_SUCCESS;
#if obc
    CcmImStatus     ccmImStatus;
#endif

    FMC_FUNC_START("FMC_CORE_TransportOn");

    FMC_LOG_INFO(("FMC_CORE_TransportOn: Calling TI_CHIP_MNGR_FMOn"));

#if obc                
    /* Notify chip manager that FM need to turn on */
    ccmImStatus = CCM_IM_StackOn(_fmcTransportData.ccmImStackHandle);
            
    if (ccmImStatus == CCM_IM_STATUS_SUCCESS) 
    {
        FMC_LOG_INFO(("FMC_CORE_TransportOn: CCM IM FM On Completed Successfully (Immediately)"));
        status = FMC_STATUS_SUCCESS;
    }
    else if (ccmImStatus == CCM_IM_STATUS_PENDING)
    {
        FMC_LOG_INFO(("FMC_CORE_TransportOn: Waiting for CCM IM FM On To Complete"));
        status = FMC_STATUS_PENDING;
    }
    else
    {
        FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, ("FMC_CORE_TransportOn: CCM_IM_FmOn Failed (%d)", ccmImStatus));
    }
#endif

    
    FMC_FUNC_END();

    return status;
}

FmcStatus FMC_CORE_TransportOff(void)
{
    FmcStatus       status = FMC_STATUS_SUCCESS;
#if obc
    CcmImStatus     ccmImStatus;
#endif


    FMC_FUNC_START("FMC_CORE_TransportOff");

    FMC_LOG_INFO(("FMC_CORE_TransportOff: Calling TI_CHIP_MNGR_FMOff"));

#if obc
    ccmImStatus = CCM_IM_StackOff(_fmcTransportData.ccmImStackHandle);

    if (ccmImStatus == CCM_IM_STATUS_SUCCESS) 
    {
        FMC_LOG_INFO(("FMC_CORE_TransportOff: CCM IM FM Off Completed Successfully (Immediately)"));
        status = FMC_STATUS_SUCCESS;
    }
    else if (ccmImStatus == CCM_IM_STATUS_PENDING)
    {
        FMC_LOG_INFO(("FMC_CORE_TransportOff: Waiting for CCM IM FM Off To Complete"));
        status = FMC_STATUS_PENDING;
    }
    else
    {
        FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, ("FMC_CORE_TransportOff: CCM_IM_FmOff Failed (%d)", ccmImStatus ));
    }
#endif
    
    FMC_FUNC_END();

    return status;
}

FmcStatus FMC_CORE_RegisterUnregisterIntCallback(FMC_BOOL reg)
{
    FmcStatus       status = FMC_STATUS_SUCCESS;

    FMC_FUNC_START("FMC_CORE_RegisterUnregisterIntCallback");
    

    status = _FMC_CORE_HCI_RegisterUnregisterGeneralEvent(reg);     

    _fmcTransportData.fmInterruptRecieved = FMC_FALSE;
    
    FMC_FUNC_END();

    return status;
}
/*
    This function is called whenever a command that was processed by the transport layer completes.
    It is called regardless of the transport used to send the command (HCI / I2C).

    The function initializes the fields of the transport event and sends it to the registered client callback
*/
void _FMC_CORE_CmdCompleteCb(   FmcStatus   status,
                                        FMC_U8      *data,
                                        FMC_UINT    dataLen)
{
    /* _fmcTransportData.event.type was set when the command was originally received */
    
    _fmcTransportData.event.status = status;

    /* the data is not copied. The client's callback must copy the data if it wishes to access it afterwards */
    _fmcTransportData.event.data = data;
    _fmcTransportData.event.dataLen = dataLen;
    _fmcTransportData.event.fmInter = FMC_FALSE;

    if(_fmcTransportData.clientCb != NULL)
    {
        (_fmcTransportData.clientCb)(&_fmcTransportData.event);
    }
    else
    {
        FM_ERROR_SYS("_FMC_CORE_CmdCompleteCb NULL pointer!");
    }
}

void _FMC_CORE_interruptCb( FmcStatus   status,
                                    FmcCoreEventType type)
{
    /* _fmcTransportData.event.type was set when the command was originally received */
    FMC_UNUSED_PARAMETER(type); 
    _fmcTransportData.event.status = status;

    _fmcTransportData.event.fmInter = FMC_TRUE;

    if(_fmcTransportData.clientCb != NULL)
    {
        (_fmcTransportData.clientCb)(&_fmcTransportData.event);
    }
    else
    {
        FM_ERROR_SYS("_FMC_CORE_interruptCb NULL pointer!");
    }
}

#if obc
void _FMC_CORE_CcmImCallback(CcmImEvent *event)
{
    FMC_FUNC_START("_FMC_CORE_CcmImCallback");
    
    FMC_LOG_INFO(("Stack: %d, Event: %d", event->stackId, event->type));

    FMC_VERIFY_FATAL_NO_RETVAR((event->stackId == CCM_IM_STACK_ID_FM), 
                                    ("Received an event for another stack (%d)", event->stackId));

    switch (event->type)
    {
        case CCM_IM_EVENT_TYPE_ON_COMPLETE:

            _fmcTransportData.event.type = FMC_CORE_EVENT_TRANSPORT_ON_COMPLETE;
            _fmcTransportData.event.data = NULL;
            _fmcTransportData.event.dataLen = 0;
            
            (_fmcTransportData.clientCb)(&_fmcTransportData.event);
            
            break;

        case CCM_IM_EVENT_TYPE_OFF_COMPLETE:

            _fmcTransportData.event.type = FMC_CORE_EVENT_TRANSPORT_OFF_COMPLETE;
            _fmcTransportData.event.data = NULL;
            _fmcTransportData.event.dataLen = 0;
            
            (_fmcTransportData.clientCb)(&_fmcTransportData.event);
            
            break;

        default:
            FMC_FATAL_NO_RETVAR(("_FMC_CORE_CcmImCallback: Invalid CCM IM Event (%d)", event->type));           
    };

    FMC_FUNC_END();
}
#endif
CCM_IM_StackHandle _FMC_CORE_GetCcmImStackHandle(void)
{
	return NULL;
}
CcmObj* _FMC_CORE_GetCcmObjStackHandle(void)
{
	return NULL;
}


/*************************************************************************************************
                HCI-Specific implementation of transport API  - 
                This Implementation should be changed when working over I2C.
*************************************************************************************************/
/*  Several clients can use the FM Transport. 
    Currently the VAC is the only module (through the sequencer) that uses the FM transport.*/
typedef enum{
    _FMC_CORE_TRANSPORT_CLIENT_FM,
    _FMC_CORE_TRANSPORT_CLIENT_VAC,
    _FMC_CORE_TRANSPORT_MAX_NUM_OF_CLIENTS
}_FmcCoreTransportClients;

/* Vendor Specific Opcodes for the various FM-related commands over HCI */
typedef enum{
	_FMC_CMD_I2C_FM_READ = 0x0133,
	_FMC_CMD_I2C_FM_WRITE = 0x0135,
	_FMC_CMD_FM_POWER_MODE = 0x0137,
	_FMC_CMD_FM_SET_AUDIO_PATH = 0x0139,
	_FMC_CMD_FM_CHANGE_I2C_ADDR = 0x013A,
	_FMC_CMD_FM_I2C_FM_READ_HW_REG = 0x0134,
}_FmcCoreTransportFmCOmmands;
/* 
    Format of an HCI WRITE command to FM over I2C is:
    
    HCI Header:
    ----------
    - HCI Packet Type (Added internally by the HCI Transport Layer)
    
    - HCI Opcode :                                          2 bytes (LSB, MSB - LE)
    - HCI Parameters Total Len (total length of all subsequent fields):     1 byte

    HCI Parameters:
    --------------
    - FM Opcode :                                   1 byte
    - FM Parameters Len:                                2 bytes (LSB, MSB - LE)
    - FM Cmd Parameter Value:                           N bytes 
*/

/* Length of 2nd field of HCI Parameters (described above) */
#define FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN         (2)

/* Total length of HCI Parameters section */
#define FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(fmParmsLen)       \
        (sizeof(FmcFwOpcode) + FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN + fmParmsLen)

/* 
    Format of an HCI READ command to FM over I2C is:
    
    HCI Header:
    ----------
    - HCI Packet Type (Added internally by the HCI Transport Layer)
    
    - HCI Opcode :                                          2 bytes (LSB, MSB - LE)
    - HCI Parameters Total Len (total length of all subsequent fields):     1 byte

    HCI Parameters:
    --------------
    - FM Opcode :                                           1 byte
    - Len of FM Parameters to read:                             2 bytes (LSB, MSB - LE)
*/

/* Length of 2nd field of HCI Parameters (described above) */
#define FMC_CORE_HCI_LEN_OF_FM_PARMS_TO_READ_FIELD_LEN          (2)

/* Total length of HCI Parameters section */
#define FMC_CORE_HCI_READ_FM_TOTAL_PARMS_LEN        \
        (sizeof(FmcFwOpcode) + FMC_CORE_HCI_LEN_OF_FM_PARMS_TO_READ_FIELD_LEN)


FMC_STATIC FmcStatus _FMC_CORE_HCI_SendFmCommand(   
                    _FmcCoreTransportClients                                clientHandle,
                    _FmcTransportCommandCompleteCb      cmdCompleteCb,
                    FMC_U16                             hciOpcode, 
                    FMC_U8                              *hciCmdParms,
                    FMC_UINT                            parmsLen);



FmcStatus FMC_CORE_SendPowerModeCommand(FMC_BOOL powerOn)
{
    FmcStatus status;

    FMC_FUNC_START("FMC_CORE_SendPowerModeCommand");

    /* Convert the transport interface power on indication to the firmware power command value */
    if (powerOn == FMC_TRUE)
    {
        _fmcTransportData.cmdParms[0] =  FMC_FW_FM_CORE_POWER_UP;
    }
    else
    {
        _fmcTransportData.cmdParms[0] =  FMC_FW_FM_CORE_POWER_DOWN;
    }
    
    _fmcTransportData.parmsLen = 1;
    
    _fmcTransportData.event.type = FMC_CORE_EVENT_POWER_MODE_COMMAND_COMPLETE;
    
    status = _FMC_CORE_HCI_SendFmCommand(       _FMC_CORE_TRANSPORT_CLIENT_FM,
                                                    _FMC_CORE_CmdCompleteCb,
                                                    _FMC_CMD_FM_POWER_MODE,
                                                    _fmcTransportData.cmdParms,
                                                    (FMC_U8)_fmcTransportData.parmsLen);
    FMC_OS_Sleep(30);

    FMC_VERIFY_FATAL((status == FMC_STATUS_PENDING), status, 
                    ("FMC_CORE_SendWriteCommand: _FMC_CORE_HCI_SendFmCommand Failed (%d)", status));
    

    FMC_FUNC_END();

    return status;
}

FmcStatus FMC_CORE_SendWriteCommand(FmcFwOpcode fmOpcode, FMC_U16 fmCmdParms)
{
    FmcStatus   status;
    FMC_U8      fmCmdParmsBe[2];    /* Buffer to hols the 2 bytes of FM command parameters */
    
    FMC_FUNC_START("FMC_CORE_SendWriteCommand");
    
    /* Store the cmd parms in BE (always 2 bytes for a write command) */
    FMC_UTILS_StoreBE16(&fmCmdParmsBe[0], fmCmdParms);
    _fmcTransportData.event.type = FMC_CORE_EVENT_WRITE_COMPLETE;
    status = _FMC_CORE_SendAnyWriteCommand( 
                    fmOpcode, 
                    fmCmdParmsBe,
                    sizeof(fmCmdParms));
    FMC_VERIFY_ERR((status == FMC_STATUS_PENDING), status, ("FMC_CORE_SendWriteCommand"));
    

    FMC_FUNC_END();

    return status;
}
FmcStatus FMC_CORE_SendWriteRdsDataCommand(FmcFwOpcode fmOpcode, FMC_U8 *rdsData, FMC_UINT len)
{
    FmcStatus status;

    FMC_FUNC_START("FMC_CORE_SendWriteRdsDataCommand");
    _fmcTransportData.event.type = FMC_CORE_EVENT_WRITE_COMPLETE;
        
    status = _FMC_CORE_SendAnyWriteCommand( 
                    fmOpcode, 
                    rdsData,
                    len);
    FMC_VERIFY_ERR((status == FMC_STATUS_PENDING), status, ("FMC_CORE_SendWriteRdsDataCommand"));
        

    FMC_FUNC_END();

    return status;
}
FmcStatus FMC_CORE_SendReadCommand(FmcFwOpcode fmOpcode,FMC_U16 fmParameter)
{
    FmcStatus status;
    FMC_FUNC_START("FMC_CORE_SendReadCommand");

    /* 
    Format of an HCI READ command to FM over I2C is:
    
    HCI Header:
    ----------
    - HCI Packet Type (Added internally by the HCI Transport Layer)
    
    - HCI Opcode :                                          2 bytes (LSB, MSB - LE)
    - HCI Parameters Total Len (total length of all subsequent fields):     1 byte

    HCI Parameters:
    --------------
    - FM Opcode :                                           1 byte
    - Len of FM Parameters to read:                             2 bytes (LSB, MSB - LE)

        HCI Opcode & HCI Packet Len are arguments in the call to _FMC_CORE_HCI_SendFmCommand.
        FM Opcode & FM Size of Data to read are part of the HCI Command parameters.
    */

    /* Preapre the HCI Command Parameters buffer */
    
    /* Store the FM Opcode*/
    _fmcTransportData.cmdParms[0] = fmOpcode;

    /* Store the FM Parameters Len in LE - For a "simple" (non-RDS) read command it is always 2 bytes */
    FMC_UTILS_StoreLE16(&_fmcTransportData.cmdParms[1], fmParameter);

    _fmcTransportData.parmsLen = FMC_CORE_HCI_READ_FM_TOTAL_PARMS_LEN;
    _fmcTransportData.event.type = FMC_CORE_EVENT_READ_COMPLETE;
        
    status = _FMC_CORE_HCI_SendFmCommand(       _FMC_CORE_TRANSPORT_CLIENT_FM,
                                                    _FMC_CORE_CmdCompleteCb,
                                                    _FMC_CMD_I2C_FM_READ,
                                                    _fmcTransportData.cmdParms,
                                                    _fmcTransportData.parmsLen);
    FMC_VERIFY_FATAL((status == FMC_STATUS_PENDING), status, 
                    ("FMC_CORE_SendReadCommand: _FMC_CORE_HCI_SendFmCommand Failed (%d)", status));

    FMC_FUNC_END();

    return status;
}

static int count = 0;

FmcStatus FMC_CORE_SendHciScriptCommand(    FMC_U16     hciOpcode, 
                                                                FMC_U8      *hciCmdParms, 
                                                                FMC_UINT    len)
{
    FmcStatus status;

    FMC_FUNC_START("FMC_CORE_SendHciScriptCommand");
    
    FMC_VERIFY_ERR((len <= FMC_CORE_MAX_PARMS_LEN), FMC_STATUS_INVALID_PARM, 
                    ("FMC_CORE_SendHciScriptCommand: data len (%d) exceeds max len (%d)", len, FMC_CORE_MAX_PARMS_LEN));

    ++count;

    FMC_OS_MemCopy(_fmcTransportData.cmdParms, hciCmdParms, len);
    _fmcTransportData.parmsLen = len;
    _fmcTransportData.event.type = FMC_CORE_EVENT_SCRIPT_CMD_COMPLETE;
        
    status = _FMC_CORE_HCI_SendFmCommand(           _FMC_CORE_TRANSPORT_CLIENT_FM,
                                                        _FMC_CORE_CmdCompleteCb,
                                                        hciOpcode,
                                                        _fmcTransportData.cmdParms,
                                                        _fmcTransportData.parmsLen);

    FMC_VERIFY_ERR((status == FMC_STATUS_PENDING), status, 
                    ("FMC_CORE_SendHciScriptCommand: _FMC_CORE_HCI_SendFmCommand Failed (%d)", status));
    

    FMC_FUNC_END();

    return status;
}


FmcStatus _FMC_CORE_SendAnyWriteCommand(FmcFwOpcode fmOpcode,
                                                                FMC_U8*     fmCmdParms,
                                                                FMC_UINT    len)
{
    FmcStatus status;

    FMC_FUNC_START("_FMC_CORE_SendAnyWriteCommand");

/*
    HCI Parameters:
    --------------
    - FM Opcode :                                   1 byte
    - FM Parameters Len:                                2 bytes (LSB, MSB - LE)
    - FM Cmd Parameters:                                len bytes (passed in the correct endiannity

    HCI Command Header fields (HCI Opcode & HCI Parameters Total Len) are arguments in the call to 
    _FMC_CORE_HCI_SendFmCommand.
*/

    /* Preapre the HCI Command Parameters buffer */
    
    /* Store the FM Opcode*/
    _fmcTransportData.cmdParms[0] = fmOpcode;

    /* Store the FM Parameters Len in LE */

    FMC_UTILS_StoreLE16(&_fmcTransportData.cmdParms[1], (FMC_U16)len);
    
    /* Copy the FM cmd parms (should be in the correct endiannity already) */
    FMC_OS_MemCopy(&_fmcTransportData.cmdParms[3], fmCmdParms, len);

    _fmcTransportData.parmsLen = FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(len);
    
    status = _FMC_CORE_HCI_SendFmCommand(       _FMC_CORE_TRANSPORT_CLIENT_FM,
                                                    _FMC_CORE_CmdCompleteCb,
                                                    _FMC_CMD_I2C_FM_WRITE,
                                                    _fmcTransportData.cmdParms,
                                                    _fmcTransportData.parmsLen);

    FMC_VERIFY_ERR((status == FMC_STATUS_PENDING), status, 
                    ("_FMC_CORE_SendAnyWriteCommand: _FMC_CORE_HCI_SendFmCommand Failed (%d)", status));
    
    FMC_FUNC_END();

    return status;
}

/*************************************************************************************************
                PORTING TO A PLATFORM SPECIFIC HCI TRANSPORT FROM HERE
*************************************************************************************************/
//#include "hci.h"
//#include "me.h"
/*Fm Transport for VAC operations*/
#include "fm_transport_if.h"
#include "bt_hci_if.h"

typedef struct {
	BtHciIfClientCb sequencerCb;
	void* pUserData;
	_FmcTransportCommandCompleteCb		cmdCompleteCb;
} _FmcTransportHciData;

FMC_STATIC _FmcTransportHciData _fmcTransportHciData[_FMC_CORE_TRANSPORT_MAX_NUM_OF_CLIENTS];
/*
	Parsess the HCI Event.
*/
FMC_STATIC void _FMC_CORE_HCI_ProcessHciEvent(FMC_U8 *rparam, FMC_U8 rlen,
																FmcStatus		*cmdCompleteStatus,
																FMC_U8			**data,
																FMC_U8			*len);
/*
	Callback for calles to FM transport done by the FM client handle.
*/
void _FMC_CORE_HCI_CmdCompleteCb(FMC_U8 *rparam, FMC_U8 rlen, _FmcCoreTransportClients clientHandle);

/*
	Callback for calles to FM transport done by the VAC client handle.
*/
FMC_STATIC void _FM_IF_FmVacCmdCompleteCb( FmcStatus	status,
											FMC_U8		*data,
											FMC_UINT	dataLen);

void FMC_OS_RegisterFMInterruptCallback(FmcOsEventCallback func);
	
/*
	This Function is used to register Callback for interrupts. Since FM Interrupts are recieved any command related we have 
	To capture the FM Interrupt in the transport layer and send it to this callback.
*/
FmcStatus _FMC_CORE_HCI_RegisterUnregisterGeneralEvent(FMC_BOOL registerInt)
{
    static FMC_BOOL unregistered = FMC_TRUE;

    if(registerInt&&unregistered)
    {
    	/*
		When working over HCI the FM Interrupt is generated throghu a Special HCI event HCE_FM_EVENT.
		This registartion function will verify that each time this event is recieved over the HCI the callback will be called.
	*/
        FMC_OS_RegisterFMInterruptCallback(_FMC_CORE_InterruptCb);
        unregistered = FMC_FALSE;
    }
    else if(!(registerInt||unregistered))
    {
        FMC_OS_RegisterFMInterruptCallback(NULL);
        unregistered = FMC_TRUE;
    }

    return FMC_STATUS_SUCCESS;
}

static pthread_cond_t int_cond = PTHREAD_COND_INITIALIZER;

static void _FMC_CORE_InterruptCb(FmcOsEvent evtMask)
{
	FMC_UNUSED_PARAMETER(evtMask);
	FM_BEGIN();
	_FMC_CORE_interruptCb(FMC_STATUS_SUCCESS, FMC_CORE_EVENT_FM_INTERRUPT);
	FM_END();
}
/*
    Send an any FM command over HCI (using ME layer of BT stack)
*/

/* socket for sending hci commands */
static int g_fm_cmd_socket = -1;

FmcStatus _FMC_CORE_HCI_SendFmCommand(  
		_FmcCoreTransportClients            clientHandle,
		_FmcTransportCommandCompleteCb      cmdCompleteCb,
		FMC_U16                             hciOpcode, 
		FMC_U8                              *hciCmdParms,
		FMC_UINT                            parmsLen)
{
	FmcStatus	status = FMC_STATUS_PENDING; //all callers expect PENDING as success indication

	static FMC_U8 hciCmd[HCI_MAX_FRAME_SIZE];
	FMC_U16 opcode;
	FMC_U8 pktType = HCI_COMMAND_PKT, parmsLenU8;

	/* HCI Vendor-specific commands */
#define HCC_GROUP_SHIFT(x)				((x) << 10)

	FMC_FUNC_START("_FMC_TRANSPORT_HCI_SendFmCommandOverHci");

	if(_fmcTransportHciData[clientHandle].cmdCompleteCb!=NULL)
	{
		FMC_FATAL(FMC_STATUS_INTERNAL_ERROR, 
				("_FMC_TRANSPORT_HCI_SendFmCommandOverHci: Waiting for Command complete and tried to send command Failed (%d)", 
				 FMC_STATUS_FAILED));

	}

	_fmcTransportHciData[clientHandle].cmdCompleteCb = cmdCompleteCb;

	hciOpcode = (FMC_U16)(hciOpcode | HCC_GROUP_SHIFT(OGF_VENDOR_CMD));

	FMC_ASSERT(0 == pthread_mutex_lock(&g_current_request_opcode_guard));
	g_current_request_opcode = hciOpcode;

	/* build the HCI command */
	memset(hciCmd, '\0', HCI_MAX_FRAME_SIZE);

	memcpy(hciCmd, &pktType, sizeof(pktType));

	opcode = htobs(hciOpcode);
	//FMC_LOG_INFO(("opcode network byte order = %x", opcode));
	memcpy(hciCmd + 1, &opcode, sizeof(opcode));

	parmsLenU8 = (FMC_U8)parmsLen;
	memcpy(hciCmd + 3, &parmsLenU8, sizeof(parmsLenU8));
	memcpy(hciCmd + 4, hciCmdParms, parmsLen);

	/* write the RAW HCI command */
	if (write(g_fm_cmd_socket, hciCmd, parmsLen + 4) < 0) {
		FM_ERROR_SYS("failed to send command");
		status = FMC_STATUS_FAILED;
		goto out;
	}

	pthread_cond_wait(&int_cond, &g_current_request_opcode_guard);
out:	
	FMC_ASSERT(0 == pthread_mutex_unlock(&g_current_request_opcode_guard));
	FMC_FUNC_END();

	return status;
}

void _FMC_CORE_HCI_CmdCompleteCb(FMC_U8 *rparam, FMC_U8 rlen, _FmcCoreTransportClients clientHandle)
{
    FmcStatus   cmdCompleteStatus;
    FMC_U8      len = 0; 
    FMC_U8      *data = NULL;

	_FmcTransportCommandCompleteCb tempCmdCompleteCb = _fmcTransportHciData[clientHandle].cmdCompleteCb;

	_fmcTransportHciData[clientHandle].cmdCompleteCb = NULL;
    if(tempCmdCompleteCb == NULL)
    {
        FM_ERROR_SYS("_FMC_CORE_HCI_CmdCompleteCb NULL pointer!");
    }
    else
    {
        /* Parse HCI event */
        _FMC_CORE_HCI_ProcessHciEvent(  rparam, rlen,
                                            &cmdCompleteStatus,
                                            &data,
                                            &len);

        /* Call trasnport-independent callback that will forward to the FM stack */
        (tempCmdCompleteCb)(cmdCompleteStatus, data, len);
        pthread_cond_signal(&int_cond);
    }
}

void _FMC_CORE_HCI_ProcessHciEvent(FMC_U8 *rparam, FMC_U8 rlen,
                                                        FmcStatus       *cmdCompleteStatus,
                                                        FMC_U8          **data,
                                                        FMC_U8          *len)
{
	FMC_U8 *result = rparam;
	FMC_U8 result_len = rlen;

	FMC_FUNC_START("_FMC_CORE_HCI_ProcessHciEvent");
		
	/*
	 * if got a status byte, pass it forward, otherwise pass a failure
	 * status
	 */
	if (result_len >= 1 && 0 == result[0])
	{
		*cmdCompleteStatus = FMC_STATUS_SUCCESS;

		/* if got additional data, pass it forward */
		if (result_len >= 2)
			*data = &result[1];
		else
			*data = NULL;

		/*
		 * pass forward total length of data that has arrived (besides the
		 * status byte)
		 */
		*len = result_len - 1;
	}
	else
	{
		*cmdCompleteStatus = FMC_STATUS_FM_COMMAND_FAILED;
		FMC_ERR_NO_RETVAR(("_FMC_CORE_HCI_ProcessHciEvent: HCI Event BT Status: %d", result[0]));
	}

out:
	FMC_FUNC_END();
}

FmcStatus fm_open_cmd_socket(int hci_dev)
{
	FmcStatus ret = FMC_STATUS_SUCCESS;
	struct sockaddr_hci addr;
	FM_BEGIN();

	/*
	 * we do not close this socket; it will stay open as long as FM stack
	 * lives, and will be used to send commands
	 */
	g_fm_cmd_socket = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (g_fm_cmd_socket < 0) {
		FM_ERROR_SYS("Can't create raw socket");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	/* Bind socket to the HCI device */
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = hci_dev;
	if (bind(g_fm_cmd_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		FMC_LOG_ERROR(("Can't bind to device hci%d", hci_dev));
		close(g_fm_cmd_socket);
		ret = FMC_STATUS_FAILED;
	}

out:
	FM_END();
	return ret;
}

FmcStatus fm_close_cmd_socket(void)
{
	FmcStatus ret = FMC_STATUS_SUCCESS;

	close(g_fm_cmd_socket);

	return ret;
}
/*
*   FM_TRANSPORT_IF_SendFmVacCommand()
*
*   This API function was created for the of the VAC Module to sent FM Commands when configuring the 
*   FM IF. The reason we need this is since the FM can work over I2C (and not HCI). 
*   In this case the implementation of this function will be changed.
*
*/
BtHciIfStatus FM_TRANSPORT_IF_SendFmVacCommand( 
                                                McpU8               *hciCmdParms, 
                                                McpU8               hciCmdParmsLen,
                                                BtHciIfClientCb sequencerCb,
                                                void* pUserData)
{
    FmcStatus   status;
    FMC_UINT    len;
    FMC_FUNC_START("FM_TRANSPORT_IF_SendFmVacCommand");

    /* save the sequencer cb to be called when completed. and save the user data.*/
    _fmcTransportHciData[_FMC_CORE_TRANSPORT_CLIENT_VAC].sequencerCb = sequencerCb;
    _fmcTransportHciData[_FMC_CORE_TRANSPORT_CLIENT_VAC].pUserData = pUserData;

    _fmcTransportData.cmdParms[0] = hciCmdParms[0];
    
    /* Store the FM Parameters Len in LE */
    len = (hciCmdParmsLen -(sizeof(FmcFwOpcode) + FMC_CORE_HCI_FM_PARMS_LEN_FIELD_LEN));
    
    FMC_UTILS_StoreLE16(&_fmcTransportData.cmdParms[1], (FMC_U16)len);
    
    /* Copy the FM cmd parms (should be in the correct endiannity already) */
    FMC_OS_MemCopy(&_fmcTransportData.cmdParms[3], &hciCmdParms[3], len);
    
    _fmcTransportData.parmsLen = FMC_CORE_HCI_WRITE_FM_TOTAL_PARMS_LEN(len);
    
    status = _FMC_CORE_HCI_SendFmCommand(       _FMC_CORE_TRANSPORT_CLIENT_VAC,
                                                    _FM_IF_FmVacCmdCompleteCb,
                                                    _FMC_CMD_I2C_FM_WRITE,
                                                    _fmcTransportData.cmdParms,
                                                    _fmcTransportData.parmsLen);
    
    FMC_VERIFY_ERR((status == FMC_STATUS_PENDING), status, ("FM_TRANSPORT_IF_SendFmVacCommand"));
    
    FMC_FUNC_END();

    return BT_HCI_IF_STATUS_PENDING;
}
/*
*   _FM_IF_FmVacCmdCompleteCb()
*
*   callback for FM commands sent by vac - it will call the sequencer to continue the process.
*/
void _FM_IF_FmVacCmdCompleteCb( FmcStatus   status,
                                        FMC_U8      *data,
                                        FMC_UINT    dataLen)
{
    BtHciIfClientEvent      clientEvent;

	#define HCE_COMMAND_COMPLETE               0x0E;
    
    FMC_FUNC_START("_FM_IF_FmVacCmdCompleteCb");

    FMC_VERIFY_FATAL_NO_RETVAR((status == FMC_STATUS_SUCCESS), 
                                    ("_FM_IF_FmVacCmdCompleteCb: Unexpected Event Type or complition status "));
    
    clientEvent.type = BT_HCI_IF_CLIENT_EVENT_HCI_CMD_COMPLETE;
    /* the status is checked previosly for success */
    clientEvent.status = BT_HCI_IF_STATUS_SUCCESS;
    /* get the event data.*/
    clientEvent.data.hciEventData.eventType = (BtHciIfHciEventType )HCE_COMMAND_COMPLETE;
    clientEvent.data.hciEventData.parmLen = (FMC_U8)dataLen;
    clientEvent.data.hciEventData.parms = data;
    /* get the user data set in the send command*/  
    clientEvent.pUserData = _fmcTransportHciData[_FMC_CORE_TRANSPORT_CLIENT_VAC].pUserData;

    /* call the sequencer cb.*/
    _fmcTransportHciData[_FMC_CORE_TRANSPORT_CLIENT_VAC].sequencerCb(&clientEvent);
    

    FMC_FUNC_END();

    
}

#endif /*FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED*/




