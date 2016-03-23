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
File Name: INDEX_COLLATE.H
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
#ifndef __INDEX_COLLATE_H__
#define __INDEX_COLLATE_H__

#ifdef  __cplusplus
extern "C"
{
#endif

typedef uint16_t UNICODE_CHAR;

typedef struct _index_extent
{
    uint64_t addr;  // start address
    uint64_t len;         // length
} index_extent_t;

// collate rules
enum
{
    CR_BINARY = 0,
    CR_ANSI_STRING,
    CR_UNICODE_STRING,
    CR_U64,
    CR_EXTENT,
    
    CR_BUTT
} COLLATE_RULE_E;

#define CR_MASK               0x000F

extern int32_t os_collate_binary(const uint8_t *b1, uint32_t v_sizeB1,
    const uint8_t *b2, uint32_t b2_size);
extern int32_t os_collate_unicode_string(const UNICODE_CHAR *str1, uint32_t str1_size,
    const UNICODE_CHAR *str2, uint32_t str2_size);
extern int32_t os_collate_ansi_string(const char *str1, uint32_t str1_size,
    const char *str2, uint32_t str2_size);
uint32_t os_u64_to_bstr(uint64_t u64, uint8_t *b);
uint64_t os_bstr_to_u64(const uint8_t *b, uint32_t b_size);
int32_t os_collate_u64(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size);
uint32_t os_u64_size(uint64_t u64);
uint32_t os_extent_to_extent_pair(const index_extent_t *ext, uint8_t *ext_pair);
void os_extent_pair_to_extent(const uint8_t *ext_pair, uint32_t ext_pair_size, index_extent_t *ext);
int32_t os_collate_extent(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size);

extern int32_t collate_key(uint16_t collate_rule, INDEX_ENTRY * v_pstIE,
    const void * key, uint16_t key_len);


#ifdef  __cplusplus
}
#endif

#endif


