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

/** \file   queue.c 
 *  \brief  This module provides generic queueing services, including enqueue, dequeue
 *            and requeue of any object that contains TQueNodeHdr in its structure.                                  
 *
 *  \see    queue.h
 */



#include "mcpf_report.h"
#include "mcpf_queue.h"
#include "mcp_hal_os.h"


/* Queue structure */
typedef struct 
{
    TQueNodeHdr tHead;          /* The queue first node */
    McpU32   uCount;         /* Current number of nodes in queue */
    McpU32   uLimit;         /* Upper limit of nodes in queue */
    McpU32   uMaxCount;      /* Maximum uCount value (for debug) */
    McpU32   uOverflow;      /* Number of overflow occurrences - couldn't insert node (for debug) */
    McpU32   uNodeHeaderOffset; /* Offset of NodeHeader field from the entry of the queued item */
    handle_t   hOs;
    handle_t   hReport; 
} TQueue;   



/*
 *              INTERNAL  FUNCTIONS 
 *        =============================== 
 */


/*
 *  InsertNode():  Insert new node between pPrev and pNext 
 */
static INLINE void InsertNode( TQueNodeHdr *pNode, TQueNodeHdr *pPrev, TQueNodeHdr *pNext)
{
    pNext->pPrev = pNode;
    pNode->pNext = pNext;
    pNode->pPrev = pPrev;
    pPrev->pNext = pNode;
}

/*
 *  RemoveNode():  Remove node from between pPrev and pNext 
 */
static INLINE void RemoveNode( TQueNodeHdr *pPrev, TQueNodeHdr *pNext)
{
    pNext->pPrev = pPrev;
    pPrev->pNext = pNext;
}

/*
 *  AddToHead():  Add node to queue head (last in queue) 
 */
static INLINE void AddToHead( TQueNodeHdr *pNode, TQueNodeHdr *pListHead)
{
    InsertNode (pNode, pListHead, pListHead->pNext);
}

/*
 *  AddToTail():  Add node to queue tail (first in queue)
 */
static INLINE void AddToTail( TQueNodeHdr *pNode, TQueNodeHdr *pListHead)
{
    InsertNode( pNode, pListHead->pPrev, pListHead );
}

/*
 *  DelFromTail():  Delete node from queue tail (first in queue)
 */
static INLINE void DelFromTail (TQueNodeHdr *pNode)
{
    RemoveNode (pNode->pPrev, pNode->pNext);
}



/*
 *              EXTERNAL  FUNCTIONS 
 *        =============================== 
 */


/** 
 * \fn     que_Create 
 * \brief  Create a queue. 
 * 
 * Allocate and init a queue object.
 * 
 * \note    
 * \param  hOs               - Handle to Os Abstraction Layer
 * \param  hReport           - Handle to report module
 * \param  uLimit            - Maximum items to store in queue
 * \param  uNodeHeaderOffset - Offset of NodeHeader field from the entry of the queued item.
 * \return Handle to the allocated queue 
 * \sa     que_Destroy
 */ 
handle_t que_Create (handle_t hOs, handle_t hReport, McpU32 uLimit, McpU32 uNodeHeaderOffset)
{
    TQueue *pQue;

    /* allocate queue module */
    pQue = os_memoryAlloc (hOs, sizeof(TQueue));
    
    if (!pQue)
    {
        MCPF_OS_REPORT (("Error allocating the Queue Module\n"));
        return NULL;
    }
    
    os_memoryZero (hOs, pQue, sizeof(TQueue));

    /* Initialize the queue header */
    pQue->tHead.pNext = pQue->tHead.pPrev = &pQue->tHead;

    /* Set the Queue parameters */
    pQue->hOs               = hOs;
    pQue->hReport           = hReport;
    pQue->uLimit            = uLimit;
    pQue->uNodeHeaderOffset = uNodeHeaderOffset;

    return (handle_t)pQue;
}


/** 
 * \fn     que_Destroy
 * \brief  Destroy the queue. 
 * 
 * Free the queue memory.
 * 
 * \note   The queue's owner should first free the queued items!
 * \param  hQue - The queue object
 * \return E_COMPLETE on success or E_ERROR on failure 
 * \sa     que_Create
 */ 
mcpf_res_t que_Destroy (handle_t hQue)
{
    TQueue *pQue = (TQueue *)hQue;

    /* Alert if the queue is unloaded before it was cleared from items */
    if (pQue->uCount)
    {
        MCPF_REPORT_ERROR(pQue->hReport, QUEUE_MODULE_LOG, ("que_Destroy() Queue Not Empty!!"));
    }
    /* free Queue object */
    os_memoryFree (pQue->hOs, pQue, sizeof(TQueue));
    
    return E_COMPLETE;
}


/** 
 * \fn     que_Init 
 * \brief  Init required handles 
 * 
 * Init required handles.
 * 
 * \note    
 * \param  hQue      - The queue object
 * \param  hOs       - Handle to Os Abstraction Layer
 * \param  hReport   - Handle to report module
 * \return E_COMPLETE on success or E_ERROR on failure  
 * \sa     
 */ 
mcpf_res_t que_Init (handle_t hQue, handle_t hOs, handle_t hReport)
{
    TQueue *pQue = (TQueue *)hQue;

    pQue->hOs = hOs;
    pQue->hReport = hReport;
    
    return E_COMPLETE;
}


/** 
 * \fn     que_Enqueue
 * \brief  Enqueue an item 
 * 
 * Enqueue an item at the queue's head (last in queue).
 * 
 * \note   
 * \param  hQue   - The queue object
 * \param  hItem  - Handle to queued item
 * \return E_COMPLETE if item was queued, or E_ERROR if not queued due to overflow
 * \sa     que_Dequeue, que_Requeue
 */ 
mcpf_res_t que_Enqueue (handle_t hQue, handle_t hItem)
{
    TQueue      *pQue = (TQueue *)hQue;
    TQueNodeHdr *pQueNodeHdr;  /* the Node-Header in the given item */

    /* Check queue limit */
    if(pQue->uCount < pQue->uLimit)
    {
        /* Find NodeHeader in the given item */
        pQueNodeHdr = (TQueNodeHdr *)((McpU8*)hItem + pQue->uNodeHeaderOffset);

        /* Enqueue item and increment items counter */
        AddToHead (pQueNodeHdr, &pQue->tHead);
        pQue->uCount++;

#ifdef _DEBUG
        if (pQue->uCount > pQue->uMaxCount)
        {
            pQue->uMaxCount = pQue->uCount;
        }
        MCPF_REPORT_INFORMATION (pQue->hReport, QUEUE_MODULE_LOG, ("que_Enqueue(): Enqueued Successfully\n"));
#endif
        
        return E_COMPLETE;
    }
    
    /* 
     *  Queue is overflowed, return E_ERROR.
     */
#ifdef _DEBUG
    pQue->uOverflow++;
    MCPF_REPORT_WARNING (pQue->hReport, QUEUE_MODULE_LOG, ("que_Enqueue(): Queue Overflow\n"));
#endif
    
    return E_ERROR;
}


/** 
 * \fn     que_Dequeue
 * \brief  Dequeue an item 
 * 
 * Dequeue an item from the queue's tail (first in queue).
 * 
 * \note   
 * \param  hQue - The queue object
 * \return pointer to dequeued item or NULL if queue is empty
 * \sa     que_Enqueue, que_Requeue
 */ 
handle_t que_Dequeue (handle_t hQue)
{
    TQueue   *pQue = (TQueue *)hQue;
    handle_t hItem;
 
    if (pQue->uCount)
    {
         /* Queue is not empty, take packet from the queue tail */
      /* find pointer to the node entry */
         hItem = (handle_t)((McpU8*)pQue->tHead.pPrev - pQue->uNodeHeaderOffset); 
         DelFromTail (pQue->tHead.pPrev);    /* remove node from the queue */
         pQue->uCount--;
         return (hItem);
    }
    
    /* Queue is empty */
    MCPF_REPORT_INFORMATION (pQue->hReport, QUEUE_MODULE_LOG, ("que_Dequeue(): Queue is empty\n"));
    return NULL;
}


/** 
 * \fn     que_Requeue
 * \brief  Requeue an item 
 * 
 * Requeue an item at the queue's tail (first in queue).
 * 
 * \note   
 * \param  hQue   - The queue object
 * \param  hItem  - Handle to queued item
 * \return E_COMPLETE if item was queued, or E_ERROR if not queued due to overflow
 * \sa     que_Enqueue, que_Dequeue
 */ 
mcpf_res_t que_Requeue (handle_t hQue, handle_t hItem)
{
    TQueue      *pQue = (TQueue *)hQue;
    TQueNodeHdr *pQueNodeHdr;  /* the NodeHeader in the given item */

    /* 
     *  If queue's limits not exceeded add the packet to queue's tail and return E_COMPLETE 
     */
    if (pQue->uCount < pQue->uLimit)
    {
        /* Find NodeHeader in the given item */
        pQueNodeHdr = (TQueNodeHdr *)((McpU8*)hItem + pQue->uNodeHeaderOffset);

        /* Enqueue item and increment items counter */
        AddToTail (pQueNodeHdr, &pQue->tHead);
        pQue->uCount++;

#ifdef _DEBUG
        if (pQue->uCount > pQue->uMaxCount)
            pQue->uMaxCount = pQue->uCount;
        MCPF_REPORT_INFORMATION (pQue->hReport, QUEUE_MODULE_LOG, ("que_Requeue(): Requeued successfully\n"));
#endif

        return E_COMPLETE;
    }
    

    /* 
     *  Queue is overflowed, return E_ERROR.
     *  Note: This is not expected in the current design, since Tx packet may be requeued
     *          only right after it was dequeued in the same context so the queue can't be full.
     */
#ifdef _DEBUG
    pQue->uOverflow++;
    MCPF_REPORT_ERROR (pQue->hReport, QUEUE_MODULE_LOG, ("que_Requeue(): Queue Overflow\n"));
#endif
    
    return E_ERROR;
}


/** 
 * \fn     que_Size
 * \brief  Return queue size 
 * 
 * Return number of items in queue.
 * 
 * \note   
 * \param  hQue - The queue object
 * \return McpU32 - the items count
 * \sa     
 */ 
McpU32 que_Size (handle_t hQue)
{
    TQueue *pQue = (TQueue *)hQue;
 
    return (pQue->uCount);
}

    
/** 
 * \fn     que_Print
 * \brief  Print queue status
 * 
 * Print the queue's parameters (not the content).
 * 
 * \note   
 * \param  hQue - The queue object
 * \return void
 * \sa     
 */ 

#ifdef _DEBUG

void que_Print(handle_t hQue)
{
    TQueue *pQue = (TQueue *)hQue;

    MCPF_OS_REPORT(("que_Print: Count=%u MaxCount=%u Limit=%u Overflow=%u NodeHeaderOffset=%u Next=0x%x Prev=0x%x\n",
                    pQue->uCount, pQue->uMaxCount, pQue->uLimit, pQue->uOverflow, 
                    pQue->uNodeHeaderOffset, pQue->tHead.pNext, pQue->tHead.pPrev));
}

#endif /* _DEBUG */



