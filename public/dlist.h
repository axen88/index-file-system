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
File Name: OS_DLIST.H
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

#ifndef __OS_DLIST_H__
#define __OS_DLIST_H__

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


static inline void dlist_init_entry(dlist_entry_t *entry)
{
    ASSERT(entry);

    entry->next = entry;
    entry->prev = entry;
}

static inline void dlist_init_head(dlist_head_t *head)
{
    ASSERT(head);

    head->num = 0;
    dlist_init_entry(&head->head);
}

static void add_entry(dlist_entry_t *entry, dlist_entry_t *prev, dlist_entry_t *next)
{
    ASSERT(entry);
    ASSERT(prev);
    ASSERT(next);

    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

static void remove_entry(dlist_entry_t *entry, dlist_entry_t *prev, dlist_entry_t *next)
{
    ASSERT(prev);
    ASSERT(next);

    next->prev = prev;
    prev->next = next;
    dlist_init_entry(entry);
}

static inline void dlist_add_head(dlist_head_t *head, dlist_entry_t *entry)
{
    ASSERT(head);
    ASSERT(entry);
    ASSERT(head->head.next);

    add_entry(entry, &head->head, head->head.next);
    head->num++;
}

static inline void dlist_add_tail(dlist_head_t *head, dlist_entry_t *entry)
{
    ASSERT(head);
    ASSERT(entry);
    ASSERT(head->head.prev);

	add_entry(entry, head->head.prev, &head->head);
    head->num++;
}

static inline void dlist_remove_entry(dlist_head_t *head, dlist_entry_t *entry)
{
    ASSERT(entry);
    ASSERT(entry->prev);
    ASSERT(entry->next);

    if (entry->prev != entry)
    {
        remove_entry(entry, entry->prev, entry->next);
        head->num--;
    }
}

static inline dlist_entry_t *dlist_get_entry(dlist_head_t *head, uint32_t position)
{
    uint32_t cnt = 0;
    dlist_entry_t *next = NULL;

    ASSERT(head);

    next = head->head.next;
    ASSERT(next);

    while (next != &head->head)
    {
        if (position == cnt)
        {
            return next;
        }

        next = next->next;
        ASSERT(next);

        cnt++;
    }

    return NULL;
}

static inline int32_t dlist_remove_target_entry(dlist_head_t *head, uint32_t position)
{
    dlist_entry_t *entry = NULL;

    ASSERT(head);

    entry = dlist_get_entry(head, position);

    if (entry)
    {
        dlist_remove_entry(head, entry);
        return 0;
    }

    return -1;
}

typedef int32_t (*dlist_cb)(void *, dlist_entry_t *);

static inline int32_t dlist_walk_all(dlist_head_t *head, dlist_cb cb, void *para)
{
    int32_t ret = 0;
    dlist_entry_t *next = NULL;
    dlist_entry_t *next_next = NULL;

    ASSERT(cb);
    ASSERT(head);

    next = head->head.next;
    ASSERT(next);

    next_next = next->next;
    ASSERT(next_next);

    while (next != &head->head)
    {
        ret = cb(para, next);
        if (ret != 0)
        {
            return ret;
        }

        next = next_next;

        next_next = next->next;
        ASSERT(next_next);
    }

    return 0;
}

static inline int32_t dlist_count(dlist_head_t *head)
{
    ASSERT(head);

    return (int32_t)head->num;
}

static inline bool_t dlist_is_empty(dlist_head_t *head)
{
    ASSERT(head);
    ASSERT(head->head.next);

    return (head->head.next == &head->head) ? TRUE : FALSE;
}


#ifdef __cplusplus
}
#endif

#endif
