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
File Name: TEST_BLOCK.C
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

#include <string.h>
#include <time.h>
#include <stdio.h>
#include "Basic.h"

#include "os_adapter.h"
#include "os_disk_if.h"
#include "index_layout.h"
#include "index_block_if.h"
#include "os_log.h"

#define TEST_BLOCK_FILE "Test.dat"
#define TEST_BLOCK_SHIFT 12
#define TEST_BLOCK_SIZE (1 << TEST_BLOCK_SHIFT)
#define TEST_START_LBA   5

static int init_suite(void)
{
    LOG_SYSTEM_INIT();
    return 0;
}

static int clean_suite(void)
{
    LOG_SYSTEM_EXIT();
    //_CrtDumpMemoryLeaks();
	return 0;
}


static void random_buffer(uint8_t *buf, uint32_t size)
{
	while (size--)
	{
		*buf++ = (uint8_t)rand();
	}
}

void test_block_rw(void)
{
    block_handle_t *hnd;
    int64_t vbn = 10;
    uint64_t blkNum = 100;
    uint8_t wrBuf[TEST_BLOCK_SIZE];
    uint8_t rdBuf[TEST_BLOCK_SIZE];
    
	srand((unsigned)time(NULL));
    
    CU_ASSERT(0 == block_create(&hnd, TEST_BLOCK_FILE, 10000, TEST_BLOCK_SHIFT, 1, TEST_START_LBA));
    
	while (blkNum--)
	{
		random_buffer(wrBuf, TEST_BLOCK_SIZE);
		CU_ASSERT(index_update_block(hnd, wrBuf, TEST_BLOCK_SIZE, 0, vbn) == TEST_BLOCK_SIZE);
		CU_ASSERT(index_read_block(hnd, rdBuf, TEST_BLOCK_SIZE, 0, vbn) == TEST_BLOCK_SIZE);
		CU_ASSERT(memcmp(rdBuf, wrBuf, TEST_BLOCK_SIZE) == 0);
		vbn++;
	}

    CU_ASSERT(block_close(hnd) == 0);

    
    CU_ASSERT(block_open(&hnd, TEST_BLOCK_FILE, TEST_START_LBA) == 0);
    CU_ASSERT(block_close(hnd) == 0);
}


int add_block_test_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_block_suite", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test block read write", test_block_rw))
    {
       return -2;
    }

    return 0;
}


