
#include <CUnit/CUnit.h>  
#include <CUnit/TestDB.h>  


#include "os_adapter.h"
#include "../tx_cache.h"

void test_tx_cache_case0(void)
{


}



// 将多个测试用例打包成组，以便指定给一个Suite 
CU_TestInfo test_tx_cache_cases[]
= {  
    {to_str(test_tx_cache_case0), test_tx_cache_case0},  
    CU_TEST_INFO_NULL  
};  


