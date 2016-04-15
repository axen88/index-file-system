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
File Name: INDEX_TOOLS_LIST.C
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

int32_t print_one_cache_info(net_para_t *net, ifs_block_cache_t *cache)
{
    ASSERT(NULL != cache);

    OS_PRINT(net, "cache: %p, vbn: %lld, state: 0x%x, ib: %p\n",
        cache, cache->vbn, cache->state, cache->ib);

    return 0;
}

int32_t print_one_fs_info(net_para_t *net, index_handle_t *index)
{
    ASSERT(NULL != index);

    OS_PRINT(net, "name: %s, flags: 0x%x, ref: %u\n", index->name, index->flags, index->index_ref_cnt);

    return 0;
}

int32_t print_one_obj_info(net_para_t *net, object_info_t *obj_info)
{
    ASSERT(NULL != obj_info);

    OS_PRINT(net, "objid: %lld, inode_no: %lld, obj_state: 0x%x, ref_cnt: %u, name: %s\n",
        obj_info->objid, obj_info->inode_no, obj_info->obj_state, obj_info->obj_ref_cnt, obj_info->obj_name);

    return 0;
}

int32_t print_super_block(char *index_name, uint64_t start_lba, net_para_t *net)
{
    index_handle_t *index = NULL;
    int32_t ret = 0;
    
    ASSERT (NULL != index_name);
    ASSERT (0 != strlen(index_name));

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        OS_PRINT(net, "Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }
    
    OS_RWLOCK_WRLOCK(&index->index_lock);

    OS_PRINT(net, "blk_id                : 0x%X\n",   index->sb.head.blk_id);
    OS_PRINT(net, "alloc_size            : %d\n",     index->sb.head.alloc_size);
    OS_PRINT(net, "real_size             : %d\n",     index->sb.head.real_size);
    OS_PRINT(net, "seq_no                : 0x%04X\n", index->sb.head.seq_no);
    OS_PRINT(net, "fixup                 : 0x%04X\n\n", index->sb.head.fixup);

    
    OS_PRINT(net, "block_size_shift      : %d\n", index->sb.block_size_shift);
    OS_PRINT(net, "block_size            : %d\n", index->sb.block_size);
    OS_PRINT(net, "sectors_per_block     : %d\n\n", index->sb.sectors_per_block);
    
    OS_PRINT(net, "start_lba             : %lld\n",  index->sb.start_lba);
    OS_PRINT(net, "total_blocks          : %lld\n\n", index->sb.total_blocks);
    
    OS_PRINT(net, "objid_id              : %lld\n",  index->sb.objid_id);
    OS_PRINT(net, "objid_inode_no        : %lld\n\n",  index->sb.objid_inode_no);
    
    OS_PRINT(net, "space_id              : %lld\n",  index->sb.space_id);
    OS_PRINT(net, "space_inode_no        : %lld\n",  index->sb.space_inode_no);
    OS_PRINT(net, "free_blocks           : %lld\n",  index->sb.free_blocks);
    OS_PRINT(net, "first_free_block      : %lld\n\n",  index->sb.first_free_block);
    
    OS_PRINT(net, "base_id               : %lld\n",  index->sb.base_id);
    OS_PRINT(net, "base_inode_no         : %lld\n",  index->sb.base_inode_no);
    OS_PRINT(net, "base_free_blocks      : %lld\n",  index->sb.base_free_blocks);
    OS_PRINT(net, "base_first_free_block : %lld\n\n",  index->sb.base_first_free_block);

    OS_PRINT(net, "base_blk              : %lld\n\n",  index->sb.base_blk);

    OS_PRINT(net, "snapshot_no           : %lld\n\n",  index->sb.snapshot_no);
    
    OS_PRINT(net, "flags                 : 0x%08X\n", index->sb.flags);
    OS_PRINT(net, "version               : %d\n",     index->sb.version);
    OS_PRINT(net, "magic_num             : 0x%04X\n", index->sb.magic_num);
    
    OS_RWLOCK_WRUNLOCK(&index->index_lock);
    
    (void)index_close(index);
    
    return 0;
}

void print_fs_info(net_para_t *net, index_handle_t *index)
{
    OS_PRINT(net, "FS info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    OS_PRINT(net, "name        : %s\n", index->name);
    OS_PRINT(net, "flags       : 0x%x\n", index->flags);
    OS_PRINT(net, "ref_cnt     : %u\n", index->index_ref_cnt);
    
    OS_PRINT(net, "\nObject list:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&index->obj_info_list, (avl_walk_cb_t)print_one_obj_info, net);
    
    OS_PRINT(net, "\nCache list:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&index->metadata_cache, (avl_walk_cb_t)print_one_cache_info, net);
}

void print_obj_info(net_para_t *net, object_info_t *obj_info)
{
    OS_PRINT(net, "Object info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    OS_PRINT(net, "objid                  : %lld\n", obj_info->objid);
    OS_PRINT(net, "inode_no               : %lld\n", obj_info->inode_no);
    OS_PRINT(net, "obj_name               : %s\n",   obj_info->obj_name);
    OS_PRINT(net, "state                  : 0x%x\n", obj_info->obj_state);
    OS_PRINT(net, "ref_cnt                : %d\n",   obj_info->obj_ref_cnt);
    OS_PRINT(net, "vbn                    : %lld\n", obj_info->root_ibc.vbn);
    OS_PRINT(net, "state                  : 0x%x\n", obj_info->root_ibc.state);
    
    OS_PRINT(net, "\nCache info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&obj_info->caches, (avl_walk_cb_t)print_one_cache_info, net);
}

int32_t cmd_list(char *index_name, uint64_t objid, uint64_t start_lba, net_para_t *net)
{
    int32_t ret = 0;
    index_handle_t *index = NULL;
    object_info_t *obj_info = NULL;
    
    ASSERT(NULL != index_name);
    
    if (0 == strlen(index_name))
    {
        ret = walk_all_opened_index((int32_t (*)(void *, index_handle_t *))print_one_fs_info, net);
        if (0 > ret)
        {
            OS_PRINT(net, "Walk all opened index failed. ret(%d)\n", ret);
        }

        return ret;
    }

    index = index_get_handle(index_name);
    if (NULL == index)
    {
        OS_PRINT(net, "The index is not opened. index(%s)\n", index_name);
        return -2;
    }
    
    if (OBJID_IS_INVALID(objid))
    {
        print_fs_info(net, index);
		return 0;
    }

    obj_info = index_get_object_info(index, objid);
    if (NULL == obj_info)
    {
        OS_PRINT(net, "The object is not opened. index(%p) objid(%lld)\n", index, objid);
        return ret;
    }

    print_obj_info(net, obj_info);

    return ret;
}

int do_list_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    
    if (0 != (para->flags & TOOLS_FLAGS_SB))
    {
        (void)print_super_block(para->index_name, para->start_lba, net);
        OS_FREE(para);
        return -2;
    }

    cmd_list(para->index_name, para->objid, para->start_lba, net);

    OS_FREE(para);
    para = NULL;

    return -3;
}


