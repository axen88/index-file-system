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
File Name: OS_FILE_IF.C
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

#ifdef __KERNEL__

#include <linux/version.h> 
#include <linux/fs.h> 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <asm/uaccess.h> 
#else
#include <linux/uaccess.h> 
#endif

#include "os_adapter.h"

#define FILE_BUF_LEN  1024

typedef struct file_handle
{
    struct file *disk_hnd;
    loff_t offset;
    char buf[FILE_BUF_LEN];
    os_rwlock rwlock;
} file_handle_t;

int32_t file_open_or_create(void **hnd, const char *name, uint32_t flags)
{
    file_handle_t *tmp_hnd = NULL;

    if ((!hnd) || (!name))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    tmp_hnd = OS_MALLOC(sizeof(file_handle_t));
    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_MALLOC;
    }

    memset(tmp_hnd, 0, sizeof(file_handle_t));
    
    tmp_hnd->disk_hnd = filp_open(name, (int32_t)flags, 0);
    if (IS_ERR(tmp_hnd->disk_hnd))
    {
        OS_FREE(tmp_hnd);
    	return -FILE_IO_ERR_OPEN;
    }

    OS_RWLOCK_INIT(&tmp_hnd->rwlock);
    *hnd = tmp_hnd;

    return 0;
}

int32_t os_file_open(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, O_RDWR | O_LARGEFILE);
}

int32_t os_file_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, O_RDWR | O_LARGEFILE | O_CREAT | O_TRUNC);
}

int32_t os_file_seek(void *hnd, u64_t offset)
{
    file_handle_t *tmp_hnd = hnd;
    
    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    tmp_hnd->offset = offset;
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    
    return 0;
}

int32_t os_file_pwrite(void *hnd, void *buf,
    uint32_t size, u64_t offset)
{
    file_handle_t *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    oldFs = get_fs();
    set_fs(get_ds());

    tmp_hnd->offset = (loff_t)offset;
    ret = vfs_write(tmp_hnd->disk_hnd, buf, size, &tmp_hnd->offset);

    set_fs(oldFs);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

int32_t os_file_pread(void *hnd, void *buf,
    uint32_t size, u64_t offset)
{
    file_handle_t *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    oldFs = get_fs();
    set_fs(get_ds());

    tmp_hnd->offset = (loff_t)offset;
    ret = vfs_read(tmp_hnd->disk_hnd, buf, size, &tmp_hnd->offset);

    set_fs(oldFs);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

int32_t os_file_write(void *hnd, void *buf, uint32_t size)
{
    file_handle_t *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    
    oldFs = get_fs();
    set_fs(get_ds());
    ret = vfs_write(tmp_hnd->disk_hnd, buf, size, &tmp_hnd->offset);
    set_fs(oldFs);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

int32_t os_file_read(void *hnd, void *buf, uint32_t size)
{
    file_handle_t *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;
    loff_t offset = 0;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    
    oldFs = get_fs();
    set_fs(get_ds());
    ret = vfs_read(tmp_hnd->disk_hnd, buf, size, &tmp_hnd->offset);
    set_fs(oldFs);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

int32_t os_file_close(void *hnd)
{
    file_handle_t *tmp_hnd = hnd;

    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_DESTROY(&tmp_hnd->rwlock);

    if (filp_close(tmp_hnd->disk_hnd, NULL) != 0)
    {
    	//return -FILE_IO_ERR_CLOSE;
    }

    OS_FREE(tmp_hnd);

    return 0;
}

void os_file_printf(void *hnd, const char *format, ...)
{
    file_handle_t *tmp_hnd = hnd;
    va_list ap;

    if (tmp_hnd == NULL)
    {
        return;
    }

    va_start(ap,format);
    OS_VSNPRINTF(tmp_hnd->buf, FILE_BUF_LEN, format, ap);
    va_end(ap);

    (void)os_file_write(tmp_hnd, tmp_hnd->buf, strlen(tmp_hnd->buf));

    return;
}

#include <linux/module.h>

EXPORT_SYMBOL(os_file_open);
EXPORT_SYMBOL(os_file_create);
EXPORT_SYMBOL(os_file_pwrite);
EXPORT_SYMBOL(os_file_pread);
EXPORT_SYMBOL(os_file_read);
EXPORT_SYMBOL(os_file_write);
EXPORT_SYMBOL(os_file_seek);
EXPORT_SYMBOL(os_file_close);

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "os_adapter.h"
#include "file_if.h"

typedef struct file_handle
{
    FILE *disk_hnd;
    os_rwlock rwlock;
} file_handle_t;

int32_t file_open_or_create(void **hnd, const char *name, char *v_pcMethod)
{
    file_handle_t *tmp_hnd = NULL;

    if ((!hnd) || (!name) || (!v_pcMethod))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    tmp_hnd = OS_MALLOC(sizeof(file_handle_t));
    if (tmp_hnd == NULL)
    {
        return FILE_IO_ERR_MALLOC;
    }
    
    memset(tmp_hnd, 0, sizeof(file_handle_t));
    
    tmp_hnd->disk_hnd = fopen(name, v_pcMethod);
    if (tmp_hnd->disk_hnd == NULL)
    {
        OS_FREE(tmp_hnd);
    	return -FILE_IO_ERR_OPEN;
    }

    OS_RWLOCK_INIT(&tmp_hnd->rwlock);
    //MyFileSetBuf(pstHandle->pstFile, NULL, 0); // ÎÞbuffer
    
    *hnd = tmp_hnd;
    
    return 0;
}

int32_t os_file_open(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, "rb+");
}

int32_t os_file_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, "wb+");
}

int32_t os_file_open_or_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name,
        (os_file_exist(name) == 0) ? "rb+" : "wb+");
}

int32_t os_file_seek(void *hnd, u64_t offset)
{
    file_handle_t *tmp_hnd = hnd;

    if (!hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
    return (_fseeki64(tmp_hnd->disk_hnd, offset, SEEK_SET));
#else
    return (fseeko(tmp_hnd->disk_hnd, offset, SEEK_SET));
#endif
}

int32_t os_file_pwrite(void *hnd, void *buf, uint32_t size,
    u64_t offset)
{
    int32_t ret = 0;
    file_handle_t *tmp_hnd = hnd;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }
    
    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

    if (os_file_seek(tmp_hnd, offset))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    ret = os_file_write(tmp_hnd, buf, size);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

int32_t os_file_pread(void *hnd, void *buf, uint32_t size,
    u64_t offset)
{
    int32_t ret = 0;
    file_handle_t *tmp_hnd = hnd;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }
    
    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

    if (os_file_seek(tmp_hnd, offset))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    ret = os_file_read(tmp_hnd, buf, size);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    
    return ret;
}

int32_t os_file_write(void *hnd, void *buf, uint32_t size)
{
    file_handle_t *tmp_hnd = hnd;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    return (fwrite(buf, 1, size, tmp_hnd->disk_hnd));
    //fflush((FILE *)pF);
}

int32_t os_file_read(void *hnd, void *buf, uint32_t size)
{
    file_handle_t *tmp_hnd = hnd;

    if ((tmp_hnd == NULL) || (buf == NULL) || (size == 0))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    return (fread(buf, 1, size, tmp_hnd->disk_hnd));
}

int32_t os_file_close(void *hnd)
{
    file_handle_t *tmp_hnd = hnd;

    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_DESTROY(&tmp_hnd->rwlock);

    if (fclose(tmp_hnd->disk_hnd) != 0)
    {
    	//return -FILE_IO_ERR_CLOSE;
    }

    OS_FREE(tmp_hnd);

    return 0;
}

int32_t os_file_resize(void *hnd, u64_t new_size)
{
	int32_t fd = 0;
    file_handle_t *tmp_hnd = hnd;

    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
	fd = _fileno(tmp_hnd->disk_hnd);
	return _chsize_s(fd, new_size);
#else
	fd = fileno(tmp_hnd->disk_hnd);
	return ftruncate(fd, new_size);
#endif
}

int64_t os_file_get_size(void *hnd)
{
    int64_t offset = 0;
    file_handle_t *tmp_hnd = hnd;

    if (tmp_hnd == NULL)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }
    
    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

#ifdef WIN32
    if (_fseeki64(tmp_hnd->disk_hnd, 0, SEEK_END))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    offset = _ftelli64(tmp_hnd->disk_hnd);
#else
    if (fseeko(tmp_hnd->disk_hnd, 0, SEEK_END))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    offset = ftell(tmp_hnd->disk_hnd);
#endif

    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return offset;
}

void os_file_set_buf(void *hnd, void *buf, uint32_t size)
{
    file_handle_t *tmp_hnd = hnd;

    if (tmp_hnd == NULL)
    {
        return;
    }
    
#ifdef WIN32
    setvbuf(tmp_hnd->disk_hnd, buf, _IONBF, size);
#else
    setbuf(tmp_hnd->disk_hnd, buf);
#endif
}

int32_t os_file_exist(const char *name)
{
    if (!name)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
    return (_access(name, 0));
#else
    return (access(name, 0));
#endif
}

void os_file_printf(void *hnd, const char *format, ...)
{
    file_handle_t *tmp_hnd = hnd;
    va_list ap;

    if (tmp_hnd == NULL)
    {
        return;
    }

    va_start(ap, format);
    (void)vfprintf(tmp_hnd->disk_hnd, format, ap);
    va_end(ap);

    return;
}

#endif

