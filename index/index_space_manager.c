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
 File Name: INDEX_SPACE_MANAGER.C
 Author   : axen.hook
 Version  : 1.00
 Date     : 20/Mar/2016
 Description: 
 Function List: 
     1. ...: 
 History: 
     Version: 1.00  Author: axen.hook  Date: 20/Mar/2016
 --------------------------------------------------------------------------------
     1. Primary version
 *******************************************************************************/
#include "index_if.h"
 
 MODULE(PID_INDEX);
#include "os_log.h"

int32_t alloc_space(OBJECT_HANDLE *obj, uint64_t start_blk, uint32_t blk_cnt, uint64_t *real_start_blk)
{
    uint64_t addr;
    uint32_t len;
    uint64_t end;
    uint64_t end_blk;
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;
    int32_t ret;

    ASSERT(blk_cnt != 0);

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);
    
    ret = index_search_key_nolock(obj, addr_str, addr_size, len_str, len_size);
    if (ret < 0)
    {
        if (ret != -INDEX_ERR_KEY_NOT_FOUND)
        {
            LOG_ERROR("Search key failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
            return ret;
        }

        if (obj->ie->flags & INDEX_ENTRY_END) // no key here
        {
            ret = walk_tree(obj, INDEX_GET_FIRST); // get first key
            if (0 != ret)
            { // no any key
                if (ret != -INDEX_ERR_ROOT)
                {
                    LOG_ERROR("walk tree failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
                    return ret;
                }
                
                return -INDEX_ERR_NO_FREE_BLOCKS; // no free space
            }
        }

        // no overlap
        addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
        start_blk = addr; //  reset the start block
    }
    else
    {
        addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
    }
    
    len = (uint32_t)os_bstr_to_u64(GET_IE_VALUE(obj->ie), obj->ie->value_len);
    end = addr + len;
    
    ASSERT(len != 0);

    ret = tree_remove_ie(obj);
    if (ret < 0)
    {
        LOG_ERROR("remove entry failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
        return ret;
    }
        
    if (start_blk <= addr)
    {
        start_blk = addr;  // allocate from addr
        end_blk = start_blk + blk_cnt;
        
        *real_start_blk = addr;
        
        if (end_blk < end)
        {
            addr_size = os_u64_to_bstr(end_blk, addr_str);
            len_size = os_u64_to_bstr(end - end_blk, len_str);
            ret = index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
            if (ret < 0)
            {
                LOG_ERROR("insert key failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
                return ret;
            }
            
            return (uint32_t)(end_blk - addr);
        }

        return len;
    }

    // below: start_blk > addr
    
    end_blk = start_blk + blk_cnt;
    ASSERT(start_blk < end);
    *real_start_blk = start_blk;
    
    addr_size = os_u64_to_bstr(addr, addr_str);
    len_size = os_u64_to_bstr(start_blk - addr, len_str);
    ret = index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
    if (ret < 0)
    {
        LOG_ERROR("insert key failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
        return ret;
    }
    
    if (end_blk < end)
    {
        addr_size = os_u64_to_bstr(end_blk, addr_str);
        len_size = os_u64_to_bstr(end - end_blk, len_str);
        ret = index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
        if (ret < 0)
        {
            LOG_ERROR("insert key failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
            return ret;
        }
        
        return blk_cnt;
    }

    return (uint32_t)(end - start_blk);
}

int32_t free_space(OBJECT_HANDLE *obj, uint64_t start_blk, uint32_t blk_cnt)
{
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    return index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
}

void index_init_sm(space_manager_t *sm, OBJECT_HANDLE *obj, uint64_t first_free_block,
    uint64_t total_free_blocks, uint64_t total_blocks)
{
    memset(sm->blk, 0, sizeof(sm->blk));
    sm->blk_no = 0;
    sm->blk_num= 0;
    sm->free_blk_obj = obj;
    sm->first_free_block = first_free_block;
    sm->total_free_blocks = total_free_blocks;
    sm->total_blocks = total_blocks;
    OS_RWLOCK_INIT(&sm->lock);
    OS_RWLOCK_INIT(&sm->blk_lock);
}

void index_destroy_sm(space_manager_t *sm)
{
    OS_RWLOCK_DESTROY(&sm->blk_lock);
    OS_RWLOCK_DESTROY(&sm->lock);
}


int32_t index_init_free_space(space_manager_t *sm, uint64_t start_blk, uint64_t blk_cnt)
{
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    return index_insert_key_nolock(sm->free_blk_obj, addr_str, addr_size, len_str, len_size);
}

int64_t index_alloc_blk(space_manager_t *sm, uint64_t *real_start_blk)
{
    uint64_t blk;
    uint32_t cnt = 0;
    
    OS_RWLOCK_WRLOCK(&sm->blk_lock);
    if (sm->blk_num == 0)
    {
        OS_RWLOCK_WRUNLOCK(&sm->blk_lock);
        return 0;
    }

    blk = sm->blk[sm->blk_no];
    while (blk == 0)
    {
        blk = sm->blk[sm->blk_no++];
        if (sm->blk_no >= MAX_BLK_NUM)
        {
            sm->blk_no = 0;
        }

        cnt++;
        if (cnt >= MAX_BLK_NUM)
        {
            break;
        }
    }
    
    OS_RWLOCK_WRUNLOCK(&sm->blk_lock);

    return blk;
}

int32_t index_free_blk(space_manager_t *sm, uint64_t start_blk)
{
    OS_RWLOCK_WRLOCK(&sm->blk_lock);
    
    OS_RWLOCK_WRUNLOCK(&sm->blk_lock);

	return 0;
}



// return value:
// >  0: real blk cnt
// == 0: no free blk
// <  0: error code
int32_t index_alloc_space(space_manager_t *sm, uint32_t blk_cnt, uint64_t *real_start_blk)
{
    int32_t ret;

    if (0 == sm->total_free_blocks)
    {
        return -INDEX_ERR_NO_FREE_BLOCKS;
    }

    OS_RWLOCK_WRLOCK(&sm->lock);
    ret = alloc_space(sm->free_blk_obj, sm->first_free_block, blk_cnt, real_start_blk);
    if (ret < 0) // error
    {
        OS_RWLOCK_WRUNLOCK(&sm->lock);
        LOG_ERROR("alloc space failed. objid(0x%llx) ret(%d)\n", sm->free_blk_obj->obj_info->objid, ret);
        return ret;
    }

    if (ret == 0) // no space
    {
        sm->first_free_block = 0;
        sm->total_free_blocks = 0;
        OS_RWLOCK_WRUNLOCK(&sm->lock);
        return -INDEX_ERR_NO_FREE_BLOCKS;
    }

    sm->first_free_block = *real_start_blk + ret;
    if (sm->total_free_blocks < (uint64_t)ret)
    {
        LOG_ERROR("the sm system chaos. objid(0x%llx) ret(%d)\n", sm->free_blk_obj->obj_info->objid, ret);
    }
    else
    {
        sm->total_free_blocks -= ret;
    }
    
    OS_RWLOCK_WRUNLOCK(&sm->lock);

    return ret;
}

int32_t index_free_space(space_manager_t *sm, uint64_t start_blk, uint32_t blk_cnt)
{
    int32_t ret;
    
    OS_RWLOCK_WRLOCK(&sm->lock);
    ret = free_space(sm->free_blk_obj, start_blk, blk_cnt);
    if (ret > 0)
    {
        sm->total_free_blocks += blk_cnt;
    }
    OS_RWLOCK_WRUNLOCK(&sm->lock);

    return ret;
}

