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
File Name: INDEX_TREE.H
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

#ifndef __INDEX_TREE_H__
#define __INDEX_TREE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* flags in index_walk_all routine */
#define INDEX_GET_FIRST      0x01 /* 0: get the next key, 1: get the first */
#define INDEX_GET_LAST       0x02 /* get the last key */
#define INDEX_GET_PREV       0x04 /* get the prev key */
#define INDEX_GET_CURRENT    0x08 /* get the current key */
#define INDEX_GET_LAST_ENTRY 0x10 /* get the last entry */
#define INDEX_REMOVE_BLOCK   0x20 /* remove the block of the current key occupied */
#define INDEX_ADD_BLOCK      0x40 /* add the block of the current key occupied */
#define INDEX_WALK_MASK      0x1F

typedef int32_t (*tree_walk_cb_t) (void *obj, void *para);

extern int32_t index_search_key_nolock(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len);
extern int32_t index_remove_key_nolock(object_handle_t * obj, const void * key,
    uint16_t key_len);
extern int32_t index_insert_key_nolock(object_handle_t * obj, const void * key,
    uint16_t key_len, const void *value, uint16_t value_len);

extern int32_t index_search_key(object_handle_t *obj, const void *key,
    uint16_t key_len);
extern int32_t index_remove_key(object_handle_t *obj, const void *key,
    uint16_t key_len);
extern int32_t index_insert_key(object_handle_t *obj, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len);
extern int32_t index_update_value(object_handle_t * tree, const void * key,
    uint16_t key_len, const void *value, uint16_t value_len);

extern int32_t index_walk_all(object_handle_t *obj, bool_t v_bReverse,
    uint8_t flags, void *para, tree_walk_cb_t cb);

extern int32_t walk_tree(object_handle_t *obj, uint8_t flags);
extern int64_t index_get_total_key(object_handle_t *obj);
extern int64_t index_get_target_key(object_handle_t *obj, uint64_t target);


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

#ifdef	__cplusplus
}
#endif


#endif

