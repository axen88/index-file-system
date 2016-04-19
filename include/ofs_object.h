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

#ifdef	__cplusplus
extern "C" {
#endif

/* open object */
extern int32_t index_open_object(index_handle_t *index, uint64_t objid, object_handle_t **obj);

/* create object */
extern int32_t index_create_object(index_handle_t *index, uint64_t objid, uint16_t flags, object_handle_t **obj);

extern int32_t index_close_object(object_handle_t *obj);

/* delete object */
extern int32_t index_delete_object(index_handle_t *index, uint64_t objid);

/* rename object */
extern int32_t index_rename_object(object_handle_t *obj, const char *new_obj_name);

object_info_t *index_get_object_info(index_handle_t *index, uint64_t objid);
object_handle_t *index_get_object_handle(index_handle_t *index, uint64_t objid);

int32_t set_object_name(object_handle_t *obj, char *name);

/* for internal only */
int32_t create_object(index_handle_t *index, uint64_t objid, uint16_t flags, object_handle_t **obj);
int32_t open_object(index_handle_t *index, uint64_t objid, uint64_t inode_no, object_handle_t **obj);
extern void close_object(object_info_t *obj_info);
int32_t create_object_at_inode(index_handle_t *index, uint64_t objid, uint64_t inode_no,
    uint16_t flags, object_handle_t **obj_out);

extern int32_t compare_attr_info2(const char *attr_name, object_info_t *attr_info_node);

#ifdef	__cplusplus
}
#endif

#endif

