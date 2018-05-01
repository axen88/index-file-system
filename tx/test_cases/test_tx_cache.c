
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  


#include "os_adapter.h"
#include "../tx_cache.h"
#include "log.h"

#define BD_NAME  "test"

void test_tx_cache_case0(void)
{
    #define BD_BLOCK_SIZE 1024
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    tx_t *tx;

    #define BLOCK_ID1 100
    #define BLOCK_ID2 10
        
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    CU_ASSERT(tx_new(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);

    // 对一个块先写后读
    write_buf = tx_get_write_buffer(tx, BLOCK_ID1);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // 修改内容
    write_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    write_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容
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
    CU_ASSERT(read_cache->side_cache[mgr->write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);

    CU_ASSERT(mgr->onfly_tx_num == 1);

    // 提交修改的数据到日志
    CU_ASSERT(mgr->write_side == 0);
    tx_commit(tx);
    CU_ASSERT(mgr->write_side == 1);
    
    CU_ASSERT(mgr->onfly_tx_num == 0);
    CU_ASSERT(write_cache->ref_cnt == 0);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, dat);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);

    // 真正下盘
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

void test_tx_cache_case1(void)
{
    #define BD_BLOCK_SIZE 1024
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    uint8_t  checkpoint_side = (mgr->write_side + 1) & 0x1;
    tx_t *tx;

    #define BLOCK_ID1 100
    #define BLOCK_ID2 10
        
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    CU_ASSERT(tx_new(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);

    // 对一个块先写后读
    write_buf = tx_get_write_buffer(tx, BLOCK_ID1);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // 修改内容
    write_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    write_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容
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
    CU_ASSERT(read_cache->side_cache[mgr->write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[checkpoint_side] == NULL);

    CU_ASSERT(mgr->onfly_tx_num == 1);

    // 提交修改的数据到日志
    CU_ASSERT(mgr->write_side == 0);
    tx_commit(tx);
    CU_ASSERT(mgr->write_side == 1);
    
    CU_ASSERT(mgr->onfly_tx_num == 0);
    CU_ASSERT(write_cache->ref_cnt == 0);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, dat);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);

    // 真正下盘
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



// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_tx_cache_case0), test_tx_cache_case0},  
    CU_TEST_INFO_NULL  
};  

#include "file_if.h"
  
// suite初始化过程 
int test_tx_cache_init(void)
{  
    void *hnd;
    
    LOG_SYSTEM_INIT(".", "test_log");

    assert(os_file_open_or_create(&hnd, BD_NAME) == 0);
    assert(os_file_resize(hnd, 10*1024*1024) == 0); // 10MB
    assert(os_file_close(hnd) == 0);
    
    return 0;  
      
}  
  
// suite清理过程，以便恢复原状，使结果不影响到下次运行 
int test_tx_cache_clean(void)
{  
    LOG_SYSTEM_EXIT();
    return 0;  
}  
  

