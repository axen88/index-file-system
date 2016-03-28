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
File Name: INDEX_BLOCK_MANAGER.C
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
#include "index_if.h"
MODULE(PID_INDEX);
#include "os_log.h"

int32_t check_and_init_block_resource(BLOCK_HANDLE_S ** hnd,
    const char * path)
{
    BLOCK_HANDLE_S *tmp_hnd = NULL;

    if ((NULL == hnd) || (NULL == path))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) path(%p)\n", hnd, path);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if (strlen(path) >= FILE_NAME_SIZE)
    {
        LOG_ERROR("The path is too long. path(%s)\n", path);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    tmp_hnd = OS_MALLOC(sizeof(BLOCK_HANDLE_S));
    if (NULL == tmp_hnd)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(BLOCK_HANDLE_S));
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_hnd, 0, sizeof(BLOCK_HANDLE_S));
    strncpy(tmp_hnd->name, path, FILE_NAME_SIZE);
    OS_RWLOCK_INIT(&tmp_hnd->rwlock);

    *hnd = tmp_hnd;

    return 0;
}

int32_t check_and_set_fixup_flag(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    ASSERT(NULL != hnd);

    if ((0 == (hnd->flags & FLAG_DIRTY))
        && (0 == (hnd->sb.flags & FLAG_FIXUP)))
    {
        hnd->flags |= FLAG_DIRTY;
        hnd->sb.flags |= FLAG_FIXUP;
        ret = block_update_super_block(hnd);
        if (0 > ret)
        {
            LOG_ERROR("Update super block failed. hnd(%p) ret(%d)\n", hnd, ret);
        }
    }

    return ret;
}

int32_t block_finish_fixup(BLOCK_HANDLE_S * hnd)
{
    if (NULL == hnd)
    {
        LOG_ERROR("The parameter is invalid. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    hnd->sb.flags = 0;

    return check_and_set_fixup_flag(hnd);
}

int32_t block_create(BLOCK_HANDLE_S **hnd, const char *path,
    uint64_t total_sectors, uint32_t block_size_shift,
    uint32_t reserved_blocks, uint64_t start_lba)
{
    BLOCK_BOOT_SECTOR_S *sb = NULL;
    void *file_hnd = NULL;
    BLOCK_HANDLE_S *tmp_hnd = NULL;
    int32_t ret = 0;
    uint64_t total_blocks = 0;

    if (block_size_shift < BYTES_PER_SECTOR_SHIFT)
    {
        LOG_ERROR("The parameter is invalid. block_size_shift(%d)\n", block_size_shift);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    reserved_blocks++;
    total_blocks = total_sectors >> (block_size_shift - BYTES_PER_SECTOR_SHIFT);
    if (total_blocks <= reserved_blocks)
    {
        LOG_ERROR("The parameter is invalid. total_blocks(%lld) reserved_blocks(%d)\n",
            total_blocks, reserved_blocks);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    ret = check_and_init_block_resource(&tmp_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("Check and init resource failed. ret(%d)\n", ret);
        return ret;
    }

    sb = &tmp_hnd->sb;

    sb->head.blk_id = SUPER_BLOCK_ID;
    sb->head.real_size = SUPER_BLOCK_SIZE;
    sb->head.alloc_size = SUPER_BLOCK_SIZE;
    sb->head.seq_no = 0;
    sb->block_size_shift = block_size_shift;
    sb->block_size = 1 << block_size_shift;
    sb->sectors_per_block = sb->block_size / BYTES_PER_SECTOR;
    sb->bitmap_start_block = reserved_blocks;
    sb->bitmap_blocks =
        (uint32_t) RoundUp2(RoundUp2(total_blocks, BITS_PER_BYTE,
            BITS_PER_BYTE_SHIFT), sb->block_size, block_size_shift);
    sb->total_blocks = total_blocks;
    sb->free_blocks = total_blocks;
    sb->first_free_block = 0;
    sb->start_lba = start_lba;
    sb->version = VERSION;
    sb->flags = 0;
    sb->magic_num = BLOCK_MAGIC_NUMBER;

    ret = os_disk_create(&file_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("Create file failed. ret(%d)\n", ret);
        block_close(tmp_hnd);
        return ret;
    }

    tmp_hnd->file_hnd = file_hnd;

    ret = index_update_block_pingpong_init(tmp_hnd, &sb->head, SUPER_BLOCK_VBN);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. tmp_hnd(%p) buf(%p) vbn(%lld) ret(%d)\n",
            tmp_hnd, &sb->head, SUPER_BLOCK_VBN, ret);
        block_close(tmp_hnd);
        return ret;
    }

    *hnd = tmp_hnd;

    return 0;
}

int32_t check_super_block(BLOCK_BOOT_SECTOR_S * sb, uint64_t start_lba)
{
    ASSERT(NULL != sb);
    
    if (sb->magic_num != BLOCK_MAGIC_NUMBER)
    {
        LOG_ERROR( "magic_num not match. magic_num(%x) expect(%x)\n",
            sb->magic_num, BLOCK_MAGIC_NUMBER);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->version != VERSION)
    {
        LOG_ERROR("version not match. version(%04d) expect(%04d)\n", sb->version, VERSION);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->sectors_per_block != SECTORS_PER_BLOCK)
    {
        LOG_ERROR("sectors_per_block not match. sectors_per_block(%d) expect(%d)\n",
            sb->sectors_per_block, SECTORS_PER_BLOCK);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->block_size != BYTES_PER_BLOCK)
    {
        LOG_ERROR("block_size not match. block_size(%d) expect(%d)\n",
            sb->block_size, BYTES_PER_BLOCK);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->start_lba != start_lba)
    {
        LOG_ERROR("start_lba changed. old(%lld) new(%lld)\n", sb->start_lba, start_lba);
        sb->start_lba = start_lba;
    }

    return 0;
}

int32_t block_open(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t start_lba)
{
    BLOCK_BOOT_SECTOR_S *sb = NULL;
    void *file_hnd = NULL;
    BLOCK_HANDLE_S *tmp_hnd = NULL;
    int32_t ret = 0;

    ret = check_and_init_block_resource(&tmp_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("check_and_init_block_resource failed. ret(%d)\n", ret);
        return ret;
    }

    ret = os_disk_open(&file_hnd, path);
    if (ret < 0)
    {
        LOG_ERROR("Open file failed. path(%s) ret(%d)\n", path, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }

    tmp_hnd->file_hnd = file_hnd;
    sb = &tmp_hnd->sb;

    sb->sectors_per_block = SECTORS_PER_BLOCK;
    sb->block_size = BYTES_PER_BLOCK;
    sb->start_lba = start_lba;

    ret = index_read_block_pingpong(tmp_hnd, &sb->head, SUPER_BLOCK_VBN,
        SUPER_BLOCK_ID, SUPER_BLOCK_SIZE);
    if (0 > ret)
    {
        LOG_ERROR("Read lba failed. tmp_hnd(%p) sb(%p) vbn(%lld) ret(%d)\n",
            tmp_hnd, sb, SUPER_BLOCK_VBN, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }

    ret = check_super_block(sb, start_lba);
    if (0 > ret)
    {
        LOG_ERROR("Check super block failed. name(%s) start_lba(%lld) ret(%d)\n",
            path, start_lba, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }
    
    if ((0 == (sb->flags & FLAG_FIXUP)) && (0 == sb->free_blocks))
    {
        tmp_hnd->flags |= FLAG_NOSPACE;
    }

    *hnd = tmp_hnd;

    return 0;
}

bool_t block_need_fixup(BLOCK_HANDLE_S * hnd)
{
    ASSERT(NULL != hnd);

    return (((BLOCK_HANDLE_S *) hnd)->sb.flags & FLAG_FIXUP)
        ? B_TRUE : B_FALSE;
}

int32_t block_close(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    if (NULL == hnd)
    {
        LOG_ERROR("The parameter is invalid. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if (0 != (hnd->flags & FLAG_DIRTY))
    {
        hnd->flags &= ~FLAG_DIRTY;
        hnd->sb.flags &= ~FLAG_FIXUP;
        ret = block_update_super_block(hnd);
        if (0 > ret)
        {
            LOG_ERROR("Update super block failed. hnd(%p) ret(%d)\n", hnd, ret);
        }
    }

    if (NULL != hnd->file_hnd)
    {
        (void) os_disk_close(hnd->file_hnd);
        hnd->file_hnd = NULL;
    }

    OS_RWLOCK_DESTROY(&hnd->rwlock);
    OS_FREE(hnd);

    return 0;
}

int32_t block_update_super_block(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    if (NULL == hnd)
    {
        LOG_ERROR("Invalid parameter. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    ret = index_update_block_pingpong(hnd, &hnd->sb.head, SUPER_BLOCK_VBN);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. hnd(%p) vbn(%lld) ret(%d)\n",
            hnd, SUPER_BLOCK_VBN, ret);
    }

    return ret;
}

