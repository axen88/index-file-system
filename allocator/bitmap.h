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
文 件 名: BITMAP.H
版    本: 1.00
日    期: 2011年6月19日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "tx_cache.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define U64_ALL_1     INVALID_U64

// 获取第一个不为0的位，设置为1，并返回这个位置；total_bits为从start_bit开始
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

// total_bits为从buf[0]的bit0开始
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

// 可以用来管理块
typedef struct bitmap_hnd
{
    void *cache_mgr;

    uint32_t block_size;
    uint32_t bits_per_block;      // 
    u64_t    total_bit_blocks;    // 这些位占用的block数目
    u64_t    total_bits;          // 总共的位数

    u64_t    cur_bit;             // current bit
} bitmap_hnd_t;

// 初始化文件位图缓存系统
bitmap_hnd_t *bitmap_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops, u64_t total_bits);

// 退出位图系统
void bitmap_exit_system(bitmap_hnd_t *hnd);



#ifdef  __cplusplus
}
#endif

#endif
