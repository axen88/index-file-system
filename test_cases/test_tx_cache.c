
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  

#include "globals.h"
#include "os_adapter.h"
#include "tx_cache.h"
#include "log.h"

#include "disk_if.h"
#include "file_if.h"

// 操作函数定义
static int test_bd_open(void **hnd, char *bd_name)
{
    return os_disk_open(hnd, bd_name);
}

static int test_bd_read(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pread(hnd, buf, size, offset);
}

static int test_bd_write(void *hnd, void *buf, int size, u64_t offset)
{
    return os_disk_pwrite(hnd, buf, size, offset);
}

static void test_bd_close(void *hnd)
{
    os_disk_close(hnd);
}

static space_ops_t test_bd_ops
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
#define BLOCK_ID2 100
#define BLOCK_ID3 53

// 测试get/put buffer
void test_cache_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *rw_buf;
    u64_t *rw_buf2;
    u64_t *commit_buf;
    u64_t *flush_buf;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_commit_tx == 0);

    rw_buf = get_buffer_by_type(mgr, BLOCK_ID1, RW_BUF);
    CU_ASSERT(rw_buf == NULL);
    
    rw_buf = get_buffer(mgr, BLOCK_ID1, 0);
    CU_ASSERT(rw_buf != NULL);
    put_buffer(mgr, rw_buf);
    
    rw_buf2 = get_buffer_by_type(mgr, BLOCK_ID1, RW_BUF);
    CU_ASSERT(rw_buf2 == rw_buf);
    put_buffer(mgr, rw_buf2);
    
    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf == NULL);
    
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf == NULL);

    tx_cache_exit_system(mgr);
}

// 测试get/put buffer
void test_cache_case1(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *rw_buf;
    u64_t *rw_buf2;
    u64_t *commit_buf;
    u64_t *flush_buf;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_commit_tx == 0);

    rw_buf = get_buffer_by_type(mgr, BLOCK_ID1, RW_BUF);
    CU_ASSERT(rw_buf == NULL);
    
    rw_buf = get_buffer(mgr, BLOCK_ID1, F_NO_READ);
    CU_ASSERT(rw_buf != NULL);
    CU_ASSERT(rw_buf[0] == 0);
    put_buffer(mgr, rw_buf);
    
    rw_buf2 = get_buffer_by_type(mgr, BLOCK_ID1, RW_BUF);
    CU_ASSERT(rw_buf2 == rw_buf);
    put_buffer(mgr, rw_buf2);
    
    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf == NULL);
    
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf == NULL);

    tx_cache_exit_system(mgr);
}

// 测试get/put/commit buffer
void test_cache_case2(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *rw_buf;
    u64_t *commit_buf;
    u64_t *flush_buf;
    int    ret;

    rw_buf = get_buffer(mgr, BLOCK_ID1, F_NO_READ);
    CU_ASSERT(rw_buf != NULL);
    CU_ASSERT(rw_buf[0] == 0);
    rw_buf[0] = 0x1;
    mark_buffer_dirty(mgr, rw_buf);
    ret = commit_buffer(mgr, rw_buf);
    CU_ASSERT(ret == SUCCESS);
    put_buffer(mgr, rw_buf);
    
    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf != NULL);
    CU_ASSERT(commit_buf[0] == 0x1);
    put_buffer(mgr, commit_buf);
    
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf == NULL);
    
    rw_buf = get_buffer(mgr, BLOCK_ID1, 0);
    CU_ASSERT(rw_buf[0] == 0x1);
    rw_buf[0] = 0x2;
    mark_buffer_dirty(mgr, rw_buf);
    ret = commit_buffer(mgr, rw_buf);
    CU_ASSERT(ret == -ERR_CACHE_STATE); // 此时不允许commit
    put_buffer(mgr, rw_buf);

    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf == NULL);
    
    // checkpoint之后，就可以commit
    checkpoint_all_cache_block(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 1);

    // 此时commit buf为空，flush buf有内容
    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf == NULL);
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf != NULL);
    CU_ASSERT(flush_buf[0] == 0x1);
    put_buffer(mgr, flush_buf);

    rw_buf = get_buffer(mgr, BLOCK_ID1, 0);
    CU_ASSERT(rw_buf[0] == 0x2);
    ret = commit_buffer(mgr, rw_buf);
    CU_ASSERT(ret == SUCCESS); // 此时允许commit了
    put_buffer(mgr, rw_buf);

    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf != NULL);
    CU_ASSERT(commit_buf[0] == 0x2);
    put_buffer(mgr, commit_buf);
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf != NULL);
    CU_ASSERT(flush_buf[0] == 0x1);
    put_buffer(mgr, flush_buf);

    rw_buf = get_buffer(mgr, BLOCK_ID1, 0);
    CU_ASSERT(rw_buf[0] == 0x2);
    rw_buf[0] = 0x3;
    mark_buffer_dirty(mgr, rw_buf);
    ret = commit_buffer(mgr, rw_buf);
    CU_ASSERT(ret == -ERR_CACHE_STATE); // 此时不允许commit
    put_buffer(mgr, rw_buf);

    // 此时checkpoint会失败
    checkpoint_all_cache_block(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 1); // checkpoint sn没有变化，说明checkpoint失败

    // 下盘之后，就可以checkpoint了
    ret = flush_all_cache_block(mgr);
    CU_ASSERT(ret == SUCCESS);
    CU_ASSERT(mgr->flush_sn == 1);
    checkpoint_all_cache_block(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 2); // checkpoint sn变化，说明checkpoint成功
    
    rw_buf = get_buffer(mgr, BLOCK_ID1, 0);
    CU_ASSERT(rw_buf[0] == 0x3);
    ret = commit_buffer(mgr, rw_buf);
    CU_ASSERT(ret == SUCCESS); // 此时又允许commit了
    put_buffer(mgr, rw_buf);

    commit_buf = get_buffer_by_type(mgr, BLOCK_ID1, COMMIT_BUF);
    CU_ASSERT(commit_buf != NULL);
    CU_ASSERT(commit_buf[0] == 0x3);
    put_buffer(mgr, commit_buf);
    flush_buf = get_buffer_by_type(mgr, BLOCK_ID1, FLUSH_BUF);
    CU_ASSERT(flush_buf != NULL);
    CU_ASSERT(flush_buf[0] == 0x2);
    put_buffer(mgr, flush_buf);

    tx_cache_exit_system(mgr);
}

// 测试提交事务
void test_tx_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *tx_buf;
    tx_t *tx;
    int   ret;
        
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    CU_ASSERT(tx != NULL);
    CU_ASSERT(tx->tx_id == 1);
    mgr->max_modified_blocks = 1;
    CU_ASSERT(mgr->flush_sn == 0);

    // 对一个块先写后读
    tx_buf = tx_get_buffer(tx, BLOCK_ID1, F_NO_READ);
    CU_ASSERT(tx_buf != NULL);
    CU_ASSERT(tx_buf == tx_get_buffer(tx, BLOCK_ID1, 0));
    CU_ASSERT(tx_buf == tx_get_buffer(tx, BLOCK_ID1, 0));
    tx_buf[0] = 1;  // 修改内容
    tx_buf[1] = 2;  // 修改内容
    tx_buf[2] = 3;  // 修改内容

    tx_mark_buffer_dirty(tx, tx_buf);

    tx_put_buffer(tx, tx_buf);
    tx_put_buffer(tx, tx_buf);
    tx_put_buffer(tx, tx_buf);
    
    // 提交修改的数据到日志，tx buf中的数据生效到write buf
    tx_commit(tx);

    // checkpoint并下盘
    checkpoint_all_cache_block(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 1);
    ret = flush_all_cache_block(mgr);
    CU_ASSERT(ret == SUCCESS);
    CU_ASSERT(mgr->flush_sn == 1);
    
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    tx_buf = tx_get_buffer(tx, BLOCK_ID1, 0);
    CU_ASSERT(tx_buf != NULL);
    CU_ASSERT(tx_buf[0] == 1);
    CU_ASSERT(tx_buf[1] == 2);
    CU_ASSERT(tx_buf[2] == 3);
    tx_buf[0] = 4;  // 修改内容
    tx_buf[1] = 5;  // 修改内容
    tx_buf[2] = 6;  // 修改内容
    tx_mark_buffer_dirty(tx, tx_buf);
    
    tx_put_buffer(tx, tx_buf);
    
    tx_commit(tx);
    
    // checkpoint并下盘
    checkpoint_all_cache_block(mgr);
    CU_ASSERT(mgr->checkpoint_sn == 2);
    ret = flush_all_cache_block(mgr);
    CU_ASSERT(ret == SUCCESS);
    CU_ASSERT(mgr->flush_sn == 2);

    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    tx_buf = tx_get_buffer(tx, BLOCK_ID1, 0);
    CU_ASSERT(tx_buf != NULL);
    CU_ASSERT(tx_buf[0] == 4);
    CU_ASSERT(tx_buf[1] == 5);
    CU_ASSERT(tx_buf[2] == 6);
    tx_put_buffer(tx, tx_buf);
    tx_cancel(tx);

    tx_cache_exit_system(mgr);
}

// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_cache_case0), test_cache_case0},  
    {to_str(test_cache_case1), test_cache_case1},  
    {to_str(test_cache_case2), test_cache_case2},  
        
    {to_str(test_tx_case0), test_tx_case0},  
    CU_TEST_INFO_NULL  
};  

  
// suite初始化过程 
int test_tx_cache_init(void)
{  
    void *hnd;
    
    LOG_SYSTEM_INIT(".", "test_log");

    assert(os_file_open_or_create(&hnd, BD_NAME) == 0);
    assert(os_file_resize(hnd, 5*1024*1024) == 0); // 5MB
    assert(os_file_close(hnd) == 0);
    
    return 0;  
      
}  
  
// suite清理过程，以便恢复原状，使结果不影响到下次运行 
int test_tx_cache_clean(void)
{  
    LOG_SYSTEM_EXIT();
    return 0;  
}  
  
// 定义suite数组，包括多个suite，每个suite又会包括若干个测试方法。  
CU_SuiteInfo tx_cache_suites[]
= {  
    {"test_tx_cache_cases", test_tx_cache_init, test_tx_cache_clean, test_tx_cache_cases},  
    CU_SUITE_INFO_NULL  
};  

//  
void add_tx_cache_suites(void)
{  
    assert(NULL != CU_get_registry());  
    assert(!CU_is_test_running());  

    if (CUE_SUCCESS != CU_register_suites(tx_cache_suites))
    {  
        exit(EXIT_FAILURE);  
    }  
}  

