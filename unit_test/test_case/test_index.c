#include "index_if.h"
#include "os_log.h"

#include "Basic.h"



static int init_suite(void)
{
    LOG_SYSTEM_INIT();
    return index_init_system();
}

static int clean_suite(void)
{
    index_exit_system();
    LOG_SYSTEM_EXIT();
    //_CrtDumpMemoryLeaks();
	return 0;
}

void test_create_index(void)
{
    INDEX_HANDLE *index[5];
    
    CU_ASSERT(0 == index_create("index0", 1000, 0, &index[0]));
    CU_ASSERT(0 == index_create("index1", 1000, 0, &index[1]));
    CU_ASSERT(0 == index_create("index2", 1000, 0, &index[2]));
    CU_ASSERT(0 == index_create("index3", 1000, 0, &index[3]));
    CU_ASSERT(0 == index_create("index4", 1000, 0, &index[4]));
    
    CU_ASSERT(0 == index_close(index[0]));
    CU_ASSERT(0 == index_close(index[1]));
    CU_ASSERT(0 == index_close(index[2]));
    CU_ASSERT(0 == index_close(index[3]));
    CU_ASSERT(0 == index_close(index[4]));
}

void test_open_index(void)
{
    INDEX_HANDLE *index[5];
    
    CU_ASSERT(0 == index_open("index0", 0, &index[0]));
    CU_ASSERT(0 == index_open("index1", 0, &index[1]));
    CU_ASSERT(0 == index_open("index2", 0, &index[2]));
    CU_ASSERT(0 == index_open("index3", 0, &index[3]));
    CU_ASSERT(0 == index_open("index4", 0, &index[4]));
    
    CU_ASSERT(0 == index_close(index[0]));
    CU_ASSERT(0 == index_close(index[1]));
    CU_ASSERT(0 == index_close(index[2]));
    CU_ASSERT(0 == index_close(index[3]));
    CU_ASSERT(0 == index_close(index[4]));
}

int add_index_test_case(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_index_suit", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test create index", test_create_index))
    {
       return -2;
    }

    if (NULL == CU_add_test(pSuite, "test open index", test_open_index))
    {
       return -3;
    }

    return 0;
}

