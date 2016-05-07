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
#include "ofs_if.h"
#include "os_log.h"

#include "Basic.h"

static int init_suite(void)
{
    LOG_SYSTEM_INIT();
    return ofs_init_system();
}

static int clean_suite(void)
{
    ofs_exit_system();
    LOG_SYSTEM_EXIT();
    //_CrtDumpMemoryLeaks();
	return 0;
}

#define TEST_V1  "abcdefghijklmnopqrstuvwxyz0123456789"
#define TEST_V2  "sdfhsexiocuboipseurpohgjsre"

void test_kv_2(void)
{
#define TEST_KEY_BEGIN   0
#define TEST_KEY_NUM     100000

    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t key;
    uint64_t i;
    
    // create ct and object, insert key
    CU_ASSERT(ofs_create_container("kv", 100000, &ct) == 0);
    CU_ASSERT(ofs_create_object(ct, 500, FLAG_TABLE | CR_BINARY | (CR_ANSI_STRING << 4), &obj) == 0);

    key = TEST_KEY_BEGIN;
    for (i = 0; i < TEST_KEY_NUM; i++, key++)
    {
        CU_ASSERT(index_insert_key(obj, &key, U64_MAX_SIZE, TEST_V1, strlen(TEST_V1)) == 0);
    }

    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);
    
    // open ct and object, remove key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN;
    for (i = 0; i < TEST_KEY_NUM; i++, key++)
    {
        CU_ASSERT(index_remove_key(obj, &key, U64_MAX_SIZE) == 0);
    }
    
    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);

    // open ct and object, insert key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN;
    for (i = 0; i < TEST_KEY_NUM; i++, key++)
    {
        CU_ASSERT(index_insert_key(obj, &key, U64_MAX_SIZE, TEST_V2, strlen(TEST_V2)) == 0);
    }

    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);

    // open ct and object, remove key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN;
    for (i = 0; i < TEST_KEY_NUM; i++, key++)
    {
        CU_ASSERT(index_remove_key(obj, &key, U64_MAX_SIZE) == 0);
    }

    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);
}

void test_kv_3(void)
{
#define TEST_KEY_BEGIN1   0
#define TEST_KEY_NUM1     100000
    
#define TEST_KEY_BEGIN2   (TEST_KEY_BEGIN1 + TEST_KEY_NUM1)
#define TEST_KEY_NUM2     100000

    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t key;
    uint64_t i;
    
    // create ct and object, insert key
    CU_ASSERT(ofs_create_container("kv", 100000, &ct) == 0);
    CU_ASSERT(ofs_create_object(ct, 500, FLAG_TABLE | CR_BINARY | (CR_ANSI_STRING << 4), &obj) == 0);

    key = TEST_KEY_BEGIN1;
    for (i = 0; i < TEST_KEY_NUM1; i++, key++)
    {
        CU_ASSERT(index_insert_key(obj, &key, U64_MAX_SIZE, TEST_V1, strlen(TEST_V1)) == 0);
    }

    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);

    // open ct and object, insert key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN2;
    for (i = 0; i < TEST_KEY_NUM2; i++, key++)
    {
        CU_ASSERT(index_insert_key(obj, &key, U64_MAX_SIZE, TEST_V2, strlen(TEST_V2)) == 0);
    }
    
    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);

    // open ct and object, remove key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN1;
    for (i = 0; i < TEST_KEY_NUM1; i++, key++)
    {
        CU_ASSERT(index_remove_key(obj, &key, U64_MAX_SIZE) == 0);
    }
    
    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);
    
    // open ct and object, remove key
    CU_ASSERT(ofs_open_container("kv", &ct) == 0);
    CU_ASSERT(ofs_open_object(ct, 500, &obj) == 0);

    key = TEST_KEY_BEGIN2;
    for (i = 0; i < TEST_KEY_NUM2; i++, key++)
    {
        CU_ASSERT(index_remove_key(obj, &key, U64_MAX_SIZE) == 0);
    }
    
    CU_ASSERT(ofs_close_object(obj) == 0);
    CU_ASSERT(ofs_close_container(ct) == 0);
}

int add_kv_test_case2(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_kv_suit2", init_suite, clean_suite);
    if (!pSuite) {
       return -1;
    }
    
    if (!CU_add_test(pSuite, "test kv 2", test_kv_2))
    {
       return -2;
    }

    if (!CU_add_test(pSuite, "test kv 3", test_kv_3))
    {
       return -2;
    }

    return 0;
}

