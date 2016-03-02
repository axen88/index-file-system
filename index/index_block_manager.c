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

            Copyright(C), 2016~2019, axen2012@qq.com
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

/*******************************************************************************
函数名称: check_and_init_block_resource
功能说明: 检查参数有效性，并申请操作块管理系统需要的资源
输入参数:
    path: 文件名
输出参数:
    hnd : 申请到的块管理系统操作句柄
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t check_and_init_block_resource(BLOCK_HANDLE_S ** hnd,
    const char * path)
{
    BLOCK_HANDLE_S *tmp_hnd = NULL;

    /* 检查输入参数 */
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

    /* 分配内存 */
    tmp_hnd = OS_MALLOC(sizeof(BLOCK_HANDLE_S));
    if (NULL == tmp_hnd)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(BLOCK_HANDLE_S));
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    /* 初始化内存 */
    memset(tmp_hnd, 0, sizeof(BLOCK_HANDLE_S));
    strncpy(tmp_hnd->name, path, FILE_NAME_SIZE);
    OS_RWLOCK_INIT(&tmp_hnd->rwlock);

    *hnd = tmp_hnd;

    return 0;
}

/*******************************************************************************
函数名称: check_and_set_fixup_flag
功能说明: 设置一致性检查标记到硬盘
输入参数:
    hnd : 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t check_and_set_fixup_flag(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    ASSERT(NULL != hnd);

    if ((0 == (hnd->flags & FLAG_DIRTY))
        && (0 == (hnd->sb.flags & FLAG_FIXUP)))
    {   /* 打开时是准确的 */
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

/*******************************************************************************
函数名称: block_reset_bitmap
功能说明: 回复位图区域到格式化后的状态
输入参数:
    hnd : 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_reset_bitmap(BLOCK_HANDLE_S * hnd)
{
    uint32_t reserved_blocks = 0;
    int32_t ret = 0;

    /* 检查输入参数 */
    if (NULL == hnd)
    {
        LOG_ERROR("The parameter is invalid. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    hnd->sb.free_blocks = hnd->sb.total_blocks;
    hnd->sb.first_free_block = 0;

    /* 将位图设置成全0 */
    ret = bitmap_clean(hnd->bitmap_hnd);
    if (0 > ret)
    {
        LOG_ERROR("bitmap_clean failed. bitmap_hnd(%p) ret(%d)\n", hnd->bitmap_hnd, ret);
        return ret;
    }

    reserved_blocks = (uint32_t) (hnd->sb.bitmap_start_block
        + hnd->sb.bitmap_blocks);

    /* 将位图区域占用的块设为已占用 */
    ret = block_set_status(hnd, (uint64_t) 0, reserved_blocks, B_TRUE);
    if (ret != (int32_t) reserved_blocks)
    {
        LOG_ERROR("Set blocks status failed. hnd(%p) reserved_blocks(%d) ret(%d)\n",
            hnd, reserved_blocks, ret);
        return -FILE_BLOCK_ERR_BITMAP_BLOCKS_ALLOC;
    }

    return 0;
}

/*******************************************************************************
函数名称: block_finish_fixup
功能说明: 修复后需调用此函数
输入参数:
    hnd : 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_finish_fixup(BLOCK_HANDLE_S * hnd)
{
    /* 检查输入参数 */
    if (NULL == hnd)
    {
        LOG_ERROR("The parameter is invalid. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    hnd->sb.flags = 0;

    /* 设置重启时需检查一致性标记 */
    return check_and_set_fixup_flag(hnd);
}

/*******************************************************************************
函数名称: block_create
功能说明: 创建块管理系统
输入参数:
    path           : 块管理系统所在的文件名
    total_sectors  : 需要管理的总扇区数目
    block_size_shift : 以字节为单位的块大小的幂
    reserved_blocks : 保留扇区数
    start_lba      : 块管理区域的起始lba地址
输出参数:
    hnd              : 申请到的块管理系统操作句柄
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_create(BLOCK_HANDLE_S **hnd, const char *path,
    uint64_t total_sectors, uint32_t block_size_shift,
    uint32_t reserved_blocks, uint64_t start_lba)
{
    BLOCK_BOOT_SECTOR_S *sb = NULL;
    void *file_hnd = NULL;
    BITMAP_HANDLE *bitmap_hnd = NULL;
    BLOCK_HANDLE_S *tmp_hnd = NULL;
    int32_t ret = 0;
    uint64_t total_blocks = 0;
    uint64_t bitmap_start_lba = 0;
    uint32_t bitmap_total_sectors = 0;

    /* 检查输入参数 */
    if (block_size_shift < BYTES_PER_SECTOR_SHIFT)
    {
        LOG_ERROR("The parameter is invalid. block_size_shift(%d)\n", block_size_shift);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    reserved_blocks++;       /* 超级块也占用了一个块 */
    total_blocks = total_sectors >> (block_size_shift - BYTES_PER_SECTOR_SHIFT);
    if (total_blocks <= reserved_blocks)
    {
        LOG_ERROR("The parameter is invalid. total_blocks(%lld) reserved_blocks(%d)\n",
            total_blocks, reserved_blocks);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* 检查输入参数并申请相关资源 */
    ret = check_and_init_block_resource(&tmp_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("Check and init resource failed. ret(%d)\n", ret);
        return ret;
    }

    sb = &tmp_hnd->sb;

    /* 初始化super block */
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

    /* 创建文件 */
    ret = os_disk_create(&file_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("Create file failed. ret(%d)\n", ret);
        block_close(tmp_hnd);
        return ret;
    }

    tmp_hnd->file_hnd = file_hnd;

    /* 将super block下盘 */
    ret = index_update_block_pingpong_init(tmp_hnd, &sb->head, SUPER_BLOCK_VBN);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. tmp_hnd(%p) buf(%p) vbn(%lld) ret(%d)\n",
            tmp_hnd, &sb->head, SUPER_BLOCK_VBN, ret);
        block_close(tmp_hnd);
        return ret;
    }

    bitmap_start_lba = sb->bitmap_start_block * sb->sectors_per_block
        + start_lba;
    bitmap_total_sectors = sb->bitmap_blocks * sb->sectors_per_block;

    /* 初始化位图缓存系统 */
    ret = bitmap_init(&bitmap_hnd, file_hnd, bitmap_start_lba, bitmap_total_sectors, sb->total_blocks);
    if (ret < 0)
    {
        LOG_ERROR("Init bitmap failed. file_hnd(%p) bitmap_start_lba(%lld) bitmap_total_sectors(%d) total_blocks(%lld) ret(%d)\n",
            file_hnd, bitmap_start_lba, bitmap_total_sectors, sb->total_blocks, ret);
        block_close(tmp_hnd);
        return ret;
    }

    tmp_hnd->bitmap_hnd = bitmap_hnd;

    /* 初始化bitmap区域 */
    ret = block_reset_bitmap(tmp_hnd);
    if (0 > ret)
    {
        LOG_ERROR("Reset bitmap failed. tmp_hnd(%p) ret(%d)\n", tmp_hnd, ret);
        block_close(tmp_hnd);
        return ret;
    }

    *hnd = tmp_hnd;

    return 0;
}

/*******************************************************************************
函数名称: check_super_block
功能说明: 检查超级块是否有效
输入参数:
    sb       : 要检查的超级块数据
    start_lba : 超级块所在地址
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
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

/*******************************************************************************
函数名称: block_open
功能说明: 打开块管理系统
输入参数:
    path      : 块管理系统所在的文件
    start_lba : 块管理区域的起始lba地址
输出参数:
    hnd         : 申请到的块管理系统操作句柄
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_open(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t start_lba)
{
    BLOCK_BOOT_SECTOR_S *sb = NULL;
    void *file_hnd = NULL;
    BITMAP_HANDLE *bitmap_hnd = NULL;
    BLOCK_HANDLE_S *tmp_hnd = NULL;
    int32_t ret = 0;
    uint64_t bitmap_start_lba = 0;
    uint32_t bitmap_total_sectors = 0;

    /* 检查输入参数并申请相关资源 */
    ret = check_and_init_block_resource(&tmp_hnd, path);
    if (0 > ret)
    {
        LOG_ERROR("check_and_init_block_resource failed. ret(%d)\n", ret);
        return ret;
    }

    /* 打开文件 */
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

    /* 读取super block */
    ret = index_read_block_pingpong(tmp_hnd, &sb->head, SUPER_BLOCK_VBN,
        SUPER_BLOCK_ID, SUPER_BLOCK_SIZE);
    if (0 > ret)
    {
        LOG_ERROR("Read lba failed. tmp_hnd(%p) sb(%p) vbn(%lld) ret(%d)\n",
            tmp_hnd, sb, SUPER_BLOCK_VBN, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }

    /* 检查super block合法性 */
    ret = check_super_block(sb, start_lba);
    if (0 > ret)
    {
        LOG_ERROR("Check super block failed. name(%s) start_lba(%lld) ret(%d)\n",
            path, start_lba, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }

    bitmap_start_lba = sb->bitmap_start_block * sb->sectors_per_block
        + sb->start_lba;
    bitmap_total_sectors = sb->bitmap_blocks * sb->sectors_per_block;

    /* 初始化位图缓存系统 */
    ret = bitmap_init(&bitmap_hnd, file_hnd, bitmap_start_lba, bitmap_total_sectors, sb->total_blocks);
    if (ret < 0)
    {
        LOG_ERROR("Init bitmap failed. file_hnd(%p) bitmap_start_lba(%lld)"
            " bitmap_total_sectors(%d) total_blocks(%lld) ret(%d)\n",
            file_hnd, bitmap_start_lba, bitmap_total_sectors, sb->total_blocks, ret);
        (void)block_close(tmp_hnd);
        return ret;
    }

    tmp_hnd->bitmap_hnd = bitmap_hnd;
    
    if ((0 == (sb->flags & FLAG_FIXUP)) && (0 == sb->free_blocks))
    { /* 超级块中的数据是准确的 */
        tmp_hnd->flags |= FLAG_NOSPACE;
    }

    *hnd = tmp_hnd;

    return 0;
}

/*******************************************************************************
函数名称: block_need_fixup
功能说明: 检查是否需要块管理系统修复
输入参数:
    hnd: 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
bool_t block_need_fixup(BLOCK_HANDLE_S * hnd)
{
    ASSERT(NULL != hnd);

    return (((BLOCK_HANDLE_S *) hnd)->sb.flags & FLAG_FIXUP)
        ? B_TRUE : B_FALSE;
}

/*******************************************************************************
函数名称: block_close
功能说明: 关闭块管理系统
输入参数:
    hnd: 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_close(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    if (NULL == hnd)
    {
        LOG_ERROR("The parameter is invalid. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* 释放位图操作句柄占用的资源 */
    if (NULL != hnd->bitmap_hnd)
    {
        (void) bitmap_destroy(hnd->bitmap_hnd);
        hnd->bitmap_hnd = NULL;
    }

    /* 有标记发生了变化 */
    if (0 != (hnd->flags & FLAG_DIRTY))
    {
        hnd->flags &= ~FLAG_DIRTY;      /* 清除dirty标记 */
        hnd->sb.flags &= ~FLAG_FIXUP; /* 清除fixup标记 */
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

/*******************************************************************************
函数名称: block_update_super_block
功能说明: 更新磁盘上的超级块信息
输入参数:
    hnd       : 块管理系统操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_update_super_block(BLOCK_HANDLE_S * hnd)
{
    int32_t ret = 0;

    /* 检查输入参数 */
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


/*******************************************************************************
函数名称: block_find_free
功能说明: 找寻一段连续的空块
输入参数:
    hnd        : 块管理系统操作句柄
    blk_cnt    : 要分配的块数目
输出参数:
    start_vbn: 找到的连续空块的首地址
返 回 值:
    >=0: 实际找到的空块数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_find_free(BLOCK_HANDLE_S * hnd, uint32_t blk_cnt,
    uint64_t * start_vbn)
{
    int32_t ret = 0;

    ASSERT(NULL != hnd);
    ASSERT(NULL != start_vbn);
    ASSERT(0 != blk_cnt);

    for (;;)
    {
        ret = bitmap_get_free_bits(hnd->bitmap_hnd, hnd->sb.first_free_block,
            blk_cnt, start_vbn);
        if (ret > 0)
        {       /* 找到了空块 */
            break;
        }
        else if (-FILE_BITMAP_ERR_0BIT_NOT_FOUND == ret)
        {       /* 没有找到空块 */
            if (0 == hnd->sb.first_free_block)
            {   /* 确认是否真的没有空块了 */
                //hnd->stSB.ullFreeBlocks = 0;
                hnd->flags |= FLAG_NOSPACE;
                LOG_ERROR("No free blocks. hnd(%p) free_blocks(%lld)\n",
                    hnd, hnd->sb.free_blocks);
                return -FILE_BLOCK_ERR_NO_FREE_BLOCKS;
            }
            else
            {   /* 从头再找一遍 */
                hnd->sb.first_free_block = 0;
            }
        }
        else
        {       /* 其他错误 */
            LOG_ERROR("Other false. hnd(%p) ret(%d)\n", hnd, ret);
            break;
        }
    }

    return ret;
}

/*******************************************************************************
函数名称: block_alloc
功能说明: 分配指定数目的块
输入参数:
    hnd          : 块管理系统操作句柄
    blk_cnt    : 要分配的块数目
输出参数:
    start_vbn: 分配到的连续块的首地址
返 回 值:
    >=0: 分配成功的块数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_alloc(BLOCK_HANDLE_S * hnd, uint32_t blk_cnt,
    uint64_t * start_vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;

    /* 检查输入参数 */
    if ((NULL == hnd) || (0 == blk_cnt) || (NULL == start_vbn))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) blk_cnt(%d) start_vbn(%p)\n",
            hnd, blk_cnt, start_vbn);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if (0 != (hnd->flags & FLAG_NOSPACE))
    {   /* 无空块了 */
        LOG_ERROR("No free blocks. hnd(%p) free_blocks(%lld)\n",
            hnd, hnd->sb.free_blocks);
        return -FILE_BLOCK_ERR_NO_FREE_BLOCKS;
    }

    OS_RWLOCK_WRLOCK(&hnd->rwlock);

    /* 找寻空块 */
    ret = block_find_free(hnd, blk_cnt, start_vbn);
    if (ret < 0)
    {
        LOG_ERROR("Find free blocks failed. hnd(%p) ret(%d)\n", hnd, ret);
        OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
        return ret;
    }

    /* 设置块为已用 */
    ret2 = bitmap_set_nbits(hnd->bitmap_hnd, *start_vbn, (uint32_t) ret,
        B_TRUE);
    if (ret2 != ret)
    {
        LOG_ERROR("Set bitmap failed. bitmap_hnd(%p) vbn(%lld) nBits(%d) ret(1) ret2(%d)\n",
            hnd->bitmap_hnd, *start_vbn, ret, ret2);
        OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
        return ret2;
    }

    hnd->sb.free_blocks -= (uint32_t) ret;
    hnd->sb.first_free_block = *start_vbn + (uint32_t) ret;

    /* 设置重启时需检查一致性标记 */
    ret2 = check_and_set_fixup_flag(hnd);
    if (0 > ret2)
    {
        LOG_ERROR("Set fixup flag failed. hnd(%p) ret2(%d)\n", hnd, ret2);

        /* 收回刚刚占用的空间 */
        (void) bitmap_set_nbits(hnd->bitmap_hnd, *start_vbn,
            (uint32_t) ret, B_FALSE);
        hnd->sb.free_blocks += (uint32_t) ret;
        OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
        return ret2;
    }

    OS_RWLOCK_WRUNLOCK(&hnd->rwlock);

    return ret;
}

/*******************************************************************************
函数名称: block_set_status
功能说明: 设置指定数目的块为空闲或者占用
输入参数:
    hnd         : 块管理系统操作句柄
    start_vbn: 要设置的连续块的首地址
    blk_cnt   : 要设置的块数目
    is_used      : 占用或空闲
输出参数: 无
返 回 值:
    >=0: 设置成功的块数目
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t block_set_status(BLOCK_HANDLE_S * hnd, uint64_t start_vbn,
    uint32_t blk_cnt, bool_t is_used)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    int32_t cnt = 0;
    bool_t is_one = B_FALSE;

    /* 检查输入参数 */
    if ((NULL == hnd) || (0 == blk_cnt))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) blk_cnt(%d)\n", hnd, blk_cnt);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&hnd->rwlock);

    while (blk_cnt--)
    {
        ret = bitmap_check_bit(hnd->bitmap_hnd, start_vbn);
        if (ret < 0)
        {
            LOG_ERROR("Check bitmap failed. bitmap_hnd(%p) vbn(%lld) nBits(%d) is_used(%d) ret(%d)\n",
                hnd->bitmap_hnd, start_vbn, 1, is_used, ret);
            OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
            return ret;
        }

        if (0 != ret)
        {
            is_one = B_TRUE;
        }

        if (is_one == is_used)
        {
            start_vbn++;
            cnt++;
            continue;
        }
        
        ret = bitmap_set_nbits(hnd->bitmap_hnd, start_vbn, 1, is_used);
        if (ret < 0)
        {
            LOG_ERROR("Set bitmap failed. bitmap_hnd(%p) vbn(%lld) nBits(%d) is_used(%d) ret(%d)\n",
                hnd->bitmap_hnd, start_vbn, 1, is_used, ret);
            OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
            return ret;
        }

        if (B_FALSE == is_used)
        {
            hnd->flags &= ~FLAG_NOSPACE;
            hnd->sb.free_blocks += (uint32_t) ret;

            if (hnd->sb.first_free_block > start_vbn)
            { /* 下面语句是否打开决定是否重复利用前面释放的块 */
                // tmp_hnd->stSB.ullFirstFreeBlock = start_vbn;
            }
        }
        else
        {
            hnd->sb.free_blocks -= (uint32_t) ret;
        }

        start_vbn++;
        cnt++;
    }

    /* 设置重启时需检查一致性标记 */
    ret2 = check_and_set_fixup_flag(hnd);
    if (0 > ret2)
    {
        LOG_ERROR("Set fixup flag failed. hnd(%p) ret2(%d)\n", hnd, ret2);
        OS_RWLOCK_WRUNLOCK(&hnd->rwlock);
        return ret2;
    }

    OS_RWLOCK_WRUNLOCK(&hnd->rwlock);

    return cnt;
}

//SIZE_OF_TYPE_EQUAL_TO(BLOCK_BOOT_SECTOR_S, 512)



EXPORT_SYMBOL(block_set_status);
EXPORT_SYMBOL(block_alloc);

