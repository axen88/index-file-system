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
File Name: OS_THREADS_GROUP.C
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
#ifdef __KERNEL__
#include <linux/kthread.h>

#elif defined(WIN32)
#include <stdlib.h>
#include <string.h>

#else
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#endif

#include "os_adapter.h"
#include "os_threads_group.h"

typedef struct tagTHREAD_ARRAY_S
{
    uint32_t uiRealNum;
    OS_THREAD_T *pPids;
} THREAD_ARRAY_S;

void *threads_group_create(uint32_t num, void *(*func)(void *),
    void *para, char *thread_name)
{
    THREAD_ARRAY_S *pstThreadGroup = NULL;
    uint32_t i = 0;
    int32_t ret = 0;

    pstThreadGroup = (THREAD_ARRAY_S *)OS_MALLOC(sizeof(THREAD_ARRAY_S));
    if (NULL == pstThreadGroup)
    {
        return NULL;
    }

    pstThreadGroup->pPids = (OS_THREAD_T *)OS_MALLOC(sizeof(OS_THREAD_T) * num);
    if (NULL == pstThreadGroup->pPids)
    {
        OS_FREE(pstThreadGroup);
        return NULL;
    }

    for (i = 0; i < num; i++)
    {
#ifdef __KERNEL__
        pstThreadGroup->pPids[i] = kthread_run((int (*)(void*))func, para, 
            thread_name);
        if (IS_ERR(pstThreadGroup->pPids[i]))
        {
            break;
        }
#elif defined(WIN32)
        pstThreadGroup->pPids[i] = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)func, para, 0, NULL);
        if (NULL == pstThreadGroup->pPids[i])
        {
            break;
        }

#else
        OS_THREAD_T tid = 0;

        ret = pthread_create(&tid, NULL, func, para);
        if (ret == 0)
        {
            pstThreadGroup->pPids[i] = tid;
        }
        else
        {
            break;
        }
#endif
    }

    if (0 == i)
    {
        OS_FREE(pstThreadGroup->pPids);
        OS_FREE(pstThreadGroup);
        return NULL;
    }

    pstThreadGroup->uiRealNum = i;

    return pstThreadGroup;
}

int32_t threads_group_get_real_num(void *threads_group)
{
    if (NULL == threads_group)
    {
        return -ERR_THREADS_GROUP_INVALID_PARA;
    }
    
    return (int32_t)((THREAD_ARRAY_S *)threads_group)->uiRealNum;
}

void threads_group_destroy(void *threads_group, uint32_t force,
    uint64_t over_time_ms)
{
    uint32_t i = 0;
    THREAD_ARRAY_S *pstThreadGroup = (THREAD_ARRAY_S *)threads_group;

    ASSERT (NULL != pstThreadGroup);

    for (i = 0; i < pstThreadGroup->uiRealNum; i++)
    {
#ifdef __KERNEL__
        (void)kthread_stop(pstThreadGroup->pPids[i]);
#elif defined(WIN32)
        if (force)
        {
            TerminateThread(pstThreadGroup->pPids[i], 0);
        }
#else
        if (force)
        {
            pthread_cancel(pstThreadGroup->pPids[i]);
        }
        
        pthread_join(pstThreadGroup->pPids[i], NULL);
#endif
    }

    OS_FREE(pstThreadGroup->pPids);
    OS_FREE(pstThreadGroup);

    return;
}


