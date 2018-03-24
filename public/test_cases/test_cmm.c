
#include <stdio.h>

#include "../os_adapter.h"
#include "../cmm/cmm.h"

void test_case0(void)
{
    void *pMem1 = NULL;
    void *pMem2 = NULL;

    MemInit();
    ASSERT(MemGetTotal() == 0);
    ASSERT(MemGetMax() == 0);

    pMem1 = MemAlloc(1024);
    ASSERT(MemGetTotal() == 1024);
    ASSERT(MemGetMax() == 1024);
    
    pMem2 = MemAlignedAlloc(2048, 4096);
    ASSERT(MemGetTotal() == 1024+2048);
    ASSERT(MemGetMax() == 1024+2048);

    MemFree(pMem1, 1024);
    ASSERT(MemGetTotal() == 2048);
    ASSERT(MemGetMax() == 1024+2048);

    MemAlignedFree(pMem2, 2048);
    ASSERT(MemGetTotal() == 0);
    ASSERT(MemGetMax() == 1024+2048);

    ASSERT(MemExit() == 0);

}

int main(int argc, char *argv[])
{

    return 0;
}


