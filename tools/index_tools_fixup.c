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
文 件 名: OS_INDEX_FIXUP.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 修复索引区工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"
#include "os_log.h"

MODULE(PID_INDEX);

int32_t fixup_callback(void *tree, void *para)
{
#if 0
    ATTR_HANDLE *pstTree = tree;

    /* 跳过目录树 */
    if (0 != (pstTree->pstInode->flags & TREE_FLAGS_DIR))
    {
        LOG_INFO("Got a directory tree. name(%s)\n", pstTree->pstInode->name);
        return 0;
    }
#endif
    return 0;
}

/*******************************************************************************
函数名称: fixup_attr
功能说明: 对指定的树进行修复
输入参数:
    v_pTree: 要操作的树的句柄
    para: 操作参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t fixup_attr(ATTR_HANDLE *tree, void *para)
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

int32_t before_fixup(INDEX_HANDLE *index, WALK_ALL_TREES_PARA_S *para)
{
#if 0
    int32_t ret = 0;
    
    memset(para, 0, sizeof(WALK_ALL_TREES_PARA_S));
    para->pCallBack = fixup_attr;
    para->flags = INDEX_ADD_BLOCK;

    ret = block_reset_bitmap(index->pF);
    if (0 > ret)
    {
        LOG_ERROR("block_reset_bitmap failed. pF(%p) ret(%d)\n", index->pF, ret);
        return ret;
    }
#endif
    return 0;
}

/*******************************************************************************
函数名称: FixupIndex
功能说明: 修复指定树所在的索引区
输入参数:
    v_pTree: 索引区上的随便一棵树操作句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t fixup_index(INDEX_HANDLE *index)
{
#if 0
    int32_t ret = 0;
    WALK_ALL_TREES_PARA_S para;
    
    /* 检查输入参数 */
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

/*******************************************************************************
函数名称: FixupIndexByName
功能说明: 修复指定的索引区
输入参数:
    index_name: 索引区名称
    start_lba: 索引区的起始lba
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t fixup_index_by_name(char *index_name, uint64_t start_lba)
{
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;
    
    /* 检查输入参数 */
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

/*******************************************************************************
函数名称: IndexDoFixupCmd
功能说明: 执行用户输入的fixup相关命令行
输入参数:
    v_pcCmd: 要执行的命令
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int do_fixup_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if (0 == strlen(para->index_name))
    {
        OS_FREE(para);
        para = NULL;
        OS_PRINT(net, "invalid index name(%s).\n", para->index_name);
        return -2;
    }

    (void)fixup_index_by_name(para->index_name, para->start_lba);
    
    OS_FREE(para);
    para = NULL;

    return 0;
}


