/*******************************************************************************

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: MML_USER.C
��    ��: ������ (zeng_hr@163.com)
��    ��: 1.00
��    ��: 2012��4��15��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2012��4��15��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#include <signal.h>
#include <sys/ioctl.h>

#include "os_adapter.h"
#include "os_mml.h"

#define SEND_CMD(cmd)  ioctl(g_fd, cmd, &g_userCmd)

MML_CMD g_userCmd;

int32_t g_fd = 0;

bool_t g_exit = B_FALSE;

/*******************************************************************************
��������: MML_Exit
����˵��: ��
�������: ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
��    ��: ������ (zeng_hr@163.com)
�޸�ʱ��: 2012��4��15��
˵    ��: ��
*******************************************************************************/
void MML_Exit(int32_t v_para)
{
    int32_t ret = 0;

    g_exit = B_TRUE;
    ret = SEND_CMD(MML_CMD_INIT);
    if (0 != ret)
    {
        OS_PRINT("Send cmd failed. cmd: %d, ret: %d\n", MML_CMD_INIT, ret);
    }

    return;
}

/*******************************************************************************
��������: MmlSendCmd
����˵��: ��
�������: ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
��    ��: ������ (zeng_hr@163.com)
�޸�ʱ��: 2012��4��15��
˵    ��: ��
*******************************************************************************/
void MmlSendCmd(void)
{
    int32_t ret = 0;
    uint32_t waitTimeCnt = 0;

    /* ��ʼ�� */
    memset(&g_userCmd, 0, sizeof(g_userCmd));
    g_userCmd.status = MML_STATUS_INIT;
    g_exit = B_FALSE;

    /* ע���źź��� */
	signal(SIGINT, MML_Exit);
	signal(SIGKILL, MML_Exit);
	signal(SIGTERM, MML_Exit);
    
    /* �������� */
    g_userCmd.status = MML_STATUS_INPUT_OK;
    ret = SEND_CMD(MML_CMD_PROCESS_CMD);
    if (0 != ret)
    {
        OS_PRINT("Send cmd failed. cmd: %d, ret: %d\n", MML_CMD_PROCESS_CMD, ret);
        MML_Exit(0);
        return;
    }

    /* �������ִ�н�� */
    while (B_FALSE == g_exit)
    {
        if ((MML_STATUS_INIT == g_userCmd.status)
            || (MML_STATUS_OUTPUT_BEGIN == g_userCmd.status))
        {
            if (MML_STATUS_INIT == g_userCmd.status)
            {
                waitTimeCnt += MML_WAIT_TIME;
                if (MML_WAIT_TIMEOUT < waitTimeCnt)
                {
                    OS_PRINT("Wait for result overtime.\n");
                    MML_Exit(0);
                    return;
                }
            }
            
            ret = SEND_CMD(MML_CMD_GET_RESULT);
            if (0 != ret)
            {
                OS_PRINT("Send cmd failed. cmd: %d, ret: %d\n", MML_CMD_GET_RESULT, ret);
                MML_Exit(0);
                return;
            }

            OS_SLEEP_MS(MML_WAIT_TIME);
        }

        waitTimeCnt = 0;
        OS_PRINT("%s", g_userCmd.dat);

        if (MML_STATUS_OUTPUT_CONTINUE == g_userCmd.status)
        {
            g_userCmd.status = MML_STATUS_OUTPUT_BEGIN;
        }
        else if (MML_STATUS_OUTPUT_END == g_userCmd.status)
        {
            break;
        }
    }

    MML_Exit(0);

    return;
}



int32_t main(int32_t argc, char **argv)
{
    MmlSendCmd();
    
    return 0;
}

