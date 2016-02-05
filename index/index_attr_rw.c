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
文 件 名: INDEX_ATTR_RW.C
版    本: 1.00
日    期: 2012年7月24日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年7月24日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "index_if.h"

int32_t index_truncate_attr(ATTR_HANDLE *attr, uint64_t new_size)
{
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(attr != NULL);

    obj = attr->attr_info->obj;

    
    return 0;
}


int32_t index_pread_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size)
{
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(attr != NULL);

    obj = attr->attr_info->obj;

    
    return 0;
}


int32_t index_pwrite_attr(ATTR_HANDLE *attr, uint64_t position,
    void *content, uint32_t size)
{
    OBJECT_HANDLE *obj = NULL;
    
    ASSERT(attr != NULL);

    obj = attr->attr_info->obj;

    
    return 0;
}

