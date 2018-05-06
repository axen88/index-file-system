
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
    u64_t dat;

    dat = 0;
    set_bit(&dat, 0);
    CU_ASSERT(dat == 1);
    set_bit(&dat, 1);
    CU_ASSERT(dat == 3);
    set_bit(&dat, 63);
    CU_ASSERT(dat == 0x8000000000000003);
    set_bit(&dat, 63);
    CU_ASSERT(dat == 0x8000000000000003);
    set_bit(&dat, 62);
    CU_ASSERT(dat == 0xC000000000000003);
    set_bit(&dat, 4);
    CU_ASSERT(dat == 0xC000000000000013);

    CU_ASSERT(check_bit(&dat, 0) == TRUE);
    CU_ASSERT(check_bit(&dat, 1) == TRUE);
    CU_ASSERT(check_bit(&dat, 63) == TRUE);
    CU_ASSERT(check_bit(&dat, 4) == TRUE);
    CU_ASSERT(check_bit(&dat, 62) == TRUE);
    CU_ASSERT(check_bit(&dat, 2) == FALSE);
    CU_ASSERT(check_bit(&dat, 3) == FALSE);

    clr_bit(&dat, 62);
    CU_ASSERT(dat == 0x8000000000000013);
    clr_bit(&dat, 0);
    CU_ASSERT(dat == 0x8000000000000012);
    clr_bit(&dat, 1);
    CU_ASSERT(dat == 0x8000000000000010);
    clr_bit(&dat, 63);
    CU_ASSERT(dat == 0x0000000000000010);
    clr_bit(&dat, 4);
    CU_ASSERT(dat == 0x0000000000000000);
    
    CU_ASSERT(check_bit(&dat, 0) == FALSE);
    CU_ASSERT(check_bit(&dat, 1) == FALSE);
    CU_ASSERT(check_bit(&dat, 63) == FALSE);
    CU_ASSERT(check_bit(&dat, 4) == FALSE);
    CU_ASSERT(check_bit(&dat, 62) == FALSE);
    CU_ASSERT(check_bit(&dat, 2) == FALSE);
    CU_ASSERT(check_bit(&dat, 3) == FALSE);

    dat = 0x8000000000000012;

    CU_ASSERT(set_dat_first_0bit(&dat, 0) == 0);
    CU_ASSERT(set_dat_first_0bit(&dat, 0) == 2);
    CU_ASSERT(set_dat_first_0bit(&dat, 0) == 3);
    CU_ASSERT(dat == 0x800000000000001F);
    CU_ASSERT(set_dat_first_0bit(&dat, 7) == 7);
    CU_ASSERT(dat == 0x800000000000009F);
    CU_ASSERT(set_dat_first_0bit(&dat, 7) == 8);
    CU_ASSERT(dat == 0x800000000000019F);
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
  

