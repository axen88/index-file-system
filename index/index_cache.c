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
文 件 名: OS_INDEX_IO.C
版    本: 1.00
日    期: 2011年6月19日
功能描述: 索引区的读写操作
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

#define INDEX_WALK_FINISHED       1

typedef struct tagFIND_CACHE_PARA_S
{
    uint64_t vbn;
    INDEX_BLOCK_CACHE *cache;
} FIND_CACHE_PARA_S;


int32_t compare_cache2(const uint64_t *vbn, INDEX_BLOCK_CACHE *cache_node)
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


/*******************************************************************************
函数名称: alloc_cache
功能说明: 分配cache
输入参数:
    obj: 要操作的树
输出参数: 无
返 回 值:
    ==NULL: 分配失败
    !=NULL: 分配成功的地址
说    明: 无
*******************************************************************************/
INDEX_BLOCK_CACHE *alloc_cache(ATTR_INFO *attr_info, uint64_t vbn)
{
    INDEX_BLOCK_CACHE *cache = NULL;

    ASSERT(NULL != attr_info);
    
    cache = OS_MALLOC(sizeof(INDEX_BLOCK_CACHE));
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(INDEX_BLOCK_CACHE));
        return NULL;
    }

    cache->ib = OS_MALLOC(attr_info->obj->index->hnd->sb.block_size);
    if (NULL == cache)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", attr_info->obj->index->hnd->sb.block_size);
        OS_FREE(cache);
        cache = NULL;
        return NULL;
    }

    cache->state = EMPTY;
    cache->vbn = vbn;

    return cache;
}

int32_t index_alloc_cache_and_block(ATTR_INFO *attr_info, INDEX_BLOCK_CACHE **cache)
{
    INDEX_BLOCK_CACHE *tmp_cache = NULL;
    int32_t ret = 0;
    uint64_t vbn = 0;

    ASSERT(NULL != attr_info);
    
    ret = block_alloc(attr_info->obj->index->hnd, 1, &vbn);
    if (0 > ret)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    tmp_cache = alloc_cache(attr_info, vbn);
    if (NULL == tmp_cache)
    {
        LOG_ERROR("Allocate cache failed.\n");
        block_free(attr_info->obj->index->hnd, vbn, 1);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    OS_RWLOCK_WRLOCK(&attr_info->obj->caches_lock);
    avl_add(&attr_info->attr_caches, tmp_cache);
    avl_add(&attr_info->obj->obj_caches, tmp_cache);
    OS_RWLOCK_WRUNLOCK(&attr_info->obj->caches_lock);
    
    *cache = tmp_cache;

    return 0;
}

/*******************************************************************************
函数名称: find_free_cache
功能说明: 查找free cache
输入参数:
    entry : cache所在的entry
    para: 查找cache的参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t find_free_cache(INDEX_BLOCK_CACHE **target_cache, INDEX_BLOCK_CACHE *cache)
{
    ASSERT(NULL != target_cache);
    ASSERT(NULL != cache);

    if (EMPTY == cache->state)
    {
        *target_cache = cache;
        return INDEX_WALK_FINISHED; /* 返回大于0表明已经找到 */
    }

    return 0;
}

INDEX_BLOCK_CACHE *index_find_free_cache(OBJECT_HANDLE * obj)
{
    INDEX_BLOCK_CACHE *cache = NULL;
    
    (void)avl_walk_all(&obj->obj_caches, (avl_walk_call_back)find_free_cache, &cache);

    return cache;
}

int32_t flush_cache_in_attr(ATTR_INFO *attr_info, INDEX_BLOCK_CACHE *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != attr_info);
    ASSERT(NULL != cache);

    if (DIRTY != cache->state)
    {
        return 0;
    }

    ret = index_update_block_fixup(attr_info->obj->index->hnd, &cache->ib->head,
        cache->vbn);
    if (ret != (int32_t)cache->ib->head.alloc_size)
    {
        LOG_ERROR("Update index block failed. name(%s) vbn(%lld) size(%d) ret(%d)\n",
            attr_info->obj->obj_name, cache->vbn, cache->ib->head.alloc_size, ret);
        return -INDEX_ERR_UPDATE;
    }

    LOG_DEBUG("Update index block success. name(%s) vbn(%lld) size(%d)\n",
            attr_info->obj->obj_name, cache->vbn, cache->ib->head.alloc_size);
    
    cache->state = CLEAN;

    return 0;
}

int32_t index_flush_all_caches_in_attr(ATTR_INFO * attr_info)
{
    OS_RWLOCK_WRLOCK(&attr_info->obj->caches_lock);
    avl_walk_all(&attr_info->attr_caches, (avl_walk_call_back)flush_cache_in_attr, attr_info);
    OS_RWLOCK_WRUNLOCK(&attr_info->obj->caches_lock);

    return 0;
}

int32_t flush_cache_in_obj(OBJECT_HANDLE *obj, INDEX_BLOCK_CACHE *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != obj);
    ASSERT(NULL != cache);

    if (DIRTY != cache->state)
    {
        return 0;
    }

    ret = index_update_block_fixup(obj->index->hnd, &cache->ib->head,
        cache->vbn);
    if (ret != (int32_t)cache->ib->head.alloc_size)
    {
        LOG_ERROR("Update index block failed. name(%s) vbn(%lld) size(%d) ret(%d)\n",
            obj->obj_name, cache->vbn, cache->ib->head.alloc_size, ret);
        return -INDEX_ERR_UPDATE;
    }

    LOG_DEBUG("Update index block success. name(%s) vbn(%lld) size(%d)\n",
            obj->obj_name, cache->vbn, cache->ib->head.alloc_size);
    
    cache->state = CLEAN;

    return 0;
}

int32_t index_flush_all_caches_in_obj(OBJECT_HANDLE * obj)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&(obj)->obj_caches, (avl_walk_call_back)flush_cache_in_obj, obj);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return 0;
}

int32_t release_free_cache(ATTR_INFO *attr_info, INDEX_BLOCK_CACHE *cache)
{
    ASSERT(NULL != attr_info);
    ASSERT(NULL != cache);

    if (EMPTY != cache->state)
    {
        return 0;
    }

    avl_remove(&attr_info->attr_caches, cache);
    avl_remove(&attr_info->obj->obj_caches, cache);
    
    OS_FREE(cache->ib);
    cache->ib = NULL;
    OS_FREE(cache);
    cache = NULL;

    return 0;
}

int32_t index_release_all_free_caches_in_attr(OBJECT_HANDLE *obj, ATTR_INFO *attr_info)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&attr_info->attr_caches, (avl_walk_call_back)release_free_cache, attr_info);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return 0;
}

void index_release_all_free_caches_in_obj(OBJECT_HANDLE * obj)
{
    index_release_all_free_caches_in_attr(obj, &obj->attr_info);

    return;
}

int32_t release_cache(ATTR_INFO *attr_info, INDEX_BLOCK_CACHE *cache)
{
    ASSERT(NULL != attr_info);
    ASSERT(NULL != cache);

    if (DIRTY == cache->state)
    {
        LOG_INFO("The dirty cache will be released. name(%s) vbn(%lld)\n",
            attr_info->obj->obj_name, cache->vbn);
    }

    avl_remove(&attr_info->attr_caches, cache);
    avl_remove(&attr_info->obj->obj_caches, cache);
    
    OS_FREE(cache->ib);
    cache->ib = NULL;
    OS_FREE(cache);
    cache = NULL;

    return 0;
}

int32_t index_release_all_caches_in_attr(OBJECT_HANDLE *obj, ATTR_INFO *attr_info)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&attr_info->attr_caches, (avl_walk_call_back)release_cache, attr_info);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return 0;
}

int32_t index_release_all_caches_in_obj(OBJECT_HANDLE * obj)
{
    index_release_all_caches_in_attr(obj, &obj->attr_info);

    return 0;
}

int32_t cancel_cache(ATTR_INFO *attr_info, INDEX_BLOCK_CACHE *cache)
{
    int32_t ret = 0;

    ASSERT(NULL != attr_info);
    ASSERT(NULL != cache);

    if (DIRTY != cache->state)
    {
        return 0;
    }

    ret = INDEX_FREE_BLOCK(attr_info->obj, cache->vbn);
    if (0 > ret)
    {
        LOG_ERROR("Free block failed. vbn(%lld) ret(%d)\n", cache->vbn, ret);
    }

    cache->state = EMPTY;

    return 0;
}

int32_t index_cancel_all_caches_in_attr(OBJECT_HANDLE *obj, ATTR_INFO *attr_info)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&attr_info->attr_caches, (avl_walk_call_back)cancel_cache, attr_info);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return 0;
}

int32_t index_cancel_all_caches_in_obj(OBJECT_HANDLE * obj)
{
    index_cancel_all_caches_in_attr(obj, &obj->attr_info);

    return 0;
}

int32_t release_old_block(ATTR_INFO *attr_info, INDEX_OLD_BLOCK *old_blk)
{
    int32_t ret = 0;

    ASSERT(NULL != attr_info);
    ASSERT(NULL != old_blk);

    ret = INDEX_FREE_BLOCK(attr_info->obj, old_blk->vbn);
    if (0 > ret)
    {
        LOG_ERROR("Free block failed. vbn(%lld) ret(%d)\n", old_blk->vbn, ret);
    }

    avl_remove(&attr_info->attr_old_blocks, old_blk);
    avl_remove(&attr_info->obj->obj_old_blocks, old_blk);

    OS_FREE(old_blk);

    return 0;
}

void index_release_all_old_blocks_in_attr(OBJECT_HANDLE *obj, ATTR_INFO *attr_info)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&attr_info->attr_old_blocks, (avl_walk_call_back)release_old_block, attr_info);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return;
}

void index_release_all_old_blocks_in_obj(OBJECT_HANDLE * obj)
{
    index_release_all_old_blocks_in_attr(obj, &obj->attr_info);

    return;
}

int32_t release_old_block_mem(ATTR_INFO *attr_info, INDEX_OLD_BLOCK *old_blk)
{
    int32_t ret = 0;

    ASSERT(NULL != attr_info);
    ASSERT(NULL != old_blk);

    avl_remove(&attr_info->attr_old_blocks, old_blk);
    avl_remove(&attr_info->obj->obj_old_blocks, old_blk);

    OS_FREE(old_blk);

    return 0;
}

void index_release_all_old_blocks_mem_in_attr(OBJECT_HANDLE *obj, ATTR_INFO *attr_info)
{
    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    avl_walk_all(&attr_info->attr_old_blocks, (avl_walk_call_back)release_old_block_mem, attr_info);
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return;
}

void index_release_all_old_blocks_mem_in_obj(OBJECT_HANDLE * obj)
{
    index_release_all_old_blocks_mem_in_attr(obj, &obj->attr_info);

    return;
}


/*******************************************************************************
函数名称: index_block_read
功能说明: 读取指定位置的索引块
输入参数:
    tree: 要操作的树
    vbn  : 要读取的位置
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t index_block_read(ATTR_HANDLE * tree, uint64_t vbn)
{
    int32_t ret = 0;
    INDEX_BLOCK *ib = NULL;
    INDEX_BLOCK_CACHE *cache = NULL;
    OBJECT_HANDLE *obj = NULL;
    avl_index_t where = 0;

    /* 检查输入参数 */
    ASSERT(NULL != tree);
    
    obj = tree->attr_info->obj;

    if (tree->cache->vbn == vbn)
    {   // 就是当前操作的cache
        return 0;
    }

    OS_RWLOCK_WRLOCK(&obj->caches_lock);
    cache = avl_find(&obj->obj_caches, (int (*)(const void*, void *))compare_cache2, &vbn, &where);
    if (NULL != cache)
    { /* cache命中 */
        OS_RWLOCK_WRUNLOCK(&obj->caches_lock);
        tree->cache = cache;
        return 0;
    }

    //cache = index_find_free_cache(obj);
    //if (NULL == cache)
    //{ /* 未找到了未使用的cache，那么就申请新cache */
        cache = alloc_cache(tree->attr_info, vbn);
        if (NULL == cache)
        {
            OS_RWLOCK_WRUNLOCK(&obj->caches_lock);
            LOG_ERROR("Allocate cache failed.\n");
            return -INDEX_ERR_ALLOCATE_MEMORY;
        }
    //}
    //else
    //{
    //    avl_remove(&tree->attr_info->attr_caches, cache);
    //    avl_remove(&obj->obj_caches, cache);
    //    cache->vbn = vbn;
    //}
    
    avl_add(&tree->attr_info->attr_caches, cache);
    avl_add(&obj->obj_caches, cache);

    tree->cache = cache;
    ib = cache->ib;

    ret = index_read_block_fixup(obj->index->hnd, &ib->head, vbn,
        INDEX_MAGIC, obj->index->hnd->sb.block_size);
    if (ret < 0)
    {   // Read the index block
        LOG_ERROR("Read index block failed. name(%s) ib(%p) vbn(%lld) size(%d) ret(%d)\n",
            obj->obj_name, ib, vbn, obj->index->hnd->sb.block_size, ret);
        OS_RWLOCK_WRUNLOCK(&obj->caches_lock);
        return ret;
    }

    LOG_DEBUG("Read index block success. name(%s) ib(%p) vbn(%lld) size(%d)\n",
        obj->obj_name, ib, vbn, obj->index->hnd->sb.block_size);

    tree->cache->state = CLEAN;
    OS_RWLOCK_WRUNLOCK(&obj->caches_lock);

    return 0;
}

/*******************************************************************************
函数名称: index_record_old_block
功能说明: 将块记录到指定的目标块队列
输入参数:
    vbn   : 要记录的块的起始地址
    blk_cnt: 要记录的块数目
输出参数:
    q    : 目标块队列
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t index_record_old_block(ATTR_INFO *attr_info, uint64_t vbn)
{
    INDEX_OLD_BLOCK *old_blk = NULL;

    /* 检查输入参数 */
    ASSERT(NULL != attr_info);

    old_blk = OS_MALLOC(sizeof(INDEX_OLD_BLOCK));
    if (NULL == old_blk)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(INDEX_OLD_BLOCK));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    old_blk->vbn = vbn;

    OS_RWLOCK_WRLOCK(&attr_info->obj->caches_lock);
    avl_add(&attr_info->attr_old_blocks, old_blk);
    avl_add(&attr_info->obj->obj_old_blocks, old_blk);
    OS_RWLOCK_WRUNLOCK(&attr_info->obj->caches_lock);
    
    return 0;
}

