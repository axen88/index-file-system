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
File Name: OS_QUEUE.H
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

#ifndef __OS_QUEUE_H__
#define __OS_QUEUE_H__


#ifdef __cplusplus
extern "C" {
#endif

//#define __EN_LIST_QUEUE__

#define QUEUE_NO_LIMIT    (-1)

typedef enum tagQUEUE_ERROR_CODE_E
{
    ERR_QUEUE_MALLOC = 100,
    ERR_QUEUE_INVALID_PARA,
    ERR_QUEUE_FULL,
    ERR_QUEUE_EMPTY,
    ERR_QUEUE_MEMB_NOT_FOUND,
} QUEUE_ERROR_CODE_E;

#ifdef __EN_LIST_QUEUE__

#include "os_list_double.h"

typedef struct queue
{
    dlist_head_t head;
    uint32_t max_num;
} queue_t;

#else

typedef struct queue
{
    uint64_t *member;
    uint32_t head;  
    uint32_t tail;
    uint32_t num; 
    uint32_t max_num; 
} queue_t;



#endif


extern queue_t *queue_create(int32_t max_num);
extern int32_t queue_push(queue_t *q, uint64_t member);
extern int32_t queue_pop(queue_t *q, uint64_t *member);
extern int32_t queue_pop_push(queue_t *q, uint64_t push_member,
    uint64_t *pop_member);
extern int32_t queue_remove_member(queue_t *q, uint64_t member);
extern int32_t queue_walk_all(queue_t *q,
    int32_t (*func)(uint64_t, void *), void *para);
extern int32_t queue_get_size(queue_t *q);
extern int32_t queue_get_max_size(queue_t *q);
extern void queue_clean(queue_t *q);
extern int32_t queue_destroy(queue_t *q);

#ifdef __cplusplus
}
#endif

#endif
