/*******************************************************************************

            Copyright(C), 2018~2021, axen.hook@foxmail.com
********************************************************************************
File Name: TX_LAYOUT.H
Author   : axen.hook
Version  : 1.00
Date     : 29/Apr/2018
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 29/Apr/2018
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/
#ifndef __TX_LAYOUT_H__
#define __TX_LAYOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1) /* aligned by 1 byte */

typedef struct block_head
{
    uint32_t blk_id;              /* obj id */
    uint32_t alloc_size;          /* allocated size */
    uint32_t real_size;           /* real size */
    uint16_t seq_no;              /* update sequence no. */
    uint16_t fixup;               /* do integrity verify */
} block_head_t;



/*
 * Standard header for all descriptor blocks:
 */
typedef struct journal_header_s
{
	uint32_t		magic;
	uint32_t		blocktype;
	uint32_t		sequence;
} journal_header_t;

/*
 * Checksum types.
 */
#define JBD2_CRC32_CHKSUM   1
#define JBD2_MD5_CHKSUM     2
#define JBD2_SHA1_CHKSUM    3
#define JBD2_CRC32C_CHKSUM  4

#define JBD2_CRC32_CHKSUM_SIZE 4

#define JBD2_CHECKSUM_BYTES (32 / sizeof(uint32_t))

struct commit_header {
	uint32_t		h_magic;
	uint32_t          h_blocktype;
	uint32_t          h_sequence;
	unsigned char   h_chksum_type;
	unsigned char   h_chksum_size;
	unsigned char 	h_padding[2];
	uint32_t 		h_chksum[JBD2_CHECKSUM_BYTES];
	__be64		h_commit_sec;
	uint32_t		h_commit_nsec;
};


/*
 * The journal superblock.  All fields are in big-endian byte order.
 */
typedef struct journal_superblock_s
{
    block_head_t header;

    uint32_t blocksize;        /* journal device blocksize */
    uint32_t total_blocks;     /* total blocks in journal file */
    uint32_t first_block;      /* first block of log information */

    /* Dynamic information describing the current state of the log */
    uint32_t    start_commitid;     /* first commit ID expected in log */
    uint32_t    start_block;        /* blocknr of start of log */

    uint32_t    errno;

    uint8_t     uuid[16];        /* 128-bit uuid for journal */
    
    u64_t objid;

    uint32_t    users_num;        /* Nr of filesystems sharing log */

    uint32_t    sb_num;        /* Blocknr of dynamic superblock copy*/

    uint32_t    max_transaction;    /* Limit of journal blocks per trans.*/
    uint32_t    max_trans_data;    /* Limit of data blocks per trans. */

    uint8_t    checksum_type;    /* checksum type */
    uint8_t    s_padding2[3];
    uint32_t    s_padding[42];
    uint32_t    s_checksum;        /* crc32c(superblock) */

    uint8_t    s_users[16*48];        /* ids of all fs'es sharing the log */
} journal_superblock_t;


#pragma pack()  // Resume to default for performance


#ifdef __cplusplus
}
#endif



#endif
