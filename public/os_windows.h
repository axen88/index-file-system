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
File Name: OS_WINDOWS.H
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
#ifndef __OS_WINDOWS_H__
#define __OS_WINDOWS_H__

// detect the leackage on windows
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>

// ANSI header file
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define inline _inline

#if _MSC_VER
#define snprintf _snprintf
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define OS_MALLOC   malloc
#define OS_FREE     free
#define OS_PRINT(n, fmt, ...)   (n)->print((n)->net, fmt, ##__VA_ARGS__)

#define OS_STR2ULL(pcBuf, end, base)   strtoul(pcBuf, end, base)
#define OS_SLEEP_SECOND(x)               Sleep(x)
#define OS_SLEEP_MS(x)                   Sleep(x/1000)
#define OS_THREAD_EXIT()                  ExitThread(0)

typedef CRITICAL_SECTION            os_mutex_t;
typedef CRITICAL_SECTION            os_rwlock;
typedef uint64_t                     os_thread_id_t;
typedef HANDLE                      os_thread_t;

#define INVALID_TID                 NULL

#define OS_GET_THREAD_ID()         GetCurrentThreadId()
   
#define OS_MUTEX_INIT(v_pMutex)    InitializeCriticalSection(v_pMutex)
#define OS_MUTEX_LOCK(v_pMutex)    EnterCriticalSection(v_pMutex)
#define OS_MUTEX_UNLOCK(v_pMutex)  LeaveCriticalSection(v_pMutex)
#define OS_MUTEX_DESTROY(v_pMutex) DeleteCriticalSection(v_pMutex)

#define OS_RWLOCK_INIT(v_pMutex)      InitializeCriticalSection(v_pMutex)
#define OS_RWLOCK_RDLOCK(v_pMutex)    EnterCriticalSection(v_pMutex)
#define OS_RWLOCK_RDUNLOCK(v_pMutex)  LeaveCriticalSection(v_pMutex)
#define OS_RWLOCK_WRLOCK(v_pMutex)    EnterCriticalSection(v_pMutex)
#define OS_RWLOCK_WRUNLOCK(v_pMutex)  LeaveCriticalSection(v_pMutex)
#define OS_RWLOCK_DESTROY(v_pMutex)   DeleteCriticalSection(v_pMutex)

#define OS_SNPRINTF(buf, size, fmt, ...) \
do { \
    (void)_snprintf(buf, size, fmt, ##__VA_ARGS__); \
    ((uint8_t *)buf)[size - 1] = 0; \
} while (0)

#define OS_VSNPRINTF(buf, size, fmt, ap) \
do { \
    (void)_vsnprintf(buf, size, fmt, ap); \
    ((uint8_t *)buf)[size - 1] = 0; \
} while (0)
    
#define ASSERT(x) assert(x)
#define EXPORT_SYMBOL(x)
#define module_init(x)
#define module_exit(x)

#define atomic_set(x, n)  (*(x)) = n
#define atomic_read(x, n)  (*(x))

#define atomic_inc(x)  InterlockedIncrement(x)
#define atomic_dec(x)  InterlockedDecrement(x)

#define atomic_add(x, n)  InterlockedExchangeAdd(x, n)
#define atomic_sub(x, n)  InterlockedExchangeAdd(x, -(n))

static inline os_thread_t thread_create(void *(*func)(void *), void *para, char *thread_name)
{
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, para, 0, NULL);
}

static inline void thread_destroy(os_thread_t tid, bool_t force)
{
    if (force)
    {
        TerminateThread(tid, 0);
    }
}



#ifdef	__cplusplus
}
#endif

#endif
