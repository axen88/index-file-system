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
File Name: OS_FILE_IF.H
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

#ifndef __OS_FILE_IO_H__
#define __OS_FILE_IO_H__

#ifdef  __cplusplus
extern "C" {
#endif

enum file_io_error_code
{
    FILE_IO_ERR_START = 100000,
    FILE_IO_ERR_SEEK,
    FILE_IO_ERR_MALLOC,
    FILE_IO_ERR_OPEN,
    FILE_IO_ERR_CREATE,
    FILE_IO_ERR_READ,
    FILE_IO_ERR_WRITE,
    FILE_IO_ERR_CLOSE,
    FILE_IO_ERR_INVALID_PARA,

    FILE_IO_ERR_BUTT
};

extern int32_t os_file_exist(const char *path);
extern int32_t os_file_open_or_create(void **hnd, const char *path);
extern int32_t os_file_resize(void *f, u64_t newSize);
extern int64_t os_file_get_size(void *f);
extern void os_file_set_buf(void *f, void *buf, uint32_t size);

extern int32_t os_file_seek(void *f, u64_t offset);
extern int32_t os_file_read(void *f, void *buf, uint32_t size);
extern int32_t os_file_write(void *f, void *buf, uint32_t size);

extern int32_t os_file_open(void **hnd, const char *name);
extern int32_t os_file_create(void **hnd, const char *name);
extern int32_t os_file_close(void *f);
extern int32_t os_file_pwrite(void *hnd, void *buf,
    uint32_t size, u64_t offset);
extern int32_t os_file_pread(void *hnd, void *buf,
    uint32_t size, u64_t offset);
extern void os_file_printf(void *hnd, const char *format, ...);

#ifdef  __cplusplus
}
#endif


#endif

