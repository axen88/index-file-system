

#include "os_adapter.h"
#include "globals.h"
#include "utils.h"


MODULE(PID_CACHE);
#include "log.h"

#include "tx_cache.h"

// 申请cache node
cache_node_t *alloc_cache_node(uint32_t block_size)
{
    int cache_size = sizeof(cache_node_t) + block_size;
    cache_node_t *cache = OS_MALLOC(cache_size);
    if (cache == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed\n", cache_size);
        return NULL;
    }

    memset(cache, 0, cache_size);
    list_init_head(&cache->node);
    cache->owner_tx_id = 0;
    cache->state = EMPTY;

    return cache;
}

// 释放cache node
void free_cache_node(cache_node_t *cache)
{
    if (cache == NULL)
        return;
    
    if (cache->ref_cnt != 0)
    {
        LOG_ERROR("cache block(%llu) ref_cnt(%d)\n", cache->block_id, cache->ref_cnt);
    }
    
    if (cache->state == DIRTY)
    {
        LOG_ERROR("cache block(%llu) is DIRTY(%d)\n", cache->block_id, cache->state);
    }
    
    LOG_EVENT("destroy cache(%p) block(%llu)\n", cache, cache->block_id);

    OS_FREE(cache);
}

// get write cache node
cache_node_t *get_write_cache_node(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    uint8_t write_side = mgr->write_side;
    cache_node_t *cache = NULL;

    if (read_cache->side_cache[write_side] == NULL)
    {
        cache = alloc_cache_node(mgr->block_size);
        if (cache == NULL)
        {
            LOG_ERROR("alloc write cache(%llu) failed\n", read_cache->block_id);
            return NULL;
        }

        cache->block_id = read_cache->block_id;
        cache->read_cache = read_cache;

        read_cache->side_cache[write_side] = cache;
    }
    else
    {
        cache = read_cache->side_cache[mgr->write_side];
    }

    // 此处cache状态为EMPTY的两种场景
    // 1. 此cache是刚申请的
    // 2. 此cache曾经因为做事务失败而导致事务被取消， TODO  这里处理不对(正确处理是消除这种情况)
    if (cache->state == EMPTY)
    {
        uint8_t checkpoint_side = (write_side + 1) & 0x1;
        cache_node_t *checkpoint_cache = read_cache->side_cache[checkpoint_side];
        if (checkpoint_cache != NULL)
        { // 优先从checkpoint cache中拷贝数据，因为这里的数据更新
            memcpy(cache->buf, checkpoint_cache->buf, mgr->block_size);
        }
        else
        {
            memcpy(cache->buf, read_cache->buf, mgr->block_size);
        }

        cache->state = CLEAN;
    }

    return cache;
}

// get checkpoint cache node
cache_node_t *get_checkpoint_cache_node(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    uint8_t checkpoint_side = (mgr->write_side + 1) & 0x1;
    cache_node_t *cache = NULL;

    ASSERT(read_cache != NULL);

    if (read_cache->side_cache[checkpoint_side] == NULL)
    {
        cache = alloc_cache_node(mgr->block_size);
        if (cache == NULL)
        {
            LOG_ERROR("alloc checkpoint cache(%llu) failed\n", read_cache->block_id);
            return NULL;
        }

        cache->block_id = read_cache->block_id;
        cache->read_cache = read_cache;

        read_cache->side_cache[checkpoint_side] = cache;
    }
    else
    {
        cache = read_cache->side_cache[checkpoint_side];
    }

    // 此处cache状态为EMPTY的两种场景
    // 1. 此cache是刚申请的
    // 2. 此cache曾经因为做事务失败而导致事务被取消， TODO  这里处理不对(正确处理是消除这种情况)
    if (cache->state == EMPTY)
    {
        ASSERT(read_cache->state == CLEAN);
        memcpy(cache->buf, read_cache->buf, mgr->block_size);
        cache->state = CLEAN;
    }

    return cache;
}

// 获取指定类型的cache node
cache_node_t *get_cache_node(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    int ret;

    ASSERT(buf_type < BUF_TYPE_NUM);

    // hash表中只记录读cache
    cache_node_t *read_cache = hashtab_search(mgr->hcache, (void *)block_id);
    if (read_cache == NULL)
    { // 申请读cache
        read_cache = alloc_cache_node(mgr->block_size);
        if (read_cache == NULL)
        {
            LOG_ERROR("alloc read cache node block(%lld) failed\n", block_id);
            return NULL;
        }

        read_cache->block_id = block_id;
        ret = hashtab_insert(mgr->hcache, (void *)block_id, read_cache);
        if (ret < 0)
        {
            OS_FREE(read_cache);
            LOG_ERROR("hashtab_insert failed. block_id(%lld) ret(%d)\n", block_id, ret);
            return NULL;
        }

        // 第一次想申请的就不是读buf，说明不用管盘上的数据
        if (buf_type != FOR_READ)
        {
            read_cache->state = CLEAN; // 直接认为读cache上的数据就是有效的，全0
        }
    }

    // 没有读过数据，或上次读数据失败
    if (read_cache->state == EMPTY)
    {
        // 读盘, 此时read_cache肯定已经记录在hash表中了
        ret = mgr->bd_ops->read(mgr->bd_hnd, read_cache->buf, mgr->block_size, read_cache->block_id);
        if (ret <= 0)
        {
            LOG_ERROR("read block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
                mgr->bd_hnd, mgr->block_size, read_cache->block_id, ret);
            return NULL;
        }

        read_cache->state = CLEAN;
    }

    ASSERT(read_cache->state == CLEAN); // 读cache的状态不可能为DIRTY

    if (buf_type == FOR_READ)
        return read_cache;
    else if (buf_type == FOR_WRITE)
        return get_write_cache_node(mgr, read_cache);

    return get_checkpoint_cache_node(mgr, read_cache);
}

// 必须和put_buffer成对使用
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    cache_node_t *cache = get_cache_node(mgr, block_id, buf_type);
    if (cache == NULL)
        return NULL;

    cache->ref_cnt++;

    return cache->buf;
}

// 必须和get_buffer成对使用
void put_buffer(cache_mgr_t *mgr, void *buf)
{
    cache_node_t *cache = list_entry(buf, cache_node_t, buf);

    ASSERT(cache->ref_cnt != 0);

    cache->ref_cnt--;

    return;
}

// 标记buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *write_buf)
{
    cache_node_t *write_cache = list_entry(write_buf, cache_node_t, buf);

    ASSERT(write_cache->ref_cnt != 0);
    ASSERT(write_cache->read_cache != NULL);

    // 确保操作一定是write buf
    ASSERT(write_cache->read_cache->side_cache[mgr->write_side] == write_cache);

    if (write_cache->state != DIRTY)
    {
        write_cache->state = DIRTY;
        mgr->modified_block_num++;
    }

    return;
}

// 将指定checkpoint cache的内容写到盘上
int commit_checkpoint_cache(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    int ret;
    cache_node_t *checkpoint_cache;

    checkpoint_cache = read_cache->side_cache[(mgr->write_side + 1) & 0x1];
    if (checkpoint_cache == NULL)
    { // 说明这个位置没有要下盘的数据
        return SUCCESS;
    }

    // 说明这块数据未被修改过
    if (checkpoint_cache->state != DIRTY)
    {
        return SUCCESS;
    }

    // 还未写盘就变成0是不对的
    ASSERT(mgr->checkpoint_block_num != 0);

    // 数据下盘
    ret = mgr->bd_ops->write(mgr->bd_hnd, checkpoint_cache->buf, mgr->block_size, checkpoint_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, checkpoint_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    // 修改read cache中的内容
    memcpy(read_cache->buf, checkpoint_cache->buf, mgr->block_size);

    mgr->checkpoint_block_num--;
    checkpoint_cache->state = CLEAN;

    return SUCCESS;
}

// 将指定checkpoint cache的内容写到盘上，方便hash表遍历执行
int commit_one_checkpoint_cache(void *key, void *buf, void *arg)
{
    cache_node_t *read_cache = buf;

    ASSERT(read_cache->block_id == (u64_t)key);

    return commit_checkpoint_cache(arg, read_cache);
}

// 将当前mgr中所有的checkpoint cache的内容下盘
int commit_all_checkpoint_cache(cache_mgr_t *mgr)
{
    return hashtab_map(mgr->hcache, commit_one_checkpoint_cache, mgr);
}

// 后台任务，所有的脏数据下盘
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

// 切换cache，也就是将write cache和checkpoint cache交换
void pingpong_cache(cache_mgr_t *mgr)
{
    ASSERT(mgr->onfly_tx_num == 0);  // writing cache这边事务在进行
    ASSERT(mgr->checkpoint_block_num == 0); // checkpoint cache这边全部下盘完成

    // 切换cache
    mgr->write_side = (mgr->write_side + 1) & 0x1;
    mgr->checkpoint_block_num = mgr->modified_block_num;

    // 清除切换cache计数
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->allow_new_tx = TRUE; // 允许申请新事务

    return;
}

// 分配一个新的事务
int tx_alloc(cache_mgr_t *mgr, tx_t **new_tx)
{
    if (!mgr->allow_new_tx)
    {
        LOG_ERROR("new tx is not allowed.\n");
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

    return SUCCESS;
}

// 带事务修改时，调用这个接口，调用后不用put，在commit事务时，会自动put
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

    return cache->buf;
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

// 检查是否达到切换cache的条件，如果达到就切换
void pingpong_cache_if_possible(cache_mgr_t *mgr)
{
    // 检查是否达到切换cache的条件
    if ((mgr->modified_block_num < mgr->max_modified_blocks)
        && (mgr->modified_data_bytes < mgr->max_modified_bytes))
    {
        // 没有修改过数据，没有必要切换
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
        mgr->allow_new_tx = FALSE;
        return;
    }

    // 切换cache
    pingpong_cache(mgr);

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

// 放弃这个事务的所有修改，
// 1. 申请写buf时和其他事务发生了冲突，需先cancel，等其他事务完成后，再继续进行
// 2. 写日志失败?  不允许这种场景发生
void tx_cancel(tx_t *tx)
{
    return;
}

// 销毁cache，要将read cache、checkpoint cache、write cache都要销毁
void destroy_cache(cache_node_t *read_cache)
{
    free_cache_node(read_cache->side_cache[0]);
    free_cache_node(read_cache->side_cache[1]);
    free_cache_node(read_cache);
}

// 销毁所有的cache
void destroy_all_caches(hashtab_t *h)
{
    cache_node_t *read_cache;

    while ((read_cache = hashtab_pop_first(h)) != NULL)
        destroy_cache(read_cache);
}

// 退出cache系统
void tx_cache_exit_system(cache_mgr_t *mgr)
{
    if (mgr == NULL)
        return;

    // 禁止生成新事务
    mgr->allow_new_tx = FALSE;

    // 等待所有的事务完成，等待所有的修改下盘完成
    while ((mgr->onfly_tx_num) || (mgr->modified_block_num) || (mgr->checkpoint_block_num))
    {
        LOG_ERROR("onfly_tx_num(%d), modified_block_num(%d), checkpoint_block_num(%d)\n",
            mgr->onfly_tx_num, mgr->modified_block_num, mgr->checkpoint_block_num);
        OS_SLEEP_SECOND(1);
    }

    // 销毁所有的cache
    destroy_all_caches(mgr->hcache);

    // 关闭块设备
    if (mgr->bd_hnd)
    {
        mgr->bd_ops->close(mgr->bd_hnd);
        mgr->bd_hnd = NULL;
    }

    // 释放管理内存
    OS_FREE(mgr);
}

// hash表的key hash函数
uint32_t hash_key(hashtab_t *h, void *key)
{
    u64_t block_id = (u64_t)key;

    return block_id % h->slots_num;
}

// hash表的key比较函数
int compare_key(hashtab_t *h, void *key1, void *key2)
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

// 初始化cache系统
cache_mgr_t *tx_cache_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops)
{
    int ret = 0;

    cache_mgr_t *mgr = OS_MALLOC(sizeof(cache_mgr_t));
    if (mgr == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed.\n", sizeof(cache_mgr_t));
        return NULL;
    }

    memset(mgr, 0, sizeof(cache_mgr_t));

    mgr->bd_ops = bd_ops;

    ret = bd_ops->open(&mgr->bd_hnd, bd_name);
    if (ret < 0)
    {
        tx_cache_exit_system(mgr);
        LOG_ERROR("open block device(%s) failed(%d).\n", bd_name, ret);
        return NULL;
    }

    mgr->hcache = hashtab_create(hash_key, compare_key, 1000, 10000);
    if (mgr->hcache == NULL)
    {
        tx_cache_exit_system(mgr);
        LOG_ERROR("create device(%s) hash table failed.\n", bd_name);
        return NULL;
    }

    mgr->block_size = block_size;

    mgr->max_modified_bytes = (1*1024*1024); // 1MB
    mgr->max_time_interval = 1000; // 1000 ms
    mgr->max_modified_blocks = 1;

    mgr->cur_tx_id = 1; // tx id从1开始
    
    mgr->allow_new_tx = TRUE; // 允许申请新事务

    return mgr;

}


#if 1  // 临时代码

#include "disk_if.h"

int bd_open(void **hnd, char *bd_name)
{
    return os_disk_open(hnd, bd_name);
}

int bd_read(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pread(hnd, buf, size, offset);
}

int bd_write(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pwrite(hnd, buf, size, offset);
}

void bd_close(void *hnd)
{
    os_disk_close(hnd);
}

space_ops_t bd_ops
= {
    "bd_ops",
        
    bd_open,
    bd_read,
    bd_write,
    bd_close
};

#endif


