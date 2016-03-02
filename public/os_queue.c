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
File Name: OS_QUEUE.C
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
#include "os_queue.h"

#ifndef __EN_LIST_QUEUE__

/*******************************************************************************
函数名称: queue_create
功能说明: 创建一个队列
输入参数:
    max_size: 要创建的队列大小
输出参数: 无
返 回 值:
    !=NULL: 成功，队列地址
    ==NULL: 失败
说    明: 无
*******************************************************************************/
QUEUE_S *queue_create(int32_t max_size)
{
    QUEUE_S *q = NULL;

    if (0 >= max_size)
    {
        return NULL;
    }

    q = (QUEUE_S *)OS_MALLOC(sizeof(QUEUE_S));
    if (NULL == q)
    {
        return NULL;
    }

    q->pullMemb = OS_MALLOC(sizeof(uint64_t) * (uint32_t)max_size);
    if (NULL == q->pullMemb)
    {
        OS_FREE(q);
        return NULL;
    }

    q->uiHead = 0;
    q->uiTail = 0;
    q->num = 0;
    q->max_member = (uint32_t)max_size;

    return q;
}

/*******************************************************************************
函数名称: queue_push
功能说明: 添加一个成员到队列尾部
输入参数:
    q     : 要操作的队列
    member: 要添加的成员
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_push(QUEUE_S *q, uint64_t member)
{
    ASSERT(q != NULL);

    if (q->num >= q->max_member)
    {
        return -ERR_QUEUE_FULL;
    }

    q->pullMemb[q->uiTail++] = member;
    if (q->uiTail >= q->max_member)
    {
        q->uiTail = 0;
    }

    q->num++;

    return 0;
}

/*******************************************************************************
函数名称: queue_pop
功能说明: 取出队列的第一个成员
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
    ASSERT(q != NULL);
    ASSERT(member != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    *member = q->pullMemb[q->uiHead++];
    if (q->uiHead >= q->max_member)
    {
        q->uiHead = 0;
    }

    q->num--;

    return 0;
}

/*******************************************************************************
函数名称: queue_pop_push
功能说明: 取出队列的第一个成员，并往队列尾部添加一个新成员
输入参数:
    q      : 要操作的队列
    member : 要添加的成员
输出参数:
    member: 取到的成员
返 回 值:
    > 0: 取出失败，添加成功
    ==0: 取出和添加都成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_pop_push(QUEUE_S *q, uint64_t member, uint64_t *member)
{
    int32_t ret = 0;
    
    ASSERT(q != NULL);
    ASSERT(member != NULL);

    if (0 == q->num)
    {
        ret = ERR_QUEUE_EMPTY; /* 返回值大于0，表明pop失败，push成功 */
    }
    else
    {
        *member = q->pullMemb[q->uiHead++];
        if (q->uiHead >= q->max_member)
        {
            q->uiHead = 0;
        }
        
        q->num--;
    }
    
    q->pullMemb[q->uiTail++] = member;
    if (q->uiTail >= q->max_member)
    {
        q->uiTail = 0;
    }

    q->num++;

    return ret;
}

/*******************************************************************************
函数名称: queue_remove_member
功能说明: 从队列中删除指定成员
输入参数:
    q      : 要操作的队列
    member : 要删除的成员
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_remove_member(QUEUE_S *q, uint64_t member)
{
    uint32_t uiCur = 0;
    uint32_t num = 0;
    uint64_t member = 0;
    
    ASSERT(q != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    uiCur = q->uiHead;
    num = q->num;
    
    while (num--)
    {
        member = q->pullMemb[uiCur];
        if (member == member)
        {
            if (uiCur == q->uiHead)
            {
                q->uiHead++;
            }
            else if (uiCur > q->uiHead)
            {
                memmove(&q->pullMemb[q->uiHead + 1], &q->pullMemb[q->uiHead],
                    (uint32_t)(uiCur - q->uiHead) * sizeof(uint64_t));
                q->uiHead++;
            }
            else
            {
                ASSERT(q->uiTail > uiCur);
                memcpy(&q->pullMemb[uiCur], &q->pullMemb[uiCur + 1],
                    (uint32_t)(q->uiTail - uiCur) * sizeof(uint64_t));
                q->uiTail--;
            }
            
            if (q->uiHead >= q->max_member)
            {
                q->uiHead = 0;
            }
            
            q->num--;
            return 0;
        }

        if (++uiCur >= q->max_member)
        {
            uiCur = 0;
        }
    }

    return -ERR_QUEUE_MEMB_NOT_FOUND;
}

/*******************************************************************************
函数名称: queue_walk_all
功能说明: 遍历整个队列，并对每个成员执行指定的操作
输入参数:
    q    : 要操作的队列
    func : 对每个成员要执行的操作
    para : 上面操作的输入参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_walk_all(QUEUE_S *q,
    int32_t (*func)(uint64_t, void *), void *para)
{
    int32_t ret = 0;
    uint32_t uiCur = 0;
    uint32_t num = 0;
    uint64_t member = 0;
    
    ASSERT(q != NULL);

    if (0 == q->num)
    {
        return -ERR_QUEUE_EMPTY;
    }

    uiCur = q->uiHead;
    num = q->num;
    
    while (num--)
    {
        member = q->pullMemb[uiCur];
        ret = func(member, para);
        if (0 != ret)
        {
            break;
        }

        if (++uiCur >= q->max_member)
        {
            uiCur = 0;
        }
    }

    return ret;
}


/*******************************************************************************
函数名称: queue_get_size
功能说明: 获取队列当前的成员数目
输入参数:
    q    : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 当前的成员数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_get_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->num;
}

/*******************************************************************************
函数名称: queue_get_max_size
功能说明: 获取队列允许的最大成员数目
输入参数:
    q    : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 最大的成员数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_get_max_size(QUEUE_S *q)
{
    ASSERT(q != NULL);

    return (int32_t)q->max_member;
}

/*******************************************************************************
函数名称: queue_clean
功能说明: 清除队列中的所有成员
输入参数:
    q    : 要操作的队列
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void queue_clean(QUEUE_S *q)
{
    ASSERT(q != NULL);

    q->uiHead = 0;
    q->uiTail = 0;
    q->num = 0;

    return;
}

/*******************************************************************************
函数名称: queue_destroy
功能说明: 销毁队列
输入参数:
    q    : 要操作的队列
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t queue_destroy(QUEUE_S *q)
{
    ASSERT(q != NULL);

    OS_FREE(q->pullMemb);
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

