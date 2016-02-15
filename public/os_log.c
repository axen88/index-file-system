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
文 件 名: OS_LOG.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 日志系统
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
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
#include "os_log.h"
#include "os_file_if.h"
#include "os_utils.h"

#define DEFAULT_LEVEL   2
#define MAX_FILE_LINES    200000

#define LOG_NAME_LEN     256
#define LOG_VERSION_LEN   256

#define DATA_TIME_STR_LEN 40
#define BUF_LEN           1024

typedef struct tagLOG_S
{
    char  dir[LOG_NAME_LEN];              // 日志文件所在的目录
    char  name[LOG_NAME_LEN];         // 日志文件名
    char  version[LOG_VERSION_LEN];        // 版本信息
    int32_t total_lines;                      // 总行数计数
    uint32_t mode;                       // 日志输出位置：nul,文件,屏幕,both
    uint32_t levels[PIDS_NUM];  // 日志级别
    
    char date_time[DATA_TIME_STR_LEN];
    char buf[BUF_LEN];
    
    OS_RWLOCK   rwlock;                            // 日志锁
    void *file_hnd;                             // 日志文件句柄
} LOG_S; 

/*******************************************************************************
函数名称: open_log
功能说明: 创建日志文件
输入参数:
    log: 日志操作句柄
输出参数: 无
返 回 值:
    !=NULL: 成功，文件的操作句柄
    ==NULL: 失败
说    明: 无
*******************************************************************************/
static void *open_log(LOG_S *log)
{
    char name[LOG_NAME_LEN];
    int32_t ret = 0;

    ASSERT(NULL != log);
    
    OS_SNPRINTF(name, LOG_NAME_LEN, "%s/%s.log",
        log->dir, log->name);
    
    ret = os_file_create(&log->file_hnd, name);
    if (0 > ret)
    {
        return NULL;
    }

    os_file_printf(log->file_hnd, "%s %s\n", log->name, log->version);
    
    return log->file_hnd;
}

#ifdef __KERNEL__

/*******************************************************************************
函数名称: backup_log
功能说明: 备份日志
输入参数:
    log: 日志操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int32_t backup_log(LOG_S *log)
{
    int32_t ret = 0;

    if (0 == (log->mode & LOG_TO_FILE))
    {
        return 0;
    }
    
    OS_RWLOCK_WRLOCK(&log->rwlock);
    
    if (NULL == open_log(log))
    {
        ret = -1;
    }

    log->total_lines = 0;

    OS_RWLOCK_WRUNLOCK(&log->rwlock);
    
    return ret;
}

#else

/*******************************************************************************
函数名称: backup_log
功能说明: 备份日志
输入参数:
    log: 日志操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int32_t backup_log(LOG_S *log)
{
    int32_t ret = 0;
    char bakName[LOG_NAME_LEN];
    char name[LOG_NAME_LEN];
    time_t t = 0;    
    struct tm *ts = NULL;

    if (0 == (log->mode & LOG_TO_FILE))
    {
        return 0;
    }
    
    MkDir(log->dir);
    
    OS_RWLOCK_WRLOCK(&log->rwlock);
    
    t = time(NULL);
    ts = localtime(&t);
    if (NULL == ts)
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

    /* 关闭之前已经打开的文件 */
    if (NULL != log->file_hnd)
    {
        os_file_close(log->file_hnd);
        log->file_hnd = NULL;
    }
    
    unlink(bakName);
    rename(name, bakName);

    if (NULL == open_log(log))
    {
        ret = -1;
    }

    log->total_lines = 0;

    OS_RWLOCK_WRUNLOCK(&log->rwlock);
    
    return ret;
}

#endif

/*******************************************************************************
函数名称: log_set_level
功能说明: 调整日志级别
输入参数:
    log: 日志操作句柄
    pid: 要调整的模块id
    level: 要调整到的日志级别
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void log_set_level(void *log, uint32_t pid, uint32_t level)
{
    if ((NULL == log) || (pid >= PIDS_NUM))
    {
        return;
    }
    
    ((LOG_S *)log)->levels[pid] = level;

    return;
}

/*******************************************************************************
函数名称: log_get_level
功能说明: 获取日志级别
输入参数:
    log: 日志操作句柄
    pid: 要调整的模块id
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
int32_t log_get_level(void *log, uint32_t pid)
{
    if ((NULL == log) || (pid >= PIDS_NUM))
    {
        return -1;
    }
    
    return ((LOG_S *)log)->levels[pid];
}

/*******************************************************************************
函数名称: log_open
功能说明: 初始化日志文件
输入参数:
    file_name: 要创建的日志文件的名称
    version : 版本信息
    dir     : 日志文件所在的目录
    mode: 日志模式
输出参数: 无
返 回 值:
    !=NULL: 成功，日志操作句柄
    ==NULL: 失败
说    明: 无
*******************************************************************************/
void *log_open(const char *file_name, const char *version, const char *dir, uint32_t mode)
{
    LOG_S *log = NULL;
    uint32_t i = 0;

    if ((NULL == file_name) || (NULL == version) || (NULL == dir))
    {
        return NULL;
    }
    
    log = (LOG_S *)OS_MALLOC(sizeof(LOG_S));
    if (NULL == log)
    {
        return NULL;
    }

    memset(log, 0, sizeof(LOG_S));
    
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

/*******************************************************************************
函数名称: log_close
功能说明: 关闭日志文件
输入参数:
    log: 日志文件句柄
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void log_close(void *log)
{
    LOG_S *tmp_log = (LOG_S *)log;
    
    if (NULL == tmp_log)
    {
        return;
    }

    if (NULL != tmp_log->file_hnd)
    {
        char date_time[DATA_TIME_STR_LEN];
        
        os_get_date_time_string(date_time, DATA_TIME_STR_LEN);
        os_file_printf(tmp_log->file_hnd, "%s %s\n", date_time, "NOTE: LOG FILE CLOSE!!!");
        os_file_close(tmp_log->file_hnd);
        tmp_log->file_hnd = NULL;
    }

    OS_RWLOCK_DESTROY(&tmp_log->rwlock);
    OS_FREE(tmp_log);
}

/*******************************************************************************
函数名称: log_trace
功能说明: 添加一条日志
输入参数:
    log : 日志操作句柄
    pid: 模块号
    level: 当前这一条日志的级别
    format : 日志格式
输出参数: 无
返 回 值: 无
说    明: 无
*******************************************************************************/
void log_trace(void *log, uint32_t pid, uint32_t level, const char *format, ...)
{
    LOG_S *tmp_log = (LOG_S *)log;
    va_list ap;

    if ((NULL == tmp_log) || (pid >= PIDS_NUM) || (level > tmp_log->levels[pid])
        || (0 == (tmp_log->mode & LOG_TO_SCNFILE))
        || ((0 == (tmp_log->mode & LOG_TO_SCREEN)) && (NULL == tmp_log->file_hnd)))
    {
        return;
    }

    OS_RWLOCK_WRLOCK(&tmp_log->rwlock);
    
    os_get_date_time_string(tmp_log->date_time, DATA_TIME_STR_LEN);

    va_start(ap, format);
    OS_VSNPRINTF(tmp_log->buf, BUF_LEN, format, ap);
    va_end(ap);

    if (0 != (tmp_log->mode & LOG_TO_SCREEN))
    {
        //OS_PRINT("%s %s", tmp_log->date_time, tmp_log->buf);
    }
    
    if (NULL != tmp_log->file_hnd)
    {
        os_file_printf(tmp_log->file_hnd, "%s %s", tmp_log->date_time, tmp_log->buf);
        tmp_log->total_lines++;
    }
    
    
    OS_RWLOCK_WRUNLOCK(&tmp_log->rwlock);
    
    if (tmp_log->total_lines > MAX_FILE_LINES)
    {
        backup_log(tmp_log);
    }
}

void *g_log_hnd = NULL;


