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
u64_t bitmap_set_first_0bit(tx_t *tx, bitmap_mgr_t *bmp, u64_t start_pos)
{
    uint8_t *bmp_buf;
    uint32_t block_cnt;
    
    block_cnt = 0;
    
    u64_t    block_id           = start_pos / bmp->bits_per_block;
    uint32_t start_bit_in_block = start_pos % bmp->bits_per_block;
    uint32_t total_bit_in_block;

    if (start_bit_in_block)
    {
        bmp_buf = tx_get_buffer(tx, block_id, 0);
        if (bmp_buf == NULL)
        {
            LOG_ERROR("tx get buffer failed\n");
            return -ERR_GET_TX_BUF;
        }

        total_bit_in_block = ((bmp->total_bits - start_pos) >= bmp->bits_per_block) ? bmp->bits_per_block : (bmp->total_bits - start_pos);
        start_bit_in_block = set_buf_first_0bit(bmp_buf, start_bit_in_block, total_bit_in_block);
        if (start_bit_in_block != INVALID_U32)
        {
            return start_bit_in_block + start_pos;
        }
        
        start_pos += total_bit_in_block;
        block_id++;

    }

    while (block_cnt < bmp->total_bit_blocks)
    {
        bmp_buf = tx_get_buffer(tx, block_id, 0);
        if (bmp_buf == NULL)
        {
            LOG_ERROR("tx get buffer failed\n");
            return -ERR_GET_TX_BUF;
        }

        



        tx_put_buffer(tx, bmp_buf);

        block_cnt++;
        block_id++;
        if (block_id >= bmp->total_bit_blocks)
        {
            block_id = 0; // 从头开始找
        }
    }

    return 0;
}

// 将所有位图设置成全0
int32_t clean_all_bits(bitmap_mgr_t *bmp)
{
    ASSERT(NULL != bmp);
    
    tx_t *tx;
    int   ret;
    char *bmp_buf;
    u64_t block_id;
    
    ret = tx_alloc(bmp->cache_mgr, &tx);
    if (ret)
    {
        LOG_ERROR("alloc tx failed(%d)\n", ret);
        return ret;
    }

    for (block_id = 0; block_id < bmp->total_bit_blocks; block_id++)
    {
        bmp_buf = tx_get_buffer(tx, block_id, F_NO_READ);
        if (bmp_buf == NULL)
        {
            LOG_ERROR("tx get buffer failed\n");
            return -ERR_GET_TX_BUF;
        }
        
        memset(bmp_buf, 0, bmp->block_size);
        tx_mark_buffer_dirty(tx, bmp_buf);
        tx_put_buffer(tx, bmp_buf);
    }

    ret = tx_commit(tx);
    if (ret)
    {
        LOG_ERROR("commit tx failed(%d)\n", ret);
        tx_cancel(tx);
        return ret;
    }

    return SUCCESS;
}

// 设置指定bit的值
int32_t bitmap_set_bit(bitmap_mgr_t *bmp, u64_t pos, bool_t value)
{
    ASSERT(NULL != bmp);
    tx_t    *tx;
    int      ret;
    uint8_t *bmp_buf;
    u64_t    block_id     = pos / bmp->bits_per_block;
    uint32_t pos_in_block = pos % bmp->bits_per_block;
    
    ret = tx_alloc(bmp->cache_mgr, &tx);
    if (ret)
    {
        LOG_ERROR("alloc tx failed(%d)\n", ret);
        return ret;
    }

    bmp_buf = tx_get_buffer(tx, block_id, 0);
    if (bmp_buf == NULL)
    {
        LOG_ERROR("tx get buffer failed\n");
        return -ERR_GET_TX_BUF;
    }
    
    if (buf_bit_is_set(bmp_buf, pos_in_block))
    {
        if (!value)
        {
            set_buf_bit(bmp_buf, pos_in_block);
            tx_mark_buffer_dirty(tx, bmp_buf);
        }
    }
    else
    {
        if (value)
        {
            clr_buf_bit(bmp_buf, pos_in_block);
            tx_mark_buffer_dirty(tx, bmp_buf);
        }
    }
    
    tx_put_buffer(tx, bmp_buf);
    ret = tx_commit(tx);
    if (ret)
    {
        LOG_ERROR("commit tx failed(%d)\n", ret);
        tx_cancel(tx);
        return ret;
    }

    return SUCCESS;
}

// 初始化内存结构
bitmap_mgr_t *init_bitmap(char *bd_name, space_ops_t *bd_ops, uint32_t block_size, u64_t total_bits)
{
    bitmap_mgr_t *bmp = NULL;

    ASSERT(bd_name != NULL);
    ASSERT(block_size >= sizeof(u64_t));
    ASSERT((block_size % sizeof(u64_t)) == 0);

    bmp = OS_MALLOC(sizeof(bitmap_mgr_t));
    if (NULL == bmp)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(bitmap_mgr_t));
        return NULL;
    }

    memset(bmp, 0, sizeof(bitmap_mgr_t));

    bmp->block_size = block_size;
    bmp->bits_per_block = block_size * BITS_PER_BYTE;
    bmp->total_bits = total_bits;
    bmp->total_bit_blocks = roundup(total_bits, bmp->bits_per_block);
    
    bmp->cache_mgr = init_cache_mgr(bd_name, block_size, bd_ops);
    if (NULL == bmp->cache_mgr)
    {
        LOG_ERROR("init cache system failed\n");
        close_bitmap(bmp);
        return NULL;
    }

    return bmp;
}


// 创建位图
bitmap_mgr_t *create_bitmap(create_bitmap_para_t *para)
{
    bitmap_mgr_t *bmp = init_bitmap(para->bd_name, para->bd_ops, para->block_size, para->total_bits);
    if (bmp == NULL)
    {
        LOG_ERROR("init bitmap failed\n");
        return NULL;
    }

    int32_t ret = clean_all_bits(bmp);
    if (ret)
    {
        LOG_ERROR("clean all bits failed(%d)\n", ret);
        close_bitmap(bmp);
        return NULL;
    }

    return bmp;
}

// 打开位图
bitmap_mgr_t *open_bitmap(open_bitmap_para_t *para)
{
    bitmap_mgr_t *bmp = init_bitmap(para->bd_name, para->bd_ops, para->block_size, para->total_bits);
    if (bmp == NULL)
    {
        LOG_ERROR("init bitmap failed\n");
        return NULL;
    }

    bmp->cur_bit = 0;

    return bmp;
}


// 关闭位图
void close_bitmap(bitmap_mgr_t *bmp)
{
    if (NULL == bmp)
    {
        return;
    }

    destroy_cache_mgr(bmp->cache_mgr);

    OS_FREE(bmp);

    return;
}

