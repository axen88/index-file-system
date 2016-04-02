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
File Name: INDEX_UTILS.C
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

extern int32_t FixupTree(void *tree, void *para);

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

int32_t index_walk_all(object_handle_t *tree, bool_t v_bReverse, uint8_t flags,
    void *para, tree_walk_cb_t cb)
{
    int32_t ret = 0;
    uint8_t ucIfFlag = (B_FALSE == v_bReverse)
        ? INDEX_GET_FIRST : INDEX_GET_LAST;
    uint8_t ucWhileFlag = (B_FALSE == v_bReverse)
        ? 0 : INDEX_GET_PREV;

    if ((NULL == tree) || (NULL == cb))
    {
        LOG_ERROR("Invalid parameter. tree(%p) cb(%p)\n",
            tree, cb);
        return -INDEX_ERR_PARAMETER;
    }

    ucIfFlag |= (flags & ~INDEX_WALK_MASK);
    ucWhileFlag |= (flags & ~INDEX_WALK_MASK);
    
    OS_RWLOCK_WRLOCK(&tree->obj_info->attr_lock);
    
    if (walk_tree(tree, ucIfFlag) == 0)
    {
	    do
	    {
            ret = cb(tree, para);
            if (ret < 0)
            {
                LOG_ERROR("Call back failed. tree(%p) para(%p) ret(%d)\n",
                    tree, para, ret);
                OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);
                return ret;
            }
	    } while (walk_tree(tree, ucWhileFlag) == 0);
    }
    
    OS_RWLOCK_WRUNLOCK(&tree->obj_info->attr_lock);
    
    return 0;
}

static int32_t tree_callback(void *tree, void *para)
{
#if 0
    int32_t ret = 0;
    char name[OBJ_NAME_SIZE];
    tree_walk_para_t *para = para;
    object_handle_t *tree = NULL;

    ASSERT(NULL != tree);
    ASSERT(NULL != para);

    memset(name, 0, sizeof(name));
    (void)IndexDumpKey(tree, name, (uint16_t)sizeof(name));
    
    ret = INDEX_OpenTreeNoLock(&tree, tree, name, 0);
    if (ret < 0)
    {
        LOG_ERROR("Open tree failed. tree(%s) ret(%d)\n", name, ret);
        return ret;
    }

    ret = para->pCallBack(tree, para);
    if (0 > ret)
    {
        LOG_ERROR("pCallBack return failed. name(%s) ret(%d)\n",
            tree->pstInode->name, ret);
    }
    else if (FixupTree == para->pCallBack)
    {
        SET_TREE_DIRTY(tree);
        INDEX_CommitTreeTransNoLock(tree, COMMIT_FLAG_FORCE);
        SET_TREE_DIRTY(tree);
        INDEX_CommitTreeTransNoLock(tree, COMMIT_FLAG_FORCE);
    }

    (void)INDEX_CloseTreeNoLock(tree);

    return ret;
#endif
	return 0;
}

int32_t index_walk_all_attrs(object_handle_t *dir_tree,
    tree_walk_para_t *para)
{
#if 0
    int32_t ret = 0;
    
    if ((NULL == dir_tree) || (NULL == para)
        || (NULL == para->pCallBack))
    {
        LOG_ERROR("Invalid parameter. tree(%p) para(%p)\n",
            dir_tree, para);
        return -INDEX_ERR_PARAMETER;
    }

    ret = para->pCallBack(dir_tree, para);
    if (0 > ret)
    {
        LOG_ERROR("pCallBack return failed. name(%s) ret(%d)\n",
            dir_tree->pstInode->name, ret);
    }
    else if (FixupTree == para->pCallBack)
    {
        SET_TREE_DIRTY((object_handle_t *)dir_tree);
        INDEX_CommitTreeTransNoLock(dir_tree, COMMIT_FLAG_FORCE);
        SET_TREE_DIRTY((object_handle_t *)dir_tree);
        INDEX_CommitTreeTransNoLock(dir_tree, COMMIT_FLAG_FORCE);
    }

    ret = index_walk_all(dir_tree, B_FALSE, para->flags, para, tree_callback);
    if (0 > ret)
    {
        LOG_ERROR("index_walk_all return failed. name(%s) ret(%d)\n",
            dir_tree->pstInode->name, ret);
        return ret;
    }
    
    return ret;
#endif
	return 0;
}

int32_t index_get_opened_attr_num(object_handle_t * tree)
{
#if 0
    int32_t ret = 0;
    
    if (NULL == tree)
    {
        LOG_ERROR("Invalid parameter. tree(%p)\n", tree);
        return -INDEX_ERR_PARAMETER;
    }

    OS_RWLOCK_WRLOCK(&((object_handle_t *)tree)->index->rwlock);
    ret = dlist_count(&((object_handle_t *)tree)->index->stTreesList);
    OS_RWLOCK_WRUNLOCK(&((object_handle_t *)tree)->index->rwlock);

    return ret;
#endif
	return 0;
}


