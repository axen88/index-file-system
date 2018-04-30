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
File Name: OFS_TREE.H
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

#ifndef __OFS_TREE_H__
#define __OFS_TREE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define TREE_MAX_DEPTH     16

/* flags in index_walk_all routine */
#define INDEX_GET_FIRST      0x01 /* 0: get the next key, 1: get the first */
#define INDEX_GET_LAST       0x02 /* get the last key */
#define INDEX_GET_PREV       0x04 /* get the prev key */
#define INDEX_GET_CURRENT    0x08 /* get the current key */
#define INDEX_GET_LAST_ENTRY 0x10 /* get the last entry */
#define INDEX_REMOVE_BLOCK   0x20 /* remove the block of the current key occupied */
#define INDEX_ADD_BLOCK      0x40 /* add the block of the current key occupied */
#define INDEX_WALK_MASK      0x1F

#define KEY_MAX_SIZE    256
#define VALUE_MAX_SIZE  1024

#define ENTRY_END_SIZE             sizeof(index_entry_t)
#define ENTRY_BEGIN_SIZE           sizeof(index_entry_t)



// Index Entry ucFlags, bits field
#define INDEX_ENTRY_LEAF          0x00
#define INDEX_ENTRY_NODE          0x01
#define INDEX_ENTRY_END           0x02
#define INDEX_ENTRY_BEGIN         0x04

// Index Block ucFlags, bits field
#define INDEX_BLOCK_SMALL         0x00  // The block has no child block
#define INDEX_BLOCK_LARGE         0x01  // The block has child block

#define VBN_SIZE                  sizeof(u64_t)

#define GET_FIRST_IE(ib)     ((index_entry_t *)((uint8_t*)(ib) + ((index_block_t *)(ib))->first_entry_off))
#define GET_END_IE(ib)       ((uint8_t *)(ib) + ((index_block_t *)(ib))->head.real_size)
#define GET_NEXT_IE(ie)      ((index_entry_t *)((uint8_t*)(ie) + ((index_entry_t *)(ie))->len))
#define GET_PREV_IE(ie)      ((index_entry_t *)((uint8_t*)(ie) - ((index_entry_t *)(ie))->prev_len))
#define GET_IE_VBN(ie)       (*(u64_t*)((uint8_t *)(ie)+ (((index_entry_t *)(ie))->len - VBN_SIZE)))
#define SET_IE_VBN(ie, vbn)  (GET_IE_VBN(ie) = vbn)
#define GET_IE_KEY(ie)       ((uint8_t*)(ie) + sizeof(index_entry_t))
#define GET_IE_VALUE(ie)     ((uint8_t*)(ie) + sizeof(index_entry_t) + (((index_entry_t *)(ie))->key_len))

#define IB(b)   ((index_block_t *)(b))

typedef int32_t (*tree_walk_cb_t) (void *obj, void *para);

extern int32_t index_search_key_nolock(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len);
extern int32_t index_insert_key_nolock(object_handle_t * obj, const void * key,
    uint16_t key_len, const void *value, uint16_t value_len);
extern int32_t index_remove_key_nolock(object_handle_t * obj, const void * key, uint16_t key_len);


extern int32_t walk_tree(object_handle_t *obj, uint8_t flags);
extern int64_t index_get_total_key(object_handle_t *obj);
extern int64_t index_get_target_key(object_handle_t *obj, u64_t target);


typedef struct tree_walk_para
{
    uint8_t flags;
    int32_t (*pCallBack)(void *obj, void *para);
} tree_walk_para_t;

extern int32_t index_walk_all_attrs(object_handle_t *dir_tree,
    tree_walk_para_t *para);


extern void init_ib(index_block_t * v_pstIB, uint8_t v_ucNodeType,
    uint32_t alloc_size);

extern int32_t search_key_internal(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len);
int32_t tree_remove_ie(object_handle_t *tree);


// table/KV/index API
int32_t index_search_key(object_handle_t *obj, const void *key, uint16_t key_len);
int32_t index_remove_key(object_handle_t *obj, const void *key, uint16_t key_len);
int32_t index_insert_key(object_handle_t *obj, const void *key, uint16_t key_len, const void *value, uint16_t value_len);
int32_t index_update_value(object_handle_t * tree, const void * key, uint16_t key_len, const void *value, uint16_t value_len);
int32_t index_walk_all(object_handle_t *obj, bool_t reverse, uint8_t flags, void *para, tree_walk_cb_t cb);

#ifdef	__cplusplus
}
#endif


#endif

