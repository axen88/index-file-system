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
File Name: INDEX_BLOCK_IF.H
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
#ifndef __OS_BLOCK_MAN_H__
#define __OS_BLOCK_MAN_H__

#ifdef  __cplusplus
extern "C"
{
#endif

/* BLOCK_HANDLE_S.flags */
#define FLAG_NOSPACE     0x00000002     // no space
#define FLAG_DIRTY       0x00000001     // dirty

typedef struct tagBLOCK_HANDLE_S
{
    void *file_hnd;                       // file handle
    char name[FILE_NAME_SIZE];            // file name
    BLOCK_BOOT_SECTOR_S sb;               // super block
    uint32_t flags;                       
    OS_RWLOCK rwlock;                     
} BLOCK_HANDLE_S;

extern int32_t block_create(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t total_blocks, uint32_t block_size_shift, uint32_t reserved_sectors,
    uint64_t start_lba);
extern int32_t block_open(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t start_lba);
extern int32_t block_close(BLOCK_HANDLE_S * f);
extern bool_t block_need_fixup(BLOCK_HANDLE_S * hnd);
extern int32_t block_finish_fixup(BLOCK_HANDLE_S * hnd);

extern int32_t index_update_block(BLOCK_HANDLE_S * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);
extern int32_t index_read_block(BLOCK_HANDLE_S * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);

extern int32_t index_update_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_read_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
     uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_block_pingpong_init(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_update_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_read_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_sectors(BLOCK_HANDLE_S * f, void * buf,
    uint32_t size, uint64_t lba);
extern int32_t index_read_sectors(BLOCK_HANDLE_S * f, void * buf,
    uint32_t size, uint64_t lba);

extern int32_t block_update_super_block(BLOCK_HANDLE_S * hnd);

extern int32_t check_and_set_fixup_flag(BLOCK_HANDLE_S * hnd);

#ifdef  __cplusplus
}
#endif

#endif            
