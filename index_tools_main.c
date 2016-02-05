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
文 件 名: INDEX_TOOLS.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 工具程序linux用户态或vc下的主程序
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"
MODULE(PID_INDEX);
#include "os_log.h"

extern OS_CMD_LIST_S INDEX_CMD_LIST[];


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
    
    LOG_SYSTEM_INIT();
    ret = index_init_system();
    if (0 > ret)
    {
        OS_PRINT("Index system init failed.\n");
    }
    else
    {
        os_cmd_ui(INDEX_CMD_LIST);
        index_exit_system();
    }
    
    LOG_SYSTEM_EXIT();
    
    _CrtDumpMemoryLeaks();
    
	return 0;
}


