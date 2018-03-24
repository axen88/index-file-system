
#include <stdio.h>

#include "../os_adapter.h"
#include "../stack.h"

void test_case0(void)
{
#define S_SIZE 7
#define MEMB   90
        
    void *q = NULL;
    long push_msg = 0;
    void *pop_msg = NULL;

    ASSERT(q = stack_create(S_SIZE));

    push_msg = MEMB;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == -ERR_STACK_FULL);
    ASSERT(stack_get_size(q) == S_SIZE);

    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+6);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+5);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+4);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+3);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+2);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+1);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+0);
    ASSERT(stack_pop(q, &pop_msg) == -ERR_STACK_EMPTY);
    ASSERT(stack_get_size(q) == 0);
    
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(stack_push(q, (void *)push_msg) == 0); push_msg++;
    
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+9);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+8);
    ASSERT(stack_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+7);
    ASSERT(stack_pop(q, &pop_msg) == -ERR_STACK_EMPTY);
    ASSERT(stack_get_size(q) == 0);

    stack_destroy(q);

}

int main(int argc, char *argv[])
{
    test_case0();
    
    return 0;
}


