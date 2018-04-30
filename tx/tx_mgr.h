/*******************************************************************************

            Copyright(C), 2018~2021, axen.hook@foxmail.com
********************************************************************************
File Name: TX_MGR.H
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
#ifndef __TX_MGR_H__
#define __TX_MGR_H__



struct tx_handle
{
    void *disk_hnd;                       // file handle
    ofs_super_block_t sb;               // super block

    object_info_t *obj_info;

    // tree handle structure
    uint8_t max_depth;
    uint8_t depth;             // Number of the parent nodes
    ofs_block_cache_t *cache_stack[TREE_MAX_DEPTH];
    u64_t position_stack[TREE_MAX_DEPTH];
    ofs_block_cache_t *cache;
    u64_t position;
    index_entry_t *ie;        

    list_head_t entry;
};



typedef struct
{
    u64_t tx_id;   // 
    
} tx_mgr_t;


#endif


