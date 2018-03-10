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
File Name: OS_UTILS.H
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

#ifndef __OS_UTILS_H__
#define __OS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint64_t os_get_cycle_count(void);
extern uint64_t os_get_ms_count(void);
extern uint64_t os_get_second_count(void);
extern int32_t os_str_to_u64(const char *str, uint64_t *value, uint32_t base);
extern char os_char_to_hex(char c);
extern int32_t os_str_to_hex(char *str, uint8_t *hex, uint32_t hex_len);
extern uint64_t os_convert_u64(const uint64_t src);
extern int32_t os_get_date_time_string(char *str, int32_t str_size);

#ifdef __cplusplus
}
#endif

#endif
