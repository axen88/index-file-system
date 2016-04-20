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
#include "ofs_if.h"

MODULE(PID_UTILS);
#include "os_log.h"

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
    uint8_t ucIfFlag = (B_FALSE == reverse)
        ? INDEX_GET_FIRST : INDEX_GET_LAST;
    uint8_t ucWhileFlag = (B_FALSE == reverse)
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

