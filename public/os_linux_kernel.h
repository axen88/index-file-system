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
File Name: OS_LINUX_KERNEL.H
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
#ifndef __OS_LINUX_KERNEL_H__
#define __OS_LINUX_KERNEL_H__


#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6, 16))
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#include <linux/kthread.h>
#include <linux/delay.h>

#ifdef	__cplusplus
extern "C" {
#endif

//#define OS_MALLOC                vmalloc
//#define OS_FREE                  vfree
#define OS_MALLOC(size)        kmalloc(size, GFP_KERNEL)
#define OS_FREE                  kfree
#define OS_PRINT                (void)printk
#define OS_SNPRINTF              (void)snprintf
#define OS_VSNPRINTF             vsnprintf

#define OS_STR2ULL(pcBuf, end, base)   simple_strtoull(pcBuf, end, base)
#define OS_SLEEP_SECOND(x)               msleep(x * 1000)
#define OS_SLEEP_MS(x)                  
#define OS_THREAD_EXIT()                do {while (!kthread_should_stop()) msleep(100);} while (0)

typedef struct semaphore            os_mutex_t;
typedef rwlock_t                    os_rwlock;
typedef pid_t                       os_thread_id_t;
typedef struct task_struct *        os_thread_t;

#define INVALID_TID                 NULL

#define OS_GET_THREAD_ID()         current->pid
        
#define OS_MUTEX_INIT(v_pMutex)    init_MUTEX(v_pMutex)
#define OS_MUTEX_LOCK(v_pMutex)    down(v_pMutex)
#define OS_MUTEX_UNLOCK(v_pMutex)  up(v_pMutex)
#define OS_MUTEX_DESTROY(v_pMutex) 

#define OS_RWLOCK_INIT(v_pMutex)      rwlock_init(v_pMutex)
#define OS_RWLOCK_RDLOCK(v_pMutex)    read_lock(v_pMutex)
#define OS_RWLOCK_RDUNLOCK(v_pMutex)  read_unlock(v_pMutex)
#define OS_RWLOCK_WRLOCK(v_pMutex)    write_lock(v_pMutex)
#define OS_RWLOCK_WRUNLOCK(v_pMutex)  write_unlock(v_pMutex)
#define OS_RWLOCK_DESTROY(v_pMutex)

#define ASSERT(x)

static inline os_thread_t thread_create(void *(*func)(void *), void *para, char *thread_name)
{
    os_thread_t tid;

    tid = kthread_run((int (*)(void*))func, para, thread_name);
    if (IS_ERR(tid)
    {
        return NULL;
    }

    return tid;
}

static inline void thread_destroy(os_thread_t tid, bool_t force)
{
    (void)kthread_stop(tid);

    return;
}


#ifdef	__cplusplus
}
#endif


#endif
