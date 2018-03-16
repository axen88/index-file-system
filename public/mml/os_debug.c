/*******************************************************************************

            版权所有(C), 2012~2015, AXEN工作室
********************************************************************************
文 件 名: MML_DEBUG_TRACE.C
作    者: 曾华荣 (zeng_hr@163.com)
版    本: 1.00
日    期: 2012年4月15日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年4月15日
--------------------------------------------------------------------------------
    1. 初始版本
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




