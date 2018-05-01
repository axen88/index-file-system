

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

    memset(cache, 0, cache_size);
    list_init_head(&cache->node);
    cache->owner_tx_id = 0;
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
        uint8_t checkpoint_side = (write_side + 1) & 0x1;
        cache_node_t *checkpoint_cache = read_cache->side_cache[checkpoint_side];
        if (checkpoint_cache != NULL)
        { // ���ȴ�checkpoint cache�п������ݣ���Ϊ��������ݸ���
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

    return get_checkpoint_cache_node(mgr, read_cache);
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

// ���buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *write_buf)
{
    cache_node_t *write_cache = list_entry(write_buf, cache_node_t, buf);

    ASSERT(write_cache->ref_cnt != 0);
    ASSERT(write_cache->read_cache != NULL);

    // ȷ������һ����write buf
    ASSERT(write_cache->read_cache->side_cache[mgr->write_side] == write_cache);

    if (write_cache->state != DIRTY)
    {
        write_cache->state = DIRTY;
        mgr->modified_block_num++;
    }

    return;
}

// ��ָ��checkpoint cache������д������
int commit_checkpoint_cache(cache_mgr_t *mgr, cache_node_t *read_cache)
{
    int ret;
    cache_node_t *checkpoint_cache;

    checkpoint_cache = read_cache->side_cache[(mgr->write_side + 1) & 0x1];
    if (checkpoint_cache == NULL)
    { // ˵�����λ��û��Ҫ���̵�����
        return SUCCESS;
    }

    // ˵���������δ���޸Ĺ�
    if (checkpoint_cache->state != DIRTY)
    {
        return SUCCESS;
    }

    // ��δд�̾ͱ��0�ǲ��Ե�
    ASSERT(mgr->checkpoint_block_num != 0);

    // ��������
    ret = mgr->bd_ops->write(mgr->bd_hnd, checkpoint_cache->buf, mgr->block_size, checkpoint_cache->block_id);
    if (ret <= 0)
    {
        LOG_ERROR("write block failed. bd_hnd(%p) size(%d) block_id(%lld) ret(%d)\n",
            mgr->bd_hnd, mgr->block_size, checkpoint_cache->block_id, ret);
        return -ERR_FILE_WRITE;
    }

    // �޸�read cache�е�����
    memcpy(read_cache->buf, checkpoint_cache->buf, mgr->block_size);

    mgr->checkpoint_block_num--;
    checkpoint_cache->state = CLEAN;

    return SUCCESS;
}

// ��ָ��checkpoint cache������д�����ϣ�����hash�����ִ��
int commit_one_checkpoint_cache(void *key, void *buf, void *arg)
{
    cache_node_t *read_cache = buf;

    ASSERT(read_cache->block_id == (u64_t)key);

    return commit_checkpoint_cache(arg, read_cache);
}

// ����ǰmgr�����е�checkpoint cache����������
int commit_all_checkpoint_cache(cache_mgr_t *mgr)
{
    return hashtab_map(mgr->hcache, commit_one_checkpoint_cache, mgr);
}

// ��̨�������е�����������
void *commit_disk(void *arg)
{
    cache_mgr_t *mgr = arg;
    int ret;

    // ����ǰ���е�checkpoint cache�е���������
    ret = commit_all_checkpoint_cache(mgr);
    if (ret < 0)
    {
        LOG_ERROR("commit_all_checkpoint_cache failed(%d)\n", ret);
        return NULL;
    }

    ASSERT(mgr->checkpoint_block_num == 0);

    // ����Ӧ����־ʧЧ


    mgr->checkpoint_sn++;

    return NULL;
}

// �л�cache��Ҳ���ǽ�write cache��checkpoint cache����
void pingpong_cache(cache_mgr_t *mgr)
{
    ASSERT(mgr->onfly_tx_num == 0);  // writing cache��������ڽ���
    ASSERT(mgr->checkpoint_block_num == 0); // checkpoint cache���ȫ���������

    // �л�cache
    mgr->write_side = (mgr->write_side + 1) & 0x1;
    mgr->checkpoint_block_num = mgr->modified_block_num;

    // ����л�cache����
    mgr->modified_block_num = 0;
    mgr->modified_data_bytes = 0;
    mgr->first_modified_time = 0;

    mgr->allow_new_tx = TRUE; // ��������������

    return;
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

// �������޸�ʱ����������ӿڣ����ú���put����commit����ʱ�����Զ�put
void *tx_get_write_buffer(tx_t *tx, u64_t block_id)
{
    cache_node_t *cache = get_cache_node(tx->mgr, block_id, FOR_WRITE);
    if (cache == NULL)
        return NULL;

    // ���buffer��û������ӵ������Ȩ
    if (cache->owner_tx_id == 0)
    {
        cache->owner_tx_id = tx->tx_id;
    }

    // �������������޸�ͬһ��buffer
    if (cache->owner_tx_id != tx->tx_id)
    {
        LOG_ERROR("tx(%llu) get write buffer conflict(%llu).\n", tx->tx_id, cache->owner_tx_id);
        return NULL;
    }

    // ��һ��ʹ��
    if (cache->ref_cnt == 0)
    {
        list_add_tail(&tx->write_cache, &cache->node);

        if (tx->mgr->modified_block_num == 0)
        { // ��¼��һ�����޸ĵ�ʱ��
            tx->mgr->first_modified_time = os_get_ms_count();
        }

        tx->mgr->modified_block_num++;
        tx->mgr->modified_data_bytes += tx->mgr->block_size;
    }

    cache->state = DIRTY;
    cache->ref_cnt++;

    return cache->buf;
}

// �ͷŶ�write cache��ռ��
void put_write_cache(cache_node_t *write_cache)
{
    // �Ѿ����ͷŹ��ˣ�˵������������
    ASSERT(write_cache->ref_cnt != 0);

    write_cache->ref_cnt = 0;
    list_del(&write_cache->node);

    return;
}


// ����ʱ�����cache�е�����ˢ��write_cache��
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

// ��write buffer�е����ݷ���
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


    // ˵��checkpoint��û��������ɣ���ʱ�������л�cache
    if (mgr->checkpoint_block_num != 0)
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

// �ύ�޸ĵ����ݵ���־����ʱдcache�е����ݻ�δ����
int tx_commit(tx_t *tx)
{

    // 1. ��write_cache�е�����д��־

    // 2. ����ʱ�����cache�е�����ˢ��write_cache�У���ʱδʵ��
    commit_write_cache(tx);

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
    return;
}

// ����cache��Ҫ��read cache��checkpoint cache��write cache��Ҫ����
void destroy_cache(cache_node_t *read_cache)
{
    free_cache_node(read_cache->side_cache[0]);
    free_cache_node(read_cache->side_cache[1]);
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
    while ((mgr->onfly_tx_num) || (mgr->modified_block_num) || (mgr->checkpoint_block_num))
    {
        LOG_ERROR("onfly_tx_num(%d), modified_block_num(%d), checkpoint_block_num(%d)\n",
            mgr->onfly_tx_num, mgr->modified_block_num, mgr->checkpoint_block_num);
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


