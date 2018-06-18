

#include "os_adapter.h"
#include "globals.h"
#include "utils.h"


MODULE(PID_CACHE);
#include "log.h"

#include "tx_cache.h"

// 申请cache block
cache_block_t *alloc_cache_block(uint32_t block_size)
{
    int cb_size = sizeof(cache_block_t) + block_size;
    cache_block_t *cb = OS_MALLOC(cb_size);
    if (cb == NULL)
    {
        LOG_ERROR("alloc memory(%d) failed\n", cb_size);
        return NULL;
    }

    memset(cb, 0, sizeof(cache_block_t));
    cb->state = EMPTY;

    return cb;
}

// 释放cache block
void free_cache_block(cache_block_t *cb)
{
    if (cb == NULL)
        return;
    
    if (cb->ref_cnt != 0)
    {
        LOG_ERROR("cache block(%llu) ref_cnt(%d)\n", cb->block_id, cb->ref_cnt);
    }
    
    if (cb->state == DIRTY)
    {
        LOG_ERROR("cache block(%llu) is DIRTY(%d)\n", cb->block_id, cb->state);
    }
    
    LOG_EVENT("destroy cache(%p) block(%llu)\n", cb, cb->block_id);

    OS_FREE(cb);
}

// commit cache block状态为EMPTY的两种场景
// 1. 此cache是刚申请的
// 2. 刚pingpong过的cache
int32_t fill_commit_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];

    if (commit_cb == NULL)
    {
        return SUCCESS;
    }

    if (commit_cb->state != EMPTY)
    {
        return SUCCESS;
    }

    cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
    
    if (rw_cb->state == CLEAN) // 优先以读写buffer中的干净数据为准
    {
        memcpy(commit_cb->buf, rw_cb->buf, mgr->block_size);
    }
    else if ((flush_cb != NULL) && (flush_cb->state != EMPTY)) // 其次以flush buffer中的数据为准
    {
        memcpy(commit_cb->buf, flush_cb->buf, mgr->block_size);
    }
    else
    {
        ASSERT(0);
    }
        
    commit_cb->state = CLEAN;

    return SUCCESS;
}

// alloc commit cache block
cache_block_t *alloc_commit_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    uint8_t commit_side = mgr->commit_side;
    cache_block_t *commit_cb = NULL;
    int32_t ret;

    if (rw_cb->pp_cb[commit_side] == NULL)
    {
        commit_cb = alloc_cache_block(mgr->block_size);
        if (commit_cb == NULL)
        {
            LOG_ERROR("alloc write cache(%llu) failed\n", rw_cb->block_id);
            return NULL;
        }

        commit_cb->block_id = rw_cb->block_id;
        rw_cb->pp_cb[commit_side] = commit_cb;
    }
    else
    {
        commit_cb = rw_cb->pp_cb[mgr->commit_side];
    }

    // 填充rw cache block内容
    ret = fill_commit_cb(mgr, rw_cb);
    if (ret < SUCCESS)
    {
        LOG_ERROR("fill commit cache(%d) block(%lld) failed(%d)\n", mgr->block_size, rw_cb->block_id, ret);
        return NULL;
    }

    return commit_cb;
}

int32_t fill_rw_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    int32_t ret;
    ASSERT(rw_cb != NULL);
    
    if (rw_cb->state != EMPTY)
    {
        return SUCCESS;
    }

    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];
    cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 1];
    
    if ((commit_cb != NULL) && (commit_cb->state != EMPTY)) // 优先以commit buffer中的数据为准
    {
        memcpy(rw_cb->buf, commit_cb->buf, mgr->block_size);
    }
    else if ((flush_cb != NULL) && (flush_cb->state != EMPTY)) // 其次以flush buffer中的数据为准
    {
        memcpy(rw_cb->buf, flush_cb->buf, mgr->block_size);
    }
    else // 最后以盘上数据为准
    {
        ret = mgr->bd_ops->read(mgr->bd_hnd, rw_cb->buf, mgr->block_size, rw_cb->block_id);
        if (ret <= 0)
        {
            LOG_ERROR("read block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
                mgr->bd_hnd, mgr->block_size, rw_cb->block_id, ret);
            return -ERR_TX_READ_DISK;
        }
    }

    rw_cb->state = CLEAN;
    return SUCCESS;
}

// 获取指定类型的cache block
cache_block_t *alloc_cb(cache_mgr_t *mgr, u64_t block_id, BUF_USAGE_E usage)
{
    int ret;

    ASSERT(usage < BUF_USAGE_NUM);

    // hash表中只记录rw cache block
    cache_block_t *rw_cb = hashtab_search(mgr->hcache, (void *)block_id);
    if (rw_cb == NULL)
    { // 申请rw cache block
        rw_cb = alloc_cache_block(mgr->block_size);
        if (rw_cb == NULL)
        {
            LOG_ERROR("alloc rw cache block(%lld) failed\n", block_id);
            return NULL;
        }

        rw_cb->block_id = block_id;
        ret = hashtab_insert(mgr->hcache, (void *)block_id, rw_cb);
        if (ret < 0)
        {
            OS_FREE(rw_cb);
            LOG_ERROR("hashtab_insert failed. block_id(%lld) ret(%d)\n", block_id, ret);
            return NULL;
        }

        // 第一次就以写的方式申请，说明不用管盘上的数据
        if (usage == FOR_WRITE)
        {
            memset(rw_cb->buf, 0, mgr->block_size);
            rw_cb->state = CLEAN; // 直接认为读cache上的数据就是有效的，全0，不用读盘
        }
    }

    // 填充rw cache block内容
    ret = fill_rw_cb(mgr, rw_cb);
    if (ret < SUCCESS)
    {
        LOG_ERROR("fill rw cache(%d) block(%lld) failed(%d)\n", mgr->block_size, block_id, ret);
        return NULL;
    }

    if (usage == FOR_READ)
    {
        rw_cb->usage = FOR_READ;
        return rw_cb;
    }
    
    ASSERT(usage == FOR_WRITE);
    cache_block_t *commit_cb = alloc_commit_cb(mgr, rw_cb);  // 确保commit cb申请成功，才能写
    if (commit_cb == NULL)
    {
        LOG_ERROR("get commit cache block failed. size(%d) block_id(%lld)\n", mgr->block_size, block_id);
        return NULL;
    }

    rw_cb->usage = FOR_WRITE;
    return rw_cb;
}

// 获取指定类型的cache block
cache_block_t *get_cb(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    ASSERT(buf_type < BUF_TYPE_NUM);

    // hash表中只记录rw cache block
    cache_block_t *rw_cb = hashtab_search(mgr->hcache, (void *)block_id);
    if (rw_cb == NULL)
    {
        return NULL;
    }

    if (buf_type == RW_BUF)
    {
        return rw_cb;
    }
        
    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];
    if (buf_type == COMMIT_BUF)
    {
        return commit_cb;
    }

    cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 1];
    ASSERT(buf_type == FLUSH_BUF);
    return flush_cb;
}

// 必须和put_buffer、commit_buffer、cancel_buffer配合使用
void *get_buffer_by_type(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    cache_block_t *cache = get_cb(mgr, block_id, buf_type);
    if (cache == NULL)
        return NULL;

    cache->ref_cnt++;
    return cache->buf;
}

// 必须和put_buffer、commit_buffer、cancel_buffer配合使用
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_USAGE_E usage)
{
    cache_block_t *cache = alloc_cb(mgr, block_id, usage);
    if (cache == NULL)
        return NULL;

    cache->ref_cnt++;
    return cache->buf;
}

// 必须和get_buffer配合使用
void put_buffer(cache_mgr_t *mgr, void *buf)
{
    cache_block_t *cache = list_entry(buf, cache_block_t, buf);

    ASSERT(cache->ref_cnt != 0);
    cache->ref_cnt--;

    return;
}

// 标记buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *rw_buf)
{
    cache_block_t *rw_cb = list_entry(rw_buf, cache_block_t, buf);
    ASSERT(rw_cb->state != EMPTY);
    if (rw_cb->state == CLEAN)
    {
        rw_cb->state = DIRTY;
    }
}

// 记录统计数据
void update_mgr_info(cache_mgr_t *mgr)
{
    if (mgr->modified_block_num == 0)
    { // 记录第一个块修改的时间
        mgr->first_modified_time = os_get_ms_count();
    }

    mgr->modified_block_num++;
    mgr->modified_data_bytes += mgr->block_size;
}

// commit cache block
void commit_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    if (rw_cb->state != DIRTY)
    {
        return;
    }

    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];
    ASSERT(commit_cb != NULL);
    
    memcpy(commit_cb->buf, rw_cb->buf, mgr->block_size);
    commit_cb->state = DIRTY; // commit cb中的内容待pingpong后变成flush cb下盘
    rw_cb->state = CLEAN;     // 
    update_mgr_info(mgr);    
}

// commit buffer
void commit_buffer(cache_mgr_t *mgr, void *rw_buf)
{
    cache_block_t *rw_cb = list_entry(rw_buf, cache_block_t, buf);
    commit_cb(mgr, rw_cb);
}

// cancel cache block
void cancel_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    if (rw_cb->state == DIRTY)
    { // 将脏数据直接废弃
        rw_cb->state = EMPTY;
        //fill_rw_cb(mgr, rw_cb);
    }
}

// cancel buffer
void cancel_buffer(cache_mgr_t *mgr, void *rw_buf)
{
    cache_block_t *rw_cb = list_entry(rw_buf, cache_block_t, buf);
    cancel_cb(mgr, rw_cb);
}

// 将指定flush cache block的内容写到盘上
int flush_cache_block(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    int ret;
    cache_block_t *flush_cb;

    flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
    if (flush_cb == NULL)
    { // 说明这个位置没有要下盘的数据
        return SUCCESS;
    }

    // 说明这块数据未被修改过
    if (flush_cb->state != DIRTY)
    {
        return SUCCESS;
    }

    // 还未写盘就变成0是不对的
    ASSERT(mgr->flush_block_num != 0);

    // 数据下盘
    ret = mgr->bd_ops->write(mgr->bd_hnd, flush_cb->buf, mgr->block_size, flush_cb->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, flush_cb->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    mgr->flush_block_num--;
    flush_cb->state = CLEAN;

    return SUCCESS;
}

// 将指定flush cache block的内容写到盘上，方便hash表遍历执行
int flush_one_cb(void *cache, void *mgr)
{
    return flush_cache_block(mgr, cache);
}

// 将当前mgr中所有的flush cache block的内容下盘
int flush_all_cb(cache_mgr_t *mgr)
{
    return hashtab_walkall(mgr->hcache, flush_one_cb, mgr);
}

// 后台任务，所有checkpoint/flush buffer list中的脏数据下盘
int flush_disk(cache_mgr_t *mgr)
{
    int ret;

    // 将当前所有的flush cache中的内容下盘
    ret = flush_all_cb(mgr);
    if (ret < 0)
    {
        LOG_ERROR("flush_all_cache failed(%d)\n", ret);
        return ret;
    }

    ASSERT(mgr->flush_block_num == 0);

    // 将对应的日志失效

    mgr->flush_sn++;

    return SUCCESS;
}

// 
int pingpong_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    cache_block_t *flush_cb;
    cache_block_t *commit_cb;

    commit_cb = rw_cb->pp_cb[mgr->commit_side];
    flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
    if ((commit_cb == NULL) || (flush_cb == NULL))
    { 
        return SUCCESS;
    }

    //
    if ((commit_cb->state == DIRTY) && (flush_cb->state == CLEAN))
    {
        flush_cb->state = EMPTY;
    }

    return SUCCESS;
}

// 
int pingpong_one_cb(void *cache, void *mgr)
{
    return pingpong_cb(mgr, cache);
}

// 
int pingpong_all_cb(cache_mgr_t *mgr)
{
    return hashtab_walkall(mgr->hcache, pingpong_one_cb, mgr);
}

// 切换cache，也就是将commit cache和checkpoint/flush cache交换
void pingpong_cache(cache_mgr_t *mgr)
{
    ASSERT(mgr->onfly_tx_num == 0);    // commit cache这边事务在进行
    ASSERT(mgr->flush_block_num == 0); // flush cache这边全部下盘完成

    pingpong_all_cb(mgr);

    // 切换cache
    mgr->commit_side = (mgr->commit_side + 1) & 0x1;
    mgr->flush_block_num = mgr->modified_block_num;

    // 清除切换cache计数
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->allow_new_tx = TRUE; // 允许申请新事务

    return;
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
    list_init_head(&tx->rw_cb);
    tx->tx_id = mgr->cur_tx_id++;

    mgr->onfly_tx_num++;

    *new_tx = tx;

    return SUCCESS;
}

// 带事务修改时，调用这个接口
void *tx_get_buffer(tx_t *tx, u64_t block_id)
{
    cache_block_t *rw_cb = alloc_cb(tx->mgr, block_id, FOR_WRITE);
    if (rw_cb == NULL)
        return NULL;

    if (rw_cb->ref_cnt == 0)
    { // 第一次使用，这个write buffer还不属于任何tx
        rw_cb->owner_tx_id = tx->tx_id;
    }
    else if (rw_cb->owner_tx_id != tx->tx_id)
    { // 不允许多个事务修改同一个buffer
        LOG_ERROR("tx(%llu) get tx cache conflict(%llu).\n", tx->tx_id, rw_cb->owner_tx_id);
        return NULL;
    }
    else
    { // 同一个tx多次获取tx buffer
        // do nothing
    }

    rw_cb->ref_cnt++;
    
    return rw_cb->buf;
}

// 标记tx buffer dirty
void tx_mark_buffer_dirty(tx_t *tx, void *tx_buf)
{
    cache_block_t *rw_cb = list_entry(tx_buf, cache_block_t, buf);
    ASSERT(rw_cb->ref_cnt != 0);
    ASSERT(rw_cb->state != EMPTY);
    
    if (rw_cb->state == CLEAN)
    {
        rw_cb->state = DIRTY;
        list_add_tail(&tx->rw_cb, &rw_cb->node);
    }

    return;
}

// 带事务修改时，调用这个接口
void tx_put_buffer(tx_t *tx, void *tx_buf)
{
    cache_block_t *rw_cb = list_entry(tx_buf, cache_block_t, buf);

    ASSERT(rw_cb->ref_cnt != 0);
    if (--rw_cb->ref_cnt != 0)
    { // 说明tx还在引用
        return;
    }

    return;
}

// 将rw cb的内容刷到commit cb上
void commit_all_cb(tx_t *tx)
{
    list_head_t *pos;

    while ((pos = list_pop_first(&tx->rw_cb)) != NULL)
    {
        cache_block_t *rw_cb = list_entry(pos, cache_block_t, node);
        ASSERT(rw_cb->ref_cnt == 0);
        commit_cb(tx->mgr, rw_cb);
        rw_cb->ref_cnt = 0; // 允许其他事务引用
    }
}

// 放弃rw cb的内容
void cancel_all_cb(tx_t *tx)
{
    list_head_t *pos;

    while ((pos = list_pop_first(&tx->rw_cb)) != NULL)
    {
        cache_block_t *rw_cb = list_entry(pos, cache_block_t, node);
        ASSERT(rw_cb->ref_cnt != 0);
        cancel_cb(tx->mgr, rw_cb);
        rw_cb->ref_cnt = 0; // 允许其他事务引用
    }
}

// 提交修改的数据到日志，tx buf中的数据生效到commit buf
void tx_commit(tx_t *tx)
{

    // 1. 将rw cb中的内容写日志

    // 2. 将临时申请的cache中的内容刷到write_cache中
    commit_all_cb(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 3. 检查是否达到切换cache的条件
    pingpong_cache_if_possible(tx->mgr);

    return;
}

// 放弃这个事务的所有修改，
// 1. 申请写buf时和其他事务发生了冲突，需先cancel，等其他事务完成后，再继续进行
// 2. 写日志失败?  不允许这种场景发生
void tx_cancel(tx_t *tx)
{
    // 1. 放弃此tx的所有修改
    cancel_all_cb(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 2. 检查是否达到切换cache的条件
    pingpong_cache_if_possible(tx->mgr);

    return;
}

// 销毁cache，要将read cache、flush cache、write cache都要销毁
void destroy_cb(cache_block_t *rw_cb)
{
    if (rw_cb->pp_cb[0] != NULL)
    {
        free_cache_block(rw_cb->pp_cb[0]);
    }
    
    if (rw_cb->pp_cb[1] != NULL)
    {
        free_cache_block(rw_cb->pp_cb[1]);
    }
    
    free_cache_block(rw_cb);
}

// 销毁所有的cache
void destroy_all_caches(hashtab_t *h)
{
    cache_block_t *rw_cb;

    while ((rw_cb = hashtab_pop_first(h)) != NULL)
        destroy_cb(rw_cb);
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
    u64_t block_id2 = ((cache_block_t *)value)->block_id;

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

    mgr->hcache = hashtab_create(hash_key, compare_key, 1000, offsetof(cache_block_t, hnode));
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


#if 0  // 临时代码

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


