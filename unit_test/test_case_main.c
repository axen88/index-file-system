/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*******************************************************************************

            Copyright(C), 2016~2019, axen.hook@foxmail.com
********************************************************************************
File Name: TEST_CASE_MAIN.C
Author   : axen.hook
Version  : 1.00
Date     : 02/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 02/Mar/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "Basic.h"
#include "os_adapter.h"

int add_index_test_case(void);
int add_block_test_suite(void);
int add_kv_test_case(void);
int add_collate_test_case(void);
int add_space_manager_test_case(void);

int main(void)
{
    int ret;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    ret = add_block_test_suite();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ret = add_index_test_case();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ret = add_kv_test_case();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ret = add_collate_test_case();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ret = add_space_manager_test_case();
    if (0 != ret) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    ret = CU_get_error();
	system("pause");

    return ret;
}
