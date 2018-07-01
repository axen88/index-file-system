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

#define U64_ALL_1     INVALID_U64

// ��ȡ��һ����Ϊ0��λ������Ϊ1�����������λ�ã�total_bitsΪ��start_bit��ʼ
static inline uint32_t set_dat_first_0bit(u64_t *dat, uint32_t start_bit, uint32_t total_bits)
{
    uint32_t i;
    
    ASSERT(start_bit + total_bits <= BITS_PER_U64);

    for (i = 0; i < total_bits; i++)
    {
        if (!GET_BIT(*dat, start_bit))
        {
            *dat = SET_BIT(*dat, start_bit);
            return start_bit;
        }

        start_bit++;
    }

    return INVALID_U32;
}

// total_bitsΪ��buf[0]��bit0��ʼ
static inline uint32_t set_buf_first_0bit(u64_t *buf, uint32_t start_bit, uint32_t total_bits)
{
    uint32_t i;
    uint32_t n;
    uint32_t left;
    uint32_t start_bit_in_u64;
    
    ASSERT(NULL != buf);

    i = start_bit >> BITS_PER_U64_SHIFT;
    start_bit_in_u64 = start_bit & MASK_N(BITS_PER_U64_SHIFT);
    n = roundup3(total_bits + start_bit_in_u64, BITS_PER_U64_SHIFT);

    if (start_bit_in_u64)
    {
        start_bit -= start_bit_in_u64;
        left = BITS_PER_U64 - start_bit_in_u64;
        start_bit_in_u64 = set_dat_first_0bit(&buf[i], start_bit_in_u64, left);
        if (start_bit_in_u64 != INVALID_U32)
        {
            return start_bit_in_u64 + start_bit;
        }
        
        start_bit += BITS_PER_U64;
        total_bits -= left;
    }

    while (i < n)
    {
        if (buf[i] != U64_ALL_1)
            break;

        i++;
        if (total_bits >= BITS_PER_U64)
        {
            total_bits -= BITS_PER_U64;
            start_bit += BITS_PER_U64;
        }
    }

    if (total_bits == 0)
        return INVALID_U32;

    left = (total_bits > BITS_PER_U64) ? BITS_PER_U64 : total_bits;
    start_bit_in_u64 = set_dat_first_0bit(&buf[i], 0, left);
    if (start_bit_in_u64 != INVALID_U32)
    {
        return start_bit_in_u64 + start_bit;
    }

    return INVALID_U32;
}

static inline void clr_buf_bit(u64_t *buf, uint32_t start_bit)
{
    ASSERT(NULL != buf);
    
    buf[start_bit >> BITS_PER_U64_SHIFT] &= ~(1 << (start_bit & MASK_N(BITS_PER_U64_SHIFT)));
}

static inline void set_buf_bit(u64_t *buf, uint32_t start_bit)
{
    ASSERT(NULL != buf);
    
    buf[start_bit >> BITS_PER_U64_SHIFT] |= (1 << (start_bit & MASK_N(BITS_PER_U64_SHIFT)));
}

// �������������
typedef struct bitmap_hnd
{
    void *cache_mgr;

    uint32_t block_size;
    uint32_t bits_per_block;      // 
    u64_t    total_bit_blocks;    // ��Щλռ�õ�block��Ŀ
    u64_t    total_bits;          // �ܹ���λ��

    u64_t    cur_bit;             // current bit
} bitmap_hnd_t;

// ��ʼ���ļ�λͼ����ϵͳ
bitmap_hnd_t *bitmap_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops, u64_t total_bits);

// �˳�λͼϵͳ
void bitmap_exit_system(bitmap_hnd_t *hnd);



#ifdef  __cplusplus
}
#endif

#endif
