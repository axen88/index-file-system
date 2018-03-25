



#include "tx_cache.h"

cache_node_t *alloc_cache_node(cache_mgr_t *mgr)
{
    cache_node_t *cache = OS_MALLOC(sizeof(cache_node_t));
    if (cache == NULL)
    {
        
    }

    
    return NULL;
}

void free_cache_node(cache_mgr_t *mgr, cache_node_t *cache)
{

}

cache_node_t *get_cache_node(cache_mgr_t *mgr, uint64_t block_id, BUF_TYPE_E buf_type)
{
    cache_node_t *cache = hashtab_search(&mgr->hcache, block_id);
    if (cache == NULL)
    {
        
    }

}


void *get_buffer(cache_mgr_t *mgr, BUF_TYPE_E buf_type)
{
    
}

int put_buffer(cache_mgr_t *mgr, void *buf)
{

}

int checkin_cache(cache_mgr_t *mgr, cache_node_t *cache)
{
    
}

