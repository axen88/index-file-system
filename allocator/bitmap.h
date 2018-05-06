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
�� �� ��: BITMAP.H
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

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "tx_cache.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define BITS_PER_U64    64

// ���ĳ��λ�Ƿ�Ϊ1
static inline bool_t check_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    return (*dat & ((u64_t)1 << (pos & (BITS_PER_U64 - 1)))) ? TRUE : FALSE;
}

// ����ĳ��λΪ1
static inline void set_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    *dat |= ((u64_t)1 << (pos & (BITS_PER_U64 - 1)));
}

// ����ĳ��λΪ0
static inline void clr_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    *dat &= ~((u64_t)1 << (pos & (BITS_PER_U64 - 1)));
}

// ��ȡ��һ����Ϊ0��λ������Ϊ1�����������λ��
static inline uint32_t set_dat_first_0bit(u64_t *dat, uint32_t start_pos)
{
    uint32_t i;
    
    ASSERT(start_pos < BITS_PER_U64);

    for (i = start_pos; i < BITS_PER_U64; i++)
    {
        if (!check_bit(dat, i))
        {
            set_bit(dat, i);
            return i;
        }
    }

    ASSERT(0);

    return INVALID_U32;
}

// �������������
typedef struct bitmap_hnd
{
    void *cache_mgr;

    uint32_t block_size;
    uint32_t bits_per_block;   // 
    u64_t total_bit_blocks;        // ��Щλռ�õ�block��Ŀ
    u64_t total_bits;          // �ܹ���λ��

    u64_t cur_bit;           // current bit
} bitmap_hnd_t;

// ��ʼ���ļ�λͼ����ϵͳ
bitmap_hnd_t *bitmap_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops, u64_t total_bits);

// �˳�λͼϵͳ
void bitmap_exit_system(bitmap_hnd_t *hnd);



#ifdef  __cplusplus
}
#endif

#endif
