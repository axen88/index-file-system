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



#define HASHTAB_ERROR_OVER_MAX_NUM   -1
#define HASHTAB_ERROR_EXIST          -2
#define HASHTAB_ERROR_NO_MEMORY      -3

typedef struct hashtab_node hashtab_node_t;

struct hashtab_node
{
    void *key;
    void *dat;
    hashtab_node_t *next;
};

typedef struct hashtab hashtab_t;

struct hashtab
{
    hashtab_node_t **htable;                               /* hash table */
    uint32_t slot_num;                                         /* number of slots in hash table */
    uint32_t num;                                         /* number of elements in hash table */
    uint32_t max_num;
    uint32_t (*hash_value)(hashtab_t *h, void *key);            /* hash function */
    int (*keycmp)(hashtab_t *h, void *key1, void *key2);   /* key comparison function */
};


hashtab_t *hashtab_create(uint32_t (*hash_value)(hashtab_t *h, void *key),
                               int (*keycmp)(hashtab_t *h, void *key1, void *key2),
                               uint32_t slot_num, uint32_t max_num);

int hashtab_insert(hashtab_t *h, void *key, void *dat);

void *hashtab_delete(hashtab_t *h, void *key);  

void *hashtab_search(hashtab_t *h, void *key);

void hashtab_destroy(hashtab_t *h);

struct hashtab_info {  
  unsigned long slots_used;  
  unsigned long max_chain_len;  
};  

typedef struct hashtab_info hashtab_info_t;

int hashtab_map(struct hashtab *h,  
                int (*apply)(void *k, void *d, void *args),  
                void *args);  

void hashtab_stat(struct hashtab *h, struct hashtab_info *info);  

void hashtab_print(struct hashtab *h, void (*print)(void *key, void *data));  

#endif

