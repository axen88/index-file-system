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
File Name: INDEX_MANAGER.H
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

#ifndef __INDEX_OP_H__
#define __INDEX_OP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 打开索引区 */
extern int32_t index_open(const char *index_name, uint64_t start_lba, INDEX_HANDLE **index);

/* 创建索引区 */
extern int32_t index_create(const char *index_name, uint64_t total_sectors, uint64_t start_lba,
    INDEX_HANDLE **index);

/* 扩容索引区 */
extern int32_t index_expand(OBJECT_HANDLE *tree, uint64_t v_ullAdditionalSectors);

/* 关闭索引区 */
extern int32_t index_close(INDEX_HANDLE *index);


extern int32_t index_get_opened_attr_num(OBJECT_HANDLE * tree);

extern INDEX_HANDLE *index_get_handle(const char * index_name);

extern int32_t fixup_index_by_name(char *index_name, uint64_t start_lba);
extern int32_t verify_index(char *index_name, uint64_t start_lba);
extern int32_t index_verify_attr_by_name(char *index_name, uint64_t start_lba, uint64_t objid);

/* 以下是索引系统初始化和退出接口，线程不安全 */
extern int32_t index_init_system(void);
extern void index_exit_system(void);



#ifdef __cplusplus
}
#endif

#endif

