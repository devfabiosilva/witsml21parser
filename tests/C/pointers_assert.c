#include <ns1.nsmap>
#include <soapH.h>
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <ctest/asserts.h>
#include <cws_utils.h>
#include <pointers_asserts.h>

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

struct test_pointer_assert_rel_t {
  CWS_CONFIG *config;
  struct soap *soap_internal;
  const char *text;
  size_t textLen;
};

static void test_pointer_assert_RELEASE(void *p)
{
  struct test_pointer_assert_rel_t *test_pointer_assert_rel=(struct test_pointer_assert_rel_t *)p;

  readTextFree(&test_pointer_assert_rel->text);
  test_pointer_assert_rel->textLen=0;
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

  CHECK_CFG_P(soap_internal, soap_internal, EQ)
  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, EQ)
  CHECK_CFG_P(xmlLen, 0, EQ)
  CHECK_CFG_P(xmlSoap, NULL, EQ)
  CHECK_CFG_P(xmlSoapLen, 0, EQ)
  CHECK_CFG_P(xmlSoapSize, 0, EQ)
  CHECK_CFG_P(internal_os, NULL, EQ)
  CHECK_CFG_P(WitsmlObject, NULL, EQ)
  CHECK_CFG_P(witsml_version, VERSION_UNKNOWN, EQ)
  CHECK_CFG_P(object, NULL, EQ)
  CHECK_CFG_P(c_bson_serialized.bson, NULL, EQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, EQ)
  CHECK_CFG_P(c_json_str.json, NULL, EQ)
  CHECK_CFG_P(c_json_str.json_len, 0, EQ)
  CHECK_CFG_P(object_type, TYPE_None, EQ)
  CHECK_CFG_P(object_name, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.faultstring, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.faultstring_len, 0, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail_len, 0, EQ)
  CHECK_CFG_P(statistics.costs, 0, EQ)
  CHECK_CFG_P(statistics.strings, 0, EQ)
  CHECK_CFG_P(statistics.shorts, 0, EQ)
  CHECK_CFG_P(statistics.ints, 0, EQ)
  CHECK_CFG_P(statistics.long64s, 0, EQ)
  CHECK_CFG_P(statistics.enums, 0, EQ)
  CHECK_CFG_P(statistics.arrays, 0, EQ)
  CHECK_CFG_P(statistics.booleans, 0, EQ)
  CHECK_CFG_P(statistics.doubles, 0, EQ)
  CHECK_CFG_P(statistics.date_times, 0, EQ)
  CHECK_CFG_P(statistics.measures, 0, EQ)
  CHECK_CFG_P(statistics.event_types, 0, EQ)
  CHECK_CFG_P(statistics.total, 0, EQ)
  CHECK_CFG_P(statistics.used_memory, 0, EQ)
  CHECK_CFG_P(initial_resource_size, 0, NEQ)

  test_pointer_assert_RELEASE((void *)&test_pointer_assert_rel);

  C_ASSERT_NULL((void *)test_pointer_assert_rel.soap_internal, CTEST_SETTER(
    CTEST_TITLE("Testing soap_internal is NULL"),
    CTEST_INFO("Return value SHOULD be NULL"),
    CTEST_ON_SUCCESS("soap_internal pointer NULL SUCCESS")
  ))

  C_ASSERT_NULL((void *)test_pointer_assert_rel.config, CTEST_SETTER(
    CTEST_TITLE("Testing config is NULL"),
    CTEST_INFO("Return config value SHOULD be NULL"),
    CTEST_ON_SUCCESS("config pointer NULL SUCCESS")
  ))

  C_ASSERT_NULL((void *)test_pointer_assert_rel.text, CTEST_SETTER(
    CTEST_TITLE("Testing text is NULL"),
    CTEST_INFO("Return text value SHOULD be NULL"),
    CTEST_ON_SUCCESS("text pointer NULL SUCCESS")
  ))

}

// TODO add tests in parsers and vectors internal_soap pointers

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

