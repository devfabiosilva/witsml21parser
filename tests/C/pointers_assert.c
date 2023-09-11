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

  CHECK_CFG(soap_internal, soap_internal, EQ)
  CHECK_CFG(internal_soap_error, 0, EQ)

  C_ASSERT_TRUE(CHECK_CONFIG_OBJ_PTR_GT(internalInitFlag, 0), CTEST_SETTER(
    CTEST_TITLE("Testing internalInitFlag is greater than 0"),
    CTEST_INFO("config->internalInitFlag value SHOULD be greater than 0"),
    CTEST_ON_SUCCESS("config->internalInitFlag SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG(xmlIn, NULL, EQ)
  CHECK_CFG(xmlLen, 0, EQ)
  CHECK_CFG(xmlSoap, NULL, EQ)

  test_pointer_assert_RELEASE((void *)&test_pointer_assert_rel);
}

// TODO add tests in parsers and vectors internal_soap pointers

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

