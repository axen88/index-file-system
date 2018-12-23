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

#define U8_ALL_1     INVALID_U8

// 获取第一个不为0的位，设置为1，并返回这个位置
static inline uint8_t set_dat_first_0bit(uint8_t *dat, uint8_t start_bit, uint8_t total_bits)
{
    ASSERT(start_bit < total_bits);

    while (start_bit < total_bits)
    {
        if (!GET_BIT(*dat, start_bit))
        {
            SET_BIT(*dat, start_bit);
            return start_bit;
        }

        start_bit++;
    }

    return INVALID_U8;
}

// total_bits为从buf[0]的bit0开始
static inline uint32_t set_buf_first_0bit(uint8_t *buf, uint32_t start_bit, uint32_t total_bits)
{
    uint8_t total_bit_in_u8;
    uint8_t start_bit_in_u8;
    
    ASSERT(NULL != buf);
    
    if (start_bit >= total_bits)
    {
        return INVALID_U32;
    }

    buf += (start_bit >> 3);
    start_bit_in_u8 = start_bit & 7;

    if (start_bit_in_u8)
    {
        total_bit_in_u8 = ((total_bits - start_bit) >= 8) ? 8 : (total_bits - start_bit);
        start_bit_in_u8 = set_dat_first_0bit(buf, start_bit_in_u8, total_bit_in_u8);
        if (start_bit_in_u8 != INVALID_U8)
        {
            return (start_bit & ~(uint32_t)7) + start_bit_in_u8;
        }
        
        start_bit = (start_bit & ~(uint32_t)7) + start_bit_in_u8;
        buf++;
    }

    while (start_bit < total_bits)
    {
        if (*buf != U8_ALL_1)
        {
            break;
        }

        start_bit += 8;
        buf++;
    }

    if (start_bit >= total_bits)
    {
        return INVALID_U32;
    }

    start_bit_in_u8 = start_bit & 7;
    total_bit_in_u8 = ((total_bits - start_bit) >= 8) ? 8 : (total_bits - start_bit);
    start_bit_in_u8 = set_dat_first_0bit(buf, start_bit_in_u8, total_bit_in_u8);
    if (start_bit_in_u8 != INVALID_U8)
    {
        return start_bit_in_u8 + start_bit;
    }

    return INVALID_U32;
}

static inline void clr_buf_bit(uint8_t *buf, uint32_t pos)
{
    ASSERT(NULL != buf);
    
    buf[pos >> 3] &= ~(1 << (pos & 7));
}

static inline void set_buf_bit(uint8_t *buf, uint32_t pos)
{
    ASSERT(NULL != buf);
    
    buf[pos >> 3] |= (1 << (pos & 7));
}

static inline bool_t buf_bit_is_set(uint8_t *buf, uint32_t pos)
{
    ASSERT(NULL != buf);
    
    return (buf[pos >> 3] & (1 << (pos & 7)));
}


// 可以用来管理块
typedef struct bitmap_hnd
{
    void     *cache_mgr;

    uint32_t  block_size;
    uint32_t  bits_per_block;      // 
    u64_t     total_bit_blocks;    // 这些位占用的block数目
    u64_t     total_bits;          // 总共的位数

    u64_t     cur_bit;             // current bit
} bitmap_mgr_t;

typedef struct
{
    char        *bd_name;
    space_ops_t *bd_ops;
    uint32_t     block_size;
    u64_t        total_bits;
} create_bitmap_para_t;

typedef struct
{
    char        *bd_name;
    space_ops_t *bd_ops;
    uint32_t     block_size;
    u64_t        total_bits;
} open_bitmap_para_t;

// 创建位图
bitmap_mgr_t *create_bitmap(create_bitmap_para_t *para);

// 打开位图
bitmap_mgr_t *open_bitmap(open_bitmap_para_t *para);

// 关闭位图
void close_bitmap(bitmap_mgr_t *hnd);



#ifdef  __cplusplus
}
#endif

#endif
