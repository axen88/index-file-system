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
File Name: INDEX_BITMAP.H
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
#ifndef __OS_FILE_BITMAP_H__
#define __OS_FILE_BITMAP_H__

#ifdef  __cplusplus
extern "C"
{
#endif

typedef struct _BITMAP_HANDLE
{
    void *cache;
    uint32_t cache_size_by_bytes;   
    uint32_t cache_size_by_sectors; 
    uint32_t dat_size;           // data size
    uint64_t dat_addr;           // data address
    uint8_t status;              // cache state
    bool_t pre_flush;            // need pre flush?

    void *file_hnd;               // file handle
    uint64_t start_lba;           // bitmap zone's start lba in the file
    uint32_t total_sectors;       // bitmap zone's size

    uint64_t total_bits;          // total usable bits
} BITMAP_HANDLE;

extern int32_t bitmap_init(BITMAP_HANDLE ** hnd, void * file_hnd, uint64_t start_lba, uint32_t total_sectors, uint64_t total_bits);
extern int32_t bitmap_destroy(BITMAP_HANDLE * hnd);
extern int32_t bitmap_set_nbits(BITMAP_HANDLE * hnd, uint64_t start_position,
    uint32_t nbits, bool_t is_used);
extern int32_t bitmap_get_free_bits(BITMAP_HANDLE * hnd, uint64_t start_position,
    uint32_t required_bits, uint64_t * real_start_position);
extern int32_t bitmap_clean(BITMAP_HANDLE * hnd);
extern int32_t bitmap_check_bit(BITMAP_HANDLE * hnd, uint64_t position);

#ifdef  __cplusplus
}
#endif

#endif
