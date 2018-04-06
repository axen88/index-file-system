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

typedef enum
{
    BUF_TYPE_READ,
    BUF_TYPE_WRITE,
    
    
} BUF_TYPE_E;

// 数据块cache结构
typedef struct cache_node
{
    uint64_t block_id;    // 缓存的数据块
    uint32_t ref_cnt;     // 引用计数
    
    list_head_t node;   // 在read cache、checkin cache、write cache中登记
    
    struct cache_node *base;
    
    struct cache_node *left;
    struct cache_node *right;
    
    
} cache_node_t;

// cache下层所在空间的操作及参数
typedef struct
{
    // operations
    int (*read)(void *hnd, uint64_t offset, int size);
    int (*write)(void *hnd, uint64_t offset, int size);

    // argument
    void *hnd;
} space_ops_t;

// cache总体管理结构
typedef struct
{
    uint64_t tx_id;   // 事务id

    uint32_t block_size;

    hashtab hcache;    // 所有的数据块缓存在这都能快速找到

    // write_cache -> checkpoint_cache -> read_cache
    list_head_t read_cache;        // 只读cache，从盘上读到的未经修改过的数据
    list_head_t checkin_cache;  // 正在下盘的cache
    list_head_t write_cache;       // 正在修改的cache

    space_ops_t space_ops;
    
} cache_mgr_t;

#endif


