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
文 件 名: OS_DISK_IF.H
版    本: 1.00
日    期: 2011年6月23日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月23日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef __OS_DISK_IF_H__
#define __OS_DISK_IF_H__

#ifdef __EN_FILE_IF__

#include "os_file_if.h"

#define os_disk_open(hnd, path)    os_file_open(hnd, path)
#define os_disk_create(hnd, path)  os_file_create(hnd, path)
#define os_disk_close(hnd)            os_file_close(hnd)

#define os_disk_pwrite(hnd, buf, size, start_lba) \
    os_file_pwrite(hnd, buf, size, (start_lba) << BYTES_PER_SECTOR_SHIFT)
    
#define os_disk_pread(hnd, buf, size, start_lba) \
    os_file_pread(hnd, buf, size, (start_lba) << BYTES_PER_SECTOR_SHIFT)

#else

#endif

#endif
