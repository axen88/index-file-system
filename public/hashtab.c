#include "os_adapter.h"
#include "hashtab.h"

#define	HASHTAB_NODE2DATA(n, o)	((void *)((char *)(n) - (o)))
#define HASHTAB_DATA2NODE(d, o)	((hashtab_node_t *)((char *)(d) + (o)))


// 创建hash表
hashtab_t *hashtab_create(uint32_t (*keyhash)(hashtab_t *h, void *key),
                               int (*keycmp)(hashtab_t *h, void *key, void *value),
                               uint32_t slots_num, uint32_t offset)
{
    hashtab_t *h;
    uint32_t i;

    h = OS_MALLOC(sizeof(*h));
    if (h == NULL)
        return h;

    h->slots_num = slots_num;
    h->key_num = 0;
    h->offset = offset;
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

// 往hash表中插入key,value
int hashtab_insert(hashtab_t *h, void *key, void *value)
{
    uint32_t hvalue;
    hashtab_node_t *prev, *cur, *newnode;

    prev = NULL;
    hvalue = h->keyhash(h, key);
    cur = h->htable[hvalue];
    while (cur && h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) > 0)
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur && (h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) == 0))
        return -HASHTAB_ERROR_EXIST;

    newnode = HASHTAB_DATA2NODE(value, h->offset);

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

// 从hash表中删除指定key
void *hashtab_delete(hashtab_t *h, void *key)
{
    unsigned long hvalue;
    void *value;
    hashtab_node_t *prev;
    hashtab_node_t *cur;

    hvalue = h->keyhash(h, key);
    cur = h->htable[hvalue];

    /* if need to rem first node */
    if (cur != NULL && h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) == 0)
    {
        h->htable[hvalue] = cur->next;
        cur->next = 0;
        value = HASHTAB_NODE2DATA(cur, h->offset);
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
            if (h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) == 0)
            {
                prev->next = cur->next;
                cur->next = NULL;
                value = HASHTAB_NODE2DATA(cur, h->offset);
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

// 查找key，返回value
void *hashtab_search(hashtab_t *h, void *key)
{
    uint32_t hvalue;
    hashtab_node_t *cur;

    hvalue = h->keyhash(h, key);
    cur = h->htable[hvalue];
    while (cur != NULL && h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) > 0)
        cur = cur->next;

    if (cur == NULL || (h->keycmp(h, key, HASHTAB_NODE2DATA(cur, h->offset)) != 0))
        return NULL;

    return HASHTAB_NODE2DATA(cur, h->offset);
}

// 销毁hash表
void hashtab_destroy(hashtab_t *h)
{
    OS_FREE(h->htable);
    h->htable = NULL;

    OS_FREE(h);
}

// 对hash表中的所有key,value运行apply函数
int hashtab_map(hashtab_t *h, int (*apply)(void *value, void *arg), void *arg)
{
    unsigned long i;
    int ret;
    hashtab_node_t *cur;

    for (i = 0; i < h->slots_num; i++)
    {
        cur = h->htable[i];

        while (cur != NULL)
        {
            ret = apply(HASHTAB_NODE2DATA(cur, h->offset), arg);
            if (ret)
                return ret;

            cur = cur->next;
        }
    }

    return 0;
}

// 打印hash表的slot和key,value信息
void hashtab_print(struct hashtab *h, void (*print)(void *value))
{
    unsigned long i;
    struct hashtab_node *cur;
    int count = 0;

    printf("\n");

    for (i = 0; i < h->slots_num; i++)
    {
        cur = h->htable[i];
        printf("SLOT [%lu]:", i);

        while (cur != NULL)
        {
            printf(" %p:", cur);
            count++;
            print(HASHTAB_NODE2DATA(cur, h->offset));

            cur = cur->next;
        }

        printf("\n");
    }

    printf("Total items: %d\n", count);
}



// 统计hash表中使用了多少个slot和slot中最大链表长度
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

// 从hash表中摘除第一个key，并返回这个key的dat
void *hashtab_pop_first(hashtab_t *h)
{
    unsigned long slot;
    hashtab_node_t *cur;

    for (slot = 0; slot < h->slots_num; slot++)
    {
        cur = h->htable[slot];
        if (cur)
        {
            h->htable[slot] = cur->next;
            return HASHTAB_NODE2DATA(cur, h->offset);
        }
    }

    return NULL;
}

