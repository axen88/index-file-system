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

typedef struct block_handle
{
    void *file_hnd;                       // file handle
    char name[FILE_NAME_SIZE];            // file name
    ifs_super_block_t sb;               // super block
    uint32_t flags;                       
    os_rwlock rwlock;                     
} block_handle_t;

extern int32_t block_create(block_handle_t ** hnd, const char * path,
    uint64_t total_blocks, uint32_t block_size_shift, uint32_t reserved_sectors,
    uint64_t start_lba);
extern int32_t block_open(block_handle_t ** hnd, const char * path,
    uint64_t start_lba);
extern int32_t block_close(block_handle_t * f);
extern bool_t block_need_fixup(block_handle_t * hnd);
extern int32_t block_finish_fixup(block_handle_t * hnd);

extern int32_t index_update_block(block_handle_t * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);
extern int32_t index_read_block(block_handle_t * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);

extern int32_t index_update_block_fixup(block_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_read_block_fixup(block_handle_t * hnd, block_head_t * obj,
     uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_block_pingpong_init(block_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_update_block_pingpong(block_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_read_block_pingpong(block_handle_t * hnd, block_head_t * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_sectors(block_handle_t * f, void * buf,
    uint32_t size, uint64_t lba);
extern int32_t index_read_sectors(block_handle_t * f, void * buf,
    uint32_t size, uint64_t lba);

extern int32_t block_update_super_block(block_handle_t * hnd);

extern int32_t check_and_set_fixup_flag(block_handle_t * hnd);

#ifdef  __cplusplus
}
#endif

#endif            
