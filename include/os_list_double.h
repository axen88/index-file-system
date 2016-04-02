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

typedef struct dlist_entry
{
    struct dlist_entry *next;
    struct dlist_entry *prev;
} dlist_entry_t;

typedef struct dlist_head
{
    dlist_entry_t head;
    uint32_t num;
} dlist_head_t;

extern void dlist_init_entry(dlist_entry_t * entry);
extern void dlist_init_head(dlist_head_t * head);
extern void dlist_add_head(dlist_head_t * head,
    dlist_entry_t * entry);
extern void dlist_add_tail(dlist_head_t * head,
    dlist_entry_t * entry);
extern dlist_entry_t *dlist_get_entry(dlist_head_t * head, uint32_t position);
extern void dlist_remove_entry(dlist_head_t * head,
    dlist_entry_t * entry);
extern int32_t dlist_remove_target_entry(dlist_head_t * head,
    uint32_t position);
extern int32_t dlist_walk_all(dlist_head_t * head,
    int32_t (*func)(void *, dlist_entry_t *), void * para);
extern bool_t dlist_is_empty(dlist_head_t * head);
extern int32_t dlist_count(dlist_head_t * head);
extern int32_t dlist_walk_all_reverse(dlist_head_t * head,
    int32_t (*func)(void *, dlist_entry_t *), void * para);

#ifdef __cplusplus
}
#endif

#endif
