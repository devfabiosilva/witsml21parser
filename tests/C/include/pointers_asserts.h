#ifndef POINTER_ASSERTS_H
 #define POINTER_ASSERTS_H

void test_pointer_assert();

#define DECLARE_SOAP_INTERNAL struct soap *soap_internal;

#define CHECK_CONFIG_OBJ_PTR_EQ(obj, cond) config->obj==cond

#define CHECK_CONFIG_OBJ_PTR_GT(obj, cond) config->obj>cond

#define CHECK_CFG(child, val, cond) \
  C_ASSERT_TRUE(CHECK_CONFIG_OBJ_PTR_##cond(child, val), CTEST_SETTER( \
    CTEST_TITLE("Testing " #child " is " #val), \
    CTEST_INFO("config->" #child " value SHOULD be " #val), \
    CTEST_ON_SUCCESS("config->" #child " SUCCESS"), \
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel) \
  ))

#endif

