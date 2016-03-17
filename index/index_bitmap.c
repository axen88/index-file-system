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

            Copyright(C), 2016~2019, axen.hook@foxmail.com
********************************************************************************
File Name: INDEX_BITMAP.C
Author   : axen.hook
Version  : 1.00
Date     : 02/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 02/Mar/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/
#include "os_adapter.h"
#include "os_disk_if.h"
#include "index_bitmap.h"

MODULE(PID_BITMAP);
#include "os_log.h"

#define CACHE_SIZE_BY_SECTORS         8

typedef enum tagBUF_STATUS_E
{
    EMPTY = 0,  // no data
    CLEAN,      // clean data
    DIRTY       // dirty data
} BUF_STATUS_E;


int32_t flush_buffer(BITMAP_HANDLE * hnd)
{
    int32_t ret = 0;

    ASSERT(NULL != hnd);

    if (DIRTY == hnd->status)
    {
        ret = os_disk_pwrite(hnd->file_hnd, hnd->cache, hnd->dat_size,
            hnd->start_lba + (hnd->dat_addr * hnd->cache_size_by_sectors));
        if (ret != (int32_t) hnd->dat_size)
        {
            LOG_ERROR("Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
                hnd->file_hnd, hnd->cache, hnd->dat_size,
                hnd->start_lba + (hnd->dat_addr * hnd->cache_size_by_sectors), ret);
            return -FILE_BITMAP_ERR_WRITE;
        }

        hnd->status = CLEAN;
    }

    return 0;
}

int32_t pre_flush(BITMAP_HANDLE * hnd)
{
    int32_t ret = 0;
    uint8_t *buf = NULL;

    ASSERT(NULL != hnd);

    buf = OS_MALLOC(hnd->dat_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", hnd->dat_size);
        return -FILE_BITMAP_ERR_MALLOC;
    }

    memset(buf, 0xFF, hnd->dat_size);

    ret = os_disk_pwrite(hnd->file_hnd, buf, hnd->dat_size,
        hnd->start_lba + (hnd->dat_addr * hnd->cache_size_by_sectors));
    if (ret != (int32_t) hnd->dat_size)
    {
        LOG_ERROR("Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, hnd->cache, hnd->dat_size,
            hnd->start_lba + (hnd->dat_addr * hnd->cache_size_by_sectors), ret);
        OS_FREE(buf);
        return -FILE_BITMAP_ERR_WRITE;
    }

    OS_FREE(buf);
    
    return 0;
}


int32_t get_buf(BITMAP_HANDLE * hnd, uint64_t dat_addr)
{
    ASSERT(NULL != hnd);

    if ((EMPTY == hnd->status) || (dat_addr != hnd->dat_addr))
    {
        int32_t ret = flush_buffer(hnd);
        if (ret < 0)
        {
            return ret;
        }

        hnd->dat_size = (uint32_t) ((hnd->total_sectors
                - (dat_addr * hnd->cache_size_by_sectors))
            << BYTES_PER_SECTOR_SHIFT);
        if (hnd->dat_size > hnd->cache_size_by_bytes)
        {
            hnd->dat_size = hnd->cache_size_by_bytes;
        }

        if (os_disk_pread(hnd->file_hnd, hnd->cache, hnd->dat_size,
                hnd->start_lba + (dat_addr * hnd->cache_size_by_sectors)) <= 0)
        {
            LOG_ERROR("Read lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
                hnd->file_hnd, hnd->cache, hnd->dat_size,
                hnd->start_lba + (hnd->dat_addr * hnd->cache_size_by_sectors), ret);
            return -FILE_BITMAP_ERR_READ;
        }

        hnd->dat_addr = dat_addr;
        hnd->status = CLEAN;
        hnd->pre_flush = B_TRUE;
    }

    return 0;
}

uint8_t os_check_bit(uint8_t * buf, uint32_t position)
{
    ASSERT(NULL != buf);

    return buf[position >> 3] & ((uint8_t) 1 << ((uint8_t) (position & 0x07)));
}

void os_set_bit(uint8_t * buf, uint32_t position, bool_t is_used)
{
    ASSERT(NULL != buf);

    if (B_FALSE == is_used)
    {
        buf[position >> 3] &= ~(uint8_t) (1 << ((uint8_t) (position & 0x07)));
    }
    else
    {
        buf[position >> 3] |= (uint8_t) (1 << ((uint8_t) (position & 0x07)));
    }
}

void os_set_nbits(uint8_t * buf, uint32_t position, uint32_t num, bool_t is_used)
{
    ASSERT(NULL != buf);

    while (num--)
    {
        os_set_bit(buf, position++, is_used);
    }

    return;
}

int32_t init_bitmap_blocks(void * file_hnd, uint32_t block_size,
    uint64_t bitmap_start_lba, uint32_t bitmap_blocks)
{
    uint8_t *dat = NULL;
    uint32_t sectors_per_block = block_size / BYTES_PER_SECTOR;
    int32_t ret = 0;

    ASSERT(NULL != file_hnd);

    dat = OS_MALLOC(block_size);
    if (NULL == dat)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    memset(dat, 0, block_size);

    while (bitmap_blocks--)
    {
        ret = os_disk_pwrite(file_hnd, dat, block_size, bitmap_start_lba);
        if (ret != (int32_t)block_size)
        {
            LOG_ERROR( "Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
                file_hnd, dat, block_size, bitmap_start_lba, ret);
            OS_FREE(dat);
            return -FILE_BLOCK_ERR_WRITE;
        }

        bitmap_start_lba += sectors_per_block;
    }

    OS_FREE(dat);

    return 0;
}

int32_t bitmap_clean(BITMAP_HANDLE * hnd)
{
    if (NULL == hnd)
    {
        LOG_ERROR("Invalid parameter. hnd(%p)\n", hnd);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    hnd->status = EMPTY;
    hnd->pre_flush = B_FALSE;

    return init_bitmap_blocks(hnd->file_hnd, BYTES_PER_SECTOR,
        hnd->start_lba, hnd->total_sectors);
}


int32_t bitmap_init(BITMAP_HANDLE ** hnd, void * file_hnd, uint64_t start_lba, uint32_t total_sectors, uint64_t total_bits)
{
    BITMAP_HANDLE *tmp_hnd = NULL;

    if ((NULL == hnd) || (NULL == file_hnd) || (0 == total_sectors))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) file_hnd(%p) total_sectors(%d)\n",
            hnd, file_hnd, total_sectors);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    tmp_hnd = OS_MALLOC(sizeof(BITMAP_HANDLE));
    if (NULL == tmp_hnd)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(BITMAP_HANDLE));
        return -FILE_BITMAP_ERR_MALLOC;
    }

    tmp_hnd->cache_size_by_sectors = CACHE_SIZE_BY_SECTORS;
    tmp_hnd->cache_size_by_bytes = CACHE_SIZE_BY_SECTORS * BYTES_PER_SECTOR;
    tmp_hnd->cache = OS_MALLOC(tmp_hnd->cache_size_by_bytes);
    if (NULL == tmp_hnd->cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", tmp_hnd->cache_size_by_bytes);
        OS_FREE(tmp_hnd);
        return -FILE_BITMAP_ERR_MALLOC;
    }

    tmp_hnd->file_hnd = file_hnd;
    tmp_hnd->start_lba = start_lba;
    tmp_hnd->total_sectors = total_sectors;
    tmp_hnd->total_bits = total_bits;
    tmp_hnd->status = EMPTY;

    *hnd = tmp_hnd;

    return 0;
}

int32_t bitmap_destroy(BITMAP_HANDLE * hnd)
{
    if (NULL == hnd)
    {
        LOG_ERROR("Invalid parameter. hnd(%p)\n", hnd);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    (void)flush_buffer(hnd);

    if (NULL != hnd->cache)
    {
        OS_FREE(hnd->cache);
    }

    OS_FREE(hnd);

    return 0;
}

int32_t bitmap_check_bit(BITMAP_HANDLE * hnd, uint64_t position)
{
    int32_t ret = 0;

    if (NULL == hnd)
    {
        LOG_ERROR("Invalid parameter. hnd(%p)\n", hnd);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    ret = get_buf(hnd, ((position >> 3) / hnd->cache_size_by_bytes));
    if (ret < 0)
    {
        return ret;
    }

    return os_check_bit(hnd->cache,
        (uint32_t) (position - ((hnd->dat_addr * hnd->cache_size_by_bytes) << 3)));
}

int32_t bitmap_set_nbits(BITMAP_HANDLE * hnd, uint64_t start_position, uint32_t nbits,
    bool_t is_used)
{
    int32_t ret = 0;
    int32_t count = 0;

    if ((NULL == hnd) || (0 == nbits))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) nbits(%d)\n", hnd, nbits);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    while (nbits--)
    {
        ret = get_buf(hnd, ((start_position >> 3) / hnd->cache_size_by_bytes));
        if (ret < 0)
        {
            return ret;
        }

        os_set_bit(hnd->cache,
            (uint32_t) (start_position -
                ((hnd->dat_addr * hnd->cache_size_by_bytes) << 3)), is_used);
        if ((is_used) && (hnd->pre_flush))
        {
            ret = pre_flush(hnd);
            if (ret < 0)
            {
                return ret;
            }

            hnd->pre_flush = B_FALSE;
        }
        
        hnd->status = DIRTY;
        start_position++;
        count++;
    }

    return count;
}

int32_t bitmap_get_free_bits(BITMAP_HANDLE * hnd, uint64_t start_position,
    uint32_t required_bits, uint64_t * real_start_position)
{
    int32_t ret = 0;
    uint64_t total_bits = hnd->total_bits;
    int32_t real_bits = 0;

    if ((NULL == hnd) || (0 == required_bits) || (NULL == real_start_position))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) required_bits(%d) real_start_position(%p)\n",
            hnd, required_bits, real_start_position);
        return -FILE_BITMAP_ERR_INVALID_PARA;
    }

    total_bits -= start_position;

    while (total_bits--)
    {
        ret = bitmap_check_bit(hnd, start_position);
        if (0 > ret)
        {
            return ret;
        }
        else if (0 == ret)
        {
            if (0 == real_bits)
            {
                *real_start_position = start_position;
            }

            real_bits++;
            if ((uint32_t)real_bits == required_bits)
            {
                return real_bits;
            }
        }
        else
        {
            if (0 != real_bits)
            {
                return real_bits;
            }
        }

        start_position++;
    }

    return -FILE_BITMAP_ERR_0BIT_NOT_FOUND;
}

EXPORT_SYMBOL(os_check_bit);
EXPORT_SYMBOL(os_set_bit);
EXPORT_SYMBOL(os_set_nbits);

