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
File Name: OFS_COLLATE.H
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
#ifndef __OFS_COLLATE_H__
#define __OFS_COLLATE_H__

#ifdef  __cplusplus
extern "C"
{
#endif

typedef uint16_t unicode_char_t;

#define U64_MAX_SIZE          sizeof(uint64_t)

typedef struct index_extent
{
    uint64_t pa;         // physical address
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
    CR_EXTENT_MAP,
    
    CR_BUTT
} collate_rule_t;

#define CR_MASK               0x000F

#define EXT_PAIR_HEADER_SIZE    sizeof(uint8_t)  // ext_pair: header, addr, len
#define EXT_PAIR_MAX_SIZE       (U64_MAX_SIZE + U64_MAX_SIZE + EXT_PAIR_HEADER_SIZE)

int32_t os_collate_binary(const uint8_t *b1, uint32_t b1_size, const uint8_t *b2, uint32_t b2_size);
int32_t os_collate_unicode_string(const unicode_char_t *s1, uint32_t s1_size, const unicode_char_t *s2, uint32_t s2_size);
int32_t os_collate_ansi_string(const char *s1, uint32_t s1_size, const char *s2, uint32_t s2_size);
uint32_t os_u64_to_bstr(uint64_t u64, uint8_t *b);
uint64_t os_bstr_to_u64(const uint8_t *b, uint32_t b_size);
int32_t os_collate_u64(const uint8_t *b1, uint32_t b1_size, const uint8_t *b2, uint32_t b2_size);
uint32_t os_u64_size(uint64_t u64);
uint64_t os_extent_pair_to_extent(const uint8_t *ext_pair, uint32_t ext_pair_size, uint64_t *pa);
uint32_t os_extent_to_extent_pair(uint64_t pa, uint64_t len, uint8_t *ext_pair);
int32_t os_collate_extent(const uint8_t *k1, uint32_t k1_size, const uint8_t *v1, uint32_t v1_size,
    const uint8_t *k2, uint32_t k2_size, const uint8_t *v2, uint32_t v2_size);

int32_t collate_key(uint16_t collate_rule, index_entry_t *ie,
    const void *key, uint16_t key_len, const void *value, uint16_t value_len);


#ifdef  __cplusplus
}
#endif

#endif


