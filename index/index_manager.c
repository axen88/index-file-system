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
File Name: INDEX_MANAGER.C
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

avl_tree_t *g_index_list = NULL;
OS_RWLOCK g_index_list_rwlock;

void close_index(INDEX_HANDLE *index);
extern int32_t fixup_index(INDEX_HANDLE *index);

int32_t compare_index1(const INDEX_HANDLE *index, const INDEX_HANDLE *index_node)
{
    return os_collate_ansi_string(index->name, strlen(index->name),
        index_node->name, strlen(index_node->name));
}

int32_t compare_index2(const char *index_name, INDEX_HANDLE *index_node)
{
    return os_collate_ansi_string(index_name, strlen(index_name),
        index_node->name, strlen(index_node->name));
}


int32_t compare_object1(const OBJECT_INFO *obj_info, const OBJECT_INFO *target_obj_info)
{
    if (obj_info->objid > target_obj_info->objid)
    {
        return 1;
    }
    else if (obj_info->objid == target_obj_info->objid)
    {
        return 0;
    }

    return -1;
}

int32_t index_init_system(void)
{
    if (NULL != g_index_list)
    {
        LOG_ERROR("Init index system many times. g_index_list(%p)\n", g_index_list);
        return 0;
    }

    g_index_list = OS_MALLOC(sizeof(avl_tree_t));
    if (NULL == g_index_list)
    {
        LOG_ERROR("Malloc failed. size(%d)\n", (uint32_t)sizeof(avl_tree_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    avl_create(g_index_list, (int (*)(const void *, const void*))compare_index1, sizeof(INDEX_HANDLE),
        OS_OFFSET(INDEX_HANDLE, entry));
    OS_RWLOCK_INIT(&g_index_list_rwlock);

    return 0;
}

int32_t close_one_index(void *para, INDEX_HANDLE *index)
{
    ASSERT(NULL != index);

    close_index(index);

    return 0;
}

int32_t close_one_object(void *para, OBJECT_INFO *obj_info)
{
    ASSERT(NULL != obj_info);

    if (obj_info->objid < RESERVED_OBJ_ID)  // do not close the system object
    {
        return 0;
    }

    close_object(obj_info);
    return 0;
}

void index_exit_system(void)
{
    if (NULL == g_index_list)
    {
        LOG_ERROR("Exit index system many times. g_index_list(%p)\n", g_index_list);
        return;
    }

    (void)avl_walk_all(g_index_list, (avl_walk_call_back)close_one_index, NULL);

    OS_FREE(g_index_list);
    g_index_list = NULL;
    OS_RWLOCK_DESTROY(&g_index_list_rwlock);

	return;
}

int32_t walk_all_opened_index(
    int32_t (*func)(void *, INDEX_HANDLE *), void *para)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = avl_walk_all(g_index_list, (avl_walk_call_back)func, para);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return ret;
}

int32_t init_index_resource(INDEX_HANDLE ** index, const char * index_name)
{
    INDEX_HANDLE *tmp_index = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != index_name);

    tmp_index = (INDEX_HANDLE *) OS_MALLOC(sizeof(INDEX_HANDLE));
    if (NULL == tmp_index)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(INDEX_HANDLE));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_index, 0, sizeof(INDEX_HANDLE));
    strncpy(tmp_index->name, index_name, INDEX_NAME_SIZE);
    OS_RWLOCK_INIT(&tmp_index->index_lock);
    tmp_index->index_ref_cnt = 1;
    avl_create(&tmp_index->obj_list, (int (*)(const void *, const void*))compare_object1, sizeof(OBJECT_INFO),
        OS_OFFSET(OBJECT_INFO, entry));
    avl_add(g_index_list, tmp_index);

    *index = tmp_index;

    return 0;
}

int32_t create_system_objects(INDEX_HANDLE *index)
{
    int32_t ret;
    OBJECT_HANDLE *obj;
    
    /* create free blk object */
    ret = create_object_inode(index, SPACE_OBJ_ID, SPACE_OBJ_INODE, FLAG_SYSTEM | FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create free block object failed. name(%s)\n", index->name);
        return ret;
    }

    index->hnd->sb.first_free_block++;
    index->hnd->sb.free_blocks--;

    index_init_sm(&index->sm, obj, index->hnd->sb.first_free_block, index->hnd->sb.free_blocks, index->hnd->sb.total_blocks);
    ret = index_init_free_space(&index->sm, index->hnd->sb.first_free_block, index->hnd->sb.free_blocks);
    if (ret < 0)
    {
        LOG_ERROR("init free block space info failed. name(%s)\n", index->name);
        return ret;
    }

    index->hnd->sb.free_blk_inode_no = obj->obj_info->inode_no;
    index->hnd->sb.free_blk_id = obj->obj_info->inode.objid;
    
    /* create objid object */
    ret = create_object(index, OBJID_OBJ_ID, FLAG_SYSTEM | FLAG_TABLE | CR_U64 | (CR_U64 << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create objid object failed. name(%s)\n", index->name);
        return ret;
    }

    index->id_obj = obj;
    index->hnd->sb.objid_inode_no = obj->obj_info->inode_no;
    index->hnd->sb.objid_id = obj->obj_info->inode.objid;

    return 0;
}

int32_t open_system_objects(INDEX_HANDLE *index)
{
    int32_t ret;
    OBJECT_HANDLE *obj;
    
    /* open FREEBLK object */
    ret = open_object(index, index->hnd->sb.free_blk_id, index->hnd->sb.free_blk_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open free block object failed. index_name(%s) ret(%d)\n", index->name, ret);
        return ret;
    }
    
    index_init_sm(&index->sm, obj, index->hnd->sb.first_free_block, index->hnd->sb.free_blocks, index->hnd->sb.total_blocks);

    /* open $OBJID object */
    ret = open_object(index, index->hnd->sb.objid_id, index->hnd->sb.objid_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open objid object failed. index_name(%s) ret(%d)\n", index->name, ret);
        return ret;
    }

    index->id_obj = obj;

    return 0;
}

int32_t index_create_nolock(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    INDEX_HANDLE **index)
{
    INDEX_HANDLE *tmp_index = NULL;
    int32_t ret = 0;
    BLOCK_HANDLE_S *hnd = NULL;
    avl_index_t where = 0;

    if ((NULL == index) || (0 == total_sectors) || (NULL == index_name))
    {
        LOG_ERROR("Invalid parameter. index(%p) total_sectors(%lld) index_name(%p)\n",
            index, total_sectors, index_name);
        return -INDEX_ERR_PARAMETER;
    }
    
    if (strlen(index_name) >= INDEX_NAME_SIZE)
    {
        LOG_ERROR("file name size must < %d bytes.\n", INDEX_NAME_SIZE);
        return -INDEX_ERR_PARAMETER;
    }
    
    LOG_INFO("Create the index. index_name(%s) total_sectors(%lld) start_lba(%lld)\n",
        index_name, total_sectors, start_lba);

    /* already opened */
    tmp_index = avl_find(g_index_list, (avl_find_fn)compare_index2, index_name, &where);
    if (NULL != tmp_index)
    {
        *index = tmp_index;
        LOG_WARN("The index is opened already. index_name(%s) start_lba(%lld)\n",
            index_name, start_lba);
        return -INDEX_ERR_IS_OPENED;
    }

    /* allocate resource */
    ret = init_index_resource(&tmp_index, index_name);
    if (0 > ret)
    {
        LOG_ERROR("Init index resource failed. index_name(%s) ret(%d)", index_name, ret);
        return ret;
    }
    
    /* create block manager */
    ret = block_create(&hnd, index_name, total_sectors, BYTES_PER_BLOCK_SHIFT,
        0, start_lba);
    if (ret < 0)
    {
        LOG_ERROR("Create block file failed. name(%s)\n", index_name);
        close_index(tmp_index);
        return ret;
    }

    tmp_index->hnd = hnd;

    ret = create_system_objects(tmp_index);
    if (0 > ret)
    {
        LOG_ERROR("create system objects failed. hnd(%p) ret(%d)\n", tmp_index->hnd, ret);
        close_index(tmp_index);
        return ret;
    }

    
    ret = block_update_super_block(tmp_index->hnd);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. hnd(%p) ret(%d)\n", tmp_index->hnd, ret);
        close_index(tmp_index);
        return ret;
    }

    *index = tmp_index;

    LOG_INFO("Create the index success. index_name(%s) total_sectors(%lld) start_lba(%lld) index(%p)\n",
        index_name, total_sectors, start_lba, tmp_index);
    
    return 0;
}     

int32_t index_create(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    INDEX_HANDLE **index)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_create_nolock(index_name, total_sectors, start_lba, index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);

    return ret;
}     

int32_t index_open_nolock(const char *index_name, uint64_t start_lba, INDEX_HANDLE **index)
{
    INDEX_HANDLE *tmp_index = NULL;
    int32_t ret = 0;
    BLOCK_HANDLE_S *hnd = NULL;
    avl_index_t where = 0;

    if ((NULL == index) || (NULL == index_name))
    {
        LOG_ERROR("Invalid parameter. index(%p) index_name(%p)\n", index, index_name);
        return -INDEX_ERR_PARAMETER;
    }

    if (strlen(index_name) >= INDEX_NAME_SIZE)
    {
        LOG_ERROR("file name size must < %d bytes.\n", INDEX_NAME_SIZE);
        return -INDEX_ERR_PARAMETER;
    }

    LOG_INFO("Open the index. index_name(%s) start_lba(%lld)\n", index_name, start_lba);

    tmp_index = avl_find(g_index_list, (avl_find_fn)compare_index2, index_name, &where);
    if (NULL != tmp_index)
    {
        tmp_index->index_ref_cnt++;
        *index = tmp_index;
        LOG_WARN("File index_ref_cnt inc. index_name(%s) start_lba(%lld) index_ref_cnt(%d)\n",
            index_name, start_lba, tmp_index->index_ref_cnt);
        return 0;
    }

    ret = init_index_resource(&tmp_index, index_name);
    if (0 > ret)
    {
        LOG_ERROR("Init index resource failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = block_open(&hnd, index_name, start_lba);
    if (0 > ret)
    {
        LOG_ERROR("Open index file failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        close_index(tmp_index);
        return ret;
    }

    tmp_index->hnd = hnd;

    /* open system object */
    ret = open_system_objects(tmp_index);
    if (ret < 0)
    {
        LOG_ERROR("Open system object failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        close_index(tmp_index);
        return ret;
    }

    if (block_need_fixup(tmp_index->hnd))
    {
        ret = fixup_index(tmp_index);
        if (ret < 0)
        {
            LOG_ERROR("Fixup index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
                index_name, start_lba, ret);
            close_index(tmp_index);
            return ret;
        }
    }

    *index = tmp_index;
    
    LOG_INFO("Open the index success. index_name(%s) start_lba(%lld) index(%p)\n",
        index_name, start_lba, index);

    return 0;
}     

int32_t index_open(const char *index_name, uint64_t start_lba, INDEX_HANDLE **index)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_open_nolock(index_name, start_lba, index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return ret;
}     

void close_index(INDEX_HANDLE *index)
{
    ASSERT(NULL != index);

    // close all user object
    avl_walk_all(&index->obj_list, (avl_walk_call_back)close_one_object, NULL);

    // close system object
    if (index->id_obj != NULL)
    {
        (void)close_object(index->id_obj->obj_info);
    }

    if (index->sm.space_obj != NULL)
    {
        (void)close_object(index->sm.space_obj->obj_info);
    }

    // close block manager system
    if (NULL != index->hnd)
    {
        (void)block_close(index->hnd);
        index->hnd = NULL;
    }

    avl_destroy(&index->obj_list);
    OS_RWLOCK_DESTROY(&index->index_lock);
    avl_remove(g_index_list, index);

    OS_FREE(index);
    index = NULL;

    return;
}

int32_t index_close_nolock(INDEX_HANDLE *index)
{
    if (NULL == index)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. index(%p)\n", index);
        return -INDEX_ERR_PARAMETER;
    }

    LOG_INFO("Close the index. index(%p) index_ref_cnt(%d) name(%s)\n",
        index, index->index_ref_cnt, index->name);
    
    if (0 == index->index_ref_cnt)
    {
        LOG_ERROR("The index_ref_cnt is 0. index(%p) index_ref_cnt(%d) name(%s)\n",
            index, index->index_ref_cnt, index->name);
        
        return 0;
    }
    
    if (--index->index_ref_cnt)
    {
        LOG_WARN("The index index_ref_cnt dec. index(%p) index_ref_cnt(%d) name(%s)\n",
            index, index->index_ref_cnt, index->name);
        return 0;
    }
    
    close_index(index);

    LOG_INFO("Close the index success. index(%p)\n", index);

    return 0;
}     

int32_t index_close(INDEX_HANDLE *index)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_close_nolock(index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);

    return ret;
}     

INDEX_HANDLE *index_get_handle(const char * index_name)
{
    INDEX_HANDLE *index = NULL;
    avl_index_t where = 0;
 
    if (NULL == index_name)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. index_name(%p)\n", index_name);
        return NULL;
    }

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    index = avl_find(g_index_list, (avl_find_fn)compare_index2, index_name, &where);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return index;
}     

