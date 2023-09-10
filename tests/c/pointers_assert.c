#include <ctest/asserts.h>

void test_pointer_assert()
{
  C_ASSERT_TRUE(1==1, CTEST_SETTER(
    CTEST_TITLE("This is a title with value %d", 5),
    CTEST_INFO("This is an INFO title"),
    CTEST_WARN("This is a WARN message"),
    CTEST_ON_ERROR("This is a message when error occurs"),
    CTEST_ON_SUCCESS("This is a message when SUCCESS occurs")
  ))
}

