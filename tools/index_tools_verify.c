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
文 件 名: OS_INDEX_VERIFY.C
版    本: 1.00
日    期: 2011年6月8日
功能描述: 索引区或树的校验工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月8日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
   
#include "index_if.h"
MODULE(PID_INDEX);
#include "os_log.h"

typedef struct tagVERIFY_PARA_S
{
    bool_t bReverse;    /* 顺向或逆向 */
    uint32_t no;         /* 当前key的序号 */
    char *pcKey;        /* 之前的key */
    uint32_t uiKeySize;    /* 之前的key的长度 */
} VERIFY_PARA_S;

/*******************************************************************************
函数名称: verify_callback
功能说明: 检查树中key是否是正确的顺序
输入参数:
    v_pTree: 要校验的树
    para: 此检查需要的相关参数
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int32_t verify_callback(void *tree, void *para)
{
#if 0
    VERIFY_PARA_S *para = para;
    ATTR_HANDLE *obj = tree;
    int32_t ret = 0;

    if (0 == para->no++)
    {
        memcpy(para->pcKey, IEGetKey(obj->ie), obj->ie->key_len);
        para->uiKeySize = obj->ie->key_len;

        return 0;
    }

    ret = collate_key(obj->pstInode->ucCollateRule, obj->ie,
        para->pcKey, (uint16_t)para->uiKeySize);
    if (0 < ret)
    {   /* key比要找的key大 */
        if (B_FALSE == para->bReverse)
        {
            memcpy(para->pcKey, IEGetKey(obj->ie), obj->ie->key_len);
            para->uiKeySize = obj->ie->key_len;
            return 0;
        }

        OS_PRINT("Key is not in sequence. tree(%p) no(%d)\n",
            tree, para->no);
        return -1;
    }
    
    if (0 == ret)
    {   /* 已经找到了 */
        OS_PRINT("The same key found. tree(%p) no(%d)\n",
            tree, para->no);
        return -2;
    }
    
    /* key比要找的key小 */
    if (B_FALSE != para->bReverse)
    {
        memcpy(para->pcKey, IEGetKey(obj->ie), obj->ie->key_len);
        para->uiKeySize = obj->ie->key_len;
        return 0;
    }
    
    OS_PRINT("Key is not in reverse sequence. tree(%p) no(%d)\n",
        tree, para->no);
    return -3;
#endif
	return 0;
}

/*******************************************************************************
函数名称: IndexTreeVerify
功能说明: 校验树
输入参数:
    v_pTree: 要操作的树句柄
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t index_verify_attr(ATTR_HANDLE *tree, void *para)
{
#if 0
    int32_t ret = 0;
    VERIFY_PARA_S tmp_para;
    char *pcKey = NULL;
    uint16_t usKeyMaxSize = 0;

    /* 检查输入参数 */
    ASSERT(NULL != tree);

    usKeyMaxSize = KEY_MAX_SIZE;

    pcKey = OS_MALLOC(usKeyMaxSize);
    if (NULL == pcKey)
    {
        OS_PRINT("Allocate memory failed. size(%d)\n", usKeyMaxSize);
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    tmp_para.pcKey = pcKey;

    /* 正向遍历 */
    tmp_para.bReverse = B_FALSE;
    tmp_para.no = 0;
    ret = index_walk_all(tree, tmp_para.bReverse, 0, &tmp_para, verify_callback);
    if (0 > ret)
    {
        OS_PRINT("Verify sequence failed. index(%s) tree(%s) ret(%d)\n",
            tree->index->name, tree->pstInode->name, ret);
        OS_FREE(pcKey);
        pcKey = NULL;
        return ret;
    }

    OS_PRINT("Verify sequence success. index(%s) tree(%s)\n",
        tree->index->name, tree->pstInode->name);
    
    /* 反向遍历 */
    tmp_para.bReverse = B_TRUE;
    tmp_para.no = 0;
    ret = index_walk_all(tree, tmp_para.bReverse, 0, &tmp_para, verify_callback);
    if (0 > ret)
    {
        OS_PRINT("Verify reverse failed. index(%s) tree(%s) ret(%d)\n",
            tree->index->name, tree->pstInode->name, ret);
        OS_FREE(pcKey);
        pcKey = NULL;
        return ret;
    }
    
    OS_PRINT("Verify reverse success. index(%s) tree(%s)\n",
        tree->index->name, tree->pstInode->name);

    OS_FREE(pcKey);
    pcKey = NULL;
#endif
    return 0;
}

/*******************************************************************************
函数名称: IndexTreeVerifyByName
功能说明: 校验指定的树
输入参数:
    index_name: 索引区名称
    start_lba: 索引区的起始lba
    obj_name  : 树名称
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t index_verify_attr_by_name(char *index_name, uint64_t start_lba,
    char *obj_name)
{
    OBJECT_HANDLE *obj = NULL;
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == index_name) || (NULL == obj_name))
    {
        LOG_ERROR("Invalid parameter. index_name(%p) obj_name(%p)\n",
            index_name, obj_name);
        return -INDEX_ERR_PARAMETER;
    }

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        LOG_ERROR("Open index failed. index_name(%s) start_lba(%lld)\n",
            index_name, start_lba);
        return ret;
    }
    
    ret = index_open_object(index->root_obj, obj_name, &obj);
    if (0 > ret)
    {
        LOG_ERROR("Open tree failed. index_name(%s) start_lba(%lld) obj_name(%s)\n",
            index_name, start_lba, obj_name);
        (void)index_close(index);
        return ret;
    }

    ret = index_verify_attr(obj->mattr, NULL);
    
    (void)index_close_object(obj);
    (void)index_close(index);

    return ret;
}

/*******************************************************************************
函数名称: verify_index
功能说明: 校验指定的索引区
输入参数:
    index_name: 索引区名称
    start_lba: 索引区的起始lba
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t verify_index(char *index_name, uint64_t start_lba)
{
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;
    WALK_ALL_TREES_PARA_S para;
    
    /* 检查输入参数 */
    ASSERT (NULL != index_name);
    ASSERT (0 != strlen(index_name));

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        LOG_ERROR("Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        return ret;
    }

    memset(&para, 0, sizeof(para));
    para.pCallBack = index_verify_attr;
    
    //ret = IndexWalkAllTrees(index, &para);
    if (0 > ret)
    {
        LOG_ERROR("verify index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            index_name, start_lba, ret);
        (void)index_close(index);
        return ret;
    }
    
    LOG_ERROR("verify index success. index_name(%s) start_lba(%lld)\n",
        index_name, start_lba);
    
    (void)index_close(index);
    
    return 0;
}

/*******************************************************************************
函数名称: IndexDoVerifyCmd
功能说明: 执行用户输入的verify相关命令行
输入参数:
    v_pcCmd: 要执行的命令
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int do_verify_cmd(int argc, char *argv[], NET_PARA_S *net)
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        net->print(net->net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -INDEX_ERR_ALLOCATE_MEMORY;
    }

    parse_all_para(argc, argv, para);
    para->net = net;

    if (0 == strlen(para->index_name))
    {
        OS_FREE(para);
        para = NULL;
        net->print(net->net, "invalid index name(%s).\n", para->index_name);
        return -2;
    }

    if (0 == strlen(para->obj_name))
    {
        verify_index(para->index_name, para->start_lba);
    }
    else
    {
        index_verify_attr_by_name(para->index_name, para->start_lba,
            para->obj_name);
    }

    OS_FREE(para);
    para = NULL;

    return 0;
}




