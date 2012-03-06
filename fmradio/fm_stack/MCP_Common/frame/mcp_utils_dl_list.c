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

#include "mcp_utils_dl_list.h"
#include "mcp_hal_types.h"
#include "mcp_defs.h"
#include "mcp_hal_log.h"

MCP_HAL_LOG_SET_MODULE(MCP_HAL_LOG_MODULE_TYPE_FRAME);

/*
	Forward Declaration
*/
MCP_STATIC McpBool MCP_DL_LIST_IsCircular(const MCP_DL_LIST_Node* list);


/*
	Initially, the head is initialized to point to itself in both directions => empty list
*/
void MCP_DL_LIST_InitializeHead(MCP_DL_LIST_Node *head)
{
	head->next = head;
	head->prev = head;
}

/*
	Initialize the node's next / prev pointers to NULL => Not on any list
*/
void MCP_DL_LIST_InitializeNode(MCP_DL_LIST_Node *node)
{
	node->next = 0;
	node->prev = 0;
}

MCP_DL_LIST_Node *MCP_DL_LIST_GetHead(MCP_DL_LIST_Node *head)
{
	/* The first element is the one "next" to the head */
	return head->next;
}

McpBool MCP_DL_LIST_IsNodeConnected(const MCP_DL_LIST_Node *node)
{
	/* A node is connected if the the adjacent nodes point at it correctly */
	if ((node->prev->next == node) && (node->next->prev == node))
	{
		return MCP_TRUE;
	}
	else
	{
		return MCP_FALSE;
	}
}

McpBool MCP_DL_LIST_IsEmpty(const MCP_DL_LIST_Node *head)
{
	/* A list is empty if the head's next pointer points at the head (as ti was initialized) */
	if (MCP_DL_LIST_GetHead((MCP_DL_LIST_Node*)head) == head)
	{
		return MCP_TRUE;
	}
	else
	{
		return MCP_FALSE;
	}
}

void MCP_DL_LIST_InsertTail(MCP_DL_LIST_Node* head, MCP_DL_LIST_Node* Node)
{
	MCP_FUNC_START("MCP_DL_LIST_InsertTailList");

	/* set the new node's links */
	Node->next = head;
	Node->prev = head->prev;

	/* Make current last node one before last */
	head->prev->next = Node;

	/* Head's prev should point to last element*/
	head->prev = Node;
	
	MCP_VERIFY_FATAL_NO_RETVAR((MCP_DL_LIST_IsNodeConnected(Node) == MCP_TRUE), ("MCP_DL_LIST_InsertTailList: Node is not connected"));

	MCP_FUNC_END();
}

void MCP_DL_LIST_InsertHead(MCP_DL_LIST_Node* head, MCP_DL_LIST_Node* Node)
{
	MCP_FUNC_START("MCP_DL_LIST_InsertHeadList");
	
	/* set the new node's links */
	Node->next = head->next;
	Node->prev = head;
	
	/* Make current first node second */
	head->next->prev = Node;
	
	/* Head's next should point to first element*/
	head->next = Node;

	MCP_VERIFY_FATAL_NO_RETVAR((MCP_DL_LIST_IsNodeConnected(Node) == MCP_TRUE), ("MCP_DL_LIST_InsertHeadList: Node is not connected"));

	MCP_FUNC_END();
}


MCP_DL_LIST_Node*MCP_DL_LIST_RemoveHead(MCP_DL_LIST_Node* head)
{
	MCP_DL_LIST_Node* first = NULL;

	MCP_FUNC_START("MCP_DL_LIST_RemoveHeadList");
	
	first = head->next;
	first->next->prev = head;
	head->next = first->next;

	MCP_VERIFY_FATAL_SET_RETVAR((MCP_DL_LIST_IsCircular(head) == MCP_TRUE), (first = NULL), ("_MCP_DL_LIST_RemoveHeadList: List is not circular"));

	MCP_FUNC_END();
	
	return first;
}

void MCP_DL_LIST_RemoveNode(MCP_DL_LIST_Node* Node)
{
	MCP_FUNC_START("MCP_DL_LIST_RemoveNodeList");
	
	MCP_VERIFY_FATAL_NO_RETVAR((MCP_DL_LIST_IsCircular(Node) == MCP_TRUE), ("MCP_DL_LIST_IsNodeOnList: List is not circular"));
	
	Node->prev->next = Node->next;
	Node->next->prev = Node->prev;
	
	MCP_VERIFY_FATAL_NO_RETVAR((MCP_DL_LIST_IsCircular(Node->prev) == MCP_TRUE), ("MCP_DL_LIST_RemoveNodeList: List is not circular"));

	MCP_DL_LIST_InitializeNode(Node);

	MCP_FUNC_END();
}

MCP_DL_LIST_Node *MCP_DL_LIST_GetNext(MCP_DL_LIST_Node* node)
{
	return node->next;
}


MCP_DL_LIST_Node *MCP_DL_LIST_GetPrev(MCP_DL_LIST_Node* node)
{
	return node->prev;
}

McpDlListNodeIndex MCP_DL_LIST_GetNodeIndex(const MCP_DL_LIST_Node* head, const MCP_DL_LIST_Node* node)
{
	McpDlListNodeIndex		index;		/* Holds the result */
	McpBool			nodeOnList; /* Indicates if node was found or not */
	MCP_DL_LIST_Node* 		tmpNode;

	MCP_FUNC_START("MCP_DL_LIST_GetNodeIndex");
	
	MCP_VERIFY_FATAL_SET_RETVAR((MCP_DL_LIST_IsCircular(head) == MCP_TRUE), (index = MCP_DL_LIST_INVALID_NODE_INDEX), 
									("MCP_DL_LIST_GetNodeIndex: List is not circular"));

	/* Start the search from the first node */
	tmpNode = MCP_DL_LIST_GetHead((MCP_DL_LIST_Node*)head);
	nodeOnList = MCP_FALSE;

	index = 0;

	/* Traverse list until we find the node or exhaust the list */
	while (tmpNode != head)
	{		
		if (tmpNode == node)
		{
			/* node found  */
			nodeOnList = MCP_TRUE;
			break;
		}

		/* Move to next node */
		++index;
		tmpNode = tmpNode->next;
	}

	/* Handle the node-not-found case */
	if (nodeOnList == MCP_FALSE)
	{
		index = MCP_DL_LIST_INVALID_NODE_INDEX;
	}
	
	MCP_FUNC_END();

	return index;
}

McpBool MCP_DL_LIST_IsNodeOnList(const MCP_DL_LIST_Node* head, const MCP_DL_LIST_Node* node)
{
	McpDlListNodeIndex index = MCP_DL_LIST_GetNodeIndex(head, node);

	if (index != MCP_DL_LIST_INVALID_NODE_INDEX)
	{
		return MCP_TRUE;
	}
	else
	{
		return MCP_FALSE;
	}
}


void MCP_DL_LIST_MoveList(MCP_DL_LIST_Node* dest, MCP_DL_LIST_Node* src)
{
	MCP_FUNC_START("MCP_DL_LIST_MoveList");
	
	MCP_VERIFY_FATAL_NO_RETVAR((MCP_DL_LIST_IsCircular(src) == MCP_TRUE), ("MCP_DL_LIST_MoveList: List is not circular"));

	/* New head points at list*/
	dest->next = src->next;
	dest->prev = src->prev;

	/* Detach source head from list */
	src->next->prev = dest;
	src->prev->next = dest;
	
	MCP_DL_LIST_InitializeHead(src);

	MCP_FUNC_END();
}

McpBool MCP_DL_LIST_IsCircular(const MCP_DL_LIST_Node* list)
{
	const MCP_DL_LIST_Node* tmp = list;

	if (MCP_DL_LIST_IsNodeConnected(list) == MCP_FALSE)
	{
		return MCP_FALSE;
	}

	for (tmp = tmp->next; tmp != list; tmp = tmp->next)
	{
		if (MCP_DL_LIST_IsNodeConnected(tmp) == MCP_FALSE)
		{
			return MCP_FALSE;
		}
	}
	
	return MCP_TRUE;
}

MCP_DL_LIST_Node *MCP_DL_LIST_FindMatchingNode(	MCP_DL_LIST_Node			 	*listHead, 
														MCP_DL_LIST_Node				*prevNode,
														McpDlListSearchDir			searchDir,
														const MCP_DL_LIST_Node		*nodeToMatch, 
														McpDlListComparisonFunc	comparisonFunc)
{
	MCP_DL_LIST_Node	*checkedNode = NULL;
	MCP_DL_LIST_Node	*matchingNode = NULL;
	
	MCP_FUNC_START("MCP_DL_LIST_UTILS_FindMatchingListEntry");
	
	MCP_VERIFY_FATAL_SET_RETVAR((NULL != listHead), (matchingNode = NULL), ("Null list argument"));
	MCP_VERIFY_FATAL_SET_RETVAR((NULL != nodeToMatch), (matchingNode = NULL), ("Null nodeToMatch argument"));
	MCP_VERIFY_FATAL_SET_RETVAR((NULL != comparisonFunc), (matchingNode = NULL), ("Null comparisonFunc argument"));

	/* Assume a matching entry will not be found */
	matchingNode = NULL;

	/* Decide from which node to start the search */
	
	if (prevNode == NULL)
	{
		checkedNode = listHead;
	}
	else
	{
		checkedNode = prevNode;
	}
		
	if (searchDir == MCP_DL_LIST_SEARCH_DIR_FIRST_TO_LAST)
	{
		checkedNode = checkedNode->next;
	}
	else
	{
		checkedNode = checkedNode->prev;
	}

	/* Check all list entries, until a match is found, or list exhausted */
	while (checkedNode != listHead)
	{
		/* Check the current entry, using the specified comparison function */
		if (MCP_TRUE == comparisonFunc(nodeToMatch, checkedNode))
		{
			matchingNode = checkedNode;

			/* entry matches, stop comparing */
			break;
		}

		/* Move on to check the next entry, according to the search direction */
		if (searchDir == MCP_DL_LIST_SEARCH_DIR_FIRST_TO_LAST)
		{
			checkedNode = checkedNode->next;
		}
		else
		{
			checkedNode = checkedNode->prev;
		}
	}
	
	MCP_FUNC_END();
	
	return matchingNode;
}


