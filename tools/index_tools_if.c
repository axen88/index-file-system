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
#include "index_if.h"

void parse_index_para(int argc, char *argv[], INDEX_TOOLS_PARA_S *para)
{
    if (0 != os_parse_para(argc, argv, "-i", para->index_name, INDEX_NAME_SIZE))
    {
        para->index_name[0] = 0;
    }

    /* 起始lba地址，默认20480 */
    if (0 != os_parse_para(argc, argv, "-s", para->tmp, TMP_BUF_SIZE))
    {
        para->start_lba = 0;
    }
    else
    {
        para->start_lba = OSStrToUll(para->tmp, NULL, 0);
    }

    /* 起始扇区数目，默认0 */
    if (0 != os_parse_para(argc, argv, "-x", para->tmp, TMP_BUF_SIZE))
    {
        para->total_sectors = 1000;
    }
    else
    {
        para->total_sectors = OSStrToUll(para->tmp, NULL, 0);
    }

    return;
}

void parse_object_para(int argc, char *argv[], INDEX_TOOLS_PARA_S *para)
{
    if (0 != os_parse_para(argc, argv, "-o", para->tmp, TMP_BUF_SIZE))
    {
        para->objid = INVALID_OBJID;
    }
    else
    {
        para->objid = OSStrToUll(para->tmp, NULL, 0);
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

void parse_all_para(int argc, char *argv[], INDEX_TOOLS_PARA_S *para)
{
    memset(para, 0, sizeof(INDEX_TOOLS_PARA_S));
    
    OS_RWLOCK_INIT(&para->rwlock);
    parse_index_para(argc, argv, para);
    parse_object_para(argc, argv, para);

    if (0 != os_parse_para(argc, argv, "-n", para->tmp, TMP_BUF_SIZE))
    {
        para->threads_num = 0;
    }
    else
    {
        para->threads_num = OSStrToUll(para->tmp, NULL, 0);
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

OS_CMD_LIST_S INDEX_CMD_LIST[]
= {
    {do_create_cmd,   {"create",   NULL, NULL}, "<-i index_name> [-o obj_id] [-s start_lba]"},
    {do_open_cmd,     {"open",     NULL, NULL}, "<-i index_name> [-o obj_id] [-s start_lba]"},
    {do_close_cmd,    {"close",    NULL, NULL}, "<-i index_name> [-o obj_id]"},
    {do_delete_cmd,   {"delete",   NULL, NULL}, "<-i index_name> [-o obj_id]"},
    //{do_rename_cmd,   {"rename",   NULL, NULL}, "<-i index_name> [-o obj_name] [-no new_obj_name]"},
        
	{do_list_cmd,     {"list",     NULL, NULL}, "[-i index_name] [-o obj_id]"},
	{do_dump_cmd,     {"dump",     NULL, NULL}, "<-i index_name> [-o obj_id]"},
	{do_verify_cmd,   {"verify",   NULL, NULL}, "<-i index_name> [-o obj_id]"},
    {do_fixup_cmd,    {"fixup",    NULL, NULL}, "<-i index_name>"},
        
	{do_insert_key_cmd,   {"insert",   NULL, NULL}, "<-i index_name> [-o obj_id] [-k key] [-v value]"},
    {do_remove_key_cmd,   {"remove",   NULL, NULL}, "<-i index_name> [-o obj_id] [-k key]"},
                
	//{do_performance_cmd, {"perf", NULL, NULL}, "<-i index_name> <-o obj_id> [-n threads_num]"},
	{NULL, {NULL, NULL, NULL}, NULL}
};

#ifdef __KERNEL__

extern avl_tree_t *g_index_list;

int32_t index_init(void)
{
    int32_t ret = 0;
    
    MML_MODULE index_mml;

    strcpy(index_mml.name, "index");
    index_mml.DoCmd = IndexDoCmd;

    LOG_EVENT("Start insmod index module.\n");
    
    ret = index_init_system();
    if (0 > ret)
    {
        LOG_ERROR("Insmod index module failed.\n");
        return ret;
    }
    
    MML_Register(PID_INDEX, &index_mml);
    
    LOG_EVENT("Finish insmod index module.\n");
    
    return 0;
}

void index_exit(void)
{
    LOG_EVENT("Start rmmod index module.\n");
    
    if (NULL != g_index_list)
    {
        MML_Unregister(PID_INDEX);
        index_exit_system();
    }
    
    LOG_EVENT("Finish rmmod index module.\n");

    return;
}

module_init(index_init);
module_exit(index_exit);

#endif

