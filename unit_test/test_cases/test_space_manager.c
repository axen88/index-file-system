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
File Name: TEST_SPACE_MANAGER.C
Author   : axen.hook
Version  : 1.00
Date     : 21/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 21/Mar/2016
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

// no space
void test_space_manager_1(void)
{
    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t start_blk;
    
    CU_ASSERT(0 == ofs_create_container("index0", 1000, 0, &ct));
    CU_ASSERT(0 == ofs_create_object(ct, 500, FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj));

    CU_ASSERT(alloc_space(obj, 100, 50, &start_blk) == -INDEX_ERR_NO_FREE_BLOCKS); // no space now

    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));

    CU_ASSERT(0 == ofs_open_container("index0", 0, &ct));
    CU_ASSERT(0 == ofs_close_container(ct));
}

// allocate all space by one/two allocation action
void test_space_manager_2(void)
{
    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t start_blk;
    int32_t ret;
    
    CU_ASSERT(0 == ofs_create_container("index0", 1000, 0, &ct));
    CU_ASSERT(0 == ofs_create_object(ct, 500, FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj));

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 10, 80, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 10, 80, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 10, 90, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 10, 90, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 10, 100, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 10, 100, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 10, 200, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 10, 200, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 100, 50, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 100, 50, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 100, 60, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 100, 60, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 120, 60, &start_blk); // will alloc 120, 30
    CU_ASSERT(start_blk == 120);
    CU_ASSERT(ret == 30);
    ret = alloc_space(obj, 120, 60, &start_blk); // will alloc 100, 20
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 20);
    ret = alloc_space(obj, 120, 60, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 150, 60, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 150, 60, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 160, 60, &start_blk); // will alloc 100, 50
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 50);
    ret = alloc_space(obj, 160, 60, &start_blk); // no space now
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));

    CU_ASSERT(0 == ofs_open_container("index0", 0, &ct));
    CU_ASSERT(0 == ofs_close_container(ct));
}

void test_space_manager_3(void)
{
    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t start_blk;
    int32_t ret;
    
    CU_ASSERT(0 == ofs_create_container("index0", 1000, 0, &ct));
    CU_ASSERT(0 == ofs_create_object(ct, 500, FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj));

    CU_ASSERT(free_space(obj, 100, 50) == 0);
    ret = alloc_space(obj, 110, 20, &start_blk); // will alloc 110, 20
    CU_ASSERT(start_blk == 110);
    CU_ASSERT(ret == 20);
    ret = alloc_space(obj, 130, 5, &start_blk); // will alloc 130, 5
    CU_ASSERT(start_blk == 130);
    CU_ASSERT(ret == 5);
    ret = alloc_space(obj, 135, 30, &start_blk); // will alloc 135, 15
    CU_ASSERT(start_blk == 135);
    CU_ASSERT(ret == 15);
    ret = alloc_space(obj, 150, 4, &start_blk); // will alloc 100, 4
    CU_ASSERT(start_blk == 100);
    CU_ASSERT(ret == 4);
    ret = alloc_space(obj, 104, 4, &start_blk); // will alloc 104, 4
    CU_ASSERT(start_blk == 104);
    CU_ASSERT(ret == 4);
    ret = alloc_space(obj, 108, 4, &start_blk); // will alloc 108, 2
    CU_ASSERT(start_blk == 108);
    CU_ASSERT(ret == 2);
    ret = alloc_space(obj, 112, 4, &start_blk); // no space
    CU_ASSERT(ret == -INDEX_ERR_NO_FREE_BLOCKS);
    
    CU_ASSERT(free_space(obj, 100, 50) == 0);
    
    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));

    CU_ASSERT(0 == ofs_open_container("index0", 0, &ct));
    CU_ASSERT(0 == ofs_close_container(ct));
}

#define TEST_OBJID   200
#define TEST_NUM     5

void test_space_manager_4(void)
{
    container_handle_t *ct;
    uint64_t start_blk[TEST_NUM];
    int32_t ret[TEST_NUM];
    int32_t i = 0;
    uint32_t blk_cnt[TEST_NUM] = {1, 20, 100, 500, 2000};
    
    CU_ASSERT(0 == ofs_create_container("index0", 100000, 0, &ct));

    for (i = 0; i < TEST_NUM; i++)
    {
        ret[i] = ofs_alloc_space(ct, TEST_OBJID, blk_cnt[i], &start_blk[i]);
        CU_ASSERT(start_blk[i] > 0);
        CU_ASSERT(ret[i] == blk_cnt[i]);
    }

    for (i = 0; i < TEST_NUM; i++)
    {
        ret[i] = ofs_free_space(ct, TEST_OBJID, start_blk[i], blk_cnt[i]);
        CU_ASSERT(ret[i] == 0);
    }
    
    CU_ASSERT(0 == ofs_close_container(ct));

    CU_ASSERT(0 == ofs_open_container("index0", 0, &ct));
    CU_ASSERT(0 == ofs_close_container(ct));
}

int add_space_manager_test_case(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_space_manager_suit", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test space manager 1", test_space_manager_1))
    {
       return -2;
    }

    if (NULL == CU_add_test(pSuite, "test space manager 2", test_space_manager_2))
    {
       return -3;
    }

    if (NULL == CU_add_test(pSuite, "test space manager 3", test_space_manager_3))
    {
       return -3;
    }

    if (NULL == CU_add_test(pSuite, "test space manager 4", test_space_manager_4))
    {
       return -3;
    }

    return 0;
}

