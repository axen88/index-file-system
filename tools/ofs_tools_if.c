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
File Name: INDEX_TOOLS_IF.C
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

void parse_container_para(int argc, char *argv[], ifs_tools_para_t *para)
{
    if (0 != os_parse_para(argc, argv, "-ct", para->ct_name, OFS_NAME_SIZE))
    {
        para->ct_name[0] = 0;
    }

    if (0 != os_parse_para(argc, argv, "-s", para->tmp, TMP_BUF_SIZE))
    {
        para->total_sectors = 1000;
    }
    else
    {
        para->total_sectors = OS_STR2ULL(para->tmp, NULL, 0);
    }

    return;
}

void parse_object_para(int argc, char *argv[], ifs_tools_para_t *para)
{
    if (0 != os_parse_para(argc, argv, "-o", para->tmp, TMP_BUF_SIZE))
    {
        para->objid = INVALID_OBJID;
    }
    else
    {
        para->objid = OS_STR2ULL(para->tmp, NULL, 0);
    }

    if (os_parse_para(argc, argv, "-k", para->key, KEY_MAX_SIZE) < 0)
    {
        para->key[0] = 0;
    }

    if (os_parse_para(argc, argv, "-v", para->value, VALUE_MAX_SIZE) < 0)
    {
        para->value[0] = 0;
    }

    return;
}

void parse_all_para(int argc, char *argv[], ifs_tools_para_t *para)
{
    memset(para, 0, sizeof(ifs_tools_para_t));
    
    OS_RWLOCK_INIT(&para->rwlock);
    parse_container_para(argc, argv, para);
    parse_object_para(argc, argv, para);

    if (0 != os_parse_para(argc, argv, "-n", para->tmp, TMP_BUF_SIZE))
    {
        para->threads_num = 0;
    }
    else
    {
        para->threads_num = OS_STR2ULL(para->tmp, NULL, 0);
    }

    if (0 != os_parse_para(argc, argv, "-kn", para->tmp, TMP_BUF_SIZE))
    {
        para->keys_num = 10;
    }
    else
    {
        para->keys_num = OS_STR2ULL(para->tmp, NULL, 0);
    }

    if (0 == os_parse_para(argc, argv, "-r", NULL, 0))
    {
        para->flags |= TOOLS_FLAGS_REVERSE;
    }

    if (0 == os_parse_para(argc, argv, "-h", NULL, 0))
    {
        para->flags |= TOOLS_FLAGS_HIDE;
    }

    if (0 == os_parse_para(argc, argv, "-sb", NULL, 0))
    {
        para->flags |= TOOLS_FLAGS_SB;
    }

    return;
}

os_cmd_list_t ifs_cmd_list[]
= {
    {do_create_cmd,   {"create",   NULL, NULL}, "<-ct ct_name> [-o obj_id] [-s total_sectors]"},
    {do_open_cmd,     {"open",     NULL, NULL}, "<-ct ct_name> [-o obj_id] [-s total_sectors]"},
    {do_close_cmd,    {"close",    NULL, NULL}, "<-ct ct_name> [-o obj_id]"},
    {do_delete_cmd,   {"delete",   NULL, NULL}, "<-ct ct_name> [-o obj_id]"},
    //{do_rename_cmd,   {"rename",   NULL, NULL}, "<-ct ct_name> [-o name] [-no new_obj_name]"},
        
	{do_list_cmd,     {"list",     NULL, NULL}, "[-ct ct_name] [-o obj_id]"},
	{do_dump_cmd,     {"dump",     NULL, NULL}, "<-ct ct_name> [-o obj_id]"},
	//{do_verify_cmd,   {"verify",   NULL, NULL}, "<-ct ct_name> [-o obj_id]"},
    //{do_fixup_cmd,    {"fixup",    NULL, NULL}, "<-ct ct_name>"},
        
	{do_insert_key_cmd,   {"insert",   NULL, NULL}, "<-ct ct_name> [-o obj_id] [-k key] [-v value]"},
    {do_remove_key_cmd,   {"remove",   NULL, NULL}, "<-ct ct_name> [-o obj_id] [-k key]"},
                
	{do_performance_cmd, {"perf", NULL, NULL}, "<-ct ct_name> <-o obj_id> [-n threads_num] [-kn keys_num]"},
	{NULL, {NULL, NULL, NULL}, NULL}
};


