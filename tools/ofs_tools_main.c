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
#include "ofs_if.h"

MODULE(PID_TOOLS);
#include "os_log.h"

extern os_cmd_list_t ifs_cmd_list[];

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

int32_t main(int32_t argc, char *argv[])
{
    int32_t ret = 0;
    net_para_t net;
    
    LOG_SYSTEM_INIT();
    ret = ofs_init_system();
    if (ret < 0)
    {
        printf("Index system init failed.\n");
    }
    else
    {
        net.net = NULL;
        net.print = tools_print;
        os_cmd_ui(ifs_cmd_list, &net);
        ofs_exit_system();
    }
    
    LOG_SYSTEM_EXIT();
    
    _CrtDumpMemoryLeaks();
    
	return 0;
}


