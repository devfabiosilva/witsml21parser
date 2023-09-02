//https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#interface_function_table
//
#include "../../ns1.nsmap" // XML namespace mapping table (only needed once at the global level)
#include "../../soapH.h"    // server stubs, serializers, etc.
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <jni.h>

_Static_assert(sizeof(jbyteArray)==sizeof(void *), "Error");

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

#define WITSML_CLASS_SIG "Lorg/jwitsml21parser/JWitsmlParser;"
#define WITSML_CLASS_EXCEPTION "org/jwitsml21parser/exception/JWitsmlException"
#define JNI_EXCEPTION_CLASS "java/lang/Exception"
#define throwCWitsml21Error(msg) cWitsml21ParserException(env, JNI_EXCEPTION_CLASS, msg, 0, NULL, NULL, c_soap_internal_instance_name)
#define throwCWitsml21ErrorA(msg) cWitsml21ParserException(env, JNI_EXCEPTION_CLASS, msg, 0, NULL, NULL, NULL)
#define JNI_INIT_THROWABLE_WITH_CODE(class) \
  (*env)->GetMethodID(env, class, "<init>", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V")


jint cWitsml21ParserException(
  JNIEnv *,
  const char *,
  const char *,
  int,
  const char *,
  const char *,
  const char *
);

#ifdef WITH_STATISTICS
static jfieldID setJavaInt(JNIEnv *env, jobject parent, jclass clazz, const char *name, int value)
{
  jfieldID fieldID = (*env)->GetFieldID(env, clazz, name, "I");

  if (fieldID) {
    (*env)->SetIntField(env, parent, fieldID, (jint)value);
    if ((*env)->ExceptionCheck(env))
      return NULL;
  }

  return fieldID;
}

static jfieldID setJavaLong(JNIEnv *env, jobject parent, jclass clazz, const char *name, long int value)
{
  jfieldID fieldID = (*env)->GetFieldID(env, clazz, name, "J");

  if (fieldID) {
    (*env)->SetLongField(env, parent, fieldID, (jlong)value);
    if ((*env)->ExceptionCheck(env))
      return NULL;
  }

  return fieldID;
}

#define SET_JVM_INT(name, value, errorLevel) \
  if (!setJavaInt(env, jWitsmlParser, witsmlParserClass, name, (int)value)) {\
    throwCWitsml21Error("Could set Java int name: " #name); \
    goto Java_org_jwitsml21parser_JWitsml21_parser_##errorLevel; \
  }

#define SET_JVM_INT_STAT(name, errorLevel) \
  SET_JVM_INT(#name, statistics->name, errorLevel)

#define SET_JVM_LONG(name, value, errorLevel) \
  if (!setJavaLong(env, jWitsmlParser, witsmlParserClass, name, (long int)value)) {\
    throwCWitsml21Error("Could set Java long name: " #name); \
    goto Java_org_jwitsml21parser_JWitsml21_parser_##errorLevel; \
  }

#endif
/*
 * Class:     org_jwitsml21parser_JWitsml21
 * Method:    parser
 * Signature: (Ljava/lang/String;I)Lorg/jwitsml21parser/JWitsmlParser;
 */
JNIEXPORT jobject JNICALL Java_org_jwitsml21parser_JWitsml21_parser(JNIEnv *env, jobject thisObject, jstring xml, jint options)
{
  const char
    *c_xml,
    *c_soap_internal_instance_name=NULL;
  size_t c_xml_len;
  CWS_CONFIG *config;
  struct soap *soap_internal;
  struct c_bson_serialized_t *c_bson_serialized;
#ifdef WITH_STATISTICS
  struct statistics_t *statistics;
#endif
  int c_options, *c_options_ptr;

  jclass witsmlParserClass;
  jmethodID witsmlParserClassMethodId;
  jobject jWitsmlParser;
  jbyteArray j_bson;
  jstring
    jWitsmlParseInstanceName,
    jWitsmlObjectName;
  jfieldID
    jwitsmlParseInstanceNameFieldID,
    jWitsmlObjectNameFieldID,
    j_bsonFieldID;

  if (options>-1) {
    if (options&(~(SOAP_XML_IGNORENS|SOAP_XML_STRICT))) {
      throwCWitsml21Error("Invalid options");
      return NULL;
    }
    c_options=SOAP_C_UTFSTRING|SOAP_ENC_PLAIN|(int)options;
    c_options_ptr=&c_options;
  } else
    c_options_ptr=NULL;

  if (!xml) {
    throwCWitsml21Error("xml cannot be null");
    return NULL;
  }

  if (!(c_xml=(*env)->GetStringUTFChars(env, xml, JNI_FALSE))) {
    throwCWitsml21Error("Cannot parse xml utf8 to C char");
    return NULL;
  }

  c_xml_len=(*env)->GetStringUTFLength(env, xml);

  if (!(config=cws_config_new("jWITSMLParser 2.1"))) {
    throwCWitsml21Error("Cannot init C Witsml Parser config");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume1;
  }

  if (cws_internal_soap_new(&soap_internal, config, c_options_ptr)) {
    throwCWitsml21Error("Cannot init C gSoap Witsml Parser instance");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume2;
  }

  c_soap_internal_instance_name=GET_INSTANCE_NAME;

  if (!(cws_parse_XML_soap_envelope(soap_internal, (char *)c_xml, c_xml_len))) {
cWitsml21ParserException_resume1:
    cWitsml21ParserException(
      env, WITSML_CLASS_EXCEPTION, NULL,
      config->internal_soap_error, config->cws_soap_fault.faultstring, config->cws_soap_fault.XMLfaultdetail, c_soap_internal_instance_name
    );
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume3;
  }

  if (cws_soap_serve(soap_internal))
    goto cWitsml21ParserException_resume1;

  if (!(c_bson_serialized=bsonSerialize(soap_internal))) {
    throwCWitsml21Error("C bson serialize error");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume3;
  }

  witsmlParserClass=(*env)->FindClass(env, WITSML_CLASS_SIG);

  if (!witsmlParserClass) {
    throwCWitsml21Error("Could not find class signature \""WITSML_CLASS_SIG"\"");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume3;
  }

  witsmlParserClassMethodId = (*env)->GetMethodID(env, witsmlParserClass, "<init>", "()V");

  if (!witsmlParserClassMethodId) {
    throwCWitsml21Error("Could not get method of signature \""WITSML_CLASS_SIG"\"");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume3;
  }

  jWitsmlParser = (*env)->NewObject(env, witsmlParserClass, witsmlParserClassMethodId);

  if (!jWitsmlParser) {
    throwCWitsml21Error("Could not init object Witsml Parser 2.1");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume3;
  }

  if (!(jwitsmlParseInstanceNameFieldID=(*env)->GetFieldID(env, witsmlParserClass, "witsmlParserInstanceName", "Ljava/lang/String;"))) {
    throwCWitsml21Error("Could not find 'witsmlParserInstanceNameID'");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume4;
  }

  if (!(jWitsmlParseInstanceName=(*env)->NewStringUTF(env, c_soap_internal_instance_name))) {
    throwCWitsml21Error("Could not create string instance 'witsmlParserInstanceName'");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume4;
  }

  (*env)->SetObjectField(env, jWitsmlParser, jwitsmlParseInstanceNameFieldID, jWitsmlParseInstanceName);

  if ((*env)->ExceptionCheck(env)) {
    throwCWitsml21Error("Could not set witsmlParseInstanceName to JWitsml Parser");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume5;
  }

//

  if (!(jWitsmlObjectNameFieldID=(*env)->GetFieldID(env, witsmlParserClass, "witsmlObjectName", "Ljava/lang/String;"))) {
    throwCWitsml21Error("Could not find 'witsmlObjectNameID'");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume5;
  }

  if (!(jWitsmlObjectName=(*env)->NewStringUTF(env, GET_OBJECT_NAME))) {
    throwCWitsml21Error("Could not create string instance 'witsmlObjectName'");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume5;
  }

  (*env)->SetObjectField(env, jWitsmlParser, jWitsmlObjectNameFieldID, jWitsmlObjectName);

  if ((*env)->ExceptionCheck(env)) {
    throwCWitsml21Error("Could not set witsmlObjectName to JWitsml Parser");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume6;
  }

//

  j_bsonFieldID = (*env)->GetFieldID(env, witsmlParserClass, "object", "[B");

  if (!j_bsonFieldID) {
    throwCWitsml21Error("Could not get field id of type [B");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume6;
  }

  j_bson = (*env)->NewByteArray(env, (jsize)c_bson_serialized->bson_size);

  if (!j_bson) {
    throwCWitsml21Error("Could not create byte array in object");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume6;
  }

  (*env)->SetByteArrayRegion(env, j_bson, 0, c_bson_serialized->bson_size, (const jbyte *)c_bson_serialized->bson);

  if ((*env)->ExceptionCheck(env)) {
    throwCWitsml21Error("Could not copy BSON data to object");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume7;
  }

  (*env)->SetObjectField(env, jWitsmlParser, j_bsonFieldID, j_bson);

  if ((*env)->ExceptionCheck(env)) {
    throwCWitsml21Error("Could not set object field BSON to JWitsml 2.1 Parser");
    goto Java_org_jwitsml21parser_JWitsml21_parser_resume7;
  }

#ifdef WITH_STATISTICS
  statistics=getStatistics(soap_internal); // Pointer always exists

  SET_JVM_INT("type", GET_OBJECT_TYPE, resume7)

  SET_JVM_INT_STAT(costs, resume7)
  SET_JVM_INT_STAT(strings, resume7)
  SET_JVM_INT_STAT(shorts, resume7)
  SET_JVM_INT_STAT(ints, resume7)
  SET_JVM_INT_STAT(long64s, resume7)
  SET_JVM_INT_STAT(enums, resume7)
  SET_JVM_INT_STAT(arrays, resume7)
  SET_JVM_INT_STAT(booleans, resume7)
  SET_JVM_INT_STAT(doubles, resume7)
  SET_JVM_INT_STAT(long64s, resume7)
  SET_JVM_INT_STAT(date_times, resume7)
  SET_JVM_INT_STAT(event_types, resume7)
  SET_JVM_INT_STAT(measures, resume7)

  SET_JVM_INT("total", statistics->total, resume7)
  SET_JVM_LONG("memoryUsed", statistics->used_memory, resume7)
#endif
///
  cws_internal_soap_free(&soap_internal);
  cws_config_free(&config);
  (*env)->ReleaseStringUTFChars(env, xml, c_xml);

  return jWitsmlParser;

Java_org_jwitsml21parser_JWitsml21_parser_resume7:
  (*env)->DeleteLocalRef(env, j_bson);

Java_org_jwitsml21parser_JWitsml21_parser_resume6:
  (*env)->DeleteLocalRef(env, jWitsmlObjectName);

Java_org_jwitsml21parser_JWitsml21_parser_resume5:
  (*env)->DeleteLocalRef(env, jWitsmlParseInstanceName);

Java_org_jwitsml21parser_JWitsml21_parser_resume4:
  (*env)->DeleteLocalRef(env, jWitsmlParser);

Java_org_jwitsml21parser_JWitsml21_parser_resume3:
  cws_internal_soap_free(&soap_internal);

Java_org_jwitsml21parser_JWitsml21_parser_resume2:
  cws_config_free(&config);

Java_org_jwitsml21parser_JWitsml21_parser_resume1:
  (*env)->ReleaseStringUTFChars(env, xml, c_xml);

  return NULL;
}

jint cWitsml21ParserException(
  JNIEnv *env,
  const char *class,
  const char *message,
  int cwitsmlError,
  const char *faultstring,
  const char *XMLfaultdetail,
  const char *c_soap_internal_instance_name
)
{
  jint err;
  jclass exClass;
  jmethodID methodId;
  jobject jErrObj;
  jstring
    errMsg, jFaultString, jXMLfaultdetail, jSoapInternalInstanceName;
  const char *err_msg;

  if (!(exClass=(*env)->FindClass(env, class))) {
    if ((void *)class==(void *)JNI_EXCEPTION_CLASS)
      return 1; // PANIC (It should never happen)

    if (!(exClass=(*env)->FindClass(env, JNI_EXCEPTION_CLASS)))
      return 2; // PANIC (It should never happen)

    err_msg="cWitsml21ParserException: Class not found WITSML_CLASS_EXCEPTION = \""WITSML_CLASS_EXCEPTION"\"";

  } else if (cwitsmlError) {

    err_msg=(message)?message:"cWitsml21ParserException: CWitsml Soap Internal error or WITSML error. Please, see faultstring or XMLfaultdetail for details";

    if (!(errMsg=(*env)->NewStringUTF(env, err_msg))) {
      throwCWitsml21Error("cWitsml21ParserException: Cannot set message error 3");
      return 3;
    }

    if (!(jFaultString=(*env)->NewStringUTF(env, (faultstring)?faultstring:""))) {
      err=4;
      throwCWitsml21Error("cWitsml21ParserException: Cannot set faultstring error 4");
      goto cWitsml21ParserException_EXIT1;
    }

    if (!(jXMLfaultdetail=(*env)->NewStringUTF(env, (XMLfaultdetail)?XMLfaultdetail:""))) {
      err=5;
      throwCWitsml21Error("cWitsml21ParserException: Cannot set XMLfaultdetail error 5");
      goto cWitsml21ParserException_EXIT2;
    }

    if (!(jSoapInternalInstanceName=(*env)->NewStringUTF(env, (c_soap_internal_instance_name)?c_soap_internal_instance_name:""))) {
      err=6;
      throwCWitsml21Error("cWitsml21ParserException: Cannot set soapInternalInstanceName error 6");
      goto cWitsml21ParserException_EXIT3;
    }

    if (!(methodId=JNI_INIT_THROWABLE_WITH_CODE(exClass))) {
      err=7;
      throwCWitsml21Error("cWitsml21ParserException: Cannot initialize throwable class \""WITSML_CLASS_EXCEPTION"\". Error 7");
      goto cWitsml21ParserException_EXIT4;
    }

    if (!(jErrObj=(*env)->NewObject(
      env, exClass, methodId, errMsg, cwitsmlError, jFaultString, jXMLfaultdetail, jSoapInternalInstanceName))) {
      err=8;
      throwCWitsml21Error("cWitsml21ParserException: Cannot create throwable class \""WITSML_CLASS_EXCEPTION"\". Error 8");
      goto cWitsml21ParserException_EXIT4;
    } else if ((err=(*env)->Throw(env, jErrObj))) {
      throwCWitsml21Error("cWitsml21ParserException: Can't throw jErrObj");
      goto cWitsml21ParserException_EXIT5;
    }

    return 0;

cWitsml21ParserException_EXIT5:
  (*env)->DeleteLocalRef(env, jErrObj);

cWitsml21ParserException_EXIT4:
  (*env)->DeleteLocalRef(env, jSoapInternalInstanceName);

cWitsml21ParserException_EXIT3:
  (*env)->DeleteLocalRef(env, jXMLfaultdetail);

cWitsml21ParserException_EXIT2:
  (*env)->DeleteLocalRef(env, jFaultString);

cWitsml21ParserException_EXIT1:
  (*env)->DeleteLocalRef(env, errMsg); // NEVER will be NULL

   return err;
  } else
    err_msg=message;

  return (*env)->ThrowNew(env, exClass, err_msg);
}

/*
 * Class:     org_jwitsml21parser_JWitsml21
 * Method:    bsonToString
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_jwitsml21parser_JWitsml21_bsonToString(JNIEnv *env, jclass thisClass, jbyteArray bson)
{
  char *c_json;
  uint8_t *c_bson;
  size_t
    c_bson_size,
    c_json_len;
  jbyteArray j_json;

  if (!bson) {
    throwCWitsml21ErrorA("Binary BSON cannot be null");
    return NULL;
  }

  if (!(c_bson=(uint8_t *)(*env)->GetByteArrayElements(env, bson, JNI_FALSE))) {
    throwCWitsml21ErrorA("Can't reference Java byte array to C pointer");
    return NULL;
  }

  if (!(c_bson_size=(size_t)(*env)->GetArrayLength(env, bson))) {
    throwCWitsml21ErrorA("Empty Binary BSON");
    goto Java_org_jwitsml21parser_JWitsml21_bsonToString_resume1;
  }

  if (!(c_json=cws_data_to_json(&c_json_len, c_bson, c_bson_size))) {
    throwCWitsml21ErrorA("Invalid BSON data");
    goto Java_org_jwitsml21parser_JWitsml21_bsonToString_resume1;
  }

  j_json = (*env)->NewByteArray(env, (jsize)c_json_len);

  if (!j_json) {
    throwCWitsml21ErrorA("Could not allocate UTF-8 in Java Byte");
    goto Java_org_jwitsml21parser_JWitsml21_bsonToString_resume2;
  }

  (*env)->SetByteArrayRegion(env, j_json, 0, (jsize)c_json_len, (const jbyte *)c_json);

  if ((*env)->ExceptionCheck(env)) {
    throwCWitsml21ErrorA("Could not parse UTF-8 C char to Java string");
    goto Java_org_jwitsml21parser_JWitsml21_bsonToString_resume3;
  }

  free((void *)c_json);

  (*env)->ReleaseByteArrayElements(env, bson, (jbyte *)c_bson, JNI_ABORT);

  return j_json;

Java_org_jwitsml21parser_JWitsml21_bsonToString_resume3:
  (*env)->DeleteLocalRef(env, j_json);

Java_org_jwitsml21parser_JWitsml21_bsonToString_resume2:
  free((void *)c_json);

Java_org_jwitsml21parser_JWitsml21_bsonToString_resume1:
  (*env)->ReleaseByteArrayElements(env, bson, (jbyte *)c_bson, JNI_ABORT);

  return NULL;
}

/////////////////////////////////////////////////////// C Witsml Parser /////////////////////////////////////////////////////// 

int ns21__readWitsmlObject(struct soap *soap_internal, struct ns21__witsmlObject *WitsmlObject, char **result)
{
  readWitsmlObjectFn parser=readWitsmlObjectBsonParser(soap_internal, WitsmlObject);

  if ((parser!=NULL)&&(parser(soap_internal)==SOAP_OK))
      return SOAP_OK;

  return SOAP_FAULT;
}

