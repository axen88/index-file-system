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
File Name: OS_LINUX_USER.H
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
#ifndef __OS_LINUX_USER_H__
#define __OS_LINUX_USER_H__

#include <pthread.h>
#include <unistd.h>

// ANSI header file
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define OS_MALLOC(size)   malloc(size)
#define OS_FREE(mem)     free(mem)

#define OS_VSNPRINTF             vsnprintf

#define OS_PRINT(fmt, ...)   printf("[%s][%d]"fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define NET_PRINT(n, fmt, ...)   (n)->print((n)->net, fmt, ##__VA_ARGS__)
    
#define _CrtDumpMemoryLeaks()

#define OS_STR2ULL(pcBuf, end, base)   strtoull(pcBuf, end, base)
#define OS_SLEEP_SECOND(x)               sleep(x)
#define OS_SLEEP_MS(x)                   usleep(x * 1000)
#define OS_THREAD_EXIT()                  pthread_exit(NULL)

typedef pthread_mutex_t             os_mutex_t;
typedef pthread_rwlock_t            os_rwlock;
typedef pthread_t                   os_thread_id_t;
typedef pthread_t                   os_thread_t;

#define INVALID_TID                 0

#define OS_GET_THREAD_ID()         pthread_self()

#define OS_MUTEX_INIT(v_pMutex)    pthread_mutex_init(v_pMutex, NULL)
#define OS_MUTEX_LOCK(v_pMutex)    pthread_mutex_lock(v_pMutex)
#define OS_MUTEX_UNLOCK(v_pMutex)  pthread_mutex_unlock(v_pMutex)
#define OS_MUTEX_DESTROY(v_pMutex) pthread_mutex_destroy(v_pMutex)

#define OS_RWLOCK_INIT(v_pMutex)      pthread_rwlock_init(v_pMutex, NULL)
#define OS_RWLOCK_RDLOCK(v_pMutex)    pthread_rwlock_rdlock(v_pMutex)
#define OS_RWLOCK_RDUNLOCK(v_pMutex)  pthread_rwlock_unlock(v_pMutex)
#define OS_RWLOCK_WRLOCK(v_pMutex)    pthread_rwlock_wrlock(v_pMutex)
#define OS_RWLOCK_WRUNLOCK(v_pMutex)  pthread_rwlock_unlock(v_pMutex)
#define OS_RWLOCK_DESTROY(v_pMutex)   pthread_rwlock_destroy(v_pMutex)

#define OS_SNPRINTF    (void)snprintf

#define ASSERT(x) assert(x)
#define EXPORT_SYMBOL(x)
#define module_init(x)
#define module_exit(x)

typedef volatile uint32_t atomic_t;

#define atomic_inc(x) __sync_fetch_and_add(x, 1)
#define atomic_dec(x) __sync_fetch_and_sub(x, 1)

#define atomic_inc_return(x) __sync_add_and_fetch(x, 1) 
#define atomic_dec_return(x) __sync_sub_and_fetch(x, 1) 


#define atomic_add(v, x) __sync_fetch_and_add(x, v)
#define atomic_sub(v, x) __sync_fetch_and_sub(x, v)

#define atomic_set(x, v)  (*(x)) = v
#define atomic_read(x)    (*(x))

static inline os_thread_t thread_create(void *(*func)(void *), void *para, char *thread_name)
{
    int32_t ret = 0;
    os_thread_t tid;

    ret = pthread_create(&tid, NULL, func, para);
    if (ret == 0)
    {
        return tid;
    }

    return INVALID_TID;
}

static inline void thread_destroy(os_thread_t tid, bool_t force)
{
    if (force)
    {
        pthread_cancel(tid);
    }
    
    pthread_join(tid, NULL);

    return;
}


#ifdef	__cplusplus
}
#endif


#endif
