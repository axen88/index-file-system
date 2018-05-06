

#include "os_adapter.h"
#include "globals.h"
#include "utils.h"


MODULE(PID_CACHE);
#include "log.h"

#include "tx_cache.h"

// ����cache node
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

// �ͷ�cache node
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

    // �˴�cache״̬ΪEMPTY�����ֳ���
    // 1. ��cache�Ǹ������
    // 2. ��cache������Ϊ������ʧ�ܶ���������ȡ���� TODO  ���ﴦ����(��ȷ�����������������)
    if (cache->state == EMPTY)
    {
        uint8_t flush_side = (write_side + 1) & 0x1;
        cache_node_t *flush_cache = read_cache->side_cache[flush_side];
        if (flush_cache != NULL)
        { // ���ȴ�flush cache�п������ݣ���Ϊ��������ݸ���
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

    // �˴�cache״̬ΪEMPTY�����ֳ���
    // 1. ��cache�Ǹ������
    // 2. ��cache������Ϊ������ʧ�ܶ���������ȡ���� TODO  ���ﴦ����(��ȷ�����������������)
    if (cache->state == EMPTY)
    {
        ASSERT(read_cache->state == CLEAN);
        memcpy(cache->buf, read_cache->buf, mgr->block_size);
        cache->state = CLEAN;
    }

    return cache;
}

// ��ȡָ�����͵�cache node
cache_node_t *get_cache_node(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    int ret;

    ASSERT(buf_type < BUF_TYPE_NUM);

    // hash����ֻ��¼��cache
    cache_node_t *read_cache = hashtab_search(mgr->hcache, (void *)block_id);
    if (read_cache == NULL)
    { // �����cache
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

        // ��һ��������ľͲ��Ƕ�buf��˵�����ù����ϵ�����
        if (buf_type != FOR_READ)
        {
            memset(read_cache->buf, 0, mgr->block_size);
            read_cache->state = CLEAN; // ֱ����Ϊ��cache�ϵ����ݾ�����Ч�ģ�ȫ0
        }
    }

    // û�ж������ݣ����ϴζ�����ʧ��
    if (read_cache->state == EMPTY)
    {
        // ����, ��ʱread_cache�϶��Ѿ���¼��hash������
        ret = mgr->bd_ops->read(mgr->bd_hnd, read_cache->buf, mgr->block_size, read_cache->block_id);
        if (ret <= 0)
        {
            LOG_ERROR("read block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
                mgr->bd_hnd, mgr->block_size, read_cache->block_id, ret);
            return NULL;
        }

        read_cache->state = CLEAN;
    }

    ASSERT(read_cache->state == CLEAN); // ��cache��״̬������ΪDIRTY

    if (buf_type == FOR_READ)
        return read_cache;
    else if (buf_type == FOR_WRITE)
        return get_write_cache_node(mgr, read_cache);

    return get_flush_cache_node(mgr, read_cache);
}

// �����put_buffer�ɶ�ʹ��
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type)
{
    cache_node_t *cache = get_cache_node(mgr, block_id, buf_type);
    if (cache == NULL)
        return NULL;

    cache->ref_cnt++;

    return cache->buf;
}

// �����get_buffer�ɶ�ʹ��
void put_buffer(cache_mgr_t *mgr, void *buf)
{
    cache_node_t *cache = list_entry(buf, cache_node_t, buf);

    ASSERT(cache->ref_cnt != 0);

    cache->ref_cnt--;

    return;
}

// ���write cache dirty
void mark_write_cache_dirty(cache_mgr_t *mgr, cache_node_t *write_cache)
{
    ASSERT(write_cache->ref_cnt != 0);
    ASSERT(write_cache->read_cache != NULL);

    // ȷ��������һ����write buf
    ASSERT(write_cache->read_cache->side_cache[mgr->write_side] == write_cache);

    ASSERT(write_cache->state != EMPTY);

    if (write_cache->state != DIRTY)
    {
        write_cache->state = DIRTY;

        if (mgr->modified_block_num == 0)
        { // ��¼��һ�����޸ĵ�ʱ��
            mgr->first_modified_time = os_get_ms_count();
        }

        mgr->modified_block_num++;
        mgr->modified_data_bytes += mgr->block_size;
    }
}

// ���buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *write_buf)
{
    cache_node_t *write_cache = list_entry(write_buf, cache_node_t, buf);

    mark_write_cache_dirty(mgr, write_cache);

    return;
}

// ��ָ��flush cache������д������
int flush_cache(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    int ret;
    cache_node_t *flush_cache;

    flush_cache = read_cache->side_cache[(mgr->write_side + 1) & 0x1];
    if (flush_cache == NULL)
    { // ˵�����λ��û��Ҫ���̵�����
        return SUCCESS;
    }

    // ˵���������δ���޸Ĺ�
    if (flush_cache->state != DIRTY)
    {
        return SUCCESS;
    }

    // ��δд�̾ͱ��0�ǲ��Ե�
    ASSERT(mgr->flush_block_num != 0);

    // ��������
    ret = mgr->bd_ops->write(mgr->bd_hnd, flush_cache->buf, mgr->block_size, flush_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, flush_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    // �޸�read cache�е�����
    memcpy(read_cache->buf, flush_cache->buf, mgr->block_size);

    mgr->flush_block_num--;
    flush_cache->state = CLEAN;

    return SUCCESS;
}

// ��ָ��flush cache������д�����ϣ�����hash�����ִ��
int flush_one_cache(void *cache, void *mgr)
{
    return flush_cache(mgr, cache);
}

// ����ǰmgr�����е�flush cache����������
int flush_all_cache(cache_mgr_t *mgr)
{
    return hashtab_map(mgr->hcache, flush_one_cache, mgr);
    //return hashtab_map(mgr->hcache, flush_cache, mgr);
}

// ��̨�������е�����������
void *commit_disk(void *arg)
{
    cache_mgr_t *mgr = arg;
    int ret;

    // ����ǰ���е�flush cache�е���������
    ret = flush_all_cache(mgr);
    if (ret < 0)
    {
        LOG_ERROR("flush_all_cache failed(%d)\n", ret);
        return NULL;
    }

    ASSERT(mgr->flush_block_num == 0);

    // ����Ӧ����־ʧЧ

    mgr->flush_sn++;

    return NULL;
}

// �л�cache��Ҳ���ǽ�write cache��flush cache����
void pingpong_cache(cache_mgr_t *mgr)
{
    ASSERT(mgr->onfly_tx_num == 0);  // writing cache��������ڽ���
    ASSERT(mgr->flush_block_num == 0); // flush cache���ȫ���������

    // �л�cache
    mgr->write_side = (mgr->write_side + 1) & 0x1;
    mgr->flush_block_num = mgr->modified_block_num;

    // ����л�cache����
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->allow_new_tx = TRUE; // ��������������

    return;
}

// ��ȡtx cache node
tx_cache_node_t *get_tx_cache(tx_t *tx, cache_node_t *write_cache)
{
    tx_cache_node_t *tx_cache;
    
    ASSERT(write_cache->ref_cnt == 0); // ȷ�����write cache��û��tx��ʹ��
    
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

        // ��������֮��Ĺ���
        tx_cache->write_cache = write_cache;
        write_cache->tx_cache = tx_cache;
    }
    else
    {
        tx_cache = write_cache->tx_cache;
    }

    // �������ݣ�����רΪ�������ĸ���
    memcpy(tx_cache->buf, write_cache->buf, tx->mgr->block_size);
    tx_cache->state = CLEAN;

    // txռ��write cache
    write_cache->owner_tx_id = tx->tx_id;
    write_cache->ref_cnt++; // ֻռ��һ��
    
    list_add_tail(&tx->write_cache, &tx_cache->node);
    
    return tx_cache;
}

// �ͷ�tx cache node
void free_tx_cache(tx_cache_node_t *tx_cache)
{
    if (tx_cache == NULL)
    {
        return;
    }
    
    cache_node_t *write_cache = tx_cache->write_cache;

    // ȡ������֮��Ĺ���
    write_cache->tx_cache = NULL;
    tx_cache->write_cache = NULL;

    OS_FREE(tx_cache);
}

// �ͷŶ�write cache��ռ��
void put_tx_cache(tx_cache_node_t *tx_cache)
{
    cache_node_t *write_cache = tx_cache->write_cache;

    ASSERT(write_cache->ref_cnt == 1);  // һ�������ù�һ��
    write_cache->ref_cnt--;
    
    //list_del(&tx_cache->node);
    //free_tx_cache(tx_cache);  // pingpong��ʱ����ͳһ�ͷ�
    //write_cache->tx_cache = NULL;
}

// ����һ���µ�����
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

// �������޸�ʱ����������ӿ�
void *tx_get_buffer(tx_t *tx, u64_t block_id)
{
    tx_cache_node_t *tx_cache;
    cache_node_t *write_cache = get_cache_node(tx->mgr, block_id, FOR_WRITE);
    if (write_cache == NULL)
        return NULL;

    // ��һ��ʹ�ã����write cache���������κ�tx
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
    { // �������������޸�ͬһ��buffer
        LOG_ERROR("tx(%llu) get tx cache conflict(%llu).\n", tx->tx_id, write_cache->owner_tx_id);
        return NULL;
    }
    else
    { // ͬһ��tx��λ�ȡtx cache
        tx_cache = write_cache->tx_cache;
        ASSERT(tx_cache != NULL);
    }

    tx_cache->ref_cnt++;
    
    return tx_cache->buf; // ������ʱ�����������޸�ʹ��
}

// ���tx buffer dirty
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

// �������޸�ʱ����������ӿ�
void tx_put_buffer(tx_t *tx, void *tx_buf)
{
    tx_cache_node_t *tx_cache = list_entry(tx_buf, tx_cache_node_t, buf);

    ASSERT(tx_cache->ref_cnt != 0);
    ASSERT(tx_cache->write_cache != NULL);

    if (--tx_cache->ref_cnt != 0)
    { // ˵��tx��������
        return;
    }

    return;
}

// ����ʱ�����tx_write_cache�е�����ˢ��write_cache��
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
            // tx cache��������Ч��write cache��
            memcpy(write_cache->buf, tx_cache->buf, tx->mgr->block_size);
            mark_write_cache_dirty(tx->mgr, write_cache); // ��ʱ�������ͷ�write cache������

            tx_cache->state = CLEAN; // 
        }

        put_tx_cache(tx_cache);
    }
}

// ������ʱ�����tx_write_cache�е�����
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
            // ����tx cache������
            tx_cache->state = CLEAN; // 
        }

        put_tx_cache(tx_cache);
    }
}


// ����Ƿ�ﵽ�л�cache������������ﵽ���л�
void pingpong_cache_if_possible(cache_mgr_t *mgr)
{
    // ����Ƿ�ﵽ�л�cache������
    if ((mgr->modified_block_num < mgr->max_modified_blocks)
        && (mgr->modified_data_bytes < mgr->max_modified_bytes))
    {
        // û���޸Ĺ����ݣ�û�б�Ҫ�л�
        if (mgr->modified_block_num == 0)
        {
            return;
        }

        // ���ʱ���Ƿ񳬹�
        u64_t t = os_get_ms_count() - mgr->first_modified_time;
        if (t < mgr->max_time_interval)
        {
            return;
        }
    }


    // ˵��flush��û��������ɣ���ʱ�������л�cache
    if (mgr->flush_block_num != 0)
    {
        return;
    }

    // �������ڽ��е�����Ҳ�����л�cache
    if (mgr->onfly_tx_num != 0)
    {
        // ˵���û��������ܴ���ֹ����������
        mgr->allow_new_tx = FALSE;
        return;
    }

    // �л�cache
    pingpong_cache(mgr);

    return;
}

// �ύ�޸ĵ����ݵ���־��tx buf�е�������Ч��write buf
int tx_commit(tx_t *tx)
{

    // 1. ��write_cache�е�����д��־

    // 2. ����ʱ�����cache�е�����ˢ��write_cache��
    commit_tx_cache(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 3. ����Ƿ�ﵽ�л�cache������
    pingpong_cache_if_possible(tx->mgr);

    return 0;
}

// �����������������޸ģ�
// 1. ����дbufʱ�������������˳�ͻ������cancel��������������ɺ��ټ�������
// 2. д��־ʧ��?  ���������ֳ�������
void tx_cancel(tx_t *tx)
{
    // 1. ������tx�������޸�
    cancel_tx_cache(tx);

    ASSERT(tx->mgr->onfly_tx_num > 0);
    tx->mgr->onfly_tx_num--;

    // 2. ����Ƿ�ﵽ�л�cache������
    pingpong_cache_if_possible(tx->mgr);

    return;
}

// ����cache��Ҫ��read cache��flush cache��write cache��Ҫ����
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

// �������е�cache
void destroy_all_caches(hashtab_t *h)
{
    cache_node_t *read_cache;

    while ((read_cache = hashtab_pop_first(h)) != NULL)
        destroy_cache(read_cache);
}

// �˳�cacheϵͳ
void tx_cache_exit_system(cache_mgr_t *mgr)
{
    if (mgr == NULL)
        return;

    // ��ֹ����������
    mgr->allow_new_tx = FALSE;

    // �ȴ����е�������ɣ��ȴ����е��޸��������
    while ((mgr->onfly_tx_num) || (mgr->modified_block_num) || (mgr->flush_block_num))
    {
        LOG_ERROR("onfly_tx_num(%d), modified_block_num(%d), flush_block_num(%d)\n",
            mgr->onfly_tx_num, mgr->modified_block_num, mgr->flush_block_num);
        OS_SLEEP_SECOND(1);
    }

    // �������е�cache
    destroy_all_caches(mgr->hcache);

    // �رտ��豸
    if (mgr->bd_hnd)
    {
        mgr->bd_ops->close(mgr->bd_hnd);
        mgr->bd_hnd = NULL;
    }

    // �ͷŹ����ڴ�
    OS_FREE(mgr);
}

// hash���key hash����
uint32_t hash_key(hashtab_t *h, void *key)
{
    u64_t block_id = (u64_t)key;

    return block_id % h->slots_num;
}

// hash���key�ȽϺ���
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

// ��ʼ��cacheϵͳ
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

    mgr->cur_tx_id = 1; // tx id��1��ʼ
    
    mgr->allow_new_tx = TRUE; // ��������������

    return mgr;

}


#if 1  // ��ʱ����

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


