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
File Name: OFS_EXTENT_MAP.C
Author   : axen.hook
Version  : 1.00
Date     : 08/May/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 08/May/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#include "ofs_if.h"
 
MODULE(MID_EXTENT_MAP);
#include "log.h"

#if 0

int32_t insert_extent(object_handle_t *obj, u64_t vbn, u64_t lbn, uint32_t blk_cnt)
{
    u64_t start_blk;
    u64_t addr;
    uint32_t len;
    u64_t end;
    u64_t end_blk;
    uint8_t vbn_str[U64_MAX_SIZE];
    uint8_t ext_pair[EXT_PAIR_MAX_SIZE];
    u64_t ext_vbn;
    u64_t ext_lbn;
    u64_t ext_len;
    uint16_t vbn_size;
    uint16_t len_size;
    uint32_t ext_pair_size;
    int32_t ret;

    ASSERT(blk_cnt != 0);

    vbn_size = os_u64_to_bstr(vbn, vbn_str);
    ext_pair_size = os_extent_to_extent_pair(lbn, blk_cnt, ext_pair);
    
    ret = index_search_key_nolock(obj, vbn_str, vbn_size, ext_pair, ext_pair_size);
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

int32_t remove_extent(object_handle_t *obj, u64_t vbn, u64_t lbn, uint32_t blk_cnt)
{
    u64_t start_blk;
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
        }
    }

    ret = walk_tree(obj, INDEX_GET_PREV); // get prev key
    if (ret == 0)
    { // prev key got
        addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
        len = (uint32_t)os_bstr_to_u64(GET_IE_VALUE(obj->ie), obj->ie->value_len);
        
        if ((addr + len) > start_blk)
        {
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

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);

    return index_insert_key_nolock(obj, addr_str, addr_size, len_str, len_size);
}

#endif

