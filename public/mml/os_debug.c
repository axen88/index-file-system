/*******************************************************************************

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: MML_DEBUG_TRACE.C
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

#include "os_adapter.h"
#include "os_mml.h"

void TraceSetModuleLogLevel(uint32_t v_pid, uint32_t v_level)
{
    ASSERT(MML_PID_NUM <= v_pid);
        
    LOG_SET_LEVEL(v_pid, v_level);

    return;
}

void TraceSetAllModulesLogLevel(uint32_t v_level)
{
    uint32_t pid = 0;

    for (pid = 0; pid < MML_PID_NUM; pid++)
    {
        LOG_SET_LEVEL(pid, v_level);
    }

    return;
}

void TracePrintAllModulesLogLevel(void)
{
    uint32_t pid = 0;

    for (pid = 0; pid < MML_PID_NUM; pid++)
    {
        if (0 != strlen(g_mmlModules[pid].name))
        {
            OS_PRINT("Module %d: name: %s, level %d\n", pid,
                g_mmlModules[pid].name, LOG_GET_LEVEL(pid));
        }
    }

    return;
}




