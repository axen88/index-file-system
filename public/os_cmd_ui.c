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
File Name: OS_CMD_UI.C
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
#include "os_adapter.h"
#include "os_cmd_ui.h"

#define CMD_MAX_SIZE   1024
#define CMD_MAX_ARGS   (CMD_MAX_SIZE / 2)

#define CMD_QUIT    0
#define CMD_OTHER   1

void format_cmd(char *cmd)
{
    while (0 != *cmd)
    {
        if ('\n' == *cmd)
        {
            *cmd = 0;
            break;
        }

        cmd++;
    }

    return;
}

#ifdef __KERNEL__

void get_cmd(char *cmd, uint32_t size)
{

}

#else

/* ��ȡ�û���һ������ */
void get_cmd(char *cmd, uint32_t size)
{
    printf(">");
    fflush(stdout);

    fgets(cmd, size, stdin);
    format_cmd(cmd);
    
    return;
}

#endif

/* ���ַ����н��������е��ֶ�*/
uint32_t parse_cmd(char *cmd, char *argv[], uint32_t argc)
{
    uint32_t cnt = 0;
    char *c = NULL;

    c = cmd;
    for (;;)
    {
        /* ���з� */
        if (('\n' == *c) || ('\r' == *c))
        {
            *c = '\0';
            break;
        }
        
        /* �ַ��������� */
        if ('\0' == *c)
        {
            break;
        }
        
        /* ȥ���ո� */
        while (' ' == *c)
        {
            *c++ = '\0';
        }
        
        /* �ҵ�����Ч���� */
        argv[cnt++] = c;
        if (cnt >= argc)
        {
            break;
        }
        
        /* ���˵���Ч�ַ�����Щ�ַ����ڲ��� */
        while ((' ' != *c) && ('\0' != *c) && ('\n' != *c) && ('\r' != *c))
        {
            c++;
        }
    }

    return cnt;
}

void show_sub_cmd_list(OS_CMD_LIST_S *cmd_list[], uint32_t cmd_num, NET_PARA_S *net)
{
	uint32_t i = 0;
	uint32_t j = 0;
	
	for (i = 0; i < cmd_num; i++)
	{
        for (j = 0; j < MAX_CMD_LEVEL; j++)
        {
            if (NULL == cmd_list[i]->cmdLevel[j])
            {
                break;
            }
            
            OS_PRINT(net, "%s ", cmd_list[i]->cmdLevel[j]);
        }
        
		OS_PRINT(net, "%s\n", cmd_list[i]->comment);
	}
}

void show_cmd_list(OS_CMD_LIST_S *cmd_list, NET_PARA_S *net)
{
    while (NULL != cmd_list->cmdLevel[0])
	{
        uint32_t j;
        
        for (j = 0; j < MAX_CMD_LEVEL; j++)
        {
            if (NULL == cmd_list->cmdLevel[j])
            {
                break;
            }
            
            OS_PRINT(net, "%s ", cmd_list->cmdLevel[j]);
        }
        
		OS_PRINT(net, ": %s\n", cmd_list->comment);

        cmd_list++;
	}
    
	OS_PRINT(net, "help : show this information\n");
	OS_PRINT(net, "quit : quit the system\n");
}

/* �Ƚ���ʲô�����ִ�� */
int32_t execute_cmd(int32_t argc, char *argv[], OS_CMD_LIST_S *cmd_list, NET_PARA_S *net)
{
    int32_t level;
    OS_CMD_LIST_S *cmd = cmd_list;
    OS_CMD_LIST_S *tmp_cmd = NULL;
    int32_t tmp_num = 0;
    OS_CMD_LIST_S *sub_cmd[MAX_SUB_CMD_NUM];
    int32_t sub_cmd_num = 0;

    if (strcmp("help", argv[0]) == 0)
    { // Get the default help command
        show_cmd_list(cmd_list, net);
        return CMD_OTHER;
    }
    else if (strcmp("quit", argv[0]) == 0)
    { // Get the default quit command
        return CMD_QUIT;
    }
    
    while (NULL != cmd->cmdLevel[0])
    {
        for (level = 0; level < MAX_CMD_LEVEL; level++)
        {
            if (level >= argc)
            {
                break;
            }
            
            if (NULL == cmd->cmdLevel[level])
            {
                break;
            }
            
            if (0 != strcmp(cmd->cmdLevel[level], argv[level]))
            {
                break;
            }
        }

        if (MAX_CMD_LEVEL == level)
        {
            tmp_cmd = cmd;
            tmp_num = level;
            break;
        }

        if (NULL == cmd->cmdLevel[level])
        {
            if (level == argc)
            {
                tmp_cmd = cmd;
                tmp_num = level;
                break;
            }

            if (level > tmp_num)
            {
                tmp_cmd = cmd;
                tmp_num = level;
            }
            else if (level > argc)
            {
                sub_cmd[sub_cmd_num++] = cmd;
            }
        }

        cmd++;
    }

    if (NULL != tmp_cmd)
    {
        (void)tmp_cmd->func(argc - (tmp_num - 1), argv + (tmp_num - 1), net);
        return CMD_OTHER;
    }

    if (0 != sub_cmd_num)
    {
        show_sub_cmd_list(sub_cmd, sub_cmd_num, net);
        return CMD_OTHER;
    }
    
    show_cmd_list(cmd_list, net);

    return CMD_OTHER;
}

/* ��ȡָ��ָ������������ */
int32_t os_parse_para(int argc, char *argv[], char *para,
    char *content, uint32_t content_size)
{
    int32_t i = 0;

    for (i = 0; i < argc; i++)
    {
        if (0 == strcmp(para, argv[i]))
        {
            break;
        }
    }

    if (i == argc)
    { /* ����û���ҵ� */
        return -2;
    }

    if (i == (argc - 1))
    { /* �����ҵ��ˣ����������� */
        if (NULL == content)
        {
            return 0;
        }
        
        content[0] = 0;
    }
    else
    {
        OS_SNPRINTF(content, content_size, "%s", argv[i + 1]);
    }
    
    return 0;
}

int32_t parse_and_exec_cmd(char *cmd, OS_CMD_LIST_S cmd_list[], NET_PARA_S *net)
{
    uint32_t tmp_argc = 0;
    char **tmp_argv = NULL;

    tmp_argv = (char **)OS_MALLOC(CMD_MAX_ARGS * sizeof(char *));
    if (NULL == tmp_argv)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            CMD_MAX_ARGS * (uint32_t)sizeof(char *));
        return CMD_OTHER;
    }

    tmp_argc = parse_cmd(cmd, tmp_argv, CMD_MAX_ARGS);
    if (0 == tmp_argc)
    {
        OS_FREE(tmp_argv);
        return CMD_OTHER;
    }
    
    if (execute_cmd((int32_t)tmp_argc, tmp_argv, cmd_list, net) != CMD_QUIT)
    {
        OS_FREE(tmp_argv);
        return CMD_OTHER;
    }

    OS_FREE(tmp_argv);
    return CMD_QUIT;
}


/*******************************************************************************
��������: OSCmdUI
����˵��: �����û����������д����������û�д��룬��ô���뽻������
�������:
    argc   : �������д���������ֶε���Ŀ
    v_pcArgv  : �������д���ĸ������ֶε���ʼ��ַ
    v_pDoCmd  : �û�����
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
void os_cmd_ui(OS_CMD_LIST_S cmd_list[], NET_PARA_S *net)
{
    int32_t i = 0;
    char *cmd = NULL;

    cmd = OS_MALLOC(CMD_MAX_SIZE);
    if (NULL == cmd)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n", CMD_MAX_SIZE);
        return;
    }

    show_cmd_list(cmd_list, net);
    
    for (;;)
    {
        get_cmd(cmd, CMD_MAX_SIZE);

        if (parse_and_exec_cmd(cmd, cmd_list, net) == CMD_QUIT)
        {
            break;
        }
    }

    OS_FREE(cmd);

    return;
}


