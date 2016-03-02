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
File Name: INDEX_TOOLS_MAIN.C
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
#include <stdarg.h>
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

extern OS_CMD_LIST_S INDEX_CMD_LIST[];

int tools_print(void *net, const char *format, ...)
{
    #define BUF_LEN  1024
    va_list ap;
    char buf[BUF_LEN];
    
    va_start(ap, format);
    OS_VSNPRINTF(buf, BUF_LEN, format, ap);
    va_end(ap);

    printf("%s", buf);

    return 0;
}


/*******************************************************************************
函数名称: main
功能说明: 进入工具系统
输入参数:
    argc: 参数数目
    argv: 各参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t main(int32_t argc, char *argv[])
{
    int32_t ret = 0;
    NET_PARA_S net;
    
    LOG_SYSTEM_INIT();
    ret = index_init_system();
    if (0 > ret)
    {
        printf("Index system init failed.\n");
    }
    else
    {
        net.net = NULL;
        net.print = tools_print;
        os_cmd_ui(INDEX_CMD_LIST, &net);
        index_exit_system();
    }
    
    LOG_SYSTEM_EXIT();
    
    _CrtDumpMemoryLeaks();
    
	return 0;
}


