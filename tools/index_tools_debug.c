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

            Copyright(C), 2016~2019, axen2012@qq.com
********************************************************************************
File Name: INDEX_TOOLS_DEBUG.C
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

static int32_t cmd_create(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;

    ASSERT(NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT(para->net, "index name not specified.\n");
        return -1;
    }
    
    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = index_create(para->index_name, para->total_sectors, para->start_lba, &index);
        if (ret < 0)
        {
            OS_PRINT(para->net, "Create index failed. index(%s) total_sectors(%lld) start_lba(%lld) ret(%d)\n",
                para->index_name, para->total_sectors, para->start_lba, ret);
            return ret;
        }
        
        OS_PRINT(para->net, "Create index success. index(%s) total_sectors(%lld) start_lba(%lld) index(%p)\n",
            para->index_name, para->total_sectors, para->start_lba, index);
        return 0;
    }

    /* find the specified index */
    index = index_get_handle(para->index_name);
    if (index == NULL)
    {
        OS_PRINT(para->net, "The index(%s) is not opened.\n", para->index_name);
        return -2;
    }

    // create object
    ret = index_create_object(index, para->objid, FLAG_TABLE | COLLATE_ANSI_STRING, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Create obj failed. objid(%lld) ret(%d)\n", para->objid, ret);
        return ret;
    }

    OS_PRINT(para->net, "Create obj success. objid(%lld) obj(%p)\n", para->objid, obj);
    
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
        OS_PRINT(para->net, "index name not specified.\n");
        return -1;
    }

    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = index_open(para->index_name, para->start_lba, &index);
        if (ret < 0)
        {
            OS_PRINT(para->net, "Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
                para->index_name, para->start_lba, ret);
            return ret;
        }
        
        OS_PRINT(para->net, "Open index success. index(%s) start_lba(%lld) index(%p), ref_cnt(%d)\n",
            para->index_name, para->start_lba, index, index->index_ref_cnt);
        return 0;
    }

    index = index_get_handle(para->index_name);
    if (NULL == index)
    {
        OS_PRINT(para->net, "The index is not opened. index(%s)\n", para->index_name);
        return -2;
    }

    ret = index_open_object(index, para->objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open obj failed. objid(%lld) ret(%d)\n", para->objid, ret);
        return ret;
    }

    OS_PRINT(para->net, "Open obj success. objid(%lld) obj(%p) ref_cnt(%d)\n",
        para->objid, obj, obj->obj_ref_cnt);
 
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
        OS_PRINT(para->net, "index name not specified.\n");
        return -1;
    }

    index = index_get_handle(para->index_name);
    if (NULL == index)
    {
        OS_PRINT(para->net, "The index is not opened. index(%s)\n", para->index_name);
        return -2;
    }
    
    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = index_close(index);
        if (ret < 0)
        {
            OS_PRINT(para->net, "Close index failed. index(%s) ret(%d)\n", para->index_name, ret);
            return ret;
        }
        
        OS_PRINT(para->net, "Close index success. index(%s)\n", para->index_name);
        return 0;
    }

    obj = index_get_object_handle(index, para->objid);
    if (NULL == obj)
    {
        OS_PRINT(para->net, "The object is not opened. objid(%lld)\n", para->objid);
        return -2;
    }
    
    ret = index_close_object(obj);
    if (0 > ret)
    {
        OS_PRINT(para->net, "Close obj failed. index(%p) objid(%lld) ret(%d)\n",
            index, para->objid, ret);
        return ret;
    }

    OS_PRINT(para->net, "Close obj success. index(%p) objid(%lld)\n", index, para->objid);
    
    return 0;
}

static int32_t cmd_delete(INDEX_TOOLS_PARA_S *para)
{
    int32_t ret = 0;
    INDEX_HANDLE *index = NULL;

    ASSERT(NULL != para);

    OS_PRINT(para->net, "comming soon.\n");
    return 0;

    if ((0 == strlen(para->index_name))
        || OBJID_IS_INVALID(para->objid))
    {
        OS_PRINT(para->net, "invalid index name(%s) or objid(%lld).\n",
            para->index_name, para->objid);
        return -1;
    }

    ret = index_open(para->index_name, para->start_lba, &index);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open index failed. index(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return ret;
    }

    ret = index_delete_object(index, para->objid);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Delete obj failed. objid(%lld) ret(%d)\n",
            para->objid, ret);
        (void)index_close(index);
        return ret;
    }

    OS_PRINT(para->net, "Delete obj success. objid(%lld)\n", para->objid);

    (void)index_close(index);
    
    return 0;
}

int do_create_cmd(int argc, char *argv[], NET_PARA_S *net)
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
    cmd_create(para);

    OS_FREE(para);

    return 0;
}

int do_open_cmd(int argc, char *argv[], NET_PARA_S *net)
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
    cmd_open(para);

    OS_FREE(para);

    return 0;
}

int do_close_cmd(int argc, char *argv[], NET_PARA_S *net)
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
    cmd_close(para);

    OS_FREE(para);

    return 0;
}

int do_delete_cmd(int argc, char *argv[], NET_PARA_S *net)
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
    cmd_delete(para);

    OS_FREE(para);

    return 0;
}

