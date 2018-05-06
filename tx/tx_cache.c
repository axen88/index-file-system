

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

    memset(cache, 0, sizeof(cache_node_t));
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
        uint8_t flush_side = (write_side + 1) & 0x1;
        cache_node_t *flush_cache = read_cache->side_cache[flush_side];
        if (flush_cache != NULL)
        { // 优先从flush cache中拷贝数据，因为这里的数据更新
            memcpy(cache->buf, flush_cache->buf, mgr->block_size);
        }
        else
        {
            memcpy(cache->buf, read_cache->buf, mgr->block_size);
        }

        cache->state = CLEAN;
    }

    return cache;
}

// get flush cache node
cache_node_t *get_flush_cache_node(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    uint8_t flush_side = (mgr->write_side + 1) & 0x1;
    cache_node_t *cache = NULL;

    ASSERT(read_cache != NULL);

    if (read_cache->side_cache[flush_side] == NULL)
    {
        cache = alloc_cache_node(mgr->block_size);
        if (cache == NULL)
        {
            LOG_ERROR("alloc flush cache(%llu) failed\n", read_cache->block_id);
            return NULL;
        }

        cache->block_id = read_cache->block_id;
        cache->read_cache = read_cache;

        read_cache->side_cache[flush_side] = cache;
    }
    else
    {
        cache = read_cache->side_cache[flush_side];
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
            memset(read_cache->buf, 0, mgr->block_size);
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

    return get_flush_cache_node(mgr, read_cache);
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

// 标记write cache dirty
void mark_write_cache_dirty(cache_mgr_t *mgr, cache_node_t *write_cache)
{
    ASSERT(write_cache->ref_cnt != 0);
    ASSERT(write_cache->read_cache != NULL);

    // 确保操作的一定是write buf
    ASSERT(write_cache->read_cache->side_cache[mgr->write_side] == write_cache);

    ASSERT(write_cache->state != EMPTY);

    if (write_cache->state != DIRTY)
    {
        write_cache->state = DIRTY;

        if (mgr->modified_block_num == 0)
        { // 记录第一个块修改的时间
            mgr->first_modified_time = os_get_ms_count();
        }

        mgr->modified_block_num++;
        mgr->modified_data_bytes += mgr->block_size;
    }
}

// 标记buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *write_buf)
{
    cache_node_t *write_cache = list_entry(write_buf, cache_node_t, buf);

    mark_write_cache_dirty(mgr, write_cache);

    return;
}

// 将指定flush cache的内容写到盘上
int flush_cache(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    int ret;
    cache_node_t *flush_cache;

    flush_cache = read_cache->side_cache[(mgr->write_side + 1) & 0x1];
    if (flush_cache == NULL)
    { // 说明这个位置没有要下盘的数据
        return SUCCESS;
    }

    // 说明这块数据未被修改过
    if (flush_cache->state != DIRTY)
    {
        return SUCCESS;
    }

    // 还未写盘就变成0是不对的
    ASSERT(mgr->flush_block_num != 0);

    // 数据下盘
    ret = mgr->bd_ops->write(mgr->bd_hnd, flush_cache->buf, mgr->block_size, flush_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, flush_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    // 修改read cache中的内容
    memcpy(read_cache->buf, flush_cache->buf, mgr->block_size);

    mgr->flush_block_num--;
    flush_cache->state = CLEAN;

    return SUCCESS;
}

// 将指定flush cache的内容写到盘上，方便hash表遍历执行
int flush_one_cache(void *cache, void *mgr)
{
    return flush_cache(mgr, cache);
}

// 将当前mgr中所有的flush cache的内容下盘
int flush_all_cache(cache_mgr_t *mgr)
{
    return hashtab_map(mgr->hcache, flush_one_cache, mgr);
    //return hashtab_map(mgr->hcache, flush_cache, mgr);
}

// 后台任务，所有的脏数据下盘
void *commit_disk(void *arg)
{
    cache_mgr_t *mgr = arg;
    int ret;

    // 将当前所有的flush cache中的内容下盘
    ret = flush_all_cache(mgr);
    if (ret < 0)
    {
        LOG_ERROR("flush_all_cache failed(%d)\n", ret);
        return NULL;
    }

    ASSERT(mgr->flush_block_num == 0);

    // 将对应的日志失效

    mgr->flush_sn++;

    return NULL;
}

// 切换cache，也就是将write cache和flush cache交换
void pingpong_cache(cache_mgr_t *mgr)
{
    ASSERT(mgr->onfly_tx_num == 0);  // writing cache这边事务在进行
    ASSERT(mgr->flush_block_num == 0); // flush cache这边全部下盘完成

    // 切换cache
    mgr->write_side = (mgr->write_side + 1) & 0x1;
    mgr->flush_block_num = mgr->modified_block_num;

    // 清除切换cache计数
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->allow_new_tx = TRUE; // 允许申请新事务

    return;
}

// 获取tx cache node
tx_cache_node_t *get_tx_cache(tx_t *tx, cache_node_t *write_cache)
{
    tx_cache_node_t *tx_cache;
    
    ASSERT(write_cache->ref_cnt == 0); // 确保这个write cache还没有tx在使用
    
    if (write_cache->tx_cache == NULL)
    {
        int cache_size = sizeof(tx_cache_node_t) + tx->mgr->block_size;
        tx_cache = OS_MALLOC(cache_size);
        if (tx_cache == NULL)
        {
            LOG_ERROR("alloc memory(%d) failed\n", cache_size);
            return NULL;
        }
        
        memset(tx_cache, 0, sizeof(tx_cache_node_t));

        // 建立两者之间的关联
        tx_cache->write_cache = write_cache;
        write_cache->tx_cache = tx_cache;
    }
    else
    {
        tx_cache = write_cache->tx_cache;
    }

    // 拷贝内容，这是专为事务做的副本
    memcpy(tx_cache->buf, write_cache->buf, tx->mgr->block_size);
    tx_cache->state = CLEAN;

    // tx占用write cache
    write_cache->owner_tx_id = tx->tx_id;
    write_cache->ref_cnt++; // 只占用一次
    
    list_add_tail(&tx->write_cache, &tx_cache->node);
    
    return tx_cache;
}

// 释放tx cache node
void free_tx_cache(tx_cache_node_t *tx_cache)
{
    if (tx_cache == NULL)
    {
        return;
    }
    
    cache_node_t *write_cache = tx_cache->write_cache;

    // 取消两者之间的关联
    write_cache->tx_cache = NULL;
    tx_cache->write_cache = NULL;

    OS_FREE(tx_cache);
}

// 释放对write cache的占用
void put_tx_cache(tx_cache_node_t *tx_cache)
{
    cache_node_t *write_cache = tx_cache->write_cache;

    ASSERT(write_cache->ref_cnt == 1);  // 一定被引用过一次
    write_cache->ref_cnt--;
    
    //list_del(&tx_cache->node);
    //free_tx_cache(tx_cache);  // pingpong的时候，再统一释放
    //write_cache->tx_cache = NULL;
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

// 带事务修改时，调用这个接口
void *tx_get_buffer(tx_t *tx, u64_t block_id)
{
    tx_cache_node_t *tx_cache;
    cache_node_t *write_cache = get_cache_node(tx->mgr, block_id, FOR_WRITE);
    if (write_cache == NULL)
        return NULL;

    // 第一次使用，这个write cache还不属于任何tx
    if (write_cache->ref_cnt == 0)
    {
        tx_cache = get_tx_cache(tx, write_cache);
        if (tx_cache == NULL)
        {
            LOG_ERROR("get tx(%llu) cache failed(%d).\n", tx->tx_id, tx->mgr->block_size);
            return NULL;
        }
    }
    else if (write_cache->owner_tx_id != tx->tx_id)
    { // 不允许多个事务修改同一个buffer
        LOG_ERROR("tx(%llu) get tx cache conflict(%llu).\n", tx->tx_id, write_cache->owner_tx_id);
        return NULL;
    }
    else
    { // 同一个tx多次获取tx cache
        tx_cache = write_cache->tx_cache;
        ASSERT(tx_cache != NULL);
    }

    tx_cache->ref_cnt++;
    
    return tx_cache->buf; // 返回临时副本给外面修改使用
}

// 标记tx buffer dirty
void tx_mark_buffer_dirty(tx_t *tx, void *tx_buf)
{
    tx_cache_node_t *tx_cache = list_entry(tx_buf, tx_cache_node_t, buf);

    ASSERT(tx_cache->ref_cnt != 0);
    ASSERT(tx_cache->write_cache != NULL);

    if (tx_cache->state != DIRTY)
    {
        tx_cache->state = DIRTY;
    }

    return;
}

// 带事务修改时，调用这个接口
void tx_put_buffer(tx_t *tx, void *tx_buf)
{
    tx_cache_node_t *tx_cache = list_entry(tx_buf, tx_cache_node_t, buf);

    ASSERT(tx_cache->ref_cnt != 0);
    ASSERT(tx_cache->write_cache != NULL);

    if (--tx_cache->ref_cnt != 0)
    { // 说明tx还在引用
        return;
    }

    return;
}

// 将临时申请的tx_write_cache中的内容刷到write_cache中
void commit_tx_cache(tx_t *tx)
{
    list_head_t *pos;

    while ((pos = list_pop_first(&tx->write_cache)) != NULL)
    {
        tx_cache_node_t *tx_cache = list_entry(pos, tx_cache_node_t, node);
        cache_node_t *write_cache = tx_cache->write_cache;

        ASSERT(write_cache->ref_cnt == 1);
        ASSERT(tx_cache->ref_cnt == 0);

        if (tx_cache->state == DIRTY)
        {
            // tx cache的内容生效到write cache中
            memcpy(write_cache->buf, tx_cache->buf, tx->mgr->block_size);
            mark_write_cache_dirty(tx->mgr, write_cache); // 此时还不能释放write cache的引用

            tx_cache->state = CLEAN; // 
        }

        put_tx_cache(tx_cache);
    }
}

// 放弃临时申请的tx_write_cache中的内容
void cancel_tx_cache(tx_t *tx)
{
    list_head_t *pos;

    while ((pos = list_pop_first(&tx->write_cache)) != NULL)
    {
        tx_cache_node_t *tx_cache = list_entry(pos, tx_cache_node_t, node);
        cache_node_t *write_cache = tx_cache->write_cache;

        ASSERT(write_cache->ref_cnt == 1);

        if (tx_cache->state == DIRTY)
        {
            // 放弃tx cache的内容
            tx_cache->state = CLEAN; // 
        }

        put_tx_cache(tx_cache);
    }
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


    // 说明flush还没有下盘完成，此时不可以切换cache
    if (mgr->flush_block_num != 0)
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

// 提交修改的数据到日志，tx buf中的数据生效到write buf
int tx_commit(tx_t *tx)
{

    // 1. 将write_cache中的内容写日志

    // 2. 将临时申请的cache中的内容刷到write_cache中
    commit_tx_cache(tx);

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
    // 1. 放弃此tx的所有修改
    cancel_tx_cache(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 2. 检查是否达到切换cache的条件
    pingpong_cache_if_possible(tx->mgr);

    return;
}

// 销毁cache，要将read cache、flush cache、write cache都要销毁
void destroy_cache(cache_node_t *read_cache)
{
    if (read_cache->side_cache[0] != NULL)
    {
        free_tx_cache(read_cache->side_cache[0]->tx_cache);
        free_cache_node(read_cache->side_cache[0]);
    }
    
    if (read_cache->side_cache[1] != NULL)
    {
        free_tx_cache(read_cache->side_cache[1]->tx_cache);
        free_cache_node(read_cache->side_cache[1]);
    }
    
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
    while ((mgr->onfly_tx_num) || (mgr->modified_block_num) || (mgr->flush_block_num))
    {
        LOG_ERROR("onfly_tx_num(%d), modified_block_num(%d), flush_block_num(%d)\n",
            mgr->onfly_tx_num, mgr->modified_block_num, mgr->flush_block_num);
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
int compare_key(hashtab_t *h, void *key, void *value)
{
    u64_t block_id1 = (u64_t)key;
    u64_t block_id2 = ((cache_node_t *)value)->block_id;

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

    mgr->hcache = hashtab_create(hash_key, compare_key, 1000, offsetof(cache_node_t, hnode));
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


