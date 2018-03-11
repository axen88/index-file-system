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
File Name: OS_LOG.H
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

#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PIDS_NUM   256

#define LOG_TO_NULL        0x00
#define LOG_TO_FILE        0x01
#define LOG_TO_SCREEN      0x02
#define LOG_TO_SCNFILE     (LOG_TO_FILE | LOG_TO_SCREEN)

extern void *log_open(const char *file_name, const char *version, const char *dir, uint32_t mode);
extern void log_close(void *log);
extern void log_set_level(void *log, uint32_t pid, uint32_t level);
extern int32_t log_get_level(void *log, uint32_t pid);
extern void log_trace(void *log, uint32_t pid, uint32_t level, const char *format, ...);

extern void *g_log_hnd;

#define LOG_SYSTEM_INIT(dir, name) g_log_hnd = log_open(name, "V100R001C01", dir, LOG_TO_FILE)
#define LOG_SYSTEM_EXIT()          log_close(g_log_hnd);
#define LOG_SET_LEVEL(level)       log_set_level(g_log_hnd, g_pid, level)
#define LOG_GET_LEVEL()            log_get_level(g_log_hnd, g_pid)

#define LOG_DEBUG(fmt, ...)    \
    log_trace(g_log_hnd, g_pid, 4, "[DEBUG][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...)       \
    log_trace(g_log_hnd, g_pid, 3, "[INFO ][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
   
#define LOG_WARN(fmt, ...)         \
    log_trace(g_log_hnd, g_pid, 2, "[WARN ][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
   
#define LOG_ERROR(fmt, ...)        \
    log_trace(g_log_hnd, g_pid, 1, "[ERROR][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
   
#define LOG_EMERG(fmt, ...)        \
    log_trace(g_log_hnd, g_pid, 0, "[EMERG][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOG_EVENT(fmt, ...)        \
    log_trace(g_log_hnd, g_pid, 0, "[EVENT][%lld][%s:%s:%d]: "fmt, \
        (uint64_t)OS_GET_THREAD_ID(),  __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
       
#ifdef __cplusplus
}
#endif

#endif
