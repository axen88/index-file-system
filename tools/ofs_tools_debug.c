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
#include "ofs_if.h"

static int32_t cmd_create(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;
    object_handle_t *attr = NULL;

    ASSERT(para);

    if (strlen(para->ct_name) == 0)
    {
        NET_PRINT(para->net, "ct name not specified.\n");
        return -1;
    }
    
    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = ofs_create_container(para->ct_name, para->total_sectors, &ct);
        if (ret < 0)
        {
            NET_PRINT(para->net, "Create ct failed. ct(%s) total_sectors(%lld) ret(%d)\n",
                para->ct_name, para->total_sectors, ret);
            return ret;
        }
        
        NET_PRINT(para->net, "Create ct success. ct(%s) total_sectors(%lld) ct(%p)\n",
            para->ct_name, para->total_sectors, ct);
        return 0;
    }

    /* find the specified ct */
    ct = ofs_get_container_handle(para->ct_name);
    if (ct == NULL)
    {
        NET_PRINT(para->net, "The ct(%s) is not opened.\n", para->ct_name);
        return -2;
    }

    // create object
    ret = ofs_create_object(ct, para->objid, FLAG_TABLE | CR_ANSI_STRING | (CR_ANSI_STRING << 4), &obj);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Create obj failed. objid(%lld) ret(%d)\n", para->objid, ret);
        return ret;
    }

    NET_PRINT(para->net, "Create obj success. objid(%lld) obj(%p)\n", para->objid, obj);
    
    return 0;
}

static int32_t cmd_open(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;

    ASSERT(para);

    if (strlen(para->ct_name) == 0)
    {
        NET_PRINT(para->net, "ct name not specified.\n");
        return -1;
    }

    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = ofs_open_container(para->ct_name, &ct);
        if (ret < 0)
        {
            NET_PRINT(para->net, "Open ct failed. ct(%s) ret(%d)\n",
                para->ct_name, ret);
            return ret;
        }
        
        NET_PRINT(para->net, "Open ct success. ct(%s) ct(%p), ref_cnt(%d)\n",
            para->ct_name, ct, ct->ref_cnt);
        return 0;
    }

    ct = ofs_get_container_handle(para->ct_name);
    if (ct == NULL)
    {
        NET_PRINT(para->net, "The ct is not opened. ct(%s)\n", para->ct_name);
        return -2;
    }

    ret = ofs_open_object(ct, para->objid, &obj);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Open obj failed. objid(%lld) ret(%d)\n", para->objid, ret);
        return ret;
    }

    NET_PRINT(para->net, "Open obj success. objid(%lld) obj(%p) ref_cnt(%d)\n",
        para->objid, obj, obj->obj_info->ref_cnt);
 
    return 0;
}

static int32_t cmd_close(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;

    ASSERT(para);

    if (strlen(para->ct_name) == 0)
    {
        NET_PRINT(para->net, "ct name not specified.\n");
        return -1;
    }

    ct = ofs_get_container_handle(para->ct_name);
    if (ct == NULL)
    {
        NET_PRINT(para->net, "The ct is not opened. ct(%s)\n", para->ct_name);
        return -2;
    }
    
    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        ret = ofs_close_container(ct);
        if (ret < 0)
        {
            NET_PRINT(para->net, "Close ct failed. ct(%s) ret(%d)\n", para->ct_name, ret);
            return ret;
        }
        
        NET_PRINT(para->net, "Close ct success. ct(%s)\n", para->ct_name);
        return 0;
    }

    obj = ofs_get_object_handle(ct, para->objid);
    if (obj == NULL)
    {
        NET_PRINT(para->net, "The object is not opened. objid(%lld)\n", para->objid);
        return -2;
    }
    
    ret = ofs_close_object(obj);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Close obj failed. ct(%p) objid(%lld) ret(%d)\n",
            ct, para->objid, ret);
        return ret;
    }

    NET_PRINT(para->net, "Close obj success. ct(%p) objid(%lld)\n", ct, para->objid);
    
    return 0;
}

static int32_t cmd_delete(ifs_tools_para_t *para)
{
    int32_t ret = 0;
    container_handle_t *ct = NULL;

    ASSERT(para);

    NET_PRINT(para->net, "comming soon.\n");
    return 0;

    if ((strlen(para->ct_name) == 0)
        || OBJID_IS_INVALID(para->objid))
    {
        NET_PRINT(para->net, "invalid ct name(%s) or objid(%lld).\n",
            para->ct_name, para->objid);
        return -1;
    }

    ret = ofs_open_container(para->ct_name, &ct);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Open ct failed. ct(%s) ret(%d)\n",
            para->ct_name, ret);
        return ret;
    }

    ret = ofs_delete_object(ct, para->objid);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Delete obj failed. objid(%lld) ret(%d)\n",
            para->objid, ret);
        (void)ofs_close_container(ct);
        return ret;
    }

    NET_PRINT(para->net, "Delete obj success. objid(%lld)\n", para->objid);

    (void)ofs_close_container(ct);
    
    return 0;
}

int do_create_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        NET_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_create(para);

    OS_FREE(para);

    return 0;
}

int do_open_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        NET_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_open(para);

    OS_FREE(para);

    return 0;
}

int do_close_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        NET_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_close(para);

    OS_FREE(para);

    return 0;
}

int do_delete_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        NET_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    cmd_delete(para);

    OS_FREE(para);

    return 0;
}

