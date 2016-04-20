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

void test_cr_extent_1(void)
{
    index_extent_t ext;
    uint8_t ext_pair[32];
    uint32_t ext_pair_size;

    ext.addr = 0;
    ext.len = 0;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 0);
    
    ext.addr = 0;
    ext.len = 1;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 3);
    CU_ASSERT(ext_pair[0] == 1);
    CU_ASSERT(ext_pair[1] == 0);
    CU_ASSERT(ext_pair[2] == 1);
    
    ext.addr = 0x55;
    ext.len = 0xAA;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 3);
    CU_ASSERT(ext_pair[0] == 1);
    CU_ASSERT(ext_pair[1] == 0x55);
    CU_ASSERT(ext_pair[2] == 0xAA);
    
    ext.addr = 0x6655;
    ext.len = 0x8877;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 5);
    CU_ASSERT(ext_pair[0] == 2);
    CU_ASSERT(ext_pair[1] == 0x55);
    CU_ASSERT(ext_pair[2] == 0x66);
    CU_ASSERT(ext_pair[3] == 0x77);
    CU_ASSERT(ext_pair[4] == 0x88);
    
    ext.addr = 0x221100;
    ext.len = 0xffeeddccbbaa99;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 11);
    CU_ASSERT(ext_pair[0] == 3);
    CU_ASSERT(ext_pair[1] == 0x00);
    CU_ASSERT(ext_pair[2] == 0x11);
    CU_ASSERT(ext_pair[3] == 0x22);
    CU_ASSERT(ext_pair[4] == 0x99);
    CU_ASSERT(ext_pair[5] == 0xaa);
    CU_ASSERT(ext_pair[6] == 0xbb);
    CU_ASSERT(ext_pair[7] == 0xcc);
    CU_ASSERT(ext_pair[8] == 0xdd);
    CU_ASSERT(ext_pair[9] == 0xee);
    CU_ASSERT(ext_pair[10] == 0xff);


    ext.addr = 0x7766554433221100;
    ext.len = 0xffeeddccbbaa9988;
    ext_pair_size = os_extent_to_extent_pair(&ext, ext_pair);
    CU_ASSERT(ext_pair_size == 17);
    CU_ASSERT(ext_pair[0] == 8);
    CU_ASSERT(ext_pair[1] == 0x00);
    CU_ASSERT(ext_pair[2] == 0x11);
    CU_ASSERT(ext_pair[3] == 0x22);
    CU_ASSERT(ext_pair[4] == 0x33);
    CU_ASSERT(ext_pair[5] == 0x44);
    CU_ASSERT(ext_pair[6] == 0x55);
    CU_ASSERT(ext_pair[7] == 0x66);
    CU_ASSERT(ext_pair[8] == 0x77);
    CU_ASSERT(ext_pair[9] == 0x88);
    CU_ASSERT(ext_pair[10] == 0x99);
    CU_ASSERT(ext_pair[11] == 0xaa);
    CU_ASSERT(ext_pair[12] == 0xbb);
    CU_ASSERT(ext_pair[13] == 0xcc);
    CU_ASSERT(ext_pair[14] == 0xdd);
    CU_ASSERT(ext_pair[15] == 0xee);
    CU_ASSERT(ext_pair[16] == 0xff);
    
}

void test_cr_extent_2(void)
{
    index_extent_t ext;
    uint8_t ext_pair[32];

    ext_pair[0] = 1;
    ext_pair[1] = 0;
    ext_pair[2] = 1;
    ext.len = os_extent_pair_to_extent(ext_pair, 2, &ext.addr); // invalid extent pair
    CU_ASSERT(ext.len == 0);
    ext.len = os_extent_pair_to_extent(ext_pair, 3, &ext.addr);
    CU_ASSERT(ext.addr == 0);
    CU_ASSERT(ext.len == 1);
    
    ext_pair[0] = 1;
    ext_pair[1] = 0x55;
    ext_pair[2] = 0xAA;
    ext.len = os_extent_pair_to_extent(ext_pair, 3, &ext.addr);
    CU_ASSERT(ext.addr == 0x55);
    CU_ASSERT(ext.len == 0xAA);
    
    ext_pair[0] = 2;
    ext_pair[1] = 0x55;
    ext_pair[2] = 0x66;
    ext_pair[3] = 0x77;
    ext_pair[4] = 0x88;
    ext.len = os_extent_pair_to_extent(ext_pair, 5, &ext.addr);
    CU_ASSERT(ext.addr == 0x6655);
    CU_ASSERT(ext.len == 0x8877);
    
    ext_pair[0] = 3;
    ext_pair[1] = 0x00;
    ext_pair[2] = 0x11;
    ext_pair[3] = 0x22;
    ext_pair[4] = 0x99;
    ext_pair[5] = 0xaa;
    ext_pair[6] = 0xbb;
    ext_pair[7] = 0xcc;
    ext_pair[8] = 0xdd;
    ext_pair[9] = 0xee;
    ext_pair[10] = 0xff;
    ext.len = os_extent_pair_to_extent(ext_pair, 11, &ext.addr);
    CU_ASSERT(ext.addr == 0x221100);
    CU_ASSERT(ext.len == 0xffeeddccbbaa99);

    ext_pair[0] = 8;
    ext_pair[1] = 0x00;
    ext_pair[2] = 0x11;
    ext_pair[3] = 0x22;
    ext_pair[4] = 0x33;
    ext_pair[5] = 0x44;
    ext_pair[6] = 0x55;
    ext_pair[7] = 0x66;
    ext_pair[8] = 0x77;
    ext_pair[9] = 0x88;
    ext_pair[10] = 0x99;
    ext_pair[11] = 0xaa;
    ext_pair[12] = 0xbb;
    ext_pair[13] = 0xcc;
    ext_pair[14] = 0xdd;
    ext_pair[15] = 0xee;
    ext_pair[16] = 0xff;
    ext.len = os_extent_pair_to_extent(ext_pair, 17, &ext.addr);
    CU_ASSERT(ext.addr == 0x7766554433221100);
    CU_ASSERT(ext.len == 0xffeeddccbbaa9988);
}

void test_cr_extent_3(void)
{
    uint8_t addr1[32];
    uint8_t addr2[32];
    uint8_t len1[32];
    uint8_t len2[32];
    uint16_t addr1_size, addr2_size, len1_size, len2_size;

    addr1_size = os_u64_to_bstr(100, addr1);
    len1_size = os_u64_to_bstr(50, len1);
    
    addr2_size = os_u64_to_bstr(100, addr2);
    len2_size = os_u64_to_bstr(50, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(100, addr2);
    len2_size = os_u64_to_bstr(1, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(149, addr2);
    len2_size = os_u64_to_bstr(1, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(120, addr2);
    len2_size = os_u64_to_bstr(10, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(149, addr2);
    len2_size = os_u64_to_bstr(2, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(99, addr2);
    len2_size = os_u64_to_bstr(2, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 0);

    addr2_size = os_u64_to_bstr(150, addr2);
    len2_size = os_u64_to_bstr(1, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == -1);

    addr2_size = os_u64_to_bstr(151, addr2);
    len2_size = os_u64_to_bstr(10, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == -1);

    addr2_size = os_u64_to_bstr(80, addr2);
    len2_size = os_u64_to_bstr(10, len2);
    CU_ASSERT(os_collate_extent(addr1, addr1_size, len1, len1_size, addr2, addr2_size, len2, len2_size) == 1);
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
       return -4;
    }

    if (NULL == CU_add_test(pSuite, "test collate extent 1", test_cr_extent_1))
    {
       return -5;
    }

    if (NULL == CU_add_test(pSuite, "test collate extent 2", test_cr_extent_2))
    {
       return -6;
    }

    if (NULL == CU_add_test(pSuite, "test collate extent 3", test_cr_extent_3))
    {
       return -7;
    }

    return 0;
}

