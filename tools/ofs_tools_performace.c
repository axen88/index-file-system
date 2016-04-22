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

int32_t test_insert_key_performance(char *ct_name, uint64_t objid, uint64_t keys_num, net_para_t *net)
{
    int32_t ret;
    container_handle_t *ct;
    object_handle_t *obj;
    uint64_t key;
    uint8_t value[TEST_VALUE_LEN];
    uint64_t time;

    ret = ofs_open_container(ct_name, &ct);
    if (ret < 0)
    {
        OS_PRINT(net, "Open ct failed. name(%s) ret(%d)\n", ct_name, ret);
        return ret;
    }

    ret = ofs_create_object(ct, objid, FLAG_TABLE | CR_BINARY | (CR_ANSI_STRING << 4), &obj);
    if (ret < 0)
    {
        OS_PRINT(net, "Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    memset(value, 0x88, sizeof(value));

    OS_PRINT(net, "Start insert key. objid(%lld) total(%lld)\n", objid, keys_num);
    time = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_insert_key(obj, &key, TEST_KEY_LEN, value, TEST_VALUE_LEN);
        if (ret < 0)
        {
            OS_PRINT(net, "Insert key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished insert key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - time);

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return 0;
}

int32_t test_remove_key_performance(char *ct_name, uint64_t objid, uint64_t keys_num, net_para_t *net)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;
    uint64_t key = 0;
    uint64_t time = 0;

    ret = ofs_open_container(ct_name, &ct);
    if (ret < 0)
    {
        OS_PRINT(net, "Open ct failed. name(%s) ret(%d)\n", ct_name, ret);
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
    time = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_remove_key(obj, &key, TEST_KEY_LEN);
        if (ret < 0)
        {
            OS_PRINT(net, "Remove key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished remove key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - time);

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return 0;
}

void *test_performance_thread(void *para)
{
    ifs_tools_para_t *tmp_para = para;

    if (tmp_para->insert)
    {
        (void)test_insert_key_performance(tmp_para->ct_name, tmp_para->objid, tmp_para->keys_num, tmp_para->net);
    }
    else
    {
        (void)test_remove_key_performance(tmp_para->ct_name, tmp_para->objid, tmp_para->keys_num, tmp_para->net);
    }

    OS_RWLOCK_WRLOCK(&tmp_para->rwlock);
    tmp_para->threads_cnt--;
    OS_RWLOCK_WRUNLOCK(&tmp_para->rwlock);

    OS_THREAD_EXIT();
    
    return NULL;
}

int32_t test_performance(ifs_tools_para_t *para, bool_t insert)
{
    void *threads_group = NULL;
    uint32_t i;
    os_thread_t *tid;

    para->insert = insert;
    para->threads_cnt = 0;
    para->no = 0;

    tid = OS_MALLOC(sizeof(os_thread_t) * para->threads_num);
    if (tid == NULL)
    {
        return -1;
    }

    memset(tid, 0, sizeof(os_thread_t) * para->threads_num);

    for (i = 0; i < para->threads_num; i++)
    {
        tid[i] = thread_create(test_performance_thread, para, "perf");
        if (tid[i] == INVALID_TID)
        {
            OS_PRINT(para->net, "Create thread %d failed.\n", i);
            break;
        }

        OS_RWLOCK_WRLOCK(&para->rwlock);
        para->threads_cnt++;
        OS_RWLOCK_WRUNLOCK(&para->rwlock);
        
        OS_PRINT(para->net, "Create thread %d success.\n", i);
    }

    while (0 != para->threads_cnt)
    {
        OS_SLEEP_SECOND(1);
    }

    for (i = 0; i < para->threads_num; i++)
    {
        if (tid[i] != INVALID_TID)
        {
            thread_destroy(tid[i], TRUE);
            OS_PRINT(para->net, "Destroy thread %d finished.\n", i);
        }
    }

    OS_FREE(tid);

    return 0;
}

int do_performance_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if ((0 == strlen(para->ct_name)) || OBJID_IS_INVALID(para->objid))
    {
        OS_PRINT(net, "invalid ct name(%s) or objid(%lld).\n", para->ct_name, para->objid);
        OS_FREE(para);
        return -2;
    }
    
    if (0 == para->threads_num)
    {
        para->threads_num = 1;
    }

    (void)test_performance(para, TRUE);
    (void)test_performance(para, FALSE);

    OS_FREE(para);

    return 0;
}




