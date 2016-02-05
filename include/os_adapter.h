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
#ifndef __OS_ADAPTER_H__
#define __OS_ADAPTER_H__

//#define __KERNEL__

#include "os_types.h"
#include "os_errors.h"

#if defined(WIN32)  // windows

#include "os_windows.h"

#elif defined(__KERNEL__) // linux kernel

#include "os_linux_kernel.h"

//#include "os_debug.h"

#else  // linux user space

#include "os_linux_user.h"

#endif

#if 0
    //==========================================================
    // Extended types
    //==========================================================
	#ifdef _BIG_ENDIAN_
    
    typedef union _UNION16
    { // 16 bits union, Big Endian
        struct
        { // 8 bits reference
            uint8_t h8; // High 8 bits
            uint8_t l8; // Low 8 bits
        } b; // Byte reference
        
        uint16_t u16; // 16 bits unsigned reference
        int16_t s16; // 16 bits signed reference
    } UNION16;
    
    typedef union _UNION32
    { // 32 bits union, Big Endian
        struct
        { // 8 bits reference
            uint8_t h8;  // High 8 bits
            uint8_t mh8; // Middle high 8 bits
            uint8_t ml8; // Middle low 8 bits
            uint8_t l8;  // Low 8 bits
        } b; // Byte reference
    
        struct
        { // 16 bits reference
            uint16_t h16; // High 16 bits
            uint16_t l16; // Low 16 bits
        } w; // Word reference
    
        uint32_t u32; // 32 bits unsigned reference
        int32_t s32; // 32 bits signed reference
    } UNION32;
    
    typedef union _UNION64
    { // 64 bits union, Big Endian
        struct
        { // 8 bits reference
            uint8_t h8;  // High 8 bits
            uint8_t mh8; // Middle high 8 bits
            uint8_t mmh8; // Middle high 8 bits
            uint8_t mmmh8; // Middle high 8 bits
            uint8_t mmml8; // Middle low 8 bits
            uint8_t mml8; // Middle low 8 bits
            uint8_t ml8; // Middle low 8 bits
            uint8_t l8;  // Low 8 bits
        } b; // Byte reference
    
        struct
        { // 16 bits reference
            uint16_t h16; // High 16 bits
            uint16_t mh16; // Middle High 16 bits
            uint16_t ml16; // Middle Low 16 bits
            uint16_t l16; // Low 16 bits
        } w; // Word reference
    
        struct
        { // 32 bits reference
            uint32_t h32; // High 32 bits
            uint32_t l32; // Low 32 bits
        } dw; // DWord reference
    
        uint64_t uint64_t; // 64 bits unsigned reference
        int64_t s64; // 64 bits signed reference
    } UNION64;
    
	#else
        
    typedef union _UNION16
    { // 16 bits union, Little Endian
        struct
        { // 8 bits reference
            uint8_t l8; // Low 8 bits
            uint8_t h8; // High 8 bits
        } b; // Byte reference
        
        uint16_t u16; // Unsigned 16bits
        int16_t s16; // Signed 16bits
    } UNION16;
    
    typedef union _UNION32
    { // 32 bits union, Little Endian
        struct
        { // 8 bits reference
            uint8_t l8;  // Low 8 bits
            uint8_t ml8; // Middle Low 8 bits
            uint8_t mh8; // Middle High 8 bits
            uint8_t h8;  // High 8 bits
        } b; // Byte reference
    
        struct
        { // 16 bits reference
            uint16_t l16; // Low 16 bits
            uint16_t h16; // High 16 bits
        } w; // Word reference
    
        uint32_t u32; // 32 bits unsigned reference
        int32_t s32; // 32 bits signed reference
    } UNION32;
    
    
    typedef union _UNION64
    { // 64 bits union, Big Endian
        struct
        { // 8 bits reference
            uint8_t l8;  // Low 8 bits
            uint8_t ml8; // Middle low 8 bits
            uint8_t mml8; // Middle low 8 bits
            uint8_t mmml8; // Middle low 8 bits
            uint8_t mmmh8; // Middle high 8 bits
            uint8_t mmh8; // Middle high 8 bits
            uint8_t mh8; // Middle high 8 bits
            uint8_t h8;  // High 8 bits
        } b; // Byte reference
    
        struct
        { // 16 bits reference
            uint16_t l16; // Low 16 bits
            uint16_t ml16; // Middle Low 16 bits
            uint16_t mh16; // Middle High 16 bits
            uint16_t h16; // High 16 bits
        } w; // Word reference
    
        struct
        { // 32 bits reference
            uint32_t l32; // Low 32 bits
            uint32_t h32; // High 32 bits
        } dw; // DWord reference
    
        uint64_t uint64_t; // 64 bits unsigned reference
        int64_t s64; // 64 bits signed reference
    } UNION64;
    
	#endif
    
    //==========================================================
    // Some common operations
    //==========================================================
#define SwapConst16(x) \
        ((uint16_t)((((uint16_t)(x) & 0xff00) >> 8) | (((uint16_t)(x) & 0x00ff) << 8)))
#define SwapConst32(x) \
        ((uint32_t)((((uint32_t)(x) & 0xff000000ul) >> 24) \
        | (((uint32_t)(x) & 0x00ff0000ul) >>  8) \
        | (((uint32_t)(x) & 0x0000ff00ul) <<  8) \
        | (((uint32_t)(x) & 0x000000fful) << 24)))
#define SwapConst64(x) \
        ((uint64_t)((((uint64_t)(x) & 0xff00000000000000ull) >> 56) \
        | (((uint64_t)(x) & 0x00ff000000000000ull) >> 40) \
        | (((uint64_t)(x) & 0x0000ff0000000000ull) >> 24) \
        | (((uint64_t)(x) & 0x000000ff00000000ull) >>  8) \
        | (((uint64_t)(x) & 0x00000000ff000000ull) <<  8) \
        | (((uint64_t)(x) & 0x0000000000ff0000ull) << 24) \
        | (((uint64_t)(x) & 0x000000000000ff00ull) << 40) \
        | (((uint64_t)(x) & 0x00000000000000ffull) << 56)))
        
	#ifdef _BIG_ENDIAN_
    
    //
    // Swap dat/const between CPU and Big-Endian system
    //
#define VarSwapBE16(x)   (x)
#define VarSwapBE32(x)   (x)
#define VarSwapBE64(x)   (x)
    
#define ConstSwapBE16(x) (x)
#define ConstSwapBE32(x) (x)
#define ConstSwapBE64(x) (x)
    
    //
    // Swap dat/const between CPU and Little-Endian system
    //
#define VarSwapLE16(x)   SwapVar16(x)
#define VarSwapLE32(x)   SwapVar32(x)
#define VarSwapLE64(x)   SwapVar64(x)
    
#define ConstSwapLE16(x) SwapConst16(x)
#define ConstSwapLE32(x) SwapConst32(x)
#define ConstSwapLE64(x) SwapConst64(x)
    
	#else
        
    //
    // Swap dat/const between CPU and Big-Endian system
    //
#define VarSwapBE16(x)   SwapVar16(x)
#define VarSwapBE32(x)   SwapVar32(x)
#define VarSwapBE64(x)   SwapVar64(x)
    
#define ConstSwapBE16(x) SwapConst16(x)
#define ConstSwapBE32(x) SwapConst32(x)
#define ConstSwapBE64(x) SwapConst64(x)
    
    //
    // Swap dat/const between CPU and Little-Endian system
    //
#define VarSwapLE16(x)   (x)
#define VarSwapLE32(x)   (x)
#define VarSwapLE64(x)   (x)
    
#define ConstSwapLE16(x) (x)
#define ConstSwapLE32(x) (x)
#define ConstSwapLE64(x) (x)
    
	#endif
    
#endif
    
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
/* 0x1234是为了防止pclint报错 */
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

#ifndef BYTES_PER_SECTOR
#define BYTES_PER_SECTOR  512
#endif

#ifndef BYTES_PER_SECTOR_SHIFT
#define BYTES_PER_SECTOR_SHIFT 9
#endif

#define BITS_PER_SECTOR_SHIFT (BYTES_PER_SECTOR_SHIFT + 3)
#define BITS_PER_SECTOR       (1 << BITS_PER_SECTOR_SHIFT)
        
#ifndef BYTES_PER_BLOCK_SHIFT
#define BYTES_PER_BLOCK_SHIFT 12
#endif

#ifndef BYTES_PER_BLOCK
#define BYTES_PER_BLOCK 4096
#endif

#ifndef SECTORS_PER_BLOCK_SHIFT
#define SECTORS_PER_BLOCK_SHIFT 3
#endif

#ifndef SECTORS_PER_BLOCK
#define SECTORS_PER_BLOCK (1 << SECTORS_PER_BLOCK_SHIFT)
#endif

#ifndef BITS_PER_BYTE_SHIFT
#define BITS_PER_BYTE_SHIFT 3
#endif


#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE (1 << BITS_PER_BYTE_SHIFT)
#endif

enum
{
    PID_INDEX = 10,
    PID_BLOCK = 11,
    PID_BITMAP = 12,

    PID_BUTT
};

//static uint32_t g_pid = PID_INDEX;
#define MODULE(pid)  static uint32_t g_pid = (pid)    

#endif /* End of __OS_ADAPTER_H__ */
