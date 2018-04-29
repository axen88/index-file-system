/*******************************************************************************

            Copyright(C), 2018~2021, axen.hook@foxmail.com
********************************************************************************
File Name: TX_CACHE.H
Author   : axen.hook
Version  : 1.00
Date     : 25/Mar/2018
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 25/Mar/2018
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#ifndef __TX_CACHE_H__
#define __TX_CACHE_H__

#include "dlist.h"
#include "hashtab.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    READ_BUF,
    CHECKPOINT_BUF,
    WRITE_BUF,
    
    BUF_TYPE_NUM
} BUF_TYPE_E;

// ���ݿ�cache�ṹ
typedef struct cache_node
{
    uint64_t block_id;    // ��������ݿ�
    uint32_t ref_cnt;     // ���ü���
    
    list_head_t node;   // ��read cache��checkin cache��write cache�еǼ�
    
    struct cache_node *read_node;  // ��ӦREAD_BUF
    struct cache_node *side_node[2];

    char dat[0];  // buffer
    
} cache_node_t;

// cache�²����ڿռ�Ĳ���������
typedef struct
{
    // operations
    int (*read)(void *hnd, uint64_t offset, int size);
    int (*write)(void *hnd, uint64_t offset, int size);

    // argument
    void *hnd;
} space_ops_t;

// cache�������ṹ
typedef struct
{
    uint64_t tx_id;   // ����id

    void *bd_hnd;  // block device handle

    uint32_t block_size;

    uint8_t  writing_side;   //  0 or 1, 

    hashtab_t hcache;    // ���е����ݿ黺�����ⶼ�ܿ����ҵ�

    // write_cache -> checkpoint_cache -> read_cache
    list_head_t read_cache;        // ֻ��cache�������϶�����δ���޸Ĺ�������
    list_head_t checkin_cache;  // �������̵�cache
    list_head_t write_cache;       // �����޸ĵ�cache

    space_ops_t space_ops;
    
} cache_mgr_t;

#ifdef __cplusplus
}
#endif

#endif


