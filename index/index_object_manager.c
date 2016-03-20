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
File Name: INDEX_OBJECT_MANAGER.C
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
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

int32_t compare_object2(const void *objid, OBJECT_INFO *obj_info)
{
    if ((*(uint64_t *)objid) > obj_info->objid)
    {
        return 1;
    }
    else if ((*(uint64_t *)objid) == obj_info->objid)
    {
        return 0;
    }

    return -1;
}

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

// copy origin attr record from inode
void recover_attr_record(OBJECT_INFO *obj_info)
{
    ATTR_RECORD *attr_record;
    
    attr_record = INODE_GET_ATTR_RECORD(&obj_info->inode);
    memcpy(&obj_info->attr_record, attr_record, attr_record->record_size);
}

void init_attr(OBJECT_INFO *obj_info, uint64_t inode_no)
{
    recover_attr_record(obj_info);
    obj_info->root_ibc.vbn = inode_no;
    obj_info->root_ibc.ib = (INDEX_BLOCK *)obj_info->attr_record.content;
    obj_info->root_ibc.state = CLEAN;
}

int32_t get_object_info(INDEX_HANDLE *index, uint64_t objid, OBJECT_INFO **obj_info_out)
{
    OBJECT_INFO *obj_info = NULL;
    
    obj_info = (OBJECT_INFO *)OS_MALLOC(sizeof(OBJECT_INFO));
    if (NULL == obj_info)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_INFO));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(obj_info, 0, sizeof(OBJECT_INFO));
    obj_info->obj_ref_cnt = 0;
    obj_info->index = index;
    obj_info->objid = objid;
    
    dlist_init_head(&obj_info->obj_hnd_list);
    OS_RWLOCK_INIT(&obj_info->obj_hnd_lock);
    
    OS_RWLOCK_INIT(&obj_info->attr_lock);
    
    avl_create(&obj_info->old_blocks, (int (*)(const void *, const void*))compare_old_block1, sizeof(INDEX_OLD_BLOCK),
        OS_OFFSET(INDEX_OLD_BLOCK, entry));
    avl_create(&obj_info->caches, (int (*)(const void *, const void*))compare_cache1, sizeof(INDEX_BLOCK_CACHE),
        OS_OFFSET(INDEX_BLOCK_CACHE, entry));
    OS_RWLOCK_INIT(&obj_info->caches_lock);
    
    OS_RWLOCK_INIT(&obj_info->obj_lock);

    avl_add(&index->obj_list, obj_info);

    *obj_info_out = obj_info;
    
    LOG_INFO("init object info finished. objid(%lld)\n", obj_info->objid);

    return 0;
}

void put_object_info(OBJECT_INFO *obj_info)
{
    LOG_INFO("destroy object info start. objid(%lld)\n", obj_info->objid);

    avl_remove(&obj_info->index->obj_list, obj_info);

    index_release_all_old_blocks_mem(obj_info);
    avl_destroy(&obj_info->old_blocks);

    index_release_all_caches(obj_info);
    avl_destroy(&obj_info->caches);
    
    OS_RWLOCK_DESTROY(&obj_info->caches_lock);
    OS_RWLOCK_DESTROY(&obj_info->attr_lock);
    OS_RWLOCK_DESTROY(&obj_info->obj_hnd_lock);
    OS_RWLOCK_DESTROY(&obj_info->obj_lock);

    OS_FREE(obj_info);
}

int32_t get_object_handle(OBJECT_INFO *obj_info, OBJECT_HANDLE **obj_out)
{
    int32_t ret = 0;
    OBJECT_HANDLE *obj = NULL;

    obj = (OBJECT_HANDLE *)OS_MALLOC(sizeof(OBJECT_HANDLE));
    if (NULL == obj)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(obj, 0, sizeof(OBJECT_HANDLE));
    obj->obj_info = obj_info;
    obj->index = obj_info->index;

    dlist_add_tail(&obj_info->obj_hnd_list, &obj->entry);
    obj_info->obj_ref_cnt++;
    
    *obj_out = obj;

    return 0;
}

void put_object_handle(OBJECT_HANDLE *obj)
{
    obj->obj_info->obj_ref_cnt--;
    dlist_remove_entry(&obj->obj_info->obj_hnd_list, &obj->entry);
    OS_FREE(obj);

    return;
}

void recover_obj_inode(OBJECT_INFO *obj_info)
{
    memcpy(&obj_info->inode, &obj_info->old_inode, sizeof(INODE_RECORD));
}

void backup_obj_inode(OBJECT_INFO *obj_info)
{
    memcpy(&obj_info->old_inode, &obj_info->inode, sizeof(INODE_RECORD));
}

int32_t flush_inode(OBJECT_INFO *obj_info)
{
    int32_t ret = 0;

    ASSERT(NULL != obj_info);

    if (!INODE_DIRTY(obj_info))
    {
        return 0;
    }

    ret = INDEX_UPDATE_INODE(obj_info);
    if (0 > ret)
    {
        LOG_ERROR("Update inode failed. objid(%lld) inode(%p) inode_no(%lld) ret(%d)\n",
            obj_info->objid, obj_info->inode, obj_info->inode_no, ret);
        return ret;
    }

    LOG_DEBUG("Update inode success. objid(%lld) inode(%p) inode_no(%lld)\n",
        obj_info->objid, obj_info->inode, obj_info->inode_no);

    // set recover dot
    backup_obj_inode(obj_info);
    INODE_CLR_DIRTY(obj_info);

    return 0;
}

// copy attr record into inode
void validate_attr(OBJECT_INFO *obj_info)
{
    if (!ATTR_INFO_DIRTY(obj_info))
    {
        return;
    }
    
    ASSERT(obj_info->attr_record.record_size
        == (INODE_GET_ATTR_RECORD(&obj_info->inode))->record_size);
    memcpy((INODE_GET_ATTR_RECORD(&obj_info->inode)),
        &obj_info->attr_record, obj_info->attr_record.record_size);
    
    INODE_SET_DIRTY(obj_info);
    ATTR_INFO_CLR_DIRTY(obj_info);

    return;
}

void cancel_object_modification(OBJECT_INFO *obj_info)
{
    ASSERT(obj_info != NULL);

    // recover obj inode
    recover_obj_inode(obj_info);
    INODE_CLR_DIRTY(obj_info);

    if (!ATTR_INFO_DIRTY(obj_info))
    {
        return;
    }

    // free old block memory
    index_release_all_old_blocks_mem(obj_info);
    
    // discard all dirty block cache
    index_release_all_dirty_blocks(obj_info);

    // recover the attr record
    recover_attr_record(obj_info);
    ATTR_INFO_CLR_DIRTY(obj_info);
    
    return;
}

int32_t commit_object_modification(OBJECT_INFO *obj_info)
{
    int32_t ret = 0;
    
    ASSERT(obj_info != NULL);

    // write dirty block caches to disk
    ret = index_flush_all_dirty_caches(obj_info);
    if (0 > ret)
    {
        LOG_ERROR("Flush index block cache failed. objid(%lld) ret(%d)\n",
            obj_info->objid, ret);
        return ret;
    }

    // update the attribute root into inode
    validate_attr(obj_info);

    ret = flush_inode(obj_info);
    if (0 > ret)
    {
        LOG_ERROR("Flush object inode failed. objid(%lld) ret(%d)\n",
            obj_info->objid, ret);
        return ret;
    }

    // release old blocks
    index_release_all_old_blocks(obj_info);

	return 0;
}

void put_all_object_handle(OBJECT_INFO *obj_info)
{
    OBJECT_HANDLE *obj;
    
    while (obj_info->obj_ref_cnt != 0)
    {
        obj = OS_CONTAINER(obj_info->obj_hnd_list.head.next, OBJECT_HANDLE, entry);
        put_object_handle(obj);
    }
}

int32_t close_object(OBJECT_INFO *obj_info)
{
    int32_t ret;
    
    put_all_object_handle(obj_info);
    
    ret = commit_object_modification(obj_info);
    if (ret < 0)
    {
        cancel_object_modification(obj_info);
        LOG_ERROR("commit object modification failed. objid(%lld) ret(%d)\n", obj_info->objid, ret);
    }
    
    put_object_info(obj_info);

    return 0;
}

int32_t create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj_out)
{
     int32_t ret = sizeof(INODE_RECORD);
     OBJECT_HANDLE *obj = NULL;
     uint64_t inode_no = 0;
     ATTR_RECORD *attr_record = NULL;
     OBJECT_INFO *obj_info;

    ASSERT(NULL != index);
    ASSERT(NULL != obj_out);
    ASSERT(INODE_SIZE == sizeof(INODE_RECORD));

    /* allocate inode block */
    ret = block_alloc(index->hnd, 1, &inode_no);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    ret = get_object_info(index, objid, &obj_info);
    if (ret < 0)
    {
        block_free(index->hnd, inode_no, 1);
        LOG_ERROR("get_object_info failed. ret(%d)\n", ret);
        return ret;
    }

    /* init inode */
    obj_info->inode.head.blk_id = INODE_MAGIC;
    obj_info->inode.head.alloc_size = INODE_SIZE;
    obj_info->inode.head.real_size = INODE_SIZE;
    
    obj_info->inode.first_attr_off = OS_OFFSET(INODE_RECORD, reserved);
    
    obj_info->inode.objid = objid;
    obj_info->inode.base_objid = 0;
    
    obj_info->inode.mode = 0;
    obj_info->inode.uid = 0;
    obj_info->inode.gid = 0;
    obj_info->inode.size = 0;
    obj_info->inode.links = 0;
    obj_info->inode.ctime = 0;
    obj_info->inode.atime = 0;
    obj_info->inode.mtime = 0;
    
    obj_info->inode.snapshot_no = index->hnd->sb.snapshot_no;
    obj_info->inode.name_size = DEFAULT_OBJ_NAME_SIZE;
    strncpy(obj_info->inode.name, DEFAULT_OBJ_NAME, OBJ_NAME_SIZE);
    
    strncpy(obj_info->obj_name, DEFAULT_OBJ_NAME, OBJ_NAME_SIZE);
    obj_info->inode_no = inode_no;

    /* init attr */
    attr_record = INODE_GET_ATTR_RECORD(&obj_info->inode);
    attr_record->record_size = ATTR_RECORD_SIZE;
    attr_record->flags = flags;
    if (flags & FLAG_TABLE)
    { /* table */
        init_ib((INDEX_BLOCK *)&attr_record->content, INDEX_BLOCK_SMALL, ATTR_RECORD_CONTENT_SIZE);
    }
    else
    { /* data stream */
        memset(attr_record->content, 0, ATTR_RECORD_CONTENT_SIZE);
    }

    init_attr(obj_info, inode_no);
    IBC_SET_DIRTY(&obj_info->root_ibc);

    // validate the attribute into inode
    validate_attr(obj_info);

    // update index block
    ret = index_update_block_pingpong_init(index->hnd, &obj_info->inode.head, inode_no);
    if (0 > ret)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_info(obj_info);
        LOG_ERROR("Create inode failed. obj_id(%lld) vbn(%lld) ret(%d)\n",
            objid, inode_no, ret);
        return ret;
    }
    
    LOG_DEBUG("Create inode success. obj_id(%lld) vbn(%lld)\n",
        objid, inode_no);

    backup_obj_inode(obj_info);
    
    ret = get_object_handle(obj_info, &obj);
    if (ret < 0)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_info(obj_info);
        LOG_ERROR("Open attr failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    *obj_out = obj;

    return 0;
}

int32_t open_object(INDEX_HANDLE *index, uint64_t objid, uint64_t inode_no, OBJECT_HANDLE **obj_out)
{
    int32_t ret = 0;
    OBJECT_INFO *obj_info = NULL;
    OBJECT_HANDLE *obj = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != obj_out);

    ret = get_object_info(index, objid, &obj_info);
    if (ret < 0)
    {
        LOG_ERROR("Get object info resource failed. objid(%lld)\n", objid);
        return ret;
    }

    ret = INDEX_READ_INODE(index, obj_info, inode_no);
    if (0 > ret)
    {
        LOG_ERROR("Read inode failed. ret(%d)\n", ret);
        put_object_info(obj_info);
        return ret;
    }

    obj_info->inode_no = inode_no;
    strncpy(obj_info->obj_name, obj_info->inode.name, obj_info->inode.name_size);

    backup_obj_inode(obj_info);
    init_attr(obj_info, inode_no);
    
    ret = get_object_handle(obj_info, &obj);
    if (ret < 0)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_info(obj_info);
        LOG_ERROR("Open attr failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    *obj_out = obj;

    return 0;
}

uint64_t get_objid(INDEX_HANDLE *index)
{
    return 1;
}


int32_t index_create_object_nolock(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj_out)
{
    int32_t ret = 0;
    OBJECT_HANDLE *obj = NULL;
    uint16_t name_size = 0;
    avl_index_t where = 0;
    OBJECT_INFO *obj_info;

    ASSERT(NULL != index);
    ASSERT(!OBJID_IS_INVALID(objid));
    ASSERT(NULL != obj_out);

    LOG_INFO("Create the obj start. objid(%lld)\n", objid);

    obj_info = avl_find(&index->obj_list, (avl_find_fn)compare_object2, &objid, &where);
    if (NULL != obj_info)
    {
        LOG_ERROR("The obj already exist. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return -INDEX_ERR_OBJ_EXIST;
    }

    ret = search_key_internal(index->id_obj, &objid, sizeof(uint64_t));
    if (0 <= ret)
    {
        LOG_ERROR("The obj already exist. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return -INDEX_ERR_OBJ_EXIST;
    }

    if (-INDEX_ERR_KEY_NOT_FOUND != ret)
    {
        LOG_ERROR("Search key failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = create_object(index, objid, flags, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = index_insert_key_nolock(index->id_obj, &objid, os_u64_size(objid),
        &obj->obj_info->inode_no, os_u64_size(obj->obj_info->inode_no));
    if (ret < 0)
    {
        LOG_ERROR("Insert obj failed. obj(%p) objid(%lld) ret(%d)\n",
            obj, objid, ret);
        (void)INDEX_FREE_BLOCK(obj, obj->obj_info->inode_no);
        close_object(obj->obj_info);
        return ret;
    }
    
    LOG_INFO("Create the obj success. objid(%lld) obj(%p) index_name(%s)\n",
        objid, obj, index->name);

    *obj_out = obj;
    
    return 0;
}    

int32_t index_create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;

    if ((NULL == index) || (OBJID_IS_INVALID(objid || (NULL == obj))))
    {
        LOG_ERROR("Invalid parameter. index(%p) objid(%lld) obj(%p)\n", index, objid, obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    OS_RWLOCK_WRLOCK(&index->index_lock);
    ret = index_create_object_nolock(index, objid, flags, obj);
    OS_RWLOCK_WRUNLOCK(&index->index_lock);
    
    return ret;
}    

int32_t index_open_object_nolock(struct _INDEX_HANDLE *index, uint64_t objid, uint32_t open_flags, OBJECT_HANDLE **obj_out)
{
    int32_t ret = 0;
    OBJECT_HANDLE *obj = NULL;
    uint64_t inode_no = 0;
    avl_index_t where = 0;
    OBJECT_HANDLE *id_obj;
    OBJECT_INFO *obj_info = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != obj_out);

    LOG_INFO("Open the obj. objid(%lld)\n", objid);

    obj_info = avl_find(&index->obj_list, (avl_find_fn)compare_object2, &objid, &where);
    if (NULL != obj_info)
    {
        ret = get_object_handle(obj_info, &obj);
        if (ret < 0)
        {
            LOG_ERROR("get_object_handle failed. objid(%lld) ret(%d)\n", objid, ret);
            return ret;
        }

        *obj_out = obj;
        LOG_WARN("The obj obj_ref_cnt inc. obj(%p) obj_ref_cnt(%d) objid(%lld)\n",
            obj_info, obj_info->obj_ref_cnt, objid);
        return 0;
    }

    id_obj = index->id_obj;
    
    ret = search_key_internal(id_obj, &objid, sizeof(uint64_t));
    if (0 > ret)
    {
        LOG_DEBUG("Search for obj failed. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return ret;
    }
    
   // if (id_obj->ie->value_len != VBN_SIZE)
    {
       // LOG_ERROR("The attr chaos. obj(%p) objid(%lld) value_len(%d)\n", obj, objid, id_obj->ie->value_len);
      //  return -INDEX_ERR_CHAOS;
    }
    
    //memcpy(&inode_no, GET_IE_VALUE(id_obj->ie), VBN_SIZE);
    inode_no = os_bstr_to_u64(GET_IE_VALUE(id_obj->ie), id_obj->ie->value_len);

    ret = open_object(index, objid, inode_no, &obj);
    if (0 > ret)
    {
        LOG_ERROR("Open obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    LOG_INFO("Open the obj success. index(%p) objid(%lld) obj(%p)\n",
        index, objid, obj);

    *obj_out = obj;

    return 0;
}      

int32_t index_open_object(struct _INDEX_HANDLE *index, uint64_t objid, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;

    if ((NULL == index) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. index(%p) obj(%p)\n", index, obj);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&index->index_lock);
    ret = index_open_object_nolock(index, objid, 0, obj);
    OS_RWLOCK_WRUNLOCK(&index->index_lock);

    return ret;
}      

OBJECT_HANDLE *index_get_object_handle(INDEX_HANDLE *index, uint64_t objid)
{
    OBJECT_HANDLE *tmp_obj = NULL;
    avl_index_t where = 0;

    ASSERT(NULL != index);
    ASSERT(!OBJID_IS_INVALID(objid));

    OS_RWLOCK_RDLOCK(&index->index_lock);
    tmp_obj = avl_find(&index->obj_list, (avl_find_fn)compare_object2, &objid, &where);
    OS_RWLOCK_RDLOCK(&index->index_lock);

    return tmp_obj;
}

int32_t index_close_object_nolock(OBJECT_HANDLE *obj)
{
    OBJECT_INFO *obj_info = NULL;
    
    if (NULL == obj)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    obj_info = obj->obj_info;
    if (NULL == obj_info)
    {
        LOG_ERROR("Invalid object info. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&obj_info->obj_hnd_lock);
    
    if (obj_info->obj_ref_cnt == 0)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->obj_hnd_lock);
        LOG_EMERG("Too many times put object info. objid(%lld)\n", obj_info->objid);
        return -INDEX_ERR_MANY_TIMES_PUT;
    }

    put_object_handle(obj);

    if (obj_info->obj_ref_cnt == 0) // decrease to 0
    {
        close_object(obj_info);
    }
    else
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->obj_hnd_lock);
    }
    
	return 0;
}     

int32_t index_close_object(OBJECT_HANDLE *obj)
{
    int32_t ret = 0;
	INDEX_HANDLE *index;

    if (NULL == obj)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }

	index = obj->index;
    
    OS_RWLOCK_WRLOCK(&index->index_lock);
    ret = index_close_object_nolock(obj);
    OS_RWLOCK_WRUNLOCK(&index->index_lock);
    
    return ret;
}     

int32_t index_delete_object(INDEX_HANDLE *index, uint64_t objid)
{
    return 0;
}

int32_t index_rename_object(OBJECT_HANDLE *obj, const char *new_obj_name)
{
    return 0;
}

