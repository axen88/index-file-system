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
#include "ofs_if.h"
 
MODULE(MID_SPACE_MANAGER);
#include "log.h"

int32_t alloc_space(object_handle_t *obj, u64_t start_blk, uint32_t blk_cnt, u64_t *real_start_blk)
{
    u64_t addr;
    uint32_t len;
    u64_t end;
    u64_t end_blk;
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
            if (ret != 0)
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

int32_t free_space(object_handle_t *obj, u64_t start_blk, uint32_t blk_cnt)
{
    u64_t addr;
    uint32_t len;
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;
    int32_t ret;

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    ret = index_search_key_nolock(obj, addr_str, addr_size, len_str, len_size);
    if (ret != -INDEX_ERR_KEY_NOT_FOUND) // there is error if found
    {
        LOG_ERROR("Search key failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
        return ret;
    }

    if ((obj->ie->flags & INDEX_ENTRY_END) == 0) // it is not the last key
    { // check the current key
        addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
        len = (uint32_t)os_bstr_to_u64(GET_IE_VALUE(obj->ie), obj->ie->value_len);

        if (addr < (start_blk + blk_cnt))
        {
            LOG_ERROR("the tree chaos. objid(0x%llx)\n", obj->obj_info->objid);
            return -INDEX_ERR_CHAOS;
        }
        
        if (addr == (start_blk + blk_cnt))
        {
            ret = tree_remove_ie(obj);
            if (ret < 0)
            {
                LOG_ERROR("remove entry failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
                return ret;
            }
            
            blk_cnt += len;
            
            /*ret = walk_tree(obj, INDEX_GET_CURRENT);
            if (ret < 0)
            {
                LOG_ERROR("walk entry failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
                return ret;
            }*/
        }
    }

    while (1) // temp code, marked by axen.hook
    {
        ret = walk_tree(obj, INDEX_GET_PREV); // get prev key
        if (ret == 0)
        { // prev key got
            addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
            len = (uint32_t)os_bstr_to_u64(GET_IE_VALUE(obj->ie), obj->ie->value_len);
            
            if ((addr + len) > start_blk)
            {
                if (addr > start_blk)
                {
                    continue;
                }
                
                LOG_ERROR("the tree chaos. objid(0x%llx)\n", obj->obj_info->objid);
                return -INDEX_ERR_CHAOS;
            }
            
            if ((addr + len) == start_blk)
            {
                ret = tree_remove_ie(obj);
                if (ret < 0)
                {
                    LOG_ERROR("remove entry failed. objid(0x%llx) ret(%d)\n", obj->obj_info->objid, ret);
                    return ret;
                }
            
                start_blk = addr;
                blk_cnt += len;
            }
        }

        break;
    }

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    return index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
}

void ofs_init_sm(space_manager_t *sm, object_handle_t *obj, u64_t first_free_block,
    u64_t total_free_blocks)
{
    sm->space_obj = obj;
    sm->first_free_block = first_free_block;
    sm->total_free_blocks = total_free_blocks;
    OS_RWLOCK_INIT(&sm->lock);
}

int32_t ofs_init_free_space(space_manager_t *sm, u64_t start_blk, u64_t blk_cnt)
{
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    return index_insert_key_nolock(sm->space_obj, addr_str, addr_size, len_str, len_size);
}

void ofs_destroy_sm(space_manager_t *sm)
{
    if (sm->space_obj == NULL)
    {
        return;
    }
    
    close_object(sm->space_obj->obj_info);
    sm->space_obj = NULL;
    OS_RWLOCK_DESTROY(&sm->lock);
}

// return value:
// >  0: real blk cnt
// == 0: no free blk
// <  0: error code
int32_t sm_alloc_space(space_manager_t *sm, uint32_t blk_cnt, u64_t *real_start_blk)
{
    int32_t ret;

    OS_RWLOCK_WRLOCK(&sm->lock);
    if (sm->total_free_blocks == 0)
    {
        OS_RWLOCK_WRUNLOCK(&sm->lock);
        return -INDEX_ERR_NO_FREE_BLOCKS;
    }

    ret = alloc_space(sm->space_obj, sm->first_free_block, blk_cnt, real_start_blk);
    if (ret == -INDEX_ERR_NO_FREE_BLOCKS) // no space
    {
        sm->first_free_block = 0;
        sm->total_free_blocks = 0;
        OS_RWLOCK_WRUNLOCK(&sm->lock);
        return ret;
    }
    
    if (ret < 0) // error
    {
        OS_RWLOCK_WRUNLOCK(&sm->lock);
        LOG_ERROR("alloc space failed. objid(0x%llx) ret(%d)\n", sm->space_obj->obj_info->objid, ret);
        return ret;
    }


    sm->first_free_block = *real_start_blk + ret;
    if (sm->total_free_blocks < (u64_t)ret)
    {
        LOG_ERROR("the sm system chaos. objid(0x%llx) ret(%d)\n", sm->space_obj->obj_info->objid, ret);
    }
    else
    {
        sm->total_free_blocks -= ret;
    }
    
    OS_RWLOCK_WRUNLOCK(&sm->lock);

    LOG_DEBUG("alloc space, obj_id: %lld, start_blk: %lld, blk_cnt: %d\n",
        sm->space_obj->obj_info->objid, *real_start_blk, blk_cnt);

    return ret;
}

int32_t sm_free_space(space_manager_t *sm, u64_t start_blk, uint32_t blk_cnt)
{
    int32_t ret;
    
    OS_RWLOCK_WRLOCK(&sm->lock);
    ret = free_space(sm->space_obj, start_blk, blk_cnt);
    if (ret >= 0)
    {
        sm->total_free_blocks += blk_cnt;
    }
    OS_RWLOCK_WRUNLOCK(&sm->lock);

    LOG_DEBUG("free space, obj_id: %lld, start_blk: %lld, blk_cnt: %d\n",
        sm->space_obj->obj_info->objid, start_blk, blk_cnt);

    return ret;
}

int32_t reserve_base_space(container_handle_t *ct)
{
    uint32_t blk_cnt;
    int32_t ret;
    u64_t real_start_blk;

    if (ct->base_blk == 0)
    {
        ret = sm_alloc_space(&ct->sm, 1, &real_start_blk);
        if (ret < 0)
        {
            LOG_ERROR("allocate base block failed. ret(%d)\n", ret);
            return ret;
        }

        ct->base_blk = real_start_blk;
    }
    
    while (ct->bsm.total_free_blocks <= MIN_BLK_NUM)
    {
        blk_cnt = (uint32_t)(MAX_BLK_NUM - ct->bsm.total_free_blocks);
        ret = sm_alloc_space(&ct->sm, blk_cnt, &real_start_blk);
        if (ret < 0)
        {
            LOG_ERROR("allocate base space failed. blk_cnt(%d) ret(%d)\n", blk_cnt, ret);
            return ret;
        }

        blk_cnt = ret;
        ret = sm_free_space(&ct->bsm, real_start_blk, blk_cnt);
        if (ret < 0)
        {
            LOG_ERROR("free base space failed. blk_cnt(%d) ret(%d)\n", blk_cnt, ret);
            return ret;
        }
    }

    return 0;
}

// return value:
// >  0: real blk cnt
// == 0: no free blk
// <  0: error code
int32_t ofs_alloc_space(container_handle_t *ct, u64_t objid, uint32_t blk_cnt, u64_t *real_start_blk)
{
    int32_t ret;
    
    if (objid == ct->bsm.space_obj->obj_info->objid)
    {
        ASSERT(ct->base_blk != 0);
        ASSERT(blk_cnt == 1);
        
        *real_start_blk = ct->base_blk;
        ct->base_blk = 0;
        LOG_DEBUG("alloc space, obj_id: xxx, start_blk: %lld, blk_cnt: %d\n", *real_start_blk, blk_cnt);

        return blk_cnt;
    }

    if (objid == ct->sm.space_obj->obj_info->objid)
    {
        return sm_alloc_space(&ct->bsm, blk_cnt, real_start_blk);
    }

    ret = reserve_base_space(ct);
    if (ret < 0)
    {
        LOG_ERROR("reserve base space failed. ret(%d)\n", ret);
        return ret;
    }
    
    return sm_alloc_space(&ct->sm, blk_cnt, real_start_blk);
}

int32_t ofs_free_space(container_handle_t *ct, u64_t objid, u64_t start_blk, uint32_t blk_cnt)
{
    int32_t ret;

    if (objid == ct->bsm.space_obj->obj_info->objid)
    {
        ASSERT(ct->base_blk == 0);
        ASSERT(blk_cnt == 1);
        
        ct->base_blk = start_blk;
        LOG_DEBUG("free space, obj_id: xxx, start_blk: %lld, blk_cnt: %d\n", start_blk, blk_cnt);

        return 0;
    }
    
    if (objid == ct->sm.space_obj->obj_info->objid)
    {
        return sm_free_space(&ct->bsm, start_blk, blk_cnt);
    }

    ret = reserve_base_space(ct);
    if (ret < 0)
    {
        LOG_ERROR("reserve base space failed. ret(%d)\n", ret);
        return ret;
    }

    return sm_free_space(&ct->sm, start_blk, blk_cnt);
}


