
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  


#include "os_adapter.h"
#include "../tx_cache.h"

void test_tx_cache_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system("test", 1024);
    cache_node_t *write_cache;
    void *write_buf;
    cache_node_t *read_cache;
    void *read_buf;
    uint8_t  checkpoint_side = (mgr->writing_side + 1) & 0x1;

    #define BLOCK_ID1 100
    #define BLOCK_ID2 10
        
    CU_ASSERT(mgr != NULL);

    // 测试先写
    write_buf = get_buffer(mgr, BLOCK_ID1, WRITE_BUF);
    CU_ASSERT(write_buf != NULL);
    write_cache = list_entry(write_buf, cache_node_t, dat);
    CU_ASSERT(write_cache->block_id == BLOCK_ID1);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, READ_BUF);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, dat);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);

    CU_ASSERT(write_buf != read_buf);
    CU_ASSERT(read_cache->side_node[mgr->writing_side] == write_cache);
    CU_ASSERT(read_cache->side_node[checkpoint_side] == NULL);


    tx_cache_exit_system(mgr);
}



// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_tx_cache_case0), test_tx_cache_case0},  
    CU_TEST_INFO_NULL  
};  


