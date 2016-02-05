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
�� �� ��: INDEX_OP.H
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

#ifndef __INDEX_OP_H__
#define __INDEX_OP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* �������� */
extern int32_t index_open(const char *index_name, uint64_t start_lba, INDEX_HANDLE **index);

/* ���������� */
extern int32_t index_create(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    INDEX_HANDLE **index);

/* ���������� */
extern int32_t index_expand(ATTR_HANDLE *tree, uint64_t v_ullAdditionalSectors);

/* �ر������� */
extern int32_t index_close(INDEX_HANDLE *index);


extern int32_t index_get_opened_attr_num(ATTR_HANDLE * tree);

extern INDEX_HANDLE *index_find_handle(const char * index_name);


/* ����������ϵͳ��ʼ�����˳��ӿڣ��̲߳���ȫ */
extern int32_t index_init_system(void);
extern void index_exit_system(void);



#ifdef __cplusplus
}
#endif

#endif

