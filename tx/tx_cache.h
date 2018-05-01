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

// ���ݿ�cache�ṹ
typedef struct cache_node
{
    u64_t block_id;    // ��������ݿ�
    uint32_t ref_cnt;     // ���ü���
    
    BUF_STATE_E state;  // TODO

    u64_t owner_tx_id;  // дcacheʱ��ֻ��һ��������ӵ��
    
    list_head_t node;   // ��tx��write cache�����еǼ�
    
    struct cache_node *read_cache;  // �����cache�Ƕ�cache����Ϊ�գ�����Ϊ��cache
    struct cache_node *side_cache[2];  // ��¼checkpoint cache��write cache������pingping�ķ�ʽ�����л�

    char dat[0];  // buffer
    
} cache_node_t;

// cache�²����ڿռ�Ĳ����ӿ�
typedef struct
{ // operations
    char *ops_name;
    
    int (*open)(void **hnd, char *bd_name);
    int (*read)(void *hnd, void *buf, int size, u64_t offset);
    int (*write)(void *hnd, void *buf, int size, u64_t offset);
    void (*close)(void *hnd);
} space_ops_t;

// cache�������ṹ
typedef struct
{
    // id
    u64_t checkpoint_sn;   // ˢ�̵Ĵ���sequence number(sn)
    u64_t cur_tx_id;        // ����������ʱΨһ������񣬳�ʼֵΪ1


    uint32_t block_size; // ��cache����ṹ��cache����   ��  ���豸�Ŀ��С?

    // ������ǰдcache���ĸ���pingpong��ʽ�л�
    uint8_t  write_side;   //  0 or 1, 
    
    // �����޸ĵ���Ϣ
    uint32_t onfly_tx_num; // ���ڽ��е�������Ŀ
    
    // �����޸ĵ�ͳ����Ϣ���Լ���Ƿ�ﵽcheckpoint����
    uint32_t modified_block_num;  // ���޸ĵĿ���Ŀ
    u64_t first_modified_time;    // ��checkpoint���һ���޸ĵ�ʱ�䣬ms
    uint32_t modified_data_bytes; // ���޸ĵ�������
    
    // checkpoint������
    uint32_t max_modified_blocks;   // �޸ĵĿ���Ŀ�ﵽ���ֵ
    uint32_t max_time_interval;     // ����һ����ʱ������ǿ��checkpoint
    uint32_t max_modified_bytes;    // �޸ĵ��������ﵽ���ֵ

    // ���checkpoint��δˢ���̣���������Ҫ�ƽ�����ʱ����ֹ����������
    uint8_t allow_new_tx;   // �Ƿ���������������

    // ����checkpoint����Ϣ
    uint32_t checkpoint_block_num; // ��Ҫ���̵Ŀ���Ŀ

    hashtab_t *hcache;    // ���е����ݿ黺�����ⶼ�ܿ����ҵ�

    // write_cache -> checkpoint_cache -> read_cache
    //list_head_t read_cache;        // ֻ��cache�������϶�����δ���޸Ĺ�������
    //list_head_t checkin_cache;  // �������̵�cache


    // ���豸����
    void *bd_hnd;  // block device handle
    space_ops_t *bd_ops;  // block device operations
    
} cache_mgr_t;

// ����
typedef struct
{
    cache_mgr_t *mgr;

    uint32_t tx_id;  // �������id
    
    list_head_t write_cache;  // �������޸Ĺ��������޸ĵ�cache���ع�ʱʹ��

    
} tx_t;



// ��ʼһ���µ�����
int tx_new(cache_mgr_t *mgr, tx_t **new_tx);

// �������޸�ʱ����������ӿڣ����ú���put����tx_commit/tx_cancelʱ�����Զ�put
void *tx_get_write_buffer(tx_t *tx, u64_t block_id);

// �ύ�޸ĵ����ݵ���־����ʱдcache�е����ݻ�δ����
int tx_commit(tx_t *tx);

// �����������������޸�
void tx_cancel(tx_t *tx);





// �����put_buffer�ɶ�ʹ��
void *get_buffer(cache_mgr_t *mgr, u64_t block_id, BUF_TYPE_E buf_type);

// �����get_buffer�ɶ�ʹ��
void put_buffer(cache_mgr_t *mgr, void *buf);




int commit_all_checkpoint_cache(cache_mgr_t *mgr);

// ��̨�������е�����������
void *commit_disk(void *mgr);




// ��ʼ��cacheϵͳ
cache_mgr_t *tx_cache_init_system(char *bd_name, uint32_t block_size, space_ops_t *bd_ops);

// �˳�cacheϵͳ
void tx_cache_exit_system(cache_mgr_t *mgr);



extern space_ops_t bd_ops;

#ifdef __cplusplus
}
#endif

#endif


