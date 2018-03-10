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
File Name: OS_CMD_UI.H
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
#ifndef __OS_CMD_UI_H__
#define __OS_CMD_UI_H__

#ifdef  __cplusplus
extern "C" {
#endif


typedef struct net_para
{
	void *net;
    int (*print)(void *net, const char *format, ...);
} net_para_t;


typedef int (*os_cmd_func_t)(int argc, char *argv[], net_para_t *net);

#define MAX_CMD_LEVEL    3
#define MAX_SUB_CMD_NUM  10

typedef struct os_cmd_list
{
	os_cmd_func_t func;
    char *level[MAX_CMD_LEVEL];
	char *comment;
} os_cmd_list_t;

extern int32_t os_parse_para(int argc, char *argv[], char *para, char *content, uint32_t content_size);
int32_t parse_and_exec_cmd(char *cmd, os_cmd_list_t cmd_list[], net_para_t *net);
extern void os_cmd_ui(os_cmd_list_t cmd_list[], net_para_t *net);

#ifdef  __cplusplus
}
#endif

#endif

