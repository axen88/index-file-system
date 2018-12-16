
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>  
#include <CUnit/Console.h>  
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  
#include <CUnit/Automated.h>  

// ������������
void add_test_bitmap_suites(void);
void add_tx_cache_suites(void);
  
// suite��ʼ������ 
int suite_success_init(void)
{  
    return 0;  
}  
  
// suite������̣��Ա�ָ�ԭ״��ʹ�����Ӱ�쵽�´����� 
int suite_success_clean(void)
{  
    return 0;  
}  
  
// ������ĵ����ܽӿ� 
void add_test_suites(void)
{  
    assert(NULL != CU_get_registry());  
    assert(!CU_is_test_running());  
  
    add_test_bitmap_suites();
	add_tx_cache_suites();
}  

// ���в������ 
int run_test(void)
{  
    if (CU_initialize_registry())
    {  
        fprintf(stderr, "Initialize Test Registry failed.");  
        exit(EXIT_FAILURE);  
    }

    add_test_suites();  
    
    /// ����ģʽ1: Automated Mode���Զ�ģʽ�������ɱ�����������鿴���Խ��
    //CU_set_output_filename("test_suite"); 
    //CU_list_tests_to_file(); 
    //CU_automated_run_tests(); 

    // ����ģʽ2: Basice Mode������ģʽ��ֱ�ӿ����Խ��
    CU_basic_set_mode(CU_BRM_VERBOSE); 
    CU_basic_run_tests(); 
    

    // ����ģʽ3: Console Mode������ģʽ���ڽ������������в��Ի�鿴���Խ��
    //CU_console_run_tests(); 


    CU_cleanup_registry();  

    return CU_get_error();  

}  

int main(int argc, char *argv[])
{
    run_test();
    
    return 0;
}

