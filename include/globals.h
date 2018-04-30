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
File Name: GLOBALS.H
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifdef	__cplusplus
extern "C" {
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

enum
{
    PID_BTREE = 10,
    PID_CONTAINER = 11,
    PID_OBJECT = 12,
    PID_BLOCK = 13,
    PID_SPACE_MANAGER = 14,
    PID_CACHE = 15,
    PID_SERVER = 16,
    PID_CLIENT = 17,
    PID_TOOLS = 18,
    PID_UTILS = 19,
    PID_EXTENT_MAP = 20,

    PID_BUTT
};

typedef enum error_code
{
    // INDEX errors
    INDEX_ERR_START = 400000,
    INDEX_ERR_SEEK,
    INDEX_ERR_READ,
    FILE_INDEX_TREE_NOT_FOUND,
    INDEX_ERR_OBJ_EXIST,
    FILE_INDEX_TREE_NO_SPACE,
    INDEX_ERR_OBJECT_NAME_SIZE,
    INDEX_ERR_WRITE,
    INDEX_ERR_0BIT_NOT_FOUND,
    INDEX_ERR_EXCEPTION,
    INDEX_ERR_PARAMETER,
    INDEX_ERR_ATTR_RESIDENT,
    INDEX_ERR_ALLOCATE_MEMORY,
    INDEX_ERR_MANY_TIMES_PUT,
    INDEX_ERR_BLOCK_MAGIC,
    INDEX_ERR_NO_CONTENT,
    INDEX_ERR_NO_FREE_BLOCKS,
    INDEX_ERR_UPDATE,
    INDEX_ERR_FLUSH,
    INDEX_ERR_MAX_DEPTH,
    INDEX_ERR_ROOT,
    INDEX_ERR_OPEN,
    INDEX_ERR_ATTR_NOT_FOUND,
    INDEX_ERR_BOOT_RECORD,
    INDEX_ERR_RESET,
    INDEX_ERR_END_ENTRY,
    INDEX_ERR_NEXT_ENTRY,
    INDEX_ERR_PREV_ENTRY,
    INDEX_ERR_CHILD_NODE,
    INDEX_ERR_PARENT_NODE,
    INDEX_ERR_COLLATE,
    INDEX_ERR_KEY_NOT_FOUND,
    INDEX_ERR_KEY_EXIST,
    INDEX_ERR_INSERT_ENTRY,
    INDEX_ERR_REMOVE_ENTRY,
    INDEX_ERR_ADD_VBN,
    INDEX_ERR_DEL_VBN,
    INDEX_ERR_CHAOS,
    INDEX_ERR_GET_ENTRY,
    INDEX_ERR_WALK_ENTRY,
    INDEX_ERR_REPARENT,
    INDEX_ERR_DELETE_BLOCK,
    INDEX_ERR_REAL_SIZE,
    INDEX_ERR_NODE_TYPE,
    INDEX_ERR_BLOCK_CHECK,
    INDEX_ERR_BLOCK_UNUSED,
    INDEX_ERR_FORMAT,
    INDEX_ERR_IS_OPENED,
    INDEX_ERR_NOT_OPENED,
    INDEX_ERR_OBJ_ID_INVALID,

    ERR_PUT_BUF,

    ERR_FILE_READ,
    ERR_FILE_WRITE,

    
    INDEX_ERR_BUTT
} error_code_t;

#ifdef	__cplusplus
}
#endif

#endif
