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
File Name: OFS_EXTENT_MAP.H
Author   : axen.hook
Version  : 1.00
Date     : 08/May/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 08/May/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#ifndef __OFS_EXTENT_MAP_H__
#define __OFS_EXTENT_MAP_H__

typedef struct ofs_extent_map_entry
{
    uint64_t vbn;
    uint64_t lbn;
    uint32_t len;
} ofs_extent_map_entry_t;

int32_t insert_extent(object_handle_t *obj, uint64_t vbn, uint64_t lbn, uint32_t blk_cnt);
int32_t remove_extent(object_handle_t *obj, uint64_t vbn, uint64_t lbn, uint32_t blk_cnt);

#endif

