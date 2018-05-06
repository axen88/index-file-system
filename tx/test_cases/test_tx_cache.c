
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  

#include "globals.h"
#include "os_adapter.h"
#include "../tx_cache.h"
#include "log.h"

#include "disk_if.h"
#include "file_if.h"

// 操作函数定义
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

// 测试get/put buffer
void test_cache_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *flush_cache;
    u64_t *flush_buf;
    uint8_t  flush_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // 获取read buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[flush_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    read_buf[0] = 0x1122334455667788;  // 修改内容
    read_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    read_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    // 获取flush buf
    flush_buf = get_buffer(mgr, BLOCK_ID1, FOR_FLUSH);
    CU_ASSERT(flush_buf != NULL);
    flush_cache = list_entry(flush_buf, cache_node_t, buf);
    CU_ASSERT(flush_cache->state == CLEAN);
    CU_ASSERT(flush_cache->block_id == BLOCK_ID1);
    CU_ASSERT(flush_cache->ref_cnt == 1);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, flush_buf);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] == read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);
    flush_buf[1] = 0x2233445566778899;  // 修改内容
    
    // 获取write buf
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
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);
    CU_ASSERT(flush_buf[0] == write_buf[0]);
    CU_ASSERT(flush_buf[1] == write_buf[1]);
    CU_ASSERT(flush_buf[2] == write_buf[2]);

    // 最后再确认一下各buffer的关系正确
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_buf[0] == write_buf[0]);
    CU_ASSERT(flush_buf[1] == write_buf[1]);
    CU_ASSERT(flush_buf[2] == write_buf[2]);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] != read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试get/put buffer
void test_cache_case1(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *flush_cache;
    u64_t *flush_buf;
    uint8_t  flush_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // 获取read buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[flush_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    read_buf[0] = 0x1122334455667788;  // 修改内容
    read_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    read_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    // 获取write buf
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
    CU_ASSERT(read_cache->side_cache[flush_side] == NULL);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    write_buf[1] = 0x2233445566778899;  // 修改内容
    
    // 获取flush buf
    flush_buf = get_buffer(mgr, BLOCK_ID1, FOR_FLUSH);
    CU_ASSERT(flush_buf != NULL);
    flush_cache = list_entry(flush_buf, cache_node_t, buf);
    CU_ASSERT(flush_cache->state == CLEAN);
    CU_ASSERT(flush_cache->block_id == BLOCK_ID1);
    CU_ASSERT(flush_cache->ref_cnt == 1);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, flush_buf);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] == read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);

    // 最后再确认一下各buffer的关系正确
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_buf[0] == write_buf[0]);
    CU_ASSERT(flush_buf[1] != write_buf[1]);
    CU_ASSERT(flush_buf[2] == write_buf[2]);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] == read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试get/put buffer
void test_cache_case2(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *flush_cache;
    u64_t *flush_buf;
    uint8_t  flush_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // 获取write buf
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
    write_buf[0] = 0x1122334455667788;  // 修改内容
    write_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    write_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容
    
    // 获取read buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == NULL);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    CU_ASSERT(read_buf[0] != write_buf[0]);
    CU_ASSERT(read_buf[1] != write_buf[1]);
    CU_ASSERT(read_buf[2] != write_buf[2]);
    read_buf[1] = 0x2233445566778899;  // 修改内容
    
    // 获取flush buf
    flush_buf = get_buffer(mgr, BLOCK_ID1, FOR_FLUSH);
    CU_ASSERT(flush_buf != NULL);
    flush_cache = list_entry(flush_buf, cache_node_t, buf);
    CU_ASSERT(flush_cache->state == CLEAN);
    CU_ASSERT(flush_cache->block_id == BLOCK_ID1);
    CU_ASSERT(flush_cache->ref_cnt == 1);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, flush_buf);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] == read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);

    // 最后再确认一下各buffer的关系正确
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_buf[0] != write_buf[0]);
    CU_ASSERT(flush_buf[1] != write_buf[1]);
    CU_ASSERT(flush_buf[2] != write_buf[2]);
    CU_ASSERT(flush_buf[0] == read_buf[0]);
    CU_ASSERT(flush_buf[1] == read_buf[1]);
    CU_ASSERT(flush_buf[2] == read_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试get/put buffer
void test_cache_case3(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    cache_node_t *write_cache;
    u64_t *write_buf;
    cache_node_t *read_cache;
    u64_t *read_buf;
    cache_node_t *flush_cache;
    u64_t *flush_buf;
    uint8_t  flush_side = (mgr->write_side + 1) & 0x1;
    uint8_t  write_side = mgr->write_side;

    // mgr内容确认
    CU_ASSERT(mgr != NULL);
    CU_ASSERT(mgr->block_size == BD_BLOCK_SIZE);
    CU_ASSERT(mgr->cur_tx_id == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);

    // 获取flush buf
    flush_buf = get_buffer(mgr, BLOCK_ID1, FOR_FLUSH);
    CU_ASSERT(flush_buf != NULL);
    flush_cache = list_entry(flush_buf, cache_node_t, buf);
    CU_ASSERT(flush_cache->state == CLEAN);
    CU_ASSERT(flush_cache->block_id == BLOCK_ID1);
    CU_ASSERT(flush_cache->ref_cnt == 1);
    CU_ASSERT(flush_cache->read_cache != NULL);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    put_buffer(mgr, flush_buf);
    flush_buf[0] = 0x1122334455667788;  // 修改内容
    flush_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    flush_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    // 获取read buf
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    read_cache = list_entry(read_buf, cache_node_t, buf);
    CU_ASSERT(read_cache->state == CLEAN);
    CU_ASSERT(read_cache->block_id == BLOCK_ID1);
    CU_ASSERT(read_cache->ref_cnt == 1);
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == NULL);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, read_buf);
    CU_ASSERT(read_cache->ref_cnt == 0);
    CU_ASSERT(flush_buf[0] != read_buf[0]);
    CU_ASSERT(flush_buf[1] != read_buf[1]);
    CU_ASSERT(flush_buf[2] != read_buf[2]);
    read_buf[1] = 0x2233445566778899;  // 修改内容
    
    // 获取write buf
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
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    put_buffer(mgr, write_buf);
    CU_ASSERT(write_cache->ref_cnt == 0);

    // 最后再确认一下各buffer的关系正确
    CU_ASSERT(read_cache->read_cache == NULL);
    CU_ASSERT(read_cache->side_cache[write_side] == write_cache);
    CU_ASSERT(read_cache->side_cache[flush_side] == flush_cache);
    CU_ASSERT(write_cache->read_cache == read_cache);
    CU_ASSERT(write_cache->side_cache[0] == NULL);
    CU_ASSERT(write_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_cache->read_cache == read_cache);
    CU_ASSERT(flush_cache->side_cache[0] == NULL);
    CU_ASSERT(flush_cache->side_cache[1] == NULL);
    CU_ASSERT(flush_buf[0] == write_buf[0]);
    CU_ASSERT(flush_buf[1] == write_buf[1]);
    CU_ASSERT(flush_buf[2] == write_buf[2]);
    CU_ASSERT(flush_buf[0] != read_buf[0]);
    CU_ASSERT(flush_buf[1] != read_buf[1]);
    CU_ASSERT(flush_buf[2] != read_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试buffer的修改，直接写数据(不读盘)，并生效到盘上
void test_cache_case4(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;

    // 获取write buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    write_buf[0] = 0x1122334455667788;  // 修改内容
    write_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    write_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容
    
    // 标记buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // 确认read buf中的内容为全0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);

    // 切换前commit，无法下内容
    CU_ASSERT(flush_all_cache(mgr) == 0);
    
    // read buf中的内容还是为全0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);   

    // 切换cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(flush_all_cache(mgr) == 0);
    
    // read buf内容改变
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    put_buffer(mgr, read_buf);   

    tx_cache_exit_system(mgr);
}

// 测试buffer的修改，先读再修改
void test_cache_case5(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;

    // 确认read buf中的内容为全0
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    put_buffer(mgr, read_buf);
    read_buf[0] = 0x1122334455667788;  // 修改内容
    read_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    read_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    // 获取write buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    CU_ASSERT(write_buf[0] == read_buf[0]);
    CU_ASSERT(write_buf[1] == read_buf[1]);
    CU_ASSERT(write_buf[2] == read_buf[2]);
    write_buf[0] = ~read_buf[0];  // 修改内容
    write_buf[1] = ~read_buf[1];  // 修改内容
    write_buf[2] = ~read_buf[2];  // 修改内容
    
    // 标记buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // 切换cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(flush_all_cache(mgr) == 0);
    
    // read buf内容改变
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    put_buffer(mgr, read_buf);   

    tx_cache_exit_system(mgr);
}

// 测试buffer的修改
void test_cache_case6(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *write_buf;
    u64_t *read_buf;
    u64_t *flush_buf;

    // 获取flush buf
    flush_buf = get_buffer(mgr, BLOCK_ID1, FOR_FLUSH);
    CU_ASSERT(flush_buf != NULL);
    put_buffer(mgr, flush_buf);
    flush_buf[0] = 0x1122334455667788;  // 修改内容
    flush_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    flush_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    // 确认read buf中的内容
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);   

    // 获取write buf
    write_buf = get_buffer(mgr, BLOCK_ID1, FOR_WRITE);
    CU_ASSERT(write_buf != NULL);
    CU_ASSERT(write_buf[0] == flush_buf[0]);
    CU_ASSERT(write_buf[1] == flush_buf[1]);
    CU_ASSERT(write_buf[2] == flush_buf[2]);
    write_buf[0] = ~flush_buf[0];  // 修改内容
    write_buf[1] = ~flush_buf[1];  // 修改内容
    write_buf[2] = ~flush_buf[2];  // 修改内容
    
    // 标记buffer dirty
    mark_buffer_dirty(mgr, write_buf);
    
    put_buffer(mgr, write_buf);

    // 切换cache
    pingpong_cache(mgr);
    
    // commit
    CU_ASSERT(flush_all_cache(mgr) == 0);
    
    // read buf内容改变
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf[0] == write_buf[0]);
    CU_ASSERT(read_buf[1] == write_buf[1]);
    CU_ASSERT(read_buf[2] == write_buf[2]);
    CU_ASSERT(read_buf[0] != flush_buf[0]);
    CU_ASSERT(read_buf[1] != flush_buf[1]);
    CU_ASSERT(read_buf[2] != flush_buf[2]);
    put_buffer(mgr, read_buf);

    tx_cache_exit_system(mgr);
}

// 测试提交事务
void test_tx_case0(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    tx_cache_node_t *tx_cache;
    u64_t *tx_buf;
    u64_t *read_buf;
    tx_t *tx;
        
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);
    mgr->max_modified_blocks = 1;

    // 对一个块先写后读
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf != NULL);
    tx_buf[0] = 0x1122334455667788;  // 修改内容
    tx_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    tx_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容
    tx_cache = list_entry(tx_buf, tx_cache_node_t, buf);
    CU_ASSERT(tx_cache->state == CLEAN);

    tx_mark_buffer_dirty(tx, tx_buf);
    
    CU_ASSERT(tx_cache->ref_cnt == 1);
    CU_ASSERT(tx_cache->state == DIRTY);
    CU_ASSERT(mgr->write_side == 0);
    CU_ASSERT(mgr->onfly_tx_num == 1);

    tx_put_buffer(tx, tx_buf);
    CU_ASSERT(tx_cache->ref_cnt == 0);
    
    // 提交修改的数据到日志，tx buf中的数据生效到write buf
    tx_commit(tx);
    
    CU_ASSERT(tx_cache->state == CLEAN);
    CU_ASSERT(mgr->write_side == 1);
    CU_ASSERT(mgr->onfly_tx_num == 0);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    put_buffer(mgr, read_buf);

    // 真正下盘
    commit_disk(mgr);
    CU_ASSERT(mgr->flush_sn == 1);
    
    CU_ASSERT(read_buf[0] == tx_buf[0]);
    CU_ASSERT(read_buf[1] == tx_buf[1]);
    CU_ASSERT(read_buf[2] == tx_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试提交事务
void test_tx_case1(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *tx_buf;
    u64_t *tx_buf1;
    u64_t *read_buf;
    tx_t *tx;
        
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    CU_ASSERT(tx->tx_id == 1);
    mgr->max_modified_blocks = 1;
    CU_ASSERT(mgr->flush_sn == 0);

    // 对一个块先写后读
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf != NULL);
    tx_buf1 = tx_buf;
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf == tx_buf1);
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf == tx_buf1);
    tx_buf[0] = 0x1122334455667788;  // 修改内容
    tx_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    tx_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    tx_mark_buffer_dirty(tx, tx_buf);

    tx_put_buffer(tx, tx_buf);
    tx_put_buffer(tx, tx_buf);
    tx_put_buffer(tx, tx_buf);
    
    // 提交修改的数据到日志，tx buf中的数据生效到write buf
    tx_commit(tx);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);

    // 真正下盘
    commit_disk(mgr);
    
    CU_ASSERT(mgr->flush_sn == 1);
    
    CU_ASSERT(read_buf[0] == tx_buf[0]);
    CU_ASSERT(read_buf[1] == tx_buf[1]);
    CU_ASSERT(read_buf[2] == tx_buf[2]);
    
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf != NULL);
    tx_buf[0] = ~0x1122334455667788;  // 修改内容
    tx_buf[1] = ~0x55AA55AA55AA55AA;  // 修改内容
    tx_buf[2] = ~0xAA55AA55AA55AA55;  // 修改内容
    tx_mark_buffer_dirty(tx, tx_buf);
    
    tx_put_buffer(tx, tx_buf);
    
    tx_commit(tx);
    
    CU_ASSERT(read_buf[0] != tx_buf[0]);
    CU_ASSERT(read_buf[1] != tx_buf[1]);
    CU_ASSERT(read_buf[2] != tx_buf[2]);
    
    commit_disk(mgr);
    
    CU_ASSERT(read_buf[0] == tx_buf[0]);
    CU_ASSERT(read_buf[1] == tx_buf[1]);
    CU_ASSERT(read_buf[2] == tx_buf[2]);

    tx_cache_exit_system(mgr);
}

// 测试取消事务
void test_tx_case2(void)
{
    cache_mgr_t *mgr = tx_cache_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops);
    u64_t *tx_buf;
    u64_t *read_buf;
    tx_t *tx;
        
    CU_ASSERT(tx_alloc(mgr, &tx) == 0);
    mgr->max_modified_blocks = 1;
    CU_ASSERT(mgr->flush_sn == 0);

    // 对一个块先写后读
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf != NULL);
    tx_buf[0] = 0x1122334455667788;  // 修改内容
    tx_buf[1] = 0x55AA55AA55AA55AA;  // 修改内容
    tx_buf[2] = 0xAA55AA55AA55AA55;  // 修改内容

    tx_mark_buffer_dirty(tx, tx_buf);

    tx_put_buffer(tx, tx_buf);
    
    tx_cancel(tx);
    
    read_buf = get_buffer(mgr, BLOCK_ID1, FOR_READ);
    CU_ASSERT(read_buf != NULL);
    CU_ASSERT(read_buf[0] == 0);
    CU_ASSERT(read_buf[1] == 0);
    CU_ASSERT(read_buf[2] == 0);
    put_buffer(mgr, read_buf);

    // 真正下盘
    commit_disk(mgr);
    
    CU_ASSERT(read_buf[0] != tx_buf[0]);
    CU_ASSERT(read_buf[1] != tx_buf[1]);
    CU_ASSERT(read_buf[2] != tx_buf[2]);
    
    CU_ASSERT(mgr->modified_block_num == 0);
    
    tx_buf = tx_get_buffer(tx, BLOCK_ID1);
    CU_ASSERT(tx_buf != NULL);
    CU_ASSERT(tx_buf[0] == 0);
    CU_ASSERT(tx_buf[1] == 0);
    CU_ASSERT(tx_buf[2] == 0);
    tx_put_buffer(tx, tx_buf);

    tx_cache_exit_system(mgr);
}

// 将多个测试用例打包成组，以便指定给一个Suite 
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
    {to_str(test_tx_case1), test_tx_case1},  
    {to_str(test_tx_case2), test_tx_case2},  
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
  

