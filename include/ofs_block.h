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
File Name: OFS_BLOCK.H
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
#ifndef __OFS_BLOCK_H__
#define __OFS_BLOCK_H__

#ifdef  __cplusplus
extern "C"
{
#endif

int32_t ofs_update_block(container_handle_t *ct, void *buf, uint32_t size, uint32_t start_lba, uint64_t vbn);
int32_t ofs_read_block(container_handle_t *ct, void *buf, uint32_t size, uint32_t start_lba, uint64_t vbn);

int32_t ofs_update_block_fixup(container_handle_t *ct, block_head_t *blk, uint64_t vbn);
int32_t ofs_read_block_fixup(container_handle_t *ct, block_head_t *blk, uint64_t vbn, uint32_t objid, uint32_t alloc_size);

int32_t ofs_update_block_pingpong_init(container_handle_t *ct, block_head_t *blk, uint64_t vbn);
int32_t ofs_update_block_pingpong(container_handle_t *ct, block_head_t *blk, uint64_t vbn);
int32_t ofs_read_block_pingpong(container_handle_t *ct, block_head_t *blk, uint64_t vbn, uint32_t objid, uint32_t alloc_size);

int32_t ofs_update_sectors(container_handle_t *ct, void *buf, uint32_t size, uint64_t lba);
int32_t ofs_read_sectors(container_handle_t *ct, void *buf, uint32_t size, uint64_t lba);

static int32_t ofs_update_super_block(container_handle_t *ct)
{
    return ofs_update_block_pingpong(ct, &ct->sb.head, SUPER_BLOCK_VBN);
}

static int32_t ofs_read_super_block(container_handle_t *ct)
{
    return ofs_read_block_pingpong(ct, &ct->sb.head, SUPER_BLOCK_VBN, SUPER_BLOCK_ID, SUPER_BLOCK_SIZE);
}

#ifdef  __cplusplus
}
#endif

#endif            
