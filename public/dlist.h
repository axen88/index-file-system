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

#define LIST_POISON1 ((void *) 0x00100100)
#define LIST_POISON2 ((void *) 0x00200200)

typedef struct list_head
{
    struct list_head *next;
    struct list_head *prev;
} list_head_t;

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member))) 

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

static inline void list_init_head(list_head_t *entry)
{
    ASSERT(entry);

    entry->next = entry;
    entry->prev = entry;
}

#define INIT_LIST_HEAD(head) list_init_head(head)

static void __list_add(list_head_t *new, list_head_t *prev, list_head_t *next)
{
    ASSERT(new);
    ASSERT(prev);
    ASSERT(next);

    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_head(list_head_t *head, list_head_t *new)
{
    ASSERT(head);
    ASSERT(new);

    __list_add(new, head, head->next);
}

#define list_add(head, new) list_add_head(head, new)

static inline void list_add_tail(list_head_t *head, list_head_t *new)
{
    ASSERT(head);
    ASSERT(new);

	__list_add(new, head->prev, head);
}

static void __list_del(list_head_t *entry, list_head_t *prev, list_head_t *next)
{
    ASSERT(prev);
    ASSERT(next);

    next->prev = prev;
    prev->next = next;
    
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

static inline void list_del(list_head_t *entry)
{
    ASSERT(entry);

    __list_del(entry, entry->prev, entry->next);
}

static inline list_head_t *list_get_node(list_head_t *head, uint32_t pos)
{
    uint32_t cnt = 0;
    list_head_t *next = NULL;

    ASSERT(head);

    next = head->next;
    ASSERT(next);

    while (next != head)
    {
        if (pos == cnt)
        {
            return next;
        }

        next = next->next;
        ASSERT(next);

        cnt++;
    }

    return NULL;
}

static inline int32_t list_remove_target_node(list_head_t *head, uint32_t position)
{
    list_head_t *entry = NULL;

    ASSERT(head);

    entry = list_get_node(head, position);

    if (entry)
    {
        list_del(entry);
        return 0;
    }

    return -1;
}

typedef int32_t (*list_cb)(void *, list_head_t *);

static inline int32_t list_walk_all(list_head_t *head, list_cb cb, void *para)
{
    int32_t ret = 0;
    list_head_t *next = NULL;
    list_head_t *next_next = NULL;

    ASSERT(cb);
    ASSERT(head);

    next = head->next;
    ASSERT(next);

    next_next = next->next;
    ASSERT(next_next);

    while (next != head)
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

static inline bool_t list_is_empty(list_head_t *head)
{
    ASSERT(head);

    return (head->next == head) ? TRUE : FALSE;
}

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)

#ifdef __cplusplus
}
#endif

#endif
