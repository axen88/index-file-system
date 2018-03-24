#ifndef __QUEUE_H__
#define __QUEUE_H__


#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

typedef enum tagQUEUE_ERROR_CODE_E
{
    ERR_QUEUE_FULL=1,
    ERR_QUEUE_EMPTY,
} QUEUE_ERROR_CODE_E; /* End of tagQUEUE_ERROR_CODE_E */


typedef struct
{
    void **memb;
    int head;
    int tail;
    int cnt;
    int size;
} queue_t; /* End of queue_t */

static inline queue_t *queue_create(int size)
{
    queue_t *q = NULL;

    q = (queue_t *)malloc(sizeof(queue_t));
    if (NULL == q)
    {
        return NULL;
    }

    q->memb = (void **)malloc(sizeof(void *) * size);
    if (NULL == q->memb)
    {
        free(q);
        return NULL;
    }

    q->head = 0;
    q->tail = 0;
    q->cnt = 0;
    q->size = size;

    return q;
}

static inline int queue_push(queue_t *q, void *memb)
{
    ASSERT(q);

    if (q->cnt == q->size)
    {
        return -ERR_QUEUE_FULL;
    }

    q->memb[q->tail++] = memb;
    q->tail %= q->size;
    q->cnt++;

    return 0;
}

static inline int queue_pop(queue_t *q, void **memb)
{
    ASSERT(q);

    if (q->cnt == 0)
    {
        return -ERR_QUEUE_EMPTY;
    }

    *memb = q->memb[q->head++];
    q->head %= q->size;
    q->cnt--;

    return 0;
}

static inline int queue_get_size(queue_t *q)
{
    ASSERT(q);

    return q->cnt;
}

static inline void queue_destroy(queue_t *q)
{
    ASSERT(q);

    free(q->memb);
    free(q);
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

#endif /* End of __QUEUE_H__ */


