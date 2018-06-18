/*******************************************************************************

            Copyright(C), 2017~2020, axen.hook@foxmail.com
********************************************************************************
File Name: HASHTAB.H
Author   : axen.hook
Version  : 1.00
Date     : 23/Jul/2017
Description: 
Function List: 
    1. ...: 
History: 
    Version: 1.00  Author: axen.hook  Date: 23/Jul/2017
--------------------------------------------------------------------------------
    1. Primary version
*******************************************************************************/

#ifndef __HASHTAB_H__
#define __HASHTAB_H__

#define HASHTAB_ERROR_EXIST          2

typedef struct hashtab_node hashtab_node_t;

struct hashtab_node
{
    //void *key;
    //void *value;
    hashtab_node_t *next;
};

typedef struct hashtab hashtab_t;

struct hashtab
{
    hashtab_node_t **htable;           /* hash table */
    uint32_t slots_num;                                         /* number of slots in hash table */
    uint32_t key_num;                                         /* number of elements in hash table */
    uint32_t offset; //  hash node在用户结构体中的偏移
    uint32_t (*keyhash)(hashtab_t *h, void *key);            /* hash function */
    int (*keycmp)(hashtab_t *h, void *key, void *value);   /* key comparison function */
};


// 创建hash表
hashtab_t *hashtab_create(uint32_t (*keyhash)(hashtab_t *h, void *key),
                          int (*keycmp)(hashtab_t *h, void *key, void *value),
                          uint32_t slots_num, uint32_t offset);

// 往hash表中插入key,value
int hashtab_insert(hashtab_t *h, void *key, void *value);

// 从hash表中删除指定key
void *hashtab_delete(hashtab_t *h, void *key);  

// 查找key，返回value
void *hashtab_search(hashtab_t *h, void *key);

// 销毁hash表
void hashtab_destroy(hashtab_t *h);


// 对hash表中的所有key,value运行apply函数
int hashtab_walkall(hashtab_t *h, int (*apply)(void *value, void *arg), void *args);  

// hash表统计信息
typedef struct hashtab_info {  
    unsigned long slots_used;  
    unsigned long max_chain_len;  
} hashtab_info_t;  

// 统计hash表中使用了多少个slot和slot中最大链表长度
void hashtab_stat(hashtab_t *h, hashtab_info_t *info);  

// 打印hash表的slot和key,value信息
void hashtab_print(hashtab_t *h, void (*print)(void *value));  

// 从hash表中摘除第一个key，并返回这个key的dat
void *hashtab_pop_first(hashtab_t *h);

#endif

