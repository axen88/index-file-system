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
File Name: INDEX_COLLATE.C
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
#include "os_types.h"
#include "os_adapter.h"

#include "index_if.h"
    
#ifdef __KERNEL__

char os_to_upper(char c)
{
    return (((c <= 'z') && (c >= 'a')) ? (c - ('a' - 'A')) : c);
}

UNICODE_CHAR os_to_wupper(UNICODE_CHAR c)
{
    return (((c <= 'z') && (c >= 'a')) ? (c - ('a' - 'A')) : c);
}

#else
#include <ctype.h>
#include <wctype.h>

#define os_to_upper toupper
#define os_to_wupper towupper

#endif
	
/*
return value:
    <0: b1 < b2
    =0: b1 == b2
    >0: b1 > b2
*/
int32_t os_collate_binary(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size)
{
    ASSERT(b1_size > 0);
    ASSERT(b2_size > 0);

    /* discard the beginning 0 */
	while ((0 == *b1) && (b1_size > 0))
	{
		b1++;
		b1_size--;
	}
	
    /* discard the beginning 0 */
	while ((0 == *b2) && (b2_size > 0))
	{
		b2++;
		b2_size--;
	}
	
    if (b1_size > b2_size)
	{
		return 1;
	}

	if (b1_size < b2_size)
	{
		return -1;
	}
    
	while (0 != b1_size)
	{
		if (*b1 > *b2)
		{
			return 1;
		}

        if (*b1 < *b2)
		{
			return -1;
		}
        
		b1++;
		b2++;
        b1_size--;
	}

	return 0;
}

/*
return value
    <0: str1 < str2
    =0: str1 == str2
    >0: str1 > str2
*/
int32_t os_collate_unicode_string(const UNICODE_CHAR *str1, uint32_t str1_size,
	const UNICODE_CHAR *str2, uint32_t str2_size)
{
	UNICODE_CHAR c1 = 0;
    UNICODE_CHAR c2 = 0;

    ASSERT(str1_size > 0);
    ASSERT(str2_size > 0);

	while ((0 != str1_size) && (0 != str2_size))
	{
		c1 = os_to_wupper(*str1);
		c2 = os_to_wupper(*str2);
        if (c1 > c2)
		{
			return 1;
		}
        
		if (c1 < c2)
		{
			return -1;
		}

		str1++;
		str2++;
        str1_size--;
        str2_size--;
	}

    if (str1_size > str2_size)
	{
		return 1;
	}
    
	if (str1_size < str2_size)
	{
		return -1;
	}

	return 0;
}

/*
    <0: str1 < str2
    =0: str1 == str2
    >0: str1 > str2
*/
int32_t os_collate_ansi_string(const char *str1, uint32_t str1_size,
	const char *str2, uint32_t str2_size)
{
	char c1 = 0;
    char c2 = 0;

    ASSERT(str1_size > 0);
    ASSERT(str2_size > 0);

	while ((0 != str1_size) && (0 != str2_size))
	{
		c1 = os_to_upper(*str1);
		c2 = os_to_upper(*str2);
        if (c1 > c2)
		{
			return 1;
		}
        
		if (c1 < c2)
		{
			return -1;
		}

		str1++;
		str2++;
        str1_size--;
        str2_size--;
	}

    if (str1_size > str2_size)
	{
		return 1;
	}
    
	if (str1_size < str2_size)
	{
		return -1;
	}

	return 0;
} 

// little endian u64
uint64_t os_bstr_to_u64(const uint8_t *b, uint32_t b_size)
{
    uint64_t u64 = 0;
    uint8_t *uc = (uint8_t *)&u64;

    ASSERT(0 != b_size);
    ASSERT(sizeof(uint64_t) >= b_size);

    while (b_size--)
    {
        *uc = *b;
        uc++;
        b++;
    }

    return u64;
}

// little endian u64
uint32_t os_u64_size(uint64_t u64)
{
    uint8_t *uc = (uint8_t *)&u64;
    uint32_t b_size = sizeof(uint64_t);
    uint32_t pos = sizeof(uint64_t) - 1;

    while (pos > 0)
    {
        if (uc[pos] != 0)
        {
            break;
        }
        
        b_size--;
        pos--;
    }

    if (b_size == 0)
    {
        b_size = 1;
    }

    return b_size;
}

// little endian u64
uint32_t os_u64_to_bstr(uint64_t u64, uint8_t *b)
{
    uint8_t *uc = (uint8_t *)&u64;
    uint32_t b_size;
    uint32_t pos = sizeof(uint64_t) - 1;

    b_size = os_u64_size(u64);

    pos = 0;
    while (pos < b_size)
    {
        *b = *uc;
        b++;
        uc++;
		pos++;
    }
    
    return b_size;
}

	
/*
return value:
    <0: b1 < b2
    =0: b1 == b2
    >0: b1 > b2
*/
int32_t os_collate_u64(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size)
{
    uint64_t u64_1 = os_bstr_to_u64(b1, b1_size);
    uint64_t u64_2 = os_bstr_to_u64(b2, b2_size);
	
    if (u64_1 > u64_2)
	{
		return 1;
	}

	if (u64_1 < u64_2)
	{
		return -1;
	}

	return 0;
}

uint32_t os_extent_to_extent_pair(const index_extent_t *ext, uint8_t *ext_pair)
{
    uint8_t addr_size = *ext_pair;

    addr_size = os_u64_to_bstr(ext->addr, ext_pair + 1);
    ext_pair[0] = addr_size;
    return (addr_size + os_u64_to_bstr(ext->len, ext_pair + 1 + addr_size));
}

void os_extent_pair_to_extent(const uint8_t *ext_pair, uint32_t ext_pair_size, index_extent_t *ext)
{
    uint8_t addr_size = *ext_pair;

    ASSERT(ext_pair_size > addr_size);

    ext->addr = os_bstr_to_u64(ext_pair + 1, addr_size);
    ext->len = os_bstr_to_u64(ext_pair + 1 + addr_size, ext_pair_size - addr_size);
}

/*
return value:
    <0: b1 < b2
    =0: b1 == b2
    >0: b1 > b2
*/
int32_t os_collate_extent(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size)
{
    index_extent_t ext1;
    index_extent_t ext2;

    os_extent_pair_to_extent(b1, b1_size, &ext1);
    os_extent_pair_to_extent(b2, b2_size, &ext2);
	
    if (ext1.addr >= (ext2.addr + ext2.len))
	{
		return 1;
	}

	if ((ext1.addr + ext1.len) <= ext2.addr)
	{
		return -1;
	}

	return 0; // overlap
}

