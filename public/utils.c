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
File Name: OS_UTILS.C
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
#include "utils.h"

uint64_t os_get_cycle_count(void)
{
    uint64_t cycle = 0;
    
#ifndef WIN32
    rdtscll(cycle);
#endif
    
    return cycle;
}

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

uint64_t os_get_us_count(void)
{
#ifdef WIN32
    return -1;
#else
    struct timeval tv;

    #ifdef __KERNEL__
    do_gettimeofday(&tv);
    #else
    gettimeofday(&tv, NULL);
    #endif
    
    return ((((uint64_t)tv.tv_sec) * 1000 * 1000) + (((uint64_t)tv.tv_usec)));
#endif
}

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


int32_t os_str_to_u64(const char *str, uint64_t *value, uint32_t base)
{
    char *end = NULL;

    *value = OS_STR2ULL(str, &end, base);

#if 0
    /*
    *  1. EINVAL: base is out of the range [2, 36]
    *  2. ERANGE: the data overrun
    */
    if (errno != 0)
    {
        return -1;
    }
#endif

    /*
    *  1. the first character is invalid, like QQ, Q8
    *  2. empty string
    */
    if (end == str)
    {
        return -2;
    }
    
    /*
    * do QQ, Q8, 0xQQ, 0xABQQ, 0xQQAB etc.
    */
    if ((*end != 0) && ('\n' != *end))
    {
        return -3;
    }

    return 0;
}

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

int32_t os_str_to_hex(char *str, uint8_t *hex, uint32_t hex_len)
{
    uint32_t str_len = 0;
    int32_t i = 0;
    char c = 0;

    str_len = (uint32_t)strlen(str);
    if (str_len == 0)
    {
        return 0;
    }

    if (str_len & 1)
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

int32_t os_get_date_time_string(char *str, int32_t str_size)
{
    struct timeval tm;

    if (!str)
    {
        return -1;
    }
    
    do_gettimeofday(&tm);

    OS_SNPRINTF(str, str_size, "%8lx. %8lx", 
        tm.tv_sec, tm.tv_usec);
    
    return 0;
}

#else

int32_t os_get_date_time_string(char *str, int32_t str_size)
{
    time_t curr_time = 0;
    struct tm *pt = NULL;

    if (!str)
    {
        return -1;
    }
    
    str[0] = 0;
    
    curr_time = time(NULL);
    pt = localtime(&curr_time);
    if (!pt)
    {
        return -2;
    }

    OS_SNPRINTF(str, str_size, "%04d-%02d-%02d %02d:%02d:%02d", 
        pt->tm_year+1900, pt->tm_mon+1, pt->tm_mday, 
        pt->tm_hour, pt->tm_min, pt->tm_sec);
    
    return 0;
}

#endif




