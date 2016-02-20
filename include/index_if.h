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

            版权所有(C), 2012~2015, AXEN工作室
********************************************************************************
文 件 名: INDEX_IF.H
版    本: 1.00
日    期: 2012年8月15日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年8月15日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#ifndef __INDEX_IF_H__
#define __INDEX_IF_H__

#include "avl.h"
#include "os_adapter.h"
#include "os_collate.h"
#include "os_list_double.h"
#include "os_queue.h"
#include "os_disk_if.h"
#include "os_utils.h"
#include "os_threads_group.h"
#include "os_cmd_ui.h"

#include "index_layout.h"
#include "index_bitmap.h"
#include "index_block_if.h"

typedef enum tagCACHE_STATUS_E
{
    EMPTY = 0,  /* 无数据 */
    CLEAN,      /* 数据是干净的 */
    DIRTY       /* 数据是脏的 */
} CACHE_STATUS_E;

#define IBC_SET_DIRTY(ibc)  ((ibc)->state = DIRTY)
#define IBC_SET_CLEAN(ibc)  ((ibc)->state == CLEAN)
#define IBC_SET_EMPTY(ibc)  ((ibc)->state == EMPTY)
#define IBC_DIRTY(ibc)  ((ibc)->state == DIRTY)
#define IBC_CLEAN(ibc)  ((ibc)->state == CLEAN)
#define IBC_EMPTY(ibc)  ((ibc)->state == EMPTY)

#define INDEX_NAME_SIZE            256


#define INDEX_MAX_DEPTH           16

#define ATTR_INFO_DIRTY(attr_info)   IBC_DIRTY(&(attr_info)->root_ibc)
#define ATTR_INFO_SET_DIRTY(attr_info) IBC_SET_DIRTY(&(attr_info)->root_ibc)
#define ATTR_INFO_CLR_DIRTY(attr_info) IBC_SET_CLEAN(&(attr_info)->root_ibc)

#define OBJ_FLAG_DIRTY   0x01

#define INODE_SET_DIRTY(obj)      ((obj)->obj_state |= OBJ_FLAG_DIRTY)
#define INODE_CLR_DIRTY(obj)      ((obj)->obj_state &= ~OBJ_FLAG_DIRTY)
#define INODE_DIRTY(obj)         ((obj)->obj_state & OBJ_FLAG_DIRTY)

typedef struct _INDEX_BLOCK_CACHE
{
	uint64_t vbn;
	uint32_t state;
	struct _INDEX_BLOCK *ib;
	avl_node_t obj_entry;
	avl_node_t attr_entry;
} INDEX_BLOCK_CACHE;

typedef struct _INDEX_OLD_BLOCK
{
	uint64_t vbn;
	avl_node_t obj_entry;
	avl_node_t attr_entry;
} INDEX_OLD_BLOCK;

typedef struct _ATTR_INFO
{
    struct _OBJECT_HANDLE *obj;           /* 属性所在对象的操作句柄 */

    char attr_name[ATTR_NAME_SIZE];  /* 属性名称，只对扩展属性有效 */
    ATTR_RECORD attr_record;      /* 属性记录 */
    ATTR_RECORD old_attr_record;      /* 属性记录 */

    INDEX_BLOCK_CACHE root_ibc;
    
    avl_tree_t attr_caches;
    avl_tree_t attr_old_blocks;
    
    avl_node_t entry;               /* recorded in object */
   
    uint32_t attr_ref_cnt;             /* 引用计数 */
    
    OS_RWLOCK attr_lock;                    /* 锁 */    
} ATTR_INFO;

typedef struct _ATTR_HANDLE
{
    ATTR_INFO *attr_info;             /* 属性信息 */

    //OS_U64 offset;
    DLIST_ENTRY_S entry;               /* recorded in object */

    /* 树操作相关信息 */
    uint8_t max_depth;
    uint8_t depth;             // Number of the parent nodes
    INDEX_BLOCK_CACHE *cache_stack[INDEX_MAX_DEPTH];

    uint64_t position_stack[INDEX_MAX_DEPTH];

    INDEX_BLOCK_CACHE *cache;
    uint64_t position;
    INDEX_ENTRY *ie;        
} ATTR_HANDLE;


typedef struct _OBJECT_HANDLE
{
    struct _INDEX_HANDLE *index;       /* 索引区操作句柄 */
    
    INODE_RECORD inode;                /* 此树对应的inode */
    INODE_RECORD old_inode;                /* 此树对应的inode */
    
    uint32_t obj_state;                     /* 描述状态变化 */

    uint64_t objid;      // object id
    uint64_t inode_no;           // inode no, also the position stored on disk

    char obj_name[OBJ_NAME_SIZE];
    
    avl_node_t entry;               /* register in index handle */

    ATTR_HANDLE *attr;
    ATTR_INFO *attr_info;
    DLIST_HEAD_S attr_hnd_list;       /* 本对象上已打开的属性句柄列表*/
    
    avl_tree_t obj_caches;
    avl_tree_t obj_old_blocks;   /* 旧块列表 */
    OS_RWLOCK caches_lock;

    uint32_t obj_ref_cnt;
    
    OS_RWLOCK obj_lock;
} OBJECT_HANDLE;

typedef struct _INDEX_HANDLE
{
    BLOCK_HANDLE_S *hnd;               // The block file handle
    char name[INDEX_NAME_SIZE];      // 文件名

    OBJECT_HANDLE *idlst_obj;
    OBJECT_HANDLE *log_obj;
    OBJECT_HANDLE *space_obj;

    
    avl_tree_t obj_list;      /* all opened object */
    OS_RWLOCK obj_list_lock;  // lock list

    avl_node_t entry;
    
    uint32_t index_ref_cnt;

    OS_RWLOCK index_lock; /* 锁索引区操作 */
} INDEX_HANDLE;

extern int32_t index_block_read(struct _ATTR_HANDLE * tree, uint64_t vbn);

#define INDEX_UPDATE_INODE(obj) \
    index_update_block_pingpong(obj->index->hnd, &obj->inode.head, obj->inode_no)
    
#define INDEX_READ_INODE(index, obj, inode_no) \
    index_read_block_pingpong(index->hnd, &obj->inode.head, inode_no, INODE_MAGIC, INODE_SIZE);

extern int32_t index_alloc_cache_and_block(struct _ATTR_INFO *attr_info, INDEX_BLOCK_CACHE **cache);
int32_t index_release_all_free_caches_in_attr(struct _OBJECT_HANDLE *obj, struct _ATTR_INFO *attr_info);
int32_t index_cancel_all_caches_in_attr(struct _OBJECT_HANDLE *obj, struct _ATTR_INFO *attr_info);
void index_release_all_old_blocks_mem_in_attr(struct _OBJECT_HANDLE *obj, struct _ATTR_INFO *attr_info);
void index_release_all_old_blocks_mem_in_obj(struct _OBJECT_HANDLE * obj);
void index_release_all_old_blocks_in_attr(struct _OBJECT_HANDLE *obj, struct _ATTR_INFO *attr_info);
void index_release_all_old_blocks_in_obj(struct _OBJECT_HANDLE * obj);
extern int32_t index_record_old_block(struct _ATTR_INFO *attr_info, uint64_t vbn);
extern int32_t index_release_all_caches_in_attr(struct _OBJECT_HANDLE *obj,
    struct _ATTR_INFO *attr_info);
extern int32_t index_release_all_caches_in_obj(struct _OBJECT_HANDLE * obj);
void index_release_all_free_caches_in_obj(struct _OBJECT_HANDLE * obj);
int32_t index_release_all_caches_in_obj(struct _OBJECT_HANDLE * obj);
int32_t index_flush_all_caches_in_obj(struct _OBJECT_HANDLE * obj);
int32_t index_cancel_all_caches_in_obj(struct _OBJECT_HANDLE * obj);



#define INDEX_ALLOC_BLOCK(obj, vbn) block_alloc((obj)->index->hnd, 1, vbn)
#define INDEX_FREE_BLOCK(obj, vbn)  block_free((obj)->index->hnd, vbn, 1)
extern int32_t IndexIBCacheFlush(struct _ATTR_HANDLE * obj);
extern void IndexIBCacheFree(struct _ATTR_HANDLE * obj);
extern void IndexIBCacheInvalidate(struct _ATTR_HANDLE * obj);
extern void index_release_all_old_blocks_in_obj(struct _OBJECT_HANDLE * obj);
int32_t index_flush_all_caches_in_attr(struct _ATTR_INFO * attr_info);


extern int32_t walk_all_opened_index(
    int32_t (*func)(void *, struct _INDEX_HANDLE *), void *para);


#include "index_attr.h"
#include "index_tree.h"
#include "index_object.h"
#include "index_manager.h"

#include "index_tools_if.h"

#ifdef __KERNEL__
#include "os_mml.h"
#undef OS_PRINT
#define OS_PRINT(...)        MML_Print(__VA_ARGS__)
#endif


#endif


