
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>  
#include <CUnit/Console.h>  
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  
#include <CUnit/Automated.h>  

// 测试用例集合
void add_test_bitmap_suites(void);
void add_tx_cache_suites(void);
  
// suite初始化过程 
int suite_success_init(void)
{  
    return 0;  
}  
  
// suite清理过程，以便恢复原状，使结果不影响到下次运行 
int suite_success_clean(void)
{  
    return 0;  
}  
  
// 测试类的调用总接口 
void add_test_suites(void)
{  
    assert(NULL != CU_get_registry());  
    assert(!CU_is_test_running());  
  
    add_test_bitmap_suites();
	add_tx_cache_suites();
}  

// 运行测试入口 
int run_test(void)
{  
    if (CU_initialize_registry())
    {  
        fprintf(stderr, "Initialize Test Registry failed.");  
        exit(EXIT_FAILURE);  
    }

    add_test_suites();  
    
    /// 测试模式1: Automated Mode，自动模式，会生成报表，用浏览器查看测试结果
    //CU_set_output_filename("test_suite"); 
    //CU_list_tests_to_file(); 
    //CU_automated_run_tests(); 

    // 测试模式2: Basice Mode，基础模式，直接看测试结果
    CU_basic_set_mode(CU_BRM_VERBOSE); 
    CU_basic_run_tests(); 
    

    // 测试模式3: Console Mode，交互模式，在交互界面上运行测试或查看测试结果
    //CU_console_run_tests(); 


    CU_cleanup_registry();  

    return CU_get_error();  

}  

int main(int argc, char *argv[])
{
    run_test();
    
    return 0;
}

