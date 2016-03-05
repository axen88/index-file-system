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


#include <string.h>
#include <time.h>
#include <stdio.h>

#include "os_types.h"
#include "index_block_if.h"

#define TEST_REPEAT_TIMES  10
#define TEST_BLOCK_FILE "Test.dat"
#define TEST_TOTAL_SECTORS 111
#define TEST_BLOCK_SHIFT 12
#define TEST_BLOCK_SIZE (1 << TEST_BLOCK_SHIFT)
#define TEST_BLOCK_NUM 1000

void RandomBuffer(uint8_t *buf, uint32_t size)
{
	while (size--)
	{
		*buf++ = (uint8_t)rand();
	}
}

void main(void)
{
    void *f;
	int32_t errCnt = 0;
	int32_t rwNum = TEST_REPEAT_TIMES;
    
    if (block_create(&f, TEST_BLOCK_FILE, TEST_TOTAL_SECTORS, TEST_BLOCK_SHIFT, 1) < 0)
    {
		PrintErrorToScreen("Create File %s failed(%ld)\n", TEST_BLOCK_FILE, f);
		errCnt++;
		system("pause");
		exit(-1);
    }
    
    (void)block_close(f);
	srand((unsigned)time(NULL));
    
	while (rwNum--)
	{
		int64_t vbn;
		uint64_t blkNum;
		uint8_t wrBuf[TEST_BLOCK_SIZE];
		uint8_t rdBuf[TEST_BLOCK_SIZE];
		
    	OS_PRINT("Start test File %s read and write now. rwNum = %d\n", TEST_BLOCK_FILE, rwNum);

		if (block_open(&f, TEST_BLOCK_FILE) < 0)
		{
    		PrintErrorToScreen("Open File %s failed(%ld)\n", TEST_BLOCK_FILE, f);
    		errCnt++;
			break;
		}
		
		blkNum = TEST_BLOCK_NUM;
		while (blkNum--)
		{
			RandomBuffer(wrBuf, TEST_BLOCK_SIZE);
			
			vbn = index_write_block(f, wrBuf, 0, TEST_BLOCK_SIZE);
			if (vbn < 0)
			{
	    		PrintErrorToScreen("Write File %s failed(%ld)\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (index_read_block(f, rdBuf, TEST_BLOCK_SIZE, 0, vbn) != TEST_BLOCK_SIZE)
			{
	    		PrintErrorToScreen("Read File %s failed, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (memcmp(rdBuf, wrBuf, TEST_BLOCK_SIZE))
			{
	    		PrintErrorToScreen("File %s content not match, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (index_update_block(f, wrBuf, TEST_BLOCK_SIZE, 0, vbn) != TEST_BLOCK_SIZE)
			{
	    		PrintErrorToScreen("Update File %s failed(%ld)\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (index_read_block(f, rdBuf, TEST_BLOCK_SIZE, 0, vbn) != TEST_BLOCK_SIZE)
			{
	    		PrintErrorToScreen("Read File %s failed, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (memcmp(rdBuf, wrBuf, TEST_BLOCK_SIZE))
			{
	    		PrintErrorToScreen("File %s content failed, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (block_free(f, vbn, 1))
			{
	    		PrintErrorToScreen("Delete File %s content failed, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (index_read_block(f, rdBuf, TEST_BLOCK_SIZE, 0, vbn) == TEST_BLOCK_SIZE)
			{
	    		PrintErrorToScreen("File %s has content after delete, vbn = %d\n", TEST_BLOCK_FILE, vbn);
	    		errCnt++;
			}
			
			if (TEST_TOTAL_SECTORS-1 == vbn)
			{
				continue;
			}
		}

		if (block_close(f))
		{
    		PrintErrorToScreen("Close File %s failed\n", TEST_BLOCK_FILE);
    		errCnt++;
			break;
		}

    	OS_PRINT("Test File %s read and write finished\nerrCnt = %d\n", TEST_BLOCK_FILE, errCnt);
	}
	system("pause");
} /* End of main */



