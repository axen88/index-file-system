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


#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

#define INDEX_WALK_FINISHED       1


int32_t compare_cache2(const uint64_t *vbn, ifs_block_cache_t *cache_node)
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

ifs_block_cache_t *alloc_cache(object_info_t *obj_info, uint64_t vbn)
{
    ifs_block_cache_t *cache = NULL;

    ASSERT(NULL != obj_info);
    
    cache = OS_MALLOC(sizeof(ifs_block_cache_t));
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(ifs_block_cache_t));
        return NULL;
    }

    cache->ib = OS_MALLOC(obj_info->index->hnd->sb.block_size);
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", obj_info->index->hnd->sb.block_size);
        OS_FREE(cache);
        return NULL;
    }

    cache->state = EMPTY;
    cache->vbn = vbn;
    
    avl_add(&obj_info->caches, cache);

    return cache;
}

void free_cache(object_info_t *obj_info, ifs_block_cache_t *cache)
{
    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);
    
    avl_remove(&obj_info->caches, cache);
    
    if (NULL != cache->ib)
    {
        OS_FREE(cache->ib);
        cache->ib = NULL;
    }
    
    OS_FREE(cache);

}

int32_t index_alloc_cache_and_block(object_info_t *obj_info, ifs_block_cache_t **cache)
{
    ifs_block_cache_t *tmp_cache = NULL;
    int32_t ret = 0;
    uint64_t vbn = 0;

    ASSERT(NULL != obj_info);
    
    ret = INDEX_ALLOC_BLOCK(obj_info->index, obj_info->objid, &vbn);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    tmp_cache = alloc_cache(obj_info, vbn);
    if (NULL == tmp_cache)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        LOG_ERROR("Allocate cache failed.\n");
        INDEX_FREE_BLOCK(obj_info->index, obj_info->objid, vbn);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
    
    *cache = tmp_cache;

    return 0;
}

int32_t flush_dirty_cache(object_info_t *obj_info, ifs_block_cache_t *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);

    if (DIRTY != cache->state)
    {
        return 0;
    }

    ret = index_update_block_fixup(obj_info->index->hnd, &cache->ib->head, cache->vbn);
    if (ret != (int32_t)cache->ib->head.alloc_size)
    {
        LOG_ERROR("Update index block failed. objid(%lld) vbn(%lld) size(%d) ret(%d)\n",
            obj_info->objid, cache->vbn, cache->ib->head.alloc_size, ret);
        return -INDEX_ERR_UPDATE;
    }

    LOG_DEBUG("Update index block success. objid(%lld) vbn(%lld) size(%d)\n",
            obj_info->objid, cache->vbn, cache->ib->head.alloc_size);
    
    cache->state = CLEAN;

    return 0;
}

int32_t index_flush_all_dirty_caches(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->caches, (avl_walk_call_back)flush_dirty_cache, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return 0;
}

int32_t release_cache(object_info_t *obj_info, ifs_block_cache_t *cache)
{
    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);

    if (DIRTY == cache->state)  // all cache should be CLEAN or EMPTY state
    {
        LOG_ERROR("The dirty cache will be released. objid(%lld) vbn(%lld)\n",
            obj_info->objid, cache->vbn);
    }

    free_cache(obj_info, cache);

    return 0;
}

int32_t index_release_all_caches(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->caches, (avl_walk_call_back)release_cache, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return 0;
}

int32_t release_dirty_block(object_info_t *obj_info, ifs_block_cache_t *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != obj_info);
    ASSERT(NULL != cache);

    if (DIRTY != cache->state)
    {
        return 0;
    }

    ret = INDEX_FREE_BLOCK(obj_info->index, obj_info->objid, cache->vbn);
    if (0 > ret)
    {
        LOG_ERROR("Free block failed. vbn(%lld) ret(%d)\n", cache->vbn, ret);
    }

    free_cache(obj_info, cache);

    return 0;
}

int32_t index_release_all_dirty_blocks(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->caches, (avl_walk_call_back)release_dirty_block, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return 0;
}

int32_t release_old_block_mem(object_info_t *obj_info, ifs_old_block_t *old_blk)
{
    int32_t ret = 0;

    ASSERT(NULL != obj_info);
    ASSERT(NULL != old_blk);

    avl_remove(&obj_info->old_blocks, old_blk);
    OS_FREE(old_blk);

    return 0;
}

void index_release_all_old_blocks_mem(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->old_blocks, (avl_walk_call_back)release_old_block_mem, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return;
}

int32_t release_old_block(object_info_t *obj_info, ifs_old_block_t *old_blk)
{
    int32_t ret = 0;

    ASSERT(NULL != obj_info);
    ASSERT(NULL != old_blk);

    ret = INDEX_FREE_BLOCK(obj_info->index, obj_info->objid, old_blk->vbn);
    if (0 > ret)
    {
        LOG_ERROR("Free block failed. vbn(%lld) ret(%d)\n", old_blk->vbn, ret);
    }

    release_old_block_mem(obj_info, old_blk);

    return 0;
}

void index_release_all_old_blocks(object_info_t *obj_info)
{
    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_walk_all(&obj_info->old_blocks, (avl_walk_call_back)release_old_block, obj_info);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return;
}

int32_t index_block_read(object_handle_t *obj, uint64_t vbn)
{
    int32_t ret = 0;
    INDEX_BLOCK *ib = NULL;
    ifs_block_cache_t *cache = NULL;
    avl_index_t where = 0;
    object_info_t *obj_info;

    ASSERT(NULL != obj);
    
    obj_info = obj->obj_info;

    if (obj->cache->vbn == vbn)
    {
        return 0;
    }

    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    cache = avl_find(&obj_info->caches, (avl_find_fn)compare_cache2, &vbn, &where);
    if (NULL != cache) // block already in the cache
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        obj->cache = cache;
        return 0;
    }

    cache = alloc_cache(obj_info, vbn);
    if (NULL == cache)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        LOG_ERROR("Allocate cache failed.\n");
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }
    
    obj->cache = cache;
    ib = cache->ib;

    ret = index_read_block_fixup(obj->index->hnd, &ib->head, vbn,
        INDEX_MAGIC, obj->index->hnd->sb.block_size);
    if (ret < 0)
    {   // Read the block
        LOG_ERROR("Read index block failed. objid(%lld) ib(%p) vbn(%lld) size(%d) ret(%d)\n",
            obj_info->objid, ib, vbn, obj->index->hnd->sb.block_size, ret);
        OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
        return ret;
    }

    LOG_DEBUG("Read index block success. objid(%lld) ib(%p) vbn(%lld) size(%d)\n",
        obj_info->objid, ib, vbn, obj->index->hnd->sb.block_size);

    obj->cache->state = CLEAN;
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);

    return 0;
}

int32_t index_record_old_block(object_info_t *obj_info, uint64_t vbn)
{
    ifs_old_block_t *old_blk = NULL;

    ASSERT(NULL != obj_info);

    old_blk = OS_MALLOC(sizeof(ifs_old_block_t));
    if (NULL == old_blk)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(ifs_old_block_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    old_blk->vbn = vbn;

    OS_RWLOCK_WRLOCK(&obj_info->caches_lock);
    avl_add(&obj_info->old_blocks, old_blk);
    OS_RWLOCK_WRUNLOCK(&obj_info->caches_lock);
    
    return 0;
}

