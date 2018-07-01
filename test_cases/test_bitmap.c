
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  

#include "globals.h"
#include "os_adapter.h"
#include "tx_cache.h"
#include "log.h"

#include "disk_if.h"
#include "file_if.h"

#include "bitmap.h"

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

// 测试bit
void test_bitmap_case0(void)
{
    u64_t buf[3] = {0};
    uint32_t x;

    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 0);
    CU_ASSERT(buf[0] == 0x0000000000000001);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 1);
    CU_ASSERT(buf[0] == 0x0000000000000003);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 2);
    CU_ASSERT(buf[0] == 0x0000000000000007);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == INVALID_U32);

    buf[0] = 0x0FFFFFFFFFFFFFFF;
    x = set_buf_first_0bit(buf, 0, 65);
    CU_ASSERT(x == 60);
    CU_ASSERT(buf[0] == 0x1FFFFFFFFFFFFFFF);
    
    buf[0] = 0x7FFFFFFFFFFFFFFF;
    x = set_buf_first_0bit(buf, 0, 65);
    CU_ASSERT(x == 63);
    CU_ASSERT(buf[0] == 0xFFFFFFFFFFFFFFFF);
    
    x = set_buf_first_0bit(buf, 0, 65);
    CU_ASSERT(x == 64);
    CU_ASSERT(buf[0] == 0xFFFFFFFFFFFFFFFF);
    CU_ASSERT(buf[1] == 0x0000000000000001);

    x = set_buf_first_0bit(buf, 0, 65);
    CU_ASSERT(x == INVALID_U32);

    buf[0] = 0xFFFFFFFFFFFFFFFF;
    buf[1] = 0xFFFFFFFFFFFFFFFF;
    buf[2] = 0x0000000000000001;
    x = set_buf_first_0bit(buf, 0, 130);
    CU_ASSERT(x == 129);
    CU_ASSERT(buf[0] == 0xFFFFFFFFFFFFFFFF);
    CU_ASSERT(buf[1] == 0xFFFFFFFFFFFFFFFF);
    CU_ASSERT(buf[2] == 0x0000000000000003);

    buf[0] = 0xFFFFFFFFFFFFFFFF;
    buf[1] = 0xFFFFFFFFFFFFFFFF;
    buf[2] = 0x0000000000000001;
    x = set_buf_first_0bit(buf, 128, 2);
    CU_ASSERT(x == 129);
    CU_ASSERT(buf[0] == 0xFFFFFFFFFFFFFFFF);
    CU_ASSERT(buf[1] == 0xFFFFFFFFFFFFFFFF);
    CU_ASSERT(buf[2] == 0x0000000000000003);

}

// 测试
void test_bitmap_case1(void)
{
    bitmap_hnd_t *hnd = bitmap_init_system(BD_NAME, BD_BLOCK_SIZE, &test_bd_ops, 80);

    CU_ASSERT(hnd != NULL);


    bitmap_exit_system(hnd);
}


// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_bitmap_cases[]
= {  
    {to_str(test_bitmap_case0), test_bitmap_case0},  
    CU_TEST_INFO_NULL  
};  

  
// suite初始化过程 
int test_bitmap_init(void)
{  
    void *hnd;
    
    LOG_SYSTEM_INIT(".", "test_log");

    assert(os_file_open_or_create(&hnd, BD_NAME) == 0);
    assert(os_file_resize(hnd, 5*1024*1024) == 0); // 5MB
    assert(os_file_close(hnd) == 0);
    
    return 0;  
      
}  
  
// suite清理过程，以便恢复原状，使结果不影响到下次运行 
int test_bitmap_clean(void)
{  
    LOG_SYSTEM_EXIT();
    return 0;  
}  
  

