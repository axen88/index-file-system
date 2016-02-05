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

            ��Ȩ����(C), 2011~2014, AXEN������
********************************************************************************
�� �� ��: OS_LIST_DOUBLE.C
��    ��: 1.00
��    ��: 2011��8��12��
��������: ˫���������
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��8��12��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/

#include "os_adapter.h"
#include "os_list_double.h"

/*******************************************************************************
��������: DListInitEntry
����˵��: ��ʼ��entry
�������:
    entry: Ҫ��ʼ����entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
*******************************************************************************/
void dlist_init_entry(DLIST_ENTRY_S * entry)
{
    ASSERT(NULL != entry);

    entry->next = entry;
    entry->prev = entry;
}

/*******************************************************************************
��������: DListInitHead
����˵��: ��ʼ��head
�������:
    head: Ҫ��ʼ����head
�������: ��
�� �� ֵ: ��
˵    ��: ��
*******************************************************************************/
void dlist_init_head(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);

    head->num = 0;
    dlist_init_entry(&head->head);
}

/*******************************************************************************
��������: add_entry
����˵��: ��entry��ӵ�������ȥ
�������:
    entry: Ҫ��ӵ�entry
    prev : Ҫ��ӵ�entry��ǰһ��entry
    next : Ҫ��ӵ�entry�ĺ�һ��entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
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
��������: remove_entry
����˵��: ɾ��entry
�������:
    entry: Ҫɾ����entry
    prev : Ҫɾ����entry��ǰһ��entry
    next : Ҫɾ����entry�ĺ�һ��entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
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
��������: DListAddHead
����˵��: �������ͷ��λ�����һ��entry
�������:
    head : Ҫ����������
    entry: Ҫ��ӵ�entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
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
��������: DListAddTail
����˵��: �������β��λ�����һ��entry
�������:
    head : Ҫ����������
    entry: Ҫ��ӵ�entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
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
��������: DListremove_entry
����˵��: ɾ��ָ��entry
�������:
    head : Ҫ����������
    entry: Ҫ��ӵ�entry
�������: ��
�� �� ֵ: ��
˵    ��: ��
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
��������: DListGetEntry
����˵��: ��ȡָ��λ�ô���entry
�������:
    head : Ҫ����������
    position   : ָ����λ��
�������: ��
�� �� ֵ:
    !=NULL: ��ȡ�ɹ�����ȡ����entryָ��
    ==NULL: ��ȡʧ��
˵    ��: ��
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
��������: DListRemoveTargetEntry
����˵��: ɾ��ָ��λ�ô���entry
�������:
    head : Ҫ����������
    position   : ָ����λ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
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
��������: DListWalkAll
����˵��: ��������
�������:
    head : Ҫ����������
    func   : ��ÿ�������Ա�����Ļص�����
    para   : �ص�����������һ������
�������: ��
�� �� ֵ:
    ==0: ȫ���������
    !=0: ��;�˳�����������ֵΪ�ص������ķ���ֵ
˵    ��: ��
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
��������: DListWalkAllReverse
����˵��: ��������
�������:
    head : Ҫ����������
    func   : ��ÿ�������Ա�����Ļص�����
    para   : �ص�����������һ������
�������: ��
�� �� ֵ:
    ==0: ȫ���������
    !=0: ��;�˳�����������ֵΪ�ص������ķ���ֵ
˵    ��: ��
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
��������: DListCount
����˵��: �����г�Ա�ĸ���
�������:
    head : Ҫ����������
�������: ��
�� �� ֵ:
    >=0: ��Ŀ
˵    ��: ��
*******************************************************************************/
int32_t dlist_count(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);

    return (int32_t)head->num;
}

/*******************************************************************************
��������: DListIsEmpty
����˵��: �ж������Ƿ�Ϊ��
�������:
    head : Ҫ����������
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
bool_t dlist_is_empty(DLIST_HEAD_S * head)
{
    ASSERT(NULL != head);
    ASSERT(NULL != head->head.next);

    return (head->head.next == &head->head) ? B_TRUE : B_FALSE;
}

