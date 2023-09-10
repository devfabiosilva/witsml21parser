#include <ns1.nsmap>
#include <soapH.h>
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <ctest/asserts.h>
#include <pointers_asserts.h>

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

struct test_pointer_assert_rel_t {
  CWS_CONFIG *config;
  struct soap *soap_internal;
};

static void test_pointer_assert_RELEASE(void *p)
{
  struct test_pointer_assert_rel_t *test_pointer_assert_rel=(struct test_pointer_assert_rel_t *)p;

  cws_internal_soap_free(&test_pointer_assert_rel->soap_internal);
  cws_config_free(&test_pointer_assert_rel->config);
}

void test_pointer_assert()
{
  int err;
  CWS_CONFIG *config=cws_config_new(NULL);
  struct test_pointer_assert_rel_t test_pointer_assert_rel;
  DECLARE_SOAP_INTERNAL

  memset((void *)&test_pointer_assert_rel, 0, sizeof(test_pointer_assert_rel));

  C_ASSERT_NOT_NULL((void *)config, CTEST_SETTER(
    CTEST_TITLE("Testing CWS config pointer"),
    CTEST_WARN("This is a main pointer. soap_internal pointer must this config to initialize"),
    CTEST_ON_ERROR("Was expected NOT NULL pointer at config"),
    CTEST_ON_SUCCESS("CWS config pointer SUCCESS")
  ))

  err=cws_internal_soap_new(&soap_internal, config, NULL);

  test_pointer_assert_rel.config=config;
  test_pointer_assert_rel.soap_internal=soap_internal;

  C_ASSERT_EQUAL_INT(0, err, CTEST_SETTER(
    CTEST_TITLE("Testing cws_internal_soap_new result"),
    CTEST_INFO("Return value SHOULD be 0"),
    CTEST_ON_ERROR("Was expected value equal 0"),
    CTEST_ON_SUCCESS("cws_internal_soap_new SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  C_ASSERT_NOT_NULL((void *)soap_internal, CTEST_SETTER(
    CTEST_TITLE("Testing soap_internal is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("soap_internal pointer NOT NULL SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  cws_internal_soap_free(&soap_internal);
  cws_config_free(&config);
}

// TODO add tests in parsers and vectors internal_soap pointers

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

