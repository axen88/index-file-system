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

            ��Ȩ����(C), 2011~2014, AXEN������
********************************************************************************
�� �� ��: OS_UTILS.C
��    ��: 1.00
��    ��: 2011��8��21��
��������: �������õ�С����
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��8��21��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
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
��������: os_get_cycle_count
����˵��: ��ȡ��ǰcpu�ܵ�cycle��Ŀ
�������: ��
�������: ��
�� �� ֵ:
    >=0: cycle��Ŀ
˵    ��: ��
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
��������: os_get_ms_count
����˵��: ��ȡ��ǰʱ��ĺ�����
�������: ��
�������: ��
�� �� ֵ:
    >=0: ������
˵    ��: ��
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
��������: os_get_second_count
����˵��: ��ȡ��ǰʱ�������
�������: ��
�������: ��
�� �� ֵ:
    >=0: ����
˵    ��: ��
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
��������: os_str_to_u64
����˵��: ���ַ���ת����64λ��ֵ
�������:
    str: Ҫ������ַ���
    base: �ַ����ĵĽ���
�������:
    value: ת�����ֵ
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t os_str_to_u64(const char *str, uint64_t *value, uint32_t base)
{
    char *end = NULL;

    *value = OSStrToUll(str, &end, base);

#if 0
    /*
    * �д�����:
    *  1. EINVAL: v_base��[2, 36]��Χ֮��
    *  2. ERANGE: ת������������Խ��
    */
    if (errno != 0)
    {
        return -1;
    }
#endif

    /*
    * �����������:
    *  1. �׸��ַ�Ϊ�Ƿ��ַ�����QQ, Q8�ȣ�����0xQQ������
    *  2. �ַ���Ϊ��
    */
    if (end == str)
    {
        return -2;
    }
    
    /*
    * �����������:
    *  1. �ַ����к��зǷ��ַ�����QQ, Q8, 0xQQ, 0xABQQ, 0xQQAB��
    */
    if ((0 != *end) && ('\n' != *end))
    {
        return -3;
    }

    return 0;
}

/*******************************************************************************
��������: os_char_to_hex
����˵��: ���ַ�ת���ɶ�Ӧ��16������ֵ
�������:
    c: Ҫת�����ַ�
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �Ƿ��ַ�
˵    ��: ��
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
��������: os_str_to_hex
����˵��: ��16�����ַ���ת����16������ֵ
�������:
    str: Ҫ������ַ���
    hex_len: װ16������ֵ��buffer�ĳ���
�������:
    hex: װ16�������ݵ�buffer
�� �� ֵ:
    >=0: �ɹ���ʵ�ʵ�16�������ݵĳ���
    < 0: �������
˵    ��: ��
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
��������: os_convert_u64
����˵��: ��64λ�����ݸߵ��ֽڻ���
�������:
    src: Ҫת����Դ����
�������: ��
�� �� ֵ: ת�����64λ����
˵    ��: ��
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
��������: os_get_date_time_string
����˵��: ����ǰʱ��ת�����ַ���
�������:
    str_size: �洢�ַ�����buffer�ĳ���
�������:
    str    : �洢�ַ�����buffer
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
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
��������: os_get_date_time_string
����˵��: ����ǰʱ��ת�����ַ���
�������:
    str_size: �洢�ַ�����buffer�ĳ���
�������:
    str    : �洢�ַ�����buffer
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
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


