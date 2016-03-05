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
File Name: TEST_FILE_BITMAP.C
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
#include "os_file_if.h"
#include "index_bitmap.h"
#include "os_log.h"

#define TEST_RW_SHIFT 9
#define TEST_RW_SIZE  ((1 << TEST_RW_SHIFT) * 2)
#define TEST_START_LBA    1
#define TEST_START_BYTE    (TEST_START_LBA << BYTES_PER_SECTOR_SHIFT)
#define TEST_RW_FILE  "Test.dat"


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

void test_file_bitmap(void)
{
    void *f = NULL;
    BITMAP_HANDLE *h = NULL;
	int32_t errCnt = 0;
	int32_t rwNum = 100;
    uint64_t i = 0;
    uint64_t total0Bits = 0;
    int32_t ret = 0;
    
	srand((unsigned)time(NULL));
	
	while (rwNum--)
	{
        CU_ASSERT(os_file_open_or_create(&f, TEST_RW_FILE) == 0);

		CU_ASSERT(os_file_resize(f, TEST_RW_SIZE + TEST_START_BYTE) == 0);

        CU_ASSERT(bitmap_init(&h, f, TEST_START_LBA, TEST_RW_SIZE >> BYTES_PER_SECTOR_SHIFT, 10000) == 0);
		
		for (i = 0; i < TEST_RW_SIZE * 8; i++)
		{
            ret = bitmap_check_bit(h, i);
			if (ret)
			{
				CU_ASSERT(bitmap_set_nbits(h, i, 1, 0) == 1);
				CU_ASSERT(bitmap_check_bit(h, i) == 0);
			}
			else
			{
				CU_ASSERT(bitmap_set_nbits(h, i, 1, 1) == 1);
				CU_ASSERT(bitmap_check_bit(h, i) != 0);
			}
		}

        CU_ASSERT(bitmap_destroy(h) == 0);

		CU_ASSERT(os_file_close(f) == 0);
	}
}



int add_file_bitmap_test_suite(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_block_suite", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test file bitmap", test_file_bitmap))
    {
       return -2;
    }

    return 0;
}


