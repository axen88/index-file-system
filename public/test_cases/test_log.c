
#include "../os_adapter.h"
#include "../log.h"

MODULE(10);


int main(int argc, char *argv[])
{
    int i;

    LOG_SYSTEM_INIT("./log", "test");
    
    i = 0;
    LOG_DEBUG("Test %d\n", i++);
    LOG_INFO("Test %d\n", i++);
    LOG_WARN("Test %d\n", i++);
    LOG_ERROR("Test %d\n", i++);
    LOG_EVENT("Test %d\n", i++);
    LOG_EMERG("Test %d\n", i++);
    
    LOG_EVENT("get log level %d\n", LOG_GET_LEVEL());

    LOG_SET_LEVEL(5);
    ASSERT(LOG_GET_LEVEL() == 5);
    
    LOG_EVENT("set log level 5, real: %d\n", LOG_GET_LEVEL());
    
    LOG_DEBUG("Test %d\n", i++);
    LOG_INFO("Test %d\n", i++);
    LOG_WARN("Test %d\n", i++);
    LOG_ERROR("Test %d\n", i++);
    LOG_EVENT("Test %d\n", i++);
    LOG_EMERG("Test %d\n", i++);
    
    LOG_SET_LEVEL(3);
    ASSERT(LOG_GET_LEVEL() == 3);

    LOG_SYSTEM_EXIT();

    sleep(1);

    return 0;
} /* End of DebugLog */


