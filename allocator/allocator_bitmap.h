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
文 件 名: OS_FILE_BITMAP.H
版    本: 1.00
日    期: 2011年6月19日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef __OS_FILE_BITMAP_H__
#define __OS_FILE_BITMAP_H__

#ifdef  __cplusplus
extern "C"
{
#endif

typedef struct _BITMAP_HANDLE
{
    void *cache;
    uint32_t cache_size_by_bytes;   /* 缓存的大小，以字节为单位 */
    uint32_t cache_size_by_sectors; /* 缓存的大小，以sector为单位 */
    uint32_t dat_size;           /* 被缓存数据的大小 */
    u64_t dat_addr;           /* 内存中的数据位置，从0开始、按cache大小编号 */
    uint8_t status;        /* cache的状态 */
    bool_t pre_flush;         /* 是否需要做预刷盘动作 */

    void *file_hnd;               /* bitmap所在的文件操作句柄 */
    u64_t start_lba;           /* bitmap区域在文件中的起始位置 */
    uint32_t total_sectors;       /* bitmap区域的大小 */

    u64_t total_bits;          /* bitmap区域管理的总块数 */
} BITMAP_HANDLE;

extern int32_t bitmap_init(BITMAP_HANDLE ** hnd, void * file_hnd, u64_t start_lba, uint32_t total_sectors, u64_t total_bits);
extern int32_t bitmap_destroy(BITMAP_HANDLE * hnd);
extern int32_t bitmap_set_nbits(BITMAP_HANDLE * hnd, u64_t start_position,
    uint32_t nbits, bool_t is_used);
extern int32_t bitmap_get_free_bits(BITMAP_HANDLE * hnd, u64_t start_position,
    uint32_t required_bits, u64_t * real_start_position);
extern int32_t bitmap_clean(BITMAP_HANDLE * hnd);
extern int32_t bitmap_check_bit(BITMAP_HANDLE * hnd, u64_t position);

#ifdef  __cplusplus
}
#endif

#endif
