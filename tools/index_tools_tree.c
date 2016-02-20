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
文 件 名: OS_INDEX_TREE.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 树的算法测试程序
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

static int32_t cmd_insert_key(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT(para->net, "invalid index name(%s) or obj name(%s).\n",
            para->index_name, para->obj_name);
        return -1;
    }

    if ((0 == strlen(para->key))
        || (0 == strlen(para->value)))
    {
        OS_PRINT(para->net, "invalid key(%s) or value(%s).\n",
            para->key, para->value);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_open_object(index, para->objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Create obj failed. obj(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    ret = index_insert_key(attr, para->key, strlen(para->key),
        para->value, strlen(para->value));
    if (0 > ret)
    {
        OS_PRINT(para->net, "Insert key failed. key(%s) value(%s) ret(%d)\n",
            para->key, para->value, ret);
    }

    (void)index_close_attr(attr);
    (void)index_close_object(obj);
    (void)index_close(index);
    
    return ret;
}

static int32_t cmd_remove_key(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT(para->net, "invalid index name(%s) or obj name(%s).\n",
            para->index_name, para->obj_name);
        return -1;
    }
    
    if (0 == strlen(para->key))
    {
        OS_PRINT(para->net, "invalid key.\n");
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_open_object(index, para->objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open tree failed. tree(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    ret = index_remove_key(attr, para->key, strlen(para->key));
    if (0 > ret)
    {
        OS_PRINT(para->net, "Remove key failed. key(%s) ret(%d)\n",
            para->key, ret);
    }

    (void)index_close_attr(attr);
    (void)index_close_object(obj);
    (void)index_close(index);
    
    return ret;
}


int do_insert_key_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_insert_key(para);

    OS_FREE(para);

    return 0;
}

int do_remove_key_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_remove_key(para);

    OS_FREE(para);

    return 0;
}


