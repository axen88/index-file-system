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

extern int32_t index_verify_attr(void *tree);

#define TEST_KEY_LEN   8
#define TEST_VALUE_LEN 20

typedef struct tagKEY_ACTION_S
{
    uint8_t action;   /* 删除或插入操作 */
    char *key;     /* key */
} KEY_ACTION_S;

/* 混合测试时的操作序列 */
const KEY_ACTION_S KEY_ACTION_LIST[]
= {
    {1, "0000000000009FFF"},
    {1, "0000000000001234"},
    {0, "0000000000009FFF"},
};

static int32_t cmd_insert_key(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;
    uint64_t key = 0;
    uint8_t value[TEST_VALUE_LEN];

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT("invalid index name(%s) or obj name(%s).\n",
            para->index_name, para->obj_name);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT("Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_open_object(index->root_obj, para->obj_name, &obj);
    if (ret < 0)
    {
        OS_PRINT("Create obj failed. obj(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    ret = index_open_xattr(obj, para->attr_name, &attr);
    if (ret < 0)
    {
        ret = index_create_xattr(obj, para->attr_name, ATTR_FLAG_TABLE, &attr);
        if (ret < 0)
        {
            OS_PRINT("Create tree failed. attr(%s) ret(%d)\n",
                para->attr_name, ret);
            (void)index_close(index);
            return ret;
        }
    }

    memset(value, 0x88, sizeof(value));

    for (key = 0; key < para->keys_num; key++)
    {
        ret = index_insert_key(attr, &key, TEST_KEY_LEN,
            value, TEST_VALUE_LEN);
        if (0 > ret)
        {
            OS_PRINT("Insert key failed. key(%lld) ret(%d)\n",
                key, ret);
            break;
        }

        ret = index_verify_attr(obj);
        if (0 > ret)
        {
            OS_PRINT("Verify key failed. key(%lld) ret(%d)\n",
                key, ret);
            break;
        }
    }

    //(OS_VOID)index_close_attr(attr);
    //(OS_VOID)index_close_object(obj);
    (void)index_close(index);
    
    return 0;
}

static int32_t cmd_remove_key(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;
    uint64_t key = 0;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT("invalid index name(%s) or obj name(%s).\n",
            para->index_name, para->obj_name);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT("Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_open_object(index->root_obj, para->obj_name, &obj);
    if (ret < 0)
    {
        OS_PRINT("Open tree failed. tree(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    ret = index_open_xattr(obj, para->attr_name, &attr);
    if (ret < 0)
    {
        ret = index_create_xattr(obj, para->attr_name, ATTR_FLAG_TABLE, &attr);
        if (ret < 0)
        {
            OS_PRINT("Create tree failed. attr(%s) ret(%d)\n",
                para->attr_name, ret);
            (void)index_close_object(obj);
            (void)index_close(index);
            return ret;
        }
    }

    for (key = 0; key < para->keys_num; key++)
    {
        ret = index_remove_key(attr, &key, TEST_KEY_LEN);
        
        if (0 > ret)
        {
            OS_PRINT("Remove key failed. key(%lld) ret(%d)\n",
                key, ret);
            break;
        }

        ret = index_verify_attr(obj);
        if (0 > ret)
        {
            OS_PRINT("Verify key failed. key(%lld) ret(%d)\n",
                key, ret);
            break;
        }
    }

    (void)index_close_attr(attr);
    (void)index_close_object(obj);
    (void)index_close(index);
    
    return 0;
}

static int32_t cmd_mix_key(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    uint32_t i = 0;
    uint8_t value[TEST_VALUE_LEN];
    uint8_t key[TEST_KEY_LEN];
    int32_t key_len = 0;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name)))
    {
        OS_PRINT("invalid index name(%s) or obj name(%s).\n",
            para->index_name, para->obj_name);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT("Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_create_object(index->root_obj, para->obj_name, 0, &obj);
    if (ret < 0)
    {
        OS_PRINT("Create tree failed. tree(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    memset(value, 0x88, sizeof(value));

    for (i = 0; i < ArraySize(KEY_ACTION_LIST); i++)
    {
        key_len = os_str_to_hex(KEY_ACTION_LIST[i].key, key, 8);
        if (8 != key_len)
        {
            OS_PRINT("Key is invalid. key(%s) i(%d) ret(%d)\n",
                KEY_ACTION_LIST[i].key, i, ret);
            break;
        }

        if (0 == KEY_ACTION_LIST[i].action)
        {
            ret = index_remove_key(obj->mattr, key, TEST_KEY_LEN);
        }
        else
        {
            ret = index_insert_key(obj->mattr, key, TEST_KEY_LEN,
                value, TEST_VALUE_LEN);
        }
        
        if (0 > ret)
        {
            OS_PRINT("Operate key failed. key(%s) action(%d) i(%d) ret(%d)\n",
                KEY_ACTION_LIST[i].key, KEY_ACTION_LIST[i].action, i, ret);
            break;
        }

        ret = index_verify_attr(obj);
        if (0 > ret)
        {
            OS_PRINT("Verify tree failed. key(%s) action(%d) i(%d) ret(%d)\n",
                KEY_ACTION_LIST[i].key, KEY_ACTION_LIST[i].action, i, ret);
            break;
        }
    }

    (void)index_close_object(obj);
    (void)index_close(index);
    
    return 0;
}


int do_insert_key_cmd(int argc, char *argv[])
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT("Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);

    cmd_insert_key(para);

    OS_FREE(para);

    return 0;
}

int do_remove_key_cmd(int argc, char *argv[])
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT("Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);

    cmd_remove_key(para);

    OS_FREE(para);

    return 0;
}

int do_mix_key_cmd(int argc, char *argv[])
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT("Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);

    cmd_mix_key(para);

    OS_FREE(para);

    return 0;
}


