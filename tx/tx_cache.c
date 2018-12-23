

#include "os_adapter.h"
#include "globals.h"
#include "utils.h"
#include "log.h"
#include "tx_cache.h"
#include "tx_journal.h"

MODULE(MID_CACHE);


// 申请cache block
cache_block_t *alloc_cache_block(uint32_t block_size, u64_t block_id)
{
    int cb_size = sizeof(cache_block_t) + block_size;
    cache_block_t *cb = OS_MALLOC(cb_size);
    if (cb == NULL)
    {
        LOG_ERROR("alloc memory(%u) for block(%llu) failed\n", cb_size, block_id);
        return NULL;
    }

    memset(cb, 0, sizeof(cache_block_t));
    cb->block_id = block_id;
    cb->state = EMPTY;

    return cb;
}

// 释放cache block
void free_cache_block(cache_block_t *cb)
{
    if (cb == NULL)
    {
        return;
    }
    
    if (cb->ref_cnt != 0)
    {
        LOG_ERROR("cache block(%llu) ref_cnt(%u)\n", cb->block_id, cb->ref_cnt);
    }
    
    if (cb->state == DIRTY)
    {
        LOG_ERROR("cache block(%llu) is DIRTY(%u)\n", cb->block_id, cb->state);
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
    
    ASSERT(commit_cb != NULL);

    if (commit_cb->state != EMPTY)
    {
        return SUCCESS;
    }
    
    if (rw_cb->state == CLEAN) // 优先以读写buffer中的干净数据为准
    {
        memcpy(commit_cb->buf, rw_cb->buf, mgr->block_size);
    }
    else
    {
        cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
        if ((flush_cb != NULL) && (flush_cb->state != EMPTY)) // 其次以flush buffer中的数据为准
        {
            memcpy(commit_cb->buf, flush_cb->buf, mgr->block_size);
        }
        else
        {
            ASSERT(0);
        }
    }
        
    commit_cb->state = CLEAN;

    return SUCCESS;
}

// get commit cache block
cache_block_t *get_commit_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    cache_block_t *commit_cb = NULL;

    if (rw_cb->pp_cb[mgr->commit_side] == NULL)
    {
        commit_cb = alloc_cache_block(mgr->block_size, rw_cb->block_id);
        if (commit_cb == NULL)
        {
            LOG_ERROR("alloc commit cache(%llu) failed\n", rw_cb->block_id);
            return NULL;
        }

        rw_cb->pp_cb[mgr->commit_side] = commit_cb;
    }
    else
    {
        commit_cb = rw_cb->pp_cb[mgr->commit_side];
    }

    // 填充commit cache block内容
    int32_t ret = fill_commit_cb(mgr, rw_cb);
    if (ret < SUCCESS)
    {
        LOG_ERROR("fill commit cache(%d) block(%lld) failed(%d)\n", mgr->block_size, rw_cb->block_id, ret);
        return NULL;
    }

    return commit_cb;
}

// 填充rw cache block的内容，其来源有commit cb、flush cb或硬盘
int32_t fill_rw_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
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
        int32_t ret = mgr->bd_ops->read(mgr->bd_hnd, rw_cb->buf, mgr->block_size, rw_cb->block_id);
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

// hash表中记录的是rw cb，所以释放rw cb时，需要将flush cache、commit cache都销毁
void free_rw_cb(cache_block_t *rw_cb)
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

// 申请rw cache block
cache_block_t *get_rw_cb(cache_mgr_t *mgr, u64_t block_id, uint32_t flag)
{
    // hash表中只记录rw cache block
    cache_block_t *rw_cb = hashtab_search(mgr->hcache, (void *)block_id);
    if (rw_cb == NULL)
    { // 申请rw cache block
        rw_cb = alloc_cache_block(mgr->block_size, block_id);
        if (rw_cb == NULL)
        {
            LOG_ERROR("alloc rw cache block(%lld) failed\n", block_id);
            return NULL;
        }

        int32_t ret = hashtab_insert(mgr->hcache, (void *)block_id, rw_cb);
        if (ret < 0)
        {
            free_cache_block(rw_cb);
            LOG_ERROR("hashtab_insert failed. block_id(%lld) ret(%d)\n", block_id, ret);
            return NULL;
        }

        // 不用管盘上的数据
        if (flag & F_NO_READ)
        {
            memset(rw_cb->buf, 0, mgr->block_size);
            rw_cb->state = CLEAN; // 直接认为读cache上的数据就是有效的，全0，不用读盘
        }
    }

    // 填充rw cache block内容
    int32_t ret = fill_rw_cb(mgr, rw_cb);
    if (ret < SUCCESS)
    {
        LOG_ERROR("fill rw cache(%d) block(%lld) failed(%d)\n", mgr->block_size, block_id, ret);
        return NULL;
    }

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
        
    if (buf_type == COMMIT_BUF)
    {
        return rw_cb->pp_cb[mgr->commit_side];
    }

    return rw_cb->pp_cb[(mgr->commit_side + 1) & 1];
}

// 仅供内部使用
void *get_buffer_by_type(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    cache_block_t *cb = get_cb(mgr, block_id, buf_type);
    if (cb == NULL)
    {
        return NULL;
    }

    cb->ref_cnt++;
    return cb->buf;
}

// 必须和put_buffer配合使用
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, uint32_t flag)
{
    cache_block_t *cb = get_rw_cb(mgr, block_id, flag);
    if (cb == NULL)
    {
        return NULL;
    }

    cb->ref_cnt++;
    return cb->buf;
}

// 必须和get_buffer配合使用
void put_buffer(cache_mgr_t *mgr, void *buf)
{
    cache_block_t *cb = list_entry(buf, cache_block_t, buf);

    ASSERT(cb->ref_cnt != 0);
    cb->ref_cnt--;

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
int32_t commit_cb(cache_mgr_t *mgr, cache_block_t *rw_cb)
{
    if (rw_cb->state != DIRTY)
    { // 没有脏数据，就不用commit
        return SUCCESS;
    }

    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];
    if (commit_cb == NULL)
    {
        commit_cb = alloc_cache_block(mgr->block_size, rw_cb->block_id);
        if (commit_cb == NULL)
        {
            LOG_ERROR("alloc commit cache(%llu) failed\n", rw_cb->block_id);
            return -ERR_NO_MEMORY;
        }

        rw_cb->pp_cb[mgr->commit_side] = commit_cb;
    }

    if (commit_cb->state == DIRTY)
    { // 已经是dirty，不允许
        LOG_ERROR("commit cache(%llu) status is DIRTY already\n", rw_cb->block_id);
        return -ERR_CACHE_STATE;
    }

    // 提交内容
    memcpy(commit_cb->buf, rw_cb->buf, mgr->block_size);
    commit_cb->state = DIRTY; // commit cb中的内容待pingpong后变成flush cb下盘
    rw_cb->state = CLEAN;     // rw cb中内容提交完成
    update_mgr_info(mgr);    

    return SUCCESS;
}

// commit buffer
int32_t commit_buffer(cache_mgr_t *mgr, void *rw_buf)
{
    cache_block_t *rw_cb = list_entry(rw_buf, cache_block_t, buf);
    return commit_cb(mgr, rw_cb);
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

// 将指定flush cache block的内容写到盘上，方便hash表遍历执行
int flush_one_cb(void *_rw_cb, void *_mgr)
{
    cache_mgr_t *mgr = _mgr;
    cache_block_t *rw_cb = _rw_cb;
        
    cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
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
    int32_t ret = mgr->bd_ops->write(mgr->bd_hnd, flush_cb->buf, mgr->block_size, flush_cb->block_id);
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

// 将当前mgr中所有的flush cache block的内容下盘
int flush_all_cb(cache_mgr_t *mgr)
{
    return hashtab_walkall(mgr->hcache, flush_one_cb, mgr);
}

// 将所有flush buffer中的脏数据下盘
int flush_all_cache_block(cache_mgr_t *mgr)
{
    // 将当前所有的flush cache中的内容下盘
    int ret = flush_all_cb(mgr);
    if (ret < 0)
    {
        LOG_ERROR("flush failed(%d)\n", ret);
        return ret;
    }

    ASSERT(mgr->flush_block_num == 0);

    ///TODO: 将对应的日志失效

    mgr->flush_sn++;

    return SUCCESS;
}

// 设置commit cb和flush cb中的内容状态
int checkpoint_one_cb(void *_rw_cb, void *_mgr)
{
    cache_block_t *rw_cb = _rw_cb;
    cache_mgr_t *mgr = _mgr;
    
    cache_block_t *commit_cb = rw_cb->pp_cb[mgr->commit_side];
    cache_block_t *flush_cb = rw_cb->pp_cb[(mgr->commit_side + 1) & 0x1];
    if ((commit_cb == NULL) || (flush_cb == NULL))
    { 
        return SUCCESS;
    }

    ASSERT(flush_cb->state != DIRTY);

    if ((commit_cb->state == DIRTY) && (flush_cb->state == CLEAN))
    { // 说明commit cb中的内容比flush cb中的内容更新，所以checkpoint时，需失效flush cb中的内容
        flush_cb->state = EMPTY;
    }

    return SUCCESS;
}

// 设置commit cb和flush cb中的内容状态
int checkpoint_all_cb(cache_mgr_t *mgr)
{
    return hashtab_walkall(mgr->hcache, checkpoint_one_cb, mgr);
}

// 将commit cache和flush cache中的内容交换
void checkpoint_all_cache_block(cache_mgr_t *mgr)
{
    if (mgr->onfly_commit_tx != 0)
    { // commit cache这边事务在进行
        return;
    }
    
    if (mgr->flush_block_num != 0)
    { // flush cache这边全部下盘完成
        return;
    }

    // 设置commit cb和flush cb中的内容状态
    int ret = checkpoint_all_cb(mgr);
    ASSERT(ret == SUCCESS);

    // 切换cache
    mgr->commit_side = (mgr->commit_side + 1) & 0x1;
    mgr->flush_block_num = mgr->modified_block_num;

    // 清除切换cache计数
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->checkpoint_sn++;

    mgr->allow_commit_tx = TRUE; // 允许提交事务

    return;
}

// 检查是否达到checkpoint的条件，如果达到checkpoint
void checkpoint_cache_if_possible(cache_mgr_t *mgr)
{
    // 检查是否达到checkpoint的条件
    if ((mgr->modified_block_num < mgr->max_modified_blocks)
        && (mgr->modified_data_bytes < mgr->max_modified_bytes))
    {
        // 没有修改过数据，没有必要checkpoint
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

        // 超时时，需要checkpoint
    }

    // 说明flush还没有下盘完成，此时不可以checkpoint
    if (mgr->flush_block_num != 0)
    {
        return;
    }

    // 还有正在进行的事务，也不能checkpoint
    if (mgr->onfly_commit_tx != 0)
    {
        // 说明用户事务量很大，阻止新事务提交
        mgr->allow_commit_tx = FALSE;
        return;
    }

    // checkpoint
    checkpoint_all_cache_block(mgr);

    return;
}

// 创建一个新的事务
int tx_create(cache_mgr_t *mgr, uint32_t mode, tx_t **new_tx)
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
    tx->mode = mode;
    mgr->onfly_tx_num++;

    *new_tx = tx;

    return SUCCESS;
}

// 带事务修改时，调用这个接口
void *tx_get_buffer(tx_t *tx, u64_t block_id, uint32_t flag)
{
    cache_block_t *rw_cb = get_rw_cb(tx->mgr, block_id, flag);
    if (rw_cb == NULL)
    {
        return NULL;
    }

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
    { // 同一个tx可以多次获取tx buffer
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
int commit_tx_cb(tx_t *tx)
{
    list_head_t *pos;

    list_for_each(pos, &tx->rw_cb)
    {
        cache_block_t *rw_cb = list_entry(pos, cache_block_t, node);
        ASSERT(rw_cb->ref_cnt == 0);
        int ret = commit_cb(tx->mgr, rw_cb);
        if (ret < 0)
        {
            LOG_ERROR("commit tx(%llu) block_id(%llu) failed(%d).\n", tx->tx_id, rw_cb->block_id, ret);
            return ret;
        }
    }

    return SUCCESS;
}

// 允许tx cb可以被别的tx使用
void release_tx_cb(tx_t *tx)
{
    list_head_t *pos;

    while ((pos = list_pop_first(&tx->rw_cb)) != NULL)
    {
        cache_block_t *rw_cb = list_entry(pos, cache_block_t, node);
        rw_cb->ref_cnt = 0; // 允许其他事务引用
    }
}

// 提交修改的数据到日志，tx buf中的数据生效到commit buf
int tx_commit(tx_t *tx)
{
    // 标记有事务正在提交
    tx->mgr->onfly_commit_tx++;

    // 1. 将rw cb的内容刷到commit cb中
    int ret = commit_tx_cb(tx);
    if (ret < 0)
    {
        tx->mgr->onfly_commit_tx--;
        LOG_ERROR("commit tx(%llu) failed(%d).\n", tx->tx_id, ret);
        return ret;
    }

    // 2. 将rw cb中的内容写日志
    if (!(tx->mode & M_NO_JOURNAL))
    {
        ret = tx_write_journal(tx);
        if (ret < 0)
        {
            tx->mgr->onfly_commit_tx--;
            LOG_ERROR("commit tx(%llu) write log failed(%d).\n", tx->tx_id, ret);
            return ret;
        }
    }

    // 3. 将tx修改链表中的cache block释放
    release_tx_cb(tx);

    ASSERT(tx->mgr->onfly_commit_tx > 0);
    tx->mgr->onfly_commit_tx--;

    // 3. 检查是否达到切换cache的条件
    checkpoint_cache_if_possible(tx->mgr);

    tx->mgr->onfly_tx_num--;
    OS_FREE(tx);
    return SUCCESS;
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

// 放弃这个事务的所有修改
void tx_cancel(tx_t *tx)
{
    // 放弃此tx的所有修改
    cancel_all_cb(tx);

    tx->mgr->onfly_tx_num--;
    OS_FREE(tx);
    return;
}

// 释放所有的cache
void free_all_caches(hashtab_t *h)
{
    cache_block_t *rw_cb;

    while ((rw_cb = hashtab_pop_first(h)) != NULL)
    {
        free_rw_cb(rw_cb);
    }
}

// 销毁cache管理结构
void destroy_cache_mgr(cache_mgr_t *mgr)
{
    if (mgr == NULL)
    {
        return;
    }

    // 不允许提交事务
    mgr->allow_commit_tx = FALSE;
    
    // 不允许申请新事务
    mgr->allow_new_tx = FALSE;

    // 等待所有的事务完成，等待所有的修改下盘完成
    while ((mgr->onfly_commit_tx) || (mgr->modified_block_num) || (mgr->flush_block_num) || (mgr->onfly_tx_num))
    {
        LOG_ERROR("onfly_commit_tx(%d), modified_block_num(%d), flush_block_num(%d), onfly_tx_num(%d)\n",
            mgr->onfly_commit_tx, mgr->modified_block_num, mgr->flush_block_num, mgr->onfly_tx_num);
        
        flush_all_cache_block(mgr);
        
        checkpoint_all_cache_block(mgr);
        
        OS_SLEEP_SECOND(1);
    }

    // 释放所有的cache
    free_all_caches(mgr->hcache);

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
    {
        return 1;
    }
    else if (block_id1 == block_id2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

// 创建cache管理结构
cache_mgr_t *init_cache_mgr(char *bd_name, uint32_t block_size, space_ops_t *bd_ops)
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
        destroy_cache_mgr(mgr);
        LOG_ERROR("open block device(%s) failed(%d).\n", bd_name, ret);
        return NULL;
    }

    mgr->hcache = hashtab_create(hash_key, compare_key, 1000, offsetof(cache_block_t, hnode));
    if (mgr->hcache == NULL)
    {
        destroy_cache_mgr(mgr);
        LOG_ERROR("create device(%s) hash table failed.\n", bd_name);
        return NULL;
    }

    mgr->block_size = block_size;

    mgr->max_modified_bytes = (1*1024*1024); // 1MB
    mgr->max_time_interval = 1000; // 1000 ms
    mgr->max_modified_blocks = 1;

    mgr->cur_tx_id = 1; // tx id从1开始
    
    mgr->allow_commit_tx = TRUE; // 允许提交事务
    mgr->allow_new_tx = TRUE; // 允许提交事务

    return mgr;

}

