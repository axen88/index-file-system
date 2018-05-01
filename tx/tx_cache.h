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
    FOR_READ,
    CHECKPOINT_BUF,
    FOR_WRITE,
    
    BUF_TYPE_NUM
} BUF_TYPE_E;

typedef enum
{
    EMPTY,
    CLEAN,
    DIRTY
} BUF_STATE_E;

// 数据块cache结构
typedef struct cache_node
{
    u64_t block_id;    // 缓存的数据块
    uint32_t ref_cnt;     // 引用计数
    
    BUF_STATE_E state;  // TODO

    u64_t owner_tx_id;  // 写cache时，只有一个事务能拥有
    
    list_head_t node;   // 在tx的write cache链表中登记
    
    struct cache_node *read_cache;  // 如果本cache是读cache，则为空；否则为读cache
    struct cache_node *side_cache[2];  // 记录checkpoint cache和write cache，采用pingping的方式进行切换

    char dat[0];  // buffer
    
} cache_node_t;

// cache下层所在空间的操作接口
typedef struct
{ // operations
    char *ops_name;
    
    int (*open)(void **hnd, char *bd_name);
    int (*read)(void *hnd, void *buf, int size, u64_t offset);
    int (*write)(void *hnd, void *buf, int size, u64_t offset);
    void (*close)(void *hnd);
} space_ops_t;

// cache总体管理结构
typedef struct
{
    // id
    u64_t checkpoint_sn;   // 刷盘的次数sequence number(sn)
    u64_t cur_tx_id;        // 供分配事务时唯一标记事务，初始值为1


    uint32_t block_size; // 此cache管理结构的cache粒度   或  块设备的块大小?

    // 表明当前写cache是哪个，pingpong方式切换
    uint8_t  write_side;   //  0 or 1, 
    
    // 正在修改的信息
    uint32_t onfly_tx_num; // 正在进行的事务数目
    
    // 正在修改的统计信息，以检查是否达到checkpoint条件
    uint32_t modified_block_num;  // 已修改的块数目
    u64_t first_modified_time;    // 切checkpoint后第一次修改的时间，ms
    uint32_t modified_data_bytes; // 已修改的数据量
    
    // checkpoint的条件
    uint32_t max_modified_blocks;   // 修改的块数目达到这个值
    uint32_t max_time_interval;     // 到了一定的时间间隔，强制checkpoint
    uint32_t max_modified_bytes;    // 修改的数据量达到这个值

    // 如果checkpoint还未刷完盘，又有事务要推进，此时需阻止新事务生成
    uint8_t allow_new_tx;   // 是否允许新事物生成

    // 正在checkpoint的信息
    uint32_t checkpoint_block_num; // 需要下盘的块数目

    hashtab_t *hcache;    // 所有的数据块缓存在这都能快速找到

    // write_cache -> checkpoint_cache -> read_cache
    //list_head_t read_cache;        // 只读cache，从盘上读到的未经修改过的数据
    //list_head_t checkin_cache;  // 正在下盘的cache


    // 块设备操作
    void *bd_hnd;  // block device handle
    space_ops_t *bd_ops;  // block device operations
    
} cache_mgr_t;

// 事务
typedef struct
{
    cache_mgr_t *mgr;

    uint32_t tx_id;  // 此事务的id
    
    list_head_t write_cache;  // 本事务修改过或正在修改的cache，回滚时使用

    
} tx_t;



// 开始一个新的事务
int tx_new(cache_mgr_t *mgr, tx_t **new_tx);

// 带事务修改时，调用这个接口，调用后不用put，在tx_commit/tx_cancel时，会自动put
void *tx_get_write_buffer(tx_t *tx, u64_t block_id);

// 提交修改的数据到日志，此时写cache中的数据还未下盘
int tx_commit(tx_t *tx);

// 放弃这个事务的所有修改
void tx_cancel(tx_t *tx);





// 必须和put_buffer成对使用
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type);

// 必须和get_buffer成对使用
void put_buffer(cache_mgr_t *mgr, void *buf);




int commit_all_checkpoint_cache(cache_mgr_t *mgr);

// 后台任务，所有的脏数据下盘
void *commit_disk(void *mgr);




// 初始化cache系统
cache_mgr_t *tx_cache_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops);

// 退出cache系统
void tx_cache_exit_system(cache_mgr_t *mgr);



extern space_ops_t bd_ops;

#ifdef __cplusplus
}
#endif

#endif


