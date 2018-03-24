
#include <stdio.h>

#include "../os_adapter.h"
#include "../queue.h"

void test_case0(void)
{
#define Q_SIZE 7
#define MEMB   90
        
    void *q = NULL;
    long push_msg = 0;
    void *pop_msg = NULL;

    ASSERT(q = queue_create(Q_SIZE));

    push_msg = MEMB;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == -ERR_QUEUE_FULL);
    ASSERT(queue_get_size(q) == Q_SIZE);

    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+1);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+2);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+3);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+4);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+5);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+6);
    ASSERT(queue_pop(q, &pop_msg) == -ERR_QUEUE_EMPTY);
    ASSERT(queue_get_size(q) == 0);
    
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    ASSERT(queue_push(q, (void *)push_msg) == 0); push_msg++;
    
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+7);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+8);
    ASSERT(queue_pop(q, &pop_msg) == 0); ASSERT((long)pop_msg == MEMB+9);
    ASSERT(queue_pop(q, &pop_msg) == -ERR_QUEUE_EMPTY);
    ASSERT(queue_get_size(q) == 0);

    queue_destroy(q);

}

int main(int argc, char *argv[])
{
    test_case0();
    
    return 0;
}


