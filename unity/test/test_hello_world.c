#include "../vendor/unity.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_print_hello_world(void) {
    TEST_ASSERT_EQUAL_STRING("Hello World!","Hello World!");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_print_hello_world);
    return UNITY_END();
}