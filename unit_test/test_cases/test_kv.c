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

typedef struct test_kv_pair
{
    char *key;
    char *value;
} test_kv_pair_t;

test_kv_pair_t test_kv_pairs[]
= {
    {"a", TEST_V1},
    {"b", TEST_V1},
    {"c", TEST_V1},
    {"d", TEST_V1},
    {"e", TEST_V1},
    {"f", TEST_V1},
    {"g", TEST_V1},
    {"h", TEST_V1},
    {"i", TEST_V1},
    {"j", TEST_V1},
    {"k", TEST_V1},
    {"l", TEST_V1},
    {"m", TEST_V1},
    {"n", TEST_V1},
    {"o", TEST_V1},
    {"p", TEST_V1},
    {"q", TEST_V1},
    {"r", TEST_V1},
    {"s", TEST_V1},
    {"t", TEST_V1},
    {"u", TEST_V1},
    {"v", TEST_V1},
    {"w", TEST_V1},
    {"x", TEST_V1},
    {"y", TEST_V1},
    {"z", TEST_V1},

    {"aj", TEST_V2},
    {"bj", TEST_V2},
    {"cj", TEST_V2},
    {"dj", TEST_V2},
    {"ej", TEST_V2},
    {"fj", TEST_V2},
    {"gj", TEST_V2},
    {"hj", TEST_V2},
    {"ij", TEST_V2},
    {"jj", TEST_V2},
    {"kj", TEST_V2},
    {"lj", TEST_V2},
    {"mj", TEST_V2},
    {"nj", TEST_V2},
    {"oj", TEST_V2},
    {"pj", TEST_V2},
    {"qj", TEST_V2},
    {"rj", TEST_V2},
    {"sj", TEST_V2},
    {"tj", TEST_V2},
    {"uj", TEST_V2},
    {"vj", TEST_V2},
    {"wj", TEST_V2},
    {"xj", TEST_V2},
    {"yj", TEST_V2},
    {"zj", TEST_V2},

    {"aerg", TEST_V3},
    {"berg", TEST_V3},
    {"cerg", TEST_V3},
    {"derg", TEST_V3},
    {"eerg", TEST_V3},
    {"ferg", TEST_V3},
    {"gerg", TEST_V3},
    {"herg", TEST_V3},
    {"ierg", TEST_V3},
    {"jerg", TEST_V3},
    {"kerg", TEST_V3},
    {"lerg", TEST_V3},
    {"merg", TEST_V3},
    {"nerg", TEST_V3},
    {"oerg", TEST_V3},
    {"perg", TEST_V3},
    {"qerg", TEST_V3},
    {"rerg", TEST_V3},
    {"serg", TEST_V3},
    {"terg", TEST_V3},
    {"uerg", TEST_V3},
    {"verg", TEST_V3},
    {"werg", TEST_V3},
    {"xerg", TEST_V3},
    {"yerg", TEST_V3},
    {"zerg", TEST_V3},

    {"afgjdryjt", TEST_V1},
    {"bfgjdryjt", TEST_V1},
    {"cfgjdryjt", TEST_V1},
    {"dfgjdryjt", TEST_V1},
    {"efgjdryjt", TEST_V1},
    {"ffgjdryjt", TEST_V1},
    {"gfgjdryjt", TEST_V1},
    {"hfgjdryjt", TEST_V1},
    {"ifgjdryjt", TEST_V1},
    {"jfgjdryjt", TEST_V1},
    {"kfgjdryjt", TEST_V1},
    {"lfgjdryjt", TEST_V1},
    {"mfgjdryjt", TEST_V1},
    {"nfgjdryjt", TEST_V1},
    {"ofgjdryjt", TEST_V1},
    {"pfgjdryjt", TEST_V1},
    {"qfgjdryjt", TEST_V1},
    {"rfgjdryjt", TEST_V1},
    {"sfgjdryjt", TEST_V1},
    {"tfgjdryjt", TEST_V1},
    {"ufgjdryjt", TEST_V1},
    {"vfgjdryjt", TEST_V1},
    {"wfgjdryjt", TEST_V1},
    {"xfgjdryjt", TEST_V1},
    {"yfgjdryjt", TEST_V1},
    {"zfgjdryjt", TEST_V1},

    {"aerhrwhrweterwhgtrewh45w", TEST_V2},
    {"berhrwhrweterwhgtrewh45w", TEST_V2},
    {"cerhrwhrweterwhgtrewh45w", TEST_V2},
    {"derhrwhrweterwhgtrewh45w", TEST_V2},
    {"eerhrwhrweterwhgtrewh45w", TEST_V2},
    {"ferhrwhrweterwhgtrewh45w", TEST_V2},
    {"gerhrwhrweterwhgtrewh45w", TEST_V2},
    {"herhrwhrweterwhgtrewh45w", TEST_V2},
    {"ierhrwhrweterwhgtrewh45w", TEST_V2},
    {"jerhrwhrweterwhgtrewh45w", TEST_V2},
    {"kerhrwhrweterwhgtrewh45w", TEST_V2},
    {"lerhrwhrweterwhgtrewh45w", TEST_V2},
    {"merhrwhrweterwhgtrewh45w", TEST_V2},
    {"nerhrwhrweterwhgtrewh45w", TEST_V2},
    {"oerhrwhrweterwhgtrewh45w", TEST_V2},
    {"perhrwhrweterwhgtrewh45w", TEST_V2},
    {"qerhrwhrweterwhgtrewh45w", TEST_V2},
    {"rerhrwhrweterwhgtrewh45w", TEST_V2},
    {"serhrwhrweterwhgtrewh45w", TEST_V2},
    {"terhrwhrweterwhgtrewh45w", TEST_V2},
    {"uerhrwhrweterwhgtrewh45w", TEST_V2},
    {"verhrwhrweterwhgtrewh45w", TEST_V2},
    {"werhrwhrweterwhgtrewh45w", TEST_V2},
    {"xerhrwhrweterwhgtrewh45w", TEST_V2},
    {"yerhrwhrweterwhgtrewh45w", TEST_V2},
    {"zerhrwhrweterwhgtrewh45w", TEST_V2},
        

    {"asdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"bsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"csdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"dsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"esdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"fsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"gsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"hsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"isdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"jsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"ksdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"lsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"msdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"nsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"osdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"psdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"qsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"rsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"ssdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"tsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"usdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"vsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"wsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"xsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"ysdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},
    {"zsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3}
};


void test_kv_1(void)
{
    INDEX_HANDLE *index;
    OBJECT_HANDLE *obj;
    int32_t i;
    
    // create index and object, insert key
    CU_ASSERT(0 == index_create("index0", 1000, 0, &index));
    CU_ASSERT(0 == index_create_object(index, 500, FLAG_TABLE | CR_ANSI_STRING | (CR_ANSI_STRING << 4), &obj));

    for (i = 0; i < ArraySize(test_kv_pairs); i++)
    {
        CU_ASSERT(0 == index_insert_key(obj, test_kv_pairs[i].key, strlen(test_kv_pairs[i].key),
            test_kv_pairs[i].value, strlen(test_kv_pairs[i].value)));
    }

    CU_ASSERT(0 == index_close_object(obj));
    CU_ASSERT(0 == index_close(index));

    // open index and object, remove key
    CU_ASSERT(0 == index_open("index0", 0, &index));
    CU_ASSERT(0 == index_open_object(index, 500, &obj));

    for (i = 0; i < ArraySize(test_kv_pairs); i++)
    {
        CU_ASSERT(0 == index_remove_key(obj, test_kv_pairs[i].key, strlen(test_kv_pairs[i].key)));
    }
    
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

