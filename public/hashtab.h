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

struct hashtab_node {
    void *key;
    void *dat;
    hashtab_node_t *next;
};

typedef struct hashtab hashtab_t;

struct hashtab {
    hashtab_node_t **htable;                               /* hash table */
    uint32_t bucket_size;                                         /* number of slots in hash table */
    uint32_t num;                                         /* number of elements in hash table */
    uint32_t max_num;
    uint32_t (*hash_value)(hashtab_t *h, void *key);            /* hash function */
    int (*keycmp)(hashtab_t *h, void *key1, void *key2);   /* key comparison function */
};


#endif

