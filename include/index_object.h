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

            版权所有(C), 2012~2015, AXEN工作室
********************************************************************************
文 件 名: INDEX_OBJECT.H
版    本: 1.00
日    期: 2012年6月13日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年6月13日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#ifndef __INDEX_OBJECT_H__
#define __INDEX_OBJECT_H__

/* 打开对象 */
extern int32_t index_open_object(OBJECT_HANDLE *parent_obj,
    const char *obj_name, OBJECT_HANDLE **obj);

/* 增加对象 */
extern int32_t index_create_object(OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint64_t mode, OBJECT_HANDLE **obj);

extern int32_t index_close_object(OBJECT_HANDLE *obj);

/* 删除对象 */
extern int32_t index_remove_object(OBJECT_HANDLE *parent_obj, const char *obj_name,
    void *hnd, DeleteFunc del_func);

/* 重命名对象 */
extern int32_t index_rename_object(OBJECT_HANDLE *parent_obj,
    const char *obj_name, const char *new_obj_name);

/* 内部函数 */
int32_t create_object(struct _INDEX_HANDLE *index, OBJECT_HANDLE *parent_obj,
    const char *obj_name, uint64_t mode, OBJECT_HANDLE **obj);
int32_t open_object(struct _INDEX_HANDLE *index, OBJECT_HANDLE *parent_obj, const char *obj_name,
    uint64_t inode_no, OBJECT_HANDLE **obj);
OBJECT_HANDLE *find_child_object_handle(OBJECT_HANDLE *parent_obj,
    const char * obj_name);
extern int32_t index_commit_object_modification(OBJECT_HANDLE *obj);
extern int32_t close_object(OBJECT_HANDLE *obj);

extern int32_t compare_attr_info2(const char *attr_name, ATTR_INFO *attr_info_node);
int32_t flush_inode(OBJECT_HANDLE * obj);

#endif

