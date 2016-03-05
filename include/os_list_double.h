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
File Name: OS_LIST_DOUBLE.H
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

#ifndef __OS_LIST_DOUBLE_H__
#define __OS_LIST_DOUBLE_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tagDLIST_ENTRY_S
{
    struct tagDLIST_ENTRY_S *next;
    struct tagDLIST_ENTRY_S *prev;
} DLIST_ENTRY_S;

typedef struct tagDLIST_HEAD_S
{
    DLIST_ENTRY_S head;
    uint32_t num;
} DLIST_HEAD_S;

extern void dlist_init_entry(DLIST_ENTRY_S * entry);
extern void dlist_init_head(DLIST_HEAD_S * head);
extern void dlist_add_head(DLIST_HEAD_S * head,
    DLIST_ENTRY_S * entry);
extern void dlist_add_tail(DLIST_HEAD_S * head,
    DLIST_ENTRY_S * entry);
extern DLIST_ENTRY_S *dlist_get_entry(DLIST_HEAD_S * head, uint32_t position);
extern void dlist_remove_entry(DLIST_HEAD_S * head,
    DLIST_ENTRY_S * entry);
extern int32_t dlist_remove_target_entry(DLIST_HEAD_S * head,
    uint32_t position);
extern int32_t dlist_walk_all(DLIST_HEAD_S * head,
    int32_t (*func)(void *, DLIST_ENTRY_S *), void * para);
extern bool_t dlist_is_empty(DLIST_HEAD_S * head);
extern int32_t dlist_count(DLIST_HEAD_S * head);
extern int32_t dlist_walk_all_reverse(DLIST_HEAD_S * head,
    int32_t (*func)(void *, DLIST_ENTRY_S *), void * para);

#ifdef __cplusplus
}
#endif

#endif
