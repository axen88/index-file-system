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
�� �� ��: OS_FILE_BITMAP.H
��    ��: 1.00
��    ��: 2011��6��19��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��6��19��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/

#ifndef __OS_FILE_BITMAP_H__
#define __OS_FILE_BITMAP_H__

#ifdef  __cplusplus
extern "C"
{
#endif

typedef struct _BITMAP_HANDLE
{
    void *cache;
    uint32_t cache_size_by_bytes;   /* ����Ĵ�С�����ֽ�Ϊ��λ */
    uint32_t cache_size_by_sectors; /* ����Ĵ�С����sectorΪ��λ */
    uint32_t dat_size;           /* ���������ݵĴ�С */
    u64_t dat_addr;           /* �ڴ��е�����λ�ã���0��ʼ����cache��С��� */
    uint8_t status;        /* cache��״̬ */
    bool_t pre_flush;         /* �Ƿ���Ҫ��Ԥˢ�̶��� */

    void *file_hnd;               /* bitmap���ڵ��ļ�������� */
    u64_t start_lba;           /* bitmap�������ļ��е���ʼλ�� */
    uint32_t total_sectors;       /* bitmap����Ĵ�С */

    u64_t total_bits;          /* bitmap���������ܿ��� */
} BITMAP_HANDLE;

extern int32_t bitmap_init(BITMAP_HANDLE ** hnd, void * file_hnd, u64_t start_lba, uint32_t total_sectors, u64_t total_bits);
extern int32_t bitmap_destroy(BITMAP_HANDLE * hnd);
extern int32_t bitmap_set_nbits(BITMAP_HANDLE * hnd, u64_t start_position,
    uint32_t nbits, bool_t is_used);
extern int32_t bitmap_get_free_bits(BITMAP_HANDLE * hnd, u64_t start_position,
    uint32_t required_bits, u64_t * real_start_position);
extern int32_t bitmap_clean(BITMAP_HANDLE * hnd);
extern int32_t bitmap_check_bit(BITMAP_HANDLE * hnd, u64_t position);

#ifdef  __cplusplus
}
#endif

#endif
