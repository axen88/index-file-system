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
File Name: OFS_METADATA_CACHE.H
Author   : axen.hook
Version  : 1.00
Date     : 20/Apr/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 20/Apr/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#ifndef __OFS_METADATA_CACHE_H__
#define __OFS_METADATA_CACHE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define STATUS_MASK   0x00FF
#define STATUS_FLUSH  0x8000

typedef enum cache_status
{
    EMPTY = 0,  /* no data in cache */
    CLEAN,      /* data in buffer is the same to data on disk */
    DIRTY      /* data in buffer read from disk is changed */
} cache_status_t;


#define SET_CACHE_DIRTY(cache)  ((cache)->state = DIRTY)
#define SET_CACHE_CLEAN(cache)  ((cache)->state = CLEAN)
#define SET_CACHE_EMPTY(cache)  ((cache)->state = EMPTY)
#define SET_CACHE_FLUSH(cache)  ((cache)->state |= STATUS_FLUSH)
#define CACHE_DIRTY(cache)      (((cache)->state & STATUS_MASK) == DIRTY)
#define CACHE_CLEAN(cache)      ((cache)->state == CLEAN)
#define CACHE_EMPTY(cache)      ((cache)->state == EMPTY)
#define CACHE_FLUSH(cache)      ((cache)->state & STATUS_FLUSH)


struct ofs_block_cache
{
	uint64_t vbn;
	uint32_t state;
	block_head_t *ib;
	avl_node_t obj_entry; // recorded in object info
	avl_node_t fs_entry;  // recorded in container handle
};

int32_t index_block_read(object_handle_t *obj, uint64_t vbn, uint32_t blk_id);

int32_t alloc_obj_block_and_cache(object_info_t *obj_info, ofs_block_cache_t **cache, uint32_t blk_id);

int32_t release_obj_all_cache(object_info_t *obj_info);

void change_obj_cache_vbn(object_info_t *obj_info, ofs_block_cache_t *cache, uint64_t new_vbn);

int32_t index_block_read2(object_info_t *obj_info, uint64_t vbn, uint32_t blk_id, ofs_block_cache_t **cache_out);

ofs_block_cache_t *alloc_obj_cache(object_info_t *obj_info, uint64_t vbn, uint32_t blk_id);

int32_t release_container_all_cache(container_handle_t *ct);

int32_t flush_container_cache(container_handle_t *ct);

int32_t commit_container_modification(container_handle_t *ct);

void free_obj_cache(object_info_t *obj_info, ofs_block_cache_t *cache);


#ifdef	__cplusplus
}
#endif

#endif

