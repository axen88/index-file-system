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
文 件 名: OS_LIST_QUEUE.C
版    本: 1.00
日    期: 2011年8月12日
功能描述: 建立在链表上的队列程序
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月12日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "os_adapter.h"
#include "os_queue.h"
#include "os_list_double.h"

#ifdef __EN_LIST_QUEUE__


#define LIST_WALK_FINISHED 1

typedef struct tagQUEUE_ENTRY_S
{
    DLIST_ENTRY_S entry;
    uint64_t member;
} QUEUE_ENTRY_S;

typedef struct tagCALLBACK_PARA_S
{
    QUEUE_S *q;
    uint64_t member;
} CALLBACK_PARA_S;

typedef struct tagWALK_CALL_BACK_PARA_S
{
    int32_t (*func)(uint64_t, void *);
    void *para;
} WALK_CALL_BACK_PARA_S;

/*******************************************************************************
函数名称: queue_create
功能说明: 创建队列
输入参数:
    max_size: 队列可容纳成员的大小
输出参数: 无
返 回 值:
    !=NULL: 创建成功，队列的操作句柄
    ==NULL: 创建失败
说    明: 无
*******************************************************************************/
QUEUE_S *queue_create(int32_t max_size)
{
    QUEUE_S *q = NULL;

    q = (QUEUE_S *)OS_MALLOC(sizeof(QUEUE_S));
    if (NULL == q)
    {
        return NULL;
    }

    dlist_init_head(&q->head);
    q->max_size = (uint32_t)max_size;

    return q;
}

/*******************************************************************************
函数名称: queue_push
功能说明: 将成员放入队列尾
输入参数:
    q     : 要操作的队列
    member: 要放入到队列中的成员
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_push(QUEUE_S *q, uint64_t member)
{
    QUEUE_ENTRY_S *entry = NULL;
    
    ASSERT(q != NULL);

    if (q->head.num >= q->max_size)
    {
        return -ERR_QUEUE_FULL;
    }

    entry = OS_MALLOC(sizeof(QUEUE_ENTRY_S));
    if (NULL == entry)
    {
        return -ERR_QUEUE_MALLOC;
    }

    entry->member = member;
    dlist_add_tail(&q->head, &entry->entry);

    return 0;
}

/*******************************************************************************
函数名称: queue_pop
功能说明: 从队列取出一个成员
输入参数:
    q      : 要操作的队列
输出参数:
    member: 取到的成员
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_pop(QUEUE_S *q, uint64_t *member)
{
    QUEUE_ENTRY_S *entry = NULL;
    
    ASSERT(q != NULL);
    ASSERT(member != NULL);

    if (0 == q->head.num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    entry = OS_CONTAINER(q->head.head.next, QUEUE_ENTRY_S, entry);
    *member = entry->member;
    dlist_remove_entry(&q->head, &entry->entry);
    OS_FREE(entry);

    return 0;
}

/*******************************************************************************
函数名称: queue_pop_push
功能说明: 从队列取出一个成员，并放入一个新的成员
输入参数:
    q         : 要操作的队列
    push_member: 要放入的成员
输出参数:
    pop_member: 取到的成员
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_pop_push(QUEUE_S *q, uint64_t push_member,
    uint64_t *pop_member)
{
    int32_t ret = 0;
    QUEUE_ENTRY_S *entry = NULL;
    
    ASSERT(q != NULL);
    ASSERT(pop_member != NULL);

    if (0 == q->head.num)
    {
        ret = ERR_QUEUE_EMPTY;
        entry = OS_MALLOC(sizeof(QUEUE_ENTRY_S));
        if (NULL == entry)
        {
            return -ERR_QUEUE_MALLOC;
        }
    }
    else
    {
        entry = OS_CONTAINER(q->head.head.next, QUEUE_ENTRY_S, entry);
        *pop_member = entry->member;
        dlist_remove_entry(&q->head, &entry->entry);
    }
    
    entry->member = push_member;
    dlist_add_tail(&q->head, &entry->entry);

    return ret;
}

/*******************************************************************************
函数名称: compare_and_remove_member
功能说明: 比较并删除一个成员
输入参数:
    entry: 成员所在的链表entry
    para   : 参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int32_t compare_and_remove_member(void *para, DLIST_ENTRY_S *entry)
{
    QUEUE_ENTRY_S *tmp_entry = NULL;
    CALLBACK_PARA_S *tmp_para = para;

    ASSERT(NULL != entry);
    ASSERT(NULL != para);

    tmp_entry = OS_CONTAINER(entry, QUEUE_ENTRY_S, entry);

    if (tmp_entry->member == tmp_para->member)
    {
        dlist_remove_entry(&tmp_para->q->head, &tmp_entry->entry);
        OS_FREE(tmp_entry);
        return LIST_WALK_FINISHED;
    }

    return 0;
}

/*******************************************************************************
函数名称: queue_remove_member
功能说明: 从队列中删除指定的成员
输入参数:
    q     : 要操作的队列
    member: 要删除的成员
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_remove_member(QUEUE_S *q, uint64_t member)
{
    int32_t ret = 0;
    CALLBACK_PARA_S para;
    
    ASSERT(q != NULL);

    if (0 == q->head.num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    para.q = q;
    para.member = member;
    ret = dlist_walk_all(&q->head, compare_and_remove_member, &para);

    return ret;
}

/*******************************************************************************
函数名称: walk_call_back
功能说明: 队列遍历的回调函数
输入参数:
    entry : 成员所在的链表entry
    para    : 回调参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int32_t walk_call_back(void *para, DLIST_ENTRY_S *entry)
{
    WALK_CALL_BACK_PARA_S *tmp_para = NULL;
    QUEUE_ENTRY_S *tmp_entry = NULL;

    ASSERT(NULL != entry);
    ASSERT(NULL != para);

    tmp_para = para;
    tmp_entry = OS_CONTAINER(entry, QUEUE_ENTRY_S, entry);

    return tmp_para->func(tmp_entry->member, tmp_para->para);
}

/*******************************************************************************
函数名称: queue_walk_all
功能说明: 遍历队列中的每个成员
输入参数:
    q   : 要操作的队列
    func: 对每个队列成员操作的回调函数
    para: 回调函数的其中一个参数
输出参数: 无
返 回 值:
    ==0: 全部遍历完成
    !=0: 中途退出遍历，返回值为回调函数的返回值
说    明: 无
*******************************************************************************/
int32_t queue_walk_all(QUEUE_S *q,
    int32_t (*func)(uint64_t, void *), void *para)
{
    int32_t ret = 0;
    WALK_CALL_BACK_PARA_S tmp_para;
    
    ASSERT(q != NULL);

    if (0 == q->head.num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    tmp_para.func = func;
    tmp_para.para = para;
    ret = dlist_walk_all(&q->head, walk_call_back, &tmp_para);

    return ret;
}


/*******************************************************************************
函数名称: queue_get_size
功能说明: 获取队列的长度
输入参数:
    q   : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 队列的数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_get_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->head.num;
}

/*******************************************************************************
函数名称: queue_get_max_size
功能说明: 获取队列的最大大小
输入参数:
    q   : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 队列的最大数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_get_max_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->max_size;
}

/*******************************************************************************
函数名称: remove_member_call_back
输入参数:
    entry : 成员所在的链表entry
    para    : 回调参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t remove_member_call_back(void *para, DLIST_ENTRY_S *entry)
{
    QUEUE_ENTRY_S *tmp_entry = NULL;
    QUEUE_S *q = para;

    ASSERT(NULL != entry);

    tmp_entry = OS_CONTAINER(entry, QUEUE_ENTRY_S, entry);

    dlist_remove_entry(&q->head, &tmp_entry->entry);
    OS_FREE(tmp_entry);

    return 0;
}

/*******************************************************************************
函数名称: queue_clean
功能说明: 清除队列中的所有成员
输入参数:
    q   : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
void queue_clean(QUEUE_S *q)
{
    ASSERT(q != NULL);

    if (0 == q->head.num)
    {
        return;
    }

    (void)dlist_walk_all(&q->head, remove_member_call_back, q);

    return;
}

/*******************************************************************************
函数名称: queue_destroy
功能说明: 销毁队列
输入参数:
    q   : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_destroy(QUEUE_S *q)
{
    ASSERT(q != NULL);

    queue_clean(q);
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

