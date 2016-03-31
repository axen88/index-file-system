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
File Name: TEST_INDEX.C
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
    index_handle_t *index[5];
    
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
    index_handle_t *index[5];
    
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

