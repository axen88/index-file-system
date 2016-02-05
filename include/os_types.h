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

            版权所有(C), 2011~2014, 曾华荣 (zeng_hr@163.com)
********************************************************************************
文 件 名: OS_TYPES.H
版    本: 1.00
日    期: 2011年4月4日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年4月4日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/

#ifndef _OS_TYPES_H_
#define _OS_TYPES_H_

//==========================================================
// Complier options
//  1. System options
//  2. User defined options
//==========================================================
//#pragma pack(1)  // Aligned by 1 byte
//#pragma pack()  // Use the default alignment

//==========================================================
// Basic types
//==========================================================
typedef unsigned int         bool_t;

typedef unsigned char        uint8_t;   // unsigned 8-bit
typedef unsigned short       uint16_t;  // unsigned 16-bit
typedef unsigned int         uint32_t;  // unsigned 32-bit
typedef unsigned long long   uint64_t;  // unsigned 64-bit

typedef char                 int8_t;   // signed 8-bit
typedef short                int16_t;  // signed 16-bit
typedef int                  int32_t;  // signed 32-bit
typedef long long            int64_t;  // signed 64-bit

#ifdef WIN32
typedef unsigned long long       ptr_t;  // pointer
#else
typedef unsigned long            ptr_t;  // pointer
#endif

#define B_FALSE   0
#define B_TRUE    1

#ifndef SUCCESS
#define SUCCESS 0
#endif

#endif /* End of _OS_TYPES_H_ */

