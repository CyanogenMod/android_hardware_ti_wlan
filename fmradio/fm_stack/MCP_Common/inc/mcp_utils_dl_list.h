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
*   FILE NAME:      mcp_utils_dl_list.h
*
*   BRIEF:          Intrusive Doubly-linked list
*
*   DESCRIPTION:
*
*   AUTHOR:   Udi Ron
*
\******************************************************************************/
#ifndef __MCP_UTILS_DL_LIST_UTILS_H
#define __MCP_UTILS_DL_LIST_UTILS_H

#include "mcp_hal_types.h"

/****************************************************************************
 *
 * Type Definitions
 *
 ****************************************************************************/

/*
	0-based list element index.

	The first element (head) is at index 0
*/
typedef McpUint McpDlListNodeIndex;

#define MCP_DL_LIST_INVALID_NODE_INDEX		((McpDlListNodeIndex)0xFFFFFFFF)


/*-------------------------------------------------------------------------------
 * McpDlListSearchDir type definition
 *
 *	A prototype of a comparison function that compares 2 list entries 
 *
 *	The function is used in the MCP_DL_LIST_FindMatchingNode() function 
 *	to find a matching entry from the entries in the list
 *
 *	It allows the caller to specify a matching policy that suits his needs.
 *
 *	The function should return:
 *		MCP_HAL_TRUE - the entries match
 *		MCP_HAL_FALSE - the entries do not match
*/
typedef McpUint McpDlListSearchDir;

#define MCP_DL_LIST_SEARCH_DIR_FIRST_TO_LAST		((McpDlListSearchDir) 0)
#define MCP_DL_LIST_SEARCH_DIR_LAST_TO_FIRST		((McpDlListSearchDir) 1)

/****************************************************************************
 *
 * Data Structures
 *
 ****************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_Node type definition
 *
 *	Defines the structure of a list node.
 *
 *	An entity that is to be placed on a list, must have its first structure 
 *	field of this type.
 *
 *	The structure has 2 pointers, one to link to the next list element (next) 
 *	and one to the previous list element (prev)
*/
typedef struct  _MCP_DL_LIST_Node 
{
    struct _MCP_DL_LIST_Node *next;
    struct _MCP_DL_LIST_Node *prev;
} MCP_DL_LIST_Node;
	
/*-------------------------------------------------------------------------------
 * McpDlListComparisonFunc type definition
 *
 *	A prototype of a comparison function that compares 2 list entries 
 *
 *	The function is used in the MCP_DL_LIST_FindMatchingNode() function 
 *	to find a matching entry from the entries in the list
 *
 *	It allows the caller to specify a matching policy that suits his needs.
 *
 *	The function should return:
 *		MCP_HAL_TRUE - the entries match
 *		MCP_HAL_FALSE - the entries do not match
*/
typedef McpBool (*McpDlListComparisonFunc)(	const MCP_DL_LIST_Node *nodeToMatch, 
												const MCP_DL_LIST_Node* checkedNode);


/****************************************************************************
 *
 * Functions and Macros Reference.
 *
 ****************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_InitializeHead()
 *
 * Brief:  
 *		Initializes the head of a linked list
 *
 * Description:
 *		This function must be called once for every list head before using any list function 
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - List Head.
 */
void MCP_DL_LIST_InitializeHead(MCP_DL_LIST_Node *head);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_InitializeNode()
 *
 * Brief:  
 *		Initializes a linked list node
 *
 * Description:
 *		This function must be called once for every node before inserting it into a list
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		node [in/out] - Initialized node.
 */
void MCP_DL_LIST_InitializeNode(MCP_DL_LIST_Node *node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_GetHead()
 *
 * Brief:  
 *		Returns the first element in a linked list
 *
 * Description:
 *		This function must be called once for every node before inserting it into a list
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - The list head.
 *
 * Returns:
 *		A pointer to the first node on the list (or NULL if the list is empty)	
 */
MCP_DL_LIST_Node *MCP_DL_LIST_GetHead(MCP_DL_LIST_Node *head);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_IsNodeConnected()
 *
 * Brief:  
 *		Checks whether a node is on a linked list
 *
 * Description:
 *		Checks whether a node is on a linked list
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		node [in] - The node.
 *
 * Returns:
 *		MCP_DL_LIST_TRUE if the node is on a list, MCP_DL_LIST_FALSE otherwise
 */
McpBool MCP_DL_LIST_IsNodeConnected(const MCP_DL_LIST_Node *node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_IsEmpty()
 *
 * Brief:  
 *		Checks whether a list is empty
 *
 * Description:
 *		Checks whether there are any nodes on the list or not
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in] - The list head.
 *
 * Returns:
 *		MCP_HAL_TRUE if the list is empty, MCP_HAL_FALSE otherwise
 */
McpBool MCP_DL_LIST_IsEmpty(const MCP_DL_LIST_Node *head);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_InsertTail()
 *
 * Brief:  
 *		Inserts a node at the end of a linked list
 *
 * Description:
 *		The function inserts a node as the last node in the list.
 *		Inserting nodes that way makes the list a FIFO queue
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - The list head.
 *		node [in / out] - The node to insert
 */
void MCP_DL_LIST_InsertTail(MCP_DL_LIST_Node* head, MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_InsertHead()
 *
 * Brief:  
 *		Inserts a node at the frontof a linked list
 *
 * Description:
 *		The function inserts a node as the first node in the list.
 *		Inserting nodes that way makes the list a stack
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - The list head.
 *		node [in / out] - The node to insert
 */
void MCP_DL_LIST_InsertHead(MCP_DL_LIST_Node* head, MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_RemoveHead()
 *
 * Brief:  
 *		Removes the first node in the list
 *
 * Description:
 *		Removes the first node in the list.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in/out] - The list head.
 */
MCP_DL_LIST_Node* MCP_DL_LIST_RemoveHead(MCP_DL_LIST_Node* head);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_RemoveNodeList()
 *
 * Brief:  
 *		Removes a node from the list it is on
 *
 * Description:
 *		Removes a node from the list it is on
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		node [in/out] - The node.
 */
void MCP_DL_LIST_RemoveNode(MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_GetNext()
 *
 * Brief:  
 *		Retrieves the next element
 *
 * Description:
 *		Retrieves the next element.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		node [in] - The node.
 *
 * Returns:
 *		The next element in the list
 */
MCP_DL_LIST_Node *MCP_DL_LIST_GetNext(MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_GetPrev()
 *
 * Brief:  
 *		Retrieves the previous element
 *
 * Description:
 *		Retrieves the previous element.
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		node [in] - The node.
 *
 * Returns:
 *		The previous element in the list
 */
MCP_DL_LIST_Node *MCP_DL_LIST_GetPrev(MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_GetNodeIndex()
 *
 * Brief:  
 *		Finds the sequential index of a node in a list
 *
 * Description:
 *		The function traverses the list and seraches for the specified node.
 *
 *		The list is traversed from the first node to the last. 
 *
 *		It returns the zero-based index of the node. 
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in] - The list head.
 *		node [in] - The node.
 *
 * Returns:
 *		The index if the node is on the list, or MCP_DL_LIST_LIST_NODE_INVALID_INDEX otherwise
 */
McpUint MCP_DL_LIST_GetNodeIndex(const MCP_DL_LIST_Node* head, const MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_IsNodeOnList()
 *
 * Brief:  
 *		Checks whether a node is on a list
 *
 * Description:
 *		The function searches the list to see whether the specified node is on it
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		head [in] - The list head.
 *		node [in] - The checked node
 *
 * Returns:
 *		MCP_HAL_TRUE if the node is on the list, MCP_DL_LIST_FALSE otherwise
 */
McpBool MCP_DL_LIST_IsNodeOnList(const MCP_DL_LIST_Node* head, const MCP_DL_LIST_Node* node);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_MoveList()
 *
 * Brief:  
 *		Moves a list from one head to another
 *
 * Description:
 *		The function detacjes the list elements from the src head and attaches them
 *		to the dest head
 *
 * Type:
 *		Synchronous
 *
 * Parameters:
 *		src [in] 		- The source list head.
 *		dest [out]	- The destination list head
 */
void MCP_DL_LIST_MoveList(MCP_DL_LIST_Node* dest, MCP_DL_LIST_Node* src);

/*-------------------------------------------------------------------------------
 * MCP_DL_LIST_FindMatchingNode()
 *
 * Brief:
 *		Checks if the specified entry is on this list.
 *
 * Description:
 *		The function searches for a matching node on the list. A node is considered matching
 *		based on a specified comparison function (comparisonFunc).
 *		
 *		The search order is from the first element to the last or from last to first. However, the first node
 *		to be searched is the one after prevNode, where one after depends on the search direction.
 *		This allows the caller to find multiple matches.
 *
 * Parameters:
 *
 *		listHead [in] - List head
 *
 		prevNode [in] - The node. If NULL, the search starts from the first / last node.

 		searchDir [in] - Search direction
 *
 *		nodeToMatch [in] - node to search for
 *
 *		comparisonFunc [in] - comparison function that defines the matching policy
 *
 * Returns:
 *		The mathcing node or NULL if no match was found
 */
MCP_DL_LIST_Node *MCP_DL_LIST_FindMatchingNode(	MCP_DL_LIST_Node		*listHead,
															MCP_DL_LIST_Node		*prevNode,
															McpDlListSearchDir			searchDir,
															const MCP_DL_LIST_Node	*nodeToMatch, 
															McpDlListComparisonFunc	comparisonFunc);

/*---------------------------------------------------------------------------
 * MCP_DL_LIST_ITERATE_WITH_REMOVAL()
 *
 *    Provides an iteration loop for sequential traversal of a linked list.
 *
 *	The current traversed element may be removed during traversal (using MCP_DL_LIST_RemoveNode()).
 *
 *    The macro is invoked using the following  parameters:
 *     	head - Head of the linked list
 *     	currElementPtr - Variable to use for current list element (must be of type elementType *)
 *     	nextElementPtr - Variable to use for temporary storage of next list element (must be of type elementType *)
 *     	elementType - Type of list elements.
 *
 *	The invocation must be followed by a block of code, as any regular for loop.
 *
 *	For example:
 *
 *		MCP_DL_LIST_Node	listHead;
 *     	SomeStruct			*curr;
 *		SomeStruct			*next;
 *
 *	MCP_DL_LIST_ITERATE_WITH_REMOVAL(listHead, curr, next, SomeStruct)
 *	{
 *		Iteration code (curr points at current traversed element)
 *	}
 */
#define MCP_DL_LIST_ITERATE_WITH_REMOVAL(head, currElementPtr, nextElementPtr, elementType) 			\
    for (	(currElementPtr) = (elementType *) MCP_DL_LIST_GetHead(&(head)) ; 					\
		(nextElementPtr) = (elementType *) MCP_DL_LIST_GetNext(&(currElementPtr)->node), 	\
		(currElementPtr) != (elementType *) &(head); \
		(currElementPtr) = (nextElementPtr))


/*---------------------------------------------------------------------------
 * MCP_DL_LIST_ITERATE_NO_REMOVAL()
 *
 *    Provides an iteration loop for sequential traversal of a linked list.
 *
 *	The current traversed element may NOT be removed during traversal (using MCP_DL_LIST_RemoveNode()).
 *
 *    The macro is invoked using the following  parameters:
 *     	head - Head of the linked list
 *     	currElementPtr - Variable to use for current list element (must be of type elementType *)
 *     	elementType - Type of list elements.
 *
 *	The invocation must be followed by a block of code, as any regular for loop.
 *
 *	For example:
 *
 *		MCP_DL_LIST_Node	listHead;
 *     	SomeStruct			*curr;
 *
 *	MCP_DL_LIST_ITERATE_WITH_REMOVAL(listHead, curr, next, SomeStruct)
 *	{
 *		Iteration code (curr points at current traversed element, DO NOT Remove curr from the list)
 *	}
 */
#define MCP_DL_LIST_ITERATE_NO_REMOVAL(head, currElementPtr, elementType) 				\
    for (	(currElementPtr) = (elementType *) MCP_DL_LIST_GetHead(&(head)) ; 					\
		(currElementPtr) != (elementType *) &(head); 											\
		(currElementPtr) = (elementType *) MCP_DL_LIST_GetNext(&(currElementPtr)->node))

#endif	/* __MCP_UTILS_DL_LIST_UTILS_H */

