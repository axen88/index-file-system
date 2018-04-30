#include "os_adapter.h"
#include "hashtab.h"

// 创建hash表
hashtab_t *hashtab_create(uint32_t (*hash_value)(hashtab_t *h, void *key),
                               int (*keycmp)(hashtab_t *h, void *key1, void *key2),
                               uint32_t slot_num, uint32_t max_num)
{
    hashtab_t *h;
    uint32_t i;

    h = OS_MALLOC(sizeof(*h));
    if (h == NULL)
        return h;

    h->slot_num = slot_num;
    h->num = 0;
    h->max_num = max_num;
    h->hash_value = hash_value;
    h->keycmp = keycmp;
    h->htable = OS_MALLOC(sizeof(*(h->htable)) * slot_num);
    if (h->htable == NULL)
    {
        OS_FREE(h);
        return NULL;
    }

    for (i = 0; i < slot_num; i++)
        h->htable[i] = NULL;

    return h;
}

// 往hash表中插入key->dat
int hashtab_insert(hashtab_t *h, void *key, void *dat)
{
    uint32_t hvalue;
    hashtab_node_t *prev, *cur, *newnode;

    if (!h || h->num == h->max_num)
        return HASHTAB_ERROR_OVER_MAX_NUM;

    prev = NULL;
    hvalue = h->hash_value(h, key);
    cur = h->htable[hvalue];
    while (cur && h->keycmp(h, key, cur->key) > 0)
    {
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
    
    if (prev)
    {
        newnode->next = prev->next;
        prev->next = newnode;
    } 
    else
    {
        newnode->next = h->htable[hvalue];
        h->htable[hvalue] = newnode;
    }

    h->num++;
    
    return 0;
}

// 从hash表中删除指定key
void *hashtab_delete(hashtab_t *h, void *key)  
{  
    unsigned long hvalue;  
    void *d;  
    hashtab_node_t *prev;  
    hashtab_node_t *cur;  
    
    if(!h)  
        return NULL;  
    
    hvalue = h->hash_value(h, key);  
    cur = h->htable[hvalue];  
    
    /* if need to rem first node */  
    if (cur != NULL && h->keycmp(h, key, cur->key) == 0)
    {  
        h->htable[hvalue] = cur->next;  
        cur->next = 0;  
        d = cur->dat;  
        OS_FREE(cur);  
        h->num--;  
        return d;  
    }  
    
    /* some node after first node */  
    /* trying to avoid SEGFAULT : Gokul */  
    if (cur != NULL)
    {  
        prev = cur;  
        cur = cur->next;  
        while(cur != NULL)
        {  
            if(h->keycmp(h, key, cur->key) == 0)
            {  
                prev->next = cur->next;  
                cur->next = NULL;  
                d = cur->dat;  
                h->num--;  
                return d;  
                break;  
            }
            else
            {  
                prev = cur;  
                cur = cur->next;  
            }  
        } // end while  
    }
    
    return NULL;  
}  

// 查找key，返回dat
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

// 销毁hash表
void hashtab_destroy(hashtab_t *h)
{
    uint32_t i;
    hashtab_node_t *cur, *temp;

    if (!h)
        return;

    for (i = 0; i < h->slot_num; i++)
    {
        cur = h->htable[i];
        while (cur != NULL)
        {
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

// 对hash表中的所有node运行apply函数
int hashtab_map(hashtab_t *h, int (*apply)(void *k, void *d, void *args), void *args)  
{  
    unsigned long i;  
    int ret;  
    hashtab_node_t *cur;  
    
    if (!h)  
        return 0;  
    
    for (i = 0; i < h->slot_num; i++)
    {  
        cur = h->htable[i];  
        
        while (cur != NULL)
        {  
            ret = apply(cur->key, cur->dat, args);  
            if (ret)  
                return ret;  
            
            cur = cur->next;  
        }  
    }  
    
    return 0;  
}  

    
void hashtab_print(struct hashtab *h, void (*print)(void *key, void *data))
{  
    unsigned long i;  
    struct hashtab_node *cur;  
    int count = 0;  

    if (!h)  
        return;  

    printf("\n");  

    for (i = 0; i < h->slot_num; i++)
    {  
        cur = h->htable[i];  
        printf("SLOT [%lu]:", i);  

        while (cur != NULL)
        {  
            printf(" %p:", cur);
            count++;  
            print(cur->key, cur->dat);

            cur = cur->next;  
        }  

        printf("\n");  
    }  

    printf("Total items: %d\n", count);
}  



// 统计hash表的一些信息
void hashtab_stat(hashtab_t *h, hashtab_info_t *info)  
{  
    unsigned long i, chain_len, slots_used, max_chain_len;  
    struct hashtab_node *cur;  

    slots_used = 0;  
    max_chain_len = 0;  
    for (slots_used = max_chain_len = i = 0; i < h->slot_num; i++)
    {  
        cur = h->htable[i];  
        if (cur)
        {  
            slots_used++;  
            chain_len = 0;  
            while (cur)
            {  
                chain_len++;  
                cur = cur->next;  
            }  
        
            if (chain_len > max_chain_len)  
                max_chain_len = chain_len;  
        }  
    }  
    
    info->slots_used = slots_used;  
    info->max_chain_len = max_chain_len;  
}  

