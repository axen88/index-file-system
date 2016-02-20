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

            版权所有(C), 2011~2014, AXEN工作室
********************************************************************************
文 件 名: OS_INDEX_CORE.C
版    本: 1.00
日    期: 2011年5月8日
功能描述: 索引操作算法、事务机制等核心代码
函数列表:
    1. ...:
修改历史:
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年5月8日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

MODULE(PID_INDEX);
#include "os_log.h"

#define INDEX_GETTO_BEGIN  1

#if 0

#define PRINT_KEY(comment, tree, pcKey, keyLen) \
{ \
    char str[64]; \
    char *pStr = str; \
    int32_t i = 0; \
    for (i = 0; i < keyLen; i++) \
    { \
        OS_SNPRINTF(pStr, 64, "%02X", ((uint8_t *)pcKey)[i]); \
        pStr += 2; \
    } \
    LOG_ERROR("%s: %s, name: %s\n", comment, str, tree->pstInode->name); \
}

#else

#define PRINT_KEY(comment, tree, pcKey, keyLen)

#endif

/* 设置当前块一直到根节点块cache数据为脏数据 */
static int32_t set_ib_dirty(ATTR_HANDLE *tree, uint64_t vbn, uint8_t depth)
{
    uint64_t new_vbn = 0;
    int32_t ret = 0;
    INDEX_ENTRY *ie = NULL;

    ASSERT(NULL != tree);
    ASSERT(depth < INDEX_MAX_DEPTH);

    do
    {
        if ((0 != depth) && (DIRTY != tree->cache_stack[depth]->state))
        {   /* 分配新块以便写修改后的数据 */
            ret = INDEX_ALLOC_BLOCK(tree->attr_info->obj, &new_vbn);
            if (0 > ret)
            {
                LOG_ERROR("Allocate new block failed. ret(%d)\n", ret);
                return ret;
            }
            
            /* 将原来的块放入旧块队列以便事务处理 */
            ret = index_record_old_block(tree->attr_info, tree->cache_stack[depth]->vbn);
            
            OS_RWLOCK_WRLOCK(&tree->attr_info->obj->caches_lock);
            avl_remove(&tree->attr_info->attr_caches, tree->cache_stack[depth]);
            avl_remove(&tree->attr_info->obj->obj_caches, tree->cache_stack[depth]);
            tree->cache_stack[depth]->vbn = new_vbn;
            avl_add(&tree->attr_info->obj->obj_caches, tree->cache_stack[depth]);
            avl_add(&tree->attr_info->attr_caches, tree->cache_stack[depth]);
            OS_RWLOCK_WRUNLOCK(&tree->attr_info->obj->caches_lock);
            
            if (0 > ret)
            {
                LOG_ERROR("Record old block failed. ret(%d)\n", ret);
                return ret;
            }
        }

        if (0 != vbn)
        {
            /* 修改entry指向的索引块地址 */
            ie = (INDEX_ENTRY *) ((uint8_t *) tree->cache_stack[depth]->ib
                + tree->position_stack[depth]);
            IESetVBN(ie, vbn);
        }

        /* 已经修改过的节点之上的节点都不需要再进行修改 */
        if (DIRTY == tree->cache_stack[depth]->state)
        {
            return 0;
        }

        tree->cache_stack[depth]->state = DIRTY;
        vbn = new_vbn;
    } while (depth-- > 0);

    return 0;
}

static void get_last_ie(ATTR_HANDLE * tree)
{
    uint32_t last_ie_len = ENTRY_END_SIZE;
    
    ASSERT(NULL != tree);

    if (tree->cache->ib->node_type & INDEX_BLOCK_LARGE)
    {
        last_ie_len += VBN_SIZE;
    }

    tree->ie = (INDEX_ENTRY *)(IBGetEnd(tree->cache->ib) - last_ie_len);
    tree->position = tree->cache->ib->head.real_size - last_ie_len;

    return;
}

static void reset_cache_stack(ATTR_HANDLE * tree, uint8_t flags)
{
    ASSERT(NULL != tree);
    
    /* 使当前指针指向根节点的第一个entry */
    tree->cache = &tree->attr_info->root_ibc;
    tree->cache_stack[0] = tree->cache;
    tree->depth = 0;

    if (flags & INDEX_GET_LAST)
    {
        get_last_ie(tree);
        return;
    }

    tree->ie = IBGetFirst(tree->cache->ib);
    tree->position = tree->cache->ib->first_entry_off;

    return;
}   

/*******************************************************************************
树往下深入一级
*******************************************************************************/
static int32_t push_cache_stack(ATTR_HANDLE *tree, uint8_t flags)
{
    uint64_t vbn = 0;
    int32_t ret = 0;

    ASSERT(NULL != tree);
    
    /* 判断是否超出了最大深度 */
    if (tree->depth >= (INDEX_MAX_DEPTH - 1))
    {
        LOG_ERROR("Depth get to MAX. depth(%d)\n", tree->depth);
        return -INDEX_ERR_MAX_DEPTH;
    }

    /* 获取下一级节点的vbn */
    vbn = IEGetVBN(tree->ie);

    LOG_DEBUG("Depth increase. depth(%d) vbn(%lld) pos(%d)\n", tree->depth, vbn, tree->position);

    /* 读取当前entry所指向的节点 */
    ret = index_block_read(tree, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Read index block failed. vbn(%lld) ret(%d)\n", vbn, ret);
        return ret;
    }

	tree->position_stack[tree->depth] = tree->position;
    tree->depth++;
    tree->cache_stack[tree->depth] = tree->cache;

    if (((tree->cache->ib->node_type & INDEX_BLOCK_LARGE) == 0)
        && (tree->max_depth != tree->depth))
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

    /* 指向新节点的头部 */
    tree->position = tree->cache->ib->first_entry_off;
    tree->ie = IBGetFirst(tree->cache->ib);

    return 0;
}

/*******************************************************************************
树的当前指针指向前一个entry
*******************************************************************************/
static int32_t get_prev_ie(ATTR_HANDLE *tree)
{
    ASSERT(NULL != tree);
    
    tree->position -= tree->ie->prev_len;
    tree->ie = IEGetPrev(tree->ie);
    if ((uint8_t *) tree->ie < (uint8_t *) &tree->cache->ib->begin_entry)
    {   /* Check valid */
        LOG_ERROR("The ie is invalid. ie(%p) begin_entry(%p)\n",
            tree->ie, &tree->cache->ib->begin_entry);
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

/*******************************************************************************
树往上回退一级
*******************************************************************************/
static int32_t pop_cache_stack(ATTR_HANDLE *tree, uint8_t flags)
{
    ASSERT(NULL != tree);
    
    if (0 == tree->depth)
    {   /* Get to root */
        LOG_DEBUG("Depth get to root. depth(%d)\n", tree->depth);
        return -INDEX_ERR_ROOT;
    }

    tree->depth--;
    tree->cache = tree->cache_stack[tree->depth];
	tree->position = tree->position_stack[tree->depth];

    LOG_DEBUG("Depth decrease. depth(%d) vbn(%lld) pos(%d)\n", tree->depth,
        tree->cache->vbn, tree->position);

    /* 恢复上层节点的相关信息 */
    tree->ie = (INDEX_ENTRY *) ((uint8_t *) tree->cache->ib
        + tree->position);

    if (flags & INDEX_GET_PREV)
    {
        return get_prev_ie(tree);
    }

    return 0;
}      

void init_ib(INDEX_BLOCK *ib, uint8_t node_type,
    uint32_t aloc_size)
{
    INDEX_ENTRY *ie = NULL;

    ASSERT(NULL != ib);
    
    memset(ib, 0, aloc_size);
    
    ib->head.blk_id = INDEX_MAGIC;
    ib->head.alloc_size = aloc_size;
    
    ib->first_entry_off = sizeof(INDEX_BLOCK);
    if (node_type & INDEX_BLOCK_LARGE)
    {   /* 有子节点 */
        ib->head.real_size = sizeof(INDEX_BLOCK) + ENTRY_END_SIZE + VBN_SIZE;
    }
    else
    {   /* 无子节点 */
        ib->head.real_size = sizeof(INDEX_BLOCK) + ENTRY_END_SIZE;
    }

    ib->node_type = node_type;

    ie = IBGetFirst(ib);

    if (node_type & INDEX_BLOCK_LARGE)
    {   /* 有子节点 */
        ie->flags = INDEX_ENTRY_END | INDEX_ENTRY_NODE;
        ie->len = ENTRY_END_SIZE + VBN_SIZE;
    }
    else
    {   /* 无子节点 */
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

/*******************************************************************************
将指定索引块变成不带子树的索引块
*******************************************************************************/
static void make_ib_small(INDEX_BLOCK *ib)
{
    INDEX_ENTRY *ie = NULL;

    ASSERT(NULL != ib);
    
    ib->head.real_size = sizeof(INDEX_BLOCK) + ENTRY_END_SIZE;
    ib->node_type = INDEX_BLOCK_SMALL;
    
    ie = IBGetFirst(ib);
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

/*******************************************************************************
获取树的下一个entry
*******************************************************************************/
static int32_t get_next_ie(ATTR_HANDLE *tree)
{
    ASSERT(NULL != tree);

    if (0 != (tree->ie->flags & INDEX_ENTRY_END))
    { /* 正常结束的entry */
        return -INDEX_ERR_NEXT_ENTRY;
    }
    
    tree->position += tree->ie->len;
    tree->ie = IEGetNext(tree->ie);
    if (((uint8_t *) tree->ie >= IBGetEnd(tree->cache->ib))
        || (0 == tree->ie->len))
    {   /* Check valid */
        LOG_ERROR("The ie is invalid. len(%d) ib real_size(%d)\n",
            tree->ie->len, tree->cache->ib->head.real_size);
        return -INDEX_ERR_FORMAT;
    }

    return 0;
}    

static int32_t add_or_remove_ib(ATTR_HANDLE *tree, uint8_t flags)
{
    int32_t ret = 0;

    ASSERT(NULL != tree);
        
    if (flags & INDEX_REMOVE_BLOCK)
    {
        ret = index_record_old_block(tree->attr_info, tree->cache->vbn);
        if (0 > ret)
        {
            LOG_ERROR("Record old block failed. vbn(%lld) ret(%d)\n", tree->cache->vbn, ret);
            return ret;
        }
    }
    else if (flags & INDEX_ADD_BLOCK)
    {
        ret = block_set_status(tree->attr_info->obj->index->hnd, tree->cache->vbn,
            1, B_TRUE);
        if (0 > ret)
        {
            LOG_ERROR("Set block status failed. vbn(%lld) ret(%d)\n", tree->cache->vbn, ret);
            return ret;
        }
    }

    return 0;
}
    
/*******************************************************************************
将entry指针指向当前key
*******************************************************************************/
static int32_t get_current_ie(ATTR_HANDLE *tree, uint8_t flags)
{
    int32_t ret = 0;

    while (tree->ie->flags & INDEX_ENTRY_NODE)
    {   /* Have children */
        ret = push_cache_stack(tree, flags);
        if (ret < 0)
        {       /* Push the information OS_S32o the history, and read new index block */
            LOG_ERROR("Go to child node failed. ret(%d)\n", ret);
            return ret;
        }
    }

    while (tree->ie->flags & (INDEX_ENTRY_END | INDEX_ENTRY_BEGIN))
    {   /* The Index END */
        ret = add_or_remove_ib(tree, flags);
        if (0 > ret)
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
            if (0 > ret)
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
        {       /* Up to parent failed */
            if (ret != -INDEX_ERR_ROOT)
            {
                LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            }

            return ret;
        }
    }

    return 0;
}

int32_t walk_tree(ATTR_HANDLE *tree, uint8_t flags)
{
    int32_t ret = 0;

    if (NULL == tree)
    {   /* Not allocated yet */
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    if ((tree->attr_info->attr_record.attr_flags & FLAG_TABLE) == 0)
    {
        LOG_ERROR("The attr is resident. obj_name(%s)\n", tree->attr_info->obj->obj_name);
        return -INDEX_ERR_ATTR_RESIDENT;
    }

    if (flags & (INDEX_GET_FIRST | INDEX_GET_LAST))
    {   /* Get to the root's first entry */
        reset_cache_stack(tree, flags);
    }
    else if (flags & INDEX_GET_PREV)
    {  /* 获取上一个 */
        if (!(tree->ie->flags & INDEX_ENTRY_NODE))
        {
            ret = get_prev_ie(tree);
            if (0 > ret)
            {
                LOG_ERROR("Get prev entry failed. ret(%d)\n", ret);
                return ret;
            }
        }
    }
    else if (!(flags & INDEX_GET_CURRENT))
    {  /* 获取下一个 */
        ret = get_next_ie(tree);
        if (0 > ret)
        {
            LOG_ERROR("Get next entry failed. ret(%d)\n", ret);
            return ret;
        }
    }
    /* 不需要处理else */

    return get_current_ie(tree, flags);
}

/*******************************************************************************
比较key
*******************************************************************************/
int32_t collate_key(uint16_t collate_rule, INDEX_ENTRY *ie,
    const void *key, uint16_t key_len)
{
    ASSERT(COLLATE_UNICODE_STRING >= (collate_rule & COLLATE_RULE_MASK2));
    ASSERT(NULL != ie);
    ASSERT(NULL != key);
    ASSERT(0 != key_len);
    
    switch (collate_rule & COLLATE_RULE_MASK2)
    {
        case COLLATE_BINARY:
            return os_collate_binary((uint8_t *) IEGetKey(ie), ie->key_len,
                (uint8_t *) key, key_len);

        case COLLATE_ANSI_STRING:
            return os_collate_ansi_string((char *) IEGetKey(ie),
                ie->key_len, (char *) key, key_len);

        case COLLATE_UNICODE_STRING:
            return os_collate_unicode_string((UNICODE_CHAR *) IEGetKey(ie),
                ie->key_len, (UNICODE_CHAR *) key, key_len);

        default:
            break;
    }

    LOG_ERROR("Collate rule is invalid. collate_rule(%d)\n", collate_rule);
    
    return -INDEX_ERR_COLLATE;
}

/*******************************************************************************
搜索指定key，无加锁
*******************************************************************************/
int32_t search_key_internal(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    ASSERT(NULL != tree);
    ASSERT(NULL != key);

    if (0 == key_len)
    {
        LOG_ERROR("Invalid parameter. tree(%p) key_len(0)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    reset_cache_stack(tree, 0);

    for (;;)
    {
        while (0 == (tree->ie->flags & INDEX_ENTRY_END))
        {       /* It is not the Index END */
            ret = collate_key(tree->attr_info->attr_record.attr_flags, tree->ie,
                key, key_len);
            if (0 < ret)
            {   /* key比要找的key大 */
                break;
            }
            
            if (0 == ret)
            {   /* 已经找到了 */
                return 0;
            }

            if (-INDEX_ERR_COLLATE == ret)
            {
                return ret;
            }
            
            /* key比要找的key小 */
            ret = get_next_ie(tree);
            if (0 > ret)
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
        if (0 > ret)
        {   /* Push the information OS_S32o the history, and read new index block */
            LOG_ERROR("Get child node failed. ret(%d)\n", ret);
            return ret;
        }
    }

    return -INDEX_ERR_KEY_NOT_FOUND;
}

/*******************************************************************************
将树key指针指向离当前位置最近的key
*******************************************************************************/
static void get_to_near_key(ATTR_HANDLE *tree)
{
    int32_t ret = 0;

    ASSERT(NULL != tree);

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

/*******************************************************************************
搜索指定key，无加锁
*******************************************************************************/
int32_t index_search_key_nolock(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == tree) || (NULL == key) || (0 == key_len))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key,
            key_len);
        return -INDEX_ERR_PARAMETER;
    }

    if ((tree->attr_info->attr_record.attr_flags & FLAG_TABLE) == 0)
    {
        LOG_ERROR("The attr is resident. obj_name(%s)\n", tree->attr_info->obj->obj_name);
        return -INDEX_ERR_ATTR_RESIDENT;
    }

    ret = search_key_internal(tree, key, key_len);
    if (-INDEX_ERR_KEY_NOT_FOUND == ret)
    {
        get_to_near_key(tree);
    }

    return ret;
}

/*******************************************************************************
搜索指定key，有加锁
*******************************************************************************/
int32_t index_search_key(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == tree) || (NULL == key) || (0 == key_len))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&tree->attr_info->attr_lock);
    ret = index_search_key_nolock(tree, key, key_len);
    OS_RWLOCK_WRUNLOCK(&tree->attr_info->attr_lock);

    return ret;
}

/*******************************************************************************
获取索引块的中间entry
*******************************************************************************/
static INDEX_ENTRY *get_middle_ie(INDEX_BLOCK *ib)
{
    INDEX_ENTRY *ie = NULL;
    uint32_t uiMidPos = 0;

    ASSERT(NULL != ib);
    
    uiMidPos = (ib->head.real_size - sizeof(INDEX_BLOCK)) >> 1;
    ie = IBGetFirst(ib);
    while (!(ie->flags & INDEX_ENTRY_END))
    {
        if (uiMidPos > ie->len)
        {
            uiMidPos -= ie->len;
            ie = IEGetNext(ie);
        }
        else
        {
            break;
        }
    }

    return ie;
}  

/*******************************************************************************
计算指定entry到结束的长度
*******************************************************************************/
uint32_t get_entries_length(INDEX_ENTRY *ie)
{
    uint32_t len = 0;

    ASSERT(NULL != ie);
    
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
        ie = IEGetNext(ie);
    }

    return len;
}      

/*******************************************************************************
获取指定索引块的最后一个entry
*******************************************************************************/
static INDEX_ENTRY *ib_get_last_ie(INDEX_BLOCK *ib)
{
    uint32_t last_ie_len = ENTRY_END_SIZE;

    ASSERT(NULL != ib);
    
    if (ib->node_type & INDEX_BLOCK_LARGE)
    {
        last_ie_len += VBN_SIZE;
    }

    return (INDEX_ENTRY *)(IBGetEnd(ib) - last_ie_len);
}

/*******************************************************************************
删除索引块中指定的entry
*******************************************************************************/
static void remove_ie(INDEX_BLOCK * ib, INDEX_ENTRY * ie)
{
    INDEX_ENTRY *next_ie = NULL;

    ASSERT(NULL != ib);
    ASSERT(NULL != ie);
    
    ib->head.real_size -= ie->len;
    next_ie = IEGetNext(ie);
    next_ie->prev_len = ie->prev_len;
    memcpy(ie, next_ie, get_entries_length(next_ie));

    return;
}

/*******************************************************************************
在索引块中插入指定的entry
*******************************************************************************/
void insert_ie(INDEX_BLOCK * ib, INDEX_ENTRY * ie,
    INDEX_ENTRY * pos)
{
    ASSERT(NULL != ib);
    ASSERT(NULL != ie);
    ASSERT(NULL != pos);
    
    ib->head.real_size += ie->len;

    ie->prev_len = pos->prev_len;
    pos->prev_len = ie->len;

    memmove((uint8_t *) pos + ie->len, pos,
        get_entries_length(pos));
    memcpy(pos, ie, ie->len);

    return;
}

/*******************************************************************************
将指定entry加上vbn字段，生成一个新的entry
*******************************************************************************/
static INDEX_ENTRY *dump_ie_add_vbn(INDEX_ENTRY * ie, uint64_t vbn)
{
    INDEX_ENTRY *new_ie = NULL;
    uint16_t size = 0;
    
    ASSERT(NULL != ie);

    size = ie->len;
    if (!(ie->flags & INDEX_ENTRY_NODE))
    {   /* The old @pstIE is not NODE (without ullVBN) */
        size += VBN_SIZE;
    }

    new_ie = (INDEX_ENTRY *) OS_MALLOC(size);
    if (NULL == new_ie)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", size);
        return NULL;
    }

    memcpy(new_ie, ie, ie->len);
    new_ie->len = size;
    new_ie->flags |= INDEX_ENTRY_NODE;
    IESetVBN(new_ie, vbn);

    return new_ie;
}  

/*******************************************************************************
将指定entry删除vbn字段，生成一个新的entry
*******************************************************************************/
static INDEX_ENTRY *dump_ie_del_vbn(INDEX_ENTRY * ie)
{
    INDEX_ENTRY *new_ie = NULL;
    uint16_t size = 0;
    
    ASSERT(NULL != ie);

    size = ie->len;
    if (ie->flags & INDEX_ENTRY_NODE)
    {   /* The old is NODE (with ullVBN) */
        size -= VBN_SIZE;
    }

    new_ie = (INDEX_ENTRY *) OS_MALLOC(size);
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

/*******************************************************************************
将源索引块中从指定位置到结束位置的所有entry拷贝到目标索引块中
*******************************************************************************/
void copy_ib_tail(INDEX_BLOCK * dst_ib, INDEX_BLOCK * src_ib,
    INDEX_ENTRY * mid_ie)
{
    uint32_t tail_size = 0;

    ASSERT(NULL != dst_ib);
    ASSERT(NULL != src_ib);
    ASSERT(NULL != mid_ie);
    
    init_ib(dst_ib, src_ib->node_type, src_ib->head.alloc_size);
    mid_ie = IEGetNext(mid_ie);

    mid_ie->prev_len = ENTRY_BEGIN_SIZE;

    tail_size = (uint32_t)((uint8_t *) IBGetEnd(src_ib) - (uint8_t *) mid_ie);
    memcpy(IBGetFirst(dst_ib), mid_ie, tail_size);

    dst_ib->head.real_size = tail_size + dst_ib->first_entry_off;

    return;
}

/*******************************************************************************
将源索引块中从指定位置到结束位置的所有entry都删除
*******************************************************************************/
static void cut_ib_tail(INDEX_BLOCK *src_ib, INDEX_ENTRY *ie)
{
    uint8_t *start = NULL;
    INDEX_ENTRY *last_ie = NULL;
    INDEX_ENTRY *prev_ie = NULL;

    ASSERT(NULL != src_ib);
    ASSERT(NULL != ie);
    
    start = (uint8_t *)IBGetFirst(src_ib);

    last_ie = ib_get_last_ie(src_ib);
    if (last_ie->flags & INDEX_ENTRY_NODE)
    {
        IESetVBN(last_ie, IEGetVBN(ie));
    }

    prev_ie = IEGetPrev(ie);
    last_ie->prev_len = prev_ie->len;

    memcpy(ie, last_ie, last_ie->len);
    src_ib->head.real_size = ie->len + (uint32_t)((uint8_t *) ie - start)
        + src_ib->first_entry_off;
}

/*******************************************************************************
将当前索引块分裂成2个，并将其中间entry提出来，然后将指定entry插入
其中一个索引块
*******************************************************************************/
static INDEX_ENTRY *split_ib(ATTR_HANDLE *tree, INDEX_ENTRY *ie)
{
    INDEX_ENTRY *mid_ie = NULL;
    INDEX_ENTRY *new_ie = NULL;
    INDEX_BLOCK *new_ib = NULL;
    INDEX_BLOCK_CACHE *new_ibc = NULL;
    int32_t pos = 0;         /* Insert iOffset to the new indexHeader */
    int32_t ret = 0;
    
    ASSERT(NULL != tree);
    ASSERT(NULL != ie);

    mid_ie = get_middle_ie(tree->cache->ib);

    ret = index_alloc_cache_and_block(tree->attr_info, &new_ibc);
    if (0 > ret)
    {
        LOG_ERROR("Allocate cache failed.\n");
        return NULL;
    }
    
    new_ib = new_ibc->ib;

    copy_ib_tail(new_ib, tree->cache->ib, mid_ie);

    pos = (int32_t)((uint8_t *) mid_ie - (uint8_t *) tree->ie);
    if (pos < 0)
    {   /* Insert the entry OS_S32o newIB */
        insert_ie(new_ib, ie,
            (INDEX_ENTRY *) (((uint8_t *) IBGetFirst(new_ib) - pos)
                - mid_ie->len));
    }

    new_ibc->state = DIRTY;

    //LOG_DEBUG("Write new index block success. vbn(%lld)\n", new_ibc->vbn);

    /* 先设置成脏，因为上层块链接会变化 */
    ret = set_ib_dirty(tree, (uint64_t)0, tree->depth);
    if (0 > ret)
    {
        LOG_ERROR("Set index block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        return NULL;
    }

    /* */
    /* Cut block tail and whether insert the @pstIE OS_S32o the old index block */
    /* */
    new_ie = dump_ie_add_vbn(mid_ie, tree->cache->vbn);
    if (NULL == new_ie)
    {
        LOG_ERROR("dump_ie_add_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return NULL;
    }

    cut_ib_tail(tree->cache->ib, mid_ie);
    if (pos >= 0)
    {   /* Insert the entry OS_S32o old index block */
        insert_ie(tree->cache->ib, ie, tree->ie);
    }

    if (pop_cache_stack(tree, 0) < 0)
    {
        LOG_ERROR("Go to parent node failed. vbn(%lld)\n", tree->cache->vbn);
        OS_FREE(new_ie);
        return NULL;
    }

    IESetVBN(tree->ie, new_ibc->vbn);     /* Change the link */

    return new_ie;
}

/*******************************************************************************
将根节点中的entry挪到新的节点中来，从而使根节点所在vbn不变
*******************************************************************************/
static int32_t reparent_root(ATTR_HANDLE * tree)
{
    INDEX_ENTRY *ie = NULL;
    INDEX_BLOCK *new_ib = NULL;
    INDEX_BLOCK_CACHE *new_ibc = NULL;
    INDEX_BLOCK *old_ib = NULL;
    uint32_t alloc_size = 0;
    int32_t ret = 0;

    ASSERT(NULL != tree);
    ASSERT(NULL != tree->cache);
    ASSERT(NULL != tree->cache->ib);

    if (tree->max_depth >= (INDEX_MAX_DEPTH - 1))
    {   /* Get to max ucDepth */
        LOG_ERROR("Depth get to MAX. depth(%d)\n", tree->max_depth);
        return -INDEX_ERR_MAX_DEPTH;
    }

    old_ib = tree->cache->ib;
    alloc_size = old_ib->head.alloc_size;
    
    ret = index_alloc_cache_and_block(tree->attr_info, &new_ibc);
    if (0 > ret)
    {
        LOG_ERROR("Allocate cache failed.\n");
        return ret;
    } 

    new_ib = new_ibc->ib;

    memcpy(new_ib, old_ib, old_ib->head.real_size);
    new_ib->head.alloc_size = tree->attr_info->obj->index->hnd->sb.block_size;

    new_ibc->state = DIRTY;

    //LOG_DEBUG("Write new index block success. vbn(%lld)\n", new_ibc->vbn);

    init_ib(old_ib, INDEX_BLOCK_LARGE, alloc_size);
    ie = IBGetFirst(old_ib);
    IESetVBN(ie, new_ibc->vbn);
    
    ret = set_ib_dirty(tree, (uint64_t)0, tree->depth);
    if (0 > ret)
    {
        LOG_ERROR("Set index block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(new_ib);
        return ret;
    }
    
	tree->position_stack[tree->depth] = tree->cache->ib->first_entry_off;
    tree->depth++;
    tree->cache_stack[tree->depth] = new_ibc;
	tree->position_stack[tree->depth] = tree->position;

	tree->cache = new_ibc;
    tree->ie = (INDEX_ENTRY *)((uint8_t *)new_ib + tree->position);

    return 0;
}    

/*******************************************************************************
在树的当前位置插入指定entry
*******************************************************************************/
static int32_t tree_insert_ie(ATTR_HANDLE *tree, INDEX_ENTRY **new_ie)
{
    uint32_t uiNewSize = 0;
    INDEX_ENTRY *ie = NULL;
    
    ASSERT(NULL != tree);
    ASSERT(NULL != new_ie);
    ASSERT(NULL != *new_ie);

    ie = *new_ie;
    
    for (;;)
    {   /* The entry can't be inserted */
        uiNewSize = tree->cache->ib->head.real_size + ie->len;
        if (uiNewSize <= tree->cache->ib->head.alloc_size)
        {
            insert_ie(tree->cache->ib, ie, tree->ie);       /* Insert the entry before current entry */
            return set_ib_dirty(tree, (uint64_t)0, tree->depth);
        }

        if (0 == tree->depth)
        {       /* Current the $INDEX_ROOT opened */
            if (reparent_root(tree) < 0)
            {
                LOG_ERROR("reparent_root failed. real_size(%d)\n", tree->cache->ib->head.real_size);
                return -INDEX_ERR_REPARENT;
            }
        }
        else
        {
            ie = split_ib(tree, *new_ie);
            if (NULL == ie)
            {
                LOG_ERROR("split_ib failed. real_size(%d)\n", tree->cache->ib->head.real_size);
                return -INDEX_ERR_INSERT_ENTRY;
            }

            OS_FREE(*new_ie);
            *new_ie = ie;
        }
    }
}

/*******************************************************************************
检查是否需要删除索引块
*******************************************************************************/
int32_t check_removed_ib(ATTR_HANDLE * tree)
{
    int32_t ret = 0;
    INDEX_ENTRY *ie = IBGetFirst(tree->cache->ib);
    
    if (0 == (ie->flags & INDEX_ENTRY_END))
    { /* 在当前节点中还有entry */
        return 0;
    }

    /* 在当前节点中已经没有了entry */
    if (0 == tree->depth)
    {   /* 根节点 */
        return 0;
    }

    for (;;)
    {
        ret = index_record_old_block(tree->attr_info, tree->cache->vbn);
        if (0 > ret)
        {
            LOG_ERROR("Record old block failed. ret(%d)\n", ret);
            return ret;
        }

        //LOG_DEBUG("delete index block success. vbn(%lld)\n", tree->cache->vbn);

        /* 清除IB的dirty标志，因为此块已经被删除，数据无效，不需要更新此块 */
        tree->cache->state = EMPTY;

        ret = pop_cache_stack(tree, 0);
        if (0 > ret)
        {       /* Get the parent block */
            LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            return ret;
        }

        ie = IBGetFirst(tree->cache->ib);
        if ((ie->flags & INDEX_ENTRY_END) == 0)
        { /* 在节点中有entry */
            break;
        }

        /* 在父节点中没有任何entry */
        if (0 == tree->depth)
        {   /* 是根节点 */
            /* cache 缩减 */
            make_ib_small(tree->cache->ib);
            return set_ib_dirty(tree, (uint64_t)0, tree->depth);
        }
    }

    return 1;
}

/*******************************************************************************
删除一个叶子entry
*******************************************************************************/
int32_t remove_leaf(ATTR_HANDLE *tree)
{
    INDEX_ENTRY *ie = NULL;
    INDEX_ENTRY *prev_ie = NULL;        /* The previous entry */
    bool_t is_end = B_FALSE;
    int32_t ret = 0;
    
    ASSERT(NULL != tree);

    remove_ie(tree->cache->ib, tree->ie);
    ret = set_ib_dirty(tree, (uint64_t)0, tree->depth);
    if (0 > ret)
    {
        LOG_ERROR("Set index block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        return ret;
    }

    ret = check_removed_ib(tree);
    if (ret < 0)
    { /* 错误 */
        LOG_ERROR("Check and remove node failed. ret(%d)\n", ret);
        return ret;
    }
    else if (0 == ret)
    { /* 根节点或被删entry所在节点的entry数目不为0 */
        return 0;
    }

    if ((tree->ie->flags & INDEX_ENTRY_END))
    {   /* It is the end key, change the ullVBN link and take out the entry */
        prev_ie = IEGetPrev(tree->ie);
        IESetVBN(tree->ie, IEGetVBN(prev_ie));
        is_end = B_TRUE;    /* Set insert OS_S32o the block's last entry position */
    }
    else
    {   /* The parent key is not the end key, take out the entry */
        prev_ie = tree->ie;
        is_end = B_FALSE;    /* Set insert OS_S32o the block's first entry position */
    }

    ie = dump_ie_del_vbn(prev_ie);
    if (NULL == ie)
    {
        LOG_ERROR("dump_ie_del_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return -INDEX_ERR_DEL_VBN;
    }

    remove_ie(tree->cache->ib, prev_ie);   /* Remove the entry */
    ret = set_ib_dirty(tree, (uint64_t)0, tree->depth);
    if (0 > ret)
    {
        LOG_ERROR("Set index block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(ie);
        return ret;
    }

    tree->position = (uint32_t)((uint8_t *)prev_ie
        - (uint8_t *) tree->cache->ib);  

    ret = get_current_ie(tree, is_end
        ? (INDEX_GET_LAST | INDEX_GET_LAST_ENTRY) : INDEX_GET_CURRENT);
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

/*******************************************************************************
删除一个node，也就是带子树的entry
*******************************************************************************/
int32_t remove_node(ATTR_HANDLE *tree)
{
    INDEX_ENTRY *succ_ie = NULL;        /* The successor entry */
    uint16_t len = 0;
    uint8_t depth = 0;
    uint64_t vbn = 0;
    int32_t ret = 0;

    ASSERT(NULL != tree);

    /* */
    /* Record the current entry's information */
    /* */
    depth = tree->depth;
    len = tree->ie->len;
    vbn = IEGetVBN(tree->ie);

    ret = walk_tree(tree, 0);
    if (0 > ret)
    {   /* Get the successor entry */
        LOG_ERROR("Get successor entry failed. ret(%d)\n", ret);
        return ret;
    }

    /* 获取紧接着的entry，并在后面加上vbn */
    succ_ie = dump_ie_add_vbn(tree->ie, vbn);
    if (NULL == succ_ie)
    {
        LOG_ERROR("dump_ie_add_vbn failed. vbn(%lld)\n", tree->cache->vbn);
        return -INDEX_ERR_ADD_VBN;
    }
    
    /* 恢复成原来的节点 */
    while (tree->depth > depth)
    {
        ret = pop_cache_stack(tree, 0);
        if (0 > ret)
        {
            LOG_ERROR("Go to parent node failed. ret(%d)\n", ret);
            OS_FREE(succ_ie);
            return ret;
        }
    }

    /* 获取前一个entry的信息 */
    tree->ie = (INDEX_ENTRY *) ((uint8_t *) tree->ie - len);
    tree->position -= len;
    
    /* 删除旧entry */
    remove_ie(tree->cache->ib, tree->ie);
    ret = set_ib_dirty(tree, (uint64_t)0, tree->depth);
    if (0 > ret)
    {
        LOG_ERROR("Set index block dirty failed. tree(%p) ret(%d)\n", tree, ret);
        OS_FREE(succ_ie);
        return ret;
    }

    /* 插入新entry */
    ret = tree_insert_ie(tree, &succ_ie);
    if (0 > ret)
    {  
        LOG_ERROR("Insert entry failed. ret(%d)\n", ret);
        OS_FREE(succ_ie);
        return ret;
    }
    
    OS_FREE(succ_ie);

    /* 获取紧接着的entry */
    ret = walk_tree(tree, 0);
    if (0 > ret)
    { 
        LOG_ERROR("Get successor entry failed. ret(%d)\n", ret);
        return ret;
    }
    
    /* 删除紧接着的entry */
    return (remove_leaf(tree)); 
}

/*******************************************************************************
删除树当前指向的entry
*******************************************************************************/
int32_t tree_remove_ie(ATTR_HANDLE *tree)
{
    /* 检查输入参数 */
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    if (tree->ie->flags & (INDEX_ENTRY_END | INDEX_ENTRY_BEGIN))
    {   /* 要删除的entry是begin或end entry */
        LOG_ERROR("You can not remove begin or end entry. flags(0x%x)\n", tree->ie->flags);
        return -INDEX_ERR_END_ENTRY;
    }

    if (tree->ie->flags & INDEX_ENTRY_NODE)
    { /* 删除带子树的entry */
        return (remove_node(tree));
    }

    /* 删除不带子树的entry */
    return (remove_leaf(tree));
}

/*******************************************************************************
删除指定key，无锁
*******************************************************************************/
int32_t index_remove_key_nolock(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == tree) || (NULL == key) || (0 == key_len))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) key_len(%d)\n", tree, key, key_len);
        return -INDEX_ERR_PARAMETER;
    }

    PRINT_KEY("Remove key start", tree, key, key_len);

    if ((tree->attr_info->attr_record.attr_flags & FLAG_TABLE) == 0)
    {
        LOG_ERROR("The attr is resident. obj_name(%s)\n", tree->attr_info->obj->obj_name);
        return -INDEX_ERR_ATTR_RESIDENT;
    }

    /* 搜索是否有此key */
    ret = search_key_internal(tree, key, key_len);
    if (0 > ret)
    {
        LOG_ERROR("The key not found. ret(%d)\n", ret);
        return ret;
    }

    /* 删除此entry */
    ret = tree_remove_ie(tree);
    if (0 > ret)
    {
        LOG_ERROR("Remove key failed. ret(%d)\n", ret);
        return ret;
    }
   
    PRINT_KEY("Remove key finished", tree, key, key_len);

    return ret;
}

/*******************************************************************************
删除指定key
*******************************************************************************/
int32_t index_remove_key(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&((ATTR_HANDLE *)tree)->attr_info->attr_lock);
    ret = index_remove_key_nolock(tree, key, key_len);
    OS_RWLOCK_WRUNLOCK(&((ATTR_HANDLE *)tree)->attr_info->attr_lock);

    return ret;
}

/*******************************************************************************
插入指定的key和value, 无锁
*******************************************************************************/
int32_t index_insert_key_nolock(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len, const void *c, uint16_t value_len)
{
    INDEX_ENTRY *ie = NULL;
    uint16_t len = 0;
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == tree) || (NULL == key) || (NULL == c)
        || (0 == key_len) || (0 == value_len))
    {
        LOG_ERROR("Invalid parameter. tree(%p) key(%p) c(%p) key_len(%d) value_len(%d)\n",
            tree, key, c, key_len, value_len);
        return -INDEX_ERR_PARAMETER;
    }

    if ((tree->attr_info->attr_record.attr_flags & FLAG_TABLE) == 0)
    {
        LOG_ERROR("The attr is resident. obj_name(%s)\n", tree->attr_info->obj->obj_name);
        return -INDEX_ERR_ATTR_RESIDENT;
    }

    PRINT_KEY("Insert key start", tree, key, key_len);

    /* 搜索是否已经存在此key */
    ret = search_key_internal(tree, key, key_len);
    if (ret >= 0)
    {
        return -INDEX_ERR_KEY_EXIST;
    }
    
    if (-INDEX_ERR_KEY_NOT_FOUND != ret)
    {
        LOG_ERROR("Search key failed. obj_name(%s) ret(%d)\n", tree->attr_info->obj->obj_name, ret);
        return ret;
    }

    len = sizeof(INDEX_ENTRY) + key_len + value_len;

    ie = (INDEX_ENTRY *) OS_MALLOC(len);
    if (NULL == ie)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", len);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    /* 生成entry */
    ie->flags = 0;
    ie->len = len;
    ie->key_len = key_len;
    ie->value_len = value_len;
    memcpy(IEGetKey(ie), key, key_len);
    memcpy(IEGetValue(ie), c, value_len);

    /* 将此entry插入树中 */
    ret = tree_insert_ie(tree, &ie);
    if (0 > ret)
    {
        LOG_ERROR("%s", "The key insert failed.\n");
        OS_FREE(ie);
        return ret;
    }
    
    OS_FREE(ie);

    PRINT_KEY("Insert key finished", tree, key, key_len);

    return ret;
}

/*******************************************************************************
插入指定的key和value
*******************************************************************************/
int32_t index_insert_key(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len, const void *c, uint16_t value_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&((ATTR_HANDLE *)tree)->attr_info->attr_lock);
    ret = index_insert_key_nolock(tree, key, key_len, c, value_len);
    OS_RWLOCK_WRUNLOCK(&((ATTR_HANDLE *)tree)->attr_info->attr_lock);

    return ret;
}

int32_t index_update_value(ATTR_HANDLE *tree, const void *key,
    uint16_t key_len, const void *value, uint16_t value_len)
{
    int32_t ret = 0;

    /* 检查输入参数 */
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    if ((tree->attr_info->attr_record.attr_flags & FLAG_TABLE) == 0)
    {
        LOG_ERROR("The attr is resident. obj_name(%s)\n", tree->attr_info->obj->obj_name);
        return -INDEX_ERR_ATTR_RESIDENT;
    }

    OS_RWLOCK_WRLOCK(&tree->attr_info->attr_lock);
    ret = index_remove_key_nolock(tree, key, key_len);
    ret = index_insert_key_nolock(tree, key, key_len, value, value_len);
    OS_RWLOCK_WRUNLOCK(&tree->attr_info->attr_lock);

    return ret;
}


EXPORT_SYMBOL(index_create);
EXPORT_SYMBOL(index_open);
EXPORT_SYMBOL(index_close);

EXPORT_SYMBOL(index_search_key);
EXPORT_SYMBOL(walk_tree);
EXPORT_SYMBOL(index_insert_key);
EXPORT_SYMBOL(index_remove_key);
EXPORT_SYMBOL(tree_remove_ie);
EXPORT_SYMBOL(index_walk_all);

EXPORT_SYMBOL(index_find_handle);

EXPORT_SYMBOL(index_search_key_nolock);
EXPORT_SYMBOL(index_insert_key_nolock);
EXPORT_SYMBOL(index_remove_key_nolock);


