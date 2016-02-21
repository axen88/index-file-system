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

int32_t compare_cache1(const INDEX_BLOCK_CACHE *cache, const INDEX_BLOCK_CACHE *cache_node)
{
    if (cache->vbn > cache_node->vbn)
    {
        return 1;
    }
    
    if (cache->vbn < cache_node->vbn)
    {
        return -1;
    }

    return 0;
}

int32_t compare_old_block1(const INDEX_OLD_BLOCK *old_block, const INDEX_OLD_BLOCK *old_block_node)
{
    if (old_block->vbn > old_block_node->vbn)
    {
        return 1;
    }
    
    if (old_block->vbn < old_block_node->vbn)
    {
        return -1;
    }

    return 0;
}

// update the attr root cache into inode
void validate_attr(ATTR_INFO *attr_info)
{
    if (!ATTR_INFO_DIRTY(attr_info))
    {
        return;
    }
    
    ASSERT(attr_info->attr_record.record_size
        == (INODE_GET_ATTR(&attr_info->obj->inode))->record_size);
    memcpy((INODE_GET_ATTR(&attr_info->obj->inode)),
        &attr_info->attr_record, attr_info->attr_record.record_size);
    
    INODE_SET_DIRTY(attr_info->obj);
    ATTR_INFO_CLR_DIRTY(attr_info);

    return;
}

void init_attr_info(struct _OBJECT_HANDLE *obj)
{
    ATTR_RECORD *attr_record;
    ATTR_INFO *attr_info;
    
    ASSERT(obj != NULL);

    attr_record = INODE_GET_ATTR(&obj->inode);
    attr_info = &obj->attr_info;

    memset(attr_info, 0, sizeof(ATTR_INFO));

    attr_info->obj = obj;
    memcpy(&attr_info->attr_record, attr_record, attr_record->record_size);
    memcpy(&attr_info->old_attr_record, attr_record, attr_record->record_size);
    attr_info->root_ibc.vbn = obj->inode_no;
    attr_info->root_ibc.ib = (INDEX_BLOCK *)attr_record->content;
    attr_info->attr_ref_cnt = 0;
    dlist_init_head(&attr_info->attr_hnd_list);
    avl_create(&attr_info->attr_old_blocks, (int (*)(const void *, const void*))compare_old_block1, sizeof(INDEX_OLD_BLOCK),
        OS_OFFSET(INDEX_OLD_BLOCK, attr_entry));
    avl_create(&attr_info->attr_caches, (int (*)(const void *, const void*))compare_cache1, sizeof(INDEX_BLOCK_CACHE),
        OS_OFFSET(INDEX_BLOCK_CACHE, attr_entry));
    OS_RWLOCK_INIT(&attr_info->attr_lock);
    OS_RWLOCK_INIT(&attr_info->caches_lock);
}

void destroy_attr_info(ATTR_INFO *attr_info)
{
    LOG_INFO("Now destroy attr info. objid(%lld)\n", attr_info->obj->objid);

    commit_attr_modification(attr_info);
    
    avl_destroy(&attr_info->attr_old_blocks);

    index_release_all_caches_in_attr(attr_info);
    avl_destroy(&attr_info->attr_caches);
    OS_RWLOCK_DESTROY(&attr_info->caches_lock);
}

int32_t index_open_attr(struct _OBJECT_HANDLE *obj, ATTR_HANDLE **attr)
{
    ATTR_HANDLE *tmp_attr;
    ATTR_INFO *attr_info;

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

    attr_info = &obj->attr_info;

    OS_RWLOCK_WRLOCK(&attr_info->attr_lock);

    attr_info->attr_ref_cnt++;
	tmp_attr->attr_info = attr_info;
	dlist_add_tail(&attr_info->attr_hnd_list, &tmp_attr->entry);

    *attr = tmp_attr;
    OS_RWLOCK_WRUNLOCK(&attr_info->attr_lock);

    return 0;
}

int32_t index_close_attr(ATTR_HANDLE *attr)
{
    ATTR_INFO *attr_info = NULL;
    
    ASSERT(attr != NULL);

    attr_info = attr->attr_info;

    OS_RWLOCK_WRLOCK(&attr_info->attr_lock);
    
    if (attr_info->attr_ref_cnt == 0)
    {
        OS_RWLOCK_WRUNLOCK(&attr_info->attr_lock);
        LOG_EMERG("Too many times put attr info.\n");
        return -1;
    }
    else
    {
        attr_info->attr_ref_cnt--;
    }
    
    dlist_remove_entry(&attr_info->attr_hnd_list, &attr->entry);
    OS_RWLOCK_WRUNLOCK(&attr_info->attr_lock);
    OS_FREE(attr);
    
    return 0;
}

void recover_attr_record(ATTR_INFO *attr_info)
{
    memcpy(&attr_info->attr_record, &attr_info->old_attr_record,
        sizeof(ATTR_RECORD));
}

void backup_attr_record(ATTR_INFO *attr_info)
{
    memcpy(&attr_info->old_attr_record, &attr_info->attr_record,
        sizeof(ATTR_RECORD));
}

void cancel_attr_modification(ATTR_INFO *attr_info)
{
    ASSERT(attr_info != NULL);

    if (!ATTR_INFO_DIRTY(attr_info))
    {
        return;
    }

    // free old block memory
    index_release_all_old_blocks_mem_in_attr(attr_info);
    
    // discard all dirty block cache
    index_cancel_all_caches_in_attr(attr_info);
    
    // free all memory
    index_release_all_free_caches_in_attr(attr_info);

    // recover the attr record
    recover_attr_record(attr_info);
    ATTR_INFO_CLR_DIRTY(attr_info);
        
    return;
}

int32_t commit_attr_modification(ATTR_INFO *attr_info)
{
    int32_t ret = 0;
    
    ASSERT(attr_info != NULL);

    // validate the attribute into inode
    validate_attr(attr_info);

    // write index block caches to disk
    ret = index_flush_all_caches_in_attr(attr_info);
    if (0 > ret)
    {
        LOG_ERROR("Flush index block cache failed. objid(%lld) ret(%d)\n",
            attr_info->obj->objid, ret);
        return ret;
    }
        
    // write inode to disk
    ret = flush_inode(attr_info->obj);
    if (0 > ret)
    {
        LOG_ERROR("Flush index inode failed. objid(%lld) ret(%d)\n",
            attr_info->obj->objid, ret);
        return ret;
    }

    // release old blocks
    index_release_all_old_blocks_in_attr(attr_info);
    
    // release free caches
    index_release_all_free_caches_in_attr(attr_info);

    // backup the attr record
    backup_attr_record(attr_info);

    return 0;
}



