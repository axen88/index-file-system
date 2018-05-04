
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  

#include "globals.h"
#include "os_adapter.h"
#include "../tx_cache.h"
#include "log.h"

#include "disk_if.h"
#include "file_if.h"

// ������������
int test_bd_open(void **hnd, char *bd_name)
{
    return os_disk_open(hnd, bd_name);
}

int test_bd_read(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pread(hnd, buf, size, offset);
}

int test_bd_write(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pwrite(hnd, buf, size, offset);
}

void test_bd_close(void *hnd)
{
    os_disk_close(hnd);
}

space_ops_t test_bd_ops
= {
    "test_bd_ops",
        
    test_bd_open,
    test_bd_read,
    test_bd_write,
    test_bd_close
};

#define BD_NAME  "test"
#define BD_BLOCK_SIZE 1024

#define BLOCK_ID1 1

// ����get/put buffer
void test_cache_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *checkpoint_cache;
    u64_t *checkpoint_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr����ȷ��
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // ��ȡread buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    read_buf[0] = 0x1122334455667788;  // �޸�����
    read_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    read_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    // ��ȡcheckpoint buf
    checkpoint_buf = get_buffer(mgr, BLOCK_ID1, CHECKPOINT_BUF);
    CU_ASSERT(checkpoint_buf != NULL);
    checkpoint_cache = list_entry(checkpoint_buf, cache_node_t, buf);
    CU_ASSERT(checkpoint_cache->state == CLEAN);
    CU_ASSERT(checkpoint_cache->block_id == BLOCK_ID1);
    CU_ASSERT(checkpoint_cache->ref_cnt == 1);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, checkpoint_buf);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);
    checkpoint_buf[1] = 0x2233445566778899;  // �޸�����
    
    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_cache = list_entry(write_buf, cache_node_t, buf);
    CU_ASSERT(write_cache->state == CLEAN);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    CU_ASSERT(write_cache->ref_cnt == 1);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);
    CU_ASSERT(checkpoint_buf[0] == write_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == write_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == write_buf[2]);

    // �����ȷ��һ�¸�buffer�Ĺ�ϵ��ȷ
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_buf[0] == write_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == write_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == write_buf[2]);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] != read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// ����get/put buffer
void test_cache_case1(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *checkpoint_cache;
    u64_t *checkpoint_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr����ȷ��
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // ��ȡread buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    read_buf[0] = 0x1122334455667788;  // �޸�����
    read_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    read_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_cache = list_entry(write_buf, cache_node_t, buf);
    CU_ASSERT(write_cache->state == CLEAN);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    CU_ASSERT(write_cache->ref_cnt == 1);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    write_buf[1] = 0x2233445566778899;  // �޸�����
    
    // ��ȡcheckpoint buf
    checkpoint_buf = get_buffer(mgr, BLOCK_ID1, CHECKPOINT_BUF);
    CU_ASSERT(checkpoint_buf != NULL);
    checkpoint_cache = list_entry(checkpoint_buf, cache_node_t, buf);
    CU_ASSERT(checkpoint_cache->state == CLEAN);
    CU_ASSERT(checkpoint_cache->block_id == BLOCK_ID1);
    CU_ASSERT(checkpoint_cache->ref_cnt == 1);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, checkpoint_buf);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);

    // �����ȷ��һ�¸�buffer�Ĺ�ϵ��ȷ
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_buf[0] == write_buf[0]);
    CU_ASSERT(checkpoint_buf[1] != write_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == write_buf[2]);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// ����get/put buffer
void test_cache_case2(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *checkpoint_cache;
    u64_t *checkpoint_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr����ȷ��
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_cache = list_entry(write_buf, cache_node_t, buf);
    CU_ASSERT(write_cache->state == CLEAN);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    CU_ASSERT(write_cache->ref_cnt == 1);
    CU_ASSERT(write_cache->read_cache != NULL);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);
    write_buf[0] = 0x1122334455667788;  // �޸�����
    write_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    write_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����
    
    // ��ȡread buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    CU_ASSERT(read_buf[0] != write_buf[0]);
    CU_ASSERT(read_buf[1] != write_buf[1]);
    CU_ASSERT(read_buf[2] != write_buf[2]);
    read_buf[1] = 0x2233445566778899;  // �޸�����
    
    // ��ȡcheckpoint buf
    checkpoint_buf = get_buffer(mgr, BLOCK_ID1, CHECKPOINT_BUF);
    CU_ASSERT(checkpoint_buf != NULL);
    checkpoint_cache = list_entry(checkpoint_buf, cache_node_t, buf);
    CU_ASSERT(checkpoint_cache->state == CLEAN);
    CU_ASSERT(checkpoint_cache->block_id == BLOCK_ID1);
    CU_ASSERT(checkpoint_cache->ref_cnt == 1);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, checkpoint_buf);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);

    // �����ȷ��һ�¸�buffer�Ĺ�ϵ��ȷ
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_buf[0] != write_buf[0]);
    CU_ASSERT(checkpoint_buf[1] != write_buf[1]);
    CU_ASSERT(checkpoint_buf[2] != write_buf[2]);
    CU_ASSERT(checkpoint_buf[0] == read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// ����get/put buffer
void test_cache_case3(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *checkpoint_cache;
    u64_t *checkpoint_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr����ȷ��
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // ��ȡcheckpoint buf
    checkpoint_buf = get_buffer(mgr, BLOCK_ID1, CHECKPOINT_BUF);
    CU_ASSERT(checkpoint_buf != NULL);
    checkpoint_cache = list_entry(checkpoint_buf, cache_node_t, buf);
    CU_ASSERT(checkpoint_cache->state == CLEAN);
    CU_ASSERT(checkpoint_cache->block_id == BLOCK_ID1);
    CU_ASSERT(checkpoint_cache->ref_cnt == 1);
    CU_ASSERT(checkpoint_cache->read_cache != NULL);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    put_buffer(mgr, checkpoint_buf);
    checkpoint_buf[0] = 0x1122334455667788;  // �޸�����
    checkpoint_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    checkpoint_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    // ��ȡread buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    CU_ASSERT(checkpoint_buf[0] != read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] != read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] != read_buf[2]);
    read_buf[1] = 0x2233445566778899;  // �޸�����
    
    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_cache = list_entry(write_buf, cache_node_t, buf);
    CU_ASSERT(write_cache->state == CLEAN);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    CU_ASSERT(write_cache->ref_cnt == 1);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);

    // �����ȷ��һ�¸�buffer�Ĺ�ϵ��ȷ
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == checkpoint_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_cache->read_cache == read_cache);
    CU_ASSERT(checkpoint_cache->side_cache[0] == NULL);
    CU_ASSERT(checkpoint_cache->side_cache[1] == NULL);
    CU_ASSERT(checkpoint_buf[0] == write_buf[0]);
    CU_ASSERT(checkpoint_buf[1] == write_buf[1]);
    CU_ASSERT(checkpoint_buf[2] == write_buf[2]);
    CU_ASSERT(checkpoint_buf[0] != read_buf[0]);
    CU_ASSERT(checkpoint_buf[1] != read_buf[1]);
    CU_ASSERT(checkpoint_buf[2] != read_buf[2]);

    tx_cache_exit_system(mgr);
}

// ����buffer���޸ģ�ֱ��д����(������)������Ч������
void test_cache_case4(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;

    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // �޸�����
    write_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    write_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����
    
    // ���buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // ȷ��read buf�е�����Ϊȫ0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);

    // �л�ǰcommit���޷�������
    CU_ASSERT(commit_all_checkpoint_cache(mgr) == 0);
    
    // read buf�е����ݻ���Ϊȫ0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);   

    // �л�cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(commit_all_checkpoint_cache(mgr) == 0);
    
    // read buf���ݸı�
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    put_buffer(mgr, read_buf);   

    tx_cache_exit_system(mgr);
}

// ����buffer���޸ģ��ȶ����޸�
void test_cache_case5(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;

    // ȷ��read buf�е�����Ϊȫ0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    put_buffer(mgr, read_buf);
    read_buf[0] = 0x1122334455667788;  // �޸�����
    read_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    read_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    CU_ASSERT(write_buf[0] == read_buf[0]);
    CU_ASSERT(write_buf[1] == read_buf[1]);
    CU_ASSERT(write_buf[2] == read_buf[2]);
    write_buf[0] = ~read_buf[0];  // �޸�����
    write_buf[1] = ~read_buf[1];  // �޸�����
    write_buf[2] = ~read_buf[2];  // �޸�����
    
    // ���buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // �л�cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(commit_all_checkpoint_cache(mgr) == 0);
    
    // read buf���ݸı�
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    put_buffer(mgr, read_buf);   

    tx_cache_exit_system(mgr);
}

// ����buffer���޸�
void test_cache_case6(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;
    u64_t *checkpoint_buf;

    // ��ȡcheckpoint buf
    checkpoint_buf = get_buffer(mgr, BLOCK_ID1, CHECKPOINT_BUF);
    CU_ASSERT(checkpoint_buf != NULL);
    put_buffer(mgr, checkpoint_buf);
    checkpoint_buf[0] = 0x1122334455667788;  // �޸�����
    checkpoint_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    checkpoint_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    // ȷ��read buf�е�����
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);   

    // ��ȡwrite buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    CU_ASSERT(write_buf[0] == checkpoint_buf[0]);
    CU_ASSERT(write_buf[1] == checkpoint_buf[1]);
    CU_ASSERT(write_buf[2] == checkpoint_buf[2]);
    write_buf[0] = ~checkpoint_buf[0];  // �޸�����
    write_buf[1] = ~checkpoint_buf[1];  // �޸�����
    write_buf[2] = ~checkpoint_buf[2];  // �޸�����
    
    // ���buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // �л�cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(commit_all_checkpoint_cache(mgr) == 0);
    
    // read buf���ݸı�
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    CU_ASSERT(read_buf[0] != checkpoint_buf[0]);
    CU_ASSERT(read_buf[1] != checkpoint_buf[1]);
    CU_ASSERT(read_buf[2] != checkpoint_buf[2]);
    put_buffer(mgr, read_buf);

    tx_cache_exit_system(mgr);
}

// ����������� tx
void test_tx_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    u64_t *read_buf;
    tx_t *tx;
        
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);

    // ��һ������д���
    write_buf = tx_get_write_buffer(tx, BLOCK_ID1);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // �޸�����
    write_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    write_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����

    mark_buffer_dirty(mgr, write_buf);
    
    write_cache = list_entry(write_buf, cache_node_t, buf);
    CU_ASSERT(write_cache->ref_cnt == 1);
    CU_ASSERT(write_cache->state == DIRTY);
    CU_ASSERT(mgr->write_side == 0);
    CU_ASSERT(mgr->onfly_tx_num == 1);
    
    // �ύ�޸ĵ����ݵ���־
    tx_commit(tx);
    
    CU_ASSERT(write_cache->ref_cnt == 0);
    CU_ASSERT(write_cache->state == DIRTY);
    CU_ASSERT(mgr->write_side == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);

    // ��������
    commit_disk(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 1);
    CU_ASSERT(write_cache->state == CLEAN);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    put_buffer(mgr, read_buf);

    tx_cache_exit_system(mgr);
}

// �������������������飬�Ա�ָ����һ��Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_cache_case0), test_cache_case0},  
    {to_str(test_cache_case1), test_cache_case1},  
    {to_str(test_cache_case2), test_cache_case2},  
    {to_str(test_cache_case3), test_cache_case3},  
    {to_str(test_cache_case4), test_cache_case4},  
    {to_str(test_cache_case5), test_cache_case5},  
    {to_str(test_cache_case6), test_cache_case6},  
    {to_str(test_tx_case0), test_tx_case0},  
    CU_TEST_INFO_NULL  
};  

  
// suite��ʼ������ 
int test_tx_cache_init(void)
{  
    void *hnd;
    
    LOG_SYSTEM_INIT(".", "test_log");

    assert(os_file_open_or_create(&hnd, BD_NAME) == 0);
    assert(os_file_resize(hnd, 5*1024*1024) == 0); // 5MB
    assert(os_file_close(hnd) == 0);
    
    return 0;  
      
}  
  
// suite������̣��Ա�ָ�ԭ״��ʹ�����Ӱ�쵽�´����� 
int test_tx_cache_clean(void)
{  
    LOG_SYSTEM_EXIT();
    return 0;  
}  
  

