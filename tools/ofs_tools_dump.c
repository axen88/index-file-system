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
File Name: INDEX_TOOLS_DUMP.C
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

typedef struct dump_para
{
    uint8_t depth; 
    u64_t vbn;  
    uint32_t no;  
    net_para_t *net;
} dump_para_t;

static int32_t dump_callback(object_handle_t *tree, dump_para_t *para)
{
    uint32_t i = 0;
    uint8_t *uc = NULL;

    ASSERT(tree);
    ASSERT(para);

    if ((para->depth != tree->depth)
        || (para->vbn != tree->cache->vbn))
    {
        para->depth = tree->depth;
        para->vbn = tree->cache->vbn;
        //NET_PRINT(para->net, "-------------depth: %d, vbn: %lld-------------\n",
         //   tree->depth, tree->cache->vbn);
    }

    NET_PRINT(para->net, "%-7d", ++para->no);
   // NET_PRINT(para->net, "%(%d, %d) ", tree->ie->prev_len, tree->ie->len);

    uc = GET_IE_KEY(tree->ie);
    switch (tree->obj_info->attr_record->flags & CR_MASK)
    {
        case CR_ANSI_STRING:
            for (i = 0; i < tree->ie->key_len; i++)
            {
                NET_PRINT(para->net, "%c", uc[i]);
            }
            break;
        case CR_U64:
        case CR_EXTENT:
            NET_PRINT(para->net, "%lld", os_bstr_to_u64(uc, tree->ie->key_len));
            break;
        default:
            for (i = 0; i < tree->ie->key_len; i++)
            {
                NET_PRINT(para->net, "%02X", uc[i]);
            }
            break;
    }
    

    NET_PRINT(para->net, "%s", " : ");

    uc = GET_IE_VALUE(tree->ie);
    switch ((tree->obj_info->attr_record->flags >> 4) & CR_MASK)
    {
        case CR_ANSI_STRING:
            for (i = 0; i < tree->ie->value_len; i++)
            {
                NET_PRINT(para->net, "%c", uc[i]);
            }
            break;
        case CR_U64:
        case CR_EXTENT:
            NET_PRINT(para->net, "%lld", os_bstr_to_u64(uc, tree->ie->value_len));
            break;
        default:
            for (i = 0; i < tree->ie->value_len; i++)
            {
                NET_PRINT(para->net, "%02X", uc[i]);
            }
            break;
    }

    NET_PRINT(para->net, "%s", "\n");

    return 0;
}

static int32_t dump_key(object_handle_t *tree, const bool_t reverse, net_para_t *net)
{
    int32_t ret = 0;
    dump_para_t para;
    object_info_t *obj_info;

    ASSERT(tree);

	memset(&para, 0, sizeof(dump_para_t));

    obj_info = tree->obj_info;
    
    NET_PRINT(net, "objid: %lld, inode_no: %lld, name: %s\n", obj_info->objid, obj_info->inode_no, obj_info->name);

    para.net = net;
    ret = index_walk_all(tree, reverse, 0, &para, (tree_walk_cb_t)dump_callback);
    if (ret < 0)
    {
        NET_PRINT(net, "Walk tree failed. objid(%lld) ret(%d)\n", tree->obj_info->objid, ret);
    }
    
	return ret;
}

void dump_cmd(ifs_tools_para_t *para)
{
    container_handle_t *ct = NULL;
    object_handle_t *obj = NULL;
    int32_t ret = 0;
    
    ASSERT (para);

    if (strlen(para->ct_name) == 0)
    {
        NET_PRINT(para->net, "invalid ct name.\n");
        return;
    }
    
    ret = ofs_open_container(para->ct_name, &ct);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Open ct failed. ct_name(%s) ret(%d)\n",
            para->ct_name, ret);
        return;
    }

    if (OBJID_IS_INVALID(para->objid)) // obj id not specified
    {
        para->objid = OBJID_OBJ_ID;
    }
    
    ret = ofs_open_object(ct, para->objid, &obj);
    if (ret < 0)
    {
        NET_PRINT(para->net, "Open obj failed. ct_name(%s) objid(%lld) ret(%d)\n", para->ct_name, para->objid, ret);
        (void)ofs_close_container(ct);
		return;
    }

    ret = dump_key(obj, para->flags & TOOLS_FLAGS_REVERSE, para->net);

    (void)ofs_close_object(obj);
	(void)ofs_close_container(ct);
	
	return;
}


int do_dump_cmd(int argc, char *argv[], net_para_t *net)
{
    ifs_tools_para_t *para = NULL;

    para = OS_MALLOC(sizeof(ifs_tools_para_t));
    if (para == NULL)
    {
        NET_PRINT(net, "Allocate memory failed. size(%d)\n",
            (uint32_t)sizeof(ifs_tools_para_t));
        return -1;
    }

    parse_all_para(argc, argv, para);
    para->net = net;
    dump_cmd(para);

    OS_FREE(para);
    
    return 0;
}


