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
文 件 名: INDEX_OBJECT_MANAGER.C
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

#define DELETE_FLAG    0x01

int32_t compare_object1(const OBJECT_HANDLE *obj, const OBJECT_HANDLE *obj_node)
{
    if (obj->objid > obj_node->objid)
    {
        return 1;
    }
    else if (obj->objid == obj_node->objid)
    {
        return 0;
    }

    return -1;
}

int32_t compare_object2(const void *objid, OBJECT_HANDLE *obj_node)
{
    if ((uint64_t)objid > obj_node->objid)
    {
        return 1;
    }
    else if ((uint64_t)objid == obj_node->objid)
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

int32_t get_object_resource(INDEX_HANDLE *index, uint64_t objid, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;

    tmp_obj = (OBJECT_HANDLE *)OS_MALLOC(sizeof(OBJECT_HANDLE));
    if (NULL == tmp_obj)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_obj, 0, sizeof(OBJECT_HANDLE));
    tmp_obj->index = index;
    tmp_obj->objid = objid;
    dlist_init_head(&tmp_obj->attr_hnd_list);
    avl_create(&tmp_obj->obj_caches, (int (*)(const void *, const void*))compare_cache1, sizeof(INDEX_BLOCK_CACHE),
        OS_OFFSET(INDEX_BLOCK_CACHE, obj_entry));
    avl_create(&tmp_obj->obj_old_blocks, (int (*)(const void *, const void*))compare_old_block1, sizeof(INDEX_OLD_BLOCK),
        OS_OFFSET(INDEX_OLD_BLOCK, obj_entry));
    tmp_obj->obj_ref_cnt = 1;
    OS_RWLOCK_INIT(&tmp_obj->obj_lock);
    OS_RWLOCK_INIT(&tmp_obj->caches_lock);

    avl_add(&index->obj_list, tmp_obj);
    
    *obj = tmp_obj;

    return 0;
}

int32_t put_all_attr(OBJECT_HANDLE *obj)
{
    return 0;
}

void put_object_resource(OBJECT_HANDLE *obj)
{
    /* 检查是否为脏 */
    
    put_all_attr(obj);
    
    avl_destroy(&obj->obj_caches);
    avl_destroy(&obj->obj_old_blocks);

    OS_RWLOCK_DESTROY(&obj->caches_lock);
    OS_RWLOCK_DESTROY(&obj->obj_lock);

    OS_FREE(obj);

    return;
}

int32_t close_extra_attr(void *para, DLIST_ENTRY_S *entry)
{
    ATTR_HANDLE *attr = NULL;

    attr = OS_CONTAINER(entry, ATTR_HANDLE, entry);

    return index_close_attr(attr);

}

int32_t close_base_attr(void *para, DLIST_ENTRY_S *entry)
{
    ATTR_HANDLE *attr = NULL;

    attr = OS_CONTAINER(entry, ATTR_HANDLE, entry);
    
    return index_close_attr(attr);

}

int32_t flush_inode(OBJECT_HANDLE * obj)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    ASSERT(NULL != obj);

    if (!INODE_DIRTY(obj))
    {
        return 0;
    }

    /* 刷入脏数据 */
    ret = INDEX_UPDATE_INODE(obj);
    if (0 > ret)
    {
        LOG_ERROR("Update inode failed. name(%s) inode(%p) objid(%lld) ret(%d)\n",
            obj->obj_name, obj->inode, obj->inode.objid, ret);
        return ret;
    }

    LOG_DEBUG("Update inode success. name(%s) inode(%p) objid(%lld)\n",
        obj->obj_name, obj->inode, obj->inode.objid);

    /* 设置inode回复点，供取消修改时使用 */
    memcpy(&obj->old_inode, &obj->inode, sizeof(INODE_RECORD));
    INODE_CLR_DIRTY(obj);

    return 0;
}

int32_t close_all_attr(OBJECT_HANDLE *obj)
{
    dlist_walk_all(&obj->attr_hnd_list, close_extra_attr, NULL);
    dlist_walk_all(&obj->attr_hnd_list, close_base_attr, NULL);
    index_close_attr(obj->attr);
    
    return 0;
}

int32_t create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj)
{
     int32_t ret = sizeof(INODE_RECORD);
     OBJECT_HANDLE *tmp_obj = NULL;
     uint64_t inode_no = 0;
     ATTR_RECORD *attr_record = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != obj);
    ASSERT(INODE_SIZE == sizeof(INODE_RECORD));

    /* allocate inode block */
    ret = block_alloc(index->hnd, 1, &inode_no);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    ret = get_object_resource(index, objid, &tmp_obj);
    if (ret < 0)
    {
        block_free(index->hnd, inode_no, 1);
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    /* init inode */
    tmp_obj->inode.head.blk_id = INODE_MAGIC;
    tmp_obj->inode.head.alloc_size = INODE_SIZE;
    tmp_obj->inode.head.real_size = INODE_SIZE;
    
    tmp_obj->inode.first_attr_off = OS_OFFSET(INODE_RECORD, reserved);
    
    tmp_obj->inode.objid = objid;
    tmp_obj->inode.base_objid = 0;
    
    tmp_obj->inode.mode = 0;
    tmp_obj->inode.uid = 0;
    tmp_obj->inode.gid = 0;
    tmp_obj->inode.size = 0;
    tmp_obj->inode.links = 0;
    tmp_obj->inode.ctime = 0;
    tmp_obj->inode.atime = 0;
    tmp_obj->inode.mtime = 0;
    
    tmp_obj->inode.snapshot_no = index->hnd->sb.snapshot_no;
    tmp_obj->inode.name_size = DEFAULT_OBJ_NAME_SIZE;
    strncpy(tmp_obj->inode.name, DEFAULT_OBJ_NAME, OBJ_NAME_SIZE);
    
    strncpy(tmp_obj->obj_name, DEFAULT_OBJ_NAME, OBJ_NAME_SIZE);
    tmp_obj->inode_no = inode_no;

    /* init attr */
    attr_record = INODE_GET_ATTR(&tmp_obj->inode);
    attr_record->record_size = ATTR_RECORD_SIZE;
    attr_record->attr_flags = flags;
    if (flags & FLAG_TABLE)
    { /* table */
        init_ib((INDEX_BLOCK *)&attr_record->content, INDEX_BLOCK_SMALL, ATTR_RECORD_CONTENT_SIZE);
    }
    else
    { /* data stream */
        memset(attr_record->content, 0, ATTR_RECORD_CONTENT_SIZE);
    }
    
    init_attr_info(tmp_obj, attr_record, &tmp_obj->attr_info);

    ret = index_open_attr(tmp_obj, &tmp_obj->attr);
    if (ret < 0)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) ret(%d)\n", tmp_obj->obj_name, ret);
        return ret;
    }

    IBC_SET_DIRTY(&tmp_obj->attr->attr_info->root_ibc);

    /* 生效以上修改 */
    validate_attr(&tmp_obj->attr_info);

    /* 更新到inode信息中去 */
    ret = index_update_block_pingpong_init(index->hnd, &tmp_obj->inode.head, inode_no);
    if (0 > ret)
    {   /* 新数据覆盖旧有数据 */
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Create inode failed. name(%s) vbn(%lld) ret(%d)\n",
            tmp_obj->inode.name, inode_no, ret);
        return ret;
    }
    
    LOG_DEBUG("Create inode success. name(%s) vbn(%lld)\n",
        tmp_obj->inode.name, inode_no);

    memcpy(&tmp_obj->old_inode, &tmp_obj->inode, sizeof(INODE_RECORD));
    *obj = tmp_obj;

    return 0;
}

int32_t close_one_object(OBJECT_HANDLE *obj)
{
    index_commit_object_modification(obj);
    close_all_attr(obj);

    //index_release_all_caches_in_obj(obj);
    
    put_object_resource(obj);

    return 0;
}

int32_t close_object(OBJECT_HANDLE *obj)
{
    OBJECT_HANDLE *cur_obj = obj;
    OBJECT_HANDLE *parent_obj = obj;
    
    close_one_object(obj);

    return 0;
}

int32_t open_object(INDEX_HANDLE *index, uint64_t objid, uint64_t inode_no, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj= NULL;
    ATTR_RECORD *attr_record = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != obj);

    ret = get_object_resource(index, objid, &tmp_obj);
    if (ret < 0)
    {
        LOG_ERROR("Get object resource failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = INDEX_READ_INODE(index, tmp_obj, inode_no);
    if (0 > ret)
    {
        LOG_ERROR("Read inode failed. ret(%d)\n", ret);
        put_object_resource(tmp_obj);
        return ret;
    }
    
    strncpy(tmp_obj->obj_name, tmp_obj->inode.name, tmp_obj->inode.name_size);

    /* open attr */
    attr_record = INODE_GET_ATTR(&tmp_obj->inode);
	init_attr_info(tmp_obj, attr_record, &tmp_obj->attr_info);
    ret = index_open_attr(tmp_obj, &tmp_obj->attr);
    if (ret < 0)
    {
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) ret(%d)\n", tmp_obj->obj_name, ret);
        return ret;
    }

    memcpy(&tmp_obj->old_inode, &tmp_obj->inode, sizeof(INODE_RECORD));
    *obj = tmp_obj;

    return 0;
}

uint64_t get_objid(INDEX_HANDLE *index)
{
    return 1;
}

int32_t index_create_object_nolock(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;
    uint16_t name_size = 0;
    avl_index_t where = 0;

    ASSERT(NULL != index);
    ASSERT(!OBJID_IS_INVALID(objid));
    ASSERT(NULL != obj);

    LOG_INFO("Create the obj. obj(%p) objid(%lld)\n", tmp_obj, objid);

    tmp_obj = avl_find(&index->obj_list, compare_object2, (void *)objid, &where);
    if (NULL != tmp_obj)
    {
        LOG_ERROR("The obj already exist. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return -INDEX_ERR_OBJ_EXIST;
    }

    ret = search_key_internal(index->idlst_obj->attr, &objid, sizeof(uint64_t));
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

    ret = create_object(index, objid, flags, &tmp_obj);
    if (ret < 0)
    {
        LOG_ERROR("Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = index_insert_key_nolock(index->idlst_obj->attr, &objid, sizeof(uint64_t),
        &tmp_obj->inode_no, VBN_SIZE);
    if (ret < 0)
    {
        LOG_ERROR("Insert obj failed. obj(%p) objid(%lld) ret(%d)\n",
            obj, objid, ret);
        (void)INDEX_FREE_BLOCK(tmp_obj, tmp_obj->inode_no);
        close_object(tmp_obj);
        return ret;
    }
    
    LOG_INFO("Create the obj success. objid(%lld) obj(%p) index_name(%s)\n",
        objid, tmp_obj, index->name);

    *obj = tmp_obj;
    
    return 0;
}    

int32_t index_create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;

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

int32_t index_open_object_nolock(struct _INDEX_HANDLE *index, uint64_t objid, uint32_t open_flags, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;
    uint64_t inode_no = 0;
    avl_index_t where = 0;
    ATTR_HANDLE *attr;

    ASSERT(NULL != index);
    ASSERT(NULL != obj);

    LOG_INFO("Open the obj. objid(%lld)\n", objid);

    tmp_obj = avl_find(&index->obj_list, compare_object2, (void *)objid, &where);
    if (NULL != tmp_obj)
    {
        if (0 != (open_flags & DELETE_FLAG))
        {
			if (0 != tmp_obj->obj_ref_cnt)
			{
				*obj = tmp_obj;
				LOG_WARN("The obj is being deleted. obj(%p) obj_ref_cnt(%d) objid(%lld)\n",
					tmp_obj, tmp_obj->obj_ref_cnt, objid);
				return -INDEX_ERR_IS_OPENED;
			}

            avl_remove(&index->obj_list, tmp_obj);
        }
        
        tmp_obj->obj_ref_cnt++;
        *obj = tmp_obj;
        LOG_WARN("The obj obj_ref_cnt inc. obj(%p) obj_ref_cnt(%d) objid(%lld)\n",
            tmp_obj, tmp_obj->obj_ref_cnt, objid);
        return 0;
    }

    attr = index->idlst_obj->attr;
    
    ret = search_key_internal(attr, &objid, sizeof(uint64_t));
    if (0 > ret)
    {
        LOG_DEBUG("Search for obj failed. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return ret;
    }

    if (attr->ie->value_len != VBN_SIZE)
    {
        LOG_ERROR("The attr chaos. obj(%p) objid(%lld) value_len(%d)\n", obj, objid, attr->ie->value_len);
        return -INDEX_ERR_CHAOS;
    }

    memcpy(&inode_no, IEGetValue(attr->ie), VBN_SIZE);
    
    ret = open_object(index, objid, inode_no, &tmp_obj);
    if (0 > ret)
    {
        LOG_ERROR("Open obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    strncpy(tmp_obj->obj_name, tmp_obj->inode.name, tmp_obj->inode.name_size);

    LOG_INFO("Open the obj success. index(%p) objid(%lld) obj(%p)\n",
        index, objid, tmp_obj);

    *obj = tmp_obj;

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
    tmp_obj = avl_find(&index->obj_list, compare_object2, (void *)objid, &where);
    OS_RWLOCK_RDLOCK(&index->index_lock);

    return tmp_obj;
}

int32_t recover_attr_record(ATTR_INFO *attr_info)
{
    memcpy(&attr_info->attr_record, &attr_info->old_attr_record,
        sizeof(ATTR_RECORD));
    ATTR_INFO_CLR_DIRTY(attr_info);

    return 0;
}

void index_cancel_object_modification(OBJECT_HANDLE *obj)
{
    index_cancel_all_caches_in_obj(obj);

    /* 恢复所有属性记录到修改之前的状态 */
    recover_attr_record(&obj->attr_info);

    /* 恢复inode信息 */
    memcpy(&obj->inode, &obj->old_inode, sizeof(INODE_RECORD));
    INODE_CLR_DIRTY(obj);

    index_release_all_old_blocks_mem_in_obj(obj);

    index_release_all_free_caches_in_obj(obj);
    
    return;
}

void backup_attr_record(ATTR_INFO *attr_info)
{
    /* 备份属性记录 */
    memcpy(&attr_info->old_attr_record, &attr_info->attr_record,
        sizeof(ATTR_RECORD));

    return;
}

int32_t index_commit_object_modification(OBJECT_HANDLE *obj)
{
    int32_t ret = 0;
    
    ASSERT(obj != NULL);

    /* 使所有的属性修改生效 */
    validate_attr(&obj->attr_info);

    /* 将cache中的脏数据下盘 */
    ret = index_flush_all_caches_in_obj(obj);
    if (0 > ret)
    {
        LOG_ERROR("Flush index blocks failed. obj_name(%s) ret(%d)\n", obj->obj_name, ret);
        return ret;
    }

    /* 将inode中的脏数据下盘，此处成功则可认为事务提交成功 */
    ret = flush_inode(obj);
    if (0 > ret)
    {
        LOG_ERROR("Flush inode failed. obj_name(%s) ret(%d)\n", obj->obj_name, ret);
        return ret;
    }

    backup_attr_record(&obj->attr_info);

    /* 删除修改过的块对应的旧块，因为前面已经成功了，所以这里是否成功没关系 */
    index_release_all_old_blocks_in_obj(obj);

    index_release_all_free_caches_in_obj(obj);

    return 0;
}

int32_t index_close_object_nolock(OBJECT_HANDLE *obj)
{
    if (NULL == obj)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    LOG_INFO("Close the obj. obj(%p) obj_ref_cnt(%d)\n", obj, obj->obj_ref_cnt);
    if (0 == obj->obj_ref_cnt)
    {
        LOG_INFO("The obj_ref_cnt is 0. obj(%p) obj_ref_cnt(%d) name(%s)\n",
            obj, obj->obj_ref_cnt, obj->inode.name);
        
        return 0;
    }

    if (0 != --obj->obj_ref_cnt)
    {
        LOG_WARN("The obj obj_ref_cnt dec. obj(%p) obj_ref_cnt(%d) name(%s)\n",
            obj, obj->obj_ref_cnt, obj->inode.name);
        return 0;
    }

    close_object(obj);
    
	return 0;
}     

int32_t index_close_object(OBJECT_HANDLE *obj)
{
    int32_t ret = 0;

    if (NULL == obj)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    OS_RWLOCK_WRLOCK(&obj->index->index_lock);
    ret = index_close_object_nolock(obj);
    OS_RWLOCK_WRUNLOCK(&obj->index->index_lock);
    
    return ret;
}     

int32_t index_delete_object(INDEX_HANDLE *index, uint64_t objid, void *hnd, DeleteFunc del_func)
{
#if 0
    int32_t ret = 0;
    OBJECT_HANDLE *obj = NULL;

    if ((NULL == parent_obj) || (NULL == obj_name))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p)\n",
            parent_obj, obj_name);
        return -INDEX_ERR_PARAMETER;
    }

    /* 锁目录树，防止树再次被打开 */
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    
    LOG_INFO("Delete the obj. obj(%p) obj_name(%s) obj(%p)\n",
        obj, obj_name, obj);

    ret = index_open_object_nolock(parent_obj, obj_name, 
        DELETE_FLAG, &obj);
    if (0 > ret)
    {
        LOG_ERROR("The obj not found. name(%s)\n", obj_name);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }
    
    ret = index_remove_key_nolock(&parent_obj->attr->tree, obj_name, (uint16_t)strlen(obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Remove the obj failed. name(%s)\n", obj_name);
        (void)index_close_object_nolock(obj);
        (void)index_close_object_nolock(obj); /* 关2次，以便释放资源 */
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);

    if (walk_tree(obj, INDEX_GET_FIRST) == 0)
    {
        do
        {
            if (NULL != del_func)
            {
                (void)del_func(hnd,
                    IEGetKey(obj->ie), obj->ie->key_len,
                    IEGetValue(obj->ie), obj->ie->value_len);
            }
        }
        while (walk_tree(obj, INDEX_REMOVE_BLOCK) == 0);
    }

    ret = index_record_old_block(obj->old_block_queue, obj->inode->inode_no, 1);
    if (0 > ret)
    {
        LOG_ERROR("Record old block failed. ret(%d)\n", ret);
        (void)INDEX_CancelTreeTransNoLock(obj);
        (void)index_close_object_nolock(obj);
        (void)index_close_object_nolock(obj); /* 关2次，以便释放资源 */
        return ret;
    }
    (void)index_close_object_nolock(obj);
    (void)index_close_object_nolock(obj); /* 关2次，以便释放资源 */


    LOG_INFO("Delete the obj success. obj(%p) obj_name(%s)\n",
        obj, obj_name);
    
#endif

    return 0;
}

int32_t index_rename_object(OBJECT_HANDLE *parent_obj, const char *old_obj_name,
    const char * new_obj_name)
{
    #if 0
    int32_t ret = 0;
    OBJECT_HANDLE *obj = NULL;
    uint64_t inode_no = 0;

    if ((NULL == obj) || (NULL == old_obj_name)
        || (NULL == new_obj_name))
    {
        LOG_ERROR("Invalid parameter. obj(%p) old_obj_name(%p) new_obj_name(%p)\n",
            obj, old_obj_name, new_obj_name);
        return -INDEX_ERR_PARAMETER;
    }

    LOG_INFO("Rename the obj. obj(%p) old_obj_name(%s) new_obj_name(%s)\n",
        obj, old_obj_name, new_obj_name);
    
    /* 锁目录树，防止树再次被打开 */
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    
    ret = search_key_internal(&parent_obj->attr->tree, old_obj_name,
        (uint16_t)strlen(old_obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Search for obj failed. parent_obj(%p) name(%s) ret(%d)\n",
            parent_obj, old_obj_name, ret);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    if (parent_obj->attr->ie->value_len != VBN_SIZE)
    {
        LOG_DEBUG("The attr chaos."
            " [obj: %p, name: %s, value_len: %d]\n", obj, obj_name, parent_obj->attr->ie->value_len);
        return -INDEX_ERR_CHAOS;
    }

    memcpy(&inode_no, IEGetValue(parent_obj->attr->ie), VBN_SIZE);

    (void)INDEX_StartTreeTransNoLock(&parent_obj->attr->tree);

    ret = index_remove_key_nolock(&parent_obj->attr->tree, old_obj_name,
        (uint16_t)strlen(old_obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Remove obj failed. name(%s) ret(%d)\n", old_obj_name, ret);
        (void)INDEX_CancelTreeTransNoLock(&parent_obj->attr->tree);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    ret = index_insert_key_nolock(&parent_obj->attr->tree, new_obj_name,
        (uint16_t)strlen(new_obj_name), &inode_no, VBN_SIZE);
    if (0 > ret)
    {
        LOG_ERROR("Insert obj failed. name(%s) ret(%d)\n", new_obj_name, ret);
        (void)INDEX_CancelTreeTransNoLock(&parent_obj->attr->tree);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    ret = INDEX_CommitTreeTransNoLock(&parent_obj->attr->tree, COMMIT_FLAG_FORCE);
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);

    LOG_INFO("Rename the obj finished. parent_obj(%p) old_obj_name(%s) new_obj_name(%s) ret(%d)\n",
        parent_obj, old_obj_name, new_obj_name, ret);
    
    return ret;
    #endif
    return 0;
}

