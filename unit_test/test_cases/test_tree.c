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

typedef struct test_kv_pair
{
    char *key;
    char *value;
} test_kv_pair_t;

test_kv_pair_t test_kv_pairs1[]
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
    {"zsdgkjheaoifewjfopiehrjseonjsldkdgjropie", TEST_V3},

    {"adlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"bdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"cdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"ddlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"edlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"fdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"gdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"hdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"idlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"jdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"kdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"ldlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"mdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"ndlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"odlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"pdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"qdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"rdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"sdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"tdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"udlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"vdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"wdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"xdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"ydlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL},
    {"zdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore", NULL}
};

test_kv_pair_t test_kv_pairs2[]
= {
    {"a2", TEST_V1},
    {"b2", TEST_V1},
    {"c2", TEST_V1},
    {"d2", TEST_V1},
    {"e2", TEST_V1},
    {"f2", TEST_V1},
    {"g2", TEST_V1},
    {"h2", TEST_V1},
    {"i2", TEST_V1},
    {"j2", TEST_V1},
    {"k2", TEST_V1},
    {"l2", TEST_V1},
    {"m2", TEST_V1},
    {"n2", TEST_V1},
    {"o2", TEST_V1},
    {"p2", TEST_V1},
    {"q2", TEST_V1},
    {"r2", TEST_V1},
    {"s2", TEST_V1},
    {"t2", TEST_V1},
    {"u2", TEST_V1},
    {"v2", TEST_V1},
    {"w2", TEST_V1},
    {"x2", TEST_V1},
    {"y2", TEST_V1},
    {"z2", TEST_V1},

    {"aj2", TEST_V2},
    {"bj2", TEST_V2},
    {"cj2", TEST_V2},
    {"dj2", TEST_V2},
    {"ej2", TEST_V2},
    {"fj2", TEST_V2},
    {"gj2", TEST_V2},
    {"hj2", TEST_V2},
    {"ij2", TEST_V2},
    {"jj2", TEST_V2},
    {"kj2", TEST_V2},
    {"lj2", TEST_V2},
    {"mj2", TEST_V2},
    {"nj2", TEST_V2},
    {"oj2", TEST_V2},
    {"pj2", TEST_V2},
    {"qj2", TEST_V2},
    {"rj2", TEST_V2},
    {"sj2", TEST_V2},
    {"tj2", TEST_V2},
    {"uj2", TEST_V2},
    {"vj2", TEST_V2},
    {"wj2", TEST_V2},
    {"xj2", TEST_V2},
    {"yj2", TEST_V2},
    {"zj2", TEST_V2},

    {"aerg2", TEST_V3},
    {"berg2", TEST_V3},
    {"cerg2", TEST_V3},
    {"derg2", TEST_V3},
    {"eerg2", TEST_V3},
    {"ferg2", TEST_V3},
    {"gerg2", TEST_V3},
    {"herg2", TEST_V3},
    {"ierg2", TEST_V3},
    {"jerg2", TEST_V3},
    {"kerg2", TEST_V3},
    {"lerg2", TEST_V3},
    {"merg2", TEST_V3},
    {"nerg2", TEST_V3},
    {"oerg2", TEST_V3},
    {"perg2", TEST_V3},
    {"qerg2", TEST_V3},
    {"rerg2", TEST_V3},
    {"serg2", TEST_V3},
    {"terg2", TEST_V3},
    {"uerg2", TEST_V3},
    {"verg2", TEST_V3},
    {"werg2", TEST_V3},
    {"xerg2", TEST_V3},
    {"yerg2", TEST_V3},
    {"zerg2", TEST_V3},

    {"afgjdryjt2", TEST_V1},
    {"bfgjdryjt2", TEST_V1},
    {"cfgjdryjt2", TEST_V1},
    {"dfgjdryjt2", TEST_V1},
    {"efgjdryjt2", TEST_V1},
    {"ffgjdryjt2", TEST_V1},
    {"gfgjdryjt2", TEST_V1},
    {"hfgjdryjt2", TEST_V1},
    {"ifgjdryjt2", TEST_V1},
    {"jfgjdryjt2", TEST_V1},
    {"kfgjdryjt2", TEST_V1},
    {"lfgjdryjt2", TEST_V1},
    {"mfgjdryjt2", TEST_V1},
    {"nfgjdryjt2", TEST_V1},
    {"ofgjdryjt2", TEST_V1},
    {"pfgjdryjt2", TEST_V1},
    {"qfgjdryjt2", TEST_V1},
    {"rfgjdryjt2", TEST_V1},
    {"sfgjdryjt2", TEST_V1},
    {"tfgjdryjt2", TEST_V1},
    {"ufgjdryjt2", TEST_V1},
    {"vfgjdryjt2", TEST_V1},
    {"wfgjdryjt2", TEST_V1},
    {"xfgjdryjt2", TEST_V1},
    {"yfgjdryjt2", TEST_V1},
    {"zfgjdryjt2", TEST_V1},

    {"aerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"berhrwhrweterwhgtrewh45w2", TEST_V2},
    {"cerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"derhrwhrweterwhgtrewh45w2", TEST_V2},
    {"eerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"ferhrwhrweterwhgtrewh45w2", TEST_V2},
    {"gerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"herhrwhrweterwhgtrewh45w2", TEST_V2},
    {"ierhrwhrweterwhgtrewh45w2", TEST_V2},
    {"jerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"kerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"lerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"merhrwhrweterwhgtrewh45w2", TEST_V2},
    {"nerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"oerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"perhrwhrweterwhgtrewh45w2", TEST_V2},
    {"qerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"rerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"serhrwhrweterwhgtrewh45w2", TEST_V2},
    {"terhrwhrweterwhgtrewh45w2", TEST_V2},
    {"uerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"verhrwhrweterwhgtrewh45w2", TEST_V2},
    {"werhrwhrweterwhgtrewh45w2", TEST_V2},
    {"xerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"yerhrwhrweterwhgtrewh45w2", TEST_V2},
    {"zerhrwhrweterwhgtrewh45w2", TEST_V2},
        
    {"asdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"bsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"csdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"dsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"esdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"fsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"gsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"hsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"isdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"jsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"ksdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"lsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"msdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"nsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"osdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"psdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"qsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"rsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"ssdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"tsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"usdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"vsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"wsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"xsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"ysdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},
    {"zsdgkjheaoifewjfopiehrjseonjsldkdgjropie2", TEST_V3},

    {"adlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"bdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"cdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"ddlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"edlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"fdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"gdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"hdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"idlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"jdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"kdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"ldlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"mdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"ndlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"odlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"pdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"qdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"rdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"sdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"tdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"udlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"vdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"wdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"xdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"ydlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL},
    {"zdlckbjeoairjgtlaeksjgseoiugoireejrhgljehore2", NULL}
};


void test_kv_1(void)
{
    container_handle_t *ct;
    object_handle_t *obj;
    int32_t i;
    
    // create ct and object, insert key
    CU_ASSERT(0 == ofs_create_container("kv", 1000, &ct));
    CU_ASSERT(0 == ofs_create_object(ct, 500, FLAG_TABLE | CR_ANSI_STRING | (CR_ANSI_STRING << 4), &obj));

    for (i = 0; i < ArraySize(test_kv_pairs1); i++)
    {
        if (test_kv_pairs1[i].value != NULL)
        {
            CU_ASSERT(0 == index_insert_key(obj, test_kv_pairs1[i].key, strlen(test_kv_pairs1[i].key),
                test_kv_pairs1[i].value, strlen(test_kv_pairs1[i].value)));
        }
        else
        {
            CU_ASSERT(0 == index_insert_key(obj, test_kv_pairs1[i].key, strlen(test_kv_pairs1[i].key), NULL, 0));
        }
    }

    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));

    // open ct and object, insert key
    CU_ASSERT(0 == ofs_open_container("kv", &ct));
    CU_ASSERT(0 == ofs_open_object(ct, 500, &obj));

    for (i = 0; i < ArraySize(test_kv_pairs2); i++)
    {
        if (test_kv_pairs2[i].value != NULL)
        {
            CU_ASSERT(0 == index_insert_key(obj, test_kv_pairs2[i].key, strlen(test_kv_pairs2[i].key),
                test_kv_pairs2[i].value, strlen(test_kv_pairs2[i].value)));
        }
        else
        {
            CU_ASSERT(0 == index_insert_key(obj, test_kv_pairs2[i].key, strlen(test_kv_pairs2[i].key), NULL, 0));
        }
    }
    
    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));

    // open ct and object, remove key
    CU_ASSERT(0 == ofs_open_container("kv", &ct));
    CU_ASSERT(0 == ofs_open_object(ct, 500, &obj));

    for (i = 0; i < ArraySize(test_kv_pairs1); i++)
    {
        CU_ASSERT(0 == index_remove_key(obj, test_kv_pairs1[i].key, strlen(test_kv_pairs1[i].key)));
    }
    
    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));
    
    // open ct and object, remove key
    CU_ASSERT(0 == ofs_open_container("kv", &ct));
    CU_ASSERT(0 == ofs_open_object(ct, 500, &obj));

    for (i = 0; i < ArraySize(test_kv_pairs2); i++)
    {
        CU_ASSERT(0 == index_remove_key(obj, test_kv_pairs2[i].key, strlen(test_kv_pairs2[i].key)));
    }
    
    CU_ASSERT(0 == ofs_close_object(obj));
    CU_ASSERT(0 == ofs_close_container(ct));
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

