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

void test_kv_1(void)
{
    INDEX_HANDLE *index;
    OBJECT_HANDLE *obj;
    
    CU_ASSERT(0 == index_create("index0", 1000, 0, &index));
    CU_ASSERT(0 == index_create_object(index, 1000, FLAG_TABLE | CR_ANSI_STRING | (CR_ANSI_STRING << 4), &obj));

    CU_ASSERT(0 == index_insert_key(obj, "a", strlen("a"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "aa", strlen("aa"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "ab", strlen("ab"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "ba", strlen("ba"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "bb", strlen("bb"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "aaa", strlen("aaa"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "aab", strlen("aab"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "abb", strlen("abb"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "bbb", strlen("bbb"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "bba", strlen("bba"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "baa", strlen("baa"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "cab", strlen("cab"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "gwege", strlen("gwege"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "reb", strlen("reb"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "stern", strlen("stern"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "rsnt", strlen("rsnt"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "wtntr", strlen("wtntr"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "nwetn", strlen("nwetn"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "qqern", strlen("qqern"), TEST_V3, strlen(TEST_V3)));

    CU_ASSERT(0 == index_insert_key(obj, "1", strlen("1"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "12", strlen("12"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "123", strlen("123"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "1234", strlen("1234"), TEST_V1, strlen(TEST_V1)));
    CU_ASSERT(0 == index_insert_key(obj, "12345", strlen("12345"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "123456", strlen("123456"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "1234567", strlen("1234567"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "12345678", strlen("12345678"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "123456789", strlen("123456789"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "1234567890", strlen("1234567890"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "11", strlen("11"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "112", strlen("112"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "113", strlen("113"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "114", strlen("114"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "115", strlen("115"), TEST_V2, strlen(TEST_V2)));
    CU_ASSERT(0 == index_insert_key(obj, "116", strlen("116"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "117", strlen("117"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "118", strlen("118"), TEST_V3, strlen(TEST_V3)));
    CU_ASSERT(0 == index_insert_key(obj, "119", strlen("119"), TEST_V3, strlen(TEST_V3)));


#if 0
    CU_ASSERT(0 == index_remove_key(obj, "a", strlen("a")));
    CU_ASSERT(0 == index_remove_key(obj, "aa", strlen("aa")));
    CU_ASSERT(0 == index_remove_key(obj, "ab", strlen("ab")));
    CU_ASSERT(0 == index_remove_key(obj, "ba", strlen("ba")));
    CU_ASSERT(0 == index_remove_key(obj, "bb", strlen("bb")));
    CU_ASSERT(0 == index_remove_key(obj, "aaa", strlen("aaa")));
    CU_ASSERT(0 == index_remove_key(obj, "aab", strlen("aab")));
    CU_ASSERT(0 == index_remove_key(obj, "abb", strlen("abb")));
    CU_ASSERT(0 == index_remove_key(obj, "bbb", strlen("bbb")));
    CU_ASSERT(0 == index_remove_key(obj, "bba", strlen("bba")));
    CU_ASSERT(0 == index_remove_key(obj, "baa", strlen("baa")));
    CU_ASSERT(0 == index_remove_key(obj, "cab", strlen("cab")));
    CU_ASSERT(0 == index_remove_key(obj, "gwege", strlen("gwege")));
    CU_ASSERT(0 == index_remove_key(obj, "reb", strlen("reb")));
    CU_ASSERT(0 == index_remove_key(obj, "stern", strlen("stern")));
    CU_ASSERT(0 == index_remove_key(obj, "rsnt", strlen("rsnt")));
    CU_ASSERT(0 == index_remove_key(obj, "wtntr", strlen("wtntr")));
    CU_ASSERT(0 == index_remove_key(obj, "nwetn", strlen("nwetn")));
    CU_ASSERT(0 == index_remove_key(obj, "qqern", strlen("qqern")));
#endif
    
    CU_ASSERT(0 == index_close_object(obj));
    CU_ASSERT(0 == index_close(index));
}

int add_kv_test_case(void)
{
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("test_kv_suit", init_suite, clean_suite);
    if (NULL == pSuite) {
       return -1;
    }
    
    if (NULL == CU_add_test(pSuite, "test kv 1", test_kv_1))
    {
       return -2;
    }

    return 0;
}

