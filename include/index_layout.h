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
File Name: INDEX_LAYOUT.H
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
#ifndef __INDEX_LAYOUT_H__
#define __INDEX_LAYOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION             100         /* version: 1.00 */
#define SUPER_BLOCK_ID      0x4B4C4253  // "SBLK"
#define BLOCK_MAGIC_NUMBER  0xAA55      /* 0x55, 0xAA */
#define SUPER_BLOCK_VBN     ((uint64_t)0)
#define SUPER_BLOCK_SIZE    512         

/* flags in BLOCK_BOOT_SECTOR_S */
#define FLAG_FIXUP       0x00000001     /* need fixup */

#define INODE_MAGIC                0x454A424F   // "OBJE"
#define INDEX_MAGIC                0x58444E49   // "INDX"

#define ENTRY_END_SIZE    sizeof(INDEX_ENTRY)
#define ENTRY_BEGIN_SIZE  sizeof(INDEX_ENTRY)

#define PRV_AREA_SIZE          256       
#define FILE_NAME_SIZE         256
#define OBJ_NAME_SIZE          256

#define DEFAULT_OBJ_NAME       "NO_NAME"
#define DEFAULT_OBJ_NAME_SIZE  (sizeof(DEFAULT_OBJ_NAME) - 1)

/* flags */
#define FLAG_SYSTEM        0x8000 /* 1: system attr  0: non-system attr */
#define FLAG_TABLE         0x4000 /* 1: table        0: data stream */

typedef struct _index_extent
{
    uint64_t addr;  // start address
    uint64_t len;         // length
} index_extent_t;

typedef struct _extent_pair
{
    uint8_t addr_size;
    uint8_t len_size;
    //uint8_t addr[addr_size];
    //uint8_t len[len_size];
} extent_pair_t;

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

#define BLOCK_SIZE            (4 * 1024)
#define INODE_SIZE            (2 * 1024)


#define INODE_RESERVED_SIZE  1686
#define INODE_HEAD_SIZE      OS_OFFSET(INODE_RECORD, reserved)

#define OBJID_OBJ_NAME            "$OBJID"
#define FREEBLK_OBJ_NAME          "$FREEBLK"
#define OBJID_OBJ_ID              0ULL
#define FREEBLK_OBJ_ID            1ULL
#define RESERVED_OBJ_ID           256ULL

#define ATTR_RECORD_HEAD_SIZE       OS_OFFSET(ATTR_RECORD, content)

#define ATTR_RECORD_SIZE           (INODE_RESERVED_SIZE)
#define ATTR_RECORD_CONTENT_SIZE   (ATTR_RECORD_SIZE - ATTR_RECORD_HEAD_SIZE)

#define INODE_GET_ATTR_RECORD(inode)  ((ATTR_RECORD *)((uint8_t *)(inode) + (inode)->first_attr_off))

#define KEY_MAX_SIZE    256
#define VALUE_MAX_SIZE  1024

// Index Entry ucFlags, bits field
#define INDEX_ENTRY_LEAF          0x00
#define INDEX_ENTRY_NODE          0x01
#define INDEX_ENTRY_END           0x02
#define INDEX_ENTRY_BEGIN         0x04

// Index Block ucFlags, bits field
#define INDEX_BLOCK_SMALL         0x00  // The block has no child block
#define INDEX_BLOCK_LARGE         0x01  // The block has child block

#define VBN_SIZE                  sizeof(uint64_t)

#define GET_FIRST_IE(ib)     ((INDEX_ENTRY *)((uint8_t*)(ib) + ((INDEX_BLOCK *)(ib))->first_entry_off))
#define GET_END_IE(ib)       ((uint8_t *)(ib) + ((INDEX_BLOCK *)(ib))->head.real_size)
#define GET_NEXT_IE(ie)      ((INDEX_ENTRY *)((uint8_t*)(ie) + ((INDEX_ENTRY *)(ie))->len))
#define GET_PREV_IE(ie)      ((INDEX_ENTRY *)((uint8_t*)(ie) - ((INDEX_ENTRY *)(ie))->prev_len))
#define GET_IE_VBN(ie)       (*(uint64_t*)((uint8_t *)(ie)+ (((INDEX_ENTRY *)(ie))->len - VBN_SIZE)))
#define SET_IE_VBN(ie, vbn)  (GET_IE_VBN(ie) = vbn)
#define GET_IE_KEY(ie)       ((uint8_t*)(ie) + sizeof(INDEX_ENTRY))
#define GET_IE_VALUE(ie)     ((uint8_t*)(ie) + sizeof(INDEX_ENTRY) + (((INDEX_ENTRY *)(ie))->key_len))



#pragma pack(1) /* aligned by 1 byte */

typedef struct tagOBJECT_HEADER_S
{
    uint32_t blk_id;              /* obj id */
    uint32_t alloc_size;          /* allocated size */
    uint32_t real_size;           /* real size */
    uint16_t seq_no;              /* update sequence no. */
    uint16_t fixup;               /* do integrity verify */
} OBJECT_HEADER_S;

/* super block  */
typedef struct tagBLOCK_BOOT_SECTOR_S
{
    OBJECT_HEADER_S head;               /* header */

    uint32_t block_size_shift;          
    uint32_t block_size;                /* by bytes */
    uint32_t sectors_per_block;         /* by sectors */

    uint32_t bitmap_blocks;             /* bitmap blocks */
    uint64_t bitmap_start_block;        /* start block of bitmap */

    uint64_t total_blocks;              /* total blocks = reserved blocks + bitmap blocks + data blocks */

    uint64_t free_blocks;               /* total free blocks */
    uint64_t first_free_block;          /* first possible free block */

    uint64_t start_lba;                 /* super block's lba */
    
    uint64_t objid_inode_no;
    uint64_t objid_id;

    uint64_t free_blk_inode_no;
    uint64_t free_blk_id;
    
    uint64_t snapshot_no;
    uint8_t aucReserved2[PRV_AREA_SIZE - 24];    // Reserved bytes
    uint8_t aucReserved[160];            
    uint32_t flags;                     /* flags */
    uint16_t version;                   /* version */
    uint16_t magic_num;                 /* 0x55AA */
} BLOCK_BOOT_SECTOR_S;

typedef struct _INDEX_ENTRY
{
    uint16_t len;               // Byte size of this index entry
    uint16_t prev_len;          // Byte size of this index entry

    uint16_t key_len;           // Byte size of the key, no this field in INDEX_ENTRY_END entry
    uint16_t value_len;         // Byte size of the c, no this field in INDEX_ENTRY_END entry

    uint8_t flags;              // Bits field of INDEX_ENTRY ucFlags
    
    //uint8_t key[];            // The key
    //uint8_t value[];          // The value
    //uint64_t vbn;             // Virtual index number of child index block
} INDEX_ENTRY;

typedef struct _INDEX_BLOCK
{
    OBJECT_HEADER_S head;

    uint16_t first_entry_off;       // Byte offset to first INDEX_ENTRY
    uint8_t node_type;              // The ucFlags of current index block
    uint8_t padding[5];             // Reserved/align to 8-byte boundary

    INDEX_ENTRY begin_entry;
    //INDEX_ENTRY entries;
} INDEX_BLOCK;

#define ATTR_RECORD_MAX_SIZE  INODE_RESERVED_SIZE

typedef struct _ATTR_RECORD
{
    uint16_t record_size; // record size, include this head
    uint16_t flags;        // table/stream, resident/non-resident
    //uint8_t  cr;          // collate rule, high 4bits: value cr, low 4bits: key cr
    uint64_t attr_size;   // attr size

    uint8_t content[ATTR_RECORD_MAX_SIZE];  // the attr record content
} ATTR_RECORD;

typedef struct _INODE_RECORD
{                               /* total 2KB bytes */
    /* 0 */
    OBJECT_HEADER_S head;

    /* 16 */
    uint16_t first_attr_off;
    uint8_t  paddings[6];

    /* 24 */
    uint64_t objid;              // object id
    uint64_t base_objid;         // base object id

    /* 40 */
    uint64_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t links;
    uint64_t ctime;
    uint64_t atime;
    uint64_t mtime;

    /* 96 */
    uint64_t snapshot_no;

    /* 104 */
    uint16_t name_size;
    char name[OBJ_NAME_SIZE];      // The tree name

    /* 616 */
    uint8_t reserved[INODE_RESERVED_SIZE]; /* make the inode to 2KB length */
} INODE_RECORD;

#pragma pack()  // Resume to default for performance

#ifdef __cplusplus
}
#endif

#endif

