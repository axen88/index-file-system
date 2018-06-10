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
文 件 名: BITMAP.C
版    本: 1.00
日    期: 2011年6月19日
功能描述: 文件中某一个bitmap区域的操作
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "os_adapter.h"
#include "globals.h"
#include "bitmap.h"

MODULE(PID_BITMAP);
#include "log.h"


// 查询文件中位图区域为0的区域
u64_t bitmap_set_first_0bit(bitmap_hnd_t *hnd, u64_t start_pos)
{
    ASSERT(NULL != hnd);
    cache_mgr_t *mgr = hnd->cache_mgr;
    tx_t *tx;
    int ret;
    u64_t *tx_buf;
    u64_t block_id;
    uint32_t block_cnt;
    
    ret = tx_alloc(mgr, &tx);
    if (ret)
        return ret;

    block_cnt = 0;
    block_id = start_pos / hnd->bits_per_block;
    while (block_cnt < hnd->total_bit_blocks)
    {
        tx_buf = tx_get_buffer(tx, block_id);



        tx_put_buffer(tx, tx_buf);

        block_cnt++;
        block_id++;
        if (block_id >= hnd->total_bit_blocks)
        {
            block_id = 0; // 从头开始找
        }
    }

    tx_commit(tx);

    return 0;
}

// 将所有位图设置成全0
int32_t clean_all_bits(bitmap_hnd_t *hnd)
{
    ASSERT(NULL != hnd);
    cache_mgr_t *mgr = hnd->cache_mgr;
    tx_t *tx;
    int ret;
    char *tx_buf;
    u64_t block_id;
    
    ret = tx_alloc(mgr, &tx);
    if (ret)
        return ret;

    for (block_id = 0; block_id < hnd->total_bit_blocks; block_id++)
    {
        tx_buf = tx_get_buffer(tx, block_id);
        memset(tx_buf, 0, hnd->block_size);
        tx_mark_buffer_dirty(tx, tx_buf);
        tx_put_buffer(tx, tx_buf);
    }

    tx_commit(tx);

    return 0;
}

// 设置内存中多个位为指定值
void set_nbits(u64_t *buf, uint32_t pos, uint32_t num, bool_t value)
{
    ASSERT(NULL != buf);

    while (num--)
    {
        if (value)
        {
            set_bit(buf, pos++);
        }
        else
        {
            clr_bit(buf, pos++);
        }
    }

    return;
}

// 设置指定bit的值
int32_t bitmap_set_bit(bitmap_hnd_t *hnd, u64_t pos, bool_t value)
{
    ASSERT(NULL != hnd);
    cache_mgr_t *mgr = hnd->cache_mgr;
    tx_t *tx;
    int ret;
    u64_t *tx_buf;
    u64_t block_id = roundup(pos, hnd->bits_per_block);
    
    ret = tx_alloc(mgr, &tx);
    if (ret)
        return ret;

    tx_buf = tx_get_buffer(tx, block_id);
    set_nbits(tx_buf, pos % hnd->bits_per_block, 1, value);
    tx_mark_buffer_dirty(tx, tx_buf);
    tx_put_buffer(tx, tx_buf);

    tx_commit(tx);

    return 0;
}


// 初始化位图系统
bitmap_hnd_t *bitmap_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops, u64_t total_bits)
{
    bitmap_hnd_t *hnd = NULL;

    ASSERT(bd_name != NULL);
    ASSERT(block_size >= sizeof(u64_t));
    ASSERT((block_size % sizeof(u64_t)) == 0);

    hnd = OS_MALLOC(sizeof(bitmap_hnd_t));
    if (NULL == hnd)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(bitmap_hnd_t));
        return NULL;
    }

    memset(hnd, 0, sizeof(bitmap_hnd_t));

    hnd->block_size = block_size;
    hnd->bits_per_block = block_size * BITS_PER_BYTE;
    hnd->total_bits = total_bits;
    hnd->total_bit_blocks = roundup(total_bits, hnd->bits_per_block);
    
    hnd->cache_mgr = tx_cache_init_system(bd_name, block_size, bd_ops);
    if (NULL == hnd->cache_mgr)
    {
        LOG_ERROR("init cache system failed\n");
        OS_FREE(hnd);
        return NULL;
    }

    return hnd;
}

// 退出位图系统
void bitmap_exit_system(bitmap_hnd_t *hnd)
{
    if (NULL == hnd)
    {
        return;
    }

    tx_cache_exit_system(hnd->cache_mgr);

    OS_FREE(hnd);

    return;
}

