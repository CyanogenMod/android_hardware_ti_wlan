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
*   FILE NAME:      fmc_utils.h
*
*   BRIEF:          FM Utilities
*
*   DESCRIPTION:
*
*     This files contains utilities and associated typesthat are used throughout the FM stack.
*
*   The utilities in this file are internal and are not to be used by code other than 
*   the FM stack code.
*
*   AUTHOR:   Udi Ron
*
\******************************************************************************/
#ifndef __FMC_UTILS_H
#define __FMC_UTILS_H

#include "fmc_types.h"
#include "fmc_core.h"
#include "fmc_common.h"
#include "fmc_fw_defs.h"
#include "fm_tx.h"


/****************************************************************************
 *
 * Type Definitions
 *
 ****************************************************************************/

typedef struct  _FMC_ListNode FMC_ListNode;

typedef FMC_UINT FmcListNodeIndex;

#define FMC_LIST_NODE_INVALID_INDEX     ((FmcListNodeIndex)0xFFFFFFFF)

/*-------------------------------------------------------------------------------
 * FmcUtilsListComparisonFunc type definition
 *
 *  A prototype of a comparison function that compares 2 list entries 
 *
 *  The function is used in the BTL_UTILS_FindMatchingFMC_ListNode function 
 *  to find a matching entry from the entries in the list
 *
 *  It allows the caller to specify a matching policy that suits his needs.
 *
 *  The function should return:
 *      TRUE - the entries match
 *      FALSE - the entries do not match
*/
typedef FMC_BOOL (*FmcUtilsListComparisonFunc)(const FMC_ListNode *nodeToMatch, const FMC_ListNode* checkedNode);

/*-------------------------------------------------------------------------------
 * FmcUtilsSearchDir type definition
 *
 *  A prototype of a comparison function that compares 2 list entries 
 *
 *  The function is used in the BTL_UTILS_FindMatchingFMC_ListNode function 
 *  to find a matching entry from the entries in the list
 *
 *  It allows the caller to specify a matching policy that suits his needs.
 *
 *  The function should return:
 *      TRUE - the entries match
 *      FALSE - the entries do not match
*/
typedef FMC_UINT FmcUtilsSearchDir;

#define FMC_UTILS_SEARCH_FIRST_TO_LAST      ((FmcUtilsSearchDir) 0)
#define FMC_UTILS_SEARCH_LAST_TO_FIRST      ((FmcUtilsSearchDir) 1)

/****************************************************************************
 *
 * Data Structures
 *
 ****************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_ListNode type definition
 *
 *  Defines the structure of a list node.
 *
 *  An entity that is to be placed on a list, must have its first structure 
 *  field of this type.
 *
 *  The structure has 2 pointers, one to link to the next list element (NextNode) 
 *  and one to the previous list element (PrevNode)
*/
struct  _FMC_ListNode 
{
    struct _FMC_ListNode *NextNode;
    struct _FMC_ListNode *PrevNode;
} ;
    
/****************************************************************************
 *
 * Functions and Macros Reference.
 *
 ****************************************************************************/

/*-------------------------------------------------------------------------------
 * FMC_InitializeListHead()
 *
 * Brief:  
 *      Initializes the head of a linked list
 *
 * Description:
 *      This function must be called once for every list head before using any list function 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in/out] - List Head.
 */
void FMC_InitializeListHead(FMC_ListNode *head);

/*-------------------------------------------------------------------------------
 * FMC_InitializeListNode()
 *
 * Brief:  
 *      Initializes a linked list node
 *
 * Description:
 *      This function must be called once for every node before inserting it into a list
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      node [in/out] - Initialized node.
 */
void FMC_InitializeListNode(FMC_ListNode *node);

/*-------------------------------------------------------------------------------
 * FMC_GetHeadList()
 *
 * Brief:  
 *      Returns the first element in a linked list
 *
 * Description:
 *      This function must be called once for every node before inserting it into a list
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in/out] - The list head.
 *
 * Returns:
 *      A pointer to the first node on the list (or NULL if the list is empty)  
 */
FMC_ListNode *FMC_GetHeadList(FMC_ListNode *head);

/*-------------------------------------------------------------------------------
 * FMC_IsNodeConnected()
 *
 * Brief:  
 *      Checks whether a node is on a linked list
 *
 * Description:
 *      Checks whether a node is on a linked list
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      node [in] - The node.
 *
 * Returns:
 *      FMC_TRUE if the node is on a list, FMC_FALSE otherwise
 */
FMC_BOOL FMC_IsNodeConnected(const FMC_ListNode *node);

/*-------------------------------------------------------------------------------
 * FMC_IsListEmpty()
 *
 * Brief:  
 *      Checks whether a list is empty
 *
 * Description:
 *      Checks whether there are any nodes on the list or not
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in] - The list head.
 *
 * Returns:
 *      FMC_TRUE if the list is empty, FMC_FALSE otherwise
 */
FMC_BOOL FMC_IsListEmpty(const FMC_ListNode *head);

/*-------------------------------------------------------------------------------
 * FMC_InsertTailList()
 *
 * Brief:  
 *      Inserts a node at the end of a linked list
 *
 * Description:
 *      The function inserts a node as the last node in the list.
 *      Inserting nodes that way makes the list a FIFO queue
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in/out] - The list head.
 *      node [in / out] - The node to insert
 */
void FMC_InsertTailList(FMC_ListNode* head, FMC_ListNode* node);

/*-------------------------------------------------------------------------------
 * FMC_InsertHeadList()
 *
 * Brief:  
 *      Inserts a node at the frontof a linked list
 *
 * Description:
 *      The function inserts a node as the first node in the list.
 *      Inserting nodes that way makes the list a stack
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in/out] - The list head.
 *      node [in / out] - The node to insert
 */
void FMC_InsertHeadList(FMC_ListNode* head, FMC_ListNode* node);

/*-------------------------------------------------------------------------------
 * FMC_RemoveHeadList()
 *
 * Brief:  
 *      Removes the first node in the list
 *
 * Description:
 *      Removes the first node in the list.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in/out] - The list head.
 */
FMC_ListNode* FMC_RemoveHeadList(FMC_ListNode* head);

/*-------------------------------------------------------------------------------
 * FMC_RemoveNodeList()
 *
 * Brief:  
 *      Removes a node from the list it is on
 *
 * Description:
 *      Removes a node from the list it is on
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      node [in/out] - The node.
 */
void FMC_RemoveNodeList(FMC_ListNode* node);

/*-------------------------------------------------------------------------------
 * FMC_GetNodeIndex()
 *
 * Brief:  
 *      Finds the sequential index of a node in a list
 *
 * Description:
 *      The function traverses the list and seraches for the specified node.
 *
 *      The list is traversed from the first node to the last. 
 *
 *      It returns the zero-based index of the node. 
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in] - The list head.
 *      node [in] - The node.
 *
 * Returns:
 *      The index if the node is on the list, or FMC_LIST_NODE_INVALID_INDEX otherwise
 */
FMC_UINT FMC_GetNodeIndex(const FMC_ListNode* head, const FMC_ListNode* node);

/*-------------------------------------------------------------------------------
 * FMC_IsNodeOnList()
 *
 * Brief:  
 *      Checks whether a node is on a list
 *
 * Description:
 *      The function searches the list to see whether the specified node is on it
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      head [in] - The list head.
 *
 * Returns:
 *      FMC_TRUE if the node is on the list, FMC_FALSE otherwise
 */
FMC_BOOL FMC_IsNodeOnList(const FMC_ListNode* head, const FMC_ListNode* node);

/*-------------------------------------------------------------------------------
 * FMC_MoveList()
 *
 * Brief:  
 *      Moves a list from one head to another
 *
 * Description:
 *      The function detacjes the list elements from the src head and attaches them
 *      to the dest head
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      src [in]        - The source list head.
 *      dest [out]  - The destination list head
 */
void FMC_MoveList(FMC_ListNode* dest, FMC_ListNode* src);

/*-------------------------------------------------------------------------------
 * FMC_UTILS_FindMatchingListNode()
 *
 * Brief:
 *      Checks if the specified entry is on this list.
 *
 * Description:
 *      The function searches for a matching node on the list. A node is considered matching
 *      based on a specified comparison function (comparisonFunc).
 *      
 *      The search order is from the first element to the last or from last to first. However, the first node
 *      to be searched is the one after prevNode, where one after depends on the search direction.
 *      This allows the caller to find multiple matches.
 *
 * Parameters:
 *
 *      listHead [in] - List head
 *
        prevNode [in] - The node. If NULL, the search starts from the first / last node.

        searchDir [in] - Search direction
 *
 *      nodeToMatch [in] - node to search for
 *
 *      comparisonFunc [in] - comparison function that defines the matching policy
 *
 * Returns:
 *      The mathcing node or NULL if no match was found
 */
FMC_ListNode *FMC_UTILS_FindMatchingListNode(   FMC_ListNode                *listHead,
                                                        FMC_ListNode                *prevNode,
                                                        FmcUtilsSearchDir           searchDir,
                                                        const FMC_ListNode      *nodeToMatch, 
                                                        FmcUtilsListComparisonFunc  comparisonFunc);

/*-------------------------------------------------------------------------------
 * FMC_UTILS_FindAndCompareLocationMatchingListNodes()
 *
 * Brief:
 *      Finds witch Node was inserted first to the list.
 *
 * Description:
 *      This function searches for both first and second node and returns true if the first node was inserted first 
 *      and therefor will be executed first.
 *  
 *      This function is needed to verify the relative location in the list of 2 conflicting commands.
 *
 * Parameters:
 *
 *      listHead [in] - List head
 *
        prevNode [in] - The node. If NULL, the search starts from the first / last node.

        searchDir [in] - Search direction
 *
 *      nodeToMatch [in] - node to search for
 *
 *      comparisonFunc [in] - comparison function that defines the matching policy
 *
 * Returns:
 *      The first node was inserted first to the list.
 */
FMC_BOOL FMC_UTILS_FindAndCompareLocationMatchingListNodes( FMC_ListNode                *listHead, 
                                                        const FMC_ListNode      *firstNodeToMatch, 
                                                        const FMC_ListNode      *secondNodeToMatch, 
                                                        FmcUtilsListComparisonFunc  comparisonFunc);

/*-------------------------------------------------------------------------------
 * FMC_UTILS_FormatNumber()
 *
 * Brief:  
 *      Converts a number to a string
 *
 * Description:
 *      The function converts a numeric variable to a string using a specified format string (same as in printf).
 *
 *      The output string is supplied by the caller as an argument to avoid memory allocation issues.
 *
 *      The function also returns the output string to allow callers to use the function within expressions.
 *
 * Caveat:
 *      tempStr must have the capacity to store the resulting string, according to the format string.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      formatMsg [in]  - The format string.
 *      number [in]     - The number to convert
 *      tempStr [out]       - The output string
 *      maxTempStrSize [in]       - Max Size  tempStr buffer.
 *
 * Returns:
 *      tempStr (the argument)
 */
const char * FMC_UTILS_FormatNumber(const char *formatMsg, FMC_UINT number, char *tempStr,FMC_UINT maxTempStrSize);
    
/*-------------------------------------------------------------------------------
 * FMC_UTILS_IsFreqValid()
 *
 * Brief:  
 *      Checks whether a frequency is valid
 *
 * Description:
 *      A frequency is valid if:
 *      1. It is within the bounds of the specified band; And
 *      2. It is a multiple of 50 kHz
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      freq [in]               - The frequency in kHz
 *
 * Returns:
 *      FMC_TRUE is the frequency is valid, FMC_FALSE otherwise
 */
FMC_BOOL FMC_UTILS_IsFreqValid(FmcFreq freq);


/*-------------------------------------------------------------------------------
 * FMC_UTILS_IsAfCodeValid()
 *
 * Brief:  
 *      Checks whether a Af code is valid
 *
 * Description:
 *      AF Code is valid if:
 *      1. Af code is between the range of 1-204. ( acorrding to RDS spec)
 *      2. Af code equal to FMC_AF_CODE_NO_AF_AVAILABLE.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      afCode [in]               - Af code.
 *
 * Returns:
 *      FMC_TRUE if the afCode is valid, FMC_FALSE otherwise
 */
FMC_BOOL FMC_UTILS_IsAfCodeValid( FmcAfCode afCode);


/*-------------------------------------------------------------------------------
 * FMC_UTILS_ChannelSpacingInKhz()
 *
 * Brief:  
 *      Converts a channel spacing type value to its kHz equivalent
 *
 * Description:
 *      Converts a channel spacing type value to its kHz equivalent
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      channelSpacing [in] - The channel spacing
 *
 * Returns:
 *      The channel spacing in kHz
 */
FMC_UINT FMC_UTILS_ChannelSpacingInKhz(FmcChannelSpacing channelSpacing);

/*-------------------------------------------------------------------------------
 * FMC_UTILS_FreqToFwChannelIndex()
 *
 * Brief:  
 *      Calculates the Firmware channel index
 *
 * Description:
 *      When tuning the FM receiver or transmitter, the firmware receives a 
 *      channel index that is a function of the band, the frequency,
 *      and the spacing between the channels (50, 100, or 200 kHz).
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      band [in]               - The band
 *      channelSpacing [in]     - The channel spacing
 *      freq [in]               - The frequency
 *
 * Returns:
 *      The channel index
 */
FmcFwCmdParmsValue  FMC_UTILS_FreqToFwChannelIndex(FmcBand band ,FmcChannelSpacing channelSpacing,FmcFreq freq);
/*-------------------------------------------------------------------------------
 * FMC_UTILS_FreqToFwChannelIndex()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      band [in]               - The band
 *      channelSpacing [in]     - The channel spacing
 *      channelIndex [in]           - The channelIndex
 *
 * Returns:
 *      The channel index
 */

FmcFreq FMC_UTILS_FwChannelIndexToFreq(FmcBand band ,FmcChannelSpacing channelSpacing,FMC_U16 channelIndex);


/*-------------------------------------------------------------------------------
 * FMC_UTILS_Api2FwPowerLevel()
 *
 * Brief:  
 *      Converts a power level in FM API terms to a Firmware value
 *
 * Description:
 *      The FM API provides power level control. The caller specifies a number.
 *      The API value indicates a linear power level. However, the firmware values
 *      use a lograithmic method. Therefore, a conversion needs to be done to be able
 *      to use the value in the firmware API. This function uses an offline table that
 *      maps the values accordingly.
 *
 * Type:
 *      Synchronous
 *
 * Parameters:
 *      apiPowerLevel [in]  - The API power level value
 *
 * Returns:
 *      The firmware value
 */
FmcFwCmdParmsValue FMC_UTILS_Api2FwPowerLevel(FmTxPowerLevel apiPowerLevel);
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
FMC_BOOL FMC_IsNodeAvailable(FMC_ListNode *node);
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
 
FMC_BOOL FMC_IsListCircular(const FMC_ListNode* list);


/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
FMC_U16 FMC_UTILS_BEtoHost16(FMC_U8 * num);
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
FMC_U16 FMC_UTILS_LEtoHost16(FMC_U8 * num);
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
void FMC_UTILS_StoreBE16(FMC_U8 *buff, FMC_U16 be_value) ;
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
void FMC_UTILS_StoreLE16(FMC_U8 *buff, FMC_U16 le_value) ;
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
void FMC_UTILS_StoreBE32(FMC_U8 *buff, FMC_U32 be_value) ;
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
FmcCoreEventType FMC_UTILS_ConvertVacEvent2FmcEvent(ECCM_VAC_Event eEvent);
/*-------------------------------------------------------------------------------
 * FMC_xxx()
 *
 * Brief:  
 *      xxx
 *
 * Description:
 *      xxx
 *
 * Type:
 *      Syxxx/As
 *
 * Parameters:
 *      xxx [in]    - 
 *
 * Returns:
 *      xxx
 */
FmcStatus FMC_UTILS_ConvertVacStatus2FmcStatus(ECCM_VAC_Status eStatus);

#endif  /* __FM_UTILS_H */

