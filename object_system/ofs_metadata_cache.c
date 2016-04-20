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
File Name: INDEX_CACHE.C
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

MODULE(PID_CACHE);
#include "os_log.h"

#define INDEX_WALK_FINISHED       1


int32_t compare_cache2(const uint64_t *vbn, ofs_block_cache_t *cache_node)
{
    if (*vbn > cache_node->vbn)
    {
        return 1;
    }
    
    if (*vbn < cache_node->vbn)
    {
        return -1;
    }

    return 0;
}

void insert_obj_cache(object_info_t *obj_info, ofs_block_cache_t *cache)
{
    container_handle_t *ct = obj_info->ct;
    
    avl_add(&obj_info->caches, cache); // add to object
    
    OS_RWLOCK_WRLOCK(&ct->metadata_cache_lock);
    avl_add(&ct->metadata_cache, cache); // add to fs
    OS_RWLOCK_WRUNLOCK(&ct->metadata_cache_lock);
    
}

void remove_obj_cache(object_info_t *obj_info, ofs_block_cache_t *cache)
{
    container_handle_t *ct = obj_info->ct;
    
    avl_remove(&obj_info->caches, cache); // remove from object
    
    OS_RWLOCK_WRLOCK(&ct->metadata_cache_lock);
    avl_remove(&ct->metadata_cache, cache); // remove from fs
    OS_RWLOCK_WRUNLOCK(&ct->metadata_cache_lock);
    
}


void change_obj_cache_vbn(object_info_t *obj_info, ofs_block_cache_t *cache, uint64_t new_vbn)
{
    remove_obj_cache(obj_info, cache);
    cache->vbn = new_vbn;
    insert_obj_cache(obj_info, cache);
}



ofs_block_cache_t *alloc_obj_cache(object_info_t *obj_info, uint64_t vbn, uint32_t blk_id)
{
    ofs_block_cache_t *cache = NULL;

    ASSERT(NULL != obj_info);
    
    cache = OS_MALLOC(sizeof(ofs_block_cache_t));
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(ofs_block_cache_t));
        return NULL;
    }

    cache->ib = OS_MALLOC(obj_info->ct->sb.block_size);
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", obj_info->ct->sb.block_size);
        OS_FREE(cache);
        return NULL;
    }

    SET_CACHE_EMPTY(cache);
    cache->vbn = vbn;
    
    insert_obj_cache(obj_info, cache);

    return cache;
}

void free_obj_cache(object_info_t *obj_info, ofs_block_cache_t *cache)
{
    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);
    
    remove_obj_cache(obj_info, cache);
    
    if (NULL != cache->ib)
    {
        OS_FREE(cache->ib);
        cache->ib = NULL;
    }
    
    OS_FREE(cache);

}

int32_t alloc_obj_cache_and_block(object_info_t *obj_info, ofs_block_cache_t **cache, uint32_t blk_id)
{
    ofs_block_cache_t *tmp_cache = NULL;
    int32_t ret = 0;
    uint64_t vbn = 0;

    ASSERT(NULL != obj_info);
    
    ret = OFS_ALLOC_BLOCK(obj_info->ct, obj_info->objid, &vbn);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    tmp_cache = alloc_obj_cache(obj_info, vbn, blk_id);
    if (NULL == tmp_cache)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        LOG_ERROR("Allocate cache failed.\n");
        OFS_FREE_BLOCK(obj_info->ct, obj_info->objid, vbn);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
    
    *cache = tmp_cache;

    return 0;
}

int32_t release_obj_cache(object_info_t *obj_info, ofs_block_cache_t *cache)
{
    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);

    if (CACHE_DIRTY(cache))
    {
        SET_CACHE_FLUSH(cache);
        avl_remove(&obj_info->caches, cache);  // remove from obj tree
    }
    else
    {
        free_obj_cache(obj_info, cache); // remove from obj and fs tree
    }

    return 0;
}

int32_t release_obj_all_cache(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->caches, (avl_walk_cb_t)release_obj_cache, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return 0;
}



int32_t index_block_read2(object_info_t *obj_info, uint64_t vbn, uint32_t blk_id,
    ofs_block_cache_t **cache_out)
{
    int32_t ret = 0;
    ofs_block_cache_t *cache = NULL;
    avl_index_t where = 0;

    ASSERT(NULL != obj_info);
    
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    cache = avl_find(&obj_info->caches, (avl_find_fn_t)compare_cache2, &vbn, &where);
    if (NULL != cache) // block already in the cache
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        *cache_out = cache;
        return 0;
    }

    cache = alloc_obj_cache(obj_info, vbn, blk_id);
    if (NULL == cache)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        LOG_ERROR("Allocate cache failed.\n");
        *cache_out = NULL;
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }
    
    ret = ofs_read_block_fixup(obj_info->ct, cache->ib, vbn, blk_id, obj_info->ct->sb.block_size);
    if (ret < 0)
    {   // Read the block
        LOG_ERROR("Read ct block failed. objid(%lld) vbn(%lld) size(%d) ret(%d)\n",
            obj_info->objid, vbn, obj_info->ct->sb.block_size, ret);
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        *cache_out = NULL;
        return ret;
    }

    LOG_DEBUG("Read ct block success. objid(%lld) vbn(%lld) size(%d)\n",
        obj_info->objid, vbn, obj_info->ct->sb.block_size);

    SET_CACHE_CLEAN(cache);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    *cache_out = cache;
    return 0;
}

int32_t index_block_read(object_handle_t *obj, uint64_t vbn, uint32_t blk_id)
{
    int32_t ret = 0;

    ASSERT(NULL != obj);
    
    if (obj->cache->vbn == vbn)
    {
        return 0;
    }

    ret = index_block_read2(obj->obj_info, vbn, blk_id, &obj->cache);
    if (ret < 0)
    {
        LOG_ERROR("Read block failed. vbn(%lld) blk_id(0x%x) ret(%d)\n", vbn, blk_id, ret);
        return ret;
    }

    return 0;
}

void free_fs_cache(container_handle_t *ct, ofs_block_cache_t *cache)
{
    ASSERT(NULL != ct);
    ASSERT(NULL != cache);
    
    avl_remove(&ct->metadata_cache, cache);
    
    if (NULL != cache->ib)
    {
        OS_FREE(cache->ib);
        cache->ib = NULL;
    }
    
    OS_FREE(cache);

}

int32_t flush_fs_dirty_cache(container_handle_t *ct, ofs_block_cache_t *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != ct);
    ASSERT(NULL != cache);

    if (!CACHE_DIRTY(cache))
    {
        return 0;
    }

    ret = ofs_update_block_fixup(ct, cache->ib, cache->vbn);
    if (ret != (int32_t)cache->ib->alloc_size)
    {
        LOG_ERROR("Update ct block failed. ct(%s) vbn(%lld) size(%d) ret(%d)\n",
            ct->name, cache->vbn, cache->ib->alloc_size, ret);
        return -INDEX_ERR_UPDATE;
    }

    LOG_DEBUG("Update ct block success. ct(%s) vbn(%lld) size(%d)\n",
            ct->name, cache->vbn, cache->ib->alloc_size);

    if (CACHE_FLUSH(cache))  // the object had been released
    {
        free_fs_cache(ct, cache);
        return 0;
    }
    
    SET_CACHE_CLEAN(cache);

    return 0;
}


int32_t flush_fs_cache(container_handle_t *ct)
{
    int32_t ret = 0;
    
    ASSERT(ct != NULL);

    OS_RWLOCK_WRLOCK(&ct->metadata_cache_lock);
    avl_walk_all(&ct->metadata_cache, (avl_walk_cb_t)flush_fs_dirty_cache, ct);
    OS_RWLOCK_WRUNLOCK(&ct->metadata_cache_lock);

    if (ct->flags & FLAG_DIRTY)
    {
        ct->sb.free_blocks = ct->sm.total_free_blocks;
        ct->sb.first_free_block = ct->sm.first_free_block;
        
        ct->sb.base_free_blocks = ct->bsm.total_free_blocks;
        ct->sb.base_first_free_block = ct->bsm.first_free_block;
        
        ct->sb.base_blk = ct->base_blk;

        ret = ofs_update_super_block(ct);
        if (0 > ret)
        {
            LOG_ERROR("Update super block failed. ct(%p) ret(%d)\n", ct, ret);
        }

        ct->flags &= ~FLAG_DIRTY;
    }

	return 0;
}

int32_t release_fs_cache(container_handle_t *ct, ofs_block_cache_t *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != ct);
    ASSERT(NULL != cache);

    if (CACHE_DIRTY(cache))
    {
        LOG_ERROR("YOU ARE RELEASE DIRTY CACHE. ct(%s) vbn(%lld) size(%d) ret(%d)\n",
            ct->name, cache->vbn, cache->ib->alloc_size, ret);
    }

    free_fs_cache(ct, cache);

    return 0;
}

int32_t release_fs_all_cache(container_handle_t *ct)
{
    int32_t ret = 0;
    
    ASSERT(ct != NULL);

    OS_RWLOCK_WRLOCK(&ct->metadata_cache_lock);
    avl_walk_all(&ct->metadata_cache, (avl_walk_cb_t)release_fs_cache, ct);
    OS_RWLOCK_WRUNLOCK(&ct->metadata_cache_lock);

    return 0;
}



