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

            Copyright(C), 2016~2019, axen2012@qq.com
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

#define __EN_LIST_QUEUE__

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

typedef struct tagQUEUE_S
{
    DLIST_HEAD_S head;
    uint32_t max_size;
} QUEUE_S;

#else

typedef struct tagQUEUE_S
{
    uint64_t *pullMemb; /* 存储内容的内存 */
    uint32_t uiHead;    /* 读指针 */
    uint32_t uiTail;    /* 写指针 */
    uint32_t num;     /* 当前有效的成员数目 */
    uint32_t max_member;     /* 支持的最大成员数目 */
} QUEUE_S;



#endif


extern QUEUE_S *queue_create(int32_t max_size);
extern int32_t queue_push(QUEUE_S *q, uint64_t member);
extern int32_t queue_pop(QUEUE_S *q, uint64_t *member);
extern int32_t queue_pop_push(QUEUE_S *q, uint64_t push_member,
    uint64_t *pop_member);
extern int32_t queue_remove_member(QUEUE_S *q, uint64_t member);
extern int32_t queue_walk_all(QUEUE_S *q,
    int32_t (*func)(uint64_t, void *), void *para);
extern int32_t queue_get_size(QUEUE_S *q);
extern int32_t queue_get_max_size(QUEUE_S *q);
extern void queue_clean(QUEUE_S *q);
extern int32_t queue_destroy(QUEUE_S *q);

#ifdef __cplusplus
}
#endif

#endif
