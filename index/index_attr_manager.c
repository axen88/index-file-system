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
文 件 名: INDEX_ATTR_MANAGER.C
版    本: 1.00
日    期: 2012年6月23日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年6月23日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

int32_t compare_old_block1(const INDEX_OLD_BLOCK *old_block,
    const INDEX_OLD_BLOCK *old_block_node);
int32_t compare_cache1(const INDEX_BLOCK_CACHE *cache,
    const INDEX_BLOCK_CACHE *cache_node);

int32_t validate_attr(ATTR_INFO *attr_info)
{
    int32_t ret = 0;
    
    if (!ATTR_INFO_DIRTY(attr_info))
    {
        return 0;
    }
    
    ASSERT(attr_info->attr_record.record_size
        == (INODE_GET_ATTR(&attr_info->obj->inode))->record_size);
    memcpy((INODE_GET_ATTR(&attr_info->obj->inode)),
        &attr_info->attr_record, attr_info->attr_record.record_size);
    INODE_SET_DIRTY(attr_info->obj);

    ATTR_INFO_CLR_DIRTY(attr_info);

    return 0;
}

void init_attr_info(struct _OBJECT_HANDLE *obj, ATTR_RECORD *attr_record, ATTR_INFO *attr_info)
{
    memset(attr_info, 0, sizeof(ATTR_INFO));

    attr_info->obj = obj;
    memcpy(&attr_info->attr_record, attr_record, attr_record->record_size);
    memcpy(&attr_info->old_attr_record, attr_record, attr_record->record_size);
    attr_info->root_ibc.vbn = obj->inode.objid;
    attr_info->root_ibc.ib = (INDEX_BLOCK *)attr_record->content;
    attr_info->attr_ref_cnt = 1;
    avl_create(&attr_info->attr_old_blocks, (int (*)(const void *, const void*))compare_old_block1, sizeof(INDEX_OLD_BLOCK),
        OS_OFFSET(INDEX_OLD_BLOCK, attr_entry));
    avl_create(&attr_info->attr_caches, (int (*)(const void *, const void*))compare_cache1, sizeof(INDEX_BLOCK_CACHE),
        OS_OFFSET(INDEX_BLOCK_CACHE, attr_entry));
    OS_RWLOCK_INIT(&attr_info->attr_lock);
}

void get_attr_info(ATTR_INFO *attr_info)
{
    attr_info->attr_ref_cnt++;

    return;
}


int32_t put_attr_info(ATTR_INFO *attr_info)
{
    if (attr_info->attr_ref_cnt == 0)
    {
        LOG_EMERG("Too many times put attr info.\n");
        return -INDEX_ERR_MANY_TIMES_PUT;
    }

    attr_info->attr_ref_cnt--;
    if (attr_info->attr_ref_cnt != 0)
    {
        return 0;
    }
    
    LOG_INFO("Now put attr info. obj_name(%s)\n", attr_info->obj->obj_name);

    index_commit_attr_modification(attr_info);
    
    avl_destroy(&attr_info->attr_old_blocks);

    index_release_all_caches_in_attr(attr_info->obj, attr_info);
    avl_destroy(&attr_info->attr_caches);

    return 0;
}

int32_t index_open_attr(struct _OBJECT_HANDLE *obj, ATTR_HANDLE **attr)
{
    ATTR_HANDLE *tmp_attr = NULL;
    int32_t ret = 0;

    ASSERT(obj != NULL);
    ASSERT(attr != NULL);

    tmp_attr = (ATTR_HANDLE *)OS_MALLOC(sizeof(ATTR_HANDLE));
    if (tmp_attr == NULL)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ATTR_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_attr, 0, sizeof(ATTR_HANDLE));

    OS_RWLOCK_WRLOCK(&obj->obj_lock);

    get_attr_info(&obj->attr_info);
	tmp_attr->attr_info = &obj->attr_info;
	dlist_add_tail(&obj->attr_hnd_list, &tmp_attr->entry);

    *attr = tmp_attr;
    OS_RWLOCK_WRUNLOCK(&obj->obj_lock);

    return 0;
}

int32_t index_close_attr(ATTR_HANDLE *attr)
{
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(attr != NULL);

    obj = attr->attr_info->obj;

    OS_RWLOCK_WRLOCK(&obj->obj_lock);
    
    dlist_remove_entry(&obj->attr_hnd_list, &attr->entry);
    put_attr_info(attr->attr_info);
    OS_FREE(attr);

    OS_RWLOCK_WRUNLOCK(&obj->obj_lock);
    
    return 0;
}

void index_cancel_attr_modification(ATTR_INFO *attr_info)
{
    ASSERT(attr_info != NULL);

    if (!ATTR_INFO_DIRTY(attr_info))
    {
        return;
    }

    /* 释放旧块占用的内存 */
    index_release_all_old_blocks_mem_in_attr(attr_info->obj, attr_info);
    
    /* 取消所有的脏cache数据 */
    index_cancel_all_caches_in_attr(attr_info->obj, attr_info);
    
    /* 释放所有的空cache */
    index_release_all_free_caches_in_attr(attr_info->obj, attr_info);

    /* 恢复属性记录 */
    memcpy(&attr_info->attr_record, &attr_info->old_attr_record,
        sizeof(ATTR_RECORD));
    ATTR_INFO_CLR_DIRTY(attr_info);
        

    /* 恢复inode记录 */
    memcpy(&attr_info->obj->old_inode, &attr_info->obj->inode, sizeof(INODE_RECORD));
    INODE_CLR_DIRTY(attr_info->obj);

    return;
}

int32_t index_commit_attr_modification(ATTR_INFO *attr_info)
{
    int32_t ret = 0;
    
    ASSERT(attr_info != NULL);

    ret = validate_attr(attr_info);

    /* 释放旧块占用的内存 */
    ret = index_flush_all_caches_in_attr(attr_info);
        

    /* 恢复inode记录 */
    ret = flush_inode(attr_info->obj);

    /* 释放旧块 */
    index_release_all_old_blocks_in_attr(attr_info->obj, attr_info);
    
    /* 释放所有的空cache */
    index_release_all_free_caches_in_attr(attr_info->obj, attr_info);

    /* 备份属性记录 */
    memcpy(&attr_info->old_attr_record, &attr_info->attr_record,
        sizeof(ATTR_RECORD));
    

    return 0;
}



