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
MODULE(PID_INDEX);
#include "os_log.h"

int32_t index_update_block(index_handle_t * hnd, void * buf, uint32_t size,
    uint32_t start_lba, uint64_t vbn)
{
    int32_t ret = 0;
    uint64_t tmp_start_lba = 0;

    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    tmp_start_lba = start_lba + vbn * hnd->sb.sectors_per_block
        + hnd->sb.start_lba;
    ret = os_disk_pwrite(hnd->file_hnd, buf, size, tmp_start_lba);
    if (ret != (int32_t) size)
    {
        LOG_ERROR("Write lba failed. file_hnd(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, tmp_start_lba, ret);
    }

    return ret;
}

int32_t index_read_block(index_handle_t * hnd, void * buf, uint32_t size,
    uint32_t start_lba, uint64_t vbn)
{
    int32_t ret = 0;
    uint64_t tmp_start_lba = 0;

    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    tmp_start_lba = start_lba + vbn * hnd->sb.sectors_per_block +
        hnd->sb.start_lba;
    ret = os_disk_pread(hnd->file_hnd, buf, size, tmp_start_lba);
    if (ret != (int32_t) size)
    {
        LOG_ERROR("Read lba failed. file_hnd(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, tmp_start_lba, ret);
    }

    return ret;
}

void assemble_object(block_head_t * obj)
{
    uint16_t *foot = NULL; // the last 2 bytes

    ASSERT(NULL != obj);
    ASSERT(obj->alloc_size >= obj->real_size);

    foot = (uint16_t *) ((uint8_t *) obj + obj->alloc_size - sizeof(uint16_t));

    obj->fixup = *foot;
    *foot = obj->seq_no;

    return;
}

int32_t fixup_object(block_head_t * obj)
{
    uint16_t *foot = NULL; // the last 2 bytes

    ASSERT(NULL != obj);
    ASSERT(obj->alloc_size >= obj->real_size);

    foot = (uint16_t *) ((uint8_t *) obj + obj->alloc_size - sizeof(uint16_t));

    if (*foot == obj->seq_no)
    {
        *foot = obj->fixup;
    }
    else
    {
        LOG_ERROR("Found invalid object. obj(%p)\n", obj);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    obj->seq_no++;

    return 0;
}

int32_t verify_object(block_head_t * obj, uint32_t objid,
    uint32_t alloc_size)
{
    ASSERT(NULL != obj);
    ASSERT(0 != objid);
    ASSERT(0 != alloc_size);

    if (obj->blk_id != objid)
    {
        LOG_ERROR("Object id not match. obj(%p) objid(%x) expect(%x)",
            obj, obj->blk_id, objid);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if (obj->alloc_size != alloc_size)
    {
        LOG_ERROR("alloc_size not match. obj(%p) alloc_size(%d) expect(%d)",
            obj, obj->alloc_size, alloc_size);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if ((obj->real_size > obj->alloc_size)
        || (obj->real_size < sizeof(block_head_t)))
    {
        LOG_ERROR("real_size not match. obj(%p) real_size(%d) alloc_size(%d) sizeof(%d)",
            obj, obj->real_size, alloc_size, (uint32_t)sizeof(block_head_t));
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    return 0;
}

int32_t index_update_block_fixup(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;

    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    assemble_object(obj);

    ret = index_update_block(hnd, obj, obj->alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Update object failed. hnd(%p) obj(%p) ret(%d)\n",
            hnd, obj, ret);
    }

    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    return ret;
}

int32_t index_read_block_fixup(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size)
{
    int32_t ret = 0;

    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    ret = index_read_block(hnd, obj, alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Read data failed. hnd(%p) obj(%p) objid(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            hnd, obj, objid, alloc_size, vbn, ret);
        return ret;
    }

    ret = verify_object(obj, objid, alloc_size);
    if (0 > ret)
    {
        LOG_ERROR("Verify object failed. hnd(%p) obj(%p) objid(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            hnd, obj, objid, alloc_size, vbn, ret);
        return ret;
    }

    return fixup_object(obj);
}

int32_t check_obj(index_handle_t *hnd, block_head_t *obj)
{
    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if ((obj->real_size > obj->alloc_size)
        || (obj->alloc_size > hnd->sb.block_size))
    {
        LOG_ERROR("size is invalid. real_size(%d) alloc_size(%d) block_size(%d)\n",
            obj->real_size, obj->alloc_size, hnd->sb.block_size);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    return 0;
}

int32_t index_update_block_pingpong_init(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;

    ret = check_obj(hnd, obj);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. hnd(%p) buf(%p) ret(%d)", hnd, obj, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = hnd->sb.block_size;

    buf = OS_MALLOC(block_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    assemble_object(obj);

    memset(buf, 0, block_size);
    memcpy(buf + (obj->alloc_size * (obj->seq_no % (block_size / obj->alloc_size))), obj, obj->real_size);       /*lint !e414 */

    ret = index_update_block(hnd, buf, block_size, 0, vbn);

    OS_FREE(buf);
    buf = NULL;

    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    if (ret != (int32_t)block_size)
    {
        LOG_ERROR("Update block data failed. hnd(%p) obj(%p) size(%d) vbn(%lld) ret(%d)\n",
            hnd, obj, block_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

int32_t index_update_block_pingpong(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint32_t update_lba = 0;

    ret = check_obj(hnd, obj);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. hnd(%p) buf(%p) ret(%d)\n", hnd, obj, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = hnd->sb.block_size;

    assemble_object(obj);

    update_lba = (obj->alloc_size * (obj->seq_no % (block_size / obj->alloc_size))) >> BYTES_PER_SECTOR_SHIFT;    /*lint !e414 */

    ret = index_update_block(hnd, obj, obj->alloc_size, update_lba, vbn);

    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    if (ret != (int32_t) obj->alloc_size)
    {
        LOG_ERROR("Update block data failed. hnd(%p) obj(%p) size(%d) vbn(%lld) ret(%d)\n",
            hnd, obj, obj->alloc_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

block_head_t *get_last_correct_dat(uint8_t * buf, uint32_t objid,
    uint32_t alloc_size, uint32_t cnt)
{
    uint32_t i = 0;
    uint32_t prev_i = 0;
    uint16_t prev_seq_no = 0;
    int32_t ret = 0;
    block_head_t *obj = NULL;

    ASSERT(NULL != buf);
    ASSERT(0 != alloc_size);

    for (i = 0; i < cnt; i++)
    {
        obj = (block_head_t *)(buf + alloc_size * i);

        if (0 == obj->blk_id)
        { // not written yet
            break;
        }

        if ((0 != i) && ((prev_seq_no + 1) != obj->seq_no))
        { // current and next data is not the newest
            break;
        }

        prev_seq_no = obj->seq_no;
        prev_i = i;
    }

    for (i = 0; i < cnt; i++)
    {
        obj = (block_head_t *)(buf + alloc_size * prev_i);

        if (0 == obj->blk_id)
        { // not written yet
            break;
        }

        ret = verify_object(obj, objid, alloc_size);
        if (0 > ret)
        {
            LOG_ERROR("Verify object failed. buf(%p) obj(%p) objid(%x) alloc_size(%d) ret(%d)\n",
                buf, obj, objid, alloc_size, ret);
        }
        else
        {
            ret = fixup_object(obj);
            if (0 <= ret)
            {
                return obj;
            }

            LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret);
        }

        if (0 == prev_i)
        {
            prev_i = cnt;
        }

        prev_i--;
    }

    LOG_ERROR("No valid object. buf(%p) obj(%p) objid(%x) alloc_size(%d)",
        buf, obj, objid, alloc_size);

    return NULL;
}

int32_t index_read_block_pingpong(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size)
{
    int32_t ret = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;
    block_head_t * tmp_obj = NULL;

    if ((NULL == hnd) || (NULL == obj) || (0 == alloc_size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p) alloc_size(%d)\n", hnd, obj, alloc_size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    block_size = hnd->sb.block_size;

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

    ret = index_read_block(hnd, buf, block_size, 0, vbn);
    if (0 > ret)
    {
        OS_FREE(buf);
        return ret;
    }

    tmp_obj = get_last_correct_dat(buf, objid, alloc_size, block_size / alloc_size);
    if (NULL == tmp_obj)
    {
        LOG_ERROR("Get invalid object. hnd(%p) obj(%p) vbn(%lld)\n", hnd, tmp_obj, vbn);
        OS_FREE(buf);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    ASSERT(alloc_size >= tmp_obj->real_size);
    memcpy(obj, tmp_obj, tmp_obj->real_size);
    OS_FREE(buf);

    return 0;
}

int32_t index_update_sectors(index_handle_t * hnd, void * buf, uint32_t size,
    uint64_t start_lba)
{
    int32_t ret = 0;

    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    start_lba += hnd->sb.start_lba;
    ret = os_disk_pwrite(hnd->file_hnd, buf, size, start_lba);
    if (ret != (int32_t)size)
    {
        LOG_ERROR("Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, start_lba, ret);
    }

    return ret;
}

int32_t index_read_sectors(index_handle_t * hnd, void * buf, uint32_t size,
    uint64_t start_lba)
{
    int32_t ret = 0;

    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    start_lba += hnd->sb.start_lba;
    ret = os_disk_pread(hnd->file_hnd, buf, size, start_lba);
    if (ret != (int32_t)size)
    {
        LOG_ERROR("Read lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, start_lba, ret);
    }

    return ret;
}

EXPORT_SYMBOL(index_update_block);
EXPORT_SYMBOL(index_read_block);

EXPORT_SYMBOL(index_update_block_fixup);
EXPORT_SYMBOL(index_read_block_fixup);

EXPORT_SYMBOL(index_update_block_pingpong_init);
EXPORT_SYMBOL(index_update_block_pingpong);
EXPORT_SYMBOL(index_read_block_pingpong);

EXPORT_SYMBOL(index_update_sectors);
EXPORT_SYMBOL(index_read_sectors);

