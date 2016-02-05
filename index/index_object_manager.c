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

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: INDEX_OBJECT_MANAGER.C
��    ��: 1.00
��    ��: 2012��6��23��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2012��6��23��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

#define DELETE_FLAG    0x01


int32_t compare_attr_info1(const ATTR_INFO *attr_info, const ATTR_INFO *attr_info_node)
{
    return os_collate_ansi_string(attr_info->attr_name, strlen(attr_info->attr_name),
        attr_info_node->attr_name, strlen(attr_info_node->attr_name));
}

int32_t compare_attr_info2(const char *attr_name, ATTR_INFO *attr_info_node)
{
    return os_collate_ansi_string(attr_name, strlen(attr_name),
        attr_info_node->attr_name, strlen(attr_info_node->attr_name));
}

int32_t compare_object1(const OBJECT_HANDLE *obj, const OBJECT_HANDLE *obj_node)
{
    return os_collate_ansi_string(obj->obj_name, strlen(obj->obj_name),
        obj_node->obj_name, strlen(obj_node->obj_name));
}

int32_t compare_object2(const char *obj_name, OBJECT_HANDLE *obj_node)
{
    return os_collate_ansi_string(obj_name, strlen(obj_name),
        obj_node->obj_name, strlen(obj_node->obj_name));
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

int32_t get_object_resource(INDEX_HANDLE *index, OBJECT_HANDLE *parent_obj,
    const char *obj_name, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;

    /* ����obj�ڴ� */
    tmp_obj = (OBJECT_HANDLE *)OS_MALLOC(sizeof(OBJECT_HANDLE));
    if (NULL == tmp_obj)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_obj, 0, sizeof(OBJECT_HANDLE));

    tmp_obj->index = index;

    /* ��ʼ����inode֮�����Ϣ */
    strncpy(tmp_obj->obj_name, obj_name, OBJ_NAME_SIZE);
    tmp_obj->parent_obj = parent_obj;
    avl_create(&tmp_obj->child_obj_list, (int (*)(const void *, const void*))compare_object1, sizeof(OBJECT_HANDLE),
        OS_OFFSET(OBJECT_HANDLE, entry));
    avl_create(&tmp_obj->attr_info_list, (int (*)(const void *, const void*))compare_attr_info1, sizeof(ATTR_INFO),
        OS_OFFSET(ATTR_INFO, entry));
    dlist_init_head(&tmp_obj->attr_hnd_list);
    avl_create(&tmp_obj->obj_caches, (int (*)(const void *, const void*))compare_cache1, sizeof(INDEX_BLOCK_CACHE),
        OS_OFFSET(INDEX_BLOCK_CACHE, obj_entry));
    avl_create(&tmp_obj->obj_old_blocks, (int (*)(const void *, const void*))compare_old_block1, sizeof(INDEX_OLD_BLOCK),
        OS_OFFSET(INDEX_OLD_BLOCK, obj_entry));
    tmp_obj->obj_ref_cnt = 1;
    OS_RWLOCK_INIT(&tmp_obj->obj_lock);
    OS_RWLOCK_INIT(&tmp_obj->caches_lock);
    
    if (parent_obj != NULL)
    {
        avl_add(&parent_obj->child_obj_list, tmp_obj);
    }

    *obj = tmp_obj;

    return 0;
}

int32_t put_all_attr(OBJECT_HANDLE *obj)
{
    return 0;
}

void put_object_resource(OBJECT_HANDLE *obj)
{
    /* ����Ƿ�Ϊ�� */
    
    put_all_attr(obj);
    
    if (obj->parent_obj != NULL)
    {
        avl_remove(&obj->parent_obj->child_obj_list, obj);
    }

    avl_destroy(&obj->child_obj_list);
    avl_destroy(&obj->attr_info_list);
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
    OBJECT_HANDLE *obj = NULL;

    attr = OS_CONTAINER(entry, ATTR_HANDLE, entry);
    obj = attr->attr_info->obj;

    if (attr->attr_info->parent_attr == NULL)
    {
        return 0;
    }
    
    if (attr->attr_info->parent_attr->attr_info == obj->xattr->attr_info)
    {
        return index_close_attr(attr);
    }

    return 0;
}

int32_t close_base_attr(void *para, DLIST_ENTRY_S *entry)
{
    ATTR_HANDLE *attr = NULL;
    OBJECT_HANDLE *obj = NULL;

    attr = OS_CONTAINER(entry, ATTR_HANDLE, entry);
    obj = attr->attr_info->obj;
    
    if (attr->attr_info->parent_attr == NULL)
    {
        return 0;
    }
    
    if (attr->attr_info->parent_attr->attr_info == obj->battr->attr_info)
    {
        return index_close_attr(attr);
    }

    return 0;
}

int32_t validate_extra_attr(void *para, ATTR_INFO *attr_info)
{
    ASSERT(attr_info != NULL);

    if (attr_info->parent_attr == NULL)
    {
        return 0;
    }
    
    if (attr_info->parent_attr->attr_info == attr_info->obj->xattr->attr_info)
    {
        return validate_attr(attr_info);
    }

    return 0;
}

int32_t validate_base_attr(void *para, ATTR_INFO *attr_info)
{
    if (attr_info->parent_attr == NULL)
    {
        return 0;
    }
    
    if (attr_info->parent_attr->attr_info == attr_info->obj->battr->attr_info)
    {
        return validate_attr(attr_info);
    }

    return 0;
}

int32_t validate_all_attr(OBJECT_HANDLE *obj)
{
    avl_walk_all(&obj->attr_info_list, (avl_walk_call_back)validate_extra_attr, NULL);
    avl_walk_all(&obj->attr_info_list, (avl_walk_call_back)validate_base_attr, NULL);
    validate_attr(obj->battr->attr_info);
    
    return 0;
}

int32_t flush_inode(OBJECT_HANDLE * obj)
{
    int32_t ret = 0;

    /* ���������� */
    ASSERT(NULL != obj);

    if (!INODE_DIRTY(obj))
    {
        return 0;
    }

    /* ˢ�������� */
    ret = INDEX_UPDATE_INODE(obj);
    if (0 > ret)
    {
        LOG_ERROR("Update inode failed. name(%s) inode(%p) vbn(%lld) ret(%d)\n",
            obj->obj_name, obj->inode, obj->inode.inode_no, ret);
        return ret;
    }

    LOG_DEBUG("Update inode success. name(%s) inode(%p) vbn(%lld)\n",
        obj->obj_name, obj->inode, obj->inode.inode_no);

    /* ����inode�ظ��㣬��ȡ���޸�ʱʹ�� */
    memcpy(&obj->old_inode, &obj->inode, sizeof(INODE_RECORD));
    INODE_CLR_DIRTY(obj);

    return 0;
}

int32_t close_all_attr(OBJECT_HANDLE *obj)
{
    dlist_walk_all(&obj->attr_hnd_list, close_extra_attr, NULL);
    dlist_walk_all(&obj->attr_hnd_list, close_base_attr, NULL);
    index_close_attr(obj->battr);
    
    return 0;
}

int32_t create_object(INDEX_HANDLE *index, OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint64_t mode, OBJECT_HANDLE **obj)
{
     int32_t ret = sizeof(INODE_RECORD);
     OBJECT_HANDLE *tmp_obj = NULL;
     uint64_t inode_no = 0;
     BATTR_RECORD *battr = NULL;
     ATTR_RECORD *attr_record = NULL;

    ASSERT(NULL != obj_name);
    ASSERT(NULL != index);
    ASSERT(NULL != obj);
    ASSERT(strlen(obj_name) < OBJ_NAME_SIZE);
    ASSERT(INODE_SIZE == sizeof(INODE_RECORD));

    /* ����һ������inode��Ϣ */
    ret = block_alloc(index->hnd, 1, &inode_no);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    /* ����obj�ڴ� */
    ret = get_object_resource(index, parent_obj, obj_name, &tmp_obj);
    if (ret < 0)
    {
        block_free(index->hnd, inode_no, 1);
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(OBJECT_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    /* ��ʼ��inode��Ϣ */
    tmp_obj->inode.head.blk_id = INODE_MAGIC;
    tmp_obj->inode.head.alloc_size = INODE_SIZE;
    tmp_obj->inode.head.real_size = INODE_SIZE;
    
    tmp_obj->inode.first_attr_off = OS_OFFSET(INODE_RECORD, reserved);
    
    tmp_obj->inode.inode_no = inode_no;
    tmp_obj->inode.parent_inode_no = (parent_obj == NULL) ? 0 : parent_obj->inode.inode_no;
    
    tmp_obj->inode.mode = mode;
    tmp_obj->inode.uid = 0;
    tmp_obj->inode.gid = 0;
    tmp_obj->inode.size = 0;
    tmp_obj->inode.links = 0;
    tmp_obj->inode.ctime = 0;
    tmp_obj->inode.atime = 0;
    tmp_obj->inode.mtime = 0;
    
    tmp_obj->inode.snapshot_no = index->hnd->sb.snapshot_no;
    
    tmp_obj->inode.name_size = strlen(obj_name);
    strncpy(tmp_obj->inode.name, obj_name, OBJ_NAME_SIZE);

    /* ��ʼ��BATTR */
    battr = INODE_GET_BATTR(&tmp_obj->inode);
    attr_record = &battr->attr_record;
    attr_record->record_size = BATTR_RECORD_SIZE;
    attr_record->attr_flags = ATTR_FLAG_SYSTEM | ATTR_FLAG_TABLE | COLLATE_ANSI_STRING;
    strncpy(battr->name, BATTR_NAME, BATTR_NAME_SIZE);
    init_ib((INDEX_BLOCK *)attr_record->content, INDEX_BLOCK_SMALL,
        BATTR_RECORD_CONTENT_SIZE);
    ret = index_open_attr(tmp_obj, NULL, BATTR_NAME, attr_record,
        &tmp_obj->battr);
    if (ret < 0)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, BATTR_NAME, ret);
        return ret;
    }

    attr_record = (ATTR_RECORD *)OS_MALLOC(sizeof(ATTR_RECORD));
    if (NULL == attr_record)
    {
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(ATTR_RECORD));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    /* ��ʼ��MATTR */
    memset(attr_record, 0, sizeof(ATTR_RECORD));
    attr_record->record_size = MATTR_RECORD_SIZE;
    if (mode & OBJECT_MODE_DIR)
    { /* Ŀ¼ */
        attr_record->attr_flags = ATTR_FLAG_SYSTEM | ATTR_FLAG_TABLE | COLLATE_ANSI_STRING;
        init_ib((INDEX_BLOCK *)&attr_record->content, INDEX_BLOCK_SMALL,
            MATTR_RECORD_CONTENT_SIZE);
    }
    else if (mode & OBJECT_MODE_TABLE)
    { /* �Զ���� */
        attr_record->attr_flags = ATTR_FLAG_SYSTEM | ATTR_FLAG_TABLE | (mode & COLLATE_RULE_MASK);
        init_ib((INDEX_BLOCK *)&attr_record->content, INDEX_BLOCK_SMALL,
            MATTR_RECORD_CONTENT_SIZE);
    }
    else
    { /* ������ */
        attr_record->attr_flags = ATTR_FLAG_SYSTEM;
    }

    ret = index_create_attr(tmp_obj, tmp_obj->battr, MATTR_NAME, attr_record,
        &tmp_obj->mattr);
    if (ret < 0)
    {
        OS_FREE(attr_record);
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Create attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, MATTR_NAME, ret);
        return ret;
    }

    IBC_SET_DIRTY(&tmp_obj->mattr->attr_info->root_ibc);

    /* ����XATTR */
    memset(attr_record, 0, sizeof(ATTR_RECORD));
    attr_record->record_size = XATTR_RECORD_SIZE;
    attr_record->attr_flags = ATTR_FLAG_SYSTEM | ATTR_FLAG_TABLE | COLLATE_ANSI_STRING;
    init_ib((INDEX_BLOCK *)&attr_record->content, INDEX_BLOCK_SMALL,
        XATTR_RECORD_CONTENT_SIZE);
    ret = index_create_attr(tmp_obj, tmp_obj->battr, XATTR_NAME, attr_record,
        &tmp_obj->xattr);
    if (ret < 0)
    {
        OS_FREE(attr_record);
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Create attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, XATTR_NAME, ret);
        return ret;
    }

    IBC_SET_DIRTY(&tmp_obj->xattr->attr_info->root_ibc);

    /* ��Ч�����޸� */
    validate_all_attr(tmp_obj);

    /* ���µ�inode��Ϣ��ȥ */
    ret = index_update_block_pingpong_init(index->hnd, &tmp_obj->inode.head,
        inode_no);
    if (0 > ret)
    {   /* �����ݸ��Ǿ������� */
        OS_FREE(attr_record);
        (void)block_free(index->hnd, inode_no, 1);
        put_object_resource(tmp_obj);
        LOG_ERROR("Create inode failed. name(%s) vbn(%lld) ret(%d)\n",
            tmp_obj->inode.name, inode_no, ret);
        return ret;
    }
    
    OS_FREE(attr_record);
    
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
    
    for (;;)
    {
        if (avl_numnodes(&cur_obj->child_obj_list) == 0)
        {
            close_one_object(cur_obj);
            if (cur_obj == obj)
            {
                break;
            }
            else if (avl_numnodes(&parent_obj->child_obj_list) != 0)
            {
                cur_obj = avl_first(&parent_obj->child_obj_list);
            }
            else
            {
                cur_obj = parent_obj;
                parent_obj = cur_obj->parent_obj;
            }
        }
        else
        {
            parent_obj = cur_obj;
            cur_obj = avl_first(&parent_obj->child_obj_list);
        }
    }

    return 0;
}

int32_t open_object(INDEX_HANDLE *index, OBJECT_HANDLE *parent_obj, const char *obj_name,
    uint64_t inode_no, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj= NULL;
    BATTR_RECORD *battr = NULL;
    ATTR_RECORD *attr_record = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != obj);

    /* ����obj�ڴ� */
    ret = get_object_resource(index, parent_obj, obj_name, &tmp_obj);
    if (ret < 0)
    {
        LOG_ERROR("Get object resource failed. ret(%d)\n", ret);
        return ret;
    }

    ret = INDEX_READ_INODE(index, tmp_obj, inode_no);
    if (0 > ret)
    {
        LOG_ERROR("Read inode failed. ret(%d)\n", ret);
        put_object_resource(tmp_obj);
        return ret;
    }

    /* ��BATTR */
    battr = INODE_GET_BATTR(&tmp_obj->inode);
    if (strcmp(BATTR_NAME, battr->name) != 0)
    {
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, BATTR_NAME, ret);
        return -INDEX_ERR_ATTR_NOT_FOUND;
    }
    
    ret = index_open_attr(tmp_obj, NULL, BATTR_NAME, &battr->attr_record,
        &tmp_obj->battr);
    if (ret < 0)
    {
        put_object_resource(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, BATTR_NAME, ret);
        return ret;
    }
    
    /* ��MATTR */
    ret = index_open_attr(tmp_obj, tmp_obj->battr, MATTR_NAME, NULL,
        &tmp_obj->mattr);
    if (ret < 0)
    {
        close_one_object(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, MATTR_NAME, ret);
        return ret;
    }

    /* ��XATTR */
    ret = index_open_attr(tmp_obj, tmp_obj->battr, XATTR_NAME, NULL,
        &tmp_obj->xattr);
    if (ret < 0)
    {
        close_one_object(tmp_obj);
        LOG_ERROR("Open attr failed. obj_name(%s) attr_name(%s) ret(%d)\n",
            tmp_obj->obj_name, XATTR_NAME, ret);
        return ret;
    }

    memcpy(&tmp_obj->old_inode, &tmp_obj->inode, sizeof(INODE_RECORD));
    *obj = tmp_obj;

    return 0;
}


int32_t index_create_object_nolock(OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint64_t mode, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;
    uint16_t name_size = 0;
    avl_index_t where = 0;

    if ((NULL == parent_obj) || (NULL == obj_name) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p) obj(%p)\n",
            parent_obj, obj_name, obj);
        return -INDEX_ERR_PARAMETER;
    }

    name_size = (uint16_t)strlen(obj_name);
    if (name_size >= OBJ_NAME_SIZE)
    {
        LOG_ERROR("Tree name too long. name(%s)\n", obj_name);
        return FILE_INDEX_TREE_NAME_TOO_LONG;
    }

    tmp_obj = avl_find(&parent_obj->child_obj_list, (int (*)(const void*, void *))compare_object2, obj_name,
        &where);
    if (NULL != tmp_obj)
    {
        *obj = tmp_obj;
        LOG_WARN("The obj is opened already. obj(%p) obj_ref_cnt(%d) name(%s)\n",
            tmp_obj, tmp_obj->obj_ref_cnt, obj_name);
        return -INDEX_ERR_IS_OPENED;
    }

    LOG_INFO("Create the obj. obj(%p) obj_name(%s)\n", tmp_obj, obj_name);

    ret = search_key_internal(parent_obj->mattr, obj_name, name_size);
    if (0 <= ret)
    {
        LOG_ERROR("The obj already exist. obj(%p) name(%s) ret(%d)\n", obj, obj_name, ret);
        return -FILE_INDEX_TREE_EXIST;
    }

    if (-INDEX_ERR_KEY_NOT_FOUND != ret)
    {
        LOG_ERROR("Search key failed. name(%s) ret(%d)\n", obj_name, ret);
        return ret;
    }
    
    ret = create_object(parent_obj->index, parent_obj, obj_name, mode, &tmp_obj);
    if (ret < 0)
    {
        LOG_ERROR("Create obj failed. name(%s) ret(%d)\n", obj_name, ret);
        return ret;
    }

    ret = index_insert_key_nolock(parent_obj->mattr, obj_name, name_size,
        &tmp_obj->inode.inode_no, VBN_SIZE);
    if (ret < 0)
    {
        LOG_ERROR("Insert obj failed. obj(%p) name(%s) ret(%d)\n",
            obj, obj_name, ret);
        (void)INDEX_FREE_BLOCK(tmp_obj, tmp_obj->inode.inode_no);
        close_object(tmp_obj);
        return ret;
    }
    
    LOG_INFO("Create the obj success. parent_obj(%p) obj_name(%s) obj(%p)\n",
        parent_obj, obj_name, tmp_obj);

    *obj = tmp_obj;
    
    return 0;
}    

int32_t index_create_object(OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint64_t mode, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;
    uint16_t name_size = 0;

    if ((NULL == parent_obj) || (NULL == obj_name) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p) obj(%p)\n",
            parent_obj, obj_name, obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    ret = index_create_object_nolock(parent_obj, obj_name, mode, obj);
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
    
    //IndexReclaimTreeHandles(obj); /* ��鲢����������� */
    
    return 0;
}    

int32_t index_open_object_nolock(OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint32_t open_flags, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;
    OBJECT_HANDLE *tmp_obj = NULL;
    uint64_t inode_no = 0;
    uint16_t name_size = 0;
    avl_index_t where = 0;

    if ((NULL == parent_obj) || (NULL == obj_name) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p) obj(%p)\n",
            parent_obj, obj_name, obj);
        return -INDEX_ERR_PARAMETER;
    }

    name_size = (uint16_t)strlen(obj_name);
    if (name_size >= OBJ_NAME_SIZE)
    {
        LOG_ERROR("Obj name too long. name(%s)", obj_name);
        return -FILE_INDEX_TREE_NAME_TOO_LONG;
    }

    LOG_INFO("Open the obj. parent_obj(%p) obj_name(%s)\n", parent_obj, obj_name);

    tmp_obj = avl_find(&parent_obj->child_obj_list, (int (*)(const void*, void *))compare_object2, obj_name,
        &where);
    if (NULL != tmp_obj)
    {
        if (0 != (open_flags & DELETE_FLAG))
        {
			if (0 != tmp_obj->obj_ref_cnt)
			{
				*obj = tmp_obj;
				LOG_WARN("The obj is being deleted. obj(%p) obj_ref_cnt(%d) name(%s)\n",
					tmp_obj, tmp_obj->obj_ref_cnt, obj_name);
				return -INDEX_ERR_IS_OPENED;
			}

            avl_remove(&parent_obj->child_obj_list, tmp_obj);
        }
        
        tmp_obj->obj_ref_cnt++;
        *obj = tmp_obj;
        LOG_WARN("The obj obj_ref_cnt inc. obj(%p) obj_ref_cnt(%d) name(%s)\n",
            tmp_obj, tmp_obj->obj_ref_cnt, obj_name);
        return 0;
    }
    
    ret = search_key_internal(parent_obj->mattr, obj_name, name_size);
    if (0 > ret)
    {
        LOG_DEBUG("Search for obj failed. obj(%p) name(%s) ret(%d)\n", obj, obj_name, ret);
        return ret;
    }

    if (parent_obj->mattr->ie->value_len != VBN_SIZE)
    {
        LOG_ERROR("The attr chaos. obj(%p) name(%s) value_len(%d)\n", obj, obj_name, parent_obj->mattr->ie->value_len);
        return -INDEX_ERR_CHAOS;
    }

    memcpy(&inode_no, IEGetValue(parent_obj->mattr->ie), VBN_SIZE);
    
    ret = open_object(parent_obj->index, parent_obj, obj_name, inode_no, &tmp_obj);
    if (0 > ret)
    {
        LOG_ERROR("Open obj failed. name(%s) ret(%d)\n", obj_name, ret);
        return ret;
    }

    if (0 != (open_flags & DELETE_FLAG))
    {
        avl_remove(&parent_obj->child_obj_list, tmp_obj);
    }

    if (memcmp(tmp_obj->inode.name, obj_name, name_size) != 0)
    {
        memcpy(tmp_obj->inode.name, obj_name, name_size);
        tmp_obj->inode.name[name_size] = 0;
        INODE_SET_DIRTY(tmp_obj);
    }

    LOG_INFO("Open the obj success. parent_obj(%p) obj_name(%s) obj(%p)\n",
        parent_obj, obj_name, tmp_obj);

    *obj = tmp_obj;

    return 0;
}      

int32_t index_open_object(OBJECT_HANDLE *parent_obj,
    const char *obj_name, OBJECT_HANDLE **obj)
{
    int32_t ret = 0;

    if ((NULL == parent_obj) || (NULL == obj_name) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p) obj(%p)\n",
            parent_obj, obj_name, obj);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    ret = index_open_object_nolock(parent_obj, obj_name, 0, obj);
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);

    return ret;
}      

int32_t recover_attr_record(ATTR_INFO *attr_info, void *para)
{
    memcpy(&attr_info->attr_record, &attr_info->old_attr_record,
        sizeof(ATTR_RECORD));
    ATTR_INFO_CLR_DIRTY(attr_info);

    return 0;
}

void index_cancel_object_modification(OBJECT_HANDLE *obj)
{
    index_cancel_all_caches_in_obj(obj);

    /* �ָ��������Լ�¼���޸�֮ǰ��״̬ */
    avl_walk_all(&obj->attr_info_list, (avl_walk_call_back)recover_attr_record, NULL);

    /* �ָ�inode��Ϣ */
    memcpy(&obj->inode, &obj->old_inode, sizeof(INODE_RECORD));
    INODE_CLR_DIRTY(obj);

    index_release_all_old_blocks_mem_in_obj(obj);

    index_release_all_free_caches_in_obj(obj);
    
    return;
}

void backup_attr_record(void *para, ATTR_INFO *attr_info)
{
    /* �������Լ�¼ */
    memcpy(&attr_info->old_attr_record, &attr_info->attr_record,
        sizeof(ATTR_RECORD));

    return;
}

int32_t index_commit_object_modification(OBJECT_HANDLE *obj)
{
    int32_t ret = 0;
    
    ASSERT(obj != NULL);

    /* ʹ���е������޸���Ч */
    validate_all_attr(obj);

    /* ��cache�е����������� */
    ret = index_flush_all_caches_in_obj(obj);
    if (0 > ret)
    {
        LOG_ERROR("Flush index blocks failed. obj_name(%s) ret(%d)\n", obj->obj_name, ret);
        return ret;
    }

    /* ��inode�е����������̣��˴��ɹ������Ϊ�����ύ�ɹ� */
    ret = flush_inode(obj);
    if (0 > ret)
    {
        LOG_ERROR("Flush inode failed. obj_name(%s) ret(%d)\n", obj->obj_name, ret);
        return ret;
    }

    avl_walk_all(&obj->attr_info_list, (avl_walk_call_back)backup_attr_record, NULL);

    /* ɾ���޸Ĺ��Ŀ��Ӧ�ľɿ飬��Ϊǰ���Ѿ��ɹ��ˣ����������Ƿ�ɹ�û��ϵ */
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
    OBJECT_HANDLE *parent_obj = NULL;

    if (NULL == obj)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    parent_obj = obj->parent_obj;
    if (NULL == parent_obj)
    {
        return index_close_object_nolock(obj);  // bug here, should lock
    }
    
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    ret = index_close_object_nolock(obj);
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
    
    return ret;
}     

int32_t index_remove_object(OBJECT_HANDLE *parent_obj, const char *obj_name,
    void *hnd, DeleteFunc del_func)
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

    /* ��Ŀ¼������ֹ���ٴα��� */
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    
    /* ������Ƿ��Ѿ����� */
    obj = avl_find(&parent_obj->child_obj_list, (int (*)(const void*, void *))compare_object2, obj_name,
        &where);
    if ((NULL != obj) && (0 != obj->obj_ref_cnt))
    {
        LOG_WARN("The obj is still opened. obj(%p) obj_ref_cnt(%d) name(%s)\n",
            obj, obj->obj_ref_cnt, obj_name);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return -INDEX_ERR_IS_OPENED;
    }

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
    
    ret = index_remove_key_nolock(&parent_obj->mattr->tree, obj_name, (uint16_t)strlen(obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Remove the obj failed. name(%s)\n", obj_name);
        (void)index_close_object_nolock(obj);
        (void)index_close_object_nolock(obj); /* ��2�Σ��Ա��ͷ���Դ */
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
        (void)index_close_object_nolock(obj); /* ��2�Σ��Ա��ͷ���Դ */
        return ret;
    }
    (void)index_close_object_nolock(obj);
    (void)index_close_object_nolock(obj); /* ��2�Σ��Ա��ͷ���Դ */


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
    
    /* ��Ŀ¼������ֹ���ٴα��� */
    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    
    /* ������Ƿ��Ѿ����� */
    obj = avl_find(&parent_obj->child_obj_list, (int (*)(const void*, void *))compare_object2, obj_name,
        &where);
    if (NULL != obj)
    {
        if (0 != obj->obj_ref_cnt)
        {
            LOG_WARN("The obj is still opened. obj(%p) obj_ref_cnt(%d) name(%s)\n",
                obj, obj->obj_ref_cnt, old_obj_name);
            OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
            return -INDEX_ERR_IS_OPENED;
        }
        
        close_object(obj);
    }

    ret = search_key_internal(&parent_obj->mattr->tree, old_obj_name,
        (uint16_t)strlen(old_obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Search for obj failed. parent_obj(%p) name(%s) ret(%d)\n",
            parent_obj, old_obj_name, ret);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    if (parent_obj->mattr->ie->value_len != VBN_SIZE)
    {
        LOG_DEBUG("The attr chaos."
            " [obj: %p, name: %s, value_len: %d]\n", obj, obj_name, parent_obj->mattr->ie->value_len);
        return -INDEX_ERR_CHAOS;
    }

    memcpy(&inode_no, IEGetValue(parent_obj->mattr->ie), VBN_SIZE);

    (void)INDEX_StartTreeTransNoLock(&parent_obj->mattr->tree);

    ret = index_remove_key_nolock(&parent_obj->mattr->tree, old_obj_name,
        (uint16_t)strlen(old_obj_name));
    if (0 > ret)
    {
        LOG_ERROR("Remove obj failed. name(%s) ret(%d)\n", old_obj_name, ret);
        (void)INDEX_CancelTreeTransNoLock(&parent_obj->mattr->tree);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    ret = index_insert_key_nolock(&parent_obj->mattr->tree, new_obj_name,
        (uint16_t)strlen(new_obj_name), &inode_no, VBN_SIZE);
    if (0 > ret)
    {
        LOG_ERROR("Insert obj failed. name(%s) ret(%d)\n", new_obj_name, ret);
        (void)INDEX_CancelTreeTransNoLock(&parent_obj->mattr->tree);
        OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);
        return ret;
    }

    ret = INDEX_CommitTreeTransNoLock(&parent_obj->mattr->tree, COMMIT_FLAG_FORCE);
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);

    LOG_INFO("Rename the obj finished. parent_obj(%p) old_obj_name(%s) new_obj_name(%s) ret(%d)\n",
        parent_obj, old_obj_name, new_obj_name, ret);
    
    return ret;
    #endif
    return 0;
}


int32_t walk_all_opened_child_objects(OBJECT_HANDLE *object,
    int32_t (*func)(void *, OBJECT_HANDLE *), void *para)
{
    int32_t ret = 0;
    
    if (NULL == object)
    {
        LOG_ERROR("Invalid parameter. object(%p)\n", object);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&object->obj_lock);
    ret = avl_walk_all(&object->child_obj_list, (avl_walk_call_back)func, para);
    OS_RWLOCK_WRUNLOCK(&object->obj_lock);

    return ret;
}

OBJECT_HANDLE *find_child_object_handle(OBJECT_HANDLE *parent_obj,
    const char * obj_name)
{
    OBJECT_HANDLE *obj = NULL;
    avl_index_t where = 0;
    
    if ((NULL == parent_obj) || (NULL == obj_name))
    {
        LOG_ERROR("Invalid parameter. parent_obj(%p) obj_name(%p)\n",
            parent_obj, obj_name);
        return NULL;
    }

    OS_RWLOCK_WRLOCK(&parent_obj->obj_lock);
    obj = avl_find(&parent_obj->child_obj_list, (int (*)(const void*, void *))compare_object2, obj_name,
        &where);

    /*
    * �����������Ҫ���ⲿʹ��(��ʵ�ǿ����õ�)
    */
    if ((NULL != obj) && (0 == obj->obj_ref_cnt))
    {
        obj = NULL;
    }
    
    OS_RWLOCK_WRUNLOCK(&parent_obj->obj_lock);

    return obj;
}

