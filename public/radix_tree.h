#ifndef __RADIX_TREE_H__
#define __RADIX_TREE_H__

#ifdef __cplusplus
extern "C" {
#endif


#define CHILD_NUM 256

typedef struct _RADIX_NODE
{
    struct _RADIX_NODE *childs[CHILD_NUM];
    char *str;
} RADIX_NODE;

int RadixInsert(RADIX_NODE *node, char *str);

int RadixWalkAll(RADIX_NODE *node, int (*callback)(char *));

void RadixRelease(RADIX_NODE *node, int (*callback)(char *));

#ifdef __cplusplus
}
#endif


#endif


