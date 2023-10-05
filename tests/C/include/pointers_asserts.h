#ifndef POINTER_ASSERTS_H
 #define POINTER_ASSERTS_H

void test_pointer_assert();
void test_object_assert();

#define DECLARE_SOAP_INTERNAL struct soap *soap_internal;

#define CHECK_CONFIG_OBJ_PTR_EQ(obj, cond) config->obj==cond

#define CHECK_CONFIG_OBJ_PTR_GT(obj, cond) config->obj>cond

#define CHECK_CONFIG_OBJ_PTR_NEQ(obj, cond) config->obj!=cond

#define CHECK_CFG_P(child, val, cond) \
  C_ASSERT_TRUE(CHECK_CONFIG_OBJ_PTR_##cond(child, val), CTEST_SETTER( \
    CTEST_TITLE("Testing config->" #child " is " #cond " " #val), \
    CTEST_INFO("config->" #child " value SHOULD be " #cond " " #val), \
    CTEST_ON_SUCCESS("config->" #child " SUCCESS"), \
    CTEST_ON_ERROR("config->" #child " FAIL"), \
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel) \
  ))

#define CLEAN_TEST_POINTER_ASSERT memset((void *)&test_pointer_assert_rel, 0, sizeof(test_pointer_assert_rel));

#define CHECK_EQ(a, b) a==b
#define CHECK_NEQ(a, b) a!=b
#define CHECK_GT(a, b) a>b
#define CHECK_LT(a, b) a<b
#define CHECK_STREQ(a, b) strcmp(a, b)==0

#define COMP_VAL(a, b, cond) \
  C_ASSERT_TRUE(CHECK_##cond(a, b), CTEST_SETTER( \
    CTEST_TITLE("Testing " #a " is " #cond " " #b), \
    CTEST_INFO(#a " value SHOULD be " #cond " " #b), \
    CTEST_ON_SUCCESS(#a " " #cond " " #b " SUCCESS"), \
    CTEST_ON_ERROR(#a " " #cond " " #b " FAIL"), \
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel) \
  ))

#define OPEN_XML(file) \
  TITLE_MSG("Opening file " #file ".xml ...") \
  err=readText(&test_pointer_assert_rel.text, &test_pointer_assert_rel.textLen, "examples/xmls/" #file".xml");\
\
  C_ASSERT_EQUAL_INT(0, err, CTEST_SETTER(\
    CTEST_TITLE("Opening file " #file "..."),\
    CTEST_INFO("Return value SHOULD be 0"),\
    CTEST_ON_SUCCESS("readText SUCCESS"),\
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)\
  ))

#define CLOSE_XML \
TITLE_MSG_FMT("Closing file @ %p with size %lu", test_pointer_assert_rel.text, test_pointer_assert_rel.textLen) \
readTextFree(&test_pointer_assert_rel.text);\
test_pointer_assert_rel.textLen=0;

#endif

