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

extern int32_t index_update_block(index_handle_t * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);
extern int32_t index_read_block(index_handle_t * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);

extern int32_t index_update_block_fixup(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_read_block_fixup(index_handle_t * hnd, block_head_t * obj,
     uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_block_pingpong_init(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_update_block_pingpong(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn);
extern int32_t index_read_block_pingpong(index_handle_t * hnd, block_head_t * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size);

extern int32_t index_update_sectors(index_handle_t * f, void * buf,
    uint32_t size, uint64_t lba);
extern int32_t index_read_sectors(index_handle_t * f, void * buf,
    uint32_t size, uint64_t lba);

extern int32_t block_update_super_block(index_handle_t * hnd);

#ifdef  __cplusplus
}
#endif

#endif            
