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
#include "ofs_if.h"

MODULE(PID_CONTAINER);
#include "os_log.h"

avl_tree_t *g_container_list = NULL;
os_rwlock g_container_list_rwlock;

void close_container(container_handle_t *ct);

int32_t compare_container1(const container_handle_t *ct, const container_handle_t *ofs_node)
{
    return os_collate_ansi_string(ct->name, strlen(ct->name),
        ofs_node->name, strlen(ofs_node->name));
}

int32_t compare_container2(const char *ct_name, container_handle_t *ofs_node)
{
    return os_collate_ansi_string(ct_name, strlen(ct_name),
        ofs_node->name, strlen(ofs_node->name));
}


int32_t compare_object1(const object_info_t *obj_info, const object_info_t *target_obj_info)
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

int32_t ofs_init_system(void)
{
    if (NULL != g_container_list)
    {
        LOG_ERROR("Init ct system many times. g_container_list(%p)\n", g_container_list);
        return 0;
    }

    g_container_list = OS_MALLOC(sizeof(avl_tree_t));
    if (NULL == g_container_list)
    {
        LOG_ERROR("Malloc failed. size(%d)\n", (uint32_t)sizeof(avl_tree_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    avl_create(g_container_list, (int (*)(const void *, const void*))compare_container1, sizeof(container_handle_t),
        OS_OFFSET(container_handle_t, entry));
    OS_RWLOCK_INIT(&g_container_list_rwlock);

    return 0;
}

int32_t close_one_container(void *para, container_handle_t *ct)
{
    ASSERT(ct != NULL);

    close_container(ct);

    return 0;
}

int32_t close_one_object(void *para, object_info_t *obj_info)
{
    ASSERT(obj_info != NULL);

    if (obj_info->objid < RESERVED_OBJ_ID)  // do not close the system object
    {
        return 0;
    }

    close_object(obj_info);
    return 0;
}

void ofs_exit_system(void)
{
    if (NULL == g_container_list)
    {
        LOG_ERROR("Exit ct system many times. g_container_list(%p)\n", g_container_list);
        return;
    }

    (void)avl_walk_all(g_container_list, (avl_walk_cb_t)close_one_container, NULL);

    OS_FREE(g_container_list);
    g_container_list = NULL;
    OS_RWLOCK_DESTROY(&g_container_list_rwlock);

	return;
}

int32_t ofs_walk_all_opened_container(container_cb_t cb, void *para)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_container_list_rwlock);
    ret = avl_walk_all(g_container_list, (avl_walk_cb_t)cb, para);
    OS_RWLOCK_WRUNLOCK(&g_container_list_rwlock);
    
    return ret;
}

int32_t compare_cache1(const ofs_block_cache_t *cache, const ofs_block_cache_t *cache_node);


int32_t init_container_resource(container_handle_t **ct, const char *ct_name)
{
    container_handle_t *tmp_ct = NULL;

    ASSERT(ct != NULL);
    ASSERT(ct_name != NULL);

    tmp_ct = (container_handle_t *)OS_MALLOC(sizeof(container_handle_t));
    if (NULL == tmp_ct)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(container_handle_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_ct, 0, sizeof(container_handle_t));
    strncpy(tmp_ct->name, ct_name, OFS_NAME_SIZE);
    OS_RWLOCK_INIT(&tmp_ct->ct_lock);
    OS_RWLOCK_INIT(&tmp_ct->metadata_cache_lock);
    tmp_ct->ref_cnt = 1;
    avl_create(&tmp_ct->obj_info_list, (int (*)(const void *, const void*))compare_object1, sizeof(object_info_t),
        OS_OFFSET(object_info_t, entry));
    avl_create(&tmp_ct->metadata_cache, (int (*)(const void *, const void*))compare_cache1, sizeof(ofs_block_cache_t),
        OS_OFFSET(ofs_block_cache_t, fs_entry));
    avl_add(g_container_list, tmp_ct);

    *ct = tmp_ct;

    return 0;
}

void destroy_container_resource(container_handle_t *ct)
{
    avl_destroy(&ct->obj_info_list);
    avl_destroy(&ct->metadata_cache);
    OS_RWLOCK_DESTROY(&ct->ct_lock);
    OS_RWLOCK_DESTROY(&ct->metadata_cache_lock);
    avl_remove(g_container_list, ct);

    OS_FREE(ct);
}

int32_t create_system_objects(container_handle_t *ct)
{
    int32_t ret;
    object_handle_t *obj;

    ct->base_blk = 0;
    
    /* create base object */
    ret = create_object_at_inode(ct, BASE_OBJ_ID, BASE_OBJ_INODE, FLAG_SYSTEM | FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create base object failed. name(%s)\n", ct->name);
        return ret;
    }

    ofs_set_object_name(obj, BASE_OBJ_NAME);
    ct->sb.base_inode_no = obj->obj_info->inode_no;
    ct->sb.base_id = obj->obj_info->inode->objid;
    
    ofs_init_sm(&ct->bsm, obj, 0, 0);

    /* create space object */
    ret = create_object_at_inode(ct, SPACE_OBJ_ID, SPACE_OBJ_INODE, FLAG_SYSTEM | FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create free block object failed. name(%s)\n", ct->name);
        return ret;
    }
    
    ofs_set_object_name(obj, SPACE_OBJ_NAME);
    ct->sb.space_inode_no = obj->obj_info->inode_no;
    ct->sb.space_id = obj->obj_info->inode->objid;
    
    ct->sb.first_free_block += 2;
    ct->sb.free_blocks -= 2;

    ofs_init_sm(&ct->sm, obj, ct->sb.first_free_block, ct->sb.free_blocks);
    ret = ofs_init_free_space(&ct->sm, ct->sb.first_free_block, ct->sb.free_blocks);
    if (ret < 0)
    {
        LOG_ERROR("init free block space info failed. name(%s)\n", ct->name);
        return ret;
    }

    /* create objid object */
    ret = create_object(ct, OBJID_OBJ_ID, FLAG_SYSTEM | FLAG_TABLE | CR_U64 | (CR_U64 << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create objid object failed. name(%s)\n", ct->name);
        return ret;
    }

    ofs_set_object_name(obj, OBJID_OBJ_NAME);
    ct->sb.objid_inode_no = obj->obj_info->inode_no;
    ct->sb.objid_id = obj->obj_info->inode->objid;
    ct->id_obj = obj;

    ct->flags |= FLAG_DIRTY;
    
    return 0;
}

int32_t open_system_objects(container_handle_t *ct)
{
    int32_t ret;
    object_handle_t *obj;

    ct->base_blk = ct->sb.base_blk;
    
    /* open BASE object */
    ret = open_object(ct, ct->sb.base_id, ct->sb.base_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open base object failed. ct_name(%s) ret(%d)\n", ct->name, ret);
        return ret;
    }
    
    ofs_init_sm(&ct->bsm, obj, ct->sb.base_first_free_block, ct->sb.base_free_blocks);

    /* open SPACE object */
    ret = open_object(ct, ct->sb.space_id, ct->sb.space_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open space object failed. ct_name(%s) ret(%d)\n", ct->name, ret);
        return ret;
    }
    
    ofs_init_sm(&ct->sm, obj, ct->sb.first_free_block, ct->sb.free_blocks);

    /* open $OBJID object */
    ret = open_object(ct, ct->sb.objid_id, ct->sb.objid_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open objid object failed. ct_name(%s) ret(%d)\n", ct->name, ret);
        return ret;
    }

    ct->id_obj = obj;

    return 0;
}

int32_t init_super_block(ofs_super_block_t *sb, uint64_t total_sectors, uint32_t block_size_shift)
{
    int32_t ret = 0;
    uint64_t total_blocks = 0;

    if (block_size_shift != BYTES_PER_BLOCK_SHIFT)
    {
        LOG_ERROR("The parameter is invalid. block_size_shift(%d)\n", block_size_shift);
        return -BLOCK_ERR_PARAMETER;
    }

    total_blocks = total_sectors >> (block_size_shift - BYTES_PER_SECTOR_SHIFT);
    if (total_blocks < MIN_BLOCKS_NUM)
    {
        LOG_ERROR("The parameter is invalid. total_blocks(%lld)\n", total_blocks);
        return -BLOCK_ERR_PARAMETER;
    }

    sb->head.blk_id = SUPER_BLOCK_ID;
    sb->head.real_size = SUPER_BLOCK_SIZE;
    sb->head.alloc_size = SUPER_BLOCK_SIZE;
    sb->head.seq_no = 0;
    sb->block_size_shift = block_size_shift;
    sb->block_size = 1 << block_size_shift;
    sb->sectors_per_block = sb->block_size / BYTES_PER_SECTOR;
    sb->total_blocks = total_blocks;
    sb->free_blocks = total_blocks - 1;
    sb->first_free_block = 1;
    sb->version = VERSION;
    sb->flags = 0;
    sb->magic_num = BLOCK_MAGIC_NUMBER;

    return 0;
}

int32_t check_super_block(ofs_super_block_t *sb)
{
    ASSERT(sb != NULL);
    
    if (sb->magic_num != BLOCK_MAGIC_NUMBER)
    {
        LOG_ERROR( "magic_num not match. magic_num(%x) expect(%x)\n", sb->magic_num, BLOCK_MAGIC_NUMBER);
        return -BLOCK_ERR_FORMAT;
    }

    if (sb->version != VERSION)
    {
        LOG_ERROR("version not match. version(%04d) expect(%04d)\n", sb->version, VERSION);
        return -BLOCK_ERR_FORMAT;
    }

    if (sb->sectors_per_block != SECTORS_PER_BLOCK)
    {
        LOG_ERROR("sectors_per_block not match. sectors_per_block(%d) expect(%d)\n",
            sb->sectors_per_block, SECTORS_PER_BLOCK);
        return -BLOCK_ERR_FORMAT;
    }

    if (sb->block_size != BYTES_PER_BLOCK)
    {
        LOG_ERROR("block_size not match. block_size(%d) expect(%d)\n", sb->block_size, BYTES_PER_BLOCK);
        return -BLOCK_ERR_FORMAT;
    }

    return 0;
}

int32_t ofs_create_container_nolock(const char *ct_name, uint64_t total_sectors, container_handle_t **ct)
{
    container_handle_t *tmp_ct = NULL;
    int32_t ret = 0;
    avl_index_t where = 0;

    if ((ct == NULL) || (total_sectors == 0) || (ct_name == NULL))
    {
        LOG_ERROR("Invalid parameter. ct(%p) total_sectors(%lld) ct_name(%p)\n", ct, total_sectors, ct_name);
        return -INDEX_ERR_PARAMETER;
    }
    
    if (strlen(ct_name) >= OFS_NAME_SIZE)
    {
        LOG_ERROR("file name size must < %d bytes.\n", OFS_NAME_SIZE);
        return -INDEX_ERR_PARAMETER;
    }
    
    LOG_INFO("Create the ct. ct_name(%s) total_sectors(%lld)\n", ct_name, total_sectors);

    /* already opened */
    tmp_ct = avl_find(g_container_list, (avl_find_fn_t)compare_container2, ct_name, &where);
    if (NULL != tmp_ct)
    {
        *ct = tmp_ct;
        LOG_WARN("The ct is opened already. ct_name(%s) start_lba(%lld)\n", ct_name);
        return -INDEX_ERR_IS_OPENED;
    }

    /* allocate resource */
    ret = init_container_resource(&tmp_ct, ct_name);
    if (ret < 0)
    {
        LOG_ERROR("Init ct resource failed. ct_name(%s) ret(%d)", ct_name, ret);
        return ret;
    }
    
    /* init super block */
    ret = init_super_block(&tmp_ct->sb, total_sectors, BYTES_PER_BLOCK_SHIFT);
    if (ret < 0)
    {
        LOG_ERROR("init super block failed. name(%s)\n", ct_name);
        close_container(tmp_ct);
        return ret;
    }

    ret = os_disk_create(&tmp_ct->disk_hnd, ct_name);
    if (ret < 0)
    {
        LOG_ERROR("init disk failed. ret(%d)\n", ret);
        close_container(tmp_ct);
        return ret;
    }

    ret = ofs_init_super_block(tmp_ct);
    if (ret < 0)
    {
        LOG_ERROR("Update super block failed. ct_name(%s) vbn(%lld) ret(%d)\n", ct_name, SUPER_BLOCK_VBN, ret);
        close_container(tmp_ct);
        return ret;
    }
    
    ret = create_system_objects(tmp_ct);
    if (ret < 0)
    {
        LOG_ERROR("create system objects failed. ct_name(%s) ret(%d)\n", ct_name, ret);
        close_container(tmp_ct);
        return ret;
    }
    
    ret = ofs_update_super_block(tmp_ct);
    if (ret < 0)
    {
        LOG_ERROR("Update super block failed. hnd(%p) ret(%d)\n", tmp_ct, ret);
        close_container(tmp_ct);
        return ret;
    }

    *ct = tmp_ct;

    LOG_INFO("Create the ct success. ct_name(%s) total_sectors(%lld) ct(%p)\n", ct_name, total_sectors, tmp_ct);
    
    return 0;
}     

int32_t ofs_create_container(const char *ct_name, uint64_t total_sectors, container_handle_t **ct)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_container_list_rwlock);
    ret = ofs_create_container_nolock(ct_name, total_sectors, ct);
    OS_RWLOCK_WRUNLOCK(&g_container_list_rwlock);

    return ret;
}     

int32_t ofs_open_nolock(const char *ct_name, container_handle_t **ct)
{
    container_handle_t *tmp_ct = NULL;
    int32_t ret = 0;
    container_handle_t *hnd = NULL;
    avl_index_t where = 0;
    ofs_super_block_t *sb = NULL;

    if ((ct == NULL) || (ct_name == NULL))
    {
        LOG_ERROR("Invalid parameter. ct(%p) ct_name(%p)\n", ct, ct_name);
        return -INDEX_ERR_PARAMETER;
    }

    if (strlen(ct_name) >= OFS_NAME_SIZE)
    {
        LOG_ERROR("file name size must < %d bytes.\n", OFS_NAME_SIZE);
        return -INDEX_ERR_PARAMETER;
    }

    LOG_INFO("Open the ct. ct_name(%s)\n", ct_name);

    tmp_ct = avl_find(g_container_list, (avl_find_fn_t)compare_container2, ct_name, &where);
    if (NULL != tmp_ct)
    {
        tmp_ct->ref_cnt++;
        *ct = tmp_ct;
        LOG_WARN("File ref_cnt inc. ct_name(%s) ref_cnt(%d)\n", ct_name, tmp_ct->ref_cnt);
        return 0;
    }

    ret = init_container_resource(&tmp_ct, ct_name);
    if (ret < 0)
    {
        LOG_ERROR("Init ct resource failed. ct_name(%s) ret(%d)\n", ct_name, ret);
        return ret;
    }

    ret = os_disk_open(&tmp_ct->disk_hnd, ct_name);
    if (ret < 0)
    {
        LOG_ERROR("Open disk failed. ct_name(%s) ret(%d)\n", ct_name, ret);
        (void)close_container(tmp_ct);
        return ret;
    }

    sb = &tmp_ct->sb;

    sb->sectors_per_block = SECTORS_PER_BLOCK;
    sb->block_size = BYTES_PER_BLOCK;

    ret = ofs_read_super_block(tmp_ct);
    if (ret < 0)
    {
        LOG_ERROR("Read block failed. ct_name(%s) vbn(%lld) ret(%d)\n", ct_name, SUPER_BLOCK_VBN, ret);
        (void)close_container(tmp_ct);
        return ret;
    }

    ret = check_super_block(sb);
    if (ret < 0)
    {
        LOG_ERROR("Check super block failed. ct_name(%s) ret(%d)\n", ct_name, ret);
        (void)close_container(tmp_ct);
        return ret;
    }

    /* open system object */
    ret = open_system_objects(tmp_ct);
    if (ret < 0)
    {
        LOG_ERROR("Open system object failed. ct_name(%s) ret(%d)\n", ct_name, ret);
        close_container(tmp_ct);
        return ret;
    }

    *ct = tmp_ct;
    
    LOG_INFO("Open the ct success. ct_name(%s) ct(%p)\n", ct_name, ct);

    return 0;
}     

int32_t ofs_open_container(const char *ct_name, container_handle_t **ct)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_container_list_rwlock);
    ret = ofs_open_nolock(ct_name, ct);
    OS_RWLOCK_WRUNLOCK(&g_container_list_rwlock);
    
    return ret;
}     

void close_container(container_handle_t *ct)
{
    ASSERT(ct != NULL);

    // close all user object
    avl_walk_all(&ct->obj_info_list, (avl_walk_cb_t)close_one_object, NULL);

    // close system object
    if (ct->id_obj != NULL)
    {
        close_object(ct->id_obj->obj_info);
        ct->id_obj = NULL;
    }

    ofs_destroy_sm(&ct->sm);
    ofs_destroy_sm(&ct->bsm);

    flush_fs_cache(ct);
    
    release_fs_all_cache(ct);
    
    if (ct->disk_hnd != NULL)
    {
        (void) os_disk_close(ct->disk_hnd);
        ct->disk_hnd = NULL;
    }

    destroy_container_resource(ct);

    return;
}

int32_t ofs_close_nolock(container_handle_t *ct)
{
    if (ct == NULL)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. ct(%p)\n", ct);
        return -INDEX_ERR_PARAMETER;
    }

    LOG_INFO("Close the ct. ct(%p) ref_cnt(%d) name(%s)\n", ct, ct->ref_cnt, ct->name);
    
    if (0 == ct->ref_cnt)
    {
        LOG_ERROR("The ref_cnt is 0. ct(%p) ref_cnt(%d) name(%s)\n", ct, ct->ref_cnt, ct->name);
        return 0;
    }
    
    if (--ct->ref_cnt)
    {
        LOG_WARN("The ct ref_cnt dec. ct(%p) ref_cnt(%d) name(%s)\n", ct, ct->ref_cnt, ct->name);
        return 0;
    }
    
    close_container(ct);

    LOG_INFO("Close the ct success. ct(%p)\n", ct);

    return 0;
}     

int32_t ofs_close_container(container_handle_t *ct)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_container_list_rwlock);
    ret = ofs_close_nolock(ct);
    OS_RWLOCK_WRUNLOCK(&g_container_list_rwlock);

    return ret;
}     

container_handle_t *ofs_get_container_handle(const char *ct_name)
{
    container_handle_t *ct = NULL;
    avl_index_t where = 0;
 
    if (ct_name == NULL)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. ct_name(%p)\n", ct_name);
        return NULL;
    }

    OS_RWLOCK_WRLOCK(&g_container_list_rwlock);
    ct = avl_find(g_container_list, (avl_find_fn_t)compare_container2, ct_name, &where);
    OS_RWLOCK_WRUNLOCK(&g_container_list_rwlock);
    
    return ct;
}     

EXPORT_SYMBOL(ofs_create_container);
EXPORT_SYMBOL(ofs_open_container);
EXPORT_SYMBOL(ofs_close_container);


