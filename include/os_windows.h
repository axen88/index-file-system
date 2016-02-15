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
文 件 名: OS_WINDOWS.H
版    本: 1.00
日    期: 2011年9月29日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年9月29日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#ifndef __OS_WINDOWS_H__
#define __OS_WINDOWS_H__

// Windows下内存泄露检测
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>

// ANSI header file
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define OS_MALLOC   malloc
#define OS_FREE     free
//#define OS_PRINT   (void)printf

#define OSStrToUll(pcBuf, end, base)   strtoul(pcBuf, end, base)
#define OS_SLEEP_SECOND(x)               Sleep(x)
#define OS_SLEEP_MS(x)                  
#define OSThreadExit()                  ExitThread(0)

typedef CRITICAL_SECTION            OS_MUTEX;
typedef CRITICAL_SECTION            OS_RWLOCK;
typedef uint64_t                     OS_THREAD_ID;
typedef HANDLE                      OS_THREAD_T;
    
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
    
#include <assert.h>
#define ASSERT(x) assert(x)
#define EXPORT_SYMBOL(x)
#define module_init(x)
#define module_exit(x)

#endif
