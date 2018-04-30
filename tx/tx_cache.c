

#include "os_adapter.h"
#include "globals.h"
#include "disk_if.h"


MODULE(PID_CACHE);
#include "log.h"

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
    hashtab_delete(mgr->hcache, (void *)cache->block_id);
    if (cache->ref_cnt != 0)
    {
        LOG_ERROR("The cache block(%lld) is still be referenced\n", cache->block_id);
        //return;
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
        
        cache->read_node = read_cache;
    }

    return cache;
}

cache_node_t *get_cache_node(cache_mgr_t *mgr, uint64_t block_id, BUF_TYPE_E buf_type)
{
    int ret;
    cache_node_t *read_cache = hashtab_search(mgr->hcache, (void *)block_id);
    if (read_cache != NULL)
    {
        if (buf_type == READ_BUF)
            return read_cache;

        return get_nonread_cache_node(mgr, read_cache, buf_type);
    }

    // 这里申请的cache是读cache
    read_cache = alloc_cache_node(mgr, NULL);
    if (read_cache == NULL)
    {
        LOG_ERROR("alloc cache block(%lld) buf failed\n", block_id);
        return NULL;
    }

    read_cache->block_id = block_id;
    ret = hashtab_insert(mgr->hcache, (void *)block_id, read_cache);
    if (ret < 0)
    {
        free_cache_node(mgr, read_cache);
        LOG_ERROR("hashtab_insert failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, block_id, ret);
        return NULL;
    }

    // 第一次申请的就不是读buf，说明不想读盘
    if (buf_type != READ_BUF)
    {
        return get_nonread_cache_node(mgr, read_cache, buf_type);
    }

    // 读盘
    ret = os_disk_pread(mgr->bd_hnd, read_cache->dat, mgr->block_size, block_id);
    if (ret <= 0)
    {
        free_cache_node(mgr, read_cache);
        LOG_ERROR("read block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, block_id, ret);
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
    cache_node_t *cache = list_entry(buf, cache_node_t, dat);

    if (cache->ref_cnt == 0)
    {
        LOG_ERROR("put block(%lld) buf(0x%p) failed\n", cache->block_id, buf);
        return -ERR_PUT_BUF;
    }

    cache->ref_cnt--;
    LOG_INFO("put block(%lld) buf(0x%p) success\n", cache->block_id, buf);

    return SUCCESS;
}

int commit_cache(cache_mgr_t *mgr, cache_node_t *write_cache)
{
    int ret;
    
    ASSERT(write_cache->read_node != NULL);  // 确保不是读cache被修改

    // 数据下盘
    ret = os_disk_pwrite(mgr->bd_hnd, write_cache->dat, mgr->block_size, write_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, write_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }
    
    return 0;
}

void *commit_disk(void *arg)
{
    cache_mgr_t *mgr = arg;

    return NULL;
}

tx_t *start_tx(cache_mgr_t *mgr)
{
    tx_t *tx = OS_MALLOC(sizeof(tx_t));
    if (tx == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed.\n", sizeof(tx_t));
        return NULL;
    }

    memset(tx, 0, sizeof(tx_t));

    tx->mgr = mgr;
    list_init_head(&tx->write_cache);

    mgr->onfly_tx_num++;

    return tx;
}

int commit_tx(tx_t *tx)
{
    list_head_t *pos, *n;
    
    // 1. 将write_cache中的内容写日志

    // 2. 将write_cache中的内容生效到checkpoint_cache中，以便后台刷盘
    list_for_each_safe(pos, n, &tx->write_cache)
    {
        
    }

    return 0;
}

void cancel_tx(tx_t *tx)
{
    return;
}


void tx_cache_exit_system(cache_mgr_t *mgr)
{
    if (mgr == NULL)
        return;

    if (mgr->bd_hnd)
    {
        os_disk_close(mgr->bd_hnd);
        mgr->bd_hnd = NULL;
    }
    
    OS_FREE(mgr);
}

uint32_t hash_cache_value(hashtab_t *h, void *key)
{
    uint64_t block_id = (uint64_t)key;

    return block_id % h->slot_num;
}

int hash_compare_key(hashtab_t *h, void *key1, void *key2)
{
    uint64_t block_id1 = (uint64_t)key1;
    uint64_t block_id2 = (uint64_t)key2;

    if (block_id1 > block_id2)
        return 1;
    else if (block_id1 == block_id2)
        return 0;
    else 
        return -1;
}

cache_mgr_t *tx_cache_init_system(char *bd_name, uint32_t bd_block_size)
{
    int ret = 0;
    
    cache_mgr_t *mgr = OS_MALLOC(sizeof(cache_mgr_t));
    if (mgr == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed.\n", sizeof(cache_mgr_t));
        return NULL;
    }

    memset(mgr, 0, sizeof(cache_mgr_t));
    
    ret = os_disk_create(&mgr->bd_hnd, bd_name);
    if (ret < 0)
    {
        tx_cache_exit_system(mgr);
        LOG_ERROR("open block device(%s) failed(%d).\n", bd_name, ret);
        return NULL;
    }

    mgr->hcache = hashtab_create(hash_cache_value, hash_compare_key, 1000, 10000);
    if (mgr->hcache == NULL)
    {
        tx_cache_exit_system(mgr);
        LOG_ERROR("create device(%s) hash table failed.\n", bd_name);
        return NULL;
    }

    return mgr;

}

