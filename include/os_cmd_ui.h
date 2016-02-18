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
文 件 名: OS_CMD_UI.H
版    本: 1.00
日    期: 2011年6月19日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#ifndef __OS_CMD_UI_H__
#define __OS_CMD_UI_H__

#ifdef  __cplusplus
extern "C" {
#endif


typedef struct tagNET_PARA_S
{
	void *net;
    int (*print)(void *net, const char *format, ...);
} NET_PARA_S;


typedef int (*OS_CMD_FUNC)(int argc, char *argv[], NET_PARA_S *net);

#define MAX_CMD_LEVEL    3
#define MAX_SUB_CMD_NUM  10

typedef struct tagOS_CMD_LIST_S
{
	OS_CMD_FUNC func;
    char *cmdLevel[MAX_CMD_LEVEL];
	char *comment;
} OS_CMD_LIST_S;

extern int32_t os_parse_para(int argc, char *argv[], char *para, char *content, uint32_t content_size);
int32_t parse_and_exec_cmd(char *cmd, OS_CMD_LIST_S cmd_list[], NET_PARA_S *net);
extern void os_cmd_ui(OS_CMD_LIST_S cmd_list[], NET_PARA_S *net);

#ifdef  __cplusplus
}
#endif

#endif

