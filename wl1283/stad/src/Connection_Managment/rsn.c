/*
 * rsn.c
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


/** \file rsn.c
 *  \brief 802.11 rsniation SM source
 *
 *  \see rsnSM.h
 */


/***************************************************************************/
/*                                                                         */
/*      MODULE: rsnSM.c                                                    */
/*    PURPOSE:  802.11 rsniation SM source                                 */
/*                                                                         */
/***************************************************************************/

#define __FILE_ID__  FILE_ID_40
#include "osApi.h"
#include "paramOut.h"
#include "timer.h"
#include "report.h"
#include "DataCtrl_Api.h"
#include "siteMgrApi.h"
#include "smeApi.h"
#include "rsnApi.h"
#include "rsn.h"
#include "EvHandler.h"
#include "TI_IPC_Api.h"
#include "sme.h"
#include "apConn.h"
#include "802_11Defs.h"
#include "connApi.h"
#ifdef XCC_MODULE_INCLUDED
#include "XCCMngr.h"
#include "admCtrlXCC.h"
#endif
#include "TWDriver.h"
#include "DrvMainModules.h"
#include "PowerMgr_API.h"

/* Constants */
#define PMKID_CAND_LIST_MEMBUFF_SIZE  (2*sizeof(TI_UINT32) + (sizeof(OS_802_11_PMKID_CANDIDATE) * PMKID_MAX_NUMBER))
#define PMKID_MIN_BUFFER_SIZE    2*sizeof(TI_UINT32) + MAC_ADDR_LEN + PMKID_VALUE_SIZE
#define MAX_WPA2_UNICAST_SUITES     (TWD_CIPHER_WEP104+1)
#define MAX_WPA2_KEY_MNG_SUITES     (RSN_KEY_MNG_XCC+1)


/* Cipher suites for group key sent in RSN IE are: WEP40, WEP104, TKIP, CCCMP */
#define GRP_CIPHER_MAXNO_IN_RSNIE         4

/* Cipher suites for unicast key sent in RSN IE are TKIP, CCMP, "use Group key"*/
#define UNICAST_CIPHER_MAXNO_IN_RSNIE     3


/* WPA2 key management suites */
#define WPA2_IE_KEY_MNG_NONE             0
#define WPA2_IE_KEY_MNG_801_1X           1
#define WPA2_IE_KEY_MNG_PSK_801_1X       2
#define WPA2_IE_KEY_MNG_CCKM			 3
#define WPA2_IE_KEY_MNG_NA               4


#define WPA2_OUI_MAX_VERSION           0x1
#define WPA2_OUI_DEF_TYPE              0x1
#define WPA2_OUI_MAX_TYPE              0x2

#define WPA2_PRE_AUTH_CAPABILITY_MASK               0x0001   /* bit 0 */
#define WPA2_PRE_AUTH_CAPABILITY_SHIFT              0
#define WPA2_GROUP_4_UNICAST_CAPABILITY_MASK        0x0002   /* bit 1 No Pairwise */
#define WPA2_GROUP_4_UNICAST_CAPABILITY_SHIFT        1
#define WPA2_PTK_REPLAY_COUNTERS_CAPABILITY_MASK    0x000c   /* bit 2 and 3 */
#define WPA2_PTK_REPLAY_COUNTERS_CAPABILITY_SHIFT   2
#define WPA2_GTK_REPLAY_COUNTERS_CAPABILITY_MASK    0x0030   /* bit 4 and 5 */
#define WPA2_GTK_REPLAY_COUNTERS_CAPABILITY_SHIFT   4
                                                             /* bit 6 - 15 - reserved */
#define WPA2_IE_MIN_LENGTH                  4
#define WPA2_IE_GROUP_SUITE_LENGTH          8
#define WPA2_IE_MIN_PAIRWISE_SUITE_LENGTH   14
#define WPA2_IE_MIN_DEFAULT_LENGTH          24
#define WPA2_IE_MIN_KEY_MNG_SUITE_LENGTH(pairwiseCnt) (10+4*pairwiseCnt)


#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

static TI_UINT8 wpa2IeOuiIe[3] = { 0x00, 0x0f, 0xac};

/* Enumerations */

/* Typedefs */

/* WPA2 data parsed from RSN info element */
typedef struct
{

    ECipherSuite        broadcastSuite;
    TI_UINT16              unicastSuiteCnt;
    ECipherSuite        unicastSuite[MAX_WPA2_UNICAST_SUITES];
    TI_UINT16              KeyMngSuiteCnt;
    TI_UINT8               KeyMngSuite[MAX_WPA2_KEY_MNG_SUITES];
    TI_UINT8               preAuthentication;
    TI_UINT8               bcastForUnicatst;
    TI_UINT8               ptkReplayCounters;
    TI_UINT8               gtkReplayCounters;
    TI_UINT16              pmkIdCnt;
    TI_UINT8               pmkId[PMKID_VALUE_SIZE];
    TI_UINT8               lengthWithoutPmkid;
} wpa2IeData_t;



/* Structures */

/* External data definitions */

/* External functions definitions */

/* Global variables */

/* Local function prototypes */
TI_STATUS rsn_sendKeysNotSet(rsn_t *pRsn);
void rsn_pairwiseReKeyTimeout(TI_HANDLE hRsn, TI_BOOL bTwdInitOccured);
void rsn_groupReKeyTimeout(TI_HANDLE hRsn, TI_BOOL bTwdInitOccured);
void rsn_micFailureReportTimeout (TI_HANDLE hRsn, TI_BOOL bTwdInitOccured);
static rsn_siteBanEntry_t * findEntryForInsert(TI_HANDLE hRsn);
static rsn_siteBanEntry_t * findBannedSiteAndCleanup(TI_HANDLE hRsn, TMacAddr siteBssid);
TI_STATUS rsn_resetPMKIDCache(rsn_t *pRsn);
TI_STATUS rsn_resetPMKIDList(TI_HANDLE hRsn);
TI_STATUS rsn_getPMKIDList (rsn_t * pRsn,OS_802_11_PMKID *pmkidList);

/* functions */

/**
*
* rsn_Create - allocate memory for rsniation SM
*
* \b Description:
*
* Allocate memory for rsniation SM. \n
*       Allocates memory for Rsniation context. \n
*       Allocates memory for rsniation timer. \n
*       Allocates memory for rsniation SM matrix. \n
*
* \b ARGS:
*
*  I   - hOs - OS context  \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_mainSecKeysOnlyStop()
*/
TI_HANDLE rsn_create(TI_HANDLE hOs)
{
    rsn_t  *pRsn;

    /* allocate rsniation context memory */
    pRsn = (rsn_t*)os_memoryAlloc (hOs, sizeof(rsn_t));
    if (pRsn == NULL)
    {
        return NULL;
    }

    os_memoryZero (hOs, pRsn, sizeof(rsn_t));

    pRsn->hOs = hOs;

    return pRsn;
}


/**
*
* rsn_Unload - unload rsniation SM from memory
*
* \b Description:
*
* Unload rsniation SM from memory
*
* \b ARGS:
*
*  I   - hRsn - rsniation SM context  \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_mainSecKeysOnlyStop()
*/
TI_STATUS rsn_unload (TI_HANDLE hRsn)
{
    rsn_t           *pRsn;

    if (hRsn == NULL)
    {
        return TI_NOK;
    }

    pRsn = (rsn_t*)hRsn;

    if (pRsn->hMicFailureReportWaitTimer)
    {
    	tmr_DestroyTimer (pRsn->hMicFailureReportWaitTimer);
    }
    if (pRsn->hMicFailureGroupReKeyTimer)
    {
    	tmr_DestroyTimer (pRsn->hMicFailureGroupReKeyTimer);
    }
    if (pRsn->hMicFailurePairwiseReKeyTimer)
    {
    	tmr_DestroyTimer (pRsn->hMicFailurePairwiseReKeyTimer);
    }
   	if (pRsn->hPreAuthTimerWpa2)
   	{
   		tmr_DestroyTimer (pRsn->hPreAuthTimerWpa2);
   	}

    os_memoryFree (pRsn->hOs, hRsn, sizeof(rsn_t));
    return TI_OK;
}

#ifdef XCC_MODULE_INCLUDED

/**
*
* rsn_setNetworkEap  - Set current Network EAP Mode Status.
*
* \b Description:
*
* Set current Network EAP Mode Status..
*
* \b ARGS:
*
*  I   - prsn - context \n
*  I   - networkEap - Network EAP Mode \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_setNetworkEap(rsn_t *pRsn, OS_XCC_NETWORK_EAP networkEap)
{
    if (pRsn==NULL)
        return TI_NOK;

    if (pRsn->networkEapMode == networkEap)
    {
        return TI_OK;
    }
    pRsn->networkEapMode = networkEap;

    return TI_OK;
}

/**
*
* rsn_getNetworkEap  - Get current Network EAP Mode Status.
*
* \b Description:
*
* Get current Network EAP Mode Status.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  I   - networkEap - Network EAP Mode \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_getNetworkEap(rsn_t *pRsn, OS_XCC_NETWORK_EAP *networkEap)
{

    if (pRsn==NULL)
    {
        return TI_NOK;
    }

    switch (pRsn->networkEapMode)
    {
    case OS_XCC_NETWORK_EAP_OFF:
        *networkEap = OS_XCC_NETWORK_EAP_OFF;
        break;
    case OS_XCC_NETWORK_EAP_ON:
    case OS_XCC_NETWORK_EAP_ALLOWED:
    case OS_XCC_NETWORK_EAP_PREFERRED:
        *networkEap = OS_XCC_NETWORK_EAP_ON;
        break;
    default:
        return TI_NOK;
/*      break; - unreachable */
    }

    return TI_OK;
}
#endif /* XCC_MODULE_INCLUDED*/

/**
*
* rsn_init - Init module
*
* \b Description:
*
* Init module handles and variables.
*
* \b RETURNS:
*
*  void
*
* \sa rsn_Create, rsn_Unload
*/
void rsn_init (TStadHandlesList *pStadHandles)
{
    rsn_t *pRsn = (rsn_t*)(pStadHandles->hRsn);

    pRsn->eGroupKeyUpdate = GROUP_KEY_UPDATE_FALSE;
    pRsn->ePairwiseKeyUpdate = PAIRWISE_KEY_UPDATE_FALSE;

	pRsn->hTxCtrl   = pStadHandles->hTxCtrl;
    pRsn->hRx       = pStadHandles->hRxData;
    pRsn->hConn     = pStadHandles->hConn;
    pRsn->hTWD      = pStadHandles->hTWD;
    pRsn->hCtrlData = pStadHandles->hCtrlData;
    pRsn->hSiteMgr  = pStadHandles->hSiteMgr;
    pRsn->hReport   = pStadHandles->hReport;
    pRsn->hOs       = pStadHandles->hOs;
    pRsn->hXCCMngr  = pStadHandles->hXCCMngr;
    pRsn->hEvHandler= pStadHandles->hEvHandler;
    pRsn->hSmeSm    = pStadHandles->hSme;
    pRsn->hAPConn   = pStadHandles->hAPConnection;
    pRsn->hMlme     = pStadHandles->hMlme;
    pRsn->hPowerMgr = pStadHandles->hPowerMgr;
    pRsn->hTimer    = pStadHandles->hTimer;
    pRsn->hCurrBss  = pStadHandles->hCurrBss;

	pRsn->genericIE.length = 0;
	pRsn->numOfBannedSites = 0;
}


void rsn_clearGenInfoElement(TI_HANDLE hRsn)
{
	rsn_t *pRsn = (rsn_t*)(hRsn);

	pRsn->genericIE.length = 0;
}

/*-----------------------------------------------------------------------------
Routine Name: rsn_notifyPreAuthStatus
Routine Description: This routine is used to notify higher level application of the pre-authentication status
Arguments: newStatus - pre authentication status
Return Value:
-----------------------------------------------------------------------------*/
void rsn_notifyPreAuthStatus (rsn_t *pRsn, preAuthStatusEvent_e newStatus)
{
    TI_UINT32 memBuff;

    memBuff = (TI_UINT32) newStatus;

    EvHandlerSendEvent(pRsn->hEvHandler, IPC_EVENT_WPA2_PREAUTHENTICATION,
                            (TI_UINT8*)&memBuff, sizeof(TI_UINT32));

}

/**
*
* rsn_resetPMKIDList -
*
* \b Description:
*   Cleans up the PMKID cache.
*   Called when SSID is being changed.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*/

TI_STATUS rsn_resetPMKIDList(TI_HANDLE hRsn)
{
    rsn_t  *pRsn = (rsn_t*)hRsn;

    if (!pRsn)
        return TI_NOK;

    return rsn_resetPMKIDCache(pRsn);
}

/**
*
* rsn_findPMKID
*
* \b Description:
*
* Retrieve an AP's PMKID (if exist)

* \b ARGS:
*
*  I   - pRsn     - pointer to rsn context
*  I   - pBSSID   - pointer to AP's BSSID address
*  O   - pmkID    - pointer to AP's PMKID (if it is NULL ptr, only
*                   cache index will be returned to the caller)
*  O   - cacheIndex  - index of the cache table entry containing the
                       bssid
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_findPMKID (rsn_t * pRsn, TMacAddr *pBSSID,
                                 pmkidValue_t *pPMKID, TI_UINT8  *cacheIndex)
{

	TI_UINT8           i     = 0;
	TI_BOOL            found = TI_FALSE;
	TMacAddr    entryMac;
	TI_STATUS       status = TI_NOK;

	while(!found && (i < RSN_PMKID_CACHE_SIZE) &&
			(i <= pRsn->pmkid_cache.entriesNumber))
	{
		MAC_COPY (entryMac, pRsn->pmkid_cache.pmkidTbl[i].bssId);
		if (MAC_EQUAL (entryMac, *pBSSID))
		{
			found       = TI_TRUE;
			*cacheIndex = i;
			if(pPMKID)
			{
				os_memoryCopy(pRsn->hOs, (void*)pPMKID,
						pRsn->pmkid_cache.pmkidTbl[i].pmkId,
						PMKID_VALUE_SIZE);
			}
		}
		i++;
	}

	if(found)
		status = TI_OK;

	return status;

}


/**
*
* rsn_getPMKIDList
*
* \b Description:
*
* Returns content of the PMKID cache
*
* \b ARGS:
*
*  I   - pRsn            - pointer to rsn context
*  O   - pmkidList       - memory buffer where the procedure writes the PMKIDs
*                          Supplied by the caller procedure. .
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_getPMKIDList (rsn_t * pRsn,OS_802_11_PMKID *pmkidList)
{

    TI_UINT8   neededLength, i = 0;
    TI_UINT8   NumOfEntries = pRsn->pmkid_cache.entriesNumber;
    TI_UINT8   *bssid, *pmkid;

    if(!pRsn->preAuthSupport)
        return PARAM_NOT_SUPPORTED;

    /* Check the buffer length */
    if(NumOfEntries > 1)
       neededLength = 30 + ((NumOfEntries - 1) * (MAC_ADDR_LEN + PMKID_VALUE_SIZE));
    else
       neededLength = 30;

    if(neededLength > pmkidList->Length)
    {
        /* The buffer length is not enough */
        pmkidList->Length = neededLength;
        return TI_NOK;
    }

    /* The buffer is big enough. Fill the info */
    pmkidList->Length         = neededLength;
    pmkidList->BSSIDInfoCount = NumOfEntries;

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  Get PMKID cache.  Number of entries  = %d \n", NumOfEntries);

    for (i = 0; i < NumOfEntries; i++ )
    {
        bssid = (TI_UINT8 *) pRsn->pmkid_cache.pmkidTbl[i].bssId;
        pmkid = (TI_UINT8 *)pRsn->pmkid_cache.pmkidTbl[i].pmkId;

        MAC_COPY(pmkidList->osBSSIDInfo[i].BSSID, bssid);

        os_memoryCopy(pRsn->hOs,
                      (void *)pmkidList->osBSSIDInfo[i].PMKID,
                      &pmkid,
                      PMKID_VALUE_SIZE);

        TRACE22(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  BSSID:  %.2X-%.2X-%.2X-%.2X-%.2X-%.2X   PMKID: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X  \n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], pmkid[0], pmkid[1], pmkid[2], pmkid[3], pmkid[4], pmkid[5], pmkid[6], pmkid[7], pmkid[8], pmkid[9], pmkid[10],pmkid[11], pmkid[12],pmkid[13],pmkid[14],pmkid[15]);
    }

    return TI_OK;

}

/**
*
* rsn_addPMKID
*
* \b Description:
*
* Add/Set an AP's PMKID received from the Supplicant
*
* \b ARGS:
*
*  I   - pRsn     - pointer to rsn context
*  I   - pBSSID   - pointer to AP's BSSID address
*  I   - pmkID    - AP's PMKID
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_addPMKID (rsn_t *pRsn, TMacAddr *pBSSID, pmkidValue_t pmkID)
{
	TI_UINT8      cacheIndex;
	TI_STATUS     status = TI_NOK;

	TRACE6(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_addPMKID: received new PMKID for " MACSTR "\n",
           (*pBSSID)[0],
           (*pBSSID)[1],
           (*pBSSID)[2],
           (*pBSSID)[3],
           (*pBSSID)[4],
           (*pBSSID)[5]);
    TRACE_INFO_HEX(pRsn->hReport,  pmkID, PMKID_VALUE_SIZE);

	/* Try to find the pBSSId in the PMKID cache */
	status = rsn_findPMKID (pRsn, pBSSID, NULL, &cacheIndex);

	if(status == TI_OK)
	{
		/* Entry for the bssid has been found; Update PMKID */
		os_memoryCopy(pRsn->hOs,
				(void*)&pRsn->pmkid_cache.pmkidTbl[cacheIndex].pmkId,
				pmkID, PMKID_VALUE_SIZE);
		/*pAdmCtrl->pmkid_cache.pmkidTbl[cacheIndex].generationTs = os_timeStampMs(pAdmCtrl->hOs); */
	}
	else
	{
		/* The new entry is added to the next free entry. */
		/* Copy the new entry to the next free place.     */
		cacheIndex = pRsn->pmkid_cache.nextFreeEntry;
		MAC_COPY (pRsn->pmkid_cache.pmkidTbl[cacheIndex].bssId, *pBSSID);
		os_memoryCopy(pRsn->hOs,
				(void*)&pRsn->pmkid_cache.pmkidTbl[cacheIndex].pmkId,
				(void*)pmkID,
				PMKID_VALUE_SIZE);

		/* Update the next free entry index. (If the table is full, a new entry */
		/* will override the oldest entries from the beginning of the table)    */
		/* Update the number of entries. (it cannot be more than max cach size) */
		pRsn->pmkid_cache.nextFreeEntry  = (cacheIndex + 1) % RSN_PMKID_CACHE_SIZE;

		if(pRsn->pmkid_cache.entriesNumber < RSN_PMKID_CACHE_SIZE)
			pRsn->pmkid_cache.entriesNumber ++;
	}

	TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN   Add PMKID   Entry index is %d \n", cacheIndex);
	TRACE22(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  BSSID: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X  PMKID: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X  \n", (*pBSSID)[0], (*pBSSID)[1], (*pBSSID)[2], (*pBSSID)[3], (*pBSSID)[4], (*pBSSID)[5], pmkID[0], pmkID[1], pmkID[2], pmkID[3], pmkID[4], pmkID[5], pmkID[6], pmkID[7], pmkID[8], pmkID[9], pmkID[10],pmkID[11], pmkID[12],pmkID[13],pmkID[14],pmkID[15]);

	return TI_OK;
}

/**
*
* rsn_setPMKIDList
*
* \b Description:
*
* Set PMKID cache
*
* \b ARGS:
*
*  I   - pRsn            - pointer to rsn context
*  O   - pmkidList       - memory buffer where the procedure reads the PMKIDs from
*                          Supplied by the caller procedure.
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_setPMKIDList (rsn_t *pRsn, OS_802_11_PMKID *pmkidList)
{
    TI_UINT8          neededLength, i = 0;
    TI_UINT8          NumOfEntries;
    TMacAddr   macAddr;

    /* Check the minimal buffer length */
    if (pmkidList->Length < 2*sizeof(TI_UINT32))
    {
        TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set PMKID list - Buffer size < min length (8 bytes). Supplied length is %d .\n", pmkidList->Length);
        return TI_NOK;
    }

    /* Check the num of entries in the buffer: if 0 it means that */
    /* PMKID cache has to be cleaned                              */
    if(pmkidList->BSSIDInfoCount == 0)
    {
        rsn_resetPMKIDCache(pRsn);
        return TI_OK;
    }

    /* Check the buffer length */
    NumOfEntries = (TI_UINT8)pmkidList->BSSIDInfoCount;
    neededLength =  2*sizeof(TI_UINT32) + (NumOfEntries  *(MAC_ADDR_LEN + PMKID_VALUE_SIZE));

    if(pmkidList->Length < neededLength)
    {
        /* Something wrong goes with the buffer */
        TRACE3(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set PMKID list - no enough room for %d entries Needed length is %d. Supplied length is %d .\n", NumOfEntries, neededLength,pmkidList->Length);
        return TI_NOK;
    }

    /*  Write  the PMKID to the PMKID cashe */
    pmkidList->BSSIDInfoCount = NumOfEntries;
    for (i = 0; i < NumOfEntries; i++ )
    {
         MAC_COPY (macAddr, pmkidList->osBSSIDInfo[i].BSSID);

         TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION , "rsn_setPMKIDList: Received new pre-auth AP\n");
         if (pRsn->numberOfPreAuthCandidates)
         {
            pRsn->numberOfPreAuthCandidates--;
            if (pRsn->numberOfPreAuthCandidates == 0)
            {
               TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION , "Stopping the Pre-Auth timer since Pre-auth is finished\n");
               tmr_StopTimer (pRsn->hPreAuthTimerWpa2);
               /* Send PRE-AUTH end event to External Application */
               rsn_notifyPreAuthStatus (pRsn, RSN_PRE_AUTH_END);
            }

            TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION , "rsn_setPMKIDList: %d APs left in candidate list\n",pRsn->numberOfPreAuthCandidates);
         }
        else
        {
           TRACE0(pRsn->hReport, REPORT_SEVERITY_WARNING , "rsn_setPMKIDList: number of candidates was already zero...\n");
        }
        rsn_addPMKID(pRsn,&macAddr, (TI_UINT8 *)pmkidList->osBSSIDInfo[i].PMKID);
    }

    return TI_OK;

}

/**
*
* rsn_resetPMKIDCache
*
* \b Description:
*
* Reset PMKID Table
*
* \b ARGS:
*
*  I   - pRsn - pointer to rsn context
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_resetPMKIDCache (rsn_t *pRsn)
{
	TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  Reset PMKID cache.  %d entries are deleted. \n", pRsn->pmkid_cache.entriesNumber);
	os_memoryZero(pRsn->hOs, (void*)&pRsn->pmkid_cache, sizeof(pmkid_cache_t));
	return TI_OK;
}


/**
*
* rsn_getPreAuthStatus
*
* \b Description:
*
* Returns the status of the Pre Auth for the BSSID. If the authentictaion mode
 * is not WPA2, then TI_FALSE will be returned.
 * For WPA2 mode, if PMKID exists fro the BSSID and its liftime is valid
 * TI_TRUE will be returned.
 * Otherwise TI_FALSE.
*
*
*
* \b ARGS:
*  I   - pRsn     - pointer to rsn context
 * I   - givenAP  - required BSSID
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
static TI_BOOL rsn_getPreAuthStatus(rsn_t *pRsn, TMacAddr *givenAP, TI_UINT8  *cacheIndex)
{
    pmkidValue_t    PMKID;

    if (rsn_findPMKID (pRsn, givenAP, &PMKID, cacheIndex)!=TI_OK)
    {
        return TI_FALSE;
    }
    return TI_TRUE;

}



TI_STATUS rsn_SetDefaults (TI_HANDLE hRsn, TRsnInitParams *pInitParam)
{
    rsn_t       *pRsn = (rsn_t*)hRsn;

    pRsn->bMixedMode = pInitParam->mixedMode;
    if (pInitParam->privacyOn) {
    	pRsn->broadcastSuite = TWD_CIPHER_WEP;
    	pRsn->unicastSuite = TWD_CIPHER_WEP;
    } else {
    	pRsn->broadcastSuite = TWD_CIPHER_NONE;
    	pRsn->unicastSuite = TWD_CIPHER_NONE;
    }

    pRsn->bPairwiseMicFailureFilter = pInitParam->bPairwiseMicFailureFilter;
    pRsn->preAuthSupport     = pInitParam->preAuthSupport;
    pRsn->preAuthTimeout     = pInitParam->preAuthTimeout;
    pRsn->MaxNumOfPMKIDs     = PMKID_MAX_NUMBER;
    pRsn->numberOfPreAuthCandidates = 0;

    /* Create hPreAuthTimerWpa2 timer */
    pRsn->hPreAuthTimerWpa2 = tmr_CreateTimer (pRsn->hTimer);
    if (pRsn->hPreAuthTimerWpa2 == NULL)
    {
    	TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR , "rsn_setDefaults(): Failed to create hPreAuthTimerWpa2!\n");
    }

    /* Create the module's timers */
    pRsn->hMicFailureReportWaitTimer = tmr_CreateTimer (pRsn->hTimer);
    if (pRsn->hMicFailureReportWaitTimer == NULL)
    {
    	TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, "rsn_SetDefaults(): Failed to create hMicFailureReportWaitTimer!\n");
    	return TI_NOK;
    }
    pRsn->hMicFailureGroupReKeyTimer = tmr_CreateTimer (pRsn->hTimer);
    if (pRsn->hMicFailureGroupReKeyTimer == NULL)
    {
    	TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, "rsn_SetDefaults(): Failed to create hMicFailureGroupReKeyTimer!\n");
    	return TI_NOK;
    }

    pRsn->hMicFailurePairwiseReKeyTimer = tmr_CreateTimer (pRsn->hTimer);
    if (pRsn->hMicFailurePairwiseReKeyTimer == NULL)
    {
    	TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, "rsn_SetDefaults(): Failed to create hMicFailurePairwiseReKeyTimer!\n");
    	return TI_NOK;
    }

#ifdef XCC_MODULE_INCLUDED
    pRsn->networkEapMode = OS_XCC_NETWORK_EAP_OFF;
#endif

	return TI_OK;
}






/**
*
* rsn_Start - Start event for the rsniation SM
*
* \b Description:
*
* Start event for the rsniation SM
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Stop, rsn_Recv
*/
TI_STATUS rsn_start(TI_HANDLE hRsn)
{
    rsn_t               *pRsn;

    pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return TI_NOK;
    }

    TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_start ...\n");

    return TI_OK;
}



/**
*
* rsn_Stop - Stop event for the rsniation SM
*
* \b Description:
*
* Stop event for the rsniation SM
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Start, rsn_Recv
*/
TI_STATUS rsn_stop (TI_HANDLE hRsn, TI_BOOL removeKeys)
{
    rsn_t           *pRsn;

    pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return TI_NOK;
    }

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: calling STOP... removeKeys=%d\n", removeKeys);

    pRsn->eGroupKeyUpdate = GROUP_KEY_UPDATE_FALSE;
    pRsn->ePairwiseKeyUpdate = PAIRWISE_KEY_UPDATE_FALSE;

    tmr_StopTimer (pRsn->hMicFailureReportWaitTimer);

    /* Stop the pre-authentication timer in case we are disconnecting */
    tmr_StopTimer (pRsn->hPreAuthTimerWpa2);

#ifdef XCC_MODULE_INCLUDED
	pRsn->networkEapMode = OS_XCC_NETWORK_EAP_OFF;
#endif

	if (removeKeys)
	{   /* reset PMKID list if exist */
		rsn_resetPMKIDList(pRsn);
	}


    return TI_OK;
}



/**
*
* rsn_getCipherSuite  - get current broadcast cipher suite support.
*
* \b Description:
*
* get current broadcast cipher suite support.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  O   - suite - cipher suite to work with \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_getCipherSuite(rsn_t *pRsn, ECipherSuite *pSuite)
{
    if (pRsn == NULL)
    {
        return TI_NOK;
    }

    *pSuite = (pRsn->broadcastSuite > pRsn->unicastSuite) ? pRsn->broadcastSuite : pRsn->unicastSuite;

    return TI_OK;
}


TI_STATUS rsn_getParamEncryptionStatus(TI_HANDLE hRsn, ECipherSuite *rsnStatus)
{ /* RSN_ENCRYPTION_STATUS_PARAM */
    rsn_t       *pRsn = (rsn_t *)hRsn;
    TI_STATUS   status = TI_NOK;

    if ( (NULL == pRsn) || (NULL == rsnStatus) )
    {
        return status;
    }
    status = rsn_getCipherSuite(pRsn, rsnStatus);
    return status;
}



/**
*
* rsn_getExtAuthMode  - Get current External authentication Mode Status.
*
* \b Description:
*
* Get current External Mode Status.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  I   - pExtAuthMode - XCC External Mode Status \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_getExtAuthMode(rsn_t *pRsn, EExternalAuthMode *pExtAuthMode)
{
    *pExtAuthMode = pRsn->externalAuthMode;

    return TI_OK;
}



/**
*
* rsn_GetParam - Get a specific parameter from the rsniation SM
*
* \b Description:
*
* Get a specific parameter from the rsniation SM.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*  I/O - pParam - Parameter \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Start, rsn_Stop
*/
TI_STATUS rsn_getParam(TI_HANDLE hRsn, void *param)
{
    rsn_t       *pRsn = (rsn_t *)hRsn;
    paramInfo_t *pParam = (paramInfo_t*)param;
    TI_STATUS   status = TI_OK;

    if ( (NULL == pRsn) || (NULL == pParam) )
    {
        return TI_NOK;
    }

    switch (pParam->paramType)
    {
    case RSN_ENCRYPTION_STATUS_PARAM:
        status = rsn_getCipherSuite (pRsn, &pParam->content.rsnEncryptionStatus);
        break;

    case RSN_EXT_AUTHENTICATION_MODE:
        status = rsn_getExtAuthMode (pRsn, &pParam->content.rsnExtAuthneticationMode);
        break;

    case RSN_MIXED_MODE:
        pParam->content.rsnMixedMode = pRsn->bMixedMode;
        status = TI_OK;
        break;

    case RSN_PMKID_LIST:
         pParam->content.rsnPMKIDList.Length = pParam->paramLength;
         status = rsn_getPMKIDList (pRsn, &pParam->content.rsnPMKIDList);
         pParam->paramLength = pParam->content.rsnPMKIDList.Length + 2 * sizeof(TI_UINT32);
         break;

    case RSN_PRE_AUTH_STATUS:
          {
              TI_UINT8 cacheIndex;

              pParam->content.rsnPreAuthStatus = rsn_getPreAuthStatus (pRsn, &pParam->content.rsnApMac, &cacheIndex);
          }
          break;


#ifdef XCC_MODULE_INCLUDED
    case RSN_XCC_NETWORK_EAP:
        status = rsn_getNetworkEap (pRsn, &pParam->content.networkEap);
        break;
#endif

    case RSN_PORT_STATUS_PARAM:
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Get Port Status\n"  );
		pParam->content.rsnPortStatus = rsn_getPortStatus( pRsn );
        break;

    default:
        return TI_NOK;
    }

    return status;
}




/**
*
* rsn_setUcastSuite  - Set current unicast cipher suite support.
*
* \b Description:
*
* Set current unicast cipher suite support.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  I   - suite - cipher suite to work with \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_setUcastSuite(rsn_t *pRsn, ECipherSuite suite)
{
	pRsn->unicastSuite = suite;

	return TI_OK;
}


/**
*
* rsn_setBcastSuite  - Set current broadcast cipher suite support.
*
* \b Description:
*
* Set current broadcast cipher suite support.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  I   - suite - cipher suite to work with \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_setBcastSuite(rsn_t *pRsn, ECipherSuite suite)
{
	pRsn->broadcastSuite = suite;

	return TI_OK;
}


/**
*
* rsn_setExtAuthMode  - Set current External authentication Mode Status.
*
* \b Description:
*
* Set current External authentication Mode Status.
*
* \b ARGS:
*
*  I   - pRsn - context \n
*  I   - extAuthMode - External authentication Mode \n
*
* \b RETURNS:
*
*  TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_setExtAuthMode(rsn_t *pRsn, EExternalAuthMode extAuthMode)
{
    if (extAuthMode >= RSN_EXT_AUTH_MODEMAX)
    {
        return TI_NOK;
    }

    pRsn->externalAuthMode = extAuthMode;

    return TI_OK;
}



/**
*
* rsn_SetParam - Set a specific parameter to the rsniation SM
*
* \b Description:
*
* Set a specific parameter to the rsniation SM.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*  I/O - pParam - Parameter \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Start, rsn_Stop
*/
TI_STATUS rsn_setParam (TI_HANDLE hRsn, void *param)
{
    rsn_t               *pRsn;
    paramInfo_t         *pParam = (paramInfo_t*)param;
    TI_STATUS           status = TI_OK;

    pRsn = (rsn_t*)hRsn;

    if ( (NULL == pRsn) || (NULL == pParam) )
    {
        return TI_NOK;
    }

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set rsn_setParam   %X \n", pParam->paramType);

    switch (pParam->paramType)
    {


    case RSN_ENCRYPTION_STATUS_PARAM:
        {
            TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_ENCRYPTION_STATUS_PARAM rsnEncryptionStatus  %d \n", pParam->content.rsnEncryptionStatus);

            rsn_setUcastSuite (pRsn, pParam->content.rsnEncryptionStatus);
            rsn_setBcastSuite (pRsn, pParam->content.rsnEncryptionStatus);
            TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, " status = %d \n", status);
            status = TI_OK;
        }
        break;

    case RSN_EXT_AUTHENTICATION_MODE:
        {
        	paramInfo_t mlmeParam;

        	TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_EXT_AUTHENTICATION_MODE rsnExtAuthneticationMode  %d \n", pParam->content.rsnExtAuthneticationMode);

        	status = rsn_setExtAuthMode (pRsn, pParam->content.rsnExtAuthneticationMode);

            if (status == TI_OK) {
            	mlmeParam.paramType = MLME_LEGACY_TYPE_PARAM;
            	if (pParam->content.rsnExtAuthneticationMode == os802_11AuthModeShared)
            		mlmeParam.content.mlmeLegacyAuthType = AUTH_LEGACY_SHARED_KEY;
            	else
            		mlmeParam.content.mlmeLegacyAuthType = AUTH_LEGACY_OPEN_SYSTEM;

            	status = mlme_setParam(pRsn->hMlme, &mlmeParam);
            	if (status != TI_OK)
            		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "error on mlme_setParam ()");
            }
            else {
            	TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "error on rsn_setExtAuthMode ()");
            }

        }
        break;

#ifdef XCC_MODULE_INCLUDED
    case RSN_XCC_NETWORK_EAP:
        {
            OS_XCC_NETWORK_EAP      networkEap = 0;

            rsn_getNetworkEap (pRsn, &networkEap);
            if (networkEap != pParam->content.networkEap)
            {
                TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_XCC_NETWORK_EAP networkEap  %d \n", pParam->content.networkEap);

                status = rsn_setNetworkEap (pRsn, pParam->content.networkEap);
                if (status == TI_OK)
                {
                    /*status = RE_SCAN_NEEDED;*/
                }
            }
        }
        break;
#endif
    case RSN_MIXED_MODE:
        {
            status = TI_OK;
            pRsn->bMixedMode = pParam->content.rsnMixedMode;

            break;
        }

    case RSN_PMKID_LIST:
    	TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_PMKID_LIST \n");

    	TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8*)&pParam->content.rsnPMKIDList ,sizeof(OS_802_11_PMKID));
    	status = rsn_setPMKIDList (pRsn, &pParam->content.rsnPMKIDList);

    	if(status == TI_OK)
    	{
    		TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_PMKID_LIST:   %d PMKID entries has been added to the cache.\n", pParam->content.rsnPMKIDList.BSSIDInfoCount);
    	}
    	else
    	{
    		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set RSN_PMKID_LIST failure");
    	}
    	break;

    case RSN_SET_KEY_PARAM:
	    {
	    	TSecurityKeys *pSecurityKey = pParam->content.pRsnKey;
	    	TI_UINT32     keyIndex;

	    	TRACE2(pRsn->hReport,REPORT_SEVERITY_INFORMATION,"RSN:Set RSN_SET_KEY_PARAM KeyIndex %x,keyLength=%d\n",pSecurityKey->keyIndex,pSecurityKey->encLen);
	    	keyIndex = (TI_UINT8)pSecurityKey->keyIndex;

	    	if(keyIndex >= MAX_KEYS_NUM)
	    	{
	    		return TI_NOK;
	    	}

	    	/* Remove the key when the length is 0, or the type is not set */
	    	if ( (pSecurityKey->keyType == KEY_NULL) ||
	    			(pSecurityKey->encLen == 0))
	    	{
	    		/* Clearing a key */
	    		status = rsn_removeKey( pRsn, pSecurityKey );
	    	}
	    	else
	    		status = rsn_setKey (pRsn, pSecurityKey);  /* send key to FW*/
	    }
	    break;

    case RSN_PORT_STATUS_PARAM:
	    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set Port Status %d \n", pParam->content.rsnPortStatus);
	    status = rsn_setPortStatus( hRsn, pParam->content.rsnPortStatus );
	    break;

    case RSN_GENERIC_IE_PARAM:
	    TRACE4(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: Set Generic IE: length=%d, IE=%02x%02x%02x... \n",
		   pParam->content.rsnGenericIE.length,
	       pParam->content.rsnGenericIE.data[0], pParam->content.rsnGenericIE.data[1],pParam->content.rsnGenericIE.data[2] );

	    status = TI_OK;

	    /* make sure it's a valid IE: datal-ength > 2 AND a matching length field */
	    if ((pParam->content.rsnGenericIE.length > 2) &&
		((pParam->content.rsnGenericIE.data[1] + 2) == pParam->content.rsnGenericIE.length)) {
		    /* Setting the IE */
		    pRsn->genericIE.length = pParam->content.rsnGenericIE.length;
		    os_memoryCopy(pRsn->hOs,(void*)pRsn->genericIE.data, (TI_UINT8*)pParam->content.rsnGenericIE.data, pParam->content.rsnGenericIE.length);
	    } else if ( pParam->content.rsnGenericIE.length == 0 ) {
		    /* Deleting the IE */
		    pRsn->genericIE.length = pParam->content.rsnGenericIE.length;
	    } else {
		    TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, "RSN: Set Generic IE: FAILED sanity checks \n" );
		    status = TI_NOK;
	    }
	    break;

    default:
        return TI_NOK;
    }

    return status;
}


/**
*
* rsn_parseIe  - Parse a required information element.
*
* \b Description:
*
* Parse an Aironet information element.
* Builds a structure of all the capabilities described in the Aironet IE.
* We look at Flags field only to determine KP and MIC bits value
*
* \b ARGS:
*
*  I   - pRsn - pointer to admCtrl context
*  I   - pAironetIe - pointer to Aironet IE buffer  \n
*  O   - pAironetData - capabilities structure
*
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_parseIe(rsn_t *pRsn, TRsnData *pRsnData, TI_UINT8 **pIe, TI_UINT8 IeId)
{

    dot11_eleHdr_t      *eleHdr;
    TI_INT16             length;
    TI_UINT8            *pCurIe;


    *pIe = NULL;

    if ((pRsnData == NULL) || (pRsnData->ieLen==0))
    {
       return TI_OK;
    }

    pCurIe = pRsnData->pIe;

    length = pRsnData->ieLen;
    while (length>0)
    {
        eleHdr = (dot11_eleHdr_t*)pCurIe;

        if (length<((*eleHdr)[1] + 2))
        {
            TRACE2(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "admCtrl_parseIe ERROR: pRsnData->ieLen=%d, length=%d\n\n", pRsnData->ieLen,length);
            return TI_OK;
        }

        if ((*eleHdr)[0] == IeId)
        {
            *pIe = (TI_UINT8*)eleHdr;
            break;
        }
        length -= (*eleHdr)[1] + 2;
        pCurIe += (*eleHdr)[1] + 2;
    }
    return TI_OK;
}


/**
*
* rsn_eventRecv - Set a specific parameter to the rsniation SM
*
* \b Description:
*
* Set a specific parameter to the rsniation SM.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*  I/O - pParam - Parameter \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Start, rsn_Stop
*/
TI_STATUS rsn_reportStatus (rsn_t *pRsn, TI_STATUS rsnStatus)
{
    TI_STATUS           status = TI_OK;
    paramInfo_t         param;

    if (pRsn == NULL)
    {
        return TI_NOK;
    }

    if (rsnStatus == TI_OK)
    {
    	if (pRsn->unicastSuite == TWD_CIPHER_NONE)
    		param.content.txDataCurrentPrivacyInvokedMode = TI_FALSE;
    	else
    		param.content.txDataCurrentPrivacyInvokedMode = TI_TRUE;

		txCtrlParams_setCurrentPrivacyInvokedMode(pRsn->hTxCtrl, param.content.txDataCurrentPrivacyInvokedMode);
        /* The value of exclude unencrypted should be as privacy invoked */
        param.paramType = RX_DATA_EXCLUDE_UNENCRYPTED_PARAM;
        rxData_setParam (pRsn->hRx, &param);

        param.paramType = RX_DATA_EXCLUDE_BROADCAST_UNENCRYPTED_PARAM;
        if (pRsn->bMixedMode)
        {   /* do not exclude Broadcast packets */
            param.content.txDataCurrentPrivacyInvokedMode = TI_FALSE;
        }
        rxData_setParam (pRsn->hRx, &param);
    } else {
        rsnStatus = (TI_STATUS)STATUS_SECURITY_FAILURE;
    }

    status = conn_reportRsnStatus (pRsn->hConn, (mgmtStatus_e)rsnStatus);

    if (status!=TI_OK)
    {
        return status;
    }

    if (rsnStatus == TI_OK)
    {
        EvHandlerSendEvent (pRsn->hEvHandler, IPC_EVENT_AUTH_SUCC, NULL, 0);
    }

    /* for wpa2, encrypt eapols after port is open */
	if (   (pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2) ||
			(pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2PSK))
		txCtrlParams_setEapolEncryptionStatus(pRsn->hTxCtrl, TI_TRUE);



    TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_reportStatus \n");

    return TI_OK;
}



/**
*
* rsn_eventRecv - Set a specific parameter to the rsniation SM
*
* \b Description:
*
* Set a specific parameter to the rsniation SM.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*  I/O - pParam - Parameter \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa rsn_Start, rsn_Stop
*/
TI_STATUS rsn_getNetworkMode(rsn_t *pRsn, ERsnNetworkMode *pNetMode)
{
    paramInfo_t     param;
    TI_STATUS       status;

    param.paramType = CTRL_DATA_CURRENT_BSS_TYPE_PARAM;

    status =  ctrlData_getParam (pRsn->hCtrlData, &param);

    if (status == TI_OK)
    {
        if (param.content.ctrlDataCurrentBssType == BSS_INFRASTRUCTURE)
        {
            *pNetMode = RSN_INFRASTRUCTURE;
        }
        else
        {
            *pNetMode = RSN_IBSS;
        }
    }
    else
    {
        return TI_NOK;
    }

    return TI_OK;
}



/**
*
* rsn_getInfoElement -
*
* \b Description:
*
* Get the RSN information element.
*
* \b ARGS:
*
*  I   - hRsn - Rsn SM context  \n
*  I/O - pRsnIe - Pointer to the return information element \n
*  I/O - pRsnIeLen - Pointer to the returned IE's length \n
*
*  this function is redundant now that we're using only GenIE. consider refactoring out
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa
*/
TI_STATUS rsn_getInfoElement(TI_HANDLE hRsn, TI_UINT8 *pRsnIe, TI_UINT32 *pRsnIeLen)
{
    rsn_t       *pRsn;
    TI_STATUS   status;

    if ( (NULL == hRsn) || (NULL == pRsnIe) || (NULL == pRsnIeLen) )
    {
        return TI_NOK;
    }

    pRsn = (rsn_t*)hRsn;

    status = rsn_getGenInfoElement(hRsn, pRsnIe, pRsnIeLen);

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getInfoElement: rsn IE length = %d\n", *pRsnIeLen);
    TRACE_INFO_HEX(pRsn->hReport, pRsnIe, *pRsnIeLen);
    return status;

}


#ifdef XCC_MODULE_INCLUDED
/**
*
* rsn_getXCCExtendedInfoElement -
*
* \b Description:
*
* Get the Aironet information element.
*
* \b ARGS:
*
*  I   - hRsn - Rsn SM context  \n
*  I/O - pRsnIe - Pointer to the return information element \n
*  I/O - pRsnIeLen - Pointer to the returned IE's length \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa
*/
TI_STATUS rsn_getXCCExtendedInfoElement(TI_HANDLE hRsn, TI_UINT8 *pRsnIe, TI_UINT8 *pRsnIeLen)
{
    rsn_t       *pRsn;
    TI_STATUS   status;

    if ( (NULL == hRsn) || (NULL == pRsnIe) || (NULL == pRsnIeLen) )
    {
        return TI_NOK;
    }

    pRsn = (rsn_t*)hRsn;

    status = admCtrlXCC_getInfoElement (pRsn, pRsnIe, pRsnIeLen);

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getXCCExtendedInfoElement pRsnIeLen= %d\n",*pRsnIeLen);

    return status;
}


void rsn_XCCSetSite(rsn_t *pRsn, TRsnData *pRsnData, TI_UINT8 *pAssocIe) {
	paramInfo_t         *pParam;

	pParam = (paramInfo_t *)os_memoryAlloc(pRsn->hOs, sizeof(paramInfo_t));
	if (!pParam) {
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_XCCSetSite: malloc failed!");
		return;
	}

	if ((pRsn->externalAuthMode != RSN_EXT_AUTH_MODE_WPA2) &&
			(pRsn->externalAuthMode != RSN_EXT_AUTH_MODE_WPA))
    {
        pParam->paramType = XCC_CCKM_EXISTS;
        pParam->content.XCCCckmExists  = TI_FALSE;
        XCCMngr_setParam(pRsn->hXCCMngr, pParam);
    }
    else
    {
        admCtrlXCC_setExtendedParams(pRsn, pRsnData);

        pParam->paramType = XCC_CCKM_EXISTS;
        pParam->content.XCCCckmExists  = TI_TRUE;
        XCCMngr_setParam(pRsn->hXCCMngr, pParam);
    }

	os_memoryFree(pRsn->hOs, pParam, sizeof(paramInfo_t));
}

#endif



TI_STATUS rsn_setKey (rsn_t *pRsn, TSecurityKeys *pKey)
{
	TTwdParamInfo       tTwdParam;
	TI_STATUS           status = TI_OK;
	TI_BOOL				bDefaultKey = TI_FALSE;

	if ((pRsn == NULL) || (pKey == NULL))
	{
		return TI_NOK;
	}
	/* check whether default key and strip flags from key index */
	bDefaultKey = pKey->keyIndex & RSN_WEP_KEY_TRANSMIT_MASK;
	pKey->keyIndex = (TI_UINT8) pKey->keyIndex;

	if (pKey->keyIndex >= MAX_KEYS_NUM) {
		return TI_NOK;
	}


	if (pKey->keyType == KEY_NULL) {
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_setKey, pKey->keyType == KEY_NULL");
		return TI_OK;
	}

	/* set this only for unicast */
	if (!MAC_BROADCAST(pKey->macAddress) || (pKey->keyType == KEY_WEP))
	{
		tTwdParam.paramType = TWD_RSN_SECURITY_MODE_PARAM_ID;

		switch (pKey->keyType) {
		case KEY_TKIP:
			tTwdParam.content.rsnEncryptionStatus = (ECipherSuite)TWD_CIPHER_TKIP;
			break;
		case KEY_AES:
			tTwdParam.content.rsnEncryptionStatus = (ECipherSuite)TWD_CIPHER_AES_CCMP;
			break;
#ifdef GEM_SUPPORTED
		case KEY_GEM:
			tTwdParam.content.rsnEncryptionStatus = (ECipherSuite)TWD_CIPHER_GEM;
			status = rsn_setUcastSuite(pRsn, TWD_CIPHER_GEM);
			status = rsn_setBcastSuite(pRsn, TWD_CIPHER_GEM);
			break;
#endif
		case KEY_WEP:
			tTwdParam.content.rsnEncryptionStatus = (ECipherSuite)TWD_CIPHER_WEP;
			break;
		case KEY_NULL:
		case KEY_XCC:
		default:
			tTwdParam.content.rsnEncryptionStatus = (ECipherSuite)TWD_CIPHER_NONE;
			break;
		}
		status = TWD_SetParam(pRsn->hTWD, &tTwdParam);
		if ( status != TI_OK ) {
			return status;
		}
	}

	/* set the size to reserve for encryption to the tx */
	/* update this parameter only in accordance with pairwise key setting */
	if (!MAC_BROADCAST(pKey->macAddress))
	{

		/* set the size to reserve for encryption to the tx */
		switch (pKey->keyType)
		{
		case KEY_TKIP:
			txCtrlParams_setEncryptionFieldSizes (pRsn->hTxCtrl, IV_FIELD_SIZE);
			break;
		case KEY_AES:
			txCtrlParams_setEncryptionFieldSizes (pRsn->hTxCtrl, AES_AFTER_HEADER_FIELD_SIZE);
			break;
#ifdef GEM_SUPPORTED
		case KEY_GEM:
#endif
		case KEY_WEP:
		case KEY_NULL:
		case KEY_XCC:
		default:
			txCtrlParams_setEncryptionFieldSizes (pRsn->hTxCtrl, 0);
			break;
		}
	}

	if (MAC_BROADCAST(pKey->macAddress))
	{
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_setKey, Group ReKey timer started\n");
		tmr_StopTimer (pRsn->hMicFailureGroupReKeyTimer);
		tmr_StartTimer (pRsn->hMicFailureGroupReKeyTimer,
				rsn_groupReKeyTimeout,
				(TI_HANDLE)pRsn,
				RSN_MIC_FAILURE_RE_KEY_TIMEOUT,
				TI_FALSE);
		pRsn->eGroupKeyUpdate = GROUP_KEY_UPDATE_TRUE;
	}
	else
	{
		if (pRsn->bPairwiseMicFailureFilter)	/* the value of this flag is taken from registry */
		{
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_setKey, Pairwise ReKey timer started\n");
			tmr_StopTimer (pRsn->hMicFailurePairwiseReKeyTimer);
			tmr_StartTimer (pRsn->hMicFailurePairwiseReKeyTimer,
					rsn_pairwiseReKeyTimeout,
					(TI_HANDLE)pRsn,
					RSN_MIC_FAILURE_RE_KEY_TIMEOUT,
					TI_FALSE);
			pRsn->ePairwiseKeyUpdate = PAIRWISE_KEY_UPDATE_TRUE;
		}
	}

	tTwdParam.paramType = TWD_RSN_KEY_ADD_PARAM_ID;
	tTwdParam.content.configureCmdCBParams.pCb = (TI_UINT8*) pKey;
	tTwdParam.content.configureCmdCBParams.fCb = NULL;
	tTwdParam.content.configureCmdCBParams.hCb = NULL;
	status = TWD_SetParam (pRsn->hTWD, &tTwdParam);
	TRACE3(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_setKey, KeyType=%d, KeyId = 0x%lx,encLen=0x%x\n", pKey->keyType,pKey->keyIndex, pKey->encLen);

	TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "\nEncKey = ");

	TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8 *)pKey->encKey, pKey->encLen);

	if (pKey->keyType != KEY_WEP)
	{
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "\nMac address = ");
		TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8 *)pKey->macAddress, MAC_ADDR_LEN);
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "\nRSC = ");
		TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8 *)pKey->keyRsc, KEY_RSC_LEN);
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "\nMic RX = ");
		TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8 *)pKey->micRxKey, MAX_KEY_LEN);
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "\nMic TX = ");
		TRACE_INFO_HEX(pRsn->hReport, (TI_UINT8 *)pKey->micTxKey, MAX_KEY_LEN);
	}

	if ((pKey->keyType == KEY_WEP) && (bDefaultKey))	{
		status = rsn_setDefaultKeyId(pRsn, pKey->keyIndex);
		if (status!=TI_OK)
		{
			return status;
		}
	}

	/* for wpa, encrypt eapols after unicast key is set */
	if (   ((pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA) ||
			(pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPAPSK)) && !MAC_BROADCAST(pKey->macAddress))
		txCtrlParams_setEapolEncryptionStatus(pRsn->hTxCtrl, TI_TRUE);

	return status;
}


TI_STATUS rsn_removeKey (rsn_t *pRsn, TSecurityKeys *pKey)
{
    TI_STATUS           status = TI_OK;
    TTwdParamInfo       tTwdParam;
    TI_UINT8               keyIndex;

	if (pRsn == NULL || pKey == NULL)
	{
		return TI_NOK;
	}
    keyIndex = (TI_UINT8)pKey->keyIndex;

    if (keyIndex >= MAX_KEYS_NUM)
    {
        return TI_NOK;
    }

    TRACE2(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_removeKey Entry, keyType=%d, keyIndex=0x%lx\n",pKey->keyType, keyIndex);

    tTwdParam.paramType = TWD_RSN_KEY_REMOVE_PARAM_ID;
    /*os_memoryCopy(pRsn->hOs, &tTwdParam.content.rsnKey, pKey, sizeof(TSecurityKeys));*/
    tTwdParam.content.configureCmdCBParams.pCb = (TI_UINT8*) pKey;
    tTwdParam.content.configureCmdCBParams.fCb = NULL;
    tTwdParam.content.configureCmdCBParams.hCb = NULL;

    status = TWD_SetParam (pRsn->hTWD, &tTwdParam);

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_removeKey in whal, status =%d\n", status);

    return status;
}


TI_STATUS rsn_setDefaultKeyId(rsn_t *pRsn, TI_UINT8 keyId)
{
    TI_STATUS               status = TI_OK;
    TTwdParamInfo           tTwdParam;

    if (pRsn == NULL)
    {
        return TI_NOK;
    }
    /* Now we configure default key ID to the HAL */
    tTwdParam.paramType = TWD_RSN_DEFAULT_KEY_ID_PARAM_ID;
    tTwdParam.content.configureCmdCBParams.pCb = &keyId;
    tTwdParam.content.configureCmdCBParams.fCb = NULL;
    tTwdParam.content.configureCmdCBParams.hCb = NULL;
    status = TWD_SetParam (pRsn->hTWD, &tTwdParam);

    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN: rsn_setDefaultKeyId, KeyId = 0x%lx\n", keyId);
    return status;
}


TI_STATUS rsn_reportAuthFailure(TI_HANDLE hRsn, EAuthStatus authStatus)
{
	TI_STATUS    status = TI_OK;
	rsn_t       *pRsn;
	paramInfo_t param;

	if (hRsn==NULL)
	{
		return TI_NOK;
	}

	pRsn = (rsn_t*)hRsn;

	/* Remove AP from candidate list for a specified amount of time */
	param.paramType = CTRL_DATA_CURRENT_BSSID_PARAM;
	status = ctrlData_getParam(pRsn->hCtrlData, &param);
	if (status != TI_OK)
	{
		TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, "rsn_reportAuthFailure, unable to retrieve BSSID \n");
	}
	else
	{
		TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "current station is banned from the roaming candidates list for %d Ms\n", RSN_AUTH_FAILURE_TIMEOUT);

		rsn_banSite(hRsn, param.content.ctrlDataCurrentBSSID, RSN_SITE_BAN_LEVEL_FULL, RSN_AUTH_FAILURE_TIMEOUT);
	}


#ifdef XCC_MODULE_INCLUDED
	TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "CALLING rougeAP, status= %d \n",authStatus);
	status = XCCMngr_rogueApDetected (pRsn->hXCCMngr, authStatus);
#endif
	TI_VOIDCAST(pRsn);
	return status;
}

/******
This is the CB function for mic failure event from the FW
 *******/
TI_STATUS rsn_reportMicFailure(TI_HANDLE hRsn, TI_UINT8 *pType, TI_UINT32 Length)
{
	rsn_t                               *pRsn = (rsn_t *) hRsn;
	ERsnSiteBanLevel                    banLevel;
	OS_802_11_AUTHENTICATION_REQUEST    *request;
	TI_UINT8 AuthBuf[sizeof(TI_UINT32) + sizeof(OS_802_11_AUTHENTICATION_REQUEST)];
	paramInfo_t                         param;
	TI_UINT8                               failureType;

	failureType = *pType;

	if (((pRsn->unicastSuite == TWD_CIPHER_TKIP) && (failureType == KEY_TKIP_MIC_PAIRWISE)) ||
			(((pRsn->broadcastSuite == TWD_CIPHER_TKIP)||(pRsn->broadcastSuite == TWD_CIPHER_AES_CCMP))
					&& (failureType == KEY_TKIP_MIC_GROUP)))
	{
		/* check if the MIC failure is group and group key update */
		/* was performed during the last 3 seconds */
		if ((failureType == KEY_TKIP_MIC_GROUP) &&
				(pRsn->eGroupKeyUpdate == GROUP_KEY_UPDATE_TRUE))
		{
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Group MIC failure ignored, key update was performed within the last 3 seconds.\n");
			return TI_OK;
		}

		/* check if the MIC failure is pairwise and pairwise key update */
		/* was performed during the last 3 seconds */
		if ((failureType == KEY_TKIP_MIC_PAIRWISE) &&
				(pRsn->ePairwiseKeyUpdate == PAIRWISE_KEY_UPDATE_TRUE))
		{
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Pairwise MIC failure ignored, key update was performed within the last 3 seconds.\n");
			return TI_OK;
		}

		/* Prepare the Authentication Request */
		request = (OS_802_11_AUTHENTICATION_REQUEST *)(AuthBuf + sizeof(TI_UINT32));
		request->Length = sizeof(OS_802_11_AUTHENTICATION_REQUEST);

		param.paramType = CTRL_DATA_CURRENT_BSSID_PARAM;
		if (ctrlData_getParam (pRsn->hCtrlData, &param) != TI_OK)
		{
			return TI_NOK;
		}

		/* Generate 802 Media specific indication event */
		*(TI_UINT32*)AuthBuf = os802_11StatusType_Authentication;

		MAC_COPY (request->BSSID, param.content.ctrlDataCurrentBSSID);

		if (failureType == KEY_TKIP_MIC_PAIRWISE)
		{
			request->Flags = OS_802_11_REQUEST_PAIRWISE_ERROR;
		}
		else
		{
			request->Flags = OS_802_11_REQUEST_GROUP_ERROR;
		}

		EvHandlerSendEvent (pRsn->hEvHandler,
				IPC_EVENT_MEDIA_SPECIFIC,
				(TI_UINT8*)AuthBuf,
				sizeof(TI_UINT32) + sizeof(OS_802_11_AUTHENTICATION_REQUEST));

		/* Update and check the ban level to decide what actions need to take place */
		banLevel = rsn_banSite (hRsn, param.content.ctrlDataCurrentBSSID, RSN_SITE_BAN_LEVEL_HALF, RSN_MIC_FAILURE_TIMEOUT);
		if (banLevel == RSN_SITE_BAN_LEVEL_FULL)
		{
			/* Site is banned so prepare to disconnect */
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Second MIC failure, closing Rx port...\n");

			param.paramType = RX_DATA_PORT_STATUS_PARAM;
			param.content.rxDataPortStatus = CLOSE;
			rxData_setParam(pRsn->hRx, &param);

			/* stop the mic failure Report timer and start a new one for 0.5 seconds */
			tmr_StopTimer (pRsn->hMicFailureReportWaitTimer);
			apConn_setDeauthPacketReasonCode(pRsn->hAPConn, STATUS_MIC_FAILURE);
			tmr_StartTimer (pRsn->hMicFailureReportWaitTimer,
					rsn_micFailureReportTimeout,
					(TI_HANDLE)pRsn,
					RSN_MIC_FAILURE_REPORT_TIMEOUT,
					TI_FALSE);
		}
		else
		{
			/* Site is only half banned so nothing needs to be done for now */
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": First MIC failure, business as usual for now...\n");
		}
	}

	return TI_OK;
}



void rsn_groupReKeyTimeout(TI_HANDLE hRsn, TI_BOOL bTwdInitOccured)
{
    rsn_t *pRsn;

    pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return;
    }

    pRsn->eGroupKeyUpdate = GROUP_KEY_UPDATE_FALSE;
}


void rsn_pairwiseReKeyTimeout(TI_HANDLE hRsn, TI_BOOL bTwdInitOccured)
{
    rsn_t *pRsn;

    pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return;
    }

    pRsn->ePairwiseKeyUpdate = PAIRWISE_KEY_UPDATE_FALSE;
}


void rsn_micFailureReportTimeout (TI_HANDLE hRsn, TI_BOOL bTwdInitOccured)
{
    rsn_t *pRsn;

    pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return;
    }

    TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": MIC failure reported, disassociating...\n");

    apConn_reportRoamingEvent (pRsn->hAPConn, ROAMING_TRIGGER_SECURITY_ATTACK, NULL);
}


void rsn_debugFunc(TI_HANDLE hRsn)
{
    rsn_t *pRsn = (rsn_t*)hRsn;

    if (pRsn == NULL)
    {
        return;
    }

}


/*-----------------------------------------------------------------------------
Routine Name: rsn_preAuthTimerExpire
Routine Description: updates the preAuthStatus
Arguments:
Return Value:
-----------------------------------------------------------------------------*/
void rsn_preAuthTimerExpire(TI_HANDLE handle, TI_BOOL bTwdInitOccured)
{
	rsn_t *pRsn = (rsn_t *)handle;
	TRACE0(pRsn->hReport, REPORT_SEVERITY_WARNING , "rsn_preAuthTimerExpire: PREAUTH EXPIRED !!!!!!!!");
	/* Send PRE-AUTH end event to External Application */
	rsn_notifyPreAuthStatus (pRsn, RSN_PRE_AUTH_END);
	pRsn->numberOfPreAuthCandidates = 0;
	return;
}


TI_UINT32  rsn_parseSuiteVal(rsn_t *pRsn, TI_UINT8* suiteVal, TI_UINT32 maxVal, TI_UINT32 unknownVal)
{
    TI_UINT32  suite;

    if ((pRsn==NULL) || (suiteVal==NULL))
    {
        return TWD_CIPHER_UNKNOWN;
    }
    if (!os_memoryCompare(pRsn->hOs, suiteVal, wpa2IeOuiIe, 3))
    {
        suite =  (ECipherSuite)((suiteVal[3]<=maxVal) ? suiteVal[3] : unknownVal);
    } else
    {
        suite = unknownVal;
    }
    return  suite;

}


/**
*
* rsn_parseRsnIe  - Parse an WPA information element.
*
* \b Description:
*
* Parse an WPA information element.
* Builds a structure of the unicast adn broadcast cihper suites,
* the key management suite and the capabilities.
*
* \b ARGS:
*
*  I   - pRsn - pointer to admCtrl context
*  I   - pWpa2Ie  - pointer to WPA IE (RSN IE) buffer  \n
*  O   - pWpa2Data - WPA2 IE (RSN IE) structure after parsing
*
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/
TI_STATUS rsn_parseRsnIe(rsn_t *pRsn, TI_UINT8 *pWpa2Ie, wpa2IeData_t *pWpa2Data)
{
    dot11_RSN_t      *wpa2Ie       =  (dot11_RSN_t *)pWpa2Ie;
    TI_UINT16            temp2bytes =0, capabilities;
    TI_UINT8             dataOffset = 0, i = 0, j = 0, curKeyMngSuite = 0;
    ECipherSuite     curCipherSuite = TWD_CIPHER_NONE;

    TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "Wpa2_IE: DEBUG: admCtrlWpa2_parseIe\n\n");

    if ((pWpa2Data == NULL) || (pWpa2Ie == NULL))
    {
        return TI_NOK;
    }

    COPY_WLAN_WORD(&temp2bytes, wpa2Ie->rsnIeData);
    dataOffset += 2;

    /* Check the header fields and the version */
    if((wpa2Ie->hdr[0] != RSN_IE_ID) || (wpa2Ie->hdr[1] < WPA2_IE_MIN_LENGTH) ||
       (temp2bytes != WPA2_OUI_MAX_VERSION))
    {
        TRACE3(pRsn->hReport, REPORT_SEVERITY_ERROR, "Wpa2_ParseIe Error: length=0x%x, elementid=0x%x, version=0x%x\n", wpa2Ie->hdr[1], wpa2Ie->hdr[0], temp2bytes);

        return TI_NOK;
    }


    /* Set default values */
    os_memoryZero(pRsn->hOs, pWpa2Data, sizeof(wpa2IeData_t));

    pWpa2Data->broadcastSuite = TWD_CIPHER_AES_CCMP;
    pWpa2Data->unicastSuiteCnt = 1;
    pWpa2Data->unicastSuite[0] = TWD_CIPHER_AES_CCMP;
    pWpa2Data->KeyMngSuiteCnt = 1;
    pWpa2Data->KeyMngSuite[0] = WPA2_IE_KEY_MNG_801_1X;

    /* If we've reached the end of the received RSN IE */
    if(wpa2Ie->hdr[1] < WPA2_IE_GROUP_SUITE_LENGTH)
        return TI_OK;

    /* Processing of Group Suite field - 4 bytes*/
    pWpa2Data->broadcastSuite = (ECipherSuite)rsn_parseSuiteVal(pRsn, (TI_UINT8 *)wpa2Ie->rsnIeData + dataOffset,
                                                          TWD_CIPHER_WEP104, TWD_CIPHER_UNKNOWN);
    dataOffset +=4;
    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "Wpa2_IE: GroupSuite %x \n", pWpa2Data->broadcastSuite);


    /* Processing of Pairwise (Unicast) Cipher Suite - 2 bytes counter and list of 4-byte entries */
    if(wpa2Ie->hdr[1] < WPA2_IE_MIN_PAIRWISE_SUITE_LENGTH)
        return TI_OK;

    COPY_WLAN_WORD(&pWpa2Data->unicastSuiteCnt, wpa2Ie->rsnIeData + dataOffset);
    dataOffset += 2;

    if(pWpa2Data->unicastSuiteCnt > MAX_WPA2_UNICAST_SUITES)
    {
        /* something wrong in the RSN IE */
        TRACE1(pRsn->hReport, REPORT_SEVERITY_ERROR, "Wpa2_ParseIe Error: Pairwise cipher suite count is  %d \n", pWpa2Data->unicastSuiteCnt);
        return TI_NOK;
    }

    /* Get unicast cipher suites */
    for(i = 0; i < pWpa2Data->unicastSuiteCnt; i++)
    {
        curCipherSuite = (ECipherSuite)rsn_parseSuiteVal(pRsn, (TI_UINT8 *)wpa2Ie->rsnIeData + dataOffset,
                                                   TWD_CIPHER_WEP104, TWD_CIPHER_UNKNOWN);
        if(curCipherSuite == TWD_CIPHER_NONE)
            curCipherSuite = pWpa2Data->broadcastSuite;

        pWpa2Data->unicastSuite[i] = curCipherSuite;
        dataOffset +=4;

        TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "Wpa_IE: unicast suite %x \n", curCipherSuite);
    }

    /* Sort all the unicast suites supported by the AP in the decreasing order */
    /* (so the best cipher suite will be the first)                            */
    if(pWpa2Data->unicastSuiteCnt > 1)
    {
       for(i = 0; i < (pWpa2Data->unicastSuiteCnt -1); i ++)
       {
           for(j = 0; j < i; j ++)
           {
               if(pWpa2Data->unicastSuite[j] > pWpa2Data->unicastSuite[j + 1])
               {
                   curCipherSuite               = pWpa2Data->unicastSuite[j];
                   pWpa2Data->unicastSuite[j]   = pWpa2Data->unicastSuite[j+1];
                   pWpa2Data->unicastSuite[j+1] = curCipherSuite;
               }
           }
       }
    }

    /* If we've reached the end of the received RSN IE */
    if (wpa2Ie->hdr[1] == dataOffset)
        return TI_OK;

     /* KeyMng Suite */
    COPY_WLAN_WORD(&(pWpa2Data->KeyMngSuiteCnt), wpa2Ie->rsnIeData + dataOffset);

     dataOffset += 2;
     // pRsn->wpaAkmExists = TI_FALSE;

    if(pWpa2Data->KeyMngSuiteCnt > MAX_WPA2_KEY_MNG_SUITES)
    {
        /* something wrong in the RSN IE */
        TRACE1(pRsn->hReport, REPORT_SEVERITY_ERROR, "Wpa2_ParseIe Error: Management cipher suite count is  %d \n", pWpa2Data->KeyMngSuiteCnt);
        return TI_NOK;
    }

     for(i = 0; i < pWpa2Data->KeyMngSuiteCnt; i++)
     {
#ifdef XCC_MODULE_INCLUDED
            curKeyMngSuite = admCtrlXCC_parseCckmSuiteVal4Wpa2(pRsn, (TI_UINT8 *)(wpa2Ie->rsnIeData + dataOffset));
            if (curKeyMngSuite == WPA2_IE_KEY_MNG_CCKM)
            {  /* CCKM is the maximum AKM */
                pWpa2Data->KeyMngSuite[i] = curKeyMngSuite;
            }
            else
#endif
            {
                curKeyMngSuite = rsn_parseSuiteVal(pRsn, (TI_UINT8 *)wpa2Ie->rsnIeData + dataOffset,
                            WPA2_IE_KEY_MNG_PSK_801_1X, WPA2_IE_KEY_MNG_NA);
            }


        TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "Wpa2_IE: authKeyMng %x  \n", curKeyMngSuite);

         if ((curKeyMngSuite != WPA2_IE_KEY_MNG_NA) &&
             (curKeyMngSuite != WPA2_IE_KEY_MNG_CCKM))
         {
             pWpa2Data->KeyMngSuite[i] = curKeyMngSuite;
         }

//         if (curKeyMngSuite==WPA2_IE_KEY_MNG_801_1X)
//         {   /* If 2 AKM exist, save also the second priority */
//             pRsn->wpaAkmExists = TI_TRUE;
//         }

         dataOffset += 4;

		 /* Include all AP key management supported suites in the wpaData structure */
         /* pWpa2Data->KeyMngSuite[i+1] = curKeyMngSuite;*/

     }

    /* If we've reached the end of the received RSN IE */
    if (wpa2Ie->hdr[1] == dataOffset)
        return TI_OK;

    /* Parse capabilities */
    COPY_WLAN_WORD(&capabilities, wpa2Ie->rsnIeData + dataOffset);
    pWpa2Data->bcastForUnicatst  = (TI_UINT8)(capabilities & WPA2_GROUP_4_UNICAST_CAPABILITY_MASK)>>
                                           WPA2_GROUP_4_UNICAST_CAPABILITY_SHIFT;
    pWpa2Data->ptkReplayCounters = (TI_UINT8)(capabilities &  WPA2_PTK_REPLAY_COUNTERS_CAPABILITY_MASK)>>
                                           WPA2_PTK_REPLAY_COUNTERS_CAPABILITY_SHIFT;

    switch (pWpa2Data->ptkReplayCounters)
    {
    case 0: pWpa2Data->ptkReplayCounters=1;
            break;
    case 1: pWpa2Data->ptkReplayCounters=2;
            break;
    case 2: pWpa2Data->ptkReplayCounters=4;
            break;
    case 3: pWpa2Data->ptkReplayCounters=16;
            break;
    default: pWpa2Data->ptkReplayCounters=1;
            break;
   }
   pWpa2Data->gtkReplayCounters = (TI_UINT8)(capabilities &
                                        WPA2_GTK_REPLAY_COUNTERS_CAPABILITY_MASK) >>
                                        WPA2_GTK_REPLAY_COUNTERS_CAPABILITY_SHIFT;
   switch (pWpa2Data->gtkReplayCounters)
   {
   case 0: pWpa2Data->gtkReplayCounters=1;
            break;
   case 1: pWpa2Data->gtkReplayCounters=2;
            break;
   case 2: pWpa2Data->gtkReplayCounters=4;
            break;
   case 3: pWpa2Data->gtkReplayCounters=16;
            break;
   default: pWpa2Data->gtkReplayCounters=1;
            break;
   }

   pWpa2Data->preAuthentication = (TI_UINT8)(capabilities & WPA2_PRE_AUTH_CAPABILITY_MASK);

   dataOffset += 2;

   pWpa2Data->lengthWithoutPmkid = dataOffset + 2; /* +2 for IE header */

   TRACE6(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "Wpa2_IE: capabilities %x, preAuthentication = %x, bcastForUnicatst %x, ptk = %x, gtk = %x, lengthWithoutPmkid = %d\n", capabilities, pWpa2Data->preAuthentication, pWpa2Data->bcastForUnicatst, pWpa2Data->ptkReplayCounters, pWpa2Data->gtkReplayCounters, pWpa2Data->lengthWithoutPmkid);


   return TI_OK;
}


/**
*
* rsn_buildAndSendPMKIDCandList
*
* \b Description:
*
* New Candidate List of APs with the same SSID as the STA is connected to
* is generated and sent after the delay to the supplicant
* in order to retrieve the new PMKIDs for the APs.
*
* \b ARGS:
*  I   - pRsn - pointer to rsn context
*
* \b RETURNS:
*
* TI_OK on success, TI_NOK on failure.
*
* \sa
*/

static void rsn_buildAndSendPMKIDCandList (TI_HANDLE hHandle, TBssidList4PreAuth *apList)
{
    rsn_t             *pRsn = (rsn_t*)hHandle;
    TI_UINT8          candIndex =0, apIndex = 0, size =0;
    paramInfo_t       *pParam;
    OS_802_11_PMKID_CANDIDATELIST  *pCandList;
    TI_UINT8             memBuff[PMKID_CAND_LIST_MEMBUFF_SIZE + sizeof(TI_UINT32)];
    dot11_RSN_t       *rsnIE = 0;
    wpa2IeData_t      wpa2Data;
    TI_STATUS         status = TI_NOK;

    pParam = (paramInfo_t *)os_memoryAlloc(pRsn->hOs, sizeof(paramInfo_t));
    if (!pParam)
    {
        return;
    }

    /* Get SSID that the STA is accociated with    */
    pParam->paramType = SME_DESIRED_SSID_ACT_PARAM;
    status          = sme_GetParam (pRsn->hSmeSm, pParam);
    if(status != TI_OK) {
        os_memoryFree(pRsn->hOs, pParam, sizeof(paramInfo_t));
        return;
    }

    /* If the existing PMKID cache contains information for not relevant */
    /* ssid (i.e. ssid was changed), clean up the PMKID cache and update */
    /* the ssid in the PMKID cache */
    if ((pRsn->pmkid_cache.ssid.len != pParam->content.smeDesiredSSID.len) ||
         (os_memoryCompare(pRsn->hOs, (TI_UINT8 *)pRsn->pmkid_cache.ssid.str,
          (TI_UINT8 *) pParam->content.smeDesiredSSID.str,
          pRsn->pmkid_cache.ssid.len) != 0))
    {
        rsn_resetPMKIDCache(pRsn);

        os_memoryCopy(pRsn->hOs, (void *)pRsn->pmkid_cache.ssid.str,
                      (void *)pParam->content.smeDesiredSSID.str,
                      pParam->content.siteMgrCurrentSSID.len);
        pRsn->pmkid_cache.ssid.len = pParam->content.smeDesiredSSID.len;
    }

    /* Get list of APs of the SSID that the STA is associated with*/
    /*os_memoryZero(pAdmCtrl->hOs, (void*)&apList, sizeof(bssidListBySsid_t));
    status = siteMgr_GetApListBySsid (pAdmCtrl->pRsn->hSiteMgr,
                                      &param.content.siteMgrCurrentSSID,
                                      &apList);
    */
    os_memoryFree(pRsn->hOs, pParam, sizeof(paramInfo_t));
    if((apList == NULL) || (apList->NumOfItems == 0))
        return;

    TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_buildAndSendPMKIDCandList - Entry \n");

    /* fill the PMKID candidate list */
    pCandList = (OS_802_11_PMKID_CANDIDATELIST *)(memBuff + sizeof(TI_UINT32));
    pCandList->Version = 1;
    for (apIndex=0; apIndex<pRsn->pmkid_cache.entriesNumber; apIndex++)
    {
    	pRsn->pmkid_cache.pmkidTbl[apIndex].preAuthenticate = TI_FALSE;
    }

    /* Go over AP list and find APs supporting pre-authentication */
    for(apIndex = 0; apIndex < apList->NumOfItems && candIndex < PMKID_MAX_NUMBER; apIndex++)
    {
        TI_UINT8 *bssidMac, i = 0;

        status = TI_NOK;

        if (apList->bssidList[apIndex].pRsnIEs==NULL)
        {
            continue;
        }
        /* Check is there RSN IE in this site */
        rsnIE = 0;
        while( !rsnIE && (i < MAX_RSN_IE))
        {
            if(apList->bssidList[apIndex].pRsnIEs[i].hdr[0] == RSN_IE_ID)
            {
                rsnIE  = &apList->bssidList[apIndex].pRsnIEs[i];
                status = TI_OK;
            }
            i ++;
        }
		if (rsnIE)
		{
			TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_buildAndSendPMKIDCandList - rsnIE-hdr.eleId = %x \n", rsnIE->hdr[0]);
		}

		if(status == TI_OK)
           status = rsn_parseRsnIe(pRsn, (TI_UINT8 *)rsnIE, &wpa2Data);

        TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_buildAndSendPMKIDCandList - parseIe status = %d \n", status);
        if(status == TI_OK)
        {
            TI_BOOL        preAuthStatus;
            TI_UINT8               cacheIndex;

            preAuthStatus = rsn_getPreAuthStatus(pRsn, &apList->bssidList[apIndex].bssId, &cacheIndex);

            TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_buildAndSendPMKIDCandList, preAuthStatus=%d \n", preAuthStatus);

            if (preAuthStatus)
            {
            	pRsn->pmkid_cache.pmkidTbl[cacheIndex].preAuthenticate = TI_TRUE;
            }

            bssidMac = (TI_UINT8 *)apList->bssidList[apIndex].bssId;
            MAC_COPY (pCandList->CandidateList[candIndex].BSSID, bssidMac);

            pCandList->CandidateList[candIndex].Flags = OS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLE;
            if(pRsn->preAuthSupport && (wpa2Data.preAuthentication))
            {
               pCandList->CandidateList[candIndex].Flags =
                                 OS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLE;
               TRACE8(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  Candidate [%d] is   %.2X-%.2X-%.2X-%.2X-%.2X-%.2X , Flags=0x%x\n", candIndex, bssidMac[0], bssidMac[1], bssidMac[2], bssidMac[3], bssidMac[4], bssidMac[5], pCandList->CandidateList[candIndex].Flags);

            }
            else
            {
                pCandList->CandidateList[candIndex].Flags = 0;
                TRACE8(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  Candidate [%d] [DOESN'T SUPPORT PREAUTH] is   %.2X-%.2X-%.2X-%.2X-%.2X-%.2X , Flags=0x%x\n", candIndex, bssidMac[0], bssidMac[1], bssidMac[2], bssidMac[3], bssidMac[4], bssidMac[5], pCandList->CandidateList[candIndex].Flags);
            }

            candIndex ++;
        }

    }
    /* Add candidates that have valid PMKID, but were not in the list */
    for (apIndex=0; apIndex<pRsn->pmkid_cache.entriesNumber && candIndex < PMKID_MAX_NUMBER; apIndex++)
    {
        if (!pRsn->pmkid_cache.pmkidTbl[apIndex].preAuthenticate)
        {
            MAC_COPY (pCandList->CandidateList[candIndex].BSSID,
                      pRsn->pmkid_cache.pmkidTbl[apIndex].bssId);
            pCandList->CandidateList[apIndex].Flags =
                OS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLE;
            candIndex++;
        }
    }


    pCandList->NumCandidates = candIndex;


    /* Send Status Media specific indication to OS */
    size = sizeof(OS_802_11_PMKID_CANDIDATELIST) +
           (candIndex - 1) * sizeof(OS_802_11_PMKID_CANDIDATE) + sizeof(TI_UINT32);

     /* Fill type of indication */
    *(TI_UINT32*)memBuff = os802_11StatusType_PMKID_CandidateList;

    pCandList->NumCandidates = candIndex;

    /* Store the number of candidates sent - needed for pre-auth finish event */
    pRsn->numberOfPreAuthCandidates = candIndex;
    /* Start the pre-authentication finish event timer */
    /* If the pre-authentication process is not over by the time it expires - we send an event */
    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION , "Starting PREAUTH timer (%d mSec)\n",pRsn->preAuthTimeout*candIndex);
    tmr_StartTimer (pRsn->hPreAuthTimerWpa2,
                    rsn_preAuthTimerExpire,
                    (TI_HANDLE)pRsn,
                    pRsn->preAuthTimeout * candIndex,
                    TI_FALSE);

    EvHandlerSendEvent(pRsn->hEvHandler, IPC_EVENT_MEDIA_SPECIFIC,
                        memBuff, size);

    /* Send PRE-AUTH start event to External Application */
    rsn_notifyPreAuthStatus (pRsn, RSN_PRE_AUTH_START);
    TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "RSN:  PMKID Candidate List with %d entries has been built and sent for ssid  \n", candIndex);


    return;

}

/**
*
* rsn_startPreAuth -
*
* \b Description:
*
* Start pre-authentication on a list of given BSSIDs.
*
* \b ARGS:
*
*  I   - hRsn - Rsniation SM context  \n
*  I/O - pBssidList - list of BSSIDs that require Pre-Auth \n
*
* \b RETURNS:
*
*  TI_OK if successful, TI_NOK otherwise.
*
* \sa
*/
TI_STATUS rsn_startPreAuth(TI_HANDLE hRsn, TBssidList4PreAuth *pBssidList)
{
    rsn_t       *pRsn;

    if ( (NULL == hRsn) || (NULL == pBssidList) )
    {
        return TI_NOK;
    }

    pRsn = (rsn_t*)hRsn;

    if ((pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2) ||
    		(pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2PSK)) {

    	TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_startPreAuth \n");
    	rsn_buildAndSendPMKIDCandList(pRsn, pBssidList);
    }

    return TI_OK;
}




/**
 *
 * isSiteBanned -
 *
 * \b Description:
 *
 * Returns whether or not the site with the specified Bssid is banned or not.
 *
 * \b ARGS:
 *
 *  I   - hRsn - RSN module context \n
 *  I   - siteBssid - The desired site's bssid \n
 *
 * \b RETURNS:
 *
 *  TI_NOK iff site is banned.
 *
 */
TI_BOOL rsn_isSiteBanned(TI_HANDLE hRsn, TMacAddr siteBssid)
{
    rsn_t * pRsn = (rsn_t *) hRsn;
    rsn_siteBanEntry_t * entry;

    /* Check if site is in the list */
    if ((entry = findBannedSiteAndCleanup(hRsn, siteBssid)) == NULL)
    {
        return TI_FALSE;
    }

    TRACE7(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Site %02X-%02X-%02X-%02X-%02X-%02X found with ban level %d...\n", siteBssid[0], siteBssid[1], siteBssid[2], siteBssid[3], siteBssid[4], siteBssid[5], entry->banLevel);

    return (entry->banLevel == RSN_SITE_BAN_LEVEL_FULL);
}


/**
 *
 * rsn_PortStatus_Set API implementation-
 *
 * \b Description:
 *
 * set the status port according to the status flag
 *
 * \b ARGS:
 *
 *  I   - hRsn - RSN module context \n
 *  I   - state - The status flag \n
 *
 * \b RETURNS:
 *
 *  TI_STATUS.
 *
 */
TI_STATUS rsn_setPortStatus(TI_HANDLE hRsn, TI_BOOL state)
{
    rsn_t                   *pRsn = (rsn_t *)hRsn;

    pRsn->bPortStatus = state;

    if (state)
    	rsn_reportStatus( pRsn, TI_OK );
    return TI_OK;
}


/**
 *
 * rsn_banSite -
 *
 * \b Description:
 *
 * Bans the specified site from being associated to for the specified duration.
 * If a ban level of WARNING is given and no previous ban was in effect the
 * warning is marked down but other than that nothing happens. In case a previous
 * warning (or ban of course) is still in effect
 *
 * \b ARGS:
 *
 *  I   - hRsn - RSN module context \n
 *  I   - siteBssid - The desired site's bssid \n
 *  I   - banLevel - The desired level of ban (Warning / Ban)
 *  I   - durationMs - The duration of ban in milliseconds
 *
 * \b RETURNS:
 *
 *  The level of ban (warning / banned).
 *
 */
ERsnSiteBanLevel rsn_banSite(TI_HANDLE hRsn, TMacAddr siteBssid, ERsnSiteBanLevel banLevel, TI_UINT32 durationMs)
{
    rsn_t * pRsn = (rsn_t *) hRsn;
    rsn_siteBanEntry_t * entry;

    /* Try finding the site in the list */
    if ((entry = findBannedSiteAndCleanup(hRsn, siteBssid)) != NULL)
    {
        /* Site found so a previous ban is still in effect */
        TRACE6(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Site %02X-%02X-%02X-%02X-%02X-%02X found and has been set to ban level full!\n", siteBssid[0], siteBssid[1], siteBssid[2], siteBssid[3], siteBssid[4], siteBssid[5]);

        entry->banLevel = RSN_SITE_BAN_LEVEL_FULL;
    }
    else
    {
        /* Site doesn't appear in the list, so find a place to insert it */
        TRACE7(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Site %02X-%02X-%02X-%02X-%02X-%02X added with ban level %d!\n", siteBssid[0], siteBssid[1], siteBssid[2], siteBssid[3], siteBssid[4], siteBssid[5], banLevel);

        entry = findEntryForInsert (hRsn);

		MAC_COPY (entry->siteBssid, siteBssid);
        entry->banLevel = banLevel;

        pRsn->numOfBannedSites++;
    }

    entry->banStartedMs = os_timeStampMs (pRsn->hOs);
    entry->banDurationMs = durationMs;

    return entry->banLevel;
}


/**
 *
 * findEntryForInsert -
 *
 * \b Description:
 *
 * Returns a place to insert a new banned site.
 *
 * \b ARGS:
 *
 *  I   - hRsn - RSN module context \n
 *
 * \b RETURNS:
 *
 *  A pointer to a suitable site entry.
 *
 */
static rsn_siteBanEntry_t * findEntryForInsert(TI_HANDLE hRsn)
{
    rsn_t * pRsn = (rsn_t *) hRsn;

    /* In the extreme case that the list is full we overwrite an old entry */
    if (pRsn->numOfBannedSites == RSN_MAX_NUMBER_OF_BANNED_SITES)
    {
        TRACE0(pRsn->hReport, REPORT_SEVERITY_ERROR, ": No room left to insert new banned site, overwriting old one!\n");

        return &(pRsn->bannedSites[0]);
    }

    return &(pRsn->bannedSites[pRsn->numOfBannedSites]);
}


/**
 *
 * findBannedSiteAndCleanup -
 *
 * \b Description:
 *
 * Searches the banned sites list for the desired site while cleaning up
 * expired sites found along the way.
 *
 * Note that this function might change the structure of the banned sites
 * list so old iterators into the list might be invalidated.
 *
 * \b ARGS:
 *
 *  I   - hRsn - RSN module context \n
 *  I   - siteBssid - The desired site's bssid \n
 *
 * \b RETURNS:
 *
 *  A pointer to the desired site's entry if found,
 *  NULL otherwise.
 *
 */
static rsn_siteBanEntry_t * findBannedSiteAndCleanup(TI_HANDLE hRsn, TMacAddr siteBssid)
{
    rsn_t * pRsn = (rsn_t *) hRsn;
    int iter;

    for (iter = 0; iter < pRsn->numOfBannedSites; iter++)
    {
        /* If this entry has expired we'd like to clean it up */
        if (os_timeStampMs(pRsn->hOs) - pRsn->bannedSites[iter].banStartedMs >= pRsn->bannedSites[iter].banDurationMs)
        {
            TRACE1(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Found expired entry at index %d, cleaning it up...\n", iter);

            /* Replace this entry with the last one */
            pRsn->bannedSites[iter] = pRsn->bannedSites[pRsn->numOfBannedSites - 1];
            pRsn->numOfBannedSites--;

            /* we now repeat the iteration on this entry */
            iter--;

            continue;
        }

        /* Is this the entry for the site we're looking for? */
        if (MAC_EQUAL (siteBssid, pRsn->bannedSites[iter].siteBssid))
        {
            TRACE7(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Site %02X-%02X-%02X-%02X-%02X-%02X found at index %d!\n", siteBssid[0], siteBssid[1], siteBssid[2], siteBssid[3], siteBssid[4], siteBssid[5], iter);

            return &pRsn->bannedSites[iter];
        }
    }

    /* Entry not found... */
    TRACE6(pRsn->hReport, REPORT_SEVERITY_INFORMATION, ": Site %02X-%02X-%02X-%02X-%02X-%02X not found...\n", siteBssid[0], siteBssid[1], siteBssid[2], siteBssid[3], siteBssid[4], siteBssid[5]);

    return NULL;
}

/**
 *
 * rsn_getPortStatus -
 *
 * \b Description:
 *
 * Returns the extrenalSec port status
 *
 * \b ARGS:
 *
 *  pRsn - pointer to RSN module context \n
 *
 * \b RETURNS:
 *
 *  TI_BOOL - the port status True = Open , False = Close
 *
 */
TI_BOOL rsn_getPortStatus(rsn_t *pRsn)
{
	return pRsn->bPortStatus;
}

/**
 *
 * rsn_getGenInfoElement -
 *
 * \b Description:
 *
 * Copies the Generic IE to a given buffer
 *
 * \b ARGS:
 *
 *  I     pRsn           - pointer to RSN module context \n
 *  O     out_buff       - pointer to the output buffer  \n
 *  O     out_buf_length - length of data copied into output buffer \n
 *
 * \b RETURNS:
 *
 *  TI_UINT8 - The amount of bytes copied.
 *
 */
TI_STATUS rsn_getGenInfoElement(rsn_t *pRsn, TI_UINT8 *out_buff, TI_UINT32 *out_buf_length)
{
	wpa2IeData_t       wpa2data;
	TMacAddr           pBssid;
    pmkidValue_t       pmkId = {};
    TI_UINT8           index;
    TI_STATUS          status;

	if ( !(pRsn && out_buff && out_buf_length) ) {
			return TI_NOK;
	}

	*out_buf_length = 0;

	if (pRsn->genericIE.length == 0)
    {
		return TI_OK;
    }

	if(pRsn->preAuthSupport &&
			((pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2) ||
			(pRsn->externalAuthMode == RSN_EXT_AUTH_MODE_WPA2PSK))) {

		ctrlData_getParamBssid(pRsn->hCtrlData, CTRL_DATA_CURRENT_BSSID_PARAM, pBssid);
		status = rsn_findPMKID(pRsn, &pBssid, &pmkId, &index);

		rsn_parseRsnIe(pRsn, pRsn->genericIE.data, &wpa2data);

		if(status == TI_OK)
		{
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getInfoElement - PMKID was found! \n");

			os_memoryCopy(pRsn->hOs, out_buff, pRsn->genericIE.data, wpa2data.lengthWithoutPmkid);

			SET_WLAN_WORD(out_buff+wpa2data.lengthWithoutPmkid,ENDIAN_HANDLE_WORD(1));
			os_memoryCopy(pRsn->hOs, out_buff+wpa2data.lengthWithoutPmkid+2,
					(TI_UINT8 *)pmkId, PMKID_VALUE_SIZE);
			*out_buf_length = wpa2data.lengthWithoutPmkid+2+PMKID_VALUE_SIZE;
			out_buff[1] = *out_buf_length - 2;
			TRACE2(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getInfoElement - original IE len %d, new IE len %d\n", pRsn->genericIE.length, *out_buf_length);
		}
		else { /* PMKID wasn't found */
			TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getInfoElement - PMKID was NOT found! \n");
			os_memoryCopy(pRsn->hOs, out_buff, pRsn->genericIE.data, pRsn->genericIE.length);
			*out_buf_length = pRsn->genericIE.length;
		}
	}
	else {
		TRACE0(pRsn->hReport, REPORT_SEVERITY_INFORMATION, "rsn_getInfoElement - not WPA2 \n");
		os_memoryCopy(pRsn->hOs, out_buff, pRsn->genericIE.data, pRsn->genericIE.length);
		*out_buf_length = pRsn->genericIE.length;
	}
	//

	return TI_OK;
}


#ifdef RSN_NOT_USED

static TI_INT16 convertAscii2Unicode(TI_INT8* userPwd, TI_INT16 len)
{
    TI_INT16 i;
    TI_INT8 unsiiPwd[MAX_PASSWD_LEN];


    for (i=0; i<len; i++)
    {
        unsiiPwd[i] = userPwd[i];
    }
    for (i=0; i<len; i++)
    {
        userPwd[i*2] = unsiiPwd[i];
        userPwd[i*2+1] = 0;
    }
    return (TI_INT16)(len*2);
}

#endif

/***************************************************************************
*							rsn_reAuth				                   *
****************************************************************************
* DESCRIPTION:	This is a callback function called by the whalWPA module whenever
*				a broadcast TKIP key was configured to the FW.
*				It does the following:
*					-	resets the ReAuth flag
*					-	stops the ReAuth timer
*					-	restore the PS state
*					-	Send RE_AUTH_COMPLETED event to the upper layer.
*
* INPUTS:		hRsn - the object
*
* OUTPUT:		None
*
* RETURNS:		None
*
***************************************************************************/
//void rsn_reAuth(TI_HANDLE hRsn)
//{
//	rsn_t *pRsn;
//
//	pRsn = (rsn_t*)hRsn;
//
//	if (pRsn == NULL)
//	{
//		return;
//	}
//
//	if (rxData_IsReAuthInProgress(pRsn->hRx))
//	{
//		rxData_SetReAuthInProgress(pRsn->hRx, TI_FALSE);
//		rxData_StopReAuthActiveTimer(pRsn->hRx);
//		rxData_ReauthDisablePriority(pRsn->hRx);
//		EvHandlerSendEvent(pRsn->hEvHandler, IPC_EVENT_RE_AUTH_COMPLETED, NULL, 0);
//	}
//}
