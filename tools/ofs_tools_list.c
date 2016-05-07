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
#include "ofs_if.h"

int32_t print_one_cache_info(net_para_t *net, ofs_block_cache_t *cache)
{
    ASSERT(cache != NULL);

    OS_PRINT(net, "cache: %p, vbn: %lld, state: 0x%x, ib: %p\n",
        cache, cache->vbn, cache->state, cache->ib);

    return 0;
}

int32_t print_one_fs_info(net_para_t *net, container_handle_t *ct)
{
    ASSERT(ct != NULL);

    OS_PRINT(net, "name: %s, flags: 0x%x, ref: %u\n", ct->name, ct->flags, ct->ref_cnt);

    return 0;
}

int32_t print_one_obj_info(net_para_t *net, object_info_t *obj_info)
{
    ASSERT(obj_info != NULL);

    OS_PRINT(net, "objid: %lld, inode_no: %lld, obj_state: 0x%x, ref_cnt: %u, name: %s\n",
        obj_info->objid, obj_info->inode_no, obj_info->obj_state, obj_info->ref_cnt, obj_info->name);

    return 0;
}

int32_t print_super_block(char *ct_name, net_para_t *net)
{
    container_handle_t *ct = NULL;
    int32_t ret = 0;
    
    ASSERT(ct_name != NULL);
    ASSERT(strlen(ct_name));

    ret = ofs_open_container(ct_name, &ct);
    if (ret < 0)
    {
        OS_PRINT(net, "Open ct failed. ct_name(%s) ret(%d)\n",
            ct_name, ret);
        return ret;
    }
    
    OS_RWLOCK_WRLOCK(&ct->ct_lock);

    OS_PRINT(net, "blk_id                : 0x%X\n",   ct->sb.head.blk_id);
    OS_PRINT(net, "alloc_size            : %d\n",     ct->sb.head.alloc_size);
    OS_PRINT(net, "real_size             : %d\n",     ct->sb.head.real_size);
    OS_PRINT(net, "seq_no                : 0x%04X\n", ct->sb.head.seq_no);
    OS_PRINT(net, "fixup                 : 0x%04X\n\n", ct->sb.head.fixup);

    
    OS_PRINT(net, "block_size_shift      : %d\n", ct->sb.block_size_shift);
    OS_PRINT(net, "block_size            : %d\n", ct->sb.block_size);
    OS_PRINT(net, "sectors_per_block     : %d\n\n", ct->sb.sectors_per_block);
    
    OS_PRINT(net, "total_blocks          : %lld\n\n", ct->sb.total_blocks);
    
    OS_PRINT(net, "objid_id              : %lld\n",  ct->sb.objid_id);
    OS_PRINT(net, "objid_inode_no        : %lld\n\n",  ct->sb.objid_inode_no);
    
    OS_PRINT(net, "space_id              : %lld\n",  ct->sb.space_id);
    OS_PRINT(net, "space_inode_no        : %lld\n",  ct->sb.space_inode_no);
    OS_PRINT(net, "free_blocks           : %lld\n",  ct->sb.free_blocks);
    OS_PRINT(net, "first_free_block      : %lld\n\n",  ct->sb.first_free_block);
    
    OS_PRINT(net, "base_id               : %lld\n",  ct->sb.base_id);
    OS_PRINT(net, "base_inode_no         : %lld\n",  ct->sb.base_inode_no);
    OS_PRINT(net, "base_free_blocks      : %lld\n",  ct->sb.base_free_blocks);
    OS_PRINT(net, "base_first_free_block : %lld\n\n",  ct->sb.base_first_free_block);

    OS_PRINT(net, "base_blk              : %lld\n\n",  ct->sb.base_blk);

    OS_PRINT(net, "snapshot_no           : %lld\n\n",  ct->sb.snapshot_no);
    
    OS_PRINT(net, "flags                 : 0x%08X\n", ct->sb.flags);
    OS_PRINT(net, "version               : %d\n",     ct->sb.version);
    OS_PRINT(net, "magic_num             : 0x%04X\n", ct->sb.magic_num);
    
    OS_RWLOCK_WRUNLOCK(&ct->ct_lock);
    
    (void)ofs_close_container(ct);
    
    return 0;
}

void print_fs_info(net_para_t *net, container_handle_t *ct)
{
    OS_PRINT(net, "FS info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    OS_PRINT(net, "name        : %s\n", ct->name);
    OS_PRINT(net, "flags       : 0x%x\n", ct->flags);
    OS_PRINT(net, "ref_cnt     : %u\n", ct->ref_cnt);
    
    OS_PRINT(net, "\nObject list:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&ct->obj_info_list, (avl_walk_cb_t)print_one_obj_info, net);
    
    OS_PRINT(net, "\nCache list:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&ct->metadata_cache, (avl_walk_cb_t)print_one_cache_info, net);
}

void print_obj_info(net_para_t *net, object_info_t *obj_info)
{
    OS_PRINT(net, "Object info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    OS_PRINT(net, "objid                  : %lld\n", obj_info->objid);
    OS_PRINT(net, "inode_no               : %lld\n", obj_info->inode_no);
    OS_PRINT(net, "name               : %s\n",   obj_info->name);
    OS_PRINT(net, "state                  : 0x%x\n", obj_info->obj_state);
    OS_PRINT(net, "ref_cnt                : %d\n",   obj_info->ref_cnt);
    OS_PRINT(net, "vbn                    : %lld\n", obj_info->root_cache.vbn);
    OS_PRINT(net, "state                  : 0x%x\n", obj_info->root_cache.state);
    
    OS_PRINT(net, "\nCache info:\n");
    OS_PRINT(net, "-----------------------------------------\n");
    avl_walk_all(&obj_info->caches, (avl_walk_cb_t)print_one_cache_info, net);
}

int32_t cmd_list(char *ct_name, uint64_t objid, net_para_t *net)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_info_t *obj_info = NULL;
    
    ASSERT(ct_name != NULL);
    
    if (strlen(ct_name) == 0)
    {
        ret = ofs_walk_all_opened_container((int32_t (*)(void *, container_handle_t *))print_one_fs_info, net);
        if (ret < 0)
        {
            OS_PRINT(net, "Walk all opened ct failed. ret(%d)\n", ret);
        }

        return ret;
    }

    ct = ofs_get_container_handle(ct_name);
    if (ct == NULL)
    {
        OS_PRINT(net, "The ct is not opened. ct(%s)\n", ct_name);
        return -2;
    }
    
    if (OBJID_IS_INVALID(objid))
    {
        print_fs_info(net, ct);
		return 0;
    }

    obj_info = ofs_get_object_info(ct, objid);
    if (obj_info == NULL)
    {
        OS_PRINT(net, "The object is not opened. ct(%p) objid(%lld)\n", ct, objid);
        return ret;
    }

    print_obj_info(net, obj_info);

    return ret;
}

int do_list_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    
    if (para->flags & TOOLS_FLAGS_SB)
    {
        (void)print_super_block(para->ct_name, net);
        OS_FREE(para);
        return -2;
    }

    cmd_list(para->ct_name, para->objid, net);

    OS_FREE(para);
    para = NULL;

    return -3;
}


