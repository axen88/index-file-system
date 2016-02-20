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

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: INDEX_ATTR.H
��    ��: 1.00
��    ��: 2012��6��13��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2012��6��13��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/

#ifndef __INDEX_ATTR_H__
#define __INDEX_ATTR_H__
    
/* �ر����� */
extern int32_t index_close_attr(ATTR_HANDLE *attr);

/* ������ */
extern int32_t index_pread_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size);

/* д���� */
extern int32_t index_pwrite_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size);

/* �޸����Դ�С */
extern int32_t index_truncate_attr(ATTR_HANDLE *attr, uint64_t new_size);

/* ȡ�����Ե��޸� */
extern void index_cancel_attr_modification(ATTR_INFO *attr_info);

/* �ύ���Ե��޸� */
extern int32_t index_commit_attr_modification(ATTR_INFO *attr_info);

/* ������ */
extern int32_t index_open_attr(struct _OBJECT_HANDLE *obj, ATTR_HANDLE **attr);
extern int32_t validate_attr(ATTR_INFO *attr_info);
extern void init_attr_info(struct _OBJECT_HANDLE *obj, ATTR_RECORD *attr_record, ATTR_INFO *attr_info);


#endif

