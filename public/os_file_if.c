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

            Copyright(C), 2016~2019, axen2012@qq.com
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

typedef struct tagFILE_HANDLE_S
{
    struct file *file_hnd;
    loff_t offset;
    char buf[FILE_BUF_LEN];
    OS_RWLOCK rwlock;
} FILE_HANDLE_S;

/*******************************************************************************
��������: file_open_or_create
����˵��: �������ߴ��ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
    v_uiFlags: ����flags
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t file_open_or_create(void **hnd, const char *name, uint32_t flags)
{
    FILE_HANDLE_S *tmp_hnd = NULL;

    if ((NULL == hnd) || (NULL == name))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    tmp_hnd = OS_MALLOC(sizeof(FILE_HANDLE_S));
    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_MALLOC;
    }

    memset(tmp_hnd, 0, sizeof(FILE_HANDLE_S));
    
    tmp_hnd->file_hnd = filp_open(name, (int32_t)flags, 0);
    if (IS_ERR(tmp_hnd->file_hnd))
    {
        OS_FREE(tmp_hnd);
    	return -FILE_IO_ERR_OPEN;
    }

    OS_RWLOCK_INIT(&tmp_hnd->rwlock);
    *hnd = tmp_hnd;

    return 0;
}

/*******************************************************************************
��������: OSFileOpen
����˵��: ���ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_open(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, O_RDWR | O_LARGEFILE);
}

/*******************************************************************************
��������: OSFileCreate
����˵��: �����ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, O_RDWR | O_LARGEFILE | O_CREAT | O_TRUNC);
}

/*******************************************************************************
��������: OSFileSeek
����˵��: �趨�ļ��Ķ�дλ��
�������:
    hnd : �ļ��������
    v_ullOffset: Ҫ�趨���ļ�λ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_seek(void *hnd, uint64_t offset)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    
    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    tmp_hnd->offset = offset;
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    
    return 0;
}

/*******************************************************************************
��������: OSFilePWrite
����˵��: д�ļ�ָ��λ��
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
    v_ullOffset: Ҫ�������ļ�λ��
�������: ��
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_pwrite(void *hnd, void *buf,
    uint32_t size, uint64_t offset)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    oldFs = get_fs();
    set_fs(get_ds());

    tmp_hnd->offset = (loff_t)offset;
    ret = vfs_write(tmp_hnd->file_hnd, buf, size, &tmp_hnd->offset);

    set_fs(oldFs);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

/*******************************************************************************
��������: OSFilePRead
����˵��: ���ļ�ָ��λ��
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
    v_ullOffset: Ҫ�������ļ�λ��
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_pread(void *hnd, void *buf,
    uint32_t size, uint64_t offset)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    oldFs = get_fs();
    set_fs(get_ds());

    tmp_hnd->offset = (loff_t)offset;
    ret = vfs_read(tmp_hnd->file_hnd, buf, size, &tmp_hnd->offset);

    set_fs(oldFs);
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

/*******************************************************************************
��������: OSFileWrite
����˵��: д�ļ�
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_write(void *hnd, void *buf, uint32_t size)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    
    oldFs = get_fs();
    set_fs(get_ds());
    ret = vfs_write(tmp_hnd->file_hnd, buf, size, &tmp_hnd->offset);
    set_fs(oldFs);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

/*******************************************************************************
��������: OSFileRead
����˵��: ���ļ�
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_read(void *hnd, void *buf, uint32_t size)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    mm_segment_t oldFs;
    int32_t ret = 0;
    loff_t offset = 0;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);
    
    oldFs = get_fs();
    set_fs(get_ds());
    ret = vfs_read(tmp_hnd->file_hnd, buf, size, &tmp_hnd->offset);
    set_fs(oldFs);
    
    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return ret;
}

/*******************************************************************************
��������: OSFileClose
����˵��: �ر��ļ�
�������:
    hnd : �ļ��������
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_close(void *hnd)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_DESTROY(&tmp_hnd->rwlock);

    if (filp_close(tmp_hnd->file_hnd, NULL) != 0)
    {
    	//return -FILE_IO_ERR_CLOSE;
    }

    OS_FREE(tmp_hnd);

    return 0;
}

/*******************************************************************************
��������: OSFilePrint
����˵��: �����ݰ�ָ����ʽд�뵽����
�������:
    hnd : �ļ��������
    format: ��ʽ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
void os_file_printf(void *hnd, const char *format, ...)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    va_list ap;

    if (NULL == tmp_hnd)
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
#include "os_file_if.h"

typedef struct tagFILE_HANDLE_S
{
    FILE *file_hnd;
    OS_RWLOCK rwlock;
} FILE_HANDLE_S;

/*******************************************************************************
��������: file_open_or_create
����˵��: �������ߴ��ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
    v_pcMethod: ������ʽ
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t file_open_or_create(void **hnd, const char *name, char *v_pcMethod)
{
    FILE_HANDLE_S *tmp_hnd = NULL;

    if ((NULL == hnd) || (NULL == name) || (NULL == v_pcMethod))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    tmp_hnd = OS_MALLOC(sizeof(FILE_HANDLE_S));
    if (NULL == tmp_hnd)
    {
        return FILE_IO_ERR_MALLOC;
    }
    
    memset(tmp_hnd, 0, sizeof(FILE_HANDLE_S));
    
    tmp_hnd->file_hnd = fopen(name, v_pcMethod);
    if (NULL == tmp_hnd->file_hnd)
    {
        OS_FREE(tmp_hnd);
    	return -FILE_IO_ERR_OPEN;
    }

    OS_RWLOCK_INIT(&tmp_hnd->rwlock);
    //MyFileSetBuf(pstHandle->pstFile, NULL, 0); // ��buffer
    
    *hnd = tmp_hnd;
    
    return 0;
}

/*******************************************************************************
��������: OSFileOpen
����˵��: ���ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_open(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, "rb+");
}

/*******************************************************************************
��������: OSFileCreate
����˵��: �����ļ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name, "wb+");
}

/*******************************************************************************
��������: OSFileCreateOrOpen
����˵��: ����ļ������ھʹ�����������ڣ��ʹ�
�������:
    v_pcName: Ҫ�������ļ����ļ���
�������:
    v_ppF: �����ɹ�����ļ����
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_open_or_create(void **hnd, const char *name)
{
    return file_open_or_create(hnd, name,
        (os_file_exist(name) == 0) ? "rb+" : "wb+");
}

/*******************************************************************************
��������: OSFileSeek
����˵��: �趨�ļ��Ķ�дλ��
�������:
    hnd : �ļ��������
    v_ullOffset: Ҫ�趨���ļ�λ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_seek(void *hnd, uint64_t offset)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
    return (_fseeki64(tmp_hnd->file_hnd, offset, SEEK_SET));
#else
    return (fseeko(tmp_hnd->file_hnd, offset, SEEK_SET));
#endif
}

/*******************************************************************************
��������: OSFilePWrite
����˵��: д�ļ�ָ��λ��
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
    v_ullOffset: Ҫ�������ļ�λ��
�������: ��
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_pwrite(void *hnd, void *buf, uint32_t size,
    uint64_t offset)
{
    int32_t ret = 0;
    FILE_HANDLE_S *tmp_hnd = hnd;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
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

/*******************************************************************************
��������: OSFilePRead
����˵��: ���ļ�ָ��λ��
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
    v_ullOffset: Ҫ�������ļ�λ��
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_pread(void *hnd, void *buf, uint32_t size,
    uint64_t offset)
{
    int32_t ret = 0;
    FILE_HANDLE_S *tmp_hnd = hnd;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
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

/*******************************************************************************
��������: OSFileWrite
����˵��: д�ļ�
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_write(void *hnd, void *buf, uint32_t size)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    return (fwrite(buf, 1, size, tmp_hnd->file_hnd));
    //fflush((FILE *)pF);
}

/*******************************************************************************
��������: OSFileRead
����˵��: ���ļ�
�������:
    hnd : �ļ��������
    buf : ���ݻ���
    size : ���ݴ�С
�� �� ֵ:
    >=0: �����ɹ�������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_read(void *hnd, void *buf, uint32_t size)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if ((NULL == tmp_hnd) || (NULL == buf) || (0 == size))
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    return (fread(buf, 1, size, tmp_hnd->file_hnd));
}

/*******************************************************************************
��������: OSFileClose
����˵��: �ر��ļ�
�������:
    hnd : �ļ��������
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_close(void *hnd)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

    OS_RWLOCK_DESTROY(&tmp_hnd->rwlock);

    if (fclose(tmp_hnd->file_hnd) != 0)
    {
    	//return -FILE_IO_ERR_CLOSE;
    }

    OS_FREE(tmp_hnd);

    return 0;
}

/*******************************************************************************
��������: OSFileResize
����˵��: �����ļ���С
�������:
    hnd : �ļ��������
    new_size: �ļ��´�С
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_file_resize(void *hnd, uint64_t new_size)
{
	int32_t fd = 0;
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
	fd = _fileno(tmp_hnd->file_hnd);
	return _chsize_s(fd, new_size);
#else
	fd = fileno(tmp_hnd->file_hnd);
	return ftruncate(fd, new_size);
#endif
}

/*******************************************************************************
��������: OSFileGetSize
����˵��: ��ȡ�ļ���С
�������:
    hnd : �ļ��������
�������: ��
�� �� ֵ:
    >=0: �ļ���С
    < 0: �������
˵    ��: ��
*******************************************************************************/
int64_t os_file_get_size(void *hnd)
{
    int64_t offset = 0;
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == tmp_hnd)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }
    
    OS_RWLOCK_WRLOCK(&tmp_hnd->rwlock);

#ifdef WIN32
    if (_fseeki64(tmp_hnd->file_hnd, 0, SEEK_END))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    offset = _ftelli64(tmp_hnd->file_hnd);
#else
    if (fseeko(tmp_hnd->file_hnd, 0, SEEK_END))
    {
        OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);
    	return -FILE_IO_ERR_SEEK;
    }
    
    offset = ftell(tmp_hnd->file_hnd);
#endif

    OS_RWLOCK_WRUNLOCK(&tmp_hnd->rwlock);

    return offset;
}

/*******************************************************************************
��������: OSFileSetBuf
����˵��: �����ļ�����
�������:
    hnd : �ļ��������
    buf: �»���
    size: �����С
�������: ��
�� �� ֵ: ��
˵    ��: ��
*******************************************************************************/
void os_file_set_buf(void *hnd, void *buf, uint32_t size)
{
    FILE_HANDLE_S *tmp_hnd = hnd;

    if (NULL == tmp_hnd)
    {
        return;
    }
    
#ifdef WIN32
    setvbuf(tmp_hnd->file_hnd, buf, _IONBF, size);
#else
    setbuf(tmp_hnd->file_hnd, buf);
#endif
}

/*******************************************************************************
��������: OSFileExist
����˵��: ����ļ��Ƿ����
�������:
    v_pcName: Ҫ����ļ����ļ���
�������: ��
�� �� ֵ:
    ==0: ����
    !=: ������߲�����
˵    ��: ��
*******************************************************************************/
int32_t os_file_exist(const char *name)
{
    if (NULL == name)
    {
        return -FILE_IO_ERR_INVALID_PARA;
    }

#ifdef WIN32
    return (_access(name, 0));
#else
    return (access(name, 0));
#endif
}

/*******************************************************************************
��������: OSFilePrint
����˵��: �����ݰ�ָ����ʽд�뵽����
�������:
    hnd : �ļ��������
    format: ��ʽ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
void os_file_printf(void *hnd, const char *format, ...)
{
    FILE_HANDLE_S *tmp_hnd = hnd;
    va_list ap;

    if (NULL == tmp_hnd)
    {
        return;
    }

    va_start(ap, format);
    (void)vfprintf(tmp_hnd->file_hnd, format, ap);
    va_end(ap);

    return;
}

#endif

