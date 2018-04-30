
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  


#include "os_adapter.h"
#include "../tx_cache.h"

void test_tx_cache_case0(void)
{
    #define BD_BLOCK_SIZE 1024
    cache_mgr_t *mgr = tx_cache_init_system("test", BD_BLOCK_SIZE);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    uint8_t  checkpoint_side = (mgr->writing_side + 1) & 0x1;
    tx_t *tx;

    #define BLOCK_ID1 100
    #define BLOCK_ID2 10
        
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    CU_ASSERT(tx_new(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);

    // ��һ������д���
    write_buf = tx_get_write_buffer(tx, BLOCK_ID1);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // �޸�����
    write_buf[1] = 0x55AA55AA55AA55AA;  // �޸�����
    write_buf[2] = 0xAA55AA55AA55AA55;  // �޸�����
    write_cache = list_entry(write_buf, cache_node_t, dat);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    CU_ASSERT(write_cache->state == DIRTY);
    CU_ASSERT(write_cache->ref_cnt == 1);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, dat);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);

    CU_ASSERT(write_buf != read_buf);
    CU_ASSERT(read_cache->side_node[mgr->writing_side] == write_cache);
    CU_ASSERT(read_cache->side_node[checkpoint_side] == NULL);

    CU_ASSERT(mgr->onfly_tx_num == 1);

    // �ύ�޸ĵ����ݵ���־
    CU_ASSERT(mgr->writing_side == 0);
    tx_commit(tx);
    CU_ASSERT(mgr->writing_side == 1);
    
    CU_ASSERT(mgr->onfly_tx_num == 0);
    CU_ASSERT(write_cache->ref_cnt == 0);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, dat);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);

    // ��������
    commit_disk(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 1);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0x1122334455667788);
    CU_ASSERT(read_buf[1] == 0x55AA55AA55AA55AA);
    CU_ASSERT(read_buf[2] == 0xAA55AA55AA55AA55);
    put_buffer(mgr, read_buf);

    tx_cache_exit_system(mgr);
}



// �������������������飬�Ա�ָ����һ��Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_tx_cache_case0), test_tx_cache_case0},  
    CU_TEST_INFO_NULL  
};  


