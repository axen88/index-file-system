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
#include "ofs_if.h"

MODULE(PID_OBJECT);
#include "log.h"

int32_t compare_object2(const void *objid, object_info_t *obj_info)
{
    if ((*(u64_t *)objid) > obj_info->objid)
    {
        return 1;
    }
    else if ((*(u64_t *)objid) == obj_info->objid)
    {
        return 0;
    }

    return -1;
}

int32_t compare_cache1(const ofs_block_cache_t *cache, const ofs_block_cache_t *cache_node)
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

void init_attr(object_info_t *obj_info, u64_t inode_no)
{
    obj_info->attr_record = INODE_GET_ATTR_RECORD(obj_info->inode);
    obj_info->root_cache.vbn = inode_no;
    obj_info->root_cache.ib = (block_head_t *)obj_info->attr_record->content;
}

int32_t get_object_info(container_handle_t *ct, u64_t objid, object_info_t **obj_info_out)
{
    object_info_t *obj_info = NULL;
    
    obj_info = (object_info_t *)OS_MALLOC(sizeof(object_info_t));
    if (obj_info == NULL)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(object_info_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(obj_info, 0, sizeof(object_info_t));
    obj_info->ref_cnt = 0;
    obj_info->ct = ct;
    obj_info->objid = objid;
    
    list_init_head(&obj_info->obj_hnd_list);
    OS_RWLOCK_INIT(&obj_info->obj_hnd_lock);
    
    OS_RWLOCK_INIT(&obj_info->attr_lock);
    
    avl_create(&obj_info->caches, (int (*)(const void *, const void*))compare_cache1, sizeof(ofs_block_cache_t),
        OS_OFFSET(ofs_block_cache_t, obj_entry));
    OS_RWLOCK_INIT(&obj_info->caches_lock);
    
    OS_RWLOCK_INIT(&obj_info->obj_lock);

    avl_add(&ct->obj_info_list, obj_info);

    *obj_info_out = obj_info;
    
    LOG_INFO("init object info finished. objid(%lld)\n", obj_info->objid);

    return 0;
}

void put_object_info(object_info_t *obj_info)
{
    LOG_INFO("destroy object info start. objid(%lld)\n", obj_info->objid);

    avl_remove(&obj_info->ct->obj_info_list, obj_info);

    release_obj_all_cache(obj_info);
    avl_destroy(&obj_info->caches);
    
    OS_RWLOCK_DESTROY(&obj_info->caches_lock);
    OS_RWLOCK_DESTROY(&obj_info->attr_lock);
    OS_RWLOCK_DESTROY(&obj_info->obj_hnd_lock);
    OS_RWLOCK_DESTROY(&obj_info->obj_lock);

    OS_FREE(obj_info);
}

int32_t get_object_handle(object_info_t *obj_info, object_handle_t **obj_out)
{
    int32_t ret = 0;
    object_handle_t *obj = NULL;

    obj = (object_handle_t *)OS_MALLOC(sizeof(object_handle_t));
    if (obj == NULL)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(object_handle_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(obj, 0, sizeof(object_handle_t));
    obj->obj_info = obj_info;
    obj->ct = obj_info->ct;

    list_add_tail(&obj_info->obj_hnd_list, &obj->entry);
    obj_info->ref_cnt++;
    
    *obj_out = obj;

    return 0;
}

void put_object_handle(object_handle_t *obj)
{
    obj->obj_info->ref_cnt--;
    list_del(&obj->entry);
    OS_FREE(obj);

    return;
}

int32_t recover_obj_inode(object_info_t *obj_info, u64_t inode_no)
{
    int32_t ret;
    ofs_block_cache_t *cache;
    
    ret = index_block_read2(obj_info, inode_no, INODE_MAGIC, &cache);
    if (ret < 0)
    {
        LOG_ERROR("Read inode failed. ret(%d)\n", ret);
        return ret;
    }

    obj_info->inode_cache = cache;
	obj_info->inode = (inode_record_t *)cache->ib;
    obj_info->inode_no = inode_no;
    strncpy(obj_info->name, obj_info->inode->name, obj_info->inode->name_size);
    init_attr(obj_info, inode_no);
    SET_INODE_CLEAN(obj_info);

    return 0;
}

void validate_obj_inode(object_info_t *obj_info)
{
    u64_t new_vbn;
    ofs_super_block_t *sb;
    
    ASSERT(obj_info != NULL);

    if (!CACHE_DIRTY(&obj_info->root_cache))
    {
        return;
    }

    SET_INODE_DIRTY(obj_info);
    
    new_vbn = obj_info->root_cache.vbn;
    obj_info->inode_no = new_vbn;
    sb = &obj_info->ct->sb;

    switch (obj_info->objid)
    {
        case BASE_OBJ_ID:
        {
            sb->base_inode_no = obj_info->inode_no;
            obj_info->ct->flags |= FLAG_DIRTY;
            break;
        }
            
        case SPACE_OBJ_ID:
        {
            sb->space_inode_no = obj_info->inode_no;
            obj_info->ct->flags |= FLAG_DIRTY;
            break;
        }
            
        case OBJID_OBJ_ID:
        {
            sb->objid_inode_no = obj_info->inode_no;
            obj_info->ct->flags |= FLAG_DIRTY;
            break;
        }
            
        default:
        {
            uint8_t key_str[U64_MAX_SIZE];
            uint8_t value_str[U64_MAX_SIZE];
            uint16_t key_size;
            uint16_t value_size;
            
            key_size = os_u64_to_bstr(obj_info->objid, key_str);
            value_size = os_u64_to_bstr(new_vbn, value_str);
            
            index_update_value(obj_info->ct->id_obj, key_str, key_size, value_str, value_size);
            break;
        }
    }
}


void put_all_object_handle(object_info_t *obj_info)
{
    object_handle_t *obj;
    
    while (obj_info->ref_cnt != 0)
    {
        obj = OS_CONTAINER(obj_info->obj_hnd_list.next, object_handle_t, entry);
        put_object_handle(obj);
    }
}

void close_object(object_info_t *obj_info)
{
    put_all_object_handle(obj_info);
    validate_obj_inode(obj_info);
    put_object_info(obj_info);
}

void init_inode(inode_record_t *inode, u64_t objid, u64_t inode_no, uint16_t flags)
{
    attr_record_t *attr_record = NULL;

	memset(inode, 0, sizeof(inode_record_t));

    inode->head.blk_id = INODE_MAGIC;
    inode->head.alloc_size = INODE_SIZE;
    inode->head.real_size = INODE_SIZE;
    
    inode->first_attr_off = OS_OFFSET(inode_record_t, reserved);
    
    inode->objid = objid;
    inode->base_objid = 0;
    
    inode->mode = 0;
    inode->uid = 0;
    inode->gid = 0;
    inode->size = 0;
    inode->links = 0;
    inode->ctime = 0;
    inode->atime = 0;
    inode->mtime = 0;
    
    snprintf(inode->name, OBJ_NAME_MAX_SIZE, "OBJ%llu", (unsigned long long)objid);
    inode->name_size = strlen(inode->name);

    /* init attr */
    attr_record = INODE_GET_ATTR_RECORD(inode);
    attr_record->record_size = ATTR_RECORD_SIZE;
    attr_record->flags = flags;
    if (flags & FLAG_TABLE)
    { /* table */
        init_ib((index_block_t *)&attr_record->content, INDEX_BLOCK_SMALL, ATTR_RECORD_CONTENT_SIZE);
    }
    else
    { /* data stream */
        memset(attr_record->content, 0, ATTR_RECORD_CONTENT_SIZE);
    }
}

int32_t create_object_at_inode(container_handle_t *ct, u64_t objid, u64_t inode_no, uint16_t flags, object_handle_t **obj_out)
{
    int32_t ret;
    object_handle_t *obj;
    object_info_t *obj_info;
    ofs_block_cache_t *cache;

    ASSERT(ct != NULL);
    ASSERT(obj_out != NULL);
    ASSERT(sizeof(inode_record_t) == INODE_SIZE);

    ret = get_object_info(ct, objid, &obj_info);
    if (ret < 0)
    {
        LOG_ERROR("get_object_info failed. ret(%d)\n", ret);
        return ret;
    }

    cache = alloc_obj_cache(obj_info, inode_no, INODE_MAGIC);
    if (cache == NULL)
    {
        put_object_info(obj_info);
        LOG_ERROR("alloc_obj_cache failed\n");
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

	obj_info->inode_cache = cache;
	obj_info->inode = (inode_record_t *)cache->ib;
    obj_info->inode_no = inode_no;

    init_inode(obj_info->inode, objid, inode_no, flags);
    strncpy(obj_info->name, obj_info->inode->name, obj_info->inode->name_size);
    init_attr(obj_info, inode_no);
    SET_INODE_DIRTY(obj_info);
    SET_CACHE_DIRTY(&obj_info->root_cache);

    LOG_DEBUG("Create inode success. obj_id(%lld) vbn(%lld)\n", objid, inode_no);

    ret = get_object_handle(obj_info, &obj);
    if (ret < 0)
    {
        put_object_info(obj_info);
        LOG_ERROR("Open attr failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    *obj_out = obj;

    return 0;
}

int32_t create_object(container_handle_t *ct, u64_t objid, uint16_t flags, object_handle_t **obj_out)
{
     int32_t ret;
     u64_t inode_no = 0;

    ASSERT(ct != NULL);
    ASSERT(obj_out != NULL);
    ASSERT(INODE_SIZE == sizeof(inode_record_t));

    /* allocate inode block */
    ret = OFS_ALLOC_BLOCK(ct, objid, &inode_no);
    if (ret < 0)
    {
        LOG_ERROR("Allocate block failed. ret(%d)\n", ret);
        return ret;
    }

    ret = create_object_at_inode(ct, objid, inode_no, flags, obj_out);
    if (ret < 0)
    {
        OFS_FREE_BLOCK(ct, objid, inode_no);
        LOG_ERROR("get_object_info failed. ret(%d)\n", ret);
        return ret;
    }

    return 0;
}

int32_t open_object(container_handle_t *ct, u64_t objid, u64_t inode_no, object_handle_t **obj_out)
{
    int32_t ret = 0;
    object_info_t *obj_info = NULL;
    object_handle_t *obj = NULL;

    ASSERT(ct != NULL);
    ASSERT(obj_out != NULL);

    ret = get_object_info(ct, objid, &obj_info);
    if (ret < 0)
    {
        LOG_ERROR("Get object info resource failed. objid(%lld)\n", objid);
        return ret;
    }

    ret = recover_obj_inode(obj_info, inode_no);
    if (ret < 0)
    {
        put_object_info(obj_info);
        LOG_ERROR("Read inode failed. ret(%d)\n", ret);
        return ret;
    }

    ret = get_object_handle(obj_info, &obj);
    if (ret < 0)
    {
        put_object_info(obj_info);
        LOG_ERROR("Open attr failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    *obj_out = obj;

    return 0;
}

u64_t get_objid(container_handle_t *ct)
{
    return 1;
}

int32_t ofs_set_object_name(object_handle_t *obj, char *name)
{
    uint32_t name_size;
    
    ASSERT(obj != NULL);
    ASSERT(name != NULL);

    name_size = strlen(name);
    if (name_size >= OBJ_NAME_MAX_SIZE)
    {
        return -INDEX_ERR_PARAMETER;
    }

    strncpy(obj->obj_info->name, name, name_size);
    strncpy(obj->obj_info->inode->name, name, name_size);
    obj->obj_info->inode->name_size = name_size;

    SET_INODE_DIRTY(obj->obj_info);

    return 0;
}

int32_t ofs_create_object_nolock(container_handle_t *ct, u64_t objid, uint16_t flags, object_handle_t **obj_out)
{
    int32_t ret = 0;
    object_handle_t *obj = NULL;
    uint16_t name_size = 0;
    avl_index_t where = 0;
    object_info_t *obj_info;

    ASSERT(ct != NULL);
    ASSERT(!OBJID_IS_INVALID(objid));
    ASSERT(obj_out != NULL);

    if (objid < RESERVED_OBJ_ID)
    {
        LOG_INFO("objid(%lld) should be larger than %d\n", objid, RESERVED_OBJ_ID);
        return -INDEX_ERR_OBJ_ID_INVALID;
    }

    LOG_INFO("Create the obj start. objid(%lld)\n", objid);

    obj_info = avl_find(&ct->obj_info_list, (avl_find_fn_t)compare_object2, &objid, &where);
    if (obj_info)
    {
        LOG_ERROR("The obj already exist. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return -INDEX_ERR_OBJ_EXIST;
    }

    ret = search_key_internal(ct->id_obj, &objid, sizeof(u64_t), NULL, 0);
    if (ret >= 0)
    {
        LOG_ERROR("The obj already exist. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return -INDEX_ERR_OBJ_EXIST;
    }

    if (ret != -INDEX_ERR_KEY_NOT_FOUND)
    {
        LOG_ERROR("Search key failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = create_object(ct, objid, flags, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    ret = index_insert_key_nolock(ct->id_obj, &objid, os_u64_size(objid),
        &obj->obj_info->inode_no, os_u64_size(obj->obj_info->inode_no));
    if (ret < 0)
    {
        LOG_ERROR("Insert obj failed. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        (void)OFS_FREE_BLOCK(obj->ct, obj->obj_info->objid, obj->obj_info->inode_no);
        close_object(obj->obj_info);
        return ret;
    }
    
    LOG_INFO("Create the obj success. objid(%lld) obj(%p) ct_name(%s)\n", objid, obj, ct->name);

    *obj_out = obj;
    
    return 0;
}    

int32_t ofs_create_object(container_handle_t *ct, u64_t objid, uint16_t flags, object_handle_t **obj)
{
    int32_t ret = 0;

    if ((ct == NULL) || (OBJID_IS_INVALID(objid || (obj == NULL))))
    {
        LOG_ERROR("Invalid parameter. ct(%p) objid(%lld) obj(%p)\n", ct, objid, obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    OS_RWLOCK_WRLOCK(&ct->ct_lock);
    ret = ofs_create_object_nolock(ct, objid, flags, obj);
    OS_RWLOCK_WRUNLOCK(&ct->ct_lock);
    
    return ret;
}    

int32_t ofs_open_object_nolock(container_handle_t *ct, u64_t objid, uint32_t open_flags, object_handle_t **obj_out)
{
    int32_t ret = 0;
    object_handle_t *obj = NULL;
    u64_t inode_no = 0;
    avl_index_t where = 0;
    object_handle_t *id_obj;
    object_info_t *obj_info = NULL;

    ASSERT(ct != NULL);
    ASSERT(obj_out != NULL);

    LOG_INFO("Open the obj. objid(%lld)\n", objid);

    obj_info = avl_find(&ct->obj_info_list, (avl_find_fn_t)compare_object2, &objid, &where);
    if (obj_info)
    {
        ret = get_object_handle(obj_info, &obj);
        if (ret < 0)
        {
            LOG_ERROR("get_object_handle failed. objid(%lld) ret(%d)\n", objid, ret);
            return ret;
        }

        *obj_out = obj;
        LOG_WARN("The obj ref_cnt inc. obj(%p) ref_cnt(%d) objid(%lld)\n", obj_info, obj_info->ref_cnt, objid);
        return 0;
    }

    id_obj = ct->id_obj;
    
    ret = search_key_internal(id_obj, &objid, sizeof(u64_t), NULL, 0);
    if (ret < 0)
    {
        LOG_DEBUG("Search for obj failed. obj(%p) objid(%lld) ret(%d)\n", obj, objid, ret);
        return ret;
    }
    
    inode_no = os_bstr_to_u64(GET_IE_VALUE(id_obj->ie), id_obj->ie->value_len);

    ret = open_object(ct, objid, inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open obj failed. objid(%lld) ret(%d)\n", objid, ret);
        return ret;
    }

    LOG_INFO("Open the obj success. ct(%p) objid(%lld) obj(%p)\n", ct, objid, obj);

    *obj_out = obj;

    return 0;
}      

int32_t ofs_open_object(container_handle_t *ct, u64_t objid, object_handle_t **obj)
{
    int32_t ret = 0;

    if ((ct == NULL) || (obj == NULL))
    {
        LOG_ERROR("Invalid parameter. ct(%p) obj(%p)\n", ct, obj);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&ct->ct_lock);
    ret = ofs_open_object_nolock(ct, objid, 0, obj);
    OS_RWLOCK_WRUNLOCK(&ct->ct_lock);

    return ret;
}      

object_info_t *ofs_get_object_info(container_handle_t *ct, u64_t objid)
{
    object_info_t *obj_info = NULL;
    avl_index_t where = 0;

    ASSERT(ct != NULL);
    ASSERT(!OBJID_IS_INVALID(objid));

    OS_RWLOCK_RDLOCK(&ct->ct_lock);
    obj_info = avl_find(&ct->obj_info_list, (avl_find_fn_t)compare_object2, &objid, &where);
    OS_RWLOCK_RDLOCK(&ct->ct_lock);

    return obj_info;
}

object_handle_t *ofs_get_object_handle(container_handle_t *ct, u64_t objid)
{
    object_info_t *obj_info = NULL;

    ASSERT(ct != NULL);
    ASSERT(!OBJID_IS_INVALID(objid));

    obj_info = ofs_get_object_info(ct, objid);
    if (obj_info == NULL)
    {
        return NULL;
    }

    if (obj_info->ref_cnt == 0)
    {
        return NULL;
    }

    return OS_CONTAINER(obj_info->obj_hnd_list.next, object_handle_t, entry);
}

int32_t ofs_close_object_nolock(object_handle_t *obj)
{
    object_info_t *obj_info = NULL;
    
    if (obj == NULL)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }
    
    obj_info = obj->obj_info;
    if (obj_info == NULL)
    {
        LOG_ERROR("Invalid object info. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&obj_info->obj_hnd_lock);
    
    if (obj_info->ref_cnt == 0)
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->obj_hnd_lock);
        LOG_EMERG("Too many times put object info. objid(%lld)\n", obj_info->objid);
        return -INDEX_ERR_MANY_TIMES_PUT;
    }

    put_object_handle(obj);

    if (obj_info->ref_cnt == 0) // decrease to 0
    {
        close_object(obj_info);
    }
    else
    {
        OS_RWLOCK_WRUNLOCK(&obj_info->obj_hnd_lock);
    }
    
	return 0;
}     

int32_t ofs_close_object(object_handle_t *obj)
{
    int32_t ret = 0;
	container_handle_t *ct;

    if (obj == NULL)
    {
        LOG_ERROR("Invalid parameter. obj(%p)\n", obj);
        return -INDEX_ERR_PARAMETER;
    }

	ct = obj->ct;
    
    OS_RWLOCK_WRLOCK(&ct->ct_lock);
    ret = ofs_close_object_nolock(obj);
    OS_RWLOCK_WRUNLOCK(&ct->ct_lock);
    
    return ret;
}     

int32_t ofs_delete_object(container_handle_t *ct, u64_t objid)
{
    return 0;
}

