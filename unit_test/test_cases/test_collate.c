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
File Name: TEST_KV.C
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

#define TEST_V1  "abcdefghijklmnopqrstuvwxyz0123456789"
#define TEST_V2  "sdfhsexiocuboipseurpohgjsre"
#define TEST_V3  "dkljbdopisugopeiajebkljfdspoinbaueiogjweiphbjiopdsunopigfjdhre"

void test_cr_u64_1(void)
{
    CU_ASSERT(os_u64_size(0) == 1);
    CU_ASSERT(os_u64_size(0x01) == 1);
    CU_ASSERT(os_u64_size(0xFF) == 1);

    CU_ASSERT(os_u64_size(0x100) == 2);
    CU_ASSERT(os_u64_size(0xFFFF) == 2);
    
    CU_ASSERT(os_u64_size(0x10000) == 3);
    CU_ASSERT(os_u64_size(0xFFFFFF) == 3);
    
    CU_ASSERT(os_u64_size(0x1000000) == 4);
    CU_ASSERT(os_u64_size(0xFFFFFFFF) == 4);
    
    CU_ASSERT(os_u64_size(0x100000000) == 5);
    CU_ASSERT(os_u64_size(0xFFFFFFFFFF) == 5);
    
    CU_ASSERT(os_u64_size(0x10000000000) == 6);
    CU_ASSERT(os_u64_size(0xFFFFFFFFFFFF) == 6);
    
    CU_ASSERT(os_u64_size(0x1000000000000) == 7);
    CU_ASSERT(os_u64_size(0xFFFFFFFFFFFFFF) == 7);
    
    CU_ASSERT(os_u64_size(0x100000000000000) == 8);
    CU_ASSERT(os_u64_size(0xFFFFFFFFFFFFFFFF) == 8);
}

void test_cr_u64_2(void)
{
    uint64_t u64;
    uint8_t  *b = (uint8_t *)&u64;

    u64 = 0;
    CU_ASSERT(os_bstr_to_u64(b, 1) == 0);
    
    u64 = 1;
    CU_ASSERT(os_bstr_to_u64(b, 1) == 1);
    u64 = 0xff;
    CU_ASSERT(os_bstr_to_u64(b, 1) == 0xff);
    
    u64 = 0x100000000000000;
    CU_ASSERT(os_bstr_to_u64(b, 8) == 0x100000000000000);
    u64 = 0xFFFFFFFFFFFFFFFF;
    CU_ASSERT(os_bstr_to_u64(b, 8) == 0xFFFFFFFFFFFFFFFF);
}

void test_cr_u64_3(void)
{
    uint64_t u64;
    uint8_t  *b = (uint8_t *)&u64;

    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(0, b) == 1);
    CU_ASSERT(0 == u64);
    
    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(1, b) == 1);
    CU_ASSERT(1 == u64);
    
    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(0xFF, b) == 1);
    CU_ASSERT(0xFF == u64);
    
    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(0x100, b) == 2);
    CU_ASSERT(0x100 == u64);
    
    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(0x100000000000000, b) == 8);
    CU_ASSERT(0x100000000000000 == u64);
    
    u64 = 0;
    CU_ASSERT(os_u64_to_bstr(0xFFFFFFFFFFFFFFFF, b) == 8);
    CU_ASSERT(0xFFFFFFFFFFFFFFFF == u64);
}

int add_collate_test_case(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_collate_suit", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test collate u64 1", test_cr_u64_1))
    {
       return -2;
    }

    if (NULL == CU_add_test(pSuite, "test collate u64 2", test_cr_u64_2))
    {
       return -3;
    }

    if (NULL == CU_add_test(pSuite, "test collate u64 3", test_cr_u64_3))
    {
       return -3;
    }

    return 0;
}

