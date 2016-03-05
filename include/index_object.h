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
File Name: INDEX_OBJECT.H
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
#ifndef __INDEX_OBJECT_H__
#define __INDEX_OBJECT_H__

/* open object */
extern int32_t index_open_object(struct _INDEX_HANDLE *index, uint64_t objid, OBJECT_HANDLE **obj);

/* create object */
extern int32_t index_create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj);

extern int32_t index_close_object(OBJECT_HANDLE *obj);

/* delete object */
extern int32_t index_delete_object(INDEX_HANDLE *index, uint64_t objid);

/* rename object */
extern int32_t index_rename_object(OBJECT_HANDLE *obj, const char *new_obj_name);

extern OBJECT_HANDLE *index_get_object_handle(INDEX_HANDLE *index, uint64_t objid);

/* for internal only */
int32_t create_object(INDEX_HANDLE *index, uint64_t objid, uint16_t flags, OBJECT_HANDLE **obj);
int32_t open_object(INDEX_HANDLE *index, uint64_t objid, uint64_t inode_no, OBJECT_HANDLE **obj);
extern int32_t commit_object_modification(OBJECT_HANDLE *obj);
extern int32_t close_object(OBJECT_HANDLE *obj);

extern int32_t compare_attr_info2(const char *attr_name, ATTR_INFO *attr_info_node);
int32_t flush_inode(OBJECT_HANDLE * obj);
void recover_obj_inode(OBJECT_HANDLE *obj);
void backup_obj_inode(OBJECT_HANDLE *obj);

#endif

