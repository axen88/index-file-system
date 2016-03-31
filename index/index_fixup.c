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
File Name: INDEX_TOOLS_FIXUP.C
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
#include "os_log.h"

MODULE(PID_INDEX);

int32_t fixup_callback(void *tree, void *para)
{
#if 0
    object_handle_t *pstTree = tree;

    if (0 != (pstTree->pstInode->flags & TREE_FLAGS_DIR))
    {
        LOG_INFO("Got a directory tree. name(%s)\n", pstTree->pstInode->name);
        return 0;
    }
#endif
    return 0;
}

int32_t fixup_attr(object_handle_t *tree, void *para)
{
#if 0
    int32_t ret = 0;
    WALK_ALL_TREES_PARA_S *para = para;

    ASSERT(NULL != tree);
    ASSERT(NULL != para);
    
    LOG_INFO("Start fixup tree. name(%s)\n", tree->pstInode->name);

    ret = index_walk_all(tree, B_FALSE, para->flags, para, fixup_callback);
    if (0 > ret)
    {
        LOG_ERROR("Fixup tree failed. name(%s) ret(%d)\n", tree->pstInode->name, ret);
    }

    LOG_INFO("Fixup tree success. name(%s)\n", tree->pstInode->name);

    return ret;
#endif
	return 0;
}

int32_t before_fixup(index_handle_t *index, WALK_ALL_TREES_PARA_S *para)
{
    return 0;
}

int32_t fixup_index(index_handle_t *index)
{
#if 0
    int32_t ret = 0;
    WALK_ALL_TREES_PARA_S para;
    
    ASSERT (NULL != index);

    LOG_INFO("Start fixup index. tree(%p)\n", index);
    
    ret = before_fixup(index, &para);
    if (0 > ret)
    {
        LOG_ERROR("before_fixup failed. tree(%p) ret(%d)\n", index, ret);
        return ret;
    }

    ret = index_walk_all_attrs(index, &para);
    if (0 > ret)
    {
        LOG_ERROR("Fixup index failed. index(%p) ret(%d)\n", index, ret);
        return ret;
    }

    ret = block_finish_fixup(index->pF);
    if (0 > ret)
    {
        LOG_ERROR("Finish fixup index failed. index(%p) ret(%d)\n", index, ret);
        return ret;
    }
    
    LOG_INFO("Fixup index success. index(%p)\n", index);
    
#endif
    return 0;
}

int32_t fixup_index_by_name(char *index_name, uint64_t start_lba)
{
    index_handle_t *index = NULL;
    int32_t ret = 0;
    
    ASSERT (NULL != index_name);
    ASSERT (0 != strlen(index_name));

    LOG_EVENT("Start fixup index. index_name(%s) start_lba(%lld)\n",
        index_name, start_lba);

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        LOG_ERROR("Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    ret = fixup_index(index);
    if (0 > ret)
    {
        LOG_ERROR("Fixup index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
    }
    else
    {
        LOG_EVENT("Fixup index success. index_name(%s) start_lba(%lld)\n",
            index_name, start_lba);
    }
    
    (void)index_close(index);
    
    return 0;
}

