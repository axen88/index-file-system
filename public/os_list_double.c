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

            版权所有(C), 2011~2014, AXEN工作室
********************************************************************************
文 件 名: OS_LIST_DOUBLE.C
版    本: 1.00
日    期: 2011年8月12日
功能描述: 双向链表程序
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月12日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#include "os_adapter.h"
#include "os_list_double.h"

/*******************************************************************************
函数名称: DListInitEntry
功能说明: 初始化entry
输入参数:
    entry: 要初始化的entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void dlist_init_entry(DLIST_ENTRY_S * entry)
{
    ASSERT(NULL != entry);

    entry->next = entry;
    entry->prev = entry;
}

/*******************************************************************************
函数名称: DListInitHead
功能说明: 初始化head
输入参数:
    head: 要初始化的head
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void dlist_init_head(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);

    head->num = 0;
    dlist_init_entry(&head->head);
}

/*******************************************************************************
函数名称: add_entry
功能说明: 将entry添加到链表中去
输入参数:
    entry: 要添加的entry
    prev : 要添加的entry的前一个entry
    next : 要添加的entry的后一个entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
static void add_entry(DLIST_ENTRY_S * entry, DLIST_ENTRY_S * prev,
    DLIST_ENTRY_S * next)
{
    ASSERT(NULL != entry);
    ASSERT(NULL != prev);
    ASSERT(NULL != next);

    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/*******************************************************************************
函数名称: remove_entry
功能说明: 删除entry
输入参数:
    entry: 要删除的entry
    prev : 要删除的entry的前一个entry
    next : 要删除的entry的后一个entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
static void remove_entry(DLIST_ENTRY_S * entry, DLIST_ENTRY_S * prev,
    DLIST_ENTRY_S * next)
{
    ASSERT(NULL != prev);
    ASSERT(NULL != next);

    next->prev = prev;
    prev->next = next;
    dlist_init_entry(entry);
}

/*******************************************************************************
函数名称: DListAddHead
功能说明: 在链表的头部位置添加一个entry
输入参数:
    head : 要操作的链表
    entry: 要添加的entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void dlist_add_head(DLIST_HEAD_S * head, DLIST_ENTRY_S * entry)
{
    ASSERT(NULL != head);
    ASSERT(NULL != entry);
    ASSERT(NULL != head->head.next);

    add_entry(entry, &head->head, head->head.next);
    head->num++;
}

/*******************************************************************************
函数名称: DListAddTail
功能说明: 在链表的尾部位置添加一个entry
输入参数:
    head : 要操作的链表
    entry: 要添加的entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void dlist_add_tail(DLIST_HEAD_S * head, DLIST_ENTRY_S * entry)
{
    ASSERT(NULL != head);
    ASSERT(NULL != entry);
    ASSERT(NULL != head->head.prev);

	add_entry(entry, head->head.prev, &head->head);
    head->num++;
}

/*******************************************************************************
函数名称: DListremove_entry
功能说明: 删除指定entry
输入参数:
    head : 要操作的链表
    entry: 要添加的entry
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void dlist_remove_entry(DLIST_HEAD_S * head, DLIST_ENTRY_S * entry)
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

/*******************************************************************************
函数名称: DListGetEntry
功能说明: 获取指定位置处的entry
输入参数:
    head : 要操作的链表
    position   : 指定的位置
输出参数: 无
返 回 值:
    !=NULL: 获取成功，获取到的entry指针
    ==NULL: 获取失败
说    明: 无
*******************************************************************************/
DLIST_ENTRY_S *dlist_get_entry(DLIST_HEAD_S * head, uint32_t position)
{
    uint32_t cnt = 0;
    DLIST_ENTRY_S *next = NULL;

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

/*******************************************************************************
函数名称: DListRemoveTargetEntry
功能说明: 删除指定位置处的entry
输入参数:
    head : 要操作的链表
    position   : 指定的位置
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t dlist_remove_target_entry(DLIST_HEAD_S * head, uint32_t position)
{
    DLIST_ENTRY_S *entry = NULL;

    ASSERT(NULL != head);

    entry = dlist_get_entry(head, position);

    if (NULL != entry)
    {
        dlist_remove_entry(head, entry);
        return 0;
    }

    return -1;
}

/*******************************************************************************
函数名称: DListWalkAll
功能说明: 遍历链表
输入参数:
    head : 要操作的链表
    func   : 对每个链表成员操作的回调函数
    para   : 回调函数的其中一个参数
输出参数: 无
返 回 值:
    ==0: 全部遍历完成
    !=0: 中途退出遍历，返回值为回调函数的返回值
说    明: 无
*******************************************************************************/
int32_t dlist_walk_all(DLIST_HEAD_S * head,
    int32_t (*func)(void *, DLIST_ENTRY_S *), void * para)
{
    int32_t ret = 0;
    DLIST_ENTRY_S *next = NULL;
    DLIST_ENTRY_S *next_next = NULL;

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

/*******************************************************************************
函数名称: DListWalkAllReverse
功能说明: 遍历链表
输入参数:
    head : 要操作的链表
    func   : 对每个链表成员操作的回调函数
    para   : 回调函数的其中一个参数
输出参数: 无
返 回 值:
    ==0: 全部遍历完成
    !=0: 中途退出遍历，返回值为回调函数的返回值
说    明: 无
*******************************************************************************/
int32_t dlist_walk_all_reverse(DLIST_HEAD_S * head,
    int32_t (*func)(void *, DLIST_ENTRY_S *), void * para)
{
    int32_t ret = 0;
    DLIST_ENTRY_S *prev = NULL;
    DLIST_ENTRY_S *prev_prev = NULL;

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

/*******************************************************************************
函数名称: DListCount
功能说明: 链表中成员的个数
输入参数:
    head : 要操作的链表
输出参数: 无
返 回 值:
    >=0: 数目
说    明: 无
*******************************************************************************/
int32_t dlist_count(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);

    return (int32_t)head->num;
}

/*******************************************************************************
函数名称: DListIsEmpty
功能说明: 判断链表是否为空
输入参数:
    head : 要操作的链表
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
bool_t dlist_is_empty(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);
    ASSERT(NULL != head->head.next);

    return (head->head.next == &head->head) ? B_TRUE : B_FALSE;
}

