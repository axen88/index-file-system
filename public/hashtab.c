#include "os_adapter.h"
#include "hashtab.h"

// ����hash��
hashtab_t *hashtab_create(uint32_t (*keyhash)(hashtab_t *h, void *key),
                               int (*keycmp)(hashtab_t *h, void *key1, void *key2),
                               uint32_t slots_num, uint32_t max_key_num)
{
    hashtab_t *h;
    uint32_t i;

    h = OS_MALLOC(sizeof(*h));
    if (h == NULL)
        return h;

    h->slots_num = slots_num;
    h->key_num = 0;
    h->max_key_num = max_key_num;
    h->keyhash = keyhash;
    h->keycmp = keycmp;
    h->htable = OS_MALLOC(sizeof(*(h->htable)) * slots_num);
    if (h->htable == NULL)
    {
        OS_FREE(h);
        return NULL;
    }

    for (i = 0; i < slots_num; i++)
        h->htable[i] = NULL;

    return h;
}

// ��hash���в���key,value
int hashtab_insert(hashtab_t *h, void *key, void *value)
{
    uint32_t hvalue;
    hashtab_node_t *prev, *cur, *newnode;

    if (!h || h->key_num == h->max_key_num)
        return HASHTAB_ERROR_OVER_MAX_NUM;

    prev = NULL;
    hvalue = h->keyhash(h, key);
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
    newnode->value = value;

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

    h->key_num++;

    return 0;
}

// ��hash����ɾ��ָ��key
void *hashtab_delete(hashtab_t *h, void *key)
{
    unsigned long hvalue;
    void *value;
    hashtab_node_t *prev;
    hashtab_node_t *cur;

    if(!h)
        return NULL;

    hvalue = h->keyhash(h, key);
    cur = h->htable[hvalue];

    /* if need to rem first node */
    if (cur != NULL && h->keycmp(h, key, cur->key) == 0)
    {
        h->htable[hvalue] = cur->next;
        cur->next = 0;
        value = cur->value;
        OS_FREE(cur);
        h->key_num--;
        return value;
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
                value = cur->value;
                h->key_num--;
                return value;
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

// ����key������value
void *hashtab_search(hashtab_t *h, void *key)
{
    uint32_t hvalue;
    hashtab_node_t *cur;

    if (!h)
        return NULL;

    hvalue = h->keyhash(h, key);
    cur = h->htable[hvalue];
    while (cur != NULL && h->keycmp(h, key, cur->key) > 0)
        cur = cur->next;

    if (cur == NULL || (h->keycmp(h, key, cur->key) != 0))
        return NULL;

    return cur->value;
}

// ����hash��
void hashtab_destroy(hashtab_t *h)
{
    uint32_t i;
    hashtab_node_t *cur, *temp;

    if (!h)
        return;

    for (i = 0; i < h->slots_num; i++)
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

// ��hash���е�����key,value����apply����
int hashtab_map(hashtab_t *h, int (*apply)(void *key, void *value, void *arg), void *arg)
{
    unsigned long i;
    int ret;
    hashtab_node_t *cur;

    if (!h)
        return 0;

    for (i = 0; i < h->slots_num; i++)
    {
        cur = h->htable[i];

        while (cur != NULL)
        {
            ret = apply(cur->key, cur->value, arg);
            if (ret)
                return ret;

            cur = cur->next;
        }
    }

    return 0;
}

// ��ӡhash���slot��key,value��Ϣ
void hashtab_print(struct hashtab *h, void (*print)(void *key, void *value))
{
    unsigned long i;
    struct hashtab_node *cur;
    int count = 0;

    if (!h)
        return;

    printf("\n");

    for (i = 0; i < h->slots_num; i++)
    {
        cur = h->htable[i];
        printf("SLOT [%lu]:", i);

        while (cur != NULL)
        {
            printf(" %p:", cur);
            count++;
            print(cur->key, cur->value);

            cur = cur->next;
        }

        printf("\n");
    }

    printf("Total items: %d\n", count);
}



// ͳ��hash����ʹ���˶��ٸ�slot��slot�����������
void hashtab_stat(hashtab_t *h, hashtab_info_t *info)
{
    unsigned long i, chain_len, slots_used, max_chain_len;
    struct hashtab_node *cur;

    slots_used = 0;
    max_chain_len = 0;
    for (i = 0; i < h->slots_num; i++)
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

// ��hash����ժ����һ��key�����������key��dat
void *hashtab_pop_first(hashtab_t *h)
{
    unsigned long slot;
    struct hashtab_node *cur;
    void *value;

    for (slot = 0; slot < h->slots_num; slot++)
    {
        cur = h->htable[slot];
        if (cur)
        {
            h->htable[slot] = cur->next;
            value = cur->value;
            OS_FREE(cur);
            
            return value;
        }
    }

    return NULL;
}

