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

            版权所有(C), 2011~2014, AXEN工作室
********************************************************************************
文 件 名: OS_COLLATE.C
版    本: 1.00
日    期: 2011年6月19日
功能描述: 各种比较函数
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年6月19日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include "os_types.h"
#include "os_collate.h"
    
#ifdef __KERNEL__

/*******************************************************************************
函数名称: os_to_upper
功能说明: 将ANSI字符转换成大写的ANSI字符
输入参数:
    c: 需转换的ANSI字符
输出参数: 无
返 回 值: 大写的ANSI字符
说    明: 无
*******************************************************************************/
char os_to_upper(char c)
{
    return (((c <= 'z') && (c >= 'a')) ? (c - ('a' - 'A')) : c);
}

/*******************************************************************************
函数名称: os_to_wupper
功能说明: 将UNICODE字符转换成大写的UNICODE字符
输入参数:
    c: 需转换的UNICODE字符
输出参数: 无
返 回 值: 大写的UNICODE字符
说    明: 无
*******************************************************************************/
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
	
/*******************************************************************************
函数名称: OSCollateBinary
功能说明: 比较二进制字串
输入参数:
    b1   : 要比较的第一个二进制字串
    b1_size: 第一个二进制字串的长度
    b2   : 要比较的第二个二进制字串
    b2_size: 第二个二进制字串的长度
输出参数: 无
返 回 值:
    <0: b1 < b2
    =0: b1 == b2
    >0: b1 > b2
说    明: 无
*******************************************************************************/
int32_t os_collate_binary(const uint8_t *b1, uint32_t b1_size,
    const uint8_t *b2, uint32_t b2_size)
{
    /* 去掉以0开头的二进制字符 */
	while ((0 == *b1) && (b1_size > 0))
	{
		b1++;
		b1_size--;
	}
	
    /* 去掉以0开头的二进制字符 */
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
    
    /* 比较字串，此时v_uiSizeB1 == v_uiSizeB2 */
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

/*******************************************************************************
函数名称: OSCollateUnicodeString
功能说明: 比较unicode字串
输入参数:
    str1   : 要比较的第一个unicode字串
    str1_size: 第一个unicode字串的长度
    str2   : 要比较的第二个unicode字串
    str2_size: 第二个unicode字串的长度
输出参数: 无
返 回 值:
    <0: str1 < str2
    =0: str1 == str2
    >0: str1 > str2
说    明: 无
*******************************************************************************/
int32_t os_collate_unicode_string(const UNICODE_CHAR *str1, uint32_t str1_size,
	const UNICODE_CHAR *str2, uint32_t str2_size)
{
	UNICODE_CHAR c1 = 0;
    UNICODE_CHAR c2 = 0;

    /* 比较字串 */
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

/*******************************************************************************
函数名称: OSCollateAnsiString
功能说明: 比较ansi字串
输入参数:
    str1   : 要比较的第一个ansi字串
    str1_size: 第一个ansi字串的长度
    str2   : 要比较的第二个ansi字串
    str2_size: 第二个ansi字串的长度
输出参数: 无
返 回 值:
    <0: str1 < str2
    =0: str1 == str2
    >0: str1 > str2
说    明: 无
*******************************************************************************/
int32_t os_collate_ansi_string(const char *str1, uint32_t str1_size,
	const char *str2, uint32_t str2_size)
{
	char c1 = 0;
    char c2 = 0;

    /* 比较字串 */
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


