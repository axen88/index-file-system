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
File Name: INDEX_ATTR.H
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

#ifndef __INDEX_ATTR_H__
#define __INDEX_ATTR_H__
    
/* 关闭属性 */
extern int32_t index_close_attr(ATTR_HANDLE *attr);

/* 读属性 */
extern int32_t index_pread_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size);

/* 写属性 */
extern int32_t index_pwrite_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size);

/* 修改属性大小 */
extern int32_t index_truncate_attr(ATTR_HANDLE *attr, uint64_t new_size);

/* 取消属性的修改 */
extern void cancel_attr_modification(ATTR_INFO *attr_info);

/* 提交属性的修改 */
extern int32_t commit_attr_modification(ATTR_INFO *attr_info);

/* 打开属性 */
extern int32_t index_open_attr(struct _OBJECT_HANDLE *obj, ATTR_HANDLE **attr);
// update the attr root cache into inode
void validate_attr(ATTR_INFO *attr_info);
extern void init_attr_info(struct _OBJECT_HANDLE *obj);
extern void destroy_attr_info(ATTR_INFO *attr_info);


#endif

