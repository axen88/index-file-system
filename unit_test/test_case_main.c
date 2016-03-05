#include <stdio.h>
#include <string.h>
#include "Basic.h"

int add_index_test_case(void);

int main(void)
{
    int ret;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    // add cases
    ret = add_index_test_case();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

