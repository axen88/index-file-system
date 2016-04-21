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
File Name: INDEX_BLOCK_RW.C
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
#include "ofs_if.h"

MODULE(PID_BLOCK);
#include "os_log.h"

static void assemble_block(block_head_t *blk)
{
    uint16_t *foot = NULL; // the last 2 bytes

    ASSERT(NULL != blk);
    ASSERT(blk->alloc_size >= blk->real_size);

    foot = (uint16_t *)((uint8_t *)blk + blk->alloc_size - sizeof(uint16_t));

    blk->fixup = *foot;
    *foot = blk->seq_no;

    return;
}

static int32_t fixup_block(block_head_t *blk)
{
    uint16_t *foot = NULL; // the last 2 bytes

    ASSERT(NULL != blk);
    ASSERT(blk->alloc_size >= blk->real_size);

    foot = (uint16_t *)((uint8_t *)blk + blk->alloc_size - sizeof(uint16_t));

    if (*foot == blk->seq_no)
    {
        *foot = blk->fixup;
    }
    else
    {
        LOG_ERROR("Found invalid block. blk(%p)\n", blk);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    blk->seq_no++;

    return 0;
}

static int32_t verify_block(block_head_t *blk, uint32_t blk_id, uint32_t alloc_size)
{
    ASSERT(NULL != blk);
    ASSERT(0 != blk_id);
    ASSERT(0 != alloc_size);

    if (blk->blk_id != blk_id)
    {
        LOG_ERROR("Object id not match. blk(%p) blk_id(%x) expect(%x)",
            blk, blk->blk_id, blk_id);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if (blk->alloc_size != alloc_size)
    {
        LOG_ERROR("alloc_size not match. blk(%p) alloc_size(%d) expect(%d)",
            blk, blk->alloc_size, alloc_size);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if ((blk->real_size > blk->alloc_size) || (blk->real_size < sizeof(block_head_t)))
    {
        LOG_ERROR("real_size not match. blk(%p) real_size(%d) alloc_size(%d) sizeof(%d)",
            blk, blk->real_size, alloc_size, (uint32_t)sizeof(block_head_t));
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    return 0;
}

int32_t ofs_update_block_fixup(container_handle_t *ct, block_head_t *blk, uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;

    if ((NULL == ct) || (NULL == blk))
    {
        LOG_ERROR("Invalid parameter. ct(%p) blk(%p)\n", ct, blk);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    assemble_block(blk);

    ret = ofs_update_block(ct, blk, blk->alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Update object failed. ct(%p) blk(%p) ret(%d)\n",
            ct, blk, ret);
    }

    ret2 = fixup_block(blk);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. blk(%p) ret(%d)\n", blk, ret2);
        return ret2;
    }

    return ret;
}

int32_t ofs_read_block_fixup(container_handle_t *ct, block_head_t *blk, uint64_t vbn, uint32_t blk_id, uint32_t alloc_size)
{
    int32_t ret = 0;

    if ((NULL == ct) || (NULL == blk))
    {
        LOG_ERROR("Invalid parameter. ct(%p) blk(%p)\n", ct, blk);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    ret = ofs_read_block(ct, blk, alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Read data failed. ct(%p) blk(%p) blk_id(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            ct, blk, blk_id, alloc_size, vbn, ret);
        return ret;
    }

    ret = verify_block(blk, blk_id, alloc_size);
    if (0 > ret)
    {
        LOG_ERROR("Verify object failed. ct(%p) blk(%p) blk_id(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            ct, blk, blk_id, alloc_size, vbn, ret);
        return ret;
    }

    return fixup_block(blk);
}

static int32_t check_block(container_handle_t *ct, block_head_t *blk)
{
    if ((NULL == ct) || (NULL == blk))
    {
        LOG_ERROR("Invalid parameter. ct(%p) blk(%p)\n", ct, blk);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if ((blk->real_size > blk->alloc_size)
        || (blk->alloc_size > ct->sb.block_size))
    {
        LOG_ERROR("size is invalid. real_size(%d) alloc_size(%d) block_size(%d)\n",
            blk->real_size, blk->alloc_size, ct->sb.block_size);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    return 0;
}

int32_t ofs_update_block_pingpong_init(container_handle_t *ct, block_head_t *blk, uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;

    ret = check_block(ct, blk);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. ct(%p) buf(%p) ret(%d)", ct, blk, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = ct->sb.block_size;

    buf = OS_MALLOC(block_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    assemble_block(blk);

    memset(buf, 0, block_size);
    memcpy(buf + (blk->alloc_size * (blk->seq_no % (block_size / blk->alloc_size))), blk, blk->real_size);       /*lint !e414 */

    ret = ofs_update_block(ct, buf, block_size, 0, vbn);

    OS_FREE(buf);
    buf = NULL;

    ret2 = fixup_block(blk);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. blk(%p) ret(%d)\n", blk, ret2);
        return ret2;
    }

    if (ret != (int32_t)block_size)
    {
        LOG_ERROR("Update block data failed. ct(%p) blk(%p) size(%d) vbn(%lld) ret(%d)\n",
            ct, blk, block_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

int32_t ofs_update_block_pingpong(container_handle_t *ct, block_head_t *blk, uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint32_t update_lba = 0;

    ret = check_block(ct, blk);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. ct(%p) buf(%p) ret(%d)\n", ct, blk, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = ct->sb.block_size;

    assemble_block(blk);

    update_lba = (blk->alloc_size * (blk->seq_no % (block_size / blk->alloc_size))) >> BYTES_PER_SECTOR_SHIFT;    /*lint !e414 */

    ret = ofs_update_block(ct, blk, blk->alloc_size, update_lba, vbn);

    ret2 = fixup_block(blk);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. blk(%p) ret(%d)\n", blk, ret2);
        return ret2;
    }

    if (ret != (int32_t)blk->alloc_size)
    {
        LOG_ERROR("Update block data failed. ct(%p) blk(%p) size(%d) vbn(%lld) ret(%d)\n",
            ct, blk, blk->alloc_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

block_head_t *get_last_correct_block(uint8_t *buf, uint32_t blk_id, uint32_t alloc_size, uint32_t cnt)
{
    uint32_t i = 0;
    uint32_t prev_i = 0;
    uint16_t prev_seq_no = 0;
    int32_t ret = 0;
    block_head_t *blk = NULL;

    ASSERT(NULL != buf);
    ASSERT(0 != alloc_size);

    for (i = 0; i < cnt; i++)
    {
        blk = (block_head_t *)(buf + alloc_size * i);

        if (0 == blk->blk_id)
        { // not written yet
            break;
        }

        if ((0 != i) && ((prev_seq_no + 1) != blk->seq_no))
        { // current and next data is not the newest
            break;
        }

        prev_seq_no = blk->seq_no;
        prev_i = i;
    }

    for (i = 0; i < cnt; i++)
    {
        blk = (block_head_t *)(buf + alloc_size * prev_i);

        if (0 == blk->blk_id)
        { // not written yet
            break;
        }

        ret = verify_block(blk, blk_id, alloc_size);
        if (0 > ret)
        {
            LOG_ERROR("Verify object failed. buf(%p) blk(%p) blk_id(%x) alloc_size(%d) ret(%d)\n",
                buf, blk, blk_id, alloc_size, ret);
        }
        else
        {
            ret = fixup_block(blk);
            if (0 <= ret)
            {
                return blk;
            }

            LOG_ERROR("Fixup object failed. blk(%p) ret(%d)\n", blk, ret);
        }

        if (0 == prev_i)
        {
            prev_i = cnt;
        }

        prev_i--;
    }

    LOG_ERROR("No valid object. buf(%p) blk(%p) blk_id(%x) alloc_size(%d)",
        buf, blk, blk_id, alloc_size);

    return NULL;
}

int32_t ofs_read_block_pingpong(container_handle_t *ct, block_head_t *blk, uint64_t vbn, uint32_t blk_id, uint32_t alloc_size)
{
    int32_t ret = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;
    block_head_t * tmp_obj = NULL;

    if ((NULL == ct) || (NULL == blk) || (0 == alloc_size))
    {
        LOG_ERROR("Invalid parameter. ct(%p) blk(%p) alloc_size(%d)\n", ct, blk, alloc_size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    block_size = ct->sb.block_size;

    if (block_size < alloc_size)
    {
        LOG_ERROR("size is invalid. block_size(%d) alloc_size(%d)\n", block_size, alloc_size);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    buf = OS_MALLOC(block_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    ret = ofs_read_block(ct, buf, block_size, 0, vbn);
    if (0 > ret)
    {
        OS_FREE(buf);
        return ret;
    }

    tmp_obj = get_last_correct_block(buf, blk_id, alloc_size, block_size / alloc_size);
    if (NULL == tmp_obj)
    {
        LOG_ERROR("Get invalid object. ct(%p) blk(%p) vbn(%lld)\n", ct, tmp_obj, vbn);
        OS_FREE(buf);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    ASSERT(alloc_size >= tmp_obj->real_size);
    memcpy(blk, tmp_obj, tmp_obj->real_size);
    OS_FREE(buf);

    return 0;
}



EXPORT_SYMBOL(ofs_update_block);
EXPORT_SYMBOL(ofs_read_block);

EXPORT_SYMBOL(ofs_update_block_fixup);
EXPORT_SYMBOL(ofs_read_block_fixup);

EXPORT_SYMBOL(ofs_update_block_pingpong_init);
EXPORT_SYMBOL(ofs_update_block_pingpong);
EXPORT_SYMBOL(ofs_read_block_pingpong);

EXPORT_SYMBOL(ofs_update_sectors);
EXPORT_SYMBOL(ofs_read_sectors);

