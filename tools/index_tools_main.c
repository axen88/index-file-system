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
�� �� ��: INDEX_TOOLS.C
��    ��: 1.00
��    ��: 2011��8��21��
��������: ���߳���linux�û�̬��vc�µ�������
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��8��21��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#include "index_if.h"
MODULE(PID_INDEX);
#include "os_log.h"

extern OS_CMD_LIST_S INDEX_CMD_LIST[];


/*******************************************************************************
��������: main
����˵��: ���빤��ϵͳ
�������:
    argc: ������Ŀ
    argv: ������
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
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


