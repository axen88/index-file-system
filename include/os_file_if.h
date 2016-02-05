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
文 件 名: OS_FILE_IF.H
版    本: 1.00
日    期: 2011年6月19日
功能描述: 文件操作接口
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef __OS_FILE_IO_H__
#define __OS_FILE_IO_H__

#ifdef  __cplusplus
extern "C" {
#endif

extern int32_t os_file_exist(const char *path);
extern int32_t os_file_open_or_create(void **hnd, const char *path);
extern int32_t os_file_resize(void *f, uint64_t newSize);
extern int64_t os_file_get_size(void *f);
extern void os_file_set_buf(void *f, void *buf, uint32_t size);

extern int32_t os_file_seek(void *f, uint64_t offset);
extern int32_t os_file_read(void *f, void *buf, uint32_t size);
extern int32_t os_file_write(void *f, void *buf, uint32_t size);

extern int32_t os_file_open(void **hnd, const char *name);
extern int32_t os_file_create(void **hnd, const char *name);
extern int32_t os_file_close(void *f);
extern int32_t os_file_pwrite(void *hnd, void *buf,
    uint32_t size, uint64_t offset);
extern int32_t os_file_pread(void *hnd, void *buf,
    uint32_t size, uint64_t offset);
extern void os_file_printf(void *hnd, const char *format, ...);

#ifdef  __cplusplus
}
#endif


#endif

