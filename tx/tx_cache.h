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
    ERR_TX_READ_DISK = 1,
    //ERR_NO_MEMORY,
    ERR_CACHE_STATE,

    ERR_TX_NUM
} TX_ERROR_E;

// 对同一个block id可以获取不同的buf
typedef enum
{
    RW_BUF,          // 读写buf，用户可直接访问的buf；可读、可写、可修改
    COMMIT_BUF,      // 提交buf，这里的内容肯定已经在日志中了；仅供内部修改
    FLUSH_BUF,       // 刷盘buf，供刷盘用，这里的脏数据会写到盘上对应的位置；仅供内部修改
    
    BUF_TYPE_NUM
} BUF_TYPE_E;

// flag
#define F_NO_READ   (1 << 0) // 不需要读盘上的数据，默认数据为全0

// buf的状态
typedef enum
{
    EMPTY,   // 表明buf中无内容
    CLEAN,   // 表明buf有内容，并且未修改过
    DIRTY    // 表明buf中的内容修改过
} BUF_STATE_E;

// cache下层所在空间的操作接口
typedef struct
{ // operations
    char *ops_name;
    
    int (*open)(void **hnd, char *bd_name);
    int (*read)(void *hnd, void *buf, int size, u64_t offset);
    int (*write)(void *hnd, void *buf, int size, u64_t offset);
    void (*close)(void *hnd);
} space_ops_t;


// 数据块cache结构
typedef struct cache_block
{
    u64_t    block_id;            // 缓存的数据块
    uint32_t ref_cnt;             // 引用计数
    u64_t    owner_tx_id;         // 写cache时，只有一个事务能拥有
    
    BUF_STATE_E state;            // TODO
    
    hashtab_node_t hnode;         // 在hashtab中登记
    list_head_t    node;          // 在tx中登记
    
    struct cache_block *pp_cb[2]; // 记录commit cache和flush/checkpoint cache，采用pingping的方式进行切换
    
    char buf[0];                  //  buffer store data
} cache_block_t;

// cache总体管理结构
typedef struct
{
    // id
    u64_t flush_sn;         // 刷盘的次数sequence number(sn)
    u64_t log_sn;           // 日志下盘序号
    u64_t cur_tx_id;        // 供分配事务时唯一标记事务，初始值为1
    u64_t checkpoint_sn;    // checkpoint sequence number

    uint32_t block_size;    // 此cache管理结构的cache粒度   或  块设备的块大小?

    uint8_t  commit_side;   //  表明当前写cache是哪个，pingpong方式切换
    
    uint32_t onfly_commit_tx;       // 正在提交的事务数目
    uint32_t onfly_tx_num;          // 正在进行还未提交或cancel的事务数目
    
    // 正在修改的统计信息，以检查是否达到flush条件
    uint32_t modified_block_num;    // commit cb中已修改的块数目
    u64_t    first_modified_time;   // 切flush后第一次修改的时间，ms
    uint32_t modified_data_bytes;   // 已修改的数据量
    
    // flush的条件
    uint32_t max_modified_blocks;   // 修改的块数目达到这个值
    uint32_t max_time_interval;     // 到了一定的时间间隔，强制flush
    uint32_t max_modified_bytes;    // 修改的数据量达到这个值

    // 如果flush还未刷完盘，又有事务要推进，此时需阻止新事务提交
    uint8_t allow_commit_tx;        // 是否允许提交事务
    uint8_t allow_new_tx;           // 是否允许生成新事务

    // 正在flush的信息
    uint32_t flush_block_num;       // 需要下盘的块数目

    hashtab_t *hcache;              // 所有的数据块缓存在这都能快速找到

    // write_cache -> flush_cache -> read_cache
    //list_head_t read_cache;        // 只读cache，从盘上读到的未经修改过的数据
    //list_head_t checkin_cache;     // 正在下盘的cache


    // 块设备操作
    void *bd_hnd;         // block device handle
    space_ops_t *bd_ops;  // block device operations
} cache_mgr_t;

// 事务
typedef struct
{
    cache_mgr_t *mgr;

    uint32_t tx_id;     // 此事务的id
    list_head_t rw_cb;  // 本事务修改过或正在修改的cache，commit或cancel时使用
} tx_t;


// 分配一个新的事务
int tx_alloc(cache_mgr_t *mgr, tx_t **new_tx);

// 带事务修改时，调用这个接口
void *tx_get_buffer(tx_t *tx, u64_t block_id, uint32_t flag);

// 标记tx buffer dirty
void tx_mark_buffer_dirty(tx_t *tx, void *tx_buf);

// 带事务修改时，调用这个接口
void tx_put_buffer(tx_t *tx, void *tx_buf);

// 提交修改的数据到日志，此时写cache中的数据还未下盘
int tx_commit(tx_t *tx);

// 放弃这个事务的所有修改
void tx_cancel(tx_t *tx);


// 初始化cache系统
cache_mgr_t *tx_cache_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops);

// 退出cache系统
void tx_cache_exit_system(cache_mgr_t *mgr);


// 必须和put_buffer、commit_buffer、cancel_buffer配合使用
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, uint32_t mode);


// 必须和put_buffer、commit_buffer、cancel_buffer配合使用
void *get_buffer_by_type(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type);

// 必须和get_buffer配合使用
void put_buffer(cache_mgr_t *mgr, void *buf);

// 标记buffer dirty
void mark_buffer_dirty(cache_mgr_t *mgr, void *rw_buf);

// commit buffer
int32_t commit_buffer(cache_mgr_t *mgr, void *rw_buf);

// cancel buffer
void cancel_buffer(cache_mgr_t *mgr, void *rw_buf);

// checkpoint, 也就是将commit cache和flush cache交换
void checkpoint_all_cache_block(cache_mgr_t *mgr);

// 将当前mgr中所有的flush cache的内容下盘
int flush_all_cb(cache_mgr_t *mgr);

// 将所有flush buffer中的脏数据下盘
int flush_all_cache_block(cache_mgr_t *mgr);




//extern space_ops_t bd_ops;

#ifdef __cplusplus
}
#endif

#endif


