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

            Copyright(C), 2016~2019, axen2012@qq.com
********************************************************************************
File Name: OS_THREADS_GROUP.H
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

#ifndef __OSP_THREDS_GROUP_H__
#define __OSP_THREDS_GROUP_H__

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

typedef enum tagTHREADS_GROUP_ERROR_CODE_E
{
    ERR_THREADS_GROUP_INVALID_PARA = 300,
} THREADS_ARRAY_ERROR_CODE_E; /* End of tagTHREADS_ARRAY_ERROR_CODE_E */

extern void *threads_group_create(uint32_t num, void *(*func)(void *),
    void *para, char *thread_name);
extern int32_t threads_group_get_real_num(void *threads_group);
extern void threads_group_destroy(void *threads_group, uint32_t force,
    uint64_t over_time_ms);


#ifdef __cplusplus
}
#endif /* End of __cplusplus */

#endif /* End of __OSP_THREDS_GROUP_H__ */

