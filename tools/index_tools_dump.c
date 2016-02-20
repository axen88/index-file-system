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
    NET_PARA_S *net;
} DUMP_PARA_S;

static int32_t dump_callback(void *in_tree, DUMP_PARA_S *para)
{
    uint32_t i = 0;
    uint8_t *pucDat = NULL;
    ATTR_HANDLE *tree = in_tree;

    ASSERT(NULL != tree);
    ASSERT(NULL != para);

    if ((para->depth != tree->depth)
        || (para->vbn != tree->cache->vbn))
    {
        para->depth = tree->depth;
        para->vbn = tree->cache->vbn;
        OS_PRINT(para->net, "-------------iDepth: %d, llVbn: %lld-------------\n",
            tree->depth, tree->cache->vbn);
    }

    OS_PRINT(para->net, "%-7d(%d, %d) ", ++para->no,
        tree->ie->prev_len, tree->ie->len);

    pucDat = IEGetKey(tree->ie);
    for (i = 0; i < tree->ie->key_len; i++)
    {
        OS_PRINT(para->net, (COLLATE_BINARY == (tree->attr_info->attr_record.attr_flags & COLLATE_RULE_MASK2))
            ? "%02X" : "%c", pucDat[i]);
    }

    OS_PRINT(para->net, "%s", " = ");

    pucDat = IEGetValue(tree->ie);
    for (i = 0; i < tree->ie->value_len;)
    {
        OS_PRINT(para->net, "%02X", pucDat[i++]);
        if (0 == (i % 8))
        {
            OS_PRINT(para->net, "%c", ' ');
        }
    }

    OS_PRINT(para->net, "%s", "\n");

    return 0;
}

static int32_t dump_key(ATTR_HANDLE *tree, const bool_t v_bReverse, NET_PARA_S *net)
{
    int32_t ret = 0;
    DUMP_PARA_S para;

    ASSERT(NULL != tree);

	memset(&para, 0, sizeof(DUMP_PARA_S));
    
	OS_PRINT(net, "Start dump all keys. obj(%s)\n", tree->attr_info->obj->obj_name);

    para.net = net;
    ret = index_walk_all(tree, v_bReverse, 0, &para, (WalkAllCallBack)dump_callback);
    if (ret < 0)
    {
        OS_PRINT(net, "Walk tree failed. obj(%s) ret(%d)\n", tree->attr_info->obj->obj_name, ret);
    }
    
	OS_PRINT(net, "Finished dump all keys. obj(%s)\n", tree->attr_info->obj->obj_name);
    
	return ret;
}

void dump_cmd(INDEX_TOOLS_PARA_S *para)
{
    INDEX_HANDLE *index = NULL;
    OBJECT_HANDLE *obj = NULL;
    ATTR_HANDLE *attr = NULL;
    int32_t ret = 0;
    
    ASSERT (NULL != para);

    if (0 == strlen(para->index_name))
    {
        OS_PRINT(para->net, "invalid index name(%s).\n", para->index_name);
        return;
    }
    
    ret = index_open(para->index_name, para->start_lba, &index);
    if (0 > ret)
    {
        OS_PRINT(para->net, "Open index failed. index_name(%s) start_lba(%lld) ret(%d)\n",
            para->index_name, para->start_lba, ret);
        return;
    }
    
    if (0 == strlen(para->obj_name))
    {
        strcpy(para->obj_name, OBJID_OBJ_NAME);
        obj = index->idlst_obj;
    }
    else
    {
        ret = index_open_object(index, para->objid, &obj);
        if (0 > ret)
        {
            OS_PRINT(para->net, "Open obj failed. index_name(%s) start_lba(%lld) obj_name(%s) ret(%d)\n",
                para->index_name, para->start_lba, para->obj_name, ret);
            (void)index_close(index);
        }
       // return;
    }

    attr = obj->attr;

    ret = dump_key(attr, para->flags & TOOLS_FLAGS_REVERSE, para->net);

    index_close_attr(attr);
    if (0 != strlen(para->obj_name))
    {
        (void)index_close_object(obj);
    }
    
	(void)index_close(index);
	
	return;
}


int do_dump_cmd(int argc, char *argv[], NET_PARA_S *net)
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
    dump_cmd(para);

    OS_FREE(para);
    
    return 0;
}


