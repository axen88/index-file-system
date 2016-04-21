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
File Name: OFS_IF.H
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
#ifndef __OFS_IF_H__
#define __OFS_IF_H__

#include "avl.h"
#include "os_adapter.h"
#include "os_list_double.h"
#include "os_queue.h"
#include "os_disk_if.h"
#include "os_utils.h"
#include "os_threads_group.h"
#include "os_cmd_ui.h"

#include "ofs_layout.h"
#include "ofs_collate.h"

typedef struct space_manager space_manager_t;
typedef struct ofs_block_cache ofs_block_cache_t;
typedef struct object_info object_info_t;
typedef struct object_handle object_handle_t;
typedef struct container_handle container_handle_t;

#include "ofs_metadata_cache.h"
#include "ofs_space_manager.h"
#include "ofs_tree.h"
#include "ofs_object.h"
#include "ofs_container.h"
#include "ofs_block.h"
#include "ofs_tools_if.h"

// container API
int32_t ofs_open_container(const char *ct_name, container_handle_t **ct);
int32_t ofs_create_container(const char *ct_name, uint64_t total_sectors, container_handle_t **ct);
int32_t ofs_close_container(container_handle_t *ct);

// space manager API
void ofs_init_sm(space_manager_t *sm, object_handle_t *obj, uint64_t first_free_block, uint64_t total_free_blocks);
int32_t ofs_init_free_space(space_manager_t *sm, uint64_t start_blk, uint64_t blk_cnt);
int32_t sm_alloc_space(space_manager_t *sm, uint32_t blk_cnt, uint64_t *real_start_blk);
int32_t sm_free_space(space_manager_t *sm, uint64_t start_blk, uint32_t blk_cnt);
void ofs_destroy_sm(space_manager_t *sm);

// object API
int32_t ofs_open_object(container_handle_t *ct, uint64_t objid, object_handle_t **obj);
int32_t ofs_create_object(container_handle_t *ct, uint64_t objid, uint16_t flags, object_handle_t **obj);
int32_t ofs_close_object(object_handle_t *obj);
int32_t ofs_delete_object(container_handle_t *ct, uint64_t objid);
int32_t ofs_rename_object(object_handle_t *obj, const char *new_obj_name);
int32_t ofs_set_object_name(object_handle_t *obj, char *name);

// table/KV/index API
int32_t index_search_key(object_handle_t *obj, const void *key, uint16_t key_len);
int32_t index_remove_key(object_handle_t *obj, const void *key, uint16_t key_len);
int32_t index_insert_key(object_handle_t *obj, const void *key, uint16_t key_len, const void *value, uint16_t value_len);
int32_t index_update_value(object_handle_t * tree, const void * key, uint16_t key_len, const void *value, uint16_t value_len);
int32_t index_walk_all(object_handle_t *obj, bool_t reverse, uint8_t flags, void *para, tree_walk_cb_t cb);

// cache API


// object read/write API


#endif


