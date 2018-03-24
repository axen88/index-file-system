
#include <stdio.h>

#include "../os_adapter.h"
#include "../ini.h"

void test_case0(void)
{
#define TEST_BUF_SIZE 256
#define TEST_INI_FILE "test_ini.ini"
    
    char value[TEST_BUF_SIZE]={0};

    // section student
    ASSERT(IniWriteString(TEST_INI_FILE, "student", "name", "Tony") == 0);
    ASSERT(IniWriteString(TEST_INI_FILE, "student", "age", "20") == 0);

    ASSERT(IniReadString(TEST_INI_FILE, "student", "name", value, TEST_BUF_SIZE, NULL) == 0);
    ASSERT(strcmp(value, "Tony") == 0);

    ASSERT(IniReadInt(TEST_INI_FILE, "student", "age", 0) == 20);

    // section sucess inc
    ASSERT(IniWriteString(TEST_INI_FILE, "success inc", "test name", "what's your name") == 0);
    ASSERT(IniWriteString(TEST_INI_FILE, "success inc", "test num", "123456") == 0);

    ASSERT(IniReadString(TEST_INI_FILE, "success inc", "test name", value, TEST_BUF_SIZE, NULL) == 0);
    ASSERT(strcmp(value, "what's your name") == 0);

    ASSERT(IniReadInt(TEST_INI_FILE, "success inc", "test num", 0) == 123456);
}

int main(int argc, char *argv[])
{
    test_case0();

    return 0;
}


