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
#include "os_file_io.h"
#include "index_bitmap.h"

#define TEST_RW_NUM   1
#define TEST_RW_SHIFT 9
#define TEST_RW_SIZE  ((1 << TEST_RW_SHIFT) * 2)
#define TEST_START_LBA    1
#define TEST_START_BYTE    (TEST_START_LBA << BYTES_PER_SECTOR_SHIFT)
#define TEST_RW_FILE  "Test.dat"

extern int32_t bitmap_check_bit(void *f, uint64_t position);

void RandomBuffer(uint8_t *cache, uint32_t size)
{
	while (size--)
	{
		*cache++ = (uint8_t)rand();
	}
}

void main(void)
{
    void *f = NULL;
    void *h = NULL;
	int32_t errCnt = 0;
	int32_t rwNum = TEST_RW_NUM;
	uint8_t cache[TEST_RW_SIZE];
    uint64_t i = 0;
    uint64_t total0Bits = 0;
    int32_t ret = 0;
    
	srand((unsigned)time(NULL));
	
	while (rwNum--)
	{
    	OS_PRINT("Start test File %s read and write now. rwNum = %d\n", TEST_RW_FILE, rwNum);

        if (os_file_open_or_create(&f, TEST_RW_FILE) < 0)
        {
    		PrintErrorToScreen("Open or create File %s failed(%ld)\n", TEST_RW_FILE, f);
    		errCnt++;
            break;
        }

		if (os_file_resize(f, TEST_RW_SIZE + TEST_START_BYTE))
		{
    		PrintErrorToScreen("Resize File %s failed\n", TEST_RW_FILE);
    		errCnt++;
            break;
		}

        if (bitmap_init(&h, f, TEST_START_LBA, TEST_RW_SIZE >> BYTES_PER_SECTOR_SHIFT, 10000) < 0)
        {
    		PrintErrorToScreen("init %s failed\n", TEST_RW_FILE);
    		errCnt++;
            break;
        }
		
		for (i = 0; i < TEST_RW_SIZE * 8; i++)
		{
            ret = bitmap_check_bit(h, i);
            if (ret < 0)
            {
                PrintErrorToScreen("Clear bit %ld of File %s failed\n", i, TEST_RW_FILE);
                errCnt++;
                break;
            }
            
			if (ret)
			{
				if (bitmap_set_nbits(h, i, 1, 0) < 0)
				{
		    		PrintErrorToScreen("Clear bit %ld of File %s failed\n", i, TEST_RW_FILE);
		    		errCnt++;
				}
                
				if (bitmap_check_bit(h, i) > 0)
				{
		    		PrintErrorToScreen("Bit %ld of File clear failed\n", i);
		    		errCnt++;
				}
			}
			else
			{
				if (bitmap_set_nbits(h, i, 1, 1) < 0)
				{
		    		PrintErrorToScreen("Set bit %ld of File %s failed\n", i, TEST_RW_FILE);
		    		errCnt++;
				}
                
				if (bitmap_check_bit(h, i) == 0)
				{
		    		PrintErrorToScreen("Bit %ld of File set failed\n", i);
		    		errCnt++;
				}
			}
            
    		OS_PRINT("%ld\n", i);
		}

#if 0
		RandomBuffer(cache, TEST_RW_SIZE);
		
		if (os_file_pwrite(f, cache, TEST_RW_SIZE, 0) != TEST_RW_SIZE)
		{
    		PrintErrorToScreen("Write File %s failed\n", TEST_RW_FILE);
    		errCnt++;
		}
		
		total0Bits = 0;
		for (i = 0; i < TEST_RW_SIZE * 8; i++)
		{
			int64_t ret1, ret2;
			
			ret1 = bitmap_get_free_bits(h, TEST_RW_SIZE * 8, i, 1, &ret1);
			ret2 = FileBitmapGetFirst0Bit2(h, TEST_RW_SIZE * 8, i);
			
			if (ret1 != ret2)
			{
	    		PrintErrorToScreen("Get first 0 bit of file %s failed\n", TEST_RW_FILE);
	    		errCnt++;
			}
			else
			{
				if (ret1 > 0)
				{
					i = ret1;
				}
				else
				{
					break;
				}
				total0Bits++;
			}
		}
#endif

        (void)bitmap_destroy(h);

		if (os_file_close(f))
		{
    		PrintErrorToScreen("Close File %s failed\n", TEST_RW_FILE);
    		errCnt++;
		}

	    OS_PRINT("Get file %s total %ld 0 bits\n", TEST_RW_FILE, total0Bits);
    	OS_PRINT("Test File %s read and write finished\nerrCnt = %d\n", TEST_RW_FILE, errCnt);
	}
    
	system("pause");
} /* End of main */



