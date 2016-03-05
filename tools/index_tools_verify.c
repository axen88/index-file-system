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
File Name: INDEX_TOOLS_VERIFY.C
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
MODULE(PID_INDEX);
#include "os_log.h"

int do_verify_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    OS_PRINT(net, "comming soon.\n");
    return 0;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if (0 == strlen(para->index_name))
    {
        OS_FREE(para);
        para = NULL;
        OS_PRINT(net, "invalid index name(%s).\n", para->index_name);
        return -2;
    }

    index_verify_attr_by_name(para->index_name, para->start_lba, para->objid);

    OS_FREE(para);
    para = NULL;

    return 0;
}




