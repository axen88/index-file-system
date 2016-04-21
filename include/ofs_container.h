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
File Name: OFS_CONTAINER.H
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

#ifndef __OFS_CONTAINER_H__
#define __OFS_CONTAINER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define OFS_NAME_SIZE     256

#define FLAG_DIRTY       0x00000001     // dirty

#define MIN_BLOCKS_NUM   10

struct container_handle
{
    char name[OFS_NAME_SIZE];        // ct name

    void *disk_hnd;                       // file handle
    ofs_super_block_t sb;               // super block
    uint32_t flags;                       

    object_handle_t *id_obj;
    
    space_manager_t sm;       // space manager
    space_manager_t bsm;      // base space manager
    uint64_t        base_blk; // base block for bsm object
    
    avl_tree_t obj_info_list;              // all opened object info

    avl_tree_t metadata_cache;              // cache
    os_rwlock metadata_cache_lock;          // lock
    
    avl_node_t entry;
    
    uint32_t ref_cnt;
    os_rwlock ct_lock;             // lock
};

int32_t ofs_init_system(void);
void ofs_exit_system(void);

typedef int32_t (*container_cb_t)(void *, container_handle_t *);

int32_t ofs_walk_all_opened_container(container_cb_t cb, void *para);

// container API
int32_t ofs_open_container(const char *ct_name, container_handle_t **ct);
int32_t ofs_create_container(const char *ct_name, uint64_t total_sectors, container_handle_t **ct);
int32_t ofs_close_container(container_handle_t *ct);
container_handle_t *ofs_get_container_handle(const char *ct_name);


#ifdef __cplusplus
}
#endif

#endif

