#include "os_adapter.h"
#include "hashtab.h"


hashtab_t *hashtab_create(uint32_t (*hash_value)(hashtab_t *h, void *key),
                               int (*keycmp)(hashtab_t *h, void *key1, void *key2),
                               uint32_t bucket_size, uint32_t max_num)
{
    hashtab_t *p;
    uint32_t i;

    p = OS_MALLOC(sizeof(*p));
    if (p == NULL)
        return p;

    p->bucket_size = bucket_size;
    p->num = 0;
    p->max_num = max_num;
    p->hash_value = hash_value;
    p->keycmp = keycmp;
    p->htable = OS_MALLOC(sizeof(*(p->htable)) * bucket_size);
    if (p->htable == NULL) {
        OS_FREE(p);
        return NULL;
    }

    for (i = 0; i < bucket_size; i++)
        p->htable[i] = NULL;

    return p;
}

int hashtab_insert(hashtab_t *h, void *key, void *dat)
{
    uint32_t hvalue;
    hashtab_node_t *prev, *cur, *newnode;

    if (!h || h->num == h->max_num)
        return HASHTAB_ERROR_OVER_MAX_NUM;

    prev = NULL;
    hvalue = h->hash_value(h, key);
    cur = h->htable[hvalue];
    while (cur && h->keycmp(h, key, cur->key) > 0) {
        prev = cur;
        cur = cur->next;
    }

    if (cur && (h->keycmp(h, key, cur->key) == 0))
        return HASHTAB_ERROR_EXIST;

    newnode = OS_MALLOC(sizeof(*newnode));
    if (newnode == NULL)
        return HASHTAB_ERROR_NO_MEMORY;
    
    newnode->key = key;
    newnode->dat = dat;
    
    if (prev) {
        newnode->next = prev->next;
        prev->next = newnode;
    } else {
        newnode->next = h->htable[hvalue];
        h->htable[hvalue] = newnode;
    }

    h->num++;
    
    return 0;
}

void *hashtab_search(hashtab_t *h, void *key)
{
    uint32_t hvalue;
    hashtab_node_t *cur;

    if (!h)
        return NULL;

    hvalue = h->hash_value(h, key);
    cur = h->htable[hvalue];
    while (cur != NULL && h->keycmp(h, key, cur->key) > 0)
        cur = cur->next;

    if (cur == NULL || (h->keycmp(h, key, cur->key) != 0))
        return NULL;

    return cur->dat;
}

void hashtab_destroy(hashtab_t *h)
{
    uint32_t i;
    hashtab_node_t *cur, *temp;

    if (!h)
        return;

    for (i = 0; i < h->bucket_size; i++) {
        cur = h->htable[i];
        while (cur != NULL) {
            temp = cur;
            cur = cur->next;
            OS_FREE(temp);
        }
        h->htable[i] = NULL;
    }

    OS_FREE(h->htable);
    h->htable = NULL;

    OS_FREE(h);
}


