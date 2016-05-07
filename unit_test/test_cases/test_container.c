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

void test_create_container(void)
{
    container_handle_t *ct[5];
    
    CU_ASSERT(ofs_create_container("ct0", 1000, &ct[0]) == 0);
    CU_ASSERT(ofs_create_container("ct1", 1000, &ct[1]) == 0);
    CU_ASSERT(ofs_create_container("ct2", 1000, &ct[2]) == 0);
    CU_ASSERT(ofs_create_container("ct3", 1000, &ct[3]) == 0);
    CU_ASSERT(ofs_create_container("ct4", 1000, &ct[4]) == 0);
    
    CU_ASSERT(ofs_close_container(ct[0]) == 0);
    CU_ASSERT(ofs_close_container(ct[1]) == 0);
    CU_ASSERT(ofs_close_container(ct[2]) == 0);
    CU_ASSERT(ofs_close_container(ct[3]) == 0);
    CU_ASSERT(ofs_close_container(ct[4]) == 0);
}

void test_open_container(void)
{
    container_handle_t *ct[5];
    
    CU_ASSERT(ofs_open_container("ct0", &ct[0]) == 0);
    CU_ASSERT(ofs_open_container("ct1", &ct[1]) == 0);
    CU_ASSERT(ofs_open_container("ct2", &ct[2]) == 0);
    CU_ASSERT(ofs_open_container("ct3", &ct[3]) == 0);
    CU_ASSERT(ofs_open_container("ct4", &ct[4]) == 0);
    
    CU_ASSERT(ofs_close_container(ct[0]) == 0);
    CU_ASSERT(ofs_close_container(ct[1]) == 0);
    CU_ASSERT(ofs_close_container(ct[2]) == 0);
    CU_ASSERT(ofs_close_container(ct[3]) == 0);
    CU_ASSERT(ofs_close_container(ct[4]) == 0);
}

#define TEST_BLOCK_FILE "blk_dat"
#define TEST_BLOCK_SHIFT 12
#define TEST_BLOCK_SIZE (1 << TEST_BLOCK_SHIFT)
#define TEST_START_LBA   5

static void random_buffer(uint8_t *buf, uint32_t size)
{
	while (size--)
	{
		*buf++ = (uint8_t)rand();
	}
}

void test_block_rw(void)
{
    container_handle_t *hnd;
    int64_t vbn = 10;
    uint64_t blkNum = 100;
    uint8_t wrBuf[TEST_BLOCK_SIZE];
    uint8_t rdBuf[TEST_BLOCK_SIZE];
    
    srand((unsigned)time(NULL));
    
    CU_ASSERT(ofs_create_container(TEST_BLOCK_FILE, 10000, &hnd) == 0);
    
    while (blkNum--)
    {
        random_buffer(wrBuf, TEST_BLOCK_SIZE);
        CU_ASSERT(ofs_update_block(hnd, wrBuf, TEST_BLOCK_SIZE, 0, vbn) == TEST_BLOCK_SIZE);
        CU_ASSERT(ofs_read_block(hnd, rdBuf, TEST_BLOCK_SIZE, 0, vbn) == TEST_BLOCK_SIZE);
        CU_ASSERT(memcmp(rdBuf, wrBuf, TEST_BLOCK_SIZE) == 0);
        vbn++;
    }

    CU_ASSERT(ofs_close_container(hnd) == 0);

    
    CU_ASSERT(ofs_open_container(TEST_BLOCK_FILE, &hnd) == 0);
    CU_ASSERT(ofs_close_container(hnd) == 0);
}


int add_container_test_case(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_container_suit", init_suite, clean_suite);
    if (!pSuite) {
       return -1;
    }
    
    if (!CU_add_test(pSuite, "test create container", test_create_container))
    {
       return -2;
    }

    if (!CU_add_test(pSuite, "test open container", test_open_container))
    {
       return -3;
    }

    if (!CU_add_test(pSuite, "test block rw", test_block_rw))
    {
       return -3;
    }

    return 0;
}

