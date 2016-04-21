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
File Name: OFS_LAYOUT.H
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
#ifndef __OFS_LAYOUT_H__
#define __OFS_LAYOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION             100         /* version: 1.00 */
#define SUPER_BLOCK_ID      0x4B4C4253  // "SBLK"
#define BLOCK_MAGIC_NUMBER  0xAA55      /* 0x55, 0xAA */
#define SUPER_BLOCK_SIZE    512         

#define INODE_MAGIC                0x454A424F   // "OBJE"
#define INDEX_MAGIC                0x58444E49   // "INDX"

#define PRV_AREA_SIZE              256       
#define OBJ_NAME_MAX_SIZE          256

/* flags */
#define FLAG_SYSTEM        0x8000 /* 1: system attr  0: non-system attr */
#define FLAG_TABLE         0x4000 /* 1: table        0: data stream */

#define BLOCK_SIZE            (4 * 1024)
#define INODE_SIZE            (4 * 1024)

#define INODE_RESERVED_SIZE  (1686 + 2048)
#define INODE_HEAD_SIZE      OS_OFFSET(inode_record_t, reserved)

#define BASE_OBJ_NAME             "$BASE"
#define SPACE_OBJ_NAME            "$SPACE"
#define OBJID_OBJ_NAME            "$OBJID"

#define SUPER_BLOCK_VBN        0
#define BASE_OBJ_INODE         1
#define SPACE_OBJ_INODE        2

#define BASE_OBJ_ID               0ULL
#define SPACE_OBJ_ID              1ULL
#define OBJID_OBJ_ID              2ULL
#define SPECIAL_OBJ_ID            10ULL  // The inode_no is recorded in super block if objid < SPECIAL_OBJ_ID
#define RESERVED_OBJ_ID           128ULL

#define ATTR_RECORD_HEAD_SIZE       OS_OFFSET(attr_record_t, content)

#define ATTR_RECORD_SIZE           (INODE_RESERVED_SIZE)
#define ATTR_RECORD_CONTENT_SIZE   (ATTR_RECORD_SIZE - ATTR_RECORD_HEAD_SIZE)

#define INODE_GET_ATTR_RECORD(inode)  ((attr_record_t *)((uint8_t *)(inode) + (inode)->first_attr_off))


#pragma pack(1) /* aligned by 1 byte */

typedef struct block_head
{
    uint32_t blk_id;              /* obj id */
    uint32_t alloc_size;          /* allocated size */
    uint32_t real_size;           /* real size */
    uint16_t seq_no;              /* update sequence no. */
    uint16_t fixup;               /* do integrity verify */
} block_head_t;

/* super block  */
typedef struct ifs_super_block
{
    block_head_t head;               /* header */

    uint32_t block_size_shift;          
    uint32_t block_size;                /* by bytes */
    uint32_t sectors_per_block;         /* by sectors */

    uint64_t total_blocks;              /* total blocks = reserved blocks + bitmap blocks + data blocks */

    uint64_t objid_id;
    uint64_t objid_inode_no;

    uint64_t space_id;
    uint64_t space_inode_no;
    uint64_t free_blocks;               /* total free blocks */
    uint64_t first_free_block;          /* first possible free block */

    uint64_t base_id;
    uint64_t base_inode_no;
    uint64_t base_free_blocks;               /* total free blocks */
    uint64_t base_first_free_block;          /* first possible free block */
    
    uint64_t base_blk;
    
    uint64_t snapshot_no;
    uint8_t aucReserved2[PRV_AREA_SIZE - 64 + 20];    // Reserved bytes
    uint8_t aucReserved[160];            
    uint32_t flags;                     /* flags */
    uint16_t version;                   /* version */
    uint16_t magic_num;                 /* 0x55AA */
} ofs_super_block_t;

typedef struct index_entry
{
    uint16_t len;               // Byte size of this ct entry
    uint16_t prev_len;          // Byte size of this ct entry

    uint16_t key_len;           // Byte size of the key, no this field in INDEX_ENTRY_END entry
    uint16_t value_len;         // Byte size of the c, no this field in INDEX_ENTRY_END entry

    uint8_t flags;              // Bits field of index_entry_t ucFlags
    
    //uint8_t key[];            // The key
    //uint8_t value[];          // The value
    //uint64_t vbn;             // Virtual ct number of child ct block
} index_entry_t;

typedef struct index_block
{
    block_head_t head;

    uint16_t first_entry_off;       // Byte offset to first index_entry_t
    uint8_t node_type;              // The ucFlags of current ct block
    uint8_t padding[5];             // Reserved/align to 8-byte boundary

    index_entry_t begin_entry;
    //index_entry_t entries;
} index_block_t;

#define ATTR_RECORD_MAX_SIZE  INODE_RESERVED_SIZE

typedef struct attr_record
{
    uint16_t record_size; // record size, include this head
    uint16_t flags;        // table/stream, resident/non-resident
    //uint8_t  cr;          // collate rule, high 4bits: value cr, low 4bits: key cr
    uint64_t attr_size;   // attr size

    uint8_t content[ATTR_RECORD_MAX_SIZE];  // the attr record content
} attr_record_t;

typedef struct inode_record
{                               /* total 2KB bytes */
    /* 0 */
    block_head_t head;

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
    char name[OBJ_NAME_MAX_SIZE];      // The tree name

    /* 616 */
    uint8_t reserved[INODE_RESERVED_SIZE]; /* make the inode to 2KB length */
} inode_record_t;

#pragma pack()  // Resume to default for performance

#ifdef __cplusplus
}
#endif

#endif

