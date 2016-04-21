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
File Name: INDEX_TOOLS_TREE.C
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

static int32_t cmd_insert_key(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name)) || OBJID_IS_INVALID(para->objid))
    {
        OS_PRINT(para->net, "invalid ct name(%s) or objid(%lld).\n",
            para->index_name, para->objid);
        return -1;
    }

    if ((0 == strlen(para->key))
        || (0 == strlen(para->value)))
    {
        OS_PRINT(para->net, "invalid key(%s) or value(%s).\n",
            para->key, para->value);
        return -1;
    }

    ret = ofs_open_container(para->index_name, &ct);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open ct failed. ct(%s) ret(%d)\n", para->index_name, ret);
        return ret;
    }

    ret = ofs_open_object(ct, para->objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Create obj failed. objid(%lld) ret(%d)\n",
            para->objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    ret = index_insert_key(obj, para->key, strlen(para->key),
        para->value, strlen(para->value));
    if (0 > ret)
    {
        OS_PRINT(para->net, "Insert key failed. key(%s) value(%s) ret(%d)\n",
            para->key, para->value, ret);
    }

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return ret;
}

static int32_t cmd_remove_key(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;

    ASSERT(NULL != para);

    if ((0 == strlen(para->index_name)) || OBJID_IS_INVALID(para->objid))
    {
        OS_PRINT(para->net, "invalid ct name(%s) or objid(%lld).\n",
            para->index_name, para->objid);
        return -1;
    }
    
    if (0 == strlen(para->key))
    {
        OS_PRINT(para->net, "invalid key.\n");
        return -1;
    }

    ret = ofs_open_container(para->index_name, &ct);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open ct failed. ct(%s) ret(%d)\n", para->index_name, ret);
        return ret;
    }

    ret = ofs_open_object(ct, para->objid, &obj);
    if (ret < 0)
    {
        OS_PRINT(para->net, "Open tree failed. objid(%lld) ret(%d)\n",
            para->objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    ret = index_remove_key(obj, para->key, strlen(para->key));
    if (0 > ret)
    {
        OS_PRINT(para->net, "Remove key failed. key(%s) ret(%d)\n",
            para->key, ret);
    }

    (void)ofs_close_object(obj);
    (void)ofs_close_container(ct);
    
    return ret;
}


int do_insert_key_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_insert_key(para);

    OS_FREE(para);

    return 0;
}

int do_remove_key_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_remove_key(para);

    OS_FREE(para);

    return 0;
}


