/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*******************************************************************************

            Copyright(C), 2016~2019, axen.hook@foxmail.com
********************************************************************************
File Name: OS_QUEUE.C
Author   : axen.hook
Version  : 1.00
Date     : 02/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 02/Mar/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#include "os_adapter.h"
#include "os_queue.h"

#ifndef __EN_LIST_QUEUE__

QUEUE_S *queue_create(int32_t max_num)
{
    QUEUE_S *q = NULL;

    if (0 >= max_num)
    {
        return NULL;
    }

    q = (QUEUE_S *)OS_MALLOC(sizeof(QUEUE_S));
    if (NULL == q)
    {
        return NULL;
    }

    q->member = OS_MALLOC(sizeof(uint64_t) * (uint32_t)max_num);
    if (NULL == q->member)
    {
        OS_FREE(q);
        return NULL;
    }

    q->head = 0;
    q->tail = 0;
    q->num = 0;
    q->max_num = (uint32_t)max_num;

    return q;
}

int32_t queue_push(QUEUE_S *q, uint64_t member)
{
    ASSERT(q != NULL);

    if (q->num >= q->max_num)
    {
        return -ERR_QUEUE_FULL;
    }

    q->member[q->tail++] = member;
    if (q->tail >= q->max_num)
    {
        q->tail = 0;
    }

    q->num++;

    return 0;
}

int32_t queue_pop(QUEUE_S *q, uint64_t *member)
{
    ASSERT(q != NULL);
    ASSERT(member != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    *member = q->member[q->head++];
    if (q->head >= q->max_num)
    {
        q->head = 0;
    }

    q->num--;

    return 0;
}

int32_t queue_pop_push(QUEUE_S *q, uint64_t push_member, uint64_t *pop_member)
{
    int32_t ret = 0;
    
    ASSERT(q != NULL);
    ASSERT(pop_member != NULL);

    if (0 == q->num)
    {
        ret = -ERR_QUEUE_EMPTY;
    }
    else
    {
        *pop_member = q->member[q->head++];
        if (q->head >= q->max_num)
        {
            q->head = 0;
        }
        
        q->num--;
    }
    
    q->member[q->tail++] = push_member;
    if (q->tail >= q->max_num)
    {
        q->tail = 0;
    }

    q->num++;

    return ret;
}

int32_t queue_remove_member(QUEUE_S *q, uint64_t member)
{
    uint32_t head = 0;
    uint32_t num = 0;
    uint64_t memb = 0;
    
    ASSERT(q != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    head = q->head;
    num = q->num;
    
    while (num--)
    {
        memb = q->member[head];
        if (memb == member)
        {
            if (head == q->head)
            {
                q->head++;
            }
            else if (head > q->head)
            {
                memmove(&q->member[q->head + 1], &q->member[q->head],
                    (uint32_t)(head - q->head) * sizeof(uint64_t));
                q->head++;
            }
            else
            {
                ASSERT(q->tail > head);
                memcpy(&q->member[head], &q->member[head + 1],
                    (uint32_t)(q->tail - head) * sizeof(uint64_t));
                q->tail--;
            }
            
            if (q->head >= q->max_num)
            {
                q->head = 0;
            }
            
            q->num--;
            return 0;
        }

        if (++head >= q->max_num)
        {
            head = 0;
        }
    }

    return -ERR_QUEUE_MEMB_NOT_FOUND;
}

int32_t queue_walk_all(QUEUE_S *q,
    int32_t (*func)(uint64_t, void *), void *para)
{
    int32_t ret = 0;
    uint32_t head = 0;
    uint32_t num = 0;
    uint64_t member = 0;
    
    ASSERT(q != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    head = q->head;
    num = q->num;
    
    while (num--)
    {
        member = q->member[head];
        ret = func(member, para);
        if (0 != ret)
        {
            break;
        }

        if (++head >= q->max_num)
        {
            head = 0;
        }
    }

    return ret;
}

int32_t queue_get_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->num;
}

int32_t queue_get_max_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->max_num;
}

void queue_clean(QUEUE_S *q)
{
    ASSERT(q != NULL);

    q->head = 0;
    q->tail = 0;
    q->num = 0;

    return;
}

int32_t queue_destroy(QUEUE_S *q)
{
    ASSERT(q != NULL);

    OS_FREE(q->member);
    OS_FREE(q);

    return 0;
}

EXPORT_SYMBOL(queue_create);
EXPORT_SYMBOL(queue_push);
EXPORT_SYMBOL(queue_pop);
EXPORT_SYMBOL(queue_pop_push);
EXPORT_SYMBOL(queue_remove_member);
EXPORT_SYMBOL(queue_walk_all);
EXPORT_SYMBOL(queue_get_size);
EXPORT_SYMBOL(queue_get_max_size);
EXPORT_SYMBOL(queue_clean);
EXPORT_SYMBOL(queue_destroy);

#endif

