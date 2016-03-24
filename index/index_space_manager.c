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

// return value:
// >  0: real blk cnt
// == 0: no free blk
// <  0: error code
int32_t index_alloc_space(space_manager_t *sm, uint32_t blk_cnt, uint64_t *start_blk)
{
    int32_t ret;
    OBJECT_HANDLE *obj = sm->free_blk_obj;
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;
    uint64_t addr;
    uint64_t len;

    if (0 == sm->total_free_blocks)
    {
        return -INDEX_ERR_NO_FREE_BLOCKS;
    }

    addr_size = os_u64_to_bstr(sm->first_free_block, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);
    
    ret = search_key_internal(obj, addr_str, addr_size, len_str, len_size);
    if (ret != 0)
    {
        return ret;
    }

    addr = os_bstr_to_u64(GET_IE_KEY(obj->ie), obj->ie->key_len);
    len = os_bstr_to_u64(GET_IE_VALUE(obj->ie), obj->ie->value_len);
    

//    tree_remove_ie(obj);
    if (addr < sm->first_free_block)
    {
        
    }

    
    ret = walk_tree(obj, INDEX_GET_CURRENT);
    if (0 != ret)
    {
        return ret;
    }
    
    return 0;
}

int32_t index_free_space(space_manager_t *sm, uint64_t start_blk, uint32_t blk_cnt)
{
    OBJECT_HANDLE *obj = sm->free_blk_obj;
    int32_t ret;
    uint8_t addr_str[U64_MAX_SIZE];
    uint8_t len_str[U64_MAX_SIZE];
    uint16_t addr_size;
    uint16_t len_size;

    addr_size = os_u64_to_bstr(start_blk, addr_str);
    len_size = os_u64_to_bstr(blk_cnt, len_str);
    
    ret = search_key_internal(obj, addr_str, addr_size, len_str, len_size);
    if (0 != ret)
    {
        return ret;
    }

    ret = walk_tree(obj, INDEX_GET_CURRENT);
    if (0 != ret)
    {
        return ret;
    }
    

    return 0;
}


