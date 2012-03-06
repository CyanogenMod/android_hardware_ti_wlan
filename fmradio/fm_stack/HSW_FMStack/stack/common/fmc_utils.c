/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB
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
*   FILE NAME:      fmc_utils.c
*
*   DESCRIPTION:
*
*
*   AUTHOR:        
*
\*******************************************************************************/


#include "mcp_hal_string.h"
#include "fmc_types.h"
#include "fmc_defs.h"
#include "fmc_utils.h"
#include "fmc_log.h"
#include "fmc_os.h"

FMC_LOG_SET_MODULE(FMC_LOG_MODULE_FMUTILS);

#if FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED


/* Define The maximum representation of FMC_UINT in ASCII*/
#define MAX_FMC_UINT_SIZE_IN_ASCII    10
FMC_STATIC  const char	invalidFmcTempStr[] = "formatMsg is too long";

/* [ToDo] Calculate real mapping values */
static const FmcFwCmdParmsValue _fmcUtilsApi2FwPowerLevelMap[FM_TX_POWER_LEVEL_MAX - FM_TX_POWER_LEVEL_MIN + 1] = 
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30 , 31
    
};


static pthread_mutex_t list_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

/*
    Initially, the head is initialized to point to itself in both directions => empty list
*/
void FMC_InitializeListHead(FMC_ListNode *head)
{
    pthread_mutex_lock(&list_mutex);
    head->NextNode = head;
    head->PrevNode = head;
    pthread_mutex_unlock(&list_mutex);
}

/*
    Initialize the node's next / prev pointers to NULL => Not on any list
*/
void FMC_InitializeListNode(FMC_ListNode *node)
{
    pthread_mutex_lock(&list_mutex);
    node->NextNode = 0;
    node->PrevNode = 0;
    pthread_mutex_unlock(&list_mutex);
}

FMC_ListNode *FMC_GetHeadList(FMC_ListNode *head)
{
    /* The first element is the one "next" to the head */
    return head->NextNode;
}

FMC_BOOL FMC_IsNodeConnected(const FMC_ListNode *node)
{
    pthread_mutex_lock(&list_mutex);
    /* A node is connected if the the adjacent nodes point at it correctly */
    if ((node->PrevNode->NextNode == node) && (node->NextNode->PrevNode == node))
    {
        pthread_mutex_unlock(&list_mutex);
        return FMC_TRUE;
    }
    else
    {
        pthread_mutex_unlock(&list_mutex);
        return FMC_FALSE;
    }
}

FMC_BOOL FMC_IsListEmpty(const FMC_ListNode *head)
{
    /* A list is empty if the head's next pointer points at the head (as ti was initialized) */
    pthread_mutex_lock(&list_mutex);
    if (FMC_GetHeadList((FMC_ListNode*)head) == head)
    {
        pthread_mutex_unlock(&list_mutex);
        return FMC_TRUE;
    }
    else
    {
        pthread_mutex_unlock(&list_mutex);
        return FMC_FALSE;
    }
}

void FMC_InsertTailList(FMC_ListNode* head, FMC_ListNode* Node)
{
    FMC_FUNC_START("FMC_InsertTailList");

    pthread_mutex_lock(&list_mutex);

    /* set the new node's links */
    Node->NextNode = head;
    Node->PrevNode = head->PrevNode;

    /* Make current last node one before last */
    head->PrevNode->NextNode = Node;

    /* Head's prev should point to last element*/
    head->PrevNode = Node;
    
    pthread_mutex_unlock(&list_mutex);

    FMC_VERIFY_FATAL_NO_RETVAR((FMC_IsNodeConnected(Node) == FMC_TRUE), ("FMC_InsertTailList: Node is not connected"));

    FMC_FUNC_END();
}

void FMC_InsertHeadList(FMC_ListNode* head, FMC_ListNode* Node)
{
    FMC_FUNC_START("FMC_InsertHeadList");
    
    pthread_mutex_lock(&list_mutex);

    /* set the new node's links */
    Node->NextNode = head->NextNode;
    Node->PrevNode = head;
    
    /* Make current first node second */
    head->NextNode->PrevNode = Node;
    
    /* Head's next should point to first element*/
    head->NextNode = Node;

    pthread_mutex_unlock(&list_mutex);

    FMC_VERIFY_FATAL_NO_RETVAR((FMC_IsNodeConnected(Node) == FMC_TRUE), ("FMC_InsertHeadList: Node is not connected"));

    FMC_FUNC_END();
}


FMC_ListNode*FMC_RemoveHeadList(FMC_ListNode* head)
{
    FMC_ListNode* first = NULL;

    FMC_FUNC_START("FMC_RemoveHeadList");
    
    pthread_mutex_lock(&list_mutex);

    first = head->NextNode;
    first->NextNode->PrevNode = head;
    head->NextNode = first->NextNode;

    pthread_mutex_unlock(&list_mutex);

    FMC_VERIFY_FATAL_SET_RETVAR((FMC_IsListCircular(head) == FMC_TRUE), (first = NULL), ("_FMC_RemoveHeadList: List is not circular"));

    FMC_FUNC_END();
    
    return first;
}

void FMC_RemoveNodeList(FMC_ListNode* Node)
{
    FMC_FUNC_START("FMC_RemoveNodeList");
    
    FMC_VERIFY_FATAL_NO_RETVAR((FMC_IsListCircular(Node) == FMC_TRUE), ("FMC_IsNodeOnList: List is not circular"));
    
    pthread_mutex_lock(&list_mutex);

    Node->PrevNode->NextNode = Node->NextNode;
    Node->NextNode->PrevNode = Node->PrevNode;
    
    pthread_mutex_unlock(&list_mutex);

    FMC_VERIFY_FATAL_NO_RETVAR((FMC_IsListCircular(Node->PrevNode) == FMC_TRUE), ("FMC_RemoveNodeList: List is not circular"));

    FMC_InitializeListNode(Node);

    FMC_FUNC_END();
}

FmcListNodeIndex FMC_GetNodeIndex(const FMC_ListNode* head, const FMC_ListNode* node)
{
    FmcListNodeIndex        index;      /* Holds the result */
    FMC_BOOL            nodeOnList; /* Indicates if node was found or not */
    FMC_ListNode*       tmpNode;

    FMC_FUNC_START("FMC_GetNodeIndex");
    
    FMC_VERIFY_FATAL_SET_RETVAR((FMC_IsListCircular(head) == FMC_TRUE), (index = FMC_LIST_NODE_INVALID_INDEX), 
                                    ("FMC_GetNodeIndex: List is not circular"));

    pthread_mutex_lock(&list_mutex);

    /* Start the search from the first node */
    tmpNode = FMC_GetHeadList((FMC_ListNode*)head);
    nodeOnList = FMC_FALSE;

    index = 0;

    /* Traverse list until we find the node or exhaust the list */
    while (tmpNode != head)
    {       
        if (tmpNode == node)
        {
            /* node found  */
            nodeOnList = FMC_TRUE;
            break;
        }

        /* Move to next node */
        ++index;
        tmpNode = tmpNode->NextNode;
    }

    /* Handle the node-not-found case */
    if (nodeOnList == FMC_FALSE)
    {
        index = FMC_LIST_NODE_INVALID_INDEX;
    }
    
    pthread_mutex_unlock(&list_mutex);
    FMC_FUNC_END();

    return index;
}

FMC_BOOL FMC_IsNodeOnList(const FMC_ListNode* head, const FMC_ListNode* node)
{
    FmcListNodeIndex index = FMC_GetNodeIndex(head, node);

    if (index != FMC_LIST_NODE_INVALID_INDEX)
    {
        return FMC_TRUE;
    }
    else
    {
        return FMC_FALSE;
    }
}


void FMC_MoveList(FMC_ListNode* dest, FMC_ListNode* src)
{
    FMC_FUNC_START("FMC_MoveList");
    
    FMC_VERIFY_FATAL_NO_RETVAR((FMC_IsListCircular(src) == FMC_TRUE), ("FMC_MoveList: List is not circular"));

    pthread_mutex_lock(&list_mutex);

    /* New head points at list*/
    dest->NextNode = src->NextNode;
    dest->PrevNode = src->PrevNode;

    /* Detach source head from list */
    src->NextNode->PrevNode = dest;
    src->PrevNode->NextNode = dest;

    FMC_InitializeListHead(src);
    pthread_mutex_unlock(&list_mutex);

    FMC_FUNC_END();
}

FMC_BOOL FMC_IsListCircular(const FMC_ListNode* list)
{
    const FMC_ListNode* tmp;

    pthread_mutex_lock(&list_mutex);

    tmp = list;
    if (FMC_IsNodeConnected(list) == FMC_FALSE)
    {
        pthread_mutex_unlock(&list_mutex);
        return FMC_FALSE;
    }

    for (tmp = tmp->NextNode; tmp != list; tmp = tmp->NextNode)
    {
        if (FMC_IsNodeConnected(tmp) == FMC_FALSE)
        {
            pthread_mutex_unlock(&list_mutex);
            return FMC_FALSE;
        }
    }
    
    pthread_mutex_unlock(&list_mutex);
    return FMC_TRUE;
}
FMC_ListNode *_FMC_UTILS_FindMatchingListNodeAndLoc(    FMC_ListNode                *listHead, 
                                                        FMC_ListNode                *prevNode,
                                                        FmcUtilsSearchDir           searchDir,
                                                        const FMC_ListNode      *nodeToMatch, 
                                                        FmcUtilsListComparisonFunc  comparisonFunc,
                                                        FMC_U32             *location)
{
    FMC_ListNode        *checkedNode = NULL;
    FMC_ListNode        *matchingNode = NULL;
    FMC_U32         tempLoc=0;
    FMC_FUNC_START("FMC_UTILS_FindMatchingListEntry");
    
    FMC_VERIFY_FATAL_SET_RETVAR((NULL != listHead), (matchingNode = NULL), ("Null list argument"));
    FMC_VERIFY_FATAL_SET_RETVAR((NULL != nodeToMatch), (matchingNode = NULL), ("Null nodeToMatch argument"));
    FMC_VERIFY_FATAL_SET_RETVAR((NULL != comparisonFunc), (matchingNode = NULL), ("Null comparisonFunc argument"));

    /* Assume a matching entry will not be found */
    matchingNode = NULL;

    /* Decide from which node to start the search */
    
    pthread_mutex_lock(&list_mutex);

    if (prevNode == NULL)
    {
        checkedNode = listHead;
    }
    else
    {
        checkedNode = prevNode;
    }
        
    if (searchDir == FMC_UTILS_SEARCH_FIRST_TO_LAST)
    {
        checkedNode = checkedNode->NextNode;
    }
    else
    {
        checkedNode = checkedNode->PrevNode;
    }

    /* Check all list entries, until a match is found, or list exhausted */
    while (checkedNode != listHead)
    {
        tempLoc ++;
        /* Check the current entry, using the specified comparison function */
        
        if (FMC_TRUE == comparisonFunc(nodeToMatch, checkedNode))
        {
            matchingNode = checkedNode;

            /* entry matches, stop comparing */
            break;
        }

        /* Move on to check the next entry, according to the search direction */
        if (searchDir == FMC_UTILS_SEARCH_FIRST_TO_LAST)
        {
            checkedNode = checkedNode->NextNode;
        }
        else
        {
            checkedNode = checkedNode->PrevNode;
        }
    }
    *location = tempLoc;

    pthread_mutex_unlock(&list_mutex);

    FMC_FUNC_END();
    
    return matchingNode;
}

FMC_BOOL FMC_UTILS_FindAndCompareLocationMatchingListNodes( FMC_ListNode                *listHead, 
                                                        const FMC_ListNode      *firstNodeToMatch, 
                                                        const FMC_ListNode      *secondNodeToMatch, 
                                                        FmcUtilsListComparisonFunc  comparisonFunc)
{
    FMC_U32 locationOfFirst, locationOfSecond;
    _FMC_UTILS_FindMatchingListNodeAndLoc(listHead,
                                            NULL,
                                            FMC_UTILS_SEARCH_LAST_TO_FIRST,
                                            firstNodeToMatch,
                                            comparisonFunc,
                                            &locationOfFirst);
    _FMC_UTILS_FindMatchingListNodeAndLoc(listHead,
                                            NULL,
                                            FMC_UTILS_SEARCH_LAST_TO_FIRST,
                                            secondNodeToMatch,
                                            comparisonFunc,
                                            &locationOfSecond);
    return (locationOfFirst>locationOfSecond);



}

FMC_ListNode *FMC_UTILS_FindMatchingListNode(   FMC_ListNode                *listHead, 
                                                        FMC_ListNode                *prevNode,
                                                        FmcUtilsSearchDir           searchDir,
                                                        const FMC_ListNode      *nodeToMatch, 
                                                        FmcUtilsListComparisonFunc  comparisonFunc)
{
    FMC_U32     location;
    return _FMC_UTILS_FindMatchingListNodeAndLoc(listHead,
                                            prevNode,
                                            searchDir,
                                            nodeToMatch,
                                            comparisonFunc,
                                            &location);
}

const char * FMC_UTILS_FormatNumber(const char *formatMsg, FMC_UINT number, char *tempStr,FMC_UINT maxTempStrSize)
{
    FMC_U16 formatMsgLen;
    formatMsgLen=MCP_HAL_STRING_StrLen(formatMsg);

    /*Verify that the tempStr allocation is enough  */
    if ((FMC_U16)(formatMsgLen+MAX_FMC_UINT_SIZE_IN_ASCII)>maxTempStrSize)
        {            
            return invalidFmcTempStr;
        }
    

    MCP_HAL_STRING_Sprintf (tempStr, formatMsg, number);

    return tempStr;
}

FMC_BOOL FMC_UTILS_IsFreqValid( FmcFreq freq)
{
    /* Verify that the frequency is within the band's limits*/

    if ((freq < FMC_FIRST_FREQ_JAPAN_KHZ) || (freq > FMC_LAST_FREQ_US_EUROPE_KHZ))
    {
        return FMC_FALSE;
    }
    if((freq%10)!=0)
    {
        return FMC_FALSE;
    }
    return FMC_TRUE;
}

FMC_BOOL FMC_UTILS_IsAfCodeValid( FmcAfCode afCode)
{
    /* Verify that the afCode is within the range limits*/

    if (((afCode <= 204) &&(afCode >=1 ))||(afCode==FMC_AF_CODE_NO_AF_AVAILABLE))
    {
        return FMC_TRUE;
    }
 
        return FMC_FALSE;
}

FMC_UINT FMC_UTILS_ChannelSpacingInKhz(FmcChannelSpacing channelSpacing)
{
    FMC_UINT    channelSpacingInKhz;
    
    FMC_FUNC_START("FMC_UTILS_ChannelSpacingInKhz");
    
    switch (channelSpacing)
    {
        case FMC_CHANNEL_SPACING_50_KHZ:
            
            channelSpacingInKhz = 50;

        break;
        
        case FMC_CHANNEL_SPACING_100_KHZ:
            
            channelSpacingInKhz = 100;

        break;
        
        case FMC_CHANNEL_SPACING_200_KHZ:

            channelSpacingInKhz = 200;

        break;
        
        default:
            FMC_FATAL_SET_RETVAR((channelSpacingInKhz = 50), 
                                    ("FMC_UTILS_ChannelSpacingInKhz: Invalid Channel Spacing (%d)", channelSpacing));
    }

    FMC_FUNC_END();

    return channelSpacingInKhz;
}

/*
    The formula to calculate the firmware index is:

    index = (frequency - band start frequency) / channel spacing

    The number varies from 0 - 410 (108000 for European band)

    AF - is supported only in EUROPE
*/
    FmcFwCmdParmsValue  FMC_UTILS_FreqToFwChannelIndex(FmcBand band ,FmcChannelSpacing channelSpacing,FmcFreq freq)
{
    FmcFwCmdParmsValue  index;
    FmcBand fistChannelByBand = FMC_FIRST_FREQ_US_EUROPE_KHZ;

    if(band == FMC_BAND_JAPAN)
        fistChannelByBand = FMC_FIRST_FREQ_JAPAN_KHZ;
    index = (FmcFwCmdParmsValue)((freq - fistChannelByBand) / FMC_UTILS_ChannelSpacingInKhz(channelSpacing));

    return index;
}

FmcFreq FMC_UTILS_FwChannelIndexToFreq(FmcBand band ,FmcChannelSpacing channelSpacing,FMC_U16 channelIndex)
{
    FmcBand fistChannelByBand = FMC_FIRST_FREQ_US_EUROPE_KHZ;
    
    if(band == FMC_BAND_JAPAN)
        fistChannelByBand = FMC_FIRST_FREQ_JAPAN_KHZ;
    return ((FmcFreq)(channelIndex*FMC_UTILS_ChannelSpacingInKhz(channelSpacing))+fistChannelByBand);
}

FmcFwCmdParmsValue FMC_UTILS_Api2FwPowerLevel(FmTxPowerLevel apiPowerLevel)
{
    /* Get the value from the table (values are generated offline) */
    return _fmcUtilsApi2FwPowerLevelMap[apiPowerLevel];
}
FMC_BOOL FMC_IsNodeAvailable(FMC_ListNode *node)
{
    return((node)->NextNode == 0);
    
}

FMC_U16 FMC_UTILS_BEtoHost16(FMC_U8 * num)
{
    return (FMC_U16)(((FMC_U16) *(num) << 8) | ((FMC_U16) *(num+1)));
}
FMC_U16 FMC_UTILS_LEtoHost16(FMC_U8 * num)
{
    return (FMC_U16)(((FMC_U16) *(num+1) << 8) | ((FMC_U16) *(num)));
}
void FMC_UTILS_StoreBE16(FMC_U8 *buff, FMC_U16 be_value) 
{
   buff[0] = (FMC_U8)(be_value>>8);
   buff[1] = (FMC_U8)be_value;
}
void FMC_UTILS_StoreLE16(FMC_U8 *buff, FMC_U16 le_value) 
{
   buff[1] = (FMC_U8)(le_value>>8);
   buff[0] = (FMC_U8)le_value;
}

void FMC_UTILS_StoreBE32(FMC_U8 *buff, FMC_U32 be_value) 
{
   buff[0] = (FMC_U8)(be_value>>24);
   buff[1] = (FMC_U8)(be_value>>16);
   buff[2] = (FMC_U8)(be_value>>8);
   buff[3] = (FMC_U8)be_value;
}
FmcCoreEventType FMC_UTILS_ConvertVacEvent2FmcEvent(ECCM_VAC_Event eEvent)
{
    switch(eEvent)
    {
    case CCM_VAC_EVENT_OPERATION_STARTED:
        return FMC_VAC_EVENT_OPERATION_STARTED;
    case CCM_VAC_EVENT_OPERATION_STOPPED:
        return FMC_VAC_EVENT_OPERATION_STOPPED;
    case CCM_VAC_EVENT_RESOURCE_CHANGED:
        return FMC_VAC_EVENT_RESOURCE_CHANGED;
    default:
        return FMC_VAC_EVENT_CONFIGURATION_CHANGED;

    }
}

FmcStatus FMC_UTILS_ConvertVacStatus2FmcStatus(ECCM_VAC_Status eStatus)
{
    switch(eStatus)
    {
    case CCM_VAC_STATUS_SUCCESS:
        return FMC_STATUS_SUCCESS;
    case CCM_VAC_STATUS_PENDING:
        return FMC_STATUS_PENDING;
    case CCM_VAC_STATUS_FAILURE_UNSPECIFIED:
        return FMC_STATUS_FAILED;
    case CCM_VAC_STATUS_FAILURE_UNAVAILABLE_RESOURCES:
        return FMC_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES;
    default:
        return FMC_STATUS_FAILED;
    }
}

#endif /*FMC_CONFIG_FM_STACK == FMC_CONFIG_ENABLED*/

