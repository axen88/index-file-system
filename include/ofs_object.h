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
File Name: OFS_OBJECT.H
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
#ifndef __OFS_OBJECT_H__
#define __OFS_OBJECT_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define INVALID_OBJID                 ((uint64_t)(-1))
#define OBJID_IS_INVALID(id)          ((id) == INVALID_OBJID)

#define SET_INODE_CLEAN(obj_info)      SET_CACHE_CLEAN((obj_info)->inode_cache)
#define SET_INODE_DIRTY(obj_info)      SET_CACHE_DIRTY((obj_info)->inode_cache)
#define INODE_DIRTY(obj_info)          CACHE_DIRTY((obj_info)->inode_cache)


struct object_info
{
    struct container_handle *ct;       // ct handle
    
    inode_record_t *inode;                // inode

    ofs_block_cache_t *inode_cache;      //
    
    uint32_t obj_state;                // state

    uint64_t objid;                    // object id
    uint64_t inode_no;                 // inode no, also the position stored on disk

    char obj_name[OBJ_NAME_MAX_SIZE];
    
    avl_node_t entry;                  // register in ct handle

    attr_record_t *attr_record;           // attr record
    os_rwlock attr_lock;               // lock  tree handle

    dlist_head_t obj_hnd_list;        // all object handle
    os_rwlock    obj_hnd_lock;        // lock the obj_hnd_list operation

    ofs_block_cache_t root_cache;
    
    avl_tree_t caches;            // record all new block data
    os_rwlock caches_lock;
    
    uint32_t obj_ref_cnt;
    
    os_rwlock obj_lock;
};

struct object_handle
{
    struct container_handle *ct;       // ct handle
    object_info_t *obj_info;

    // tree handle structure
    uint8_t max_depth;
    uint8_t depth;             // Number of the parent nodes
    ofs_block_cache_t *cache_stack[TREE_MAX_DEPTH];
    uint64_t position_stack[TREE_MAX_DEPTH];
    ofs_block_cache_t *cache;
    uint64_t position;
    index_entry_t *ie;        

    dlist_entry_t entry;
};



object_info_t *ofs_get_object_info(container_handle_t *ct, uint64_t objid);
object_handle_t *ofs_get_object_handle(container_handle_t *ct, uint64_t objid);


/* for internal only */
int32_t create_object(container_handle_t *ct, uint64_t objid, uint16_t flags, object_handle_t **obj);
int32_t open_object(container_handle_t *ct, uint64_t objid, uint64_t inode_no, object_handle_t **obj);
extern void close_object(object_info_t *obj_info);
int32_t create_object_at_inode(container_handle_t *ct, uint64_t objid, uint64_t inode_no,
    uint16_t flags, object_handle_t **obj_out);

extern int32_t compare_attr_info2(const char *attr_name, object_info_t *attr_info_node);


// object API
int32_t ofs_open_object(container_handle_t *ct, uint64_t objid, object_handle_t **obj);
int32_t ofs_create_object(container_handle_t *ct, uint64_t objid, uint16_t flags, object_handle_t **obj);
int32_t ofs_close_object(object_handle_t *obj);
int32_t ofs_delete_object(container_handle_t *ct, uint64_t objid);
int32_t ofs_rename_object(object_handle_t *obj, const char *new_obj_name);
int32_t ofs_set_object_name(object_handle_t *obj, char *name);

#ifdef	__cplusplus
}
#endif

#endif

