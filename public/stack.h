#ifndef __STACK_H__
#define __STACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

typedef enum tagSTACK_ERROR_CODE_E
{
    ERR_STACK_FULL = 1,
    ERR_STACK_EMPTY,
} STACK_ERROR_CODE_E; /* End of tagSTACK_ERROR_CODE_E */

typedef struct
{
    void **memb;
    int num;
    int max;
} stack_t; /* End of stack_t */

static inline stack_t *stack_create(int size)
{
    stack_t *s = NULL;

    s = malloc(sizeof(stack_t));
    if (NULL == s)
    {
        return NULL;
    }

    s->memb = malloc(sizeof(void*) * size);
    if (NULL == s->memb)
    {
        free(s);
        return NULL;
    }

    s->num = 0;
    s->max = size;

    return s;
}

static inline int stack_push(stack_t *s, void *memb)
{
    ASSERT(s);
    ASSERT(memb);

    if (s->num >= s->max)
    {
        return -ERR_STACK_FULL;
    }

    s->memb[s->num++] = memb;

    return 0;
}

static inline int stack_pop(stack_t *s, void **memb)
{
    ASSERT(s);
    ASSERT(memb);

    if (s->num <= 0)
    {
        return -ERR_STACK_EMPTY;
    }

    *memb = s->memb[--s->num];

    return 0;
}

static inline int stack_get_size(stack_t *s)
{
    ASSERT(s);

    return s->num;
}

static inline void stack_destroy(stack_t *s)
{
    ASSERT(s);

    free(s->memb);
    free(s);
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


#endif /* End of __STACK_H__ */


