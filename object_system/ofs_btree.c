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
File Name: INDEX_BTREE.C
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

MODULE(PID_BTREE);
#include "os_log.h"

#define INDEX_GETTO_BEGIN  1

// set the block from current block to root block as dirty
static int32_t set_ib_dirty(object_handle_t *tree)
{
    uint64_t new_vbn = 0;
    int32_t ret = 0;
    index_entry_t *ie = NULL;
    uint64_t old_vbn = 0;
    uint64_t vbn = 0;
    uint8_t depth = tree->depth;

    ASSERT(tree != NULL);
    ASSERT(depth < TREE_MAX_DEPTH);

    do
    {
        if (!CACHE_DIRTY(tree->cache_stack[depth]))
        {
            // allocate new block for modified data
            ret = OFS_ALLOC_BLOCK(tree->ct, tree->obj_info->objid, &new_vbn);
            if (ret < 0)
            {
                LOG_ERROR("Allocate new block failed. ret(%d)\n", ret);
                return ret;
            }
            
            // record old block
            old_vbn = tree->cache_stack[depth]->vbn;
            
            OS_RWLOCK_WRLOCK(&tree->obj_info->caches_lock);
            if (depth == 0)
            { // root node should be treated specially
                tree->obj_info->root_cache.vbn = new_vbn;
                change_obj_cache_vbn(tree->obj_info, tree->obj_info->inode_cache, new_vbn);
            }
            else
            {
                change_obj_cache_vbn(tree->obj_info, tree->cache_stack[depth], new_vbn);
            }
            OS_RWLOCK_WRUNLOCK(&tree->obj_info->caches_lock);
        }

        if (vbn != 0)
        {
            ie = (index_entry_t *)((uint8_t *)tree->cache_stack[depth]->ib + tree->position_stack[depth]);
            SET_IE_VBN(ie, vbn);
        }

        if (CACHE_DIRTY(tree->cache_stack[depth]))
        {
            return 0;
        }

        SET_CACHE_DIRTY(tree->cache_stack[depth]);
        vbn = new_vbn;
        ret = OFS_FREE_BLOCK(tree->ct, tree->obj_info->objid, old_vbn);
        if (ret < 0)
        {
            LOG_ERROR("Free old block failed. ret(%d)\n", ret);
            return ret;
        }
    }  while (depth--);

    return 0;
}

static void get_last_ie(object_handle_t *tree)
{
    uint32_t last_ie_len = ENTRY_END_SIZE;
    
    ASSERT(tree != NULL);

    if (IB(tree->cache->ib)->node_type & INDEX_BLOCK_LARGE)
    {
        last_ie_len += VBN_SIZE;
    }

    tree->ie = (index_entry_t *)(GET_END_IE(tree->cache->ib) - last_ie_len);
    tree->position = tree->cache->ib->real_size - last_ie_len;

    return;
}

static void reset_cache_stack(object_handle_t *tree, uint8_t flags)
{
    ASSERT(tree != NULL);
    
    /* get to first entry */
    tree->cache = &tree->obj_info->root_cache;
    tree->cache_stack[0] = tree->cache;
    tree->depth = 0;

    if (flags & INDEX_GET_LAST)
    {
        get_last_ie(tree);
        return;
    }

    tree->ie = GET_FIRST_IE(tree->cache->ib);
    tree->position = IB(tree->cache->ib)->first_entry_off;

    return;
}   

// go to next level depth
static int32_t push_cache_stack(object_handle_t *tree, uint8_t flags)
{
    uint64_t vbn = 0;
    int32_t ret = 0;

    ASSERT(tree != NULL);
    
    /* get to max depth */
    if (tree->depth >= (TREE_MAX_DEPTH - 1))
    {
        LOG_ERROR("Depth get to MAX. depth(%d)\n", tree->depth);
        return -INDEX_ERR_MAX_DEPTH;
    }

    // get the vbn of the new block
    vbn = GET_IE_VBN(tree->ie);

    LOG_DEBUG("Depth increase. depth(%d) vbn(%lld) pos(%d)\n", tree->depth, vbn, tree->position);

    ret = index_block_read(tree, vbn, INDEX_MAGIC);
    if (ret < 0)
    {
        LOG_ERROR("Read ct block failed. vbn(%lld) ret(%d)\n", vbn, ret);
        return ret;
    }

	tree->position_stack[tree->depth] = tree->position;
    tree->depth++;
    tree->cache_stack[tree->depth] = tree->cache;

    if (((IB(tree->cache->ib)->node_type & INDEX_BLOCK_LARGE) == 0) && (tree->max_depth != tree->depth))
    {
        LOG_INFO("The max depth changed. oldMaxDepth(%d) newMaxDepth(%d)\n",
            tree->max_depth, tree->depth);
        tree->max_depth = tree->depth;
    }

    if (flags & (INDEX_GET_LAST | INDEX_GET_PREV))
    {
        get_last_ie(tree);
        return 0;
    }

    // go to the first entry of the new block
    tree->position = IB(tree->cache->ib)->first_entry_off;
    tree->ie = GET_FIRST_IE(tree->cache->ib);

    return 0;
}

// go to prev entry
static int32_t get_prev_ie(object_handle_t *tree)
{
    ASSERT(tree != NULL);
    
    tree->position -= tree->ie->prev_len;
    tree->ie = GET_PREV_IE(tree->ie);
    if ((uint8_t *)tree->ie < (uint8_t *)&IB(tree->cache->ib)->begin_entry)
    {   /* Check valid */
        LOG_ERROR("The ie is invalid. ie(%p) begin_entry(%p)\n",
            tree->ie, &IB(tree->cache->ib)->begin_entry);
        return -INDEX_ERR_FORMAT;
    }

    if (0 == tree->ie->prev_len)
    {
        if (tree->ie->flags & INDEX_ENTRY_BEGIN)
        {
            return INDEX_GETTO_BEGIN;
        }

        LOG_ERROR("The ie is invalid. prev_len(%d) flags(0x%x)\n",
            tree->ie->prev_len, tree->ie->flags);
        return -INDEX_ERR_FORMAT;
    }

    return 0;
}

// go to top level
static int32_t pop_cache_stack(object_handle_t *tree, uint8_t flags)
{
    ASSERT(tree != NULL);
    
    if (0 == tree->depth)
    {   /* Get to root */
        LOG_DEBUG("Depth get to root. depth(%d)\n", tree->depth);
        return -INDEX_ERR_ROOT;
    }

    tree->depth--;
    tree->cache = tree->cache_stack[tree->depth];
	tree->position = tree->position_stack[tree->depth];

    LOG_DEBUG("Depth decrease. depth(%d) vbn(%lld) pos(%d)\n", tree->depth, tree->cache->vbn, tree->position);

    // recover the entry
    tree->ie = (index_entry_t *)((uint8_t *)tree->cache->ib + tree->position);

    if (flags & INDEX_GET_PREV)
    {
        return get_prev_ie(tree);
    }

    return 0;
}      

void init_ib(index_block_t *ib, uint8_t node_type, uint32_t aloc_size)
{
    index_entry_t *ie = NULL;

    ASSERT(ib != NULL);
    
    memset(ib, 0, aloc_size);
    
    ib->head.blk_id = INDEX_MAGIC;
    ib->head.alloc_size = aloc_size;
    
    ib->first_entry_off = sizeof(index_block_t);
    if (node_type & INDEX_BLOCK_LARGE)
    {   // have child node
        ib->head.real_size = sizeof(index_block_t) + ENTRY_END_SIZE + VBN_SIZE;
    }
    else
    {   // no child node
        ib->head.real_size = sizeof(index_block_t) + ENTRY_END_SIZE;
    }

    ib->node_type = node_type;

    ie = GET_FIRST_IE(ib);

    if (node_type & INDEX_BLOCK_LARGE)
    {   // have child node
        ie->flags = INDEX_ENTRY_END | INDEX_ENTRY_NODE;
        ie->len = ENTRY_END_SIZE + VBN_SIZE;
    }
    else
    {   // no child node
        ie->flags = INDEX_ENTRY_END;
        ie->len = ENTRY_END_SIZE;
    }

    ie->prev_len = ENTRY_BEGIN_SIZE;

    ie = &ib->begin_entry;
    ie->len = ENTRY_BEGIN_SIZE;
    ie->prev_len = 0;
    ie->flags = INDEX_ENTRY_BEGIN;

    return;
}

// make the block no child
static void make_ib_small(index_block_t *ib)
{
    index_entry_t *ie = NULL;

    ASSERT(ib != NULL);
    
    ib->head.real_size = sizeof(index_block_t) + ENTRY_END_SIZE;
    ib->node_type = INDEX_BLOCK_SMALL;
    
    ie = GET_FIRST_IE(ib);
    ie->len = ENTRY_END_SIZE;
    ie->prev_len = ENTRY_BEGIN_SIZE;
    ie->key_len = 0;
    ie->value_len = 0;
    ie->flags = INDEX_ENTRY_END;

    ie = &ib->begin_entry;
    ie->len = ENTRY_BEGIN_SIZE;
    ie->prev_len = 0;
    ie->key_len = 0;
    ie->value_len = 0;
    ie->flags = INDEX_ENTRY_BEGIN;

    return;
}     

// get the next entry
static int32_t get_next_ie(object_handle_t *tree)
{
    ASSERT(NULL != tree);

    if (0 != (tree->ie->flags & INDEX_ENTRY_END))
    { // the last entry
        return -INDEX_ERR_NEXT_ENTRY;
    }
    
    tree->position += tree->ie->len;
    tree->ie = GET_NEXT_IE(tree->ie);
    if (((uint8_t *) tree->ie >= GET_END_IE(tree->cache->ib)) || (0 == tree->ie->len))
    {   /* Check valid */
        LOG_ERROR("The ie is invalid. len(%d) ib real_size(%d)\n", tree->ie->len, tree->cache->ib->real_size);
        return -INDEX_ERR_FORMAT;
    }

    return 0;
}    

static int32_t add_or_remove_ib(object_handle_t *tree, uint8_t flags)
{
    int32_t ret = 0;

    ASSERT(NULL != tree);
        
    if (flags & INDEX_REMOVE_BLOCK)
    {
        ret = OFS_FREE_BLOCK(tree->ct, tree->obj_info->objid, tree->cache->vbn);
        if (ret < 0)
        {
            LOG_ERROR("Free block failed. vbn(%lld) ret(%d)\n", tree->cache->vbn, ret);
            return ret;
        }
    }
    else if (flags & INDEX_ADD_BLOCK)
    {
       // ret = block_set_status(tree->obj_info->ct->hnd, tree->cache->vbn,
       //     1, TRUE);
       // if (ret < 0)
        {
        //    LOG_ERROR("Set block status failed. vbn(%lld) ret(%d)\n", tree->cache->vbn, ret);
          //  return ret;
        }
    }

    return 0;
}
    
// go to current entry
static int32_t get_current_ie(object_handle_t *tree, uint8_t flags)
{
    int32_t ret = 0;

    while (tree->ie->flags & INDEX_ENTRY_NODE)
    { /* Have children */
        ret = push_cache_stack(tree, flags);
        if (ret < 0)
        { /* Push the information OS_S32o the history, and read new ct block */
            LOG_ERROR("Go to child node failed. ret(%d)\n", ret);
            return ret;
        }
    }

    while (tree->ie->flags & (INDEX_ENTRY_END | INDEX_ENTRY_BEGIN))
    { /* The Index END */
        ret = add_or_remove_ib(tree, flags);
        if (ret < 0)
        {
            LOG_ERROR("add_or_remove_ib failed. flags(%x) ret(%d)\n", flags, ret);
            return ret;
        }

        if (flags & (INDEX_GET_LAST | INDEX_GET_PREV))
        {
            if (0 != (flags & INDEX_GET_LAST_ENTRY))
            {
                break;
            }
            
            ret = get_prev_ie(tree);
            if (ret < 0)
            {
                LOG_ERROR("Get prev entry failed. ret(%d)\n", ret);
                return ret;
            }
            else if (0 == ret)
            {
                break;
            }
        }

        ret = pop_cache_stack(tree, flags);
        if (ret < 0)
        { /* Up to parent failed */
            if (ret != -INDEX_ERR_ROOT)
            {
                LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            }

            return ret;
        }
    }

    return 0;
}

int32_t walk_tree(object_handle_t *tree, uint8_t flags)
{
    int32_t ret = 0;

    ASSERT(tree != NULL);
    ASSERT(tree->obj_info->attr_record->flags & FLAG_TABLE);

    if (flags & (INDEX_GET_FIRST | INDEX_GET_LAST))
    {   /* Get to the root's first entry */
        reset_cache_stack(tree, flags);
    }
    else if (flags & INDEX_GET_PREV)
    {  /* get prev */
        if (!(tree->ie->flags & INDEX_ENTRY_NODE))
        {
            ret = get_prev_ie(tree);
            if (ret < 0)
            {
                LOG_ERROR("Get prev entry failed. ret(%d)\n", ret);
                return ret;
            }
        }
    }
    else if (!(flags & INDEX_GET_CURRENT))
    {  /* get next */
        ret = get_next_ie(tree);
        if (ret < 0)
        {
            LOG_ERROR("Get next entry failed. ret(%d)\n", ret);
            return ret;
        }
    }

    return get_current_ie(tree, flags);
}

// search key
int32_t search_key_internal(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    int32_t ret = 0;

    ASSERT(tree != NULL);
    ASSERT(key != NULL);
    ASSERT(key_len != 0);

    reset_cache_stack(tree, 0);

    for (;;)
    {
        while (0 == (tree->ie->flags & INDEX_ENTRY_END))
        {       /* It is not the Index END */
            uint16_t cr = tree->obj_info->attr_record->flags & CR_MASK;
            
            ret = collate_key(cr, tree->ie, key, key_len, value, value_len);
            if (ret > 0)
            { // get a key larger
                break;
            }
            
            if (ret == 0)
            { // found
                return 0;
            }

            if (ret == -INDEX_ERR_COLLATE)
            {
                LOG_ERROR("Collate rule is invalid. collate_rule(%d)\n", cr);
                return ret;
            }
            
            // get a key smaller
            ret = get_next_ie(tree);
            if (ret < 0)
            {
                LOG_ERROR("Get next entry failed. ret(%d)\n", ret);
                return ret;
            }
        }

        if ((tree->ie->flags & INDEX_ENTRY_NODE) == 0)
        {       /* No child */
            break;
        }
        
        /* Have child */
        ret = push_cache_stack(tree, 0);
        if (ret < 0)
        {   /* Push the information OS_S32o the history, and read new ct block */
            LOG_ERROR("Get child node failed. ret(%d)\n", ret);
            return ret;
        }
    }

    return -INDEX_ERR_KEY_NOT_FOUND;
}

// go to the near key
static void get_to_near_key(object_handle_t *tree)
{
    int32_t ret = 0;

    ASSERT(tree != NULL);

    while ((tree->ie->flags & INDEX_ENTRY_END) != 0)
    {   /* The Index END */
        ret = pop_cache_stack(tree, 0);
        if (ret < 0)
        {       /* Up to parent failed */
            if (ret != -INDEX_ERR_ROOT)
            {
                LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            }

            break;
        }
    }

    return;
}

int32_t index_search_key_nolock(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    ASSERT(tree->obj_info->attr_record->flags & FLAG_TABLE);

    ret = search_key_internal(tree, key, key_len, value, value_len);
    if (ret == -INDEX_ERR_KEY_NOT_FOUND)
    {
        get_to_near_key(tree);
    }

    return ret;
}

int32_t index_search_key(object_handle_t *tree, const void *key, uint16_t key_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    ret = index_search_key_nolock(tree, key, key_len, NULL, 0);
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);

    return ret;
}

static index_entry_t *get_middle_ie(index_block_t *ib)
{
    index_entry_t *ie = NULL;
    uint32_t uiMidPos = 0;

    ASSERT(ib != NULL);
    
    uiMidPos = (ib->head.real_size - sizeof(index_block_t)) >> 1;
    ie = GET_FIRST_IE(ib);
    while (!(ie->flags & INDEX_ENTRY_END))
    {
        if (uiMidPos > ie->len)
        {
            uiMidPos -= ie->len;
            ie = GET_NEXT_IE(ie);
        }
        else
        {
            break;
        }
    }

    return ie;
}  

// get the length from current entry to the end entry
uint32_t get_entries_length(index_entry_t *ie)
{
    uint32_t len = 0;

    ASSERT(ie != NULL);
    
    for (;;)
    {   /* Not the END */
        if (ie->flags & INDEX_ENTRY_END)
        {
            return (len + ie->len);
        }

        if (0 == ie->len)
        {
            LOG_ERROR("The ie len is 0.\n");
            break;
        }

        len += ie->len;
        ie = GET_NEXT_IE(ie);
    }

    return len;
}      

// get the last entry
static index_entry_t *ib_get_last_ie(index_block_t *ib)
{
    uint32_t last_ie_len = ENTRY_END_SIZE;

    ASSERT(ib != NULL);
    
    if (ib->node_type & INDEX_BLOCK_LARGE)
    {
        last_ie_len += VBN_SIZE;
    }

    return (index_entry_t *)(GET_END_IE(ib) - last_ie_len);
}

static void remove_ie(index_block_t *ib, index_entry_t *ie)
{
    index_entry_t *next_ie = NULL;

    ASSERT(ib != NULL);
    ASSERT(ie != NULL);
    
    ib->head.real_size -= ie->len;
    next_ie = GET_NEXT_IE(ie);
    next_ie->prev_len = ie->prev_len;
    memcpy(ie, next_ie, get_entries_length(next_ie));

    return;
}

void insert_ie(index_block_t *ib, index_entry_t *ie, index_entry_t *pos)
{
    ASSERT(ib != NULL);
    ASSERT(ie != NULL);
    ASSERT(pos != NULL);
    
    ib->head.real_size += ie->len;

    ie->prev_len = pos->prev_len;
    pos->prev_len = ie->len;

    memmove((uint8_t *) pos + ie->len, pos, get_entries_length(pos));
    memcpy(pos, ie, ie->len);

    return;
}

// add vbn to an entry
static index_entry_t *dump_ie_add_vbn(index_entry_t *ie, uint64_t vbn)
{
    index_entry_t *new_ie = NULL;
    uint16_t size = 0;
    
    ASSERT(ie != NULL);

    size = ie->len;
    if (!(ie->flags & INDEX_ENTRY_NODE))
    {   /* The old @pstIE is not NODE (without ullVBN) */
        size += VBN_SIZE;
    }

    new_ie = (index_entry_t *)OS_MALLOC(size);
    if (NULL == new_ie)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", size);
        return NULL;
    }

    memcpy(new_ie, ie, ie->len);
    new_ie->len = size;
    new_ie->flags |= INDEX_ENTRY_NODE;
    SET_IE_VBN(new_ie, vbn);

    return new_ie;
}  

// delete vbn from an entry
static index_entry_t *dump_ie_del_vbn(index_entry_t *ie)
{
    index_entry_t *new_ie = NULL;
    uint16_t size = 0;
    
    ASSERT(ie != NULL);

    size = ie->len;
    if (ie->flags & INDEX_ENTRY_NODE)
    {   /* The old is NODE (with ullVBN) */
        size -= VBN_SIZE;
    }

    new_ie = (index_entry_t *)OS_MALLOC(size);
    if (NULL == new_ie)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", size);
        return NULL;
    }

    memcpy(new_ie, ie, size);
    new_ie->len = size;
    new_ie->flags &= ~INDEX_ENTRY_NODE;

    return new_ie;
}      

// copy the last half entries
void copy_ib_tail(index_block_t *dst_ib, index_block_t *src_ib, index_entry_t *mid_ie)
{
    uint32_t tail_size = 0;

    ASSERT(dst_ib != NULL);
    ASSERT(src_ib != NULL);
    ASSERT(mid_ie != NULL);
    
    init_ib(dst_ib, src_ib->node_type, src_ib->head.alloc_size);
    mid_ie = GET_NEXT_IE(mid_ie);

    mid_ie->prev_len = ENTRY_BEGIN_SIZE;

    tail_size = (uint32_t)((uint8_t *) GET_END_IE(src_ib) - (uint8_t *) mid_ie);
    memcpy(GET_FIRST_IE(dst_ib), mid_ie, tail_size);

    dst_ib->head.real_size = tail_size + dst_ib->first_entry_off;

    return;
}

// remove the last half entries
static void cut_ib_tail(index_block_t *src_ib, index_entry_t *ie)
{
    uint8_t *start = NULL;
    index_entry_t *last_ie = NULL;
    index_entry_t *prev_ie = NULL;

    ASSERT(src_ib != NULL);
    ASSERT(ie != NULL);
    
    start = (uint8_t *)GET_FIRST_IE(src_ib);

    last_ie = ib_get_last_ie(src_ib);
    if (last_ie->flags & INDEX_ENTRY_NODE)
    {
        SET_IE_VBN(last_ie, GET_IE_VBN(ie));
    }

    prev_ie = GET_PREV_IE(ie);
    last_ie->prev_len = prev_ie->len;

    memcpy(ie, last_ie, last_ie->len);
    src_ib->head.real_size = ie->len + (uint32_t)((uint8_t *) ie - start) + src_ib->first_entry_off;
}

// split one block into two block, and get the middle entry
static index_entry_t *split_ib(object_handle_t *tree, index_entry_t *ie)
{
    index_entry_t *mid_ie = NULL;
    index_entry_t *new_ie = NULL;
    index_block_t *new_ib = NULL;
    ofs_block_cache_t *new_cache = NULL;
    int32_t pos = 0;         /* Insert iOffset to the new indexHeader */
    int32_t ret = 0;
    
    ASSERT(tree != NULL);
    ASSERT(ie != NULL);

    mid_ie = get_middle_ie(IB(tree->cache->ib));

    ret = alloc_obj_block_and_cache(tree->obj_info, &new_cache, INDEX_MAGIC);
    if (ret < 0)
    {
        LOG_ERROR("Allocate cache failed.\n");
        return NULL;
    }
    
    new_ib = IB(new_cache->ib);

    copy_ib_tail(new_ib, IB(tree->cache->ib), mid_ie);

    pos = (int32_t)((uint8_t *)mid_ie - (uint8_t *)tree->ie);
    if (pos < 0)
    {   /* Insert the entry OS_S32o newIB */
        insert_ie(new_ib, ie, (index_entry_t *)(((uint8_t *)GET_FIRST_IE(new_ib) - pos) - mid_ie->len));
    }

    SET_CACHE_DIRTY(new_cache);

    //LOG_DEBUG("Write new ct block success. vbn(%lld)\n", new_cache->vbn);

    ret = set_ib_dirty(tree);
    if (ret < 0)
    {
        LOG_ERROR("Set ct block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        return NULL;
    }

    // Cut block tail and whether insert the @pstIE OS_S32o the old ct block
    new_ie = dump_ie_add_vbn(mid_ie, tree->cache->vbn);
    if (new_ie == NULL)
    {
        LOG_ERROR("dump_ie_add_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return NULL;
    }

    cut_ib_tail(IB(tree->cache->ib), mid_ie);
    
    if (pos >= 0)
    {   /* Insert the entry onto old ct block */
        insert_ie(IB(tree->cache->ib), ie, tree->ie);
    }

    if (pop_cache_stack(tree, 0) < 0)
    {
        LOG_ERROR("Go to parent node failed. vbn(%lld)\n", tree->cache->vbn);
        OS_FREE(new_ie);
        return NULL;
    }

    SET_IE_VBN(tree->ie, new_cache->vbn);     /* Change the link */

    return new_ie;
}

// copy the root entries into new block
static int32_t reparent_root(object_handle_t * tree)
{
    index_entry_t *ie = NULL;
    index_block_t *new_ib = NULL;
    ofs_block_cache_t *new_cache = NULL;
    index_block_t *old_ib = NULL;
    uint32_t alloc_size = 0;
    int32_t ret = 0;

    ASSERT(tree != NULL);
    ASSERT(tree->cache != NULL);
    ASSERT(tree->cache->ib != NULL);

    if (tree->max_depth >= (TREE_MAX_DEPTH - 1))
    {   /* Get to max ucDepth */
        LOG_ERROR("Depth get to MAX. depth(%d)\n", tree->max_depth);
        return -INDEX_ERR_MAX_DEPTH;
    }

    old_ib = IB(tree->cache->ib);
    alloc_size = old_ib->head.alloc_size;
    
    ret = alloc_obj_block_and_cache(tree->obj_info, &new_cache, INDEX_MAGIC);
    if (ret < 0)
    {
        LOG_ERROR("Allocate cache failed.\n");
        return ret;
    } 

    new_ib = IB(new_cache->ib);

    memcpy(new_ib, old_ib, old_ib->head.real_size);
    new_ib->head.alloc_size = tree->obj_info->ct->sb.block_size;

    SET_CACHE_DIRTY(new_cache);

    //LOG_DEBUG("Write new ct block success. vbn(%lld)\n", new_cache->vbn);

    init_ib(old_ib, INDEX_BLOCK_LARGE, alloc_size);
    ie = GET_FIRST_IE(old_ib);
    SET_IE_VBN(ie, new_cache->vbn);
    
    ret = set_ib_dirty(tree);
    if (ret < 0)
    {
        LOG_ERROR("Set ct block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(new_ib);
        return ret;
    }
    
	tree->position_stack[tree->depth] = IB(tree->cache->ib)->first_entry_off;
    tree->depth++;
    tree->cache_stack[tree->depth] = new_cache;
	tree->position_stack[tree->depth] = tree->position;

	tree->cache = new_cache;
    tree->ie = (index_entry_t *)((uint8_t *)new_ib + tree->position);

    return 0;
}    

static int32_t tree_insert_ie(object_handle_t *tree, index_entry_t **new_ie)
{
    uint32_t new_size = 0;
    index_entry_t *ie = NULL;
    
    ASSERT(tree != NULL);
    ASSERT(new_ie != NULL);
    ASSERT(*new_ie != NULL);

    ie = *new_ie;
    
    for (;;)
    {   /* The entry can't be inserted */
        new_size = tree->cache->ib->real_size + ie->len;
        if (new_size <= tree->cache->ib->alloc_size)
        {
            insert_ie(IB(tree->cache->ib), ie, tree->ie);       /* Insert the entry before current entry */
            return set_ib_dirty(tree);
        }

        if (0 == tree->depth)
        {       /* Current the $INDEX_ROOT opened */
            if (reparent_root(tree) < 0)
            {
                LOG_ERROR("reparent_root failed. real_size(%d)\n", tree->cache->ib->real_size);
                return -INDEX_ERR_REPARENT;
            }
        }
        else
        {
            ie = split_ib(tree, *new_ie);
            if (ie == NULL)
            {
                LOG_ERROR("split_ib failed. real_size(%d)\n", tree->cache->ib->real_size);
                return -INDEX_ERR_INSERT_ENTRY;
            }

            OS_FREE(*new_ie);
            *new_ie = ie;
        }
    }
}

int32_t check_removed_ib(object_handle_t * tree)
{
    int32_t ret = 0;
    index_entry_t *ie = GET_FIRST_IE(tree->cache->ib);
    
    if (0 == (ie->flags & INDEX_ENTRY_END))
    {
        return 0;
    }

    if (0 == tree->depth)
    { // root node
        return 0;
    }

    for (;;)
    {
        ret = OFS_FREE_BLOCK(tree->ct, tree->obj_info->objid, tree->cache->vbn);
        if (ret < 0)
        {
            LOG_ERROR("Free block failed. ret(%d)\n", ret);
            return ret;
        }

        //LOG_DEBUG("delete ct block success. vbn(%lld)\n", tree->cache->vbn);

        // this block will be deleted
        SET_CACHE_EMPTY(tree->cache);
        free_obj_cache(tree->obj_info, tree->cache);

        ret = pop_cache_stack(tree, 0);
        if (ret < 0)
        {       /* Get the parent block */
            LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            return ret;
        }

        ie = GET_FIRST_IE(tree->cache->ib);
        if ((ie->flags & INDEX_ENTRY_END) == 0)
        { // there are entries in this node
            break;
        }

        // there are no entries in this node
        if (0 == tree->depth)
        {   /* root node */
            make_ib_small(IB(tree->cache->ib));
            return set_ib_dirty(tree);
        }
    }

    return 1;
}

int32_t remove_leaf(object_handle_t *tree)
{
    index_entry_t *ie = NULL;
    index_entry_t *prev_ie = NULL;        /* The previous entry */
    bool_t is_end = FALSE;
    int32_t ret = 0;
    
    ASSERT(tree != NULL);

    remove_ie(IB(tree->cache->ib), tree->ie);
    ret = set_ib_dirty(tree);
    if (ret < 0)
    {
        LOG_ERROR("Set ct block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        return ret;
    }

    ret = check_removed_ib(tree);
    if (ret < 0)
    {
        LOG_ERROR("Check and remove node failed. ret(%d)\n", ret);
        return ret;
    }
    else if (ret == 0)
    { // it is root node, or the node have some entries
        return 0;
    }

    if ((tree->ie->flags & INDEX_ENTRY_END))
    {   /* It is the end key, change the ullVBN link and take out the entry */
        prev_ie = GET_PREV_IE(tree->ie);
        SET_IE_VBN(tree->ie, GET_IE_VBN(prev_ie));
        is_end = TRUE;    /* Set insert OS_S32o the block's last entry position */
    }
    else
    {   /* The parent key is not the end key, take out the entry */
        prev_ie = tree->ie;
        is_end = FALSE;    /* Set insert OS_S32o the block's first entry position */
    }

    ie = dump_ie_del_vbn(prev_ie);
    if (ie == NULL)
    {
        LOG_ERROR("dump_ie_del_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return -INDEX_ERR_DEL_VBN;
    }

    remove_ie(IB(tree->cache->ib), prev_ie);   /* Remove the entry */
    ret = set_ib_dirty(tree);
    if (ret < 0)
    {
        LOG_ERROR("Set ct block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(ie);
        return ret;
    }

    tree->position = (uint32_t)((uint8_t *)prev_ie - (uint8_t *) tree->cache->ib);  

    ret = get_current_ie(tree, is_end ? (INDEX_GET_LAST | INDEX_GET_LAST_ENTRY) : INDEX_GET_CURRENT);
    if (ret < 0)
    {
        LOG_ERROR("Index get current failed. ret(%d)\n", ret);
        OS_FREE(ie);
        return ret;
    }

    /* Insert the entry be taken out */
    ret = tree_insert_ie(tree, &ie);

    OS_FREE(ie);

    return ret;
}

int32_t remove_node(object_handle_t *tree)
{
    index_entry_t *succ_ie = NULL;        /* The successor entry */
    uint16_t len = 0;
    uint8_t depth = 0;
    uint64_t vbn = 0;
    int32_t ret = 0;

    ASSERT(tree != NULL);

    /* Record the current entry's information */
    depth = tree->depth;
    len = tree->ie->len;
    vbn = GET_IE_VBN(tree->ie);

    ret = walk_tree(tree, 0);
    if (ret < 0)
    {   /* Get the successor entry */
        LOG_ERROR("Get successor entry failed. ret(%d)\n", ret);
        return ret;
    }

    /* get the success entry, and add the vbn */
    succ_ie = dump_ie_add_vbn(tree->ie, vbn);
    if (succ_ie == NULL)
    {
        LOG_ERROR("dump_ie_add_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return -INDEX_ERR_ADD_VBN;
    }
    
    // recover the old node
    while (tree->depth > depth)
    {
        ret = pop_cache_stack(tree, 0);
        if (ret < 0)
        {
            LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            OS_FREE(succ_ie);
            return ret;
        }
    }

    // get the old entry
    tree->ie = (index_entry_t *)((uint8_t *)tree->ie - len);
    tree->position -= len;
    
    /* remove the old entry */
    remove_ie(IB(tree->cache->ib), tree->ie);
    ret = set_ib_dirty(tree);
    if (ret < 0)
    {
        LOG_ERROR("Set ct block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(succ_ie);
        return ret;
    }

    /* insert the new entry */
    ret = tree_insert_ie(tree, &succ_ie);
    if (ret < 0)
    {  
        LOG_ERROR("Insert entry failed. ret(%d)\n", ret);
        OS_FREE(succ_ie);
        return ret;
    }
    
    OS_FREE(succ_ie);

    /* get the next entry */
    ret = walk_tree(tree, 0);
    if (ret < 0)
    { 
        LOG_ERROR("Get successor entry failed. ret(%d)\n", ret);
        return ret;
    }
    
    /* remove the next entry */
    return (remove_leaf(tree)); 
}

int32_t tree_remove_ie(object_handle_t *tree)
{
    if (tree->ie->flags & (INDEX_ENTRY_END | INDEX_ENTRY_BEGIN))
    {
        LOG_ERROR("You can not remove begin or end entry. flags(0x%x)\n", tree->ie->flags);
        return -INDEX_ERR_END_ENTRY;
    }

    if (tree->ie->flags & INDEX_ENTRY_NODE)
    {
        return (remove_node(tree));
    }

    return (remove_leaf(tree));
}

int32_t index_remove_key_nolock(object_handle_t *tree, const void *key, uint16_t key_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    ASSERT(tree->obj_info->attr_record->flags & FLAG_TABLE);

    ret = search_key_internal(tree, key, key_len, NULL, 0);
    if (ret < 0)
    {
        LOG_ERROR("The key not found. ret(%d)\n", ret);
        return ret;
    }

    ret = tree_remove_ie(tree);
    if (ret < 0)
    {
        LOG_ERROR("Remove key failed. ret(%d)\n", ret);
        return ret;
    }
   
    return ret;
}

int32_t index_remove_key(object_handle_t *tree, const void *key, uint16_t key_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    ret = index_remove_key_nolock(tree, key, key_len);
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);

    return ret;
}

// value can be NULL, or value_len can be 0
int32_t index_insert_key_nolock(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    index_entry_t *ie = NULL;
    uint16_t len = 0;
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    ASSERT(tree->obj_info->attr_record->flags & FLAG_TABLE);

    ret = search_key_internal(tree, key, key_len, value, value_len);
    if (ret >= 0)
    {
        return -INDEX_ERR_KEY_EXIST;
    }
    
    if (ret != -INDEX_ERR_KEY_NOT_FOUND)
    {
        LOG_ERROR("Search key failed. objid(%lld) ret(%d)\n", tree->obj_info->objid, ret);
        return ret;
    }

    len = sizeof(index_entry_t) + key_len + value_len;

    ie = (index_entry_t *)OS_MALLOC(len);
    if (ie == NULL)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", len);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    ie->flags = 0;
    ie->len = len;
    ie->key_len = key_len;
    ie->value_len = value_len;
    memcpy(GET_IE_KEY(ie), key, key_len);
    if ((value != NULL) && (value_len != 0))
    {
        memcpy(GET_IE_VALUE(ie), value, value_len);
    }

    ret = tree_insert_ie(tree, &ie);
    if (ret < 0)
    {
        LOG_ERROR("%s", "The key insert failed.\n");
        OS_FREE(ie);
        return ret;
    }
    
    OS_FREE(ie);

    return ret;
}

int32_t index_insert_key(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    ret = index_insert_key_nolock(tree, key, key_len, value, value_len);
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);

    return ret;
}

int32_t index_update_value(object_handle_t *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    int32_t ret = 0;

    if ((tree == NULL) || (key == NULL) || (key_len == 0))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    ASSERT(tree->obj_info->attr_record->flags & FLAG_TABLE);

    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    ret = index_remove_key_nolock(tree, key, key_len);
    ret = index_insert_key_nolock(tree, key, key_len, value, value_len);
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);

    return ret;
}


int64_t index_get_total_key(object_handle_t *tree)
{
    int64_t cnt = 0;
    
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    if (0 == walk_tree(tree, INDEX_GET_FIRST))
    {
	    do
	    {
	    	cnt++;
	    } while (0 == walk_tree(tree, 0));
    }
    
    return cnt;
}

int64_t index_get_target_key(object_handle_t *tree, uint64_t target)
{
	int64_t cnt = 0;
	
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    if (0 == walk_tree(tree, INDEX_GET_FIRST))
    {
	    do
	    {
	    	cnt++;
	    	if (0 == --target)
	    	{
	    		break;
	    	}
	    } while (0 == walk_tree(tree, 0));
    }
    
    return cnt;
}

int32_t index_walk_all(object_handle_t *tree, bool_t reverse, uint8_t flags,
    void *para, tree_walk_cb_t cb)
{
    int32_t ret = 0;
    uint8_t if_flag = (FALSE == reverse) ? INDEX_GET_FIRST : INDEX_GET_LAST;
    uint8_t while_flag = (FALSE == reverse) ? 0 : INDEX_GET_PREV;

    if ((NULL == tree) || (NULL == cb))
    {
        LOG_ERROR("Invalid parameter. tree(%p) cb(%p)\n",
            tree, cb);
        return -INDEX_ERR_PARAMETER;
    }

    if_flag |= (flags & ~INDEX_WALK_MASK);
    while_flag |= (flags & ~INDEX_WALK_MASK);
    
    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    
    if (walk_tree(tree, if_flag) == 0)
    {
	    do
	    {
            ret = cb(tree, para);
            if (ret < 0)
            {
                LOG_ERROR("Call back failed. tree(%p) para(%p) ret(%d)\n", tree, para, ret);
                OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);
                return ret;
            }
	    } while (walk_tree(tree, while_flag) == 0);
    }
    
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);
    
    return 0;
}

EXPORT_SYMBOL(index_search_key);
EXPORT_SYMBOL(walk_tree);
EXPORT_SYMBOL(index_insert_key);
EXPORT_SYMBOL(index_remove_key);
EXPORT_SYMBOL(index_walk_all);

EXPORT_SYMBOL(index_search_key_nolock);
EXPORT_SYMBOL(index_insert_key_nolock);
EXPORT_SYMBOL(index_remove_key_nolock);


