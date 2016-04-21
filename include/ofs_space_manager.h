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
File Name: OFS_SPACE_MANAGER.H
Author   : axen.hook
Version  : 1.00
Date     : 20/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 20/Mar/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/
#ifndef __OFS_SPACE_MANAGER_H__
#define __OFS_SPACE_MANAGER_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_BLK_NUM  10
#define MIN_BLK_NUM  3

struct space_manager
{
    object_handle_t *space_obj;

    uint64_t first_free_block;
    uint64_t total_free_blocks;

    os_rwlock lock;
};

int32_t alloc_space(object_handle_t *obj, uint64_t start_blk, uint32_t blk_cnt, uint64_t *real_start_blk);
int32_t free_space(object_handle_t *obj, uint64_t start_blk, uint32_t blk_cnt);

#define OFS_ALLOC_BLOCK(ct, objid, vbn) ofs_alloc_space(ct, objid, 1, vbn)
#define OFS_FREE_BLOCK(ct, objid, vbn)  ofs_free_space(ct, objid, vbn, 1)

int32_t ofs_alloc_space(container_handle_t *ct, uint64_t objid, uint32_t blk_cnt, uint64_t *real_start_blk);
int32_t ofs_free_space(container_handle_t *ct, uint64_t objid, uint64_t start_blk, uint32_t blk_cnt);

// space manager API
void ofs_init_sm(space_manager_t *sm, object_handle_t *obj, uint64_t first_free_block, uint64_t total_free_blocks);
int32_t ofs_init_free_space(space_manager_t *sm, uint64_t start_blk, uint64_t blk_cnt);
int32_t sm_alloc_space(space_manager_t *sm, uint32_t blk_cnt, uint64_t *real_start_blk);
int32_t sm_free_space(space_manager_t *sm, uint64_t start_blk, uint32_t blk_cnt);
void ofs_destroy_sm(space_manager_t *sm);


#ifdef	__cplusplus
}
#endif

#endif

