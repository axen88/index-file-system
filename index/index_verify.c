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

            Copyright(C), 2016~2019, axen2012@qq.com
********************************************************************************
File Name: INDEX_TOOLS_VERIFY.C
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

typedef struct tagVERIFY_PARA_S
{
    bool_t bReverse;    /* 顺向或逆向 */
    uint32_t no;         /* 当前key的序号 */
    char *pcKey;        /* 之前的key */
    uint32_t uiKeySize;    /* 之前的key的长度 */
} VERIFY_PARA_S;

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

int32_t index_verify_attr_by_name(char *index_name, uint64_t start_lba, uint64_t objid)
{
    OBJECT_HANDLE *obj = NULL;
    INDEX_HANDLE *index = NULL;
    int32_t ret = 0;

    /* 检查输入参数 */
    if ((NULL == index_name) || (0 == objid))
    {
        LOG_ERROR("Invalid parameter. index_name(%p) objid(%lld)\n",
            index_name, objid);
        return -INDEX_ERR_PARAMETER;
    }

    ret = index_open(index_name, start_lba, &index);
    if (0 > ret)
    {
        LOG_ERROR("Open index failed. index_name(%s) start_lba(%lld)\n",
            index_name, start_lba);
        return ret;
    }
    
    ret = index_open_object(index, objid, &obj);
    if (0 > ret)
    {
        LOG_ERROR("Open tree failed. index_name(%s) start_lba(%lld) objid(%lld)\n",
            index_name, start_lba, objid);
        (void)index_close(index);
        return ret;
    }

    ret = index_verify_attr(obj->attr, NULL);
    
    (void)index_close_object(obj);
    (void)index_close(index);

    return ret;
}

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
    para.pCallBack = (int32_t (*)(void *, void *))index_verify_attr;
    
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




