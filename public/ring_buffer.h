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
File Name: OS_RING_BUFFER.H
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

#ifndef __OS_RING_BUFFER_H__
#define __OS_RING_BUFFER_H__


#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_NO_LIMIT    (-1)

typedef enum tagQUEUE_ERROR_CODE_E
{
    ERR_QUEUE_MALLOC = 100,
    ERR_QUEUE_INVALID_PARA,
    ERR_QUEUE_FULL,
    ERR_QUEUE_EMPTY,
    ERR_QUEUE_MEMB_NOT_FOUND,
} QUEUE_ERROR_CODE_E;

typedef struct ring_buffer
{
    u64_t *member;
    uint32_t head;  
    uint32_t tail;
    uint32_t num; 
    uint32_t max_num; 
} ring_buffer_t;


static inline ring_buffer_t *ring_buffer_create(int32_t max_num)
{
    ring_buffer_t *q = NULL;

    if (max_num <= 0)
    {
        return NULL;
    }

    q = (ring_buffer_t *)OS_MALLOC(sizeof(ring_buffer_t));
    if (q == NULL)
    {
        return NULL;
    }

    q->member = OS_MALLOC(sizeof(u64_t) * (uint32_t)max_num);
    if (q->member == NULL)
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

static inline int32_t ring_buffer_push(ring_buffer_t *q, u64_t member)
{
    ASSERT(q != NULL);

    if (q->num >= q->max_num)
    {
        return -ERR_QUEUE_FULL;
    }

    q->member[q->tail] = member;

    //smp_wmb(); // SPSC queue should enable

    if (++q->tail >= q->max_num)
    {
        q->tail = 0;
    }

    q->num++;

    return 0;
}

static inline int32_t ring_buffer_pop(ring_buffer_t *q, u64_t *member)
{
    ASSERT(q != NULL);
    ASSERT(member != NULL);

    if (q->num == 0)
    {
        return -ERR_QUEUE_EMPTY;
    }

    *member = q->member[q->head];
    
    //smp_mb(); // SPSC queue should enable

    if (++q->head >= q->max_num)
    {
        q->head = 0;
    }

    q->num--;

    return 0;
}

static inline int32_t ring_buffer_walk_all(ring_buffer_t *q, int32_t (*func)(u64_t, void *), void *para)
{
    int32_t ret = 0;
    uint32_t head = 0;
    uint32_t num = 0;
    u64_t member = 0;
    
    ASSERT(q != NULL);

    if (q->num == 0)
    {
        return -ERR_QUEUE_EMPTY;
    }

    head = q->head;
    num = q->num;
    
    while (num--)
    {
        member = q->member[head];
        ret = func(member, para);
        if (ret != 0)
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

static inline int32_t ring_buffer_get_size(ring_buffer_t *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->num;
}

static inline int32_t ring_buffer_get_max_size(ring_buffer_t *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->max_num;
}

static inline void ring_buffer_reset(ring_buffer_t *q)
{
    ASSERT(q != NULL);

    q->head = 0;
    q->tail = 0;
    q->num = 0;

    return;
}

static inline int32_t ring_buffer_destroy(ring_buffer_t *q)
{
    ASSERT(q != NULL);

    OS_FREE(q->member);
    OS_FREE(q);

    return 0;
}


#ifdef __cplusplus
}
#endif

#endif
