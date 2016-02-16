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

int32_t print_index_info(NET_PARA_S *net, INDEX_HANDLE *index)
{
    ASSERT(NULL != index);

    net->print(net->net, "index: %p, name: %s, ref: %u\n",
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
int32_t print_obj_info(NET_PARA_S *net, OBJECT_HANDLE *obj)
{
    ASSERT(NULL != obj);

    net->print(net->net, "obj: %p, name: %s, ref_cnt: %u\n",
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
int32_t print_cache_info(NET_PARA_S *net, INDEX_BLOCK_CACHE *cache)
{
    ASSERT(NULL != cache);

    net->print(net->net, "cache: %p, vbn: %lld, state: %d, ib: %p\n",
        cache, cache->vbn, cache->state, cache->ib);

    return 0;
}

int32_t print_attr_info(NET_PARA_S *net, ATTR_INFO *attr_info)
{
    ASSERT(NULL != attr_info);

    net->print(net->net, "attr_info: %p, state: %d, ref_cnt: %d, attr_name: %s\n",
        attr_info, attr_info->root_ibc.state, attr_info->attr_ref_cnt, attr_info->attr_name);

    return 0;
}

void print_block_Info(BLOCK_HANDLE_S * hnd, NET_PARA_S *net)
{
    BLOCK_HANDLE_S *tmp_hnd = hnd;

    if (NULL == hnd)
    {
        return;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

    net->print(net->net, "ObjID            : 0x%X\n", tmp_hnd->sb.head.blk_id);
    net->print(net->net, "AllocSize        : %d\n", tmp_hnd->sb.head.alloc_size);
    net->print(net->net, "RealSize         : %d\n", tmp_hnd->sb.head.real_size);
    net->print(net->net, "SeqNo            : 0x%04X\n", tmp_hnd->sb.head.seq_no);
    net->print(net->net, "Fixup            : 0x%04X\n", tmp_hnd->sb.head.fixup);

    
    net->print(net->net, "BlockSizeShift   : %d\n", tmp_hnd->sb.block_size_shift);
    net->print(net->net, "BlockSize        : %d\n", tmp_hnd->sb.block_size);
    net->print(net->net, "SectorsPerBlock  : %d\n", tmp_hnd->sb.sectors_per_block);
    
    net->print(net->net, "BitmapBlocks     : %d\n", tmp_hnd->sb.bitmap_blocks);
    net->print(net->net, "BitmapStartBlock : %lld\n", tmp_hnd->sb.bitmap_start_block);
    
    net->print(net->net, "TotalBlocks      : %lld\n", tmp_hnd->sb.total_blocks);
    net->print(net->net, "FreeBlocks       : %lld\n", tmp_hnd->sb.free_blocks);
    net->print(net->net, "FirstFreeBlock   : %lld\n", tmp_hnd->sb.first_free_block);
    
    net->print(net->net, "start_lba         : %lld\n", tmp_hnd->sb.start_lba);
    net->print(net->net, "Version          : %d\n", tmp_hnd->sb.version);
    net->print(net->net, "Flags            : 0x%08X\n", tmp_hnd->sb.flags);
    net->print(net->net, "MagicNum         : 0x%04X\n", tmp_hnd->sb.magic_num);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return;
}


int32_t list_super_block(char *index_name, uint64_t start_lba, NET_PARA_S *net)
{
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;
    
    /* 检查输入参数 */
    ASSERT (NULL != index_name);
    ASSERT (0 != strlen(index_name));

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        net->print(net->net, "Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }
    
    print_block_Info(index->hnd, net);
    
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
int32_t cmd_list(char *index_name, char *obj_name, uint64_t start_lba, NET_PARA_S *net)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(NULL != index_name);
    
    if (0 == strlen(index_name))
    {
        ret = walk_all_opened_index((int32_t (*)(void *, INDEX_HANDLE *))print_index_info, net);
        if (0 > ret)
        {
            net->print(net->net, "Walk all opened index failed. ret(%d)\n", ret);
        }

        return ret;
    }

    index = index_find_handle(index_name);
    if (NULL == index)
    {
        net->print(net->net, "The index is not opened. index(%s)\n",
            index_name);
        return -2;
    }
    
    if (0 == strlen(obj_name))
    {
        ret = walk_all_opened_child_objects(index->root_obj, (int32_t (*)(void *, OBJECT_HANDLE *))print_obj_info, net);
        if (0 > ret)
        {
            net->print(net->net, "Walk all opened trees failed. index(%p) ret(%d)\n",
                index, ret);
        }

		return ret;
    }

    ret = index_open_object(index->root_obj, obj_name, &obj);
    if (0 > ret)
    {
        net->print(net->net, "Open tree failed. index(%p) name(%s) ret(%d)\n",
            index, obj_name, ret);
        return ret;
    }

    net->print(net->net, "Obj info:\n");
    net->print(net->net, "-----------------------------------------\n");
    net->print(net->net, "inode_no               : %lld\n", obj->inode.inode_no);
    net->print(net->net, "parent_inode_no inode  : %lld\n", obj->inode.parent_inode_no);
    net->print(net->net, "mode                   : 0x%llX\n", obj->inode.mode);
    net->print(net->net, "obj_name               : %s\n", obj->obj_name);
    net->print(net->net, "state                  : 0x%x\n", obj->obj_state);
    net->print(net->net, "ref_cnt                : %d\n", obj->obj_ref_cnt);
    
    net->print(net->net, "\nAttr info:\n");
    net->print(net->net, "-----------------------------------------\n");
    avl_walk_all(&obj->attr_info_list, (int (*) (void*, void *))print_attr_info, net);
    
    net->print(net->net, "\nCache info:\n");
    net->print(net->net, "-----------------------------------------\n");
    avl_walk_all(&obj->obj_caches, (int (*) (void*, void *))print_cache_info, net);

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
int do_list_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        net->print(net->net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    
    if (0 != (para->flags & TOOLS_FLAGS_SB))
    {
        (void)list_super_block(para->index_name, para->start_lba, net);
        OS_FREE(para);
        return -2;
    }

    cmd_list(para->index_name, para->obj_name, para->start_lba, net);

    OS_FREE(para);
    para = NULL;

    return -3;
}


