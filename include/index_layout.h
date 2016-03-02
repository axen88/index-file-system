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

            Copyright(C), 2016~2019, axen2012@qq.com
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

#define VERSION             100         /* �汾��: V1.00 */
#define SUPER_BLOCK_ID      0x4B4C4253  /* �ַ���: SBLK */
#define BLOCK_MAGIC_NUMBER  0xAA55      /* 0x55, 0xAA */
#define SUPER_BLOCK_VBN     ((uint64_t)0)
#define SUPER_BLOCK_SIZE    512         /* �������С */

/* �ṹ��BLOCK_BOOT_SECTOR_S��uiFlags����λ�ĺ��� */
#define FLAG_FIXUP       0x00000001     /* �Ƿ���Ҫ�޸���� */

#define INODE_MAGIC                0x45444F4E   // "NODE"
#define INDEX_MAGIC                0x58444E49   // "INDX"

#define ENTRY_END_SIZE    sizeof(INDEX_ENTRY)
#define ENTRY_BEGIN_SIZE  sizeof(INDEX_ENTRY)

#define PRV_AREA_SIZE       256         /* ˽�������С */
#define FILE_NAME_SIZE            256
#define OBJ_NAME_SIZE            512

#define DEFAULT_OBJ_NAME       "NO_NAME"
#define DEFAULT_OBJ_NAME_SIZE  (sizeof(DEFAULT_OBJ_NAME) - 1)

/* attr_flags�е��ֶζ��� */
#define FLAG_SYSTEM        0x8000 /* 1: system attr  0: non-system attr */
#define FLAG_TABLE         0x4000 /* 1: table        0: data stream */

#define COLLATE_BINARY          0x0000
#define COLLATE_ANSI_STRING     0x0001
#define COLLATE_UNICODE_STRING  0x0002

#define COLLATE_RULE_MASK       0x000F
#define COLLATE_RULE_MASK2      0x0007
#define COLLATE_REPEAT          0x0008

#define BLOCK_SIZE            (4 * 1024)
#define INODE_SIZE            (2 * 1024)


#define INODE_RESERVED_SIZE  1430
#define INODE_HEAD_SIZE      OS_OFFSET(INODE_RECORD, reserved)

#define OBJID_OBJ_NAME            "$OBJID"
#define OBJID_OBJ_ID              0ULL

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

#define IBGetFirst(ib)     ((INDEX_ENTRY *)((uint8_t*)(ib) + ((INDEX_BLOCK *)(ib))->first_entry_off))
#define IBGetEnd(ib)       ((uint8_t *)(ib) + ((INDEX_BLOCK *)(ib))->head.real_size)
#define IEGetNext(ie)      ((INDEX_ENTRY *)((uint8_t*)(ie) + ((INDEX_ENTRY *)(ie))->len))
#define IEGetPrev(ie)      ((INDEX_ENTRY *)((uint8_t*)(ie) - ((INDEX_ENTRY *)(ie))->prev_len))
#define IEGetVBN(ie)       (*(uint64_t*)((uint8_t *)(ie)+ (((INDEX_ENTRY *)(ie))->len - VBN_SIZE)))
#define IESetVBN(ie, vbn)  (IEGetVBN(ie) = vbn)
#define IEGetKey(ie)       ((uint8_t*)(ie) + sizeof(INDEX_ENTRY))
#define IEGetValue(ie)     ((uint8_t*)(ie) + sizeof(INDEX_ENTRY) + (((INDEX_ENTRY *)(ie))->key_len))



#pragma pack(1) /* ��1�ֽڶ��� */

typedef struct tagOBJECT_HEADER_S
{
    uint32_t blk_id;              /* ����id */
    uint32_t alloc_size;          /* ����ռ�õĴ�С */
    uint32_t real_size;           /* ����ʵ�ʵĴ�С */
    uint16_t seq_no;              /* ��ǰ������µ���� */
    uint16_t fixup;            /* ����������������У�� */
} OBJECT_HEADER_S;

/* �����鶨�� */
typedef struct tagBLOCK_BOOT_SECTOR_S
{
    OBJECT_HEADER_S head;           /* ���������ͷ */

    uint32_t block_size_shift;           /* ���ֽ�Ϊ��λ�Ŀ��С���� */
    uint32_t block_size;                /* ���С�����ֽ�Ϊ��λ */
    uint32_t sectors_per_block;          /* ���С��������Ϊ��λ */

    uint32_t bitmap_blocks;             /* λͼ����ռ�õĿ���Ŀ */
    uint64_t bitmap_start_block;        /* λͼ�������ʼ��� */

    uint64_t total_blocks;             /* �ܿ���=��������Ŀ+λͼ����Ŀ+���ݿ���Ŀ */

    uint64_t free_blocks;              /* �����ݿ���Ŀ */
    uint64_t first_free_block;          /* ��һ������Ϊ�յ����ݿ� */

    uint64_t start_lba;                /* super block���ڵ�lbaλ�� */
    
    uint64_t objid_inode_no;
    uint64_t objid_id;
    
    uint64_t snapshot_no;
    uint8_t aucReserved2[PRV_AREA_SIZE - 24];    // Reserved bytes
    uint8_t aucReserved[176];            /* �������򣬹�ϵͳ��չʹ�� */
    uint32_t flags;                    /* �����ϵͳ��һЩ��ʶ */
    uint16_t version;                  /* �汾�� */
    uint16_t magic_num;                 /* ��ʽ����ʶ: 0x55AA */
} BLOCK_BOOT_SECTOR_S;

typedef struct _INDEX_ENTRY
{
    uint16_t len;              // Byte size of this index entry
    uint16_t prev_len;          // Byte size of this index entry

    uint16_t key_len;           // Byte size of the key, no this field in INDEX_ENTRY_END entry
    uint16_t value_len;         // Byte size of the c, no this field in INDEX_ENTRY_END entry

    uint8_t flags;             // Bits field of INDEX_ENTRY ucFlags
    
    //OS_U8 key[];         // The key
    //OS_U8 c[];       // The c
    //OS_U64 ullVBN;      // Virtual index number of child index block
} INDEX_ENTRY;

typedef struct _INDEX_BLOCK
{
    /* ͷ�� */
    OBJECT_HEADER_S head;

    uint16_t first_entry_off;       // Byte offset to first INDEX_ENTRY
    uint8_t node_type;             // The ucFlags of current index block
    uint8_t padding[5];       // Reserved/align to 8-byte boundary

    INDEX_ENTRY begin_entry;
    //INDEX_ENTRY entries;
} INDEX_BLOCK;

#define ATTR_RECORD_MAX_SIZE  INODE_RESERVED_SIZE

typedef struct _ATTR_RECORD
{
    uint16_t record_size; /* �˼�¼�ĳ��� */
    uint16_t attr_flags;   /* ��פ/�ǳ�פ, ��/�� */
    uint64_t attr_size;   /* ���Դ�С */

    uint8_t content[ATTR_RECORD_MAX_SIZE];  /* ��פ�������ݡ���Ŀ¼����ĸ� */
} ATTR_RECORD;

typedef struct _INODE_RECORD
{                               /* total 2KB bytes */
    /* ͷ�� 0 */
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
    char name[OBJ_NAME_SIZE];      // The tree acName

    /* 616 */
    uint8_t reserved[INODE_RESERVED_SIZE]; /* make the inode to 2KB length */
} INODE_RECORD;

#pragma pack()  // Resume to default for performance

#ifdef __cplusplus
}
#endif

#endif

