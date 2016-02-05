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
文 件 名: OS_INDEX_DEBUG.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 树和索引区的调试工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

static int32_t cmd_create(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;

    ASSERT(NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT("invalid index name(%s).\n", para->index_name);
        return -1;
    }
    
    /* 未指定对象名称，那么就是要创建索引区 */
    if (0 == strlen(para->obj_name))
    {
        ret = index_create(para->index_name, para->total_sectors, para->start_lba, &index);
        if (ret < 0)
        {
            OS_PRINT("Create index failed. index(%s) total_sectors(%lld) start_lba(%lld) ret(%d)\n",
                para->index_name, para->total_sectors, para->start_lba, ret);
            return ret;
        }
        
        OS_PRINT("Create index success. index(%s) total_sectors(%lld) start_lba(%lld) index(%p)\n",
            para->index_name, para->total_sectors, para->start_lba, index);
        return 0;
    }

    /* 有指定对象名称，那么就查找索引区 */
    index = index_find_handle(para->index_name);
    if (index == NULL)
    {
        OS_PRINT("The index(%s) is not opened.\n", para->index_name);
        return -2;
    }

    /* 未指定属性名称，那么就是要创建对象 */
    if (0 == strlen(para->attr_name))
    {
        ret = index_create_object(index->root_obj, para->obj_name, 0, &obj);
        if (ret < 0)
        {
            OS_PRINT("Create obj failed. obj_name(%s) ret(%d)\n", para->obj_name, ret);
            return ret;
        }

        OS_PRINT("Create obj success. obj_name(%s) obj(%p)\n", para->obj_name, obj);
        return 0;
    }

    /* 有指定属性名称，那么就查找索引区 */
    obj = find_child_object_handle(index->root_obj, para->obj_name);
    if (obj == NULL)
    {
        OS_PRINT("The obj is not opened. obj(%s)\n", para->obj_name);
        return -2;
    }

    /* 创建属性 */
    ret = index_create_xattr(obj, para->attr_name, ATTR_FLAG_TABLE, &attr);
    if (ret < 0)
    {
        OS_PRINT("Create attr failed. attr_name(%s) ret(%d)\n", para->attr_name, ret);
        return ret;
    }
    
    OS_PRINT("Create attr success. attr_name(%s) attr(%p)\n", para->attr_name, attr);

    return 0;
}

static int32_t cmd_open(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;

    ASSERT(NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT("invalid index name(%s).\n", para->index_name);
        return -1;
    }

    if (0 == strlen(para->obj_name))
    { /* 仅仅打开索引区 */
        ret = index_open(para->index_name, para->start_lba, &index);
        if (ret < 0)
        {
            OS_PRINT("Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
                para->index_name, para->start_lba, ret);
            return ret;
        }
        
        OS_PRINT("Open index success. index(%s) start_lba(%lld) index(%p)\n",
            para->index_name, para->start_lba, index);
        return 0;
    }

    index = index_find_handle(para->index_name);
    if (NULL == index)
    {
        OS_PRINT("The index is not opened. index(%s)\n", para->index_name);
        return -2;
    }

    ret = index_open_object(index->root_obj, para->obj_name, &obj);
    if (ret < 0)
    {
        OS_PRINT("Open obj failed. obj(%s) ret(%d)\n", para->obj_name, ret);
        return ret;
    }

    OS_PRINT("Open obj success. obj(%s) obj(%p)\n", para->obj_name, obj);
 
    return 0;
}

static int32_t cmd_close(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;

    ASSERT(NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT("invalid index name(%s).\n", para->index_name);
        return -1;
    }

    index = index_find_handle(para->index_name);
    if (NULL == index)
    {
        OS_PRINT("The index is not opened. index(%s)\n", para->index_name);
        return -2;
    }
    
    if (0 == strlen(para->obj_name))
    { /* 仅仅打开索引区 */
        ret = index_close(index);
        if (ret < 0)
        {
            OS_PRINT("Close index failed. index(%s) ret(%d)\n", para->index_name, ret);
            return ret;
        }
        
        OS_PRINT("Close index success. index(%s)\n", para->index_name);
        return 0;
    }

    obj = find_child_object_handle(index->root_obj, para->obj_name);
    if (NULL == obj)
    {
        OS_PRINT("The obj is not opened. index(%p) obj(%s)\n", index, para->obj_name);
        return -2;
    }

    ret = index_close_object(obj);
    if (0 > ret)
    {
        OS_PRINT("Close obj failed. index(%p) obj(%s) ret(%d)\n",
            index, para->obj_name, ret);
        return ret;
    }

    OS_PRINT("Close obj success. index(%p) obj(%s)\n", index, para->obj_name);
    
    return 0;
}

static int32_t cmd_delete(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;

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

    ret = index_delete_object(index->root_obj, para->obj_name, NULL, NULL);
    if (ret < 0)
    {
        OS_PRINT("Delete obj failed. obj(%s) ret(%d)\n",
            para->obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    OS_PRINT("Delete obj success. obj(%s)\n", para->obj_name);

    (void)index_close(index);
    
    return 0;
}

static int32_t cmd_rename(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name))
        || (0 == strlen(para->obj_name))
        || (0 == strlen(para->new_obj_name)))
    {
        OS_PRINT("invalid index name(%s) or obj name(%s) or new obj name(%s).\n",
            para->index_name, para->obj_name, para->new_obj_name);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT("Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_rename_object(index->root_obj, para->obj_name,
        para->new_obj_name);
    if (ret < 0)
    {
        OS_PRINT("Rename obj failed. obj(%s) new_obj(%s) ret(%d)\n",
            para->obj_name, para->new_obj_name, ret);
        (void)index_close(index);
        return ret;
    }

    OS_PRINT("Rename obj success. obj(%s) new_obj(%s)\n",
        para->obj_name, para->new_obj_name);

    (void)index_close(index);
    
    return 0;
}

int do_create_cmd(int argc, char *argv[])
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
    cmd_create(para);

    OS_FREE(para);

    return 0;
}

int do_open_cmd(int argc, char *argv[])
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
    cmd_open(para);

    OS_FREE(para);

    return 0;
}

int do_close_cmd(int argc, char *argv[])
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
    cmd_close(para);

    OS_FREE(para);

    return 0;
}

int do_delete_cmd(int argc, char *argv[])
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
    cmd_delete(para);

    OS_FREE(para);

    return 0;
}

int do_rename_cmd(int argc, char *argv[])
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
    cmd_rename(para);

    OS_FREE(para);

    return 0;
}

