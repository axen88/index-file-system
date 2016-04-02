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
File Name: OS_LIST_DOUBLE.C
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
#include "os_list_double.h"

void dlist_init_entry(dlist_entry_t * entry)
{
    ASSERT(NULL != entry);

    entry->next = entry;
    entry->prev = entry;
}

void dlist_init_head(dlist_head_t * head)
{
    ASSERT(NULL != head);

    head->num = 0;
    dlist_init_entry(&head->head);
}

static void add_entry(dlist_entry_t * entry, dlist_entry_t * prev,
    dlist_entry_t * next)
{
    ASSERT(NULL != entry);
    ASSERT(NULL != prev);
    ASSERT(NULL != next);

    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

static void remove_entry(dlist_entry_t * entry, dlist_entry_t * prev,
    dlist_entry_t * next)
{
    ASSERT(NULL != prev);
    ASSERT(NULL != next);

    next->prev = prev;
    prev->next = next;
    dlist_init_entry(entry);
}

void dlist_add_head(dlist_head_t * head, dlist_entry_t * entry)
{
    ASSERT(NULL != head);
    ASSERT(NULL != entry);
    ASSERT(NULL != head->head.next);

    add_entry(entry, &head->head, head->head.next);
    head->num++;
}

void dlist_add_tail(dlist_head_t * head, dlist_entry_t * entry)
{
    ASSERT(NULL != head);
    ASSERT(NULL != entry);
    ASSERT(NULL != head->head.prev);

	add_entry(entry, head->head.prev, &head->head);
    head->num++;
}

void dlist_remove_entry(dlist_head_t * head, dlist_entry_t * entry)
{
    ASSERT(NULL != entry);
    ASSERT(NULL != entry->prev);
    ASSERT(NULL != entry->next);

    if (entry->prev != entry)
    {
        remove_entry(entry, entry->prev, entry->next);
        head->num--;
    }
}

dlist_entry_t *dlist_get_entry(dlist_head_t * head, uint32_t position)
{
    uint32_t cnt = 0;
    dlist_entry_t *next = NULL;

    ASSERT(NULL != head);

    next = head->head.next;
    ASSERT(NULL != next);

    while (next != &head->head)
    {
        if (position == cnt)
        {
            return next;
        }

        next = next->next;
        ASSERT(NULL != next);

        cnt++;
    }

    return NULL;
}

int32_t dlist_remove_target_entry(dlist_head_t * head, uint32_t position)
{
    dlist_entry_t *entry = NULL;

    ASSERT(NULL != head);

    entry = dlist_get_entry(head, position);

    if (NULL != entry)
    {
        dlist_remove_entry(head, entry);
        return 0;
    }

    return -1;
}

int32_t dlist_walk_all(dlist_head_t * head,
    int32_t (*func)(void *, dlist_entry_t *), void * para)
{
    int32_t ret = 0;
    dlist_entry_t *next = NULL;
    dlist_entry_t *next_next = NULL;

    ASSERT(NULL != func);
    ASSERT(NULL != head);

    next = head->head.next;
    ASSERT(NULL != next);

    next_next = next->next;
    ASSERT(NULL != next_next);

    while (next != &head->head)
    {
        ret = func(para, next);
        if (0 != ret)
        {
            return ret;
        }

        next = next_next;

        next_next = next->next;
        ASSERT(NULL != next_next);
    }

    return 0;
}

int32_t dlist_walk_all_reverse(dlist_head_t * head,
    int32_t (*func)(void *, dlist_entry_t *), void * para)
{
    int32_t ret = 0;
    dlist_entry_t *prev = NULL;
    dlist_entry_t *prev_prev = NULL;

    ASSERT(NULL != func);
    ASSERT(NULL != head);

    prev = head->head.prev;
    ASSERT(NULL != prev);

    prev_prev = prev->prev;
    ASSERT(NULL != prev_prev);

    while (prev != &head->head)
    {
        ret = func(para, prev);
        if (0 != ret)
        {
            return ret;
        }

        prev = prev_prev;

        prev_prev = prev->prev;
        ASSERT(NULL != prev_prev);
    }

    return 0;
}

int32_t dlist_count(dlist_head_t * head)
{
    ASSERT(NULL != head);

    return (int32_t)head->num;
}

bool_t dlist_is_empty(dlist_head_t * head)
{
    ASSERT(NULL != head);
    ASSERT(NULL != head->head.next);

    return (head->head.next == &head->head) ? B_TRUE : B_FALSE;
}

