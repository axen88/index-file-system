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
File Name: INDEX_TOOLS_PERFORMACE.C
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

#define TEST_KEY_LEN   8
#define TEST_VALUE_LEN 20

int32_t test_insert_key_performance(char *index_name, uint64_t start_lba,
    uint64_t objid, uint64_t keys_num, net_para_t *net)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;
    uint64_t key = 0;
    uint8_t c[TEST_VALUE_LEN];
    uint64_t ullTime = 0;

    ASSERT(NULL != index_name);
    ASSERT(0 != strlen(index_name));
    ASSERT(0 != objid);

    ret = ofs_open_container(index_name, start_lba, &ct);
    if (ret < 0)
    {
        OS_PRINT(net, "Open ct failed. name(%s)  start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = ofs_create_object(ct, objid, FLAG_TABLE | CR_ANSI_STRING | (CR_ANSI_STRING << 4), &obj);
    if (ret < 0)
    {
        OS_PRINT(net, "Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    memset(c, 0x88, sizeof(c));

    OS_PRINT(net, "Start insert key. objid(%lld) total(%lld)\n", objid, keys_num);
    ullTime = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_insert_key(obj, &key, TEST_KEY_LEN,
            c, TEST_VALUE_LEN);
        if (0 > ret)
        {
            OS_PRINT(net, "Insert key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished insert key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - ullTime);

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return 0;
}

int32_t test_remove_key_performance(char *index_name, uint64_t start_lba,
    uint64_t objid, uint64_t keys_num, net_para_t *net)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;
    uint64_t key = 0;
    uint64_t ullTime = 0;

    ASSERT(NULL != index_name);
    ASSERT(0 != strlen(index_name));
    ASSERT(0 != objid);

    ret = ofs_open_container(index_name, start_lba, &ct);
    if (ret < 0)
    {
        OS_PRINT(net, "Open ct failed. name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = ofs_open_object(ct, objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(net, "Open tree failed. objid(%lld) ret(%d)\n", objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    OS_PRINT(net, "Start remove key. objid(%lld) total(%lld)\n", objid, keys_num);
    ullTime = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_remove_key(obj, &key, TEST_KEY_LEN);
        if (0 > ret)
        {
            OS_PRINT(net, "Remove key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished remove key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - ullTime);

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return 0;
}

void *test_performance_thread(void *para)
{
    ifs_tools_para_t *tmp_para = para;
    char obj_name[OBJ_NAME_MAX_SIZE];

    OS_RWLOCK_WRLOCK(&tmp_para->rwlock);
    OS_SNPRINTF(obj_name, OBJ_NAME_MAX_SIZE, "%lld%d",
        tmp_para->objid, tmp_para->no++);
    tmp_para->threads_cnt++;
    OS_RWLOCK_WRUNLOCK(&tmp_para->rwlock);

    if (tmp_para->insert)
    {
        (void)test_insert_key_performance(tmp_para->index_name,
            tmp_para->start_lba, tmp_para->objid, tmp_para->keys_num, tmp_para->net);
    }
    else
    {
        (void)test_remove_key_performance(tmp_para->index_name,
            tmp_para->start_lba, tmp_para->objid, tmp_para->keys_num, tmp_para->net);
    }

    OS_RWLOCK_WRLOCK(&tmp_para->rwlock);
    tmp_para->threads_cnt--;
    OS_RWLOCK_WRUNLOCK(&tmp_para->rwlock);

    OSThreadExit();
    
    return NULL;
}

int32_t test_performance(ifs_tools_para_t *para,
    bool_t v_bInsert)
{
    void *threads_group = NULL;

    para->insert = v_bInsert;
    para->threads_cnt = 0;
    para->no = 0;

    threads_group = threads_group_create(para->threads_num,
        test_performance_thread, para, "perf");
    if (NULL == threads_group)
    {
        OS_PRINT(para->net, "Create threads group failed. num(%d)\n",
            para->threads_num);
        return -1;
    }

    if ((int32_t)para->threads_num != threads_group_get_real_num(threads_group))
    {
        OS_PRINT(para->net, "Create threads group failed. expect(%d) real(%d)\n",
            para->threads_num, threads_group_get_real_num(threads_group));
        //threads_group_destroy(threads_group, 1, (OS_U64)0);
        //return -2;
    }

    while (0 != para->threads_cnt)
    {
        OS_SLEEP_SECOND(1);
    }

    threads_group_destroy(threads_group, 1, (uint64_t)0);

    return 0;
}

int do_performance_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if ((0 == strlen(para->index_name))
        || OBJID_IS_INVALID(para->objid))
    {
        OS_PRINT(net, "invalid ct name(%s) or objid(%lld).\n", para->index_name, para->objid);
        OS_FREE(para);
        return -2;
    }
    
    if (0 == para->threads_num)
    {
        para->threads_num = 1;
    }

    (void)test_performance(para, B_TRUE);
    (void)test_performance(para, B_FALSE);

    OS_FREE(para);

    return 0;
}




