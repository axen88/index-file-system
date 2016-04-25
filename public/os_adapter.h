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
File Name: OS_ADAPTER.H
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
 
#ifndef __OS_ADAPTER_H__
#define __OS_ADAPTER_H__


//#define __KERNEL__

#include "os_types.h"

#if defined(WIN32)  // windows

#include "os_windows.h"

#elif defined(__KERNEL__) // linux kernel

#include "os_linux_kernel.h"

//#include "os_debug.h"

#else  // linux user space

#include "os_linux_user.h"

#endif


#ifdef	__cplusplus
extern "C" {
#endif


 #define    SIZE_OF_TYPE_EQUAL_TO(type, size) \
 static inline char size_of_##type##_equal_to_##size() \
 { \
     char __dummy1[sizeof(type) - size]; \
     char __dummy2[size - sizeof(type)]; \
     return __dummy1[-1] + __dummy2[-1]; \
 }
 
 
 
 #define    SIZE_OF_TYPE_UNEQUAL_TO(type, size) \
 static inline char size_of_##type##_unequal_to_##size() \
 { \
     char __dummy1[0==(10/(sizeof(type)-size))]; \
     return __dummy1[-1]; \
 }
 
 
 
 #define    SIZE_OF_TYPE_NOT_LARGER_THAN(type, size) \
 static inline char size_of_##type##_not_larger_than_##size() \
 { \
     char __dummy1[size - sizeof(type)]; \
     return __dummy1[-1]; \
 }
 
 
 
 #define    SIZE_OF_TYPE_NOT_SMALLER_THAN(type, size) \
 static inline char size_of_##type##_not_smaller_than_##size() \
 { \
     char __dummy1[sizeof(type) - size]; \
     return __dummy1[-1]; \
 }
 
 
 
#define    SIZE_OF_TYPE_SMALLER_THAN(type, size) \
     SIZE_OF_TYPE_NOT_LARGER_THAN(type, size) \
     SIZE_OF_TYPE_UNEQUAL_TO(type, size)
 
 
 
 #define    SIZE_OF_TYPE_LARGER_THAN(type, size) \
     SIZE_OF_TYPE_NOT_SMALLER_THAN(type, size) \
     SIZE_OF_TYPE_UNEQUAL_TO(type, size)


//
// Data type conversion and combination
//
#define Low4(x)           (((uint8_t)(x)) & (uint8_t)0x0f)
#define High4(x)          (((uint8_t)(x)) >> 4)
#define Low8(x)           ((uint8_t)(x))
#define High8(x)          ((uint8_t)(((uint16_t)(x)) >> 8))
#define Low16(x)          ((uint16_t)(x))
#define High16(x)         ((uint16_t)(((uint32_t)(x)) >> 16))
#define Make16(low, high) (((uint16_t)(low)) | (((uint16_t)(high)) << 8))
#define Make32(low, high) (((uint32_t)(low)) | (((uint32_t)(high)) << 16))

//
// Bits operations
//
#define SetBits(x, bs) ((x) |= (bs))
#define ClrBits(x, bs) ((x) &= ~(bs))
#define GetBits(x, bs) ((x) & (bs))
    
#define ArraySize(a)           (sizeof(a) / sizeof(a[0])) // Get array size

#define OS_OFFSET(type, member) \
    (((ptr_t)(&(((type *)0x1234)->member))) - 0x1234)
    
#define OS_CONTAINER(ptr, type, member) \
    ((type *)(((char *)(ptr)) - OS_OFFSET(type, member)))
    
#define RoundUp(num, round)    (((num) + ((round) - 1)) & ~((round) - 1))
#define RoundUp2(num, round, roundShift)    (((num) + ((round) - 1)) >> roundShift)
#define RoundUp3(num, round)    (((num) + ((round) - 1)) / (round))

#define RoundDown(num, round)  ((num) & ~((round) - 1))

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
    
    
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef BITS_PER_BYTE_SHIFT
#define BITS_PER_BYTE_SHIFT 3
#endif


#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE (1 << BITS_PER_BYTE_SHIFT)
#endif

//static uint32_t g_pid = PID_INDEX;
#define MODULE(pid)  static uint32_t g_pid = (pid)    

#ifdef	__cplusplus
}
#endif

#endif

