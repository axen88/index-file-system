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
os_rwlock g_index_list_rwlock;

void close_index(index_handle_t *index);
extern int32_t fixup_index(index_handle_t *index);

int32_t compare_index1(const index_handle_t *index, const index_handle_t *index_node)
{
    return os_collate_ansi_string(index->name, strlen(index->name),
        index_node->name, strlen(index_node->name));
}

int32_t compare_index2(const char *index_name, index_handle_t *index_node)
{
    return os_collate_ansi_string(index_name, strlen(index_name),
        index_node->name, strlen(index_node->name));
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

    avl_create(g_index_list, (int (*)(const void *, const void*))compare_index1, sizeof(index_handle_t),
        OS_OFFSET(index_handle_t, entry));
    OS_RWLOCK_INIT(&g_index_list_rwlock);

    return 0;
}

int32_t close_one_index(void *para, index_handle_t *index)
{
    ASSERT(NULL != index);

    close_index(index);

    return 0;
}

int32_t close_one_object(void *para, object_info_t *obj_info)
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

    (void)avl_walk_all(g_index_list, (avl_walk_cb_t)close_one_index, NULL);

    OS_FREE(g_index_list);
    g_index_list = NULL;
    OS_RWLOCK_DESTROY(&g_index_list_rwlock);

	return;
}

int32_t walk_all_opened_index(
    int32_t (*func)(void *, index_handle_t *), void *para)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = avl_walk_all(g_index_list, (avl_walk_cb_t)func, para);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return ret;
}

int32_t init_index_resource(index_handle_t ** index, const char * index_name)
{
    index_handle_t *tmp_index = NULL;

    ASSERT(NULL != index);
    ASSERT(NULL != index_name);

    tmp_index = (index_handle_t *) OS_MALLOC(sizeof(index_handle_t));
    if (NULL == tmp_index)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", (uint32_t)sizeof(index_handle_t));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    memset(tmp_index, 0, sizeof(index_handle_t));
    strncpy(tmp_index->name, index_name, INDEX_NAME_SIZE);
    OS_RWLOCK_INIT(&tmp_index->index_lock);
    tmp_index->index_ref_cnt = 1;
    avl_create(&tmp_index->obj_list, (int (*)(const void *, const void*))compare_object1, sizeof(object_info_t),
        OS_OFFSET(object_info_t, entry));
    avl_add(g_index_list, tmp_index);

    *index = tmp_index;

    return 0;
}

int32_t create_system_objects(index_handle_t *index)
{
    int32_t ret;
    object_handle_t *obj;

    index->base_blk = 0;
    
    /* create base object */
    ret = create_object_at_inode(index, BASE_OBJ_ID, BASE_OBJ_INODE, FLAG_SYSTEM | FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create base object failed. name(%s)\n", index->name);
        return ret;
    }

    set_object_name(obj, BASE_OBJ_NAME);
    index->sb.base_inode_no = obj->obj_info->inode_no;
    index->sb.base_id = obj->obj_info->inode.objid;
    
    index_init_sm(&index->bsm, obj, 0, 0);

    /* create space object */
    ret = create_object_at_inode(index, SPACE_OBJ_ID, SPACE_OBJ_INODE, FLAG_SYSTEM | FLAG_TABLE | CR_EXTENT | (CR_EXTENT << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create free block object failed. name(%s)\n", index->name);
        return ret;
    }
    
    set_object_name(obj, SPACE_OBJ_NAME);
    index->sb.space_inode_no = obj->obj_info->inode_no;
    index->sb.space_id = obj->obj_info->inode.objid;
    
    index->sb.first_free_block += 2;
    index->sb.free_blocks -= 2;

    index_init_sm(&index->sm, obj, index->sb.first_free_block, index->sb.free_blocks);
    ret = index_init_free_space(&index->sm, index->sb.first_free_block, index->sb.free_blocks);
    if (ret < 0)
    {
        LOG_ERROR("init free block space info failed. name(%s)\n", index->name);
        return ret;
    }

    /* create objid object */
    ret = create_object(index, OBJID_OBJ_ID, FLAG_SYSTEM | FLAG_TABLE | CR_U64 | (CR_U64 << 4), &obj);
    if (ret < 0)
    {
        LOG_ERROR("Create objid object failed. name(%s)\n", index->name);
        return ret;
    }

    set_object_name(obj, OBJID_OBJ_NAME);
    index->sb.objid_inode_no = obj->obj_info->inode_no;
    index->sb.objid_id = obj->obj_info->inode.objid;
    index->id_obj = obj;
    
    return 0;
}

int32_t open_system_objects(index_handle_t *index)
{
    int32_t ret;
    object_handle_t *obj;

    index->base_blk = index->sb.base_blk;
    
    /* open BASE object */
    ret = open_object(index, index->sb.base_id, index->sb.base_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open base object failed. index_name(%s) ret(%d)\n", index->name, ret);
        return ret;
    }
    
    index_init_sm(&index->bsm, obj, index->sb.base_first_free_block, index->sb.base_free_blocks);

    /* open SPACE object */
    ret = open_object(index, index->sb.space_id, index->sb.space_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open space object failed. index_name(%s) ret(%d)\n", index->name, ret);
        return ret;
    }
    
    index_init_sm(&index->sm, obj, index->sb.first_free_block, index->sb.free_blocks);

    /* open $OBJID object */
    ret = open_object(index, index->sb.objid_id, index->sb.objid_inode_no, &obj);
    if (ret < 0)
    {
        LOG_ERROR("Open objid object failed. index_name(%s) ret(%d)\n", index->name, ret);
        return ret;
    }

    index->id_obj = obj;

    return 0;
}

int32_t init_super_block(ifs_super_block_t *sb, uint64_t total_sectors, uint32_t block_size_shift,
    uint32_t reserved_blocks, uint64_t start_lba)
{
    int32_t ret = 0;
    uint64_t total_blocks = 0;

    if (block_size_shift < BYTES_PER_SECTOR_SHIFT)
    {
        LOG_ERROR("The parameter is invalid. block_size_shift(%d)\n", block_size_shift);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    reserved_blocks++;
    total_blocks = total_sectors >> (block_size_shift - BYTES_PER_SECTOR_SHIFT);
    if (total_blocks <= reserved_blocks)
    {
        LOG_ERROR("The parameter is invalid. total_blocks(%lld) reserved_blocks(%d)\n",
            total_blocks, reserved_blocks);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    sb->head.blk_id = SUPER_BLOCK_ID;
    sb->head.real_size = SUPER_BLOCK_SIZE;
    sb->head.alloc_size = SUPER_BLOCK_SIZE;
    sb->head.seq_no = 0;
    sb->block_size_shift = block_size_shift;
    sb->block_size = 1 << block_size_shift;
    sb->sectors_per_block = sb->block_size / BYTES_PER_SECTOR;
    sb->bitmap_start_block = reserved_blocks;
    sb->bitmap_blocks =
        (uint32_t) RoundUp2(RoundUp2(total_blocks, BITS_PER_BYTE,
            BITS_PER_BYTE_SHIFT), sb->block_size, block_size_shift);
    sb->total_blocks = total_blocks;
    sb->free_blocks = total_blocks;
    sb->first_free_block = 1;
    sb->start_lba = start_lba;
    sb->version = VERSION;
    sb->flags = 0;
    sb->magic_num = BLOCK_MAGIC_NUMBER;

    return 0;
}

int32_t check_super_block(ifs_super_block_t * sb, uint64_t start_lba)
{
    ASSERT(NULL != sb);
    
    if (sb->magic_num != BLOCK_MAGIC_NUMBER)
    {
        LOG_ERROR( "magic_num not match. magic_num(%x) expect(%x)\n",
            sb->magic_num, BLOCK_MAGIC_NUMBER);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->version != VERSION)
    {
        LOG_ERROR("version not match. version(%04d) expect(%04d)\n", sb->version, VERSION);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->sectors_per_block != SECTORS_PER_BLOCK)
    {
        LOG_ERROR("sectors_per_block not match. sectors_per_block(%d) expect(%d)\n",
            sb->sectors_per_block, SECTORS_PER_BLOCK);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->block_size != BYTES_PER_BLOCK)
    {
        LOG_ERROR("block_size not match. block_size(%d) expect(%d)\n",
            sb->block_size, BYTES_PER_BLOCK);
        return -FILE_BLOCK_ERR_FORMAT;
    }

    if (sb->start_lba != start_lba)
    {
        LOG_ERROR("start_lba changed. old(%lld) new(%lld)\n", sb->start_lba, start_lba);
        sb->start_lba = start_lba;
    }

    return 0;
}

int32_t block_update_super_block(index_handle_t * hnd)
{
    int32_t ret = 0;

    if (NULL == hnd)
    {
        LOG_ERROR("Invalid parameter. hnd(%p)\n", hnd);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    ret = index_update_block_pingpong(hnd, &hnd->sb.head, SUPER_BLOCK_VBN);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. hnd(%p) vbn(%lld) ret(%d)\n",
            hnd, SUPER_BLOCK_VBN, ret);
    }

    return ret;
}


int32_t index_create_nolock(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    index_handle_t **index)
{
    index_handle_t *tmp_index = NULL;
    int32_t ret = 0;
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
    tmp_index = avl_find(g_index_list, (avl_find_fn_t)compare_index2, index_name, &where);
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
    
    /* init super block */
    ret = init_super_block(&tmp_index->sb, total_sectors, BYTES_PER_BLOCK_SHIFT, 0, start_lba);
    if (ret < 0)
    {
        LOG_ERROR("init super block failed. name(%s)\n", index_name);
        close_index(tmp_index);
        return ret;
    }

    ret = os_disk_create(&tmp_index->file_hnd, index_name);
    if (0 > ret)
    {
        LOG_ERROR("init disk failed. ret(%d)\n", ret);
        close_index(tmp_index);
        return ret;
    }

    ret = index_update_block_pingpong_init(tmp_index, &tmp_index->sb.head, SUPER_BLOCK_VBN);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. index_name(%s) vbn(%lld) ret(%d)\n",
            index_name, SUPER_BLOCK_VBN, ret);
        close_index(tmp_index);
        return ret;
    }

    ret = create_system_objects(tmp_index);
    if (0 > ret)
    {
        LOG_ERROR("create system objects failed. index_name(%s) ret(%d)\n", index_name, ret);
        close_index(tmp_index);
        return ret;
    }
    
    ret = block_update_super_block(tmp_index);
    if (0 > ret)
    {
        LOG_ERROR("Update super block failed. hnd(%p) ret(%d)\n", tmp_index, ret);
        close_index(tmp_index);
        return ret;
    }

    *index = tmp_index;

    LOG_INFO("Create the index success. index_name(%s) total_sectors(%lld) start_lba(%lld) index(%p)\n",
        index_name, total_sectors, start_lba, tmp_index);
    
    return 0;
}     

int32_t index_create(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    index_handle_t **index)
{
    int32_t ret = 0;
    
    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_create_nolock(index_name, total_sectors, start_lba, index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);

    return ret;
}     

int32_t index_open_nolock(const char *index_name, uint64_t start_lba, index_handle_t **index)
{
    index_handle_t *tmp_index = NULL;
    int32_t ret = 0;
    index_handle_t *hnd = NULL;
    avl_index_t where = 0;
    ifs_super_block_t *sb = NULL;

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

    tmp_index = avl_find(g_index_list, (avl_find_fn_t)compare_index2, index_name, &where);
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

    ret = os_disk_open(&tmp_index->file_hnd, index_name);
    if (ret < 0)
    {
        LOG_ERROR("Open disk failed. index_name(%s) ret(%d)\n", index_name, ret);
        (void)close_index(tmp_index);
        return ret;
    }

    sb = &tmp_index->sb;

    sb->sectors_per_block = SECTORS_PER_BLOCK;
    sb->block_size = BYTES_PER_BLOCK;
    sb->start_lba = start_lba;

    ret = index_read_block_pingpong(tmp_index, &sb->head, SUPER_BLOCK_VBN, SUPER_BLOCK_ID, SUPER_BLOCK_SIZE);
    if (0 > ret)
    {
        LOG_ERROR("Read block failed. index_name(%s) vbn(%lld) ret(%d)\n",
            index_name, SUPER_BLOCK_VBN, ret);
        (void)close_index(tmp_index);
        return ret;
    }

    ret = check_super_block(sb, start_lba);
    if (0 > ret)
    {
        LOG_ERROR("Check super block failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        (void)close_index(tmp_index);
        return ret;
    }

    /* open system object */
    ret = open_system_objects(tmp_index);
    if (ret < 0)
    {
        LOG_ERROR("Open system object failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        close_index(tmp_index);
        return ret;
    }

    *index = tmp_index;
    
    LOG_INFO("Open the index success. index_name(%s) start_lba(%lld) index(%p)\n",
        index_name, start_lba, index);

    return 0;
}     

int32_t index_open(const char *index_name, uint64_t start_lba, index_handle_t **index)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_open_nolock(index_name, start_lba, index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return ret;
}     

void close_index(index_handle_t *index)
{
    int32_t ret;
    
    ASSERT(NULL != index);

    // close all user object
    avl_walk_all(&index->obj_list, (avl_walk_cb_t)close_one_object, NULL);

    // close system object
    if (index->id_obj != NULL)
    {
        (void)close_object(index->id_obj->obj_info);
    }

    if (index->sm.space_obj != NULL)
    {
        (void)close_object(index->sm.space_obj->obj_info);
    }

    if (index->bsm.space_obj != NULL)
    {
        (void)close_object(index->bsm.space_obj->obj_info);
    }

    if (NULL != index->file_hnd)
    {
        if (0 != (index->flags & FLAG_DIRTY))
        {
            index->flags &= ~FLAG_DIRTY;
            index->sb.flags &= ~FLAG_FIXUP;
            ret = block_update_super_block(index);
            if (0 > ret)
            {
                LOG_ERROR("Update super block failed. index(%p) ret(%d)\n", index, ret);
            }
        }
        
        index->sb.base_blk = index->base_blk;
        (void) os_disk_close(index->file_hnd);
        index->file_hnd = NULL;
    }

    avl_destroy(&index->obj_list);
    OS_RWLOCK_DESTROY(&index->index_lock);
    avl_remove(g_index_list, index);

    OS_FREE(index);
    index = NULL;

    return;
}

int32_t index_close_nolock(index_handle_t *index)
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

int32_t index_close(index_handle_t *index)
{
    int32_t ret = 0;

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    ret = index_close_nolock(index);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);

    return ret;
}     

index_handle_t *index_get_handle(const char * index_name)
{
    index_handle_t *index = NULL;
    avl_index_t where = 0;
 
    if (NULL == index_name)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. index_name(%p)\n", index_name);
        return NULL;
    }

    OS_RWLOCK_WRLOCK(&g_index_list_rwlock);
    index = avl_find(g_index_list, (avl_find_fn_t)compare_index2, index_name, &where);
    OS_RWLOCK_WRUNLOCK(&g_index_list_rwlock);
    
    return index;
}     

