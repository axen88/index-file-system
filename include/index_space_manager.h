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
File Name: INDEX_SPACE_MANAGER.H
Author   : axen.hook
Version  : 1.00
Date     : 20/Mar/2016
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 20/Mar/2016
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/
#ifndef __INDEX_SPACE_MANAGER_H__
#define __INDEX_SPACE_MANAGER_H__

#ifdef	__cplusplus
extern "C" {
#endif



int32_t index_alloc_space(space_manager_t *sm, uint32_t blk_cnt, uint64_t *start_blk);
int32_t index_free_space(space_manager_t *sm, uint64_t start_blk, uint32_t blk_cnt);


#ifdef	__cplusplus
}
#endif

#endif

