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
文 件 名: OS_LINUX_KERNEL.H
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

//#define OS_MALLOC                vmalloc
//#define OS_FREE                  vfree
#define OS_MALLOC(size)        kmalloc(size, GFP_KERNEL)
#define OS_FREE                  kfree
#define OS_PRINT                (void)printk
#define OS_SNPRINTF              (void)snprintf
#define OS_VSNPRINTF             vsnprintf

#define OSStrToUll(pcBuf, end, base)   simple_strtoull(pcBuf, end, base)
#define OS_SLEEP_SECOND(x)               msleep(x * 1000)
#define OS_SLEEP_MS(x)                  
#define OSThreadExit()                while (!kthread_should_stop()) msleep(100);

typedef struct semaphore            OS_MUTEX;
typedef rwlock_t                    OS_RWLOCK;
typedef pid_t                       OS_THREAD_ID;
typedef struct task_struct *        OS_THREAD_T;
        
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

#endif
