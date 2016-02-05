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
文 件 名: OS_UTILS.C
版    本: 1.00
日    期: 2011年8月21日
功能描述: 各种有用的小函数
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年8月21日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef __KERNEL__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define rdtscll(val)  __asm__ __volatile__("rdtsc":"=A"(val))
#endif

#include "os_adapter.h"
#include "os_utils.h"

/*******************************************************************************
函数名称: os_get_cycle_count
功能说明: 获取当前cpu跑的cycle数目
输入参数: 无
输出参数: 无
返 回 值:
    >=0: cycle数目
说    明: 无
*******************************************************************************/
uint64_t os_get_cycle_count(void)
{
    uint64_t cycle = 0;
    
#ifndef WIN32
    rdtscll(cycle);
#endif
    
    return cycle;
}

/*******************************************************************************
函数名称: os_get_ms_count
功能说明: 获取当前时间的毫秒数
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 毫秒数
说    明: 无
*******************************************************************************/
uint64_t os_get_ms_count(void)
{
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval tv;

    #ifdef __KERNEL__
    do_gettimeofday(&tv);
    #else
    gettimeofday(&tv, NULL);
    #endif
    
    return ((((uint64_t)tv.tv_sec) * 1000) + (((uint64_t)tv.tv_usec) / 1000));
#endif
}

/*******************************************************************************
函数名称: os_get_second_count
功能说明: 获取当前时间的秒数
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 秒数
说    明: 无
*******************************************************************************/
uint64_t os_get_second_count(void)
{
#ifdef __KERNEL__
    struct timeval tv;
    do_gettimeofday(&tv);
    return ((uint64_t)tv.tv_sec);
#else
    return (time(NULL));
#endif
}

/*******************************************************************************
函数名称: os_str_to_u64
功能说明: 将字符串转换成64位数值
输入参数:
    str: 要处理的字符串
    base: 字符串的的进制
输出参数:
    value: 转换后的值
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t os_str_to_u64(const char *str, uint64_t *value, uint32_t base)
{
    char *end = NULL;

    *value = OSStrToUll(str, &end, base);

#if 0
    /*
    * 有错误发生:
    *  1. EINVAL: v_base在[2, 36]范围之外
    *  2. ERANGE: 转换出来的数据越界
    */
    if (errno != 0)
    {
        return -1;
    }
#endif

    /*
    * 处理以下情况:
    *  1. 首个字符为非法字符，如QQ, Q8等，但是0xQQ处理不了
    *  2. 字符串为空
    */
    if (end == str)
    {
        return -2;
    }
    
    /*
    * 处理以下情况:
    *  1. 字符串中含有非法字符，如QQ, Q8, 0xQQ, 0xABQQ, 0xQQAB等
    */
    if ((0 != *end) && ('\n' != *end))
    {
        return -3;
    }

    return 0;
}

/*******************************************************************************
函数名称: os_char_to_hex
功能说明: 将字符转换成对应的16进制数值
输入参数:
    c: 要转换的字符
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 非法字符
说    明: 无
*******************************************************************************/
char os_char_to_hex(char c)
{
    if ((c <= '9') && (c >= '0'))
    {
        return (c - '0');
    }
    
    if ((c <= 'f') && (c >= 'a'))
    {
        return (c - ('a' - 10));
    }
    
    if ((c <= 'F') && (c >= 'A'))
    {
        return (c - ('A' - 10));
    }

    return -1;
}

/*******************************************************************************
函数名称: os_str_to_hex
功能说明: 将16进制字符串转换成16进制数值
输入参数:
    str: 要处理的字符串
    hex_len: 装16进制数值的buffer的长度
输出参数:
    hex: 装16进制数据的buffer
返 回 值:
    >=0: 成功，实际的16进制数据的长度
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t os_str_to_hex(char *str, uint8_t *hex, uint32_t hex_len)
{
    uint32_t str_len = 0;
    int32_t i = 0;
    char c = 0;

    str_len = (uint32_t)strlen(str);
    if (0 == str_len)
    {
        return 0;
    }

    if (0 != (str_len & 1))
    {
        c = os_char_to_hex(*str++);
        if (0 > c)
        {
            return c;
        }

        *hex = (uint8_t)c;
        str_len--;
        
        i++;
        hex++;
    }

    while (str_len)
    {
        c = os_char_to_hex(*str++);
        if (0 > c)
        {
            return c;
        }

        *hex = (uint8_t)((uint8_t)c << 4);
        str_len--;

        c = os_char_to_hex(*str++);
        if (0 > c)
        {
            return c;
        }

        *hex |= (uint8_t)c;
        str_len--;
        
        i++;
        hex++;
    }

    return i;
}


/*******************************************************************************
函数名称: os_convert_u64
功能说明: 将64位的数据高低字节互换
输入参数:
    src: 要转换的源数据
输出参数: 无
返 回 值: 转换后的64位数据
说    明: 无
*******************************************************************************/
uint64_t os_convert_u64(const uint64_t src)
{
    uint64_t dst = 0;
    uint8_t *src_char = (uint8_t *)&src;
    uint8_t *dst_char = (uint8_t *)&dst;
    uint32_t i = 0;
    uint32_t j = sizeof(uint64_t);
    
    while (j--)
    {
        dst_char[j] = src_char[i++];
    }

    return dst;
}

#ifdef __KERNEL__

/*******************************************************************************
函数名称: os_get_date_time_string
功能说明: 将当前时间转换成字符串
输入参数:
    str_size: 存储字符串的buffer的长度
输出参数:
    str    : 存储字符串的buffer
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t os_get_date_time_string(char *str, int32_t str_size)
{
    struct timeval tm;

    if (NULL == str)
    {
        return -1;
    }
    
    do_gettimeofday(&tm);

    OS_SNPRINTF(str, str_size, "%8lx. %8lx", 
        tm.tv_sec, tm.tv_usec);
    
    return 0;
}

#else

/*******************************************************************************
函数名称: os_get_date_time_string
功能说明: 将当前时间转换成字符串
输入参数:
    str_size: 存储字符串的buffer的长度
输出参数:
    str    : 存储字符串的buffer
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int32_t os_get_date_time_string(char *str, int32_t str_size)
{
    time_t curr_time = 0;
    struct tm *pt = NULL;

    if (NULL == str)
    {
        return -1;
    }
    
    str[0] = 0;
    
    curr_time = time(NULL);
    pt = localtime(&curr_time);
    if (NULL == pt)
    {
        return -2;
    }

    OS_SNPRINTF(str, str_size, "%04d-%02d-%02d %02d:%02d:%02d", 
        pt->tm_year+1900, pt->tm_mon+1, pt->tm_mday, 
        pt->tm_hour, pt->tm_min, pt->tm_sec);
    
    return 0;
}

#endif


