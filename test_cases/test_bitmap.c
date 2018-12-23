
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

// 测试bit
void test_bitmap_case0(void)
{
    uint8_t buf[3] = {0};
    uint32_t x;

    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 0);
    CU_ASSERT(buf[0] == 0x01);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 1);
    CU_ASSERT(buf[0] == 0x03);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == 2);
    CU_ASSERT(buf[0] == 0x07);
    
    x = set_buf_first_0bit(buf, 0, 3);
    CU_ASSERT(x == INVALID_U32);

    buf[0] = 0x0F;
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == 4);
    CU_ASSERT(buf[0] == 0x1F);
    
    buf[0] = 0x0F;
    x = set_buf_first_0bit(buf, 2, 23);
    CU_ASSERT(x == 4);
    CU_ASSERT(buf[0] == 0x1F);
    
    buf[0] = 0x7F;
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == 7);
    CU_ASSERT(buf[0] == 0xFF);
    
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == 8);
    CU_ASSERT(buf[0] == 0xFF);
    CU_ASSERT(buf[1] == 0x01);

    buf[0] = 0xFF;
    buf[1] = 0xFF;
    buf[2] = 0x01;
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == 17);
    CU_ASSERT(buf[0] == 0xFF);
    CU_ASSERT(buf[1] == 0xFF);
    CU_ASSERT(buf[2] == 0x03);
    
    buf[0] = 0xFF;
    buf[1] = 0xFF;
    buf[2] = 0x3F;
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == 22);
    
    x = set_buf_first_0bit(buf, 0, 23);
    CU_ASSERT(x == INVALID_U32);
}

// 测试
void test_bitmap_case1(void)
{
    create_bitmap_para_t para;

    para.bd_name = BD_NAME;
    para.bd_ops = &test_bd_ops;
    para.block_size = BD_BLOCK_SIZE;
    para.total_bits = 80;
    bitmap_mgr_t *bmp = create_bitmap(&para);

    CU_ASSERT(bmp != NULL);


    close_bitmap(bmp);
}


// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_bitmap_cases[]
= {  
    {to_str(test_bitmap_case0), test_bitmap_case0},  
    {to_str(test_bitmap_case1), test_bitmap_case1},  
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
  

// 定义suite数组，包括多个suite，每个suite又会包括若干个测试方法。  
CU_SuiteInfo test_bitmap_suites[]
= {  
    {"test_bitmap_suite", test_bitmap_init, test_bitmap_clean, test_bitmap_cases},  
    CU_SUITE_INFO_NULL  
};  
  
//  
void add_test_bitmap_suites(void)
{  
    assert(NULL != CU_get_registry());  
    assert(!CU_is_test_running());  

    if (CUE_SUCCESS != CU_register_suites(test_bitmap_suites))
    {  
        exit(EXIT_FAILURE);  
    }  
}  
