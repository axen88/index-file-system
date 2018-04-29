/*
* 使用gcc -Wall -g -o radix_tree radix_tree.c进行编译链接
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "radix_tree.h"

int RadixInsert(RADIX_NODE *node, char *str)
{
    int i;
    int key;
    
    for (i = 0; str[i] != '\0'; i++)
    {
        key = str[i] - '0';
        if (key >= CHILD_NUM)
        {
            return -1;
        }
        
        if (node->childs[key] == NULL)
        {
            node->childs[key] = calloc(sizeof(RADIX_NODE), 1);
        }
        
        node = node->childs[key];
    }
    
    node->str = str;
    
    return 0;
}

int RadixWalkAll(RADIX_NODE *node, int (*callback)(char *))
{
    int i;
    int ret;
    
    if (node == NULL)
    {
        return 0;
    }
    
    if (callback != NULL)
    {
        ret = callback(node->str);
        if (ret < 0)
        {
            return ret;
        }
    }
    
    for (i = 0; i < CHILD_NUM; i++)
    {
        ret = RadixWalkAll(node->childs[i], callback);
        if (ret < 0)
        {
            return ret;
        }
    }
    
    return 0;
}

void RadixRelease(RADIX_NODE *node, int (*callback)(char *))
{
    int i;
    
    if (node == NULL)
    {
        return;
    }
    
    if (callback != NULL)
    {
        callback(node->str);
    }
    
    for (i = 0; i < CHILD_NUM; i++)
    {
        RadixRelease(node->childs[i], callback);
    }
    
    free(node);
    
    return;
}

