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

            版权所有(C), 2011~2014, AXEN工作室
********************************************************************************
文 件 名: INDEX_TOOLS_PERFORMANCE.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 评估树的插入、删除操作性能的工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

#define TEST_KEY_LEN   8
#define TEST_VALUE_LEN 20

/*******************************************************************************
函数名称: test_insert_key_performance
功能说明: 测试树的key插入性能
输入参数:
    index_name: 要操作的树所在的索引区名称
    start_lba: 索引区的起始lba
    obj_name  : 要操作的树名称
    keys_num : 要操作的key数目
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t test_insert_key_performance(char *index_name, uint64_t start_lba,
    uint64_t objid, uint64_t keys_num, NET_PARA_S *net)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    uint64_t key = 0;
    uint8_t c[TEST_VALUE_LEN];
    uint64_t ullTime = 0;

    ASSERT(NULL != index_name);
    ASSERT(0 != strlen(index_name));
    ASSERT(0 != objid);

    ret = index_open(index_name, start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT(net, "Open index failed. name(%s)  start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = index_create_object(index, objid, ATTR_FLAG_TABLE | COLLATE_ANSI_STRING, 0, &obj);
    if (ret < 0)
    {
        OS_PRINT(net, "Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        (void)index_close(index);
        return ret;
    }

    memset(c, 0x88, sizeof(c));

    OS_PRINT(net, "Start insert key. objid(%lld) total(%lld)\n", objid, keys_num);
    ullTime = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_insert_key(obj->attr, &key, TEST_KEY_LEN,
            c, TEST_VALUE_LEN);
        if (0 > ret)
        {
            OS_PRINT(net, "Insert key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished insert key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - ullTime);

    (void)index_close_object(obj);
    (void)index_close(index);
    
    return 0;
}

/*******************************************************************************
函数名称: test_remove_key_performance
功能说明: 测试树的key删除性能
输入参数:
    index_name: 要操作的树所在的索引区名称
    start_lba: 索引区的起始lba
    obj_name  : 要操作的树名称
    keys_num : 要操作的key数目
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t test_remove_key_performance(char *index_name, uint64_t start_lba,
    uint64_t objid, uint64_t keys_num, NET_PARA_S *net)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    uint64_t key = 0;
    uint64_t ullTime = 0;

    ASSERT(NULL != index_name);
    ASSERT(0 != strlen(index_name));
    ASSERT(0 != objid);

    ret = index_open(index_name, start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT(net, "Open index failed. name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = index_open_object(index, objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(net, "Open tree failed. objid(%lld) ret(%d)\n", objid, ret);
        (void)index_close(index);
        return ret;
    }

    OS_PRINT(net, "Start remove key. objid(%lld) total(%lld)\n", objid, keys_num);
    ullTime = os_get_ms_count();

    for (key = 0; key < keys_num; key++)
    {
        ret = index_remove_key(obj->attr, &key, TEST_KEY_LEN);
        if (0 > ret)
        {
            OS_PRINT(net, "Remove key failed. objid(%lld) key(%lld) ret(%d)\n", objid, key, ret);
            break;
        }
    }

    OS_PRINT(net, "Finished remove key. objid(%lld) total(%lld) time(%lld ms)\n",
        objid, keys_num, os_get_ms_count() - ullTime);

    (void)index_close_object(obj);
    (void)index_close(index);
    
    return 0;
}

/*******************************************************************************
函数名称: test_performance_thread
功能说明: 评估性能线程
输入参数:
    para: 评估参数
输出参数: 无
返 回 值: 无意义
说    明: 无
*******************************************************************************/
void *test_performance_thread(void *para)
{
    INDEX_TOOLS_PARA_S *tmp_para = para;
    char obj_name[OBJ_NAME_SIZE];

    OS_RWLOCK_WRLOCK(&tmp_para->rwlock);
    OS_SNPRINTF(obj_name, OBJ_NAME_SIZE, "%s%d",
        tmp_para->obj_name, tmp_para->no++);
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

/*******************************************************************************
函数名称: test_performance
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t test_performance(INDEX_TOOLS_PARA_S *para,
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

int do_performance_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT(net, "invalid index name(%s) or obj name(%s).\n", para->index_name, para->obj_name);
        OS_FREE(para);
        return -2;
    }
    
    if (0 == para->threads_num)
    {
        para->threads_num = 1; // 至少1个线程
    }

    /* 多线程测试插入性能 */
    (void)test_performance(para, B_TRUE);
    
    /* 多线程测试删除性能 */
    (void)test_performance(para, B_FALSE);

    OS_FREE(para);

    return 0;
}




