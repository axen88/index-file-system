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
文 件 名: OS_INDEX_LIST.C
版    本: 1.00
日    期: 2011年6月8日
功能描述: 列出所有已经打开索引区或树的信息的工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月8日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

int32_t print_index_info(void *para, INDEX_HANDLE *index)
{
    ASSERT(NULL != index);

    OS_PRINT("index: %p, name: %s, ref: %u\n",
        index, index->name, index->index_ref_cnt);

    return 0;
}

/*******************************************************************************
函数名称: print_obj_info
功能说明: 打印当前已经打开的树的信息
输入参数:
    entry: 当前树所在的链表中的位置
    para   : 此操作相关的参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t print_obj_info(void *para, OBJECT_HANDLE *obj)
{
    ASSERT(NULL != obj);

    OS_PRINT("obj: %p, name: %s, ref_cnt: %u\n",
        obj, obj->obj_name, obj->obj_ref_cnt);

    return 0;
}

/*******************************************************************************
函数名称: print_cache_info
功能说明: 打印当前已经打开的树的信息
输入参数:
    entry: 当前树所在的链表中的位置
    para   : 此操作相关的参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t print_cache_info(void *para, INDEX_BLOCK_CACHE *cache)
{
    ASSERT(NULL != cache);

    OS_PRINT("cache: %p, vbn: %lld, state: %d, ib: %p\n",
        cache, cache->vbn, cache->state, cache->ib);

    return 0;
}

int32_t print_attr_info(void *para, ATTR_INFO *attr_info)
{
    ASSERT(NULL != attr_info);

    OS_PRINT("attr_info: %p, state: %d, ref_cnt: %d, attr_name: %s\n",
        attr_info, attr_info->root_ibc.state, attr_info->attr_ref_cnt, attr_info->attr_name);

    return 0;
}

void print_block_Info(BLOCK_HANDLE_S * hnd)
{
    BLOCK_HANDLE_S *tmp_hnd = hnd;

    if (NULL == hnd)
    {
        return;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

    OS_PRINT("ObjID            : 0x%X\n", tmp_hnd->sb.head.blk_id);
    OS_PRINT("AllocSize        : %d\n", tmp_hnd->sb.head.alloc_size);
    OS_PRINT("RealSize         : %d\n", tmp_hnd->sb.head.real_size);
    OS_PRINT("SeqNo            : 0x%04X\n", tmp_hnd->sb.head.seq_no);
    OS_PRINT("Fixup            : 0x%04X\n", tmp_hnd->sb.head.fixup);

    
    OS_PRINT("BlockSizeShift   : %d\n", tmp_hnd->sb.block_size_shift);
    OS_PRINT("BlockSize        : %d\n", tmp_hnd->sb.block_size);
    OS_PRINT("SectorsPerBlock  : %d\n", tmp_hnd->sb.sectors_per_block);
    
    OS_PRINT("BitmapBlocks     : %d\n", tmp_hnd->sb.bitmap_blocks);
    OS_PRINT("BitmapStartBlock : %lld\n", tmp_hnd->sb.bitmap_start_block);
    
    OS_PRINT("TotalBlocks      : %lld\n", tmp_hnd->sb.total_blocks);
    OS_PRINT("FreeBlocks       : %lld\n", tmp_hnd->sb.free_blocks);
    OS_PRINT("FirstFreeBlock   : %lld\n", tmp_hnd->sb.first_free_block);
    
    OS_PRINT("start_lba         : %lld\n", tmp_hnd->sb.start_lba);
    OS_PRINT("Version          : %d\n", tmp_hnd->sb.version);
    OS_PRINT("Flags            : 0x%08X\n", tmp_hnd->sb.flags);
    OS_PRINT("MagicNum         : 0x%04X\n", tmp_hnd->sb.magic_num);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return;
}


int32_t list_super_block(char *index_name, uint64_t start_lba)
{
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;
    
    /* 检查输入参数 */
    ASSERT (NULL != index_name);
    ASSERT (0 != strlen(index_name));

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        OS_PRINT("Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }
    
    print_block_Info(index->hnd);
    
    (void)index_close(index);
    
    return 0;
}

/*******************************************************************************
函数名称: cmd_list
功能说明: 列出所有已经打开的索引区或树的信息
输入参数:
    index_name: 索引区名称
    start_lba: 索引区的起始lba
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t cmd_list(char *index_name, char *obj_name, uint64_t start_lba)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(NULL != index_name);
    
    if (0 == strlen(index_name))
    {
        ret = walk_all_opened_index(print_index_info, NULL);
        if (0 > ret)
        {
            OS_PRINT("Walk all opened index failed. ret(%d)\n", ret);
        }

        return ret;
    }

    index = index_find_handle(index_name);
    if (NULL == index)
    {
        OS_PRINT("The index is not opened. index(%s)\n",
            index_name);
        return -2;
    }
    
    if (0 == strlen(obj_name))
    {
        ret = walk_all_opened_child_objects(index->root_obj, print_obj_info, NULL);
        if (0 > ret)
        {
            OS_PRINT("Walk all opened trees failed. index(%p) ret(%d)\n",
                index, ret);
        }

		return ret;
    }

    ret = index_open_object(index->root_obj, obj_name, &obj);
    if (0 > ret)
    {
        OS_PRINT("Open tree failed. index(%p) name(%s) ret(%d)\n",
            index, obj_name, ret);
        return ret;
    }

    OS_PRINT("Obj info:\n");
    OS_PRINT("-----------------------------------------\n");
    OS_PRINT("inode_no               : %lld\n", obj->inode.inode_no);
    OS_PRINT("parent_inode_no inode  : %lld\n", obj->inode.parent_inode_no);
    OS_PRINT("mode                   : 0x%llX\n", obj->inode.mode);
    OS_PRINT("obj_name               : %s\n", obj->obj_name);
    OS_PRINT("state                  : 0x%x\n", obj->obj_state);
    OS_PRINT("ref_cnt                : %d\n", obj->obj_ref_cnt);
    
    OS_PRINT("\nAttr info:\n");
    OS_PRINT("-----------------------------------------\n");
    avl_walk_all(&obj->attr_info_list, print_attr_info, NULL);
    
    OS_PRINT("\nCache info:\n");
    OS_PRINT("-----------------------------------------\n");
    avl_walk_all(&obj->obj_caches, print_cache_info, NULL);

    (void)index_close_object(obj);
    
    return ret;
}

/*******************************************************************************
函数名称: IndexDoListCmd
功能说明: 执行用户输入的list相关命令行
输入参数:
    v_pcCmd: 要执行的命令
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
int do_list_cmd(int argc, char *argv[])
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT("Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    
    if (0 != (para->flags & TOOLS_FLAGS_SB))
    {
        (void)list_super_block(para->index_name, para->start_lba);
        OS_FREE(para);
        return -2;
    }

    cmd_list(para->index_name, para->obj_name, para->start_lba);

    OS_FREE(para);
    para = NULL;

    return -3;
}


