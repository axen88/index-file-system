
#include <stdlib.h>

#include "../os_adapter.h"

static os_mutex_t g_mutex;
static unsigned long long g_total_mem = 0;
static unsigned long long g_max_mem = 0;

void MemInit(void)
{
    g_total_mem = 0;
    g_max_mem = 0;
    OS_MUTEX_INIT(&g_mutex);
}

unsigned long long MemGetTotal(void)
{
    unsigned long long size = 0;

    OS_MUTEX_LOCK(&g_mutex);
    size = g_total_mem;
    OS_MUTEX_UNLOCK(&g_mutex);

    return size;
}


unsigned long long MemGetMax(void)
{
    unsigned long long size = 0;

    OS_MUTEX_LOCK(&g_mutex);
    size = g_max_mem;
    OS_MUTEX_UNLOCK(&g_mutex);

    return size;
}

unsigned long long MemExit(void)
{
    unsigned long long size = 0;

    OS_MUTEX_LOCK(&g_mutex);
    size = g_total_mem;
    OS_MUTEX_UNLOCK(&g_mutex);
    
    OS_MUTEX_DESTROY(&g_mutex);

    return size;
}

void *MemAlloc(unsigned int v_size)
{
    void *pMem = malloc(v_size);
    if (NULL != pMem)
    {
        OS_MUTEX_LOCK(&g_mutex);
        g_total_mem += v_size;
        if (g_max_mem < g_total_mem)
        {
            g_max_mem = g_total_mem;
        }
        
        OS_MUTEX_UNLOCK(&g_mutex);
    }

    return pMem;
}

void MemFree(void *v_pMem, unsigned int v_size)
{
    if (NULL == v_pMem)
    {
        return;
    }
    
    free(v_pMem);
    OS_MUTEX_LOCK(&g_mutex);
    g_total_mem -= v_size;
    OS_MUTEX_UNLOCK(&g_mutex);
}

void *MemAlignedAlloc(unsigned int v_size, unsigned int v_alignment)
{
    void *pMem = NULL;

#ifdef WIN32
    pMem = _aligned_malloc(v_size, v_alignment);
#else
    if (posix_memalign(&pMem, v_alignment, v_size) != 0)
    {
        return NULL;
    }
#endif

    if (NULL != pMem)
    {
        OS_MUTEX_LOCK(&g_mutex);
        g_total_mem += v_size;
        if (g_max_mem < g_total_mem)
        {
            g_max_mem = g_total_mem;
        }
        
        OS_MUTEX_UNLOCK(&g_mutex);
    }

    return pMem;
}

void MemAlignedFree(void *v_pMem, unsigned int v_size)
{
    if (NULL == v_pMem)
    {
        return;
    }
    
#ifdef WIN32
    _aligned_free(v_pMem);
#else
    free(v_pMem);
#endif

    OS_MUTEX_LOCK(&g_mutex);
    g_total_mem -= v_size;
    OS_MUTEX_UNLOCK(&g_mutex);
}



