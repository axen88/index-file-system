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
文 件 名: OS_INDEX_DUMP.C
版    本: 1.00
日    期: 2011年6月8日
功能描述: 树或索引区浏览工具
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月8日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

typedef struct tagDUMP_PARA_S
{
    uint8_t depth;   /* 树的深度 */
    uint64_t vbn;   /* 当前entry所在的VBN号 */
    uint32_t no;     /* 当前entry序号 */
} DUMP_PARA_S;

static int32_t dump_callback(void *in_tree, void *para)
{
    DUMP_PARA_S *tmp_para = para;
    uint32_t i = 0;
    uint8_t *pucDat = NULL;
    ATTR_HANDLE *tree = in_tree;

    ASSERT(NULL != tree);
    ASSERT(NULL != para);

    if ((tmp_para->depth != tree->depth)
        || (tmp_para->vbn != tree->cache->vbn))
    {
        tmp_para->depth = tree->depth;
        tmp_para->vbn = tree->cache->vbn;
        OS_PRINT("-------------iDepth: %d, llVbn: %lld-------------\n",
            tree->depth, tree->cache->vbn);
    }

    OS_PRINT("%-7d(%d, %d) ", ++tmp_para->no,
        tree->ie->prev_len, tree->ie->len);

    pucDat = IEGetKey(tree->ie);
    for (i = 0; i < tree->ie->key_len; i++)
    {
        OS_PRINT((COLLATE_BINARY == (tree->attr_info->attr_record.attr_flags & COLLATE_RULE_MASK2))
            ? "%02X" : "%c", pucDat[i]);
    }

    OS_PRINT("%s", " = ");

    pucDat = IEGetValue(tree->ie);
    for (i = 0; i < tree->ie->value_len;)
    {
        OS_PRINT("%02X", pucDat[i++]);
        if (0 == (i % 8))
        {
            OS_PRINT("%c", ' ');
        }
    }

    OS_PRINT("%s", "\n");

    return 0;
}

static int32_t dump_key(ATTR_HANDLE *tree, const bool_t v_bReverse)
{
    int32_t ret = 0;
    DUMP_PARA_S para;

    ASSERT(NULL != tree);

	memset(&para, 0, sizeof(DUMP_PARA_S));
    
	OS_PRINT("Start dump all keys. [obj: %s, attr: %s]\n",
        tree->attr_info->obj->obj_name, tree->attr_info->attr_name);

    ret = index_walk_all(tree, v_bReverse, 0, &para, dump_callback);
    if (ret < 0)
    {
        OS_PRINT("Walk tree failed. [obj: %s, attr: %s, ret: %d]\n",
            tree->attr_info->obj->obj_name, tree->attr_info->attr_name, ret);
    }
    
	OS_PRINT("Finished dump all keys. [obj: %s, attr: %s]\n",
        tree->attr_info->obj->obj_name, tree->attr_info->attr_name);
    
	return ret;
}

void dump_attr(INDEX_TOOLS_PARA_S *para)
{
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;
    int32_t ret = 0;
    
    ASSERT (NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT("invalid index name(%s).\n", para->index_name);
        return;
    }
    
    ret = index_open(para->index_name, para->start_lba, &index);
    if (0 > ret)
    {
        OS_PRINT("Open index failed. [index_name: %s, start_lba: %lld, ret: %d]\n",
            para->index_name, para->start_lba, ret);
        return;
    }
    
    if (0 == strlen(para->obj_name))
    {
        strcpy(para->obj_name, ROOT_OBJECT_NAME);
        obj = index->root_obj;
    }
    else
    {
        ret = index_open_object(index->root_obj, para->obj_name, &obj);
        if (0 > ret)
        {
            OS_PRINT("Open obj failed."
                " [index_name: %s, start_lba: %lld, obj_name: %s, ret: %d]\n",
                para->index_name, para->start_lba, para->obj_name, ret);
            (void)index_close(index);
        }
       // return;
    }
    
    if (0 == strlen(para->attr_name))
    {
        strcpy(para->attr_name, MATTR_NAME);
    }

    ret = index_open_xattr(obj, para->attr_name, &attr);
    if (0 > ret)
    {
        OS_PRINT("Open attr failed."
            " [index_name: %s, start_lba: %lld, obj_name: %s, attr_name: %s, ret: %d]\n",
            para->index_name, para->start_lba, para->obj_name, para->attr_name, ret);
        if (0 != strlen(para->obj_name))
        {
            (void)index_close_object(obj);
        }
        
        (void)index_close(index);
        return;
    }

    ret = dump_key(attr, para->flags & TOOLS_FLAGS_REVERSE);

    index_close_attr(attr);
    if (0 != strlen(para->obj_name))
    {
        (void)index_close_object(obj);
    }
    
	(void)index_close(index);
	
	return;
}


int do_dump_cmd(int argc, char *argv[])
{
    INDEX_TOOLS_PARA_S *para = NULL;

    para = OS_MALLOC(sizeof(INDEX_TOOLS_PARA_S));
    if (NULL == para)
    {
        OS_PRINT("Allocate memory failed. [size: %d]\n",
            (uint32_t)sizeof(INDEX_TOOLS_PARA_S));
        return -1;
    }

    parse_all_para(argc, argv, para);

    dump_attr(para);

    OS_FREE(para);
    
    return 0;
}


