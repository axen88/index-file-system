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

#define BITS_PER_U64    64

// 检查某个位是否为1
static inline bool_t check_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    return (*dat & ((u64_t)1 << (pos & (BITS_PER_U64 - 1)))) ? TRUE : FALSE;
}

// 设置某个位为1
static inline void set_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    *dat |= ((u64_t)1 << (pos & (BITS_PER_U64 - 1)));
}

// 设置某个位为0
static inline void clr_bit(u64_t *dat, uint32_t pos)
{
    ASSERT(pos < BITS_PER_U64);
    *dat &= ~((u64_t)1 << (pos & (BITS_PER_U64 - 1)));
}

// 获取第一个不为0的位，设置为1，并返回这个位置
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

// 可以用来管理块
typedef struct bitmap_hnd
{
    void *cache_mgr;

    uint32_t block_size;
    uint32_t bits_per_block;   // 
    u64_t total_bit_blocks;        // 这些位占用的block数目
    u64_t total_bits;          // 总共的位数

    u64_t cur_bit;           // current bit
} bitmap_hnd_t;

// 初始化文件位图缓存系统
bitmap_hnd_t *bitmap_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops, u64_t total_bits);

// 退出位图系统
void bitmap_exit_system(bitmap_hnd_t *hnd);



#ifdef  __cplusplus
}
#endif

#endif
