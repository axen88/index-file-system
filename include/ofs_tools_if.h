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
File Name: OFS_TOOLS_IF.H
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
#ifndef __OFS_TOOLS_IF_H__
#define __OFS_TOOLS_IF_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define TMP_BUF_SIZE   32
#define CMD_SIZE       16

#define TOOLS_FLAGS_HIDE              0x0001
#define TOOLS_FLAGS_REVERSE           0x0002
#define TOOLS_FLAGS_SB                0x0004

typedef struct ifs_tools_para
{
    char index_name[OFS_NAME_SIZE];
    char tmp[TMP_BUF_SIZE];
    char key[KEY_MAX_SIZE];
    char value[VALUE_MAX_SIZE];

    net_para_t *net;

    uint64_t objid;
    uint64_t total_sectors;
    uint32_t threads_num;
    uint64_t keys_num;
    
    uint32_t flags;
    uint32_t no;
    bool_t insert;
    uint32_t threads_cnt;
    os_rwlock rwlock;
} ifs_tools_para_t;


extern int do_verify_cmd(int argc, char *argv[], net_para_t *net);
extern int do_fixup_cmd(int argc, char *argv[], net_para_t *net);
extern int do_dump_cmd(int argc, char *argv[], net_para_t *net);
extern int do_list_cmd(int argc, char *argv[], net_para_t *net);
extern int do_create_cmd(int argc, char *argv[], net_para_t *net);
extern int do_open_cmd(int argc, char *argv[], net_para_t *net);
extern int do_close_cmd(int argc, char *argv[], net_para_t *net);
extern int do_delete_cmd(int argc, char *argv[], net_para_t *net);
extern int do_insert_key_cmd(int argc, char *argv[], net_para_t *net);
extern int do_remove_key_cmd(int argc, char *argv[], net_para_t *net);
extern int do_performance_cmd(int argc, char *argv[], net_para_t *net);
extern void parse_all_para(int argc, char *argv[], ifs_tools_para_t *para);

#ifdef	__cplusplus
}
#endif

#endif
