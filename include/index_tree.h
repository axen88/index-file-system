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

/* 函数IndexWalk中v_flags参数允许的值 */
#define INDEX_GET_FIRST      0x01 /* 获取第一个key，如果为0，表明是获取下一个 */
#define INDEX_GET_LAST       0x02 /* 获取最后一个key */
#define INDEX_GET_PREV       0x04 /* 获取前一个key */
#define INDEX_GET_CURRENT    0x08 /* 获取当前key (仅在内部使用) */
#define INDEX_GET_LAST_ENTRY 0x10 /* 获取最后一个entry */
#define INDEX_REMOVE_BLOCK   0x20 /* 删除key所在的块 */
#define INDEX_ADD_BLOCK      0x40 /* 将key所在的块设置为已用，修复模式时使用 */
#define INDEX_WALK_MASK      0x1F


typedef int32_t (*DeleteFunc) (void *hnd,
    const void *key, uint16_t key_len,
    const void *c, uint16_t value_len);
typedef int32_t (*WalkAllCallBack) (void *obj, void *para);

extern int32_t index_search_key_nolock(OBJECT_HANDLE * obj, const void * key,
    uint16_t key_len);
extern int32_t index_remove_key_nolock(OBJECT_HANDLE * obj, const void * key,
    uint16_t key_len);
extern int32_t index_insert_key_nolock(OBJECT_HANDLE * obj, const void * key,
    uint16_t key_len, const void * c, uint16_t value_len);

extern int32_t index_search_key(OBJECT_HANDLE *obj, const void *key,
    uint16_t key_len);
extern int32_t index_remove_key(OBJECT_HANDLE *obj, const void *key,
    uint16_t key_len);
extern int32_t index_insert_key(OBJECT_HANDLE *obj, const void *key,
    uint16_t key_len, const void *c, uint16_t value_len);
extern int32_t index_update_value(OBJECT_HANDLE * tree, const void * key,
    uint16_t key_len, const void * c, uint16_t value_len);

extern int32_t index_walk_all(OBJECT_HANDLE *obj, bool_t v_bReverse,
    uint8_t flags, void *para, WalkAllCallBack v_pCallBack);

extern int32_t walk_tree(OBJECT_HANDLE *obj, uint8_t flags);
extern int64_t index_get_total_key(OBJECT_HANDLE *obj);
extern int64_t index_get_target_key(OBJECT_HANDLE *obj, uint64_t target);

/* 以下是内部接口，仅供测试，非指定情况请勿使用，线程不安全 */
extern int32_t tree_remove_ie(OBJECT_HANDLE *obj);

typedef struct tagWALK_ALL_TREES_PARA_S
{
    uint8_t flags;
    int32_t (*pCallBack)(void *obj, void *para);
} WALK_ALL_TREES_PARA_S;

extern int32_t index_walk_all_attrs(OBJECT_HANDLE *dir_tree,
    WALK_ALL_TREES_PARA_S *para);


extern void init_ib(INDEX_BLOCK * v_pstIB, uint8_t v_ucNodeType,
    uint32_t alloc_size);

extern int32_t collate_key(uint16_t collate_rule, INDEX_ENTRY * v_pstIE,
    const void * key, uint16_t key_len);
extern int32_t search_key_internal(OBJECT_HANDLE * obj, const void * key,
    uint16_t key_len);

#ifdef	__cplusplus
}
#endif


#endif

