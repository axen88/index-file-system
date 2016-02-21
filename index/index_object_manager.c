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
    tmp_obj->obj_ref_cnt = 1;
    OS_RWLOCK_INIT(&tmp_obj->obj_lock);

    avl_add(&index->obj_list, tmp_obj);
    
    *obj = tmp_obj;

    return 0;
}

void put_object_resource(OBJECT_HANDLE *obj)
{
    OS_RWLOCK_DESTROY(&obj->obj_lock);
    avl_remove(&obj->index->obj_list, obj);

    OS_FREE(obj);

    return;
}

void recover_obj_inode(OBJECT_HANDLE *obj)
{
    memcpy(&obj->inode, &obj->old_inode, sizeof(INODE_RECORD));
}

void backup_obj_inode(OBJECT_HANDLE *obj)
{
    memcpy(&obj->old_inode, &obj->inode, sizeof(INODE_RECORD));
}

int32_t close_one_attr(void *para, DLIST_ENTRY_S *entry)
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

    // set recover dot
    backup_obj_inode(obj);
    INODE_CLR_DIRTY(obj);

    return 0;
}

int32_t close_all_attr(OBJECT_HANDLE *obj)
{
    dlist_walk_all(&obj->attr_info.attr_hnd_list, close_one_attr, NULL);
    obj->attr = NULL; // already closed
    destroy_attr_info(&obj->attr_info);
    
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
    
    init_attr_info(tmp_obj, &tmp_obj->attr_info);
    ret = index_open_attr(tmp_obj, &tmp_obj->attr);
    if (ret < 0)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) ret(%d)\n", tmp_obj->obj_name, ret);
        return ret;
    }

    IBC_SET_DIRTY(&tmp_obj->attr_info.root_ibc);

    // validate the attribute into inode
    validate_attr(&tmp_obj->attr_info);

    // update index block
    ret = index_update_block_pingpong_init(index->hnd, &tmp_obj->inode.head, inode_no);
    if (0 > ret)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Create inode failed. name(%s) vbn(%lld) ret(%d)\n",
            tmp_obj->inode.name, inode_no, ret);
        return ret;
    }
    
    LOG_DEBUG("Create inode success. name(%s) vbn(%lld)\n",
        tmp_obj->inode.name, inode_no);

    backup_obj_inode(tmp_obj);
    *obj = tmp_obj;

    return 0;
}

int32_t close_object(OBJECT_HANDLE *obj)
{
    if (0 != obj->obj_ref_cnt)
    {
        LOG_ERROR("There are still object handle not closed. objid(%lld) obj_ref_cnt(%d)\n",
            obj->objid, obj->obj_ref_cnt);
    }
    
    commit_object_modification(obj);
    close_all_attr(obj);
    put_object_resource(obj);

    return 0;
}

int32_t open_object(INDEX_HANDLE *index, uint64_t objid, uint64_t inode_no, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj= NULL;

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
	init_attr_info(tmp_obj, &tmp_obj->attr_info);
    ret = index_open_attr(tmp_obj, &tmp_obj->attr);
    if (ret < 0)
    {
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) ret(%d)\n", tmp_obj->obj_name, ret);
        return ret;
    }

    backup_obj_inode(tmp_obj);
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

    ret = search_key_internal(index->id_obj->attr, &objid, sizeof(uint64_t));
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

    ret = index_insert_key_nolock(index->id_obj->attr, &objid, sizeof(uint64_t),
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

    attr = index->id_obj->attr;
    
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

void index_cancel_object_modification(OBJECT_HANDLE *obj)
{
    ASSERT(obj != NULL);

    // recover the inode content
    recover_obj_inode(obj);
    INODE_CLR_DIRTY(obj);

    cancel_attr_modification(&obj->attr_info);
    
    return;
}

int32_t commit_object_modification(OBJECT_HANDLE *obj)
{
    int32_t ret = 0;
    
    ASSERT(obj != NULL);

    return index_commit_attr_modification(&obj->attr_info);
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

int32_t index_delete_object(INDEX_HANDLE *index, uint64_t objid)
{
    return 0;
}

int32_t index_rename_object(OBJECT_HANDLE *obj, const char *new_obj_name)
{
    return 0;
}

