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
File Name: OS_LOG.C
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


#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include <direct.h>
#define unlink   _unlink
#define mkdir    _mkdir
#define MkDir(name)  mkdir(name)

#elif defined(__KERNEL__)

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define MkDir(name)  mkdir(name, 0777)
#endif

#include "os_adapter.h"
#include "log.h"
#include "file_if.h"
#include "utils.h"

#define DEFAULT_LEVEL   2
#define MAX_FILE_LINES    200000

#define LOG_NAME_LEN     256
#define LOG_VERSION_LEN   256

#define DATA_TIME_STR_LEN 40
#define BUF_LEN           1024

typedef struct log
{
    char  dir[LOG_NAME_LEN];      
    char  name[LOG_NAME_LEN];      
    char  version[LOG_VERSION_LEN];    
    int32_t total_lines;             
    uint32_t mode;                 
    uint32_t levels[PIDS_NUM]; 
    
    char date_time[DATA_TIME_STR_LEN];
    char buf[BUF_LEN];
    
    os_rwlock   rwlock;                  
    void *disk_hnd;  
} log_t; 

static void *open_log(log_t *log)
{
    char name[LOG_NAME_LEN];
    int32_t ret = 0;

    ASSERT(log);
    
    OS_SNPRINTF(name, LOG_NAME_LEN, "%s/%s.log",
        log->dir, log->name);
    
    ret = os_file_create(&log->disk_hnd, name);
    if (ret < 0)
    {
        return NULL;
    }

    os_file_printf(log->disk_hnd, "%s %s\n", log->name, log->version);
    
    return log->disk_hnd;
}

#ifdef __KERNEL__

static int32_t backup_log(log_t *log)
{
    int32_t ret = 0;

    if ((log->mode & LOG_TO_FILE) == 0)
    {
        return 0;
    }
    
    OS_RWLOCK_WRLOCK(&log->rwlock);
    
    if (!open_log(log))
    {
        ret = -1;
    }

    log->total_lines = 0;

    OS_RWLOCK_WRUNLOCK(&log->rwlock);
    
    return ret;
}

#else

static int32_t backup_log(log_t *log)
{
    int32_t ret = 0;
    char bakName[LOG_NAME_LEN];
    char name[LOG_NAME_LEN];
    time_t t = 0;    
    struct tm *ts = NULL;

    if ((log->mode & LOG_TO_FILE) == 0)
    {
        return 0;
    }
    
    MkDir(log->dir);
    
    OS_RWLOCK_WRLOCK(&log->rwlock);
    
    t = time(NULL);
    ts = localtime(&t);
    if (!ts)
    {
        OS_SNPRINTF(bakName, LOG_NAME_LEN, "%s/%s.log.bak",
            log->dir, log->name);
    }
    else
    {
        OS_SNPRINTF(bakName, LOG_NAME_LEN, "%s/%s_%04d%02d%02d_%02d%02d%02d.log", 
            log->dir, log->name, ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, 
            ts->tm_hour, ts->tm_min, ts->tm_sec);
    }
    
    OS_SNPRINTF(name, LOG_NAME_LEN, "%s/%s.log",
        log->dir, log->name);

    if (log->disk_hnd)
    {
        os_file_close(log->disk_hnd);
        log->disk_hnd = NULL;
    }
    
    unlink(bakName);
    rename(name, bakName);

    if (!open_log(log))
    {
        ret = -1;
    }

    log->total_lines = 0;

    OS_RWLOCK_WRUNLOCK(&log->rwlock);
    
    return ret;
}

#endif

void log_set_level(void *log, uint32_t pid, uint32_t level)
{
    if ((!log) || (pid >= PIDS_NUM))
    {
        return;
    }
    
    ((log_t *)log)->levels[pid] = level;

    return;
}

int32_t log_get_level(void *log, uint32_t pid)
{
    if ((!log) || (pid >= PIDS_NUM))
    {
        return -1;
    }
    
    return ((log_t *)log)->levels[pid];
}

void *log_open(const char *file_name, const char *version, const char *dir, uint32_t mode)
{
    log_t *log = NULL;
    uint32_t i = 0;

    if ((!file_name) || (!version) || (!dir))
    {
        return NULL;
    }
    
    log = (log_t *)OS_MALLOC(sizeof(log_t));
    if (!log)
    {
        return NULL;
    }

    memset(log, 0, sizeof(log_t));
    
    OS_RWLOCK_INIT(&log->rwlock);

    strncpy(log->dir, dir, LOG_NAME_LEN);
    log->dir[LOG_NAME_LEN - 1] = 0;
    strncpy(log->name, file_name, LOG_NAME_LEN);
    log->name[LOG_NAME_LEN - 1] = 0;
    strncpy(log->version, version, LOG_NAME_LEN);
    log->version[LOG_NAME_LEN - 1] = 0;

    log->mode = mode;
    for (i = 0; i < PIDS_NUM; i++)
    {
        log->levels[i] = DEFAULT_LEVEL;
    }
    
    if (0 > backup_log(log))
    {
        OS_FREE(log);
        return NULL;
    }
    
    return log;
}

void log_close(void *log)
{
    log_t *tmp_log = (log_t *)log;
    
    if (!tmp_log)
    {
        return;
    }

    if (tmp_log->disk_hnd)
    {
        char date_time[DATA_TIME_STR_LEN];
        
        os_get_date_time_string(date_time, DATA_TIME_STR_LEN);
        os_file_printf(tmp_log->disk_hnd, "%s %s\n", date_time, "NOTE: LOG FILE CLOSE!!!");
        os_file_close(tmp_log->disk_hnd);
        tmp_log->disk_hnd = NULL;
    }

    OS_RWLOCK_DESTROY(&tmp_log->rwlock);
    OS_FREE(tmp_log);
}

void log_trace(void *log, uint32_t pid, uint32_t level, const char *format, ...)
{
    log_t *tmp_log = (log_t *)log;
    va_list ap;

    if ((tmp_log == NULL) || (pid >= PIDS_NUM) || (level > tmp_log->levels[pid])
        || ((tmp_log->mode & LOG_TO_SCNFILE) == 0)
        || (((tmp_log->mode & LOG_TO_SCREEN) == 0) && (tmp_log->disk_hnd == NULL)))
    {
        return;
    }

    OS_RWLOCK_WRLOCK(&tmp_log->rwlock);
    
    os_get_date_time_string(tmp_log->date_time, DATA_TIME_STR_LEN);

    va_start(ap, format);
    OS_VSNPRINTF(tmp_log->buf, BUF_LEN, format, ap);
    va_end(ap);

    if (tmp_log->mode & LOG_TO_SCREEN)
    {
        //OS_PRINT("%s %s", tmp_log->date_time, tmp_log->buf);
    }
    
    if (tmp_log->disk_hnd)
    {
        os_file_printf(tmp_log->disk_hnd, "%s %s", tmp_log->date_time, tmp_log->buf);
        tmp_log->total_lines++;
    }
    
    
    OS_RWLOCK_WRUNLOCK(&tmp_log->rwlock);
    
    if (tmp_log->total_lines > MAX_FILE_LINES)
    {
        backup_log(tmp_log);
    }
}

void *g_log_hnd = NULL;


