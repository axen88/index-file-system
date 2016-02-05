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

            ��Ȩ����(C), 2011~2014, AXEN������
********************************************************************************
�� �� ��: OS_CMD_UI.H
��    ��: 1.00
��    ��: 2011��6��19��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��6��19��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#ifndef __OS_CMD_UI_H__
#define __OS_CMD_UI_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef int (*OS_CMD_FUNC)(int argc, char *argv[]); // ָ������ָ��

#define MAX_CMD_LEVEL    3
#define MAX_SUB_CMD_NUM  10

typedef struct tagOS_CMD_LIST_S
{
	OS_CMD_FUNC func; /* �������Ӧ�ĺ��� */
    char *cmdLevel[MAX_CMD_LEVEL];
	char *comment;   /* �������ע����Ϣ */
} OS_CMD_LIST_S;

extern void os_execute_cmd(int32_t argc, char *argv[],
    OS_CMD_LIST_S *cmd_list, uint32_t cmd_num);
extern int32_t os_get_nth_para(char *input, int32_t nth,
    char *para, uint32_t para_size);
extern int32_t os_parse_para(int argc, char *argv[], char *para,
    char *content, uint32_t content_size);
extern void os_cmd_ui(OS_CMD_LIST_S cmd_list[]);

#ifdef  __cplusplus
}
#endif

#endif

