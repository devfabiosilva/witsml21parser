#include <ctest/asserts.h>
#include <pointers_asserts.h>

int main(int argc, char **argv)
{
  TITLE_MSG("Begin C Witsml 2.1 parser tests ...")
  test_text_reader();
  test_pointer_assert();
  test_object_assert();
  end_tests();
  return 0;
}

