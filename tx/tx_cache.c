



#include "tx_cache.h"

cache_node_t *alloc_cache_node(cache_mgr_t *mgr, cache_node_t *src_cache)
{
    int cache_size = sizeof(cache_node_t) + mgr->block_size;
    cache_node_t *cache = OS_MALLOC(cache_size);
    if (cache == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed\n", cache_size);
        return NULL;
    }

    // 只初始化头
    memset(cache, 0, cache_size);

    if (src_cache != NULL)
    {
        cache->block_id = src_cache->block_id;
        
        // 拷贝数据，多一个副本
        memcpy(cache->dat, src_cache->dat, mgr->block_size);
    }

    return cache;
}

void free_cache_node(cache_mgr_t *mgr, cache_node_t *cache)
{
    hashtab_delete(&mgr->hcache, cache->block_id);
    if (cache->ref_cnt != 0)
    {
        LOG_ERROR("The cache block(%lld) buf(0x%p) is still be referenced\n", cache->block_id, buf);
        return -ERR_PUT_BUF;
    }

    OS_FREE(cache);
}

// buf_type
cache_node_t *get_nonread_cache_node(cache_mgr_t *mgr, cache_node_t *read_cache, BUF_TYPE_E buf_type)
{
    ASSERT(buf_type != READ_BUF);
    uint8_t  checkpoint_side = (mgr->writing_side + 1) & 0x1;

    if (buf_type == WRITE_BUF)
    {
        if (read_cache->side_node[mgr->writing_side] != NULL)
            return read_cache->side_node[mgr->writing_side];
    }
    else // CHECKPOINT_BUF
    {
        if (read_cache->side_node[checkpoint_side] != NULL)
            return read_cache->side_node[checkpoint_side];
    }
    
    cache_node_t *cache = alloc_cache_node(mgr, read_cache);
    if (cache != NULL)
    {
        if (buf_type == WRITE_BUF)
        {
            read_cache->side_node[mgr->writing_side] = cache;
        }
        else // CHECKPOINT_BUF
        {
            read_cache->side_node[checkpoint_side] = cache;
        }
    }

    return cache;
}

cache_node_t *get_cache_node(cache_mgr_t *mgr, uint64_t block_id, BUF_TYPE_E buf_type)
{
    int ret;
    
    cache_node_t *read_cache = hashtab_search(&mgr->hcache, block_id);
    if (read_cache != NULL)
    {
        if (buf_type == READ_BUF)
            return read_cache;

        return get_nonread_cache_node(mgr, read_cache);
    }

    // 这里申请的cache是读cache
    read_cache = alloc_cache_node(mgr, NULL);
    if (read_cache == NULL)
    {
        LOG_ERROR("alloc cache block(%lld) buf failed\n", block_id);
        return NULL;
    }

    read_cache->block_id = block_id;
    hashtab_insert(&mgr->hcache, block_id, read_cache);

    // 第一次申请的就不是读buf，说明不想读盘
    if (buf_type != READ_BUF)
    {
        return get_nonread_cache_node(mgr, read_cache);
    }

    // 读盘
    ret = os_disk_pread(mgr->bd_hnd, read_cache->dat, mgr->block_size, block_id);
    if (ret <= 0)
    {
        hashtab_delete(&mgr->hcache, block_id);
        free_cache_node(read_cache);
        LOG_ERROR("read block failed. bd_hnd(%p) buf(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, read_cache->dat, mgr->block_size, block_id, ret);
        return NULL;
    }

    return read_cache;
}

void *get_buffer(cache_mgr_t *mgr, uint64_t block_id, BUF_TYPE_E buf_type)
{
    cache_node_t *cache = get_cache_node(mgr, block_id, buf_type);

    if (cache == NULL)
        return NULL;

    cache->ref_cnt++;
    return cache->dat;
}

int put_buffer(cache_mgr_t *mgr, void *buf)
{
    cache_node_t *cache = list_entry(buf, cache_node_t, buf);

    if (cache->ref_cnt == 0)
    {
        LOG_ERROR("put block(%lld) buf(0x%p) failed\n", cache->block_id, buf);
        return -ERR_PUT_BUF;
    }

    cache->ref_cnt--;
    LOG_INFO("put block(%lld) buf(0x%p) success\n", cache->block_id, buf);

    return SUCCESS;
}

int commit_cache(cache_mgr_t *mgr, cache_node_t *cache)
{
    
}

int cache_init_system(char *bd_name, uint32_t bd_block_size)
{
    int ret = 0;
    
    cache_mgr_t *mgr = OS_MALLOC(sizeof(cache_mgr_t));
    if (mgr == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed.\n", sizeof(cache_mgr_t));
        return ret;
    }

    memset(mgr, 0, sizeof(cache_mgr_t));
    
    ret = os_disk_open(&mgr->bd_hnd, bd_name);
    if (ret < 0)
    {
        LOG_ERROR("Open block device(%s) failed(%d).\n", bd_name, ret);
        (void)OS_FREE(mgr);
        return ret;
    }

    

}

void cache_exit_system(cache_mgr_t *mgr)
{

}

