

#include "os_adapter.h"
#include "globals.h"
#include "disk_if.h"
#include "utils.h"


MODULE(PID_CACHE);
#include "log.h"

#include "tx_cache.h"

// 申请cache node
cache_node_t *alloc_cache_node(cache_mgr_t *mgr)
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
    list_init_head(&cache->node);
    cache->ref_cnt = 0;
    cache->owner_tx_id = 0;
    cache->state = EMPTY;

    return cache;
}

// 释放cache node
void free_cache_node(cache_mgr_t *mgr, cache_node_t *cache)
{
    ASSERT(cache->ref_cnt == 0);
    
    hashtab_delete(mgr->hcache, (void *)cache->block_id);
    OS_FREE(cache);
}

// buf_type
cache_node_t *get_nonread_cache_node(cache_mgr_t *mgr, cache_node_t *read_cache, BUF_TYPE_E buf_type)
{
    ASSERT(buf_type != FOR_READ);
    uint8_t checkpoint_side = (mgr->writing_side + 1) & 0x1;
    cache_node_t *cache = NULL;

    if (buf_type == FOR_WRITE)
    {
        if (read_cache->side_node[mgr->writing_side] != NULL)
        {
            cache = read_cache->side_node[mgr->writing_side];
            if (cache->state == EMPTY)
            { // 无数据，说明这个cache有做事务取消操作
                cache_node_t *checkpoint_cache = read_cache->side_node[checkpoint_side];
                if (checkpoint_cache != NULL)
                { // 从checkpoint cache中拷贝数据，因为这里的数据更新
                    memcpy(cache->dat, checkpoint_cache->dat, mgr->block_size); 
                }
                else
                {
                    memcpy(cache->dat, read_cache->dat, mgr->block_size);
                }

                cache->state = CLEAN;
            }

            return cache;
        }
        
    }
    else // CHECKPOINT_BUF
    {
        if (read_cache->side_node[checkpoint_side] != NULL)
        {
            cache = read_cache->side_node[checkpoint_side];
            if (cache->state == EMPTY)
            { // 无数据，说明这个cache有做事务取消操作
                memcpy(cache->dat, read_cache->dat, mgr->block_size); 
                cache->state = CLEAN;
            }
        }
        
        return cache;
    }

    cache = alloc_cache_node(mgr);
    if (cache == NULL)
    {
        LOG_ERROR("alloc_cache_node(%llu) failed\n", read_cache->block_id);
        return NULL;
    }

    cache->block_id = read_cache->block_id;
    cache->state = CLEAN;
    cache->read_node = read_cache;
        
    if (buf_type == FOR_WRITE)
    {
        cache_node_t *checkpoint_cache = read_cache->side_node[checkpoint_side];
        if (checkpoint_cache != NULL)
        { // 从checkpoint cache中拷贝数据，因为这里的数据更新
            memcpy(cache->dat, checkpoint_cache->dat, mgr->block_size); 
        }
        else
        {
            memcpy(cache->dat, read_cache->dat, mgr->block_size);
        }
        
        read_cache->side_node[mgr->writing_side] = cache;
    }
    else // CHECKPOINT_BUF
    {
        // 从读cache中拷贝数据
        memcpy(cache->dat, read_cache->dat, mgr->block_size); 
        read_cache->side_node[checkpoint_side] = cache;
    }

    return cache;
}

// 一定是读cache来读盘
int read_disk(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    int ret;

    // 读盘, 此时read_cache已经记录在hash表中了
    ret = os_disk_pread(mgr->bd_hnd, read_cache->dat, mgr->block_size, read_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("read block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, read_cache->block_id, ret);
        return -ERR_FILE_READ;
    }

    read_cache->state = CLEAN;

    return 0;
}

// 获取指定类型的cache node
cache_node_t *get_cache_node(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    int ret;
    
    cache_node_t *read_cache = hashtab_search(mgr->hcache, (void *)block_id);
    if (read_cache != NULL) // hash表中只记录读cache的node
    {
        if (read_cache->state == EMPTY) // 没有读过数据，或上次读数据失败
        { // 重新读盘
            ret = read_disk(mgr, read_cache);
            if (ret < 0)
            {
                LOG_ERROR("read disk failed(%d).\n", ret);
                return NULL;
            }
        }

        ASSERT(read_cache->state == CLEAN); // 读cache的状态不可能为DIRTY
        
        if (buf_type == FOR_READ)
            return read_cache;

        return get_nonread_cache_node(mgr, read_cache, buf_type);
    }

    // 这里申请的cache是读cache
    read_cache = alloc_cache_node(mgr);
    if (read_cache == NULL)
    {
        LOG_ERROR("alloc cache block(%lld) buf failed\n", block_id);
        return NULL;
    }

    read_cache->block_id = block_id;
    
    ret = hashtab_insert(mgr->hcache, (void *)block_id, read_cache);
    if (ret < 0)
    {
        OS_FREE(read_cache);
        LOG_ERROR("hashtab_insert failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, block_id, ret);
        return NULL;
    }

    // 第一次想申请的就不是读buf，说明不用管盘上的数据
    if (buf_type != FOR_READ)
    {
        read_cache->state = CLEAN; // 直接认为读cache上的数据就是有效的，全0
        return get_nonread_cache_node(mgr, read_cache, buf_type);
    }

    // 读盘, 此时read_cache已经记录在hash表中了
    ret = read_disk(mgr, read_cache);
    if (ret < 0)
    {
        LOG_ERROR("read disk failed(%d).\n", ret);
        return NULL;
    }

    return read_cache;
}

void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
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

    ASSERT(cache->ref_cnt != 0);

    cache->ref_cnt--;
    LOG_INFO("put block(%lld) buf(0x%p) success\n", cache->block_id, buf);

    return SUCCESS;
}

// 将checkpoint_cache中的内容写到盘上
int commit_checkpoint_cache(cache_mgr_t *mgr, cache_node_t *cache)
{
    int ret;
    cache_node_t *checkpoint_cache;
        
    checkpoint_cache = cache->side_node[(mgr->writing_side + 1) & 0x1];
    if (checkpoint_cache == NULL)
    { // 说明这个位置没有要下盘的数据
        return 0;
    }

    // 说明这块数据未被修改过
    if (checkpoint_cache->state != DIRTY)
    {
        return 0;
    }

    // 还未写盘就变成0是不对的
    ASSERT(mgr->checkpoint_block_num != 0);

    // 数据下盘
    ret = os_disk_pwrite(mgr->bd_hnd, checkpoint_cache->dat, mgr->block_size, checkpoint_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, checkpoint_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    // 修改read cache中的内容
    memcpy(cache->dat, checkpoint_cache->dat, mgr->block_size);

    mgr->checkpoint_block_num--;
    checkpoint_cache->state = CLEAN;
    
    return 0;
}

int commit_one_checkpoint_cache(void *k, void *d, void *arg)
{
    cache_node_t *cache = d;

    ASSERT(cache->block_id == (u64_t)k);

    return commit_checkpoint_cache(arg, cache);
}

// 将当前所有的checkpoint cache中的内容下盘
int commit_all_checkpoint_cache(cache_mgr_t *mgr)
{
    return hashtab_map(mgr->hcache, commit_one_checkpoint_cache, mgr);
}

// 
void *commit_disk(void *arg)
{
    cache_mgr_t *mgr = arg;
    int ret;
    
    // 将当前所有的checkpoint cache中的内容下盘
    ret = commit_all_checkpoint_cache(mgr);
    if (ret < 0)
    {
        LOG_ERROR("commit_all_checkpoint_cache failed(%d)\n", ret);
        return NULL;
    }

    ASSERT(mgr->checkpoint_block_num == 0);

    // 将对应的日志失效


    mgr->checkpoint_sn++;

    return NULL;
}

// 开始一个新的事务
int tx_new(cache_mgr_t *mgr, tx_t **new_tx)
{
    if (mgr->block_new_tx)
    {
        LOG_ERROR("new tx is blocked.\n");
        return -ERR_NEW_TX_BLOCKED;
    }
    
    tx_t *tx = OS_MALLOC(sizeof(tx_t));
    if (tx == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed.\n", sizeof(tx_t));
        return -ERR_NO_MEMORY;
    }

    memset(tx, 0, sizeof(tx_t));

    tx->mgr = mgr;
    list_init_head(&tx->write_cache);
    tx->tx_id = mgr->cur_tx_id++;

    mgr->onfly_tx_num++;

    *new_tx = tx;

    return 0;
}

// 带事务修改时，调用这个接口
void *tx_get_write_buffer(tx_t *tx, u64_t block_id)
{
    cache_node_t *cache = get_cache_node(tx->mgr, block_id, FOR_WRITE);
    if (cache == NULL)
        return NULL;

    // 这个buffer还没有事务拥有所有权
    if (cache->owner_tx_id == 0)
    {
        cache->owner_tx_id = tx->tx_id;
    }

    // 不允许多个事务修改同一个buffer
    if (cache->owner_tx_id != tx->tx_id)
    {
        LOG_ERROR("tx(%llu) get write buffer conflict(%llu).\n", tx->tx_id, cache->owner_tx_id);
        return NULL;
    }

    // 第一次使用
    if (cache->ref_cnt == 0)
    {
        list_add_tail(&tx->write_cache, &cache->node);
        
        if (tx->mgr->modified_block_num == 0)
        { // 记录第一个块修改的时间
            tx->mgr->first_modified_time = os_get_ms_count();
        }
        
        tx->mgr->modified_block_num++;
        tx->mgr->modified_data_bytes += tx->mgr->block_size;
    }

    cache->state = DIRTY;
    cache->ref_cnt++;
    
    return cache->dat;
}

// 释放对write cache的占用
void tx_put_write_cache(cache_node_t *write_cache)
{
    // 已经被释放过了，说明程序有问题
    ASSERT(write_cache->ref_cnt != 0);

    write_cache->ref_cnt--;
    if (write_cache->ref_cnt == 0)
    {
        //list_del(&write_cache->node);
    }

    return;
}


// 释放对write cache的占用
void put_write_cache(cache_node_t *write_cache)
{
    // 已经被释放过了，说明程序有问题
    ASSERT(write_cache->ref_cnt != 0);

    write_cache->ref_cnt = 0;
    list_del(&write_cache->node);

    return;
}


// 将临时申请的cache中的内容刷到write_cache中
int commit_write_cache(tx_t *tx)
{
    list_head_t *pos, *n;
    
    list_for_each_safe(pos, n, &tx->write_cache)
    {
        cache_node_t *write_cache = list_entry(pos, cache_node_t, node);
        put_write_cache(write_cache);
    }

    return 0;

}

// 将write buffer中的内容废弃
int cancel_write_cache(tx_t *tx)
{
    list_head_t *pos, *n;
    
    list_for_each_safe(pos, n, &tx->write_cache)
    {
        cache_node_t *write_cache = list_entry(pos, cache_node_t, node);
        put_write_cache(write_cache);
        write_cache->state = EMPTY;
    }

    return 0;

}

// 检查是否达到切换cache的条件，如果达到就切
void pingpong_cache_if_possible(cache_mgr_t *mgr)
{
    // 未达到切换cache的条件
    if ((mgr->modified_block_num < mgr->max_modified_blocks)
        && (mgr->modified_data_bytes < mgr->max_modified_bytes))
    {
        // 没有修改过数据
        if (mgr->modified_block_num == 0)
        {
            return;
        }
        
        // 检查时间是否超过
        u64_t t = os_get_ms_count() - mgr->first_modified_time;
        if (t < mgr->max_time_interval)
        {
            return;
        }
    }


    // 说明checkpoint还没有下盘完成，此时不可以切换cache
    if (mgr->checkpoint_block_num != 0)
    {
        return;
    }

    // 还有正在进行的事务，也不能切换cache
    if (mgr->onfly_tx_num != 0)
    {
        // 说明用户事务量很大，阻止新事务生成
        mgr->block_new_tx = TRUE;
        return;
    }

    // 切换cache
    mgr->writing_side = (mgr->writing_side + 1) & 0x1;
    mgr->checkpoint_block_num = mgr->modified_block_num;

    // 清除切换cache计数
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;
    
    mgr->block_new_tx = FALSE; // 解除新事务生成

    return;
}

// 提交修改的数据到日志，此时写cache中的数据还未下盘
int tx_commit(tx_t *tx)
{
    
    // 1. 将write_cache中的内容写日志

    // 2. 将临时申请的cache中的内容刷到write_cache中，暂时未实现
    commit_write_cache(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 3. 检查是否达到切换cache的条件
    pingpong_cache_if_possible(tx->mgr);

    return 0;
}

void tx_cancel(tx_t *tx)
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
    u64_t block_id = (u64_t)key;

    return block_id % h->slot_num;
}

int hash_compare_key(hashtab_t *h, void *key1, void *key2)
{
    u64_t block_id1 = (u64_t)key1;
    u64_t block_id2 = (u64_t)key2;

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

    mgr->block_size = bd_block_size;

    mgr->max_modified_bytes = (1*1024*1024); // 1MB
    mgr->max_time_interval = 1000; // 1000 ms
    mgr->max_modified_blocks = 1;

    mgr->block_new_tx = FALSE;

    mgr->cur_tx_id = 1; // tx id从1开始

    return mgr;

}

