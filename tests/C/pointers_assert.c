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

  CLEAN_TEST_POINTER_ASSERT

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

void test_object_assert()
{
  int err;
  char *thisXml;
  CWS_CONFIG *config=cws_config_new("This is an instance");
  struct test_pointer_assert_rel_t test_pointer_assert_rel;
  DECLARE_SOAP_INTERNAL

  CLEAN_TEST_POINTER_ASSERT

  C_ASSERT_NOT_NULL((void *)config, CTEST_SETTER(
    CTEST_TITLE("Testing CWS config pointer @ test_object_assert"),
    CTEST_WARN("test_object_assert: This is a main pointer. soap_internal pointer must this config to initialize"),
    CTEST_ON_ERROR("Was expected NOT NULL pointer at config"),
    CTEST_ON_SUCCESS("CWS config pointer SUCCESS")
  ))

  err=cws_internal_soap_new(&soap_internal, config, NULL);

  test_pointer_assert_rel.config=config;
  test_pointer_assert_rel.soap_internal=soap_internal;

  C_ASSERT_EQUAL_INT(0, err, CTEST_SETTER(
    CTEST_TITLE("Testing cws_internal_soap_new result @ test_object_assert"),
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

  OPEN_XML(BhaRun)

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

  thisXml=cws_parse_XML_soap_envelope(test_pointer_assert_rel.soap_internal, (char *)test_pointer_assert_rel.text, test_pointer_assert_rel.textLen);

  C_ASSERT_NOT_NULL((void *)thisXml, CTEST_SETTER(
    CTEST_TITLE("Checking cws_parse_XML_soap_envelope is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("cws_parse_XML_soap_envelope SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, NEQ)
  CHECK_CFG_P(xmlLen, 0, NEQ)
  CHECK_CFG_P(xmlSoap, NULL, NEQ)
  CHECK_CFG_P(xmlSoapLen, 0, NEQ)
  CHECK_CFG_P(xmlSoapSize, 0, NEQ)
  CHECK_CFG_P(internal_os, NULL, EQ)

  COMP_VAL(config->xmlLen, test_pointer_assert_rel.textLen, EQ)
  COMP_VAL(config->xmlSoapLen, config->xmlLen, GT)
  COMP_VAL(config->xmlSoapLen, config->xmlSoapSize, LT)

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

  err=cws_soap_serve(soap_internal);

  C_ASSERT_EQUAL_INT(SOAP_OK, err, CTEST_SETTER(
    CTEST_TITLE("Checking cws_soap_serve is SOAP_OK"),
    CTEST_INFO("Return value SHOULD be SOAP_OK"),
    CTEST_ON_SUCCESS("cws_soap_serve SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, NEQ)
  CHECK_CFG_P(xmlLen, 0, NEQ)
  CHECK_CFG_P(xmlSoap, NULL, NEQ)
  CHECK_CFG_P(xmlSoapLen, 0, NEQ)
  CHECK_CFG_P(xmlSoapSize, 0, NEQ)
  CHECK_CFG_P(internal_os, NULL, NEQ)

  COMP_VAL(config->xmlLen, test_pointer_assert_rel.textLen, EQ)
  COMP_VAL(config->xmlSoapLen, config->xmlLen, GT)
  COMP_VAL(config->xmlSoapLen, config->xmlSoapSize, LT)

  CHECK_CFG_P(WitsmlObject, NULL, NEQ)
  CHECK_CFG_P(witsml_version, VERSION_UNKNOWN, EQ) //TODO will be deprecated. ALWAYS VERSION_UNKNOWN
  CHECK_CFG_P(object, NULL, NEQ)
  CHECK_CFG_P(c_bson_serialized.bson, NULL, EQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, EQ)
  CHECK_CFG_P(c_json_str.json, NULL, EQ)
  CHECK_CFG_P(c_json_str.json_len, 0, EQ)
  CHECK_CFG_P(object_type, TYPE_BhaRun, EQ)
  CHECK_CFG_P(object_name, NULL, NEQ)
  C_ASSERT_EQUAL_STRING("BhaRun", config->object_name, CTEST_SETTER(
    CTEST_TITLE("Checking config->object_name is BhaRun"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))
  CHECK_CFG_P(cws_soap_fault.faultstring, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.faultstring_len, 0, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail_len, 0, EQ)
  CHECK_CFG_P(statistics.costs, 0, EQ)
  CHECK_CFG_P(statistics.strings, 83, EQ)
  CHECK_CFG_P(statistics.shorts, 0, EQ)
  CHECK_CFG_P(statistics.ints, 0, EQ)
  CHECK_CFG_P(statistics.long64s, 5, EQ)
  CHECK_CFG_P(statistics.enums, 9, EQ)
  CHECK_CFG_P(statistics.arrays, 10, EQ)
  CHECK_CFG_P(statistics.booleans, 0, EQ)
  CHECK_CFG_P(statistics.doubles, 0, EQ)
  CHECK_CFG_P(statistics.date_times, 15, EQ)
  CHECK_CFG_P(statistics.measures, 63, EQ)
  CHECK_CFG_P(statistics.event_types, 0, EQ)
  CHECK_CFG_P(statistics.total, 0, EQ)
  CHECK_CFG_P(statistics.used_memory, 0, EQ)
  CHECK_CFG_P(initial_resource_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)getStatistics(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking getStatistics(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("getStatistics(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(statistics.costs, 0, EQ)
  CHECK_CFG_P(statistics.strings, 83, EQ)
  CHECK_CFG_P(statistics.shorts, 0, EQ)
  CHECK_CFG_P(statistics.ints, 0, EQ)
  CHECK_CFG_P(statistics.long64s, 5, EQ)
  CHECK_CFG_P(statistics.enums, 9, EQ)
  CHECK_CFG_P(statistics.arrays, 10, EQ)
  CHECK_CFG_P(statistics.booleans, 0, EQ)
  CHECK_CFG_P(statistics.doubles, 0, EQ)
  CHECK_CFG_P(statistics.date_times, 15, EQ)
  CHECK_CFG_P(statistics.measures, 63, EQ)
  CHECK_CFG_P(statistics.event_types, 0, EQ)
  CHECK_CFG_P(statistics.total, 185, EQ)
  CHECK_CFG_P(statistics.used_memory, 0, NEQ)
  CHECK_CFG_P(initial_resource_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)bsonSerialize(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking bsonSerialize(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("bsonSerialize(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_bson_serialized.bson, NULL, NEQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)getJson(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking getJson(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("getJson(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_json_str.json, NULL, NEQ)
  CHECK_CFG_P(c_json_str.json_len, 0, NEQ)

  cws_internal_soap_recycle(soap_internal);
  cws_recycle_config(config);

  C_ASSERT_EQUAL_INT(SOAP_OK, err, CTEST_SETTER(
    CTEST_TITLE("Checking cws_soap_serve is SOAP_OK after recycle"),
    CTEST_INFO("Return value SHOULD be SOAP_OK after recycle"),
    CTEST_ON_SUCCESS("cws_soap_serve SUCCESS after recycle"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, NEQ)
  CHECK_CFG_P(xmlLen, 0, EQ)
  CHECK_CFG_P(xmlSoap, NULL, NEQ)
  CHECK_CFG_P(xmlSoapLen, 0, EQ)
  CHECK_CFG_P(xmlSoapSize, 0, NEQ)
  CHECK_CFG_P(internal_os, NULL, NEQ)

  COMP_VAL(config->xmlLen, 0, EQ)
  COMP_VAL(config->xmlSoapLen, 0, EQ)

  CHECK_CFG_P(WitsmlObject, NULL, EQ)
  CHECK_CFG_P(witsml_version, VERSION_UNKNOWN, EQ) //TODO will be deprecated. ALWAYS VERSION_UNKNOWN
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

  C_ASSERT_NULL((void *)bsonSerialize(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking bsonSerialize(soap_internal) is NULL after recycle"),
    CTEST_INFO("Return value SHOULD be NULL after recycle"),
    CTEST_ON_SUCCESS("bsonSerialize(soap_internal) SUCCESS after recycle"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_bson_serialized.bson, NULL, EQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, EQ)

  C_ASSERT_NULL((void *)getJson(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking getJson(soap_internal) is NULL after recycle"),
    CTEST_INFO("Return value SHOULD be NULL after recycle"),
    CTEST_ON_SUCCESS("getJson(soap_internal) SUCCESS after recycle"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_json_str.json, NULL, EQ)
  CHECK_CFG_P(c_json_str.json_len, 0, EQ)

  CLOSE_XML

  OPEN_XML(Log)

  thisXml=cws_parse_XML_soap_envelope(test_pointer_assert_rel.soap_internal, (char *)test_pointer_assert_rel.text, test_pointer_assert_rel.textLen);

  C_ASSERT_NOT_NULL((void *)thisXml, CTEST_SETTER(
    CTEST_TITLE("Checking cws_parse_XML_soap_envelope is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("cws_parse_XML_soap_envelope SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, NEQ)
  CHECK_CFG_P(xmlLen, 0, NEQ)
  CHECK_CFG_P(xmlSoap, NULL, NEQ)
  CHECK_CFG_P(xmlSoapLen, 0, NEQ)
  CHECK_CFG_P(xmlSoapSize, 0, NEQ)
  CHECK_CFG_P(internal_os, NULL, NEQ)

  COMP_VAL(config->xmlLen, test_pointer_assert_rel.textLen, EQ)
  COMP_VAL(config->xmlSoapLen, config->xmlLen, GT)
  COMP_VAL(config->xmlSoapLen, config->xmlSoapSize, LT)

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

  err=cws_soap_serve(soap_internal);

  C_ASSERT_EQUAL_INT(SOAP_OK, err, CTEST_SETTER(
    CTEST_TITLE("Checking cws_soap_serve is SOAP_OK"),
    CTEST_INFO("Return value SHOULD be SOAP_OK"),
    CTEST_ON_SUCCESS("cws_soap_serve SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(internal_soap_error, 0, EQ)
  CHECK_CFG_P(internalInitFlag, 0, GT)
  CHECK_CFG_P(xmlIn, NULL, NEQ)
  CHECK_CFG_P(xmlLen, 0, NEQ)
  CHECK_CFG_P(xmlSoap, NULL, NEQ)
  CHECK_CFG_P(xmlSoapLen, 0, NEQ)
  CHECK_CFG_P(xmlSoapSize, 0, NEQ)
  CHECK_CFG_P(internal_os, NULL, NEQ)

  COMP_VAL(config->xmlLen, test_pointer_assert_rel.textLen, EQ)
  COMP_VAL(config->xmlSoapLen, config->xmlLen, GT)
  COMP_VAL(config->xmlSoapLen, config->xmlSoapSize, LT)

  CHECK_CFG_P(WitsmlObject, NULL, NEQ)
  CHECK_CFG_P(witsml_version, VERSION_UNKNOWN, EQ) //TODO will be deprecated. ALWAYS VERSION_UNKNOWN
  CHECK_CFG_P(object, NULL, NEQ)
  CHECK_CFG_P(c_bson_serialized.bson, NULL, EQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, EQ)
  CHECK_CFG_P(c_json_str.json, NULL, EQ)
  CHECK_CFG_P(c_json_str.json_len, 0, EQ)
  CHECK_CFG_P(object_type, TYPE_Log, EQ)
  CHECK_CFG_P(object_name, NULL, NEQ)
  C_ASSERT_EQUAL_STRING("Log", config->object_name, CTEST_SETTER(
    CTEST_TITLE("Checking config->object_name is Log"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))
  CHECK_CFG_P(cws_soap_fault.faultstring, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.faultstring_len, 0, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail, NULL, EQ)
  CHECK_CFG_P(cws_soap_fault.XMLfaultdetail_len, 0, EQ)
  CHECK_CFG_P(statistics.costs, 0, EQ)
  CHECK_CFG_P(statistics.strings, 365, EQ)
  CHECK_CFG_P(statistics.shorts, 0, EQ)
  CHECK_CFG_P(statistics.ints, 0, EQ)
  CHECK_CFG_P(statistics.long64s, 30, EQ)
  CHECK_CFG_P(statistics.enums, 47, EQ)
  CHECK_CFG_P(statistics.arrays, 45, EQ)
  CHECK_CFG_P(statistics.booleans, 4, EQ)
  CHECK_CFG_P(statistics.doubles, 4, EQ)
  CHECK_CFG_P(statistics.date_times, 46, EQ)
  CHECK_CFG_P(statistics.measures, 36, EQ)
  CHECK_CFG_P(statistics.event_types, 0, EQ)
  CHECK_CFG_P(statistics.total, 0, EQ)
  CHECK_CFG_P(statistics.used_memory, 0, EQ)
  CHECK_CFG_P(initial_resource_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)getStatistics(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking getStatistics(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("getStatistics(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(statistics.costs, 0, EQ)
  CHECK_CFG_P(statistics.strings, 365, EQ)
  CHECK_CFG_P(statistics.shorts, 0, EQ)
  CHECK_CFG_P(statistics.ints, 0, EQ)
  CHECK_CFG_P(statistics.long64s, 30, EQ)
  CHECK_CFG_P(statistics.enums, 47, EQ)
  CHECK_CFG_P(statistics.arrays, 45, EQ)
  CHECK_CFG_P(statistics.booleans, 4, EQ)
  CHECK_CFG_P(statistics.doubles, 4, EQ)
  CHECK_CFG_P(statistics.date_times, 46, EQ)
  CHECK_CFG_P(statistics.measures, 36, EQ)
  CHECK_CFG_P(statistics.event_types, 0, EQ)
  CHECK_CFG_P(statistics.total, 577, EQ)
  CHECK_CFG_P(statistics.used_memory, 0, NEQ)
  CHECK_CFG_P(initial_resource_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)bsonSerialize(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking bsonSerialize(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("bsonSerialize(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_bson_serialized.bson, NULL, NEQ)
  CHECK_CFG_P(c_bson_serialized.bson_size, 0, NEQ)

  C_ASSERT_NOT_NULL((void *)getJson(soap_internal), CTEST_SETTER(
    CTEST_TITLE("Checking getJson(soap_internal) is NOT NULL"),
    CTEST_INFO("Return value SHOULD be NOT NULL"),
    CTEST_ON_SUCCESS("getJson(soap_internal) SUCCESS"),
    CTEST_ON_ERROR_CB(test_pointer_assert_RELEASE, (void *)&test_pointer_assert_rel)
  ))

  CHECK_CFG_P(c_json_str.json, NULL, NEQ)
  CHECK_CFG_P(c_json_str.json_len, 0, NEQ)

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

