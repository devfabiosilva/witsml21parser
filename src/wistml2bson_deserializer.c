#include <cws_bson_utils.h>
#include <cws_config.h>
#include <cws_soap.h>
#include <cws_utils.h>
#include <wistml2bson_deserializer.h>
#include "soapH.h"

_Static_assert(sizeof(LONG64)==sizeof(int64_t), "Wrong long 64");

#define SET_MULTIPLE_ATTRIBUTES(...) __VA_ARGS__, NULL, 0, NULL

#define KEY_USCORE_ATTRIBUTES (const char *)"#attributes", (int)(sizeof("#attributes")-1)

#define ARRAY_OF_PREFIX ""

#define _CWS_NULLABLE_XSD_STR (1<<0)
#define _CWS_NULLABLE_XSD_SIGNED_SHORT (1<<1)

#define CWS_SET_SOAP_FAULT_ERR (int)-1234
#define CWS_SET_SOAP_FAULT(msg) \
  cws_set_soap_fault_util(soap_internal, CWS_SET_SOAP_FAULT_ERR, CWS_FAULTSTR(msg), -1);

#define CWS_SET_SOAP_FAULT_ERR_CLIENT (int)-1233
#define CWS_SET_SOAP_FAULT_CLIENT(msg) \
  cws_set_soap_fault_util(soap_internal, CWS_SET_SOAP_FAULT_ERR_CLIENT, CWS_FAULTSTR(msg), 0);

#define CWS_CONSTRUCT_BSON(nameObj) \
  if (!(((CWS_CONFIG *)soap_internal->user)->object=CWS_BSON_NEW)) { \
    CWS_SET_SOAP_FAULT("Could not create new BSON at " #nameObj) \
    return SOAP_FAULT; \
  }

#define GET_INTERNAL_SOAP_BSON \
  ((CWS_CONFIG *)soap_internal->user)->object

#define GET_INTERNAL_SOAP_WITSML_OBJECT \
  ((CWS_CONFIG *)soap_internal->user)->WitsmlObject

#define CWS_BSON_FREE \
  cws_bson_free(&((CWS_CONFIG *)soap_internal->user)->object);

#define CWS_DETECT_OBJ(object) \
  if (WitsmlObject->object) { \
    config->object_type=TYPE_##object; \
    config->object_name=#object; \
    fn=bson_read_##object##2_1; \
    n++; \
  }

readWitsmlObjectFn readWitsmlObjectBsonParser(struct soap *soap_internal, struct ns21__witsmlObject *WitsmlObject)
{
  CWS_CONFIG *config=((CWS_CONFIG *)soap_internal->user);
  readWitsmlObjectFn fn=NULL;
  int n=0;

  if (!WitsmlObject) {
    // Sentinel. It should not happen.
    cws_set_soap_fault_util(soap_internal, -1232, CWS_FAULTSTR("Unexpected error. Check gSoap implementation"), -1);
    return NULL;
  }

  config->WitsmlObject=WitsmlObject;

  CWS_DETECT_OBJ(BhaRun)
  CWS_DETECT_OBJ(CementJob)
  CWS_DETECT_OBJ(DepthRegImage)
  CWS_DETECT_OBJ(DownholeComponent)
  CWS_DETECT_OBJ(DrillReport)
  CWS_DETECT_OBJ(FluidsReport)
  CWS_DETECT_OBJ(Log)
  CWS_DETECT_OBJ(MudLogReport)
  CWS_DETECT_OBJ(OpsReport)
  CWS_DETECT_OBJ(Rig)
  CWS_DETECT_OBJ(Risk)
  CWS_DETECT_OBJ(StimJob)
  CWS_DETECT_OBJ(SurveyProgram)
  CWS_DETECT_OBJ(Target)
  CWS_DETECT_OBJ(ToolErrorModel)
  CWS_DETECT_OBJ(Trajectory)
  CWS_DETECT_OBJ(Tubular)
  CWS_DETECT_OBJ(Well)
  CWS_DETECT_OBJ(Wellbore)
  CWS_DETECT_OBJ(WellboreCompletion)
  CWS_DETECT_OBJ(WellboreGeology)
  CWS_DETECT_OBJ(WellCMLedger)
  CWS_DETECT_OBJ(WellCompletion)

  if (n==1)
    return fn;

  config->object_type=TYPE_None;
  config->object_name="Unknown object";

  if (n)
    CWS_SET_SOAP_FAULT_CLIENT("Too many objects. Only one allowed!")
  else
    CWS_SET_SOAP_FAULT_CLIENT("No object to parse")

  return NULL;
}

#undef CWS_DETECT_OBJ

static
int bson_put_multiple_attributes_if_they_exist(bson_t *bson, ...)
{
  int
    err=0;
  const char *key;
  size_t key_length;
  char *value;
  va_list args;
  bson_t
    *childAttributesPtr=NULL,
    childAttributes;

  va_start(args, bson);
  while ((key=(const char *)va_arg(args, const char *))) {
    key_length=(size_t)va_arg(args, size_t);
    if ((value=(char *)va_arg(args, char *))) {

      if (childAttributesPtr) {

bson_put_multiple_attributes_if_they_exist_add:
        if (!bson_append_utf8(childAttributesPtr, key, key_length, value, -1)) {
          err=20;
          break;
        }

      } else if (bson_append_document_begin(bson, KEY_USCORE_ATTRIBUTES, childAttributesPtr=&childAttributes))
        goto bson_put_multiple_attributes_if_they_exist_add;
      else {
        childAttributesPtr=NULL;
        err=19;
        break;
      }
    }
  };

  if ((childAttributesPtr!=NULL)&&(!bson_append_document_end(bson, childAttributesPtr))&&(err==0))
    err=21;

  va_end(args);

  return err;
}

static
int bson_put_two_attributes_if_exist(
  bson_t *bson,
  const char *key1, int key1_length, char *value1, int value1_length,
  const char *key2, int key2_length, char *value2, int value2_length
)
{
  int err;
  bson_t childAttributes;

  if ((value1!=NULL)||(value2!=NULL)) {
    if (!bson_append_document_begin(bson, KEY_USCORE_ATTRIBUTES, &childAttributes))
      return 15;

    err=0;
    if ((value1!=NULL)&&(!bson_append_utf8(&childAttributes, key1, key1_length, (const char *)value1, value1_length)))
      err=16;
    else if ((value2!=NULL)&&(!bson_append_utf8(&childAttributes, key2, key2_length, (const char *)value2, value2_length)))
      err=17;

    if (!bson_append_document_end(bson, &childAttributes))
      if (!err)
        err=18;

    return err;
  }

  return 0;
}

static
int bson_put_single_attribute_if_exists(bson_t *bson, const char *key, int key_length, char *value, int value_length)
{
  int err;
  bson_t childAttributes;

  if (value) {
    if (!bson_append_document_begin(bson, KEY_USCORE_ATTRIBUTES, &childAttributes))
      return 10;

    err=0;
    if (!bson_append_utf8(&childAttributes, key, key_length, (const char *)value, value_length))
      err=11;

    if (!bson_append_document_end(bson, &childAttributes))
      if (!err)
        err=12;

    return err;
  }

  return 0;
}

//value is NOT NULL
static
int bson_put_single_attribute_required(bson_t *bson, const char *key, int key_length, char *value, int value_length)
{
  int err;
  bson_t childAttributes;

  if (value) {
    if (!bson_append_document_begin(bson, KEY_USCORE_ATTRIBUTES, &childAttributes))
      return 10;

    err=0;
    if (!bson_append_utf8(&childAttributes, key, key_length, (const char *)value, value_length))
      err=11;

    if (!bson_append_document_end(bson, &childAttributes))
      if (!err)
        err=12;

    return err;
  }

  return 13;
}
//

#define KEY_USCORE_VALUE (const char *)"#value", (int)(sizeof("#value")-1)

static
int bson_read_parse_empty_object(struct soap *soap_internal, bson_t *bson, const char *key, int key_length)
{
  bson_t child;

  if (!bson_append_document_begin(bson, key, key_length, &child)) {
    CWS_SET_SOAP_FAULT("emptyObject(BEGIN). Could append empty object to BSON")
    return SOAP_FAULT;
  }

  if (!bson_append_document_end (bson, &child)) {
    CWS_SET_SOAP_FAULT("emptyObject(END). Could append empty object to BSON")
    return SOAP_FAULT;
  }

  return SOAP_OK;
}

static
int bson_read_parse_empty_array(struct soap *soap_internal, bson_t *bson, const char *key, int key_length)
{
  bson_t child;

  if (!bson_append_array_begin(bson, key, key_length, &child)) {
    CWS_SET_SOAP_FAULT("arrayObject(BEGIN). Could append empty array to BSON")
    return SOAP_FAULT;
  }

  if (!bson_append_array_end(bson, &child)) {
    CWS_SET_SOAP_FAULT("arrayObject(END). Could append empty array to BSON")
    return SOAP_FAULT;
  }

  return SOAP_OK;
}

//#define WITH_STATISTICS

#ifdef WITH_STATISTICS
 #define CAPTURE_STAT(txt) (((CWS_CONFIG *)soap_internal->user)->statistics).txt##s++;
 #define CAPTURE_STAT_A(n, txt) (((CWS_CONFIG *)soap_internal->user)->statistics).txt##s+=n;
 #define CAPTURE_STAT_ON_SUCCESS_A(n, txt) if (!(((CWS_CONFIG *)soap_internal->user)->internal_soap_error)) \
  (((CWS_CONFIG *)soap_internal->user)->statistics).txt##s+=n;
 #define CAPTURE_STAT_ON_SUCCESS(txt) if (!(((CWS_CONFIG *)soap_internal->user)->internal_soap_error)) \
  (((CWS_CONFIG *)soap_internal->user)->statistics).txt##s++;
#else
 #define CAPTURE_STAT(txt)
 #define CAPTURE_STAT_A(n, txt)
 #define CAPTURE_STAT_ON_SUCCESS_A(n, txt)
 #define CAPTURE_STAT_ON_SUCCESS(txt)
#endif

/// READER MACROS
#define READ_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, measureType, onErrorGoto) \
  if (bson_read_##measureType##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto onErrorGoto##_resume;

//TO BE USED IN ARRAY FUNCTION
#define READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, measureType) \
  if (bson_read_##measureType##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_helper_resume;

//uom is required
#define READ_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(bsonType, objectParent, objectName, measureType, onErrorGoto) \
  if (bson_read_##measureType##21_uom_required_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto onErrorGoto##_resume;

//uom is required
#define READ_MEASURE_OBJECT_21_VOID_B(bsonType, objectParent, objectName, measureType) \
  bson_read_##measureType##21_uom_required_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName);

//Used for object function
#define READ_O_MEASURE_OBJECT_21_VOID_B(objectParent, objectName, measureType) \
  READ_MEASURE_OBJECT_21_VOID_B(&child, objectParent, objectName, measureType)

//Used for object function
#define READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(objectParent, objectName, measureType) \
  if (bson_read_##measureType##21_uom_required_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_resume;

//Used for array function
#define READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(objectParent, objectName, measureType) \
  if (bson_read_##measureType##21_uom_required_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_helper_resume;

//Used for object function
#define READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, measureType) \
  if (bson_read_##measureType##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_resume;

#define READ_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for array function
#define READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for array function
#define READ_A_UTF8_OBJECT_ITEM_21_OR_ELSE_GOTO_RESUME(objectParent) \
  if (objectParent->__item) { \
    if (!bson_append_utf8(thisBson, KEY_USCORE_VALUE, (const char *)objectParent->__item, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read _value of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for objectfunction
#define READ_O_DOUBLE_ITEM_21_VOID(objectParent) \
  if (!bson_append_double(&child, KEY_USCORE_VALUE, objectParent->__item)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read _value of " #objectParent " object") \
    CAPTURE_STAT_A(-1, double) \
  } \
  CAPTURE_STAT(double)

#define READ_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, onErrorGoto) \
  if (!bson_append_double(bsonType, KEY_USCORE_VALUE, objectParent->__item)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read _value of " #objectParent " object") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(double)

//Used for object function
#define READ_O_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(objectParent) \
  READ_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(&child, objectParent, bson_read_##objectParent##21_util)

//Used for array function
#define READ_A_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(objectParent) \
  READ_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(thisBson, objectParent, bson_read_##objectParent##21_util_helper)

#define READ_DOUBLE_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (!bson_append_double(thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read double value from" #objectName " of " #objectParent " object") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(double)

//Used for array function
#define READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_DOUBLE_21_OR_ELSE_GOTO_RESUME(thisBson, objectParent, objectName, bson_read_##objectParent##21_util_helper)

//Used for array function
#define READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_double(thisBson, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) \
    { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read double value from" #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(double) \
  }

//Used for object function
#define READ_O_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_double(&child, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) \
    { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read double value from" #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(double) \
  }

//Used for array function
#define READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (objectParent->ns##__##objectName) { \
    if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->ns##__##objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for object function
#define READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for object function
#define READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (objectParent->ns##__##objectName) { \
    if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->ns##__##objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(string) \
  }

//Used for object function
#define READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_bool(&child, CWS_CONST_BSON_KEY(#objectName), (bool)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(boolean) \
  }

//Used for object function
#define READ_O_BOOLEAN_NULLABLE_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_bool(&child, CWS_CONST_BSON_KEY(#objectName), (bool)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, boolean) \
    } \
    CAPTURE_STAT(boolean) \
  }

#define READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (!bson_append_double(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto bson_read_##objectParent##21_util_resume; \
  } \
  CAPTURE_STAT(double)

#define READ_O_DOUBLE_21_VOID(objectParent, objectName) \
  if (!bson_append_double(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    CAPTURE_STAT_A(-1, double) \
  } \
  CAPTURE_STAT(double)

//Used for array function
#define READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_bool(thisBson, CWS_CONST_BSON_KEY(#objectName), (bool)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(boolean) \
  }

#define READ_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (objectParent->objectName) { \
    if (!bson_append_bool(bsonType, CWS_CONST_BSON_KEY(#objectName), (bool)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(boolean) \
  }

#define READ_BOOLEAN_NULLABLE_21_VOID(bsonType, objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_bool(bsonType, CWS_CONST_BSON_KEY(#objectName), (bool)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, boolean) \
    } \
    CAPTURE_STAT(boolean) \
  }

//For array function
#define READ_A_BOOLEAN_NULLABLE_21_VOID(objectParent, objectName) \
  READ_BOOLEAN_NULLABLE_21_VOID(thisBson, objectParent, objectName)

#define READ_BOOLEAN_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (!bson_append_bool(bsonType, CWS_CONST_BSON_KEY(#objectName), (bool)objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(boolean)

#define READ_BOOLEAN_21_VOID(bsonType, objectParent, objectName) \
  if (!bson_append_bool(bsonType, CWS_CONST_BSON_KEY(#objectName), (bool)objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    CAPTURE_STAT_A(-1, boolean) \
  } \
  CAPTURE_STAT(boolean)

//For array function
#define READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_BOOLEAN_21_OR_ELSE_GOTO_RESUME(thisBson, objectParent, objectName, bson_read_##objectParent##21_util_helper)

//For object function
#define READ_A_BOOLEAN_21_VOID(objectParent, objectName) \
  READ_BOOLEAN_21_VOID(thisBson, objectParent, objectName)

//For object function
#define READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_BOOLEAN_21_OR_ELSE_GOTO_RESUME(&child, objectParent, objectName, bson_read_##objectParent##21_util)

//For object function
#define READ_O_BOOLEAN_21_VOID(objectParent, objectName) \
  READ_BOOLEAN_21_VOID(&child, objectParent, objectName)

#define READ_UTF8_OBJECT_21_VOID(bsonType, objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) {\
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, string) \
    } \
    CAPTURE_STAT(string) \
  }

#define READ_O_UTF8_OBJECT_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, string) \
    } \
    CAPTURE_STAT(string) \
  }

//Used for array functions
#define READ_A_UTF8_OBJECT_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->objectName, -1)) {\
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, string) \
    } \
    CAPTURE_STAT(string) \
  }

#define READ_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if ((objectParent->objectName!=NULL)&&(!bson_append_int64(bsonType, \
      CWS_CONST_BSON_KEY(#objectName), (int64_t)*(objectParent->objectName)))) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto onErrorGoto##_resume; \
  }

//Used for array function
#define READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_int64(thisBson, CWS_CONST_BSON_KEY(#objectName), (int64_t)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(long64) \
  }

//Used for array function
#define READ_A_LONG64_NULLABLE_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_int64(thisBson, CWS_CONST_BSON_KEY(#objectName), (int64_t)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      CAPTURE_STAT_A(-1, long64) \
    } \
    CAPTURE_STAT(long64) \
  }

#define READ_LONG64_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (!bson_append_int64(bsonType, CWS_CONST_BSON_KEY(#objectName), (int64_t)objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(long64)

#define READ_LONG64_21_VOID(bsonType, objectParent, objectName) \
  if (!bson_append_int64(bsonType, CWS_CONST_BSON_KEY(#objectName), (int64_t)objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    CAPTURE_STAT_A(-1, long64) \
  } \
  CAPTURE_STAT(long64)

//For object function
#define READ_O_LONG64_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_LONG64_21_OR_ELSE_GOTO_RESUME(&child, objectParent, objectName, bson_read_##objectParent##21_util)

//For object function
#define READ_O_LONG64_21_VOID(objectParent, objectName) \
  READ_LONG64_21_VOID(&child, objectParent, objectName)

//Used for object function
#define READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_int64(&child, CWS_CONST_BSON_KEY(#objectName), (int64_t)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(long64) \
  }

//Used for object function
#define READ_O_LONG64_NULLABLE_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_int64(&child, CWS_CONST_BSON_KEY(#objectName), (int64_t)*(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      CAPTURE_STAT_A(-1, long64) \
    } \
    CAPTURE_STAT(long64) \
  }

//Used for array function
#define READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (!bson_append_int64(thisBson, CWS_CONST_BSON_KEY(#objectName), (int64_t)objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto bson_read_##objectParent##21_util_helper_resume; \
  } \
  CAPTURE_STAT(long64)

#define READ_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(enum) \
  }

//Used for array function
#define READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(enum) \
  }

//Used for array function
#define READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto bson_read_##objectParent##21_util_helper_resume; \
  } \
  CAPTURE_STAT(enum)

//Used for array function
#define READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->ns##__##objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto bson_read_##objectParent##21_util_helper_resume; \
  } \
  CAPTURE_STAT(enum)

//Used for object function
#define READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->ns##__##objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto bson_read_##objectParent##21_util_resume; \
  } \
  CAPTURE_STAT(enum)

//Used for object function
#define READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto bson_read_##objectParent##21_util_resume; \
  } \
  CAPTURE_STAT(enum)

//Used for object function
#define READ_O_OBJECT_ENUM_21_VOID(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    CAPTURE_STAT_A(-1, enum) \
  } \
  CAPTURE_STAT(enum)

//Used for object function
#define READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(enum) \
  }

//Used for object function
#define READ_O_OBJECT_ENUM_NULLABLE_21_VOID(ns, objectParent, objectName, enumFunctionName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      CAPTURE_STAT_A(-1, enum) \
    } \
    CAPTURE_STAT(enum) \
  }

#define READ_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (objectParent->ns##__##objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), \
      soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->ns##__##objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(enum) \
  }

#define READ_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->ns##__##objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(enum)

#define READ_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(enum)

#define READ_OBJECT_ENUM_NULLABLE_21_VOID(ns, bsonType, objectParent, objectName, enumFunctionName) \
  if (objectParent->objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, *(objectParent->objectName)), -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
      CAPTURE_STAT_A(-1 ,enum) \
    } \
    CAPTURE_STAT(enum) \
  }
/*
#define READ_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName),soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(enum)
*/
#define READ_OBJECT_ENUM_21_OR_ELSE_GOTO_VOID(ns, bsonType, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
  } \
  CAPTURE_STAT(enum)

//Used for array function
#define READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_VOID(ns, objectParent, objectName, enumFunctionName) \
  if (!bson_append_utf8(thisBson, CWS_CONST_BSON_KEY(#objectName), soap_##ns##__##enumFunctionName##2s(soap_internal, objectParent->objectName), -1)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ)->" #objectName " error") \
    CAPTURE_STAT_A(-1 ,enum) \
  } \
  CAPTURE_STAT(enum)

#define READ_A_PUT_SINGLE_ATTR_21_VOID(objectParent, objectName) \
  if (bson_put_single_attribute_if_exists(thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName, -1)) \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set uid to _attribute in " #objectName)

//Default attributes for
#define READ_PUT_DEFAULT_ATTRIBUTES_21_VOID(bsonType, objectParent) \
  if (bson_put_multiple_attributes_if_they_exist(bsonType, SET_MULTIPLE_ATTRIBUTES( \
      CWS_CONST_BSON_KEY("uuid"), objectParent->uuid, \
      CWS_CONST_BSON_KEY("schemaVersion"), objectParent->schemaVersion, \
      CWS_CONST_BSON_KEY("objectVersion"), objectParent->objectVersion \
    ))) CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set default attributes to _attribute")

//Multiple attributes for
#define READ_PUT_MULTIPLE_ATTRIBUTES_21_VOID(bsonType, objectParent, ...) \
  if (bson_put_multiple_attributes_if_they_exist(bsonType, __VA_ARGS__)) \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set default attributes to _attribute")

//For array functiom
#define READ_A_PUT_MULTIPLE_ATTRIBUTES_21_VOID(objectParent, ...) \
  if (bson_put_multiple_attributes_if_they_exist(thisBson, __VA_ARGS__)) \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set default attributes to _attribute")

#define READ_O_PUT_SINGLE_ATTR_21_VOID(objectParent, objectName) \
  if (bson_put_single_attribute_if_exists(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName, -1)) \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set uid to _attribute in " #objectName)

#define READ_PUT_TWO_ATTR_21_VOID(bsonType, objectParent, objectName1, objectName2) \
  if (bson_put_two_attributes_if_exist(bsonType, CWS_CONST_BSON_KEY(#objectName1), objectParent->objectName1, -1, CWS_CONST_BSON_KEY(#objectName2), \
    objectParent->objectName2, -1)) CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set 2 attributes to _attribute")

//Second attribute is required enum ns2
#define READ_PUT_TWO_ATTR_21_VOID_B(bsonType, objectParent, objectName1, objectName2, enumTypeSuffixFunction) \
  if (bson_put_two_attributes_if_exist(bsonType, CWS_CONST_BSON_KEY(#objectName1), objectParent->objectName1, -1, CWS_CONST_BSON_KEY(#objectName2), \
    (char *)soap_ns2__##enumTypeSuffixFunction##2s(soap_internal, objectParent->objectName2), -1)) \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not set 2 attributes to _attribute")

// for array objects
#define READ_A_PUT_TWO_ATTR_21_VOID(objectParent, objectName1, objectName2) \
  READ_PUT_TWO_ATTR_21_VOID(thisBson, objectParent, objectName1, objectName2)

// for array objects
#define READ_A_PUT_TWO_ATTR_21_VOID_B(objectParent, objectName1, objectName2, enumTypeSuffixFunction) \
  READ_PUT_TWO_ATTR_21_VOID_B(thisBson, objectParent, objectName1, objectName2, enumTypeSuffixFunction)

// for objects
#define READ_O_PUT_TWO_ATTR_21_VOID(objectParent, objectName1, objectName2) \
  READ_PUT_TWO_ATTR_21_VOID(&child, objectParent, objectName1, objectName2)

#define READ_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, onErrorGoto) \
  if (objectParent->ns##__##objectName) { \
    if (!bson_append_utf8(bsonType, CWS_CONST_BSON_KEY(#objectName), (const char *)objectParent->ns##__##objectName, -1)) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(string) \
  }

#define READ_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (bson_read_arrayOfString21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, \
    objectParent->objectName, -1)) \
    goto onErrorGoto##_resume;

//Used for array function
#define READ_A_PUT_DEFAULT_ATTRIBUTES_21_VOID(objectParent) \
  READ_PUT_DEFAULT_ATTRIBUTES_21_VOID(thisBson, objectParent)

//Used for object function
#define READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(objectParent) \
  READ_PUT_DEFAULT_ATTRIBUTES_21_VOID(&child, objectParent)

//Used for object function
#define READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (bson_read_arrayOfString21_util(soap_internal, &child, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, \
    objectParent->objectName, -1)) goto bson_read_##objectParent##21_util_resume;

//Used for object function
#define READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (bson_read_arrayOfString21_util(soap_internal, &child, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, \
    objectParent->ns##__##objectName, -1)) goto bson_read_##objectParent##21_util_resume;

#define READ_ARRAY_OF_UTF8_OBJECT_21_VOID(bsonType, objectParent, objectName) \
  bson_read_arrayOfString21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, objectParent->objectName, -1);

//Used for object function
#define READ_O_ARRAY_OF_UTF8_OBJECT_21_VOID(objectParent, objectName) \
  bson_read_arrayOfString21_util(soap_internal, &child, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, objectParent->objectName, -1);

//Used for array function
#define READ_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, onErrorGoto) \
  if (bson_read_arrayOfString21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, \
    objectParent->ns##__##objectName, -1)) goto onErrorGoto##_resume;

//Used for array function
#define READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  bson_read_arrayOfString21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName, objectParent->objectName, -1);

//Used for array function
#define READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (bson_read_arrayOfString21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), objectParent->__size##objectName,\
   objectParent->ns##__##objectName, -1)) goto bson_read_##objectParent##21_util_helper_resume;

#define READ_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(bsonType, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto onErrorGoto##_resume; \
    } \
    CAPTURE_STAT(date_time) \
  }

#define READ_TIME_NULLABLE_21_VOID(bsonType, objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(bsonType, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, date_time) \
    } \
    CAPTURE_STAT(date_time) \
  }

#define READ_A_TIME_NULLABLE_21_VOID(objectParent, objectName) \
  READ_TIME_NULLABLE_21_VOID(thisBson, objectParent, objectName)

//Used for object function
#define READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_resume; \
    } \
    CAPTURE_STAT(date_time) \
  }

//Used for object function
#define READ_O_TIME_NULLABLE_21_VOID(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      CAPTURE_STAT_A(-1, date_time) \
    } \
    CAPTURE_STAT(date_time) \
  }

//Used for array function
#define READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(thisBson, CWS_CONST_BSON_KEY(#objectName), *(objectParent->objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(date_time) \
  }

//Used for array function
#define READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (objectParent->objectName) { \
    if (!bson_append_time_t(thisBson, CWS_CONST_BSON_KEY(#objectName), *(objectParent->ns##__##objectName))) { \
      CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
      goto bson_read_##objectParent##21_util_helper_resume; \
    } \
    CAPTURE_STAT(date_time) \
  }

//Used for array function
#define READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (!bson_append_time_t(thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto bson_read_##objectParent##21_util_helper_resume; \
  } \
  CAPTURE_STAT(date_time)

#define READ_TIME_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, onErrorGoto) \
  if (!bson_append_time_t(bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto onErrorGoto##_resume; \
  } \
  CAPTURE_STAT(date_time)

//Used for array function
#define READ_A_TIME_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (!bson_append_time_t(thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto bson_read_##objectParent##21_util_helper_resume; \
  } \
  CAPTURE_STAT(date_time)

//Used for object function
#define READ_O_TIME_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto bson_read_##objectParent##21_util_resume; \
  } \
  CAPTURE_STAT(date_time)

//Used for object function
#define READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    goto bson_read_##objectParent##21_util_resume; \
  } \
  CAPTURE_STAT(date_time)

//Used for object function
#define READ_O_TIME_21_VOID(objectParent, objectName) \
  if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) { \
    CWS_SET_SOAP_FAULT(#objectParent "(READ). Could not read " #objectName " of " #objectParent " object") \
    CAPTURE_STAT_A(-1, date_time) \
  } \
  CAPTURE_STAT(date_time)

#define READ_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, arrayTypeName, onErrorGoto) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    bsonType, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1)) goto onErrorGoto##_resume;

#define READ_ARRAY_OF_OBJECT_ALIAS_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, aliasName, arrayTypeName, onErrorGoto) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    bsonType, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #aliasName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1)) goto onErrorGoto##_resume;  

//Used for array function
#define READ_A_ARRAY_OF_OBJECT_ALIAS_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, aliasName, arrayTypeName) \
  READ_ARRAY_OF_OBJECT_ALIAS_21_OR_ELSE_GOTO_RESUME(thisBson, objectParent, objectName, aliasName, arrayTypeName, bson_read_##objectParent##21_util_helper)

#define READ_ARRAY_OF_OBJECT_21_VOID(bsonType, objectParent, objectName, arrayTypeName) \
  bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    bsonType, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1);

//Used for object function
#define READ_O_ARRAY_OF_OBJECT_21_VOID(objectParent, objectName, arrayTypeName) \
  bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    &child, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1);

//Used for object function
#define READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, arrayTypeName) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    &child, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1)) goto bson_read_##objectParent##21_util_resume;

//Used for object function
#define READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, arrayTypeName) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    &child, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->ns##__##objectName, \
    -1)) goto bson_read_##objectParent##21_util_resume;

#define READ_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, arrayTypeName, onErrorGoto) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    bsonType, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->ns##__##objectName, \
    -1)) goto onErrorGoto##_resume;

//Use for array function
#define READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, arrayTypeName) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    thisBson, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->ns##__##objectName, \
    -1)) goto bson_read_##objectParent##21_util_helper_resume;

//Use for array function
#define READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, arrayTypeName) \
  if (bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    thisBson, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1)) goto bson_read_##objectParent##21_util_helper_resume;

//Use for array function
#define READ_A_ARRAY_OF_OBJECT_21_VOID(objectParent, objectName, arrayTypeName) \
  bson_read_arrayOf##arrayTypeName##21_util( \
    soap_internal, \
    thisBson, \
    CWS_CONST_BSON_KEY(ARRAY_OF_PREFIX #objectName), \
    CWS_CONST_BSON_KEY(NULL), \
    objectParent->__size##objectName, \
    objectParent->objectName, \
    -1);

#define READ_OBJECT_21_OR_ELSE_GOTO_RESUME(bsonType, objectParent, objectName, typeName, onErrorGoto) \
  if (bson_read_##typeName##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto onErrorGoto##_resume;

//Use for object function
#define READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_resume;

//Use for object function. Use ns
#define READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName)) \
    goto bson_read_##objectParent##21_util_resume;

//Use for object function. Use ns
#define READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns, objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName, -1)) \
    goto bson_read_##objectParent##21_util_resume;

//Use for object function. Extra parameter -1
#define READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_D(objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName, -1)) \
    goto bson_read_##objectParent##21_util_resume;

//Use for object function
#define READ_O_OBJECT_21_VOID(objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName);

//Use for object function
#define READ_O_OBJECT_21_VOID_B(ns, objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, &child, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName);

#define READ_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, bsonType, objectParent, objectName, typeName, onErrorGoto) \
  if (bson_read_##typeName##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName)) \
    goto onErrorGoto##_resume;

#define READ_OBJECT_21_VOID(bsonType, objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName);

#define READ_OBJECT_21_VOID_B(ns, bsonType, objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName);

//Use for array function
#define READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName)) \
    goto bson_read_##objectParent##21_util_helper_resume;

//Use for array function
#define READ_A_OBJECT_21_VOID(objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->objectName);

//Use for array function
#define READ_A_OBJECT_21_VOID_B(ns, objectParent, objectName, typeName) \
  bson_read_##typeName##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName);

//Use for array function
#define READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName)) \
    goto bson_read_##objectParent##21_util_helper_resume;

//Use for array function
#define READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns, objectParent, objectName, typeName) \
  if (bson_read_##typeName##21_util(soap_internal, thisBson, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName, -1)) \
    goto bson_read_##objectParent##21_util_helper_resume;

#define READ_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns, bsonType, objectParent, objectName, typeName, onErrorGoto) \
  if (bson_read_##typeName##21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), objectParent->ns##__##objectName, -1)) \
    goto onErrorGoto##_resume;

/// BEGIN BSON_READ_ARRAY_OF_OBJECT_BUILDER_21
#define BSON_READ_ARRAY_OF_OBJECT_BUILDER_21(ns, type) \
static \
int bson_read_arrayOf##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *array_key, int array_key_length, \
  const char *key, int key_length, \
  int numberOfObjectsInArray, \
  struct ns##__##type *type, \
  int addEmptyObjectAsEmptyJson \
) \
{ \
  int i; \
  size_t index_str_key_len; \
  IdxType index_str_buf; \
  const char *index_str_key; \
  idx2str_fn indexer; \
  bson_t \
    childArray, \
    thisArray; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!(indexer=init_indexer(numberOfObjectsInArray, index_str_buf))) { \
    CWS_SET_SOAP_FAULT(#type "(READ). Invalid index array") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_array_begin(bson, array_key, array_key_length, &childArray)) { \
    CWS_SET_SOAP_FAULT(#type "(READ). BEGIN ARRAY. Error") \
    return SOAP_FAULT; \
  } \
\
  i=0; \
  bson_init(&thisArray); \
\
  do { \
    if (type) { \
      if (bson_read_##type##21_util_helper( \
        soap_internal, \
        &thisArray, \
        key, key_length, \
        type)) break; \
\
bson_read_arrayOf##type##21_util_IDX: \
      index_str_key_len=indexer((uint32_t)i, &index_str_key, index_str_buf); \
      if (!bson_append_document(&childArray, index_str_key, (int)index_str_key_len, &thisArray)) { \
        CWS_SET_SOAP_FAULT(#type "(READ). APPEND Error") \
        break; \
      } \
\
    } else if (addEmptyObjectAsEmptyJson) { \
      if ((key!=NULL)&&(!bson_read_parse_empty_object(soap_internal, &thisArray, key, key_length))) \
        break; \
\
      goto bson_read_arrayOf##type##21_util_IDX; \
    } \
\
    if (numberOfObjectsInArray>(++i)) { \
      if (((++type)!=NULL)||(addEmptyObjectAsEmptyJson!=0)) \
        bson_reinit(&thisArray); \
    } else \
      break; \
\
  } while (1); \
\
  if (!bson_append_array_end(bson, &childArray)) \
    CWS_SET_SOAP_FAULT(#type "(READ). END Error") \
\
  bson_destroy(&thisArray); \
\
  CAPTURE_STAT_ON_SUCCESS_A(numberOfObjectsInArray, array) \
\
  CWS_RETURN(soap_internal); \
}

#define BSON_READ_MEASURE_BUILDER_21(type, enumTypeSuffixFunction) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (bson_put_single_attribute_if_exists( \
    &child, \
    CWS_CONST_BSON_KEY("uom"), \
    (char *)((type->uom)?(soap_ns2__##enumTypeSuffixFunction##2s(soap_internal, *(type->uom))):NULL), -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

//uom is required in type B
#define BSON_READ_MEASURE_BUILDER_21_B(type, enumTypeSuffixFunction) \
static \
int bson_read_##type##21_uom_required_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_uom_required_util_resume; \
  } \
\
  if (bson_put_single_attribute_required( \
    &child, CWS_CONST_BSON_KEY("uom"), (char *)(soap_ns2__##enumTypeSuffixFunction##2s(soap_internal, type->uom)), -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_uom_required_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

//No enum
#define BSON_READ_MEASURE_BUILDER_21_C(type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (bson_put_single_attribute_if_exists( \
    &child, CWS_CONST_BSON_KEY("uom"),  type->uom, -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

//required uom and value (__item)
#define BSON_READ_MEASURE_BUILDER_21_D(type, enumTypeSuffixFunction) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (bson_put_single_attribute_required( \
    &child, \
    CWS_CONST_BSON_KEY("uom"), \
    (char *)soap_ns2__##enumTypeSuffixFunction##2s(soap_internal, type->uom), -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

//uom is required
#define BSON_READ_MEASURE_BUILDER_21_E(type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (bson_put_single_attribute_required( \
    &child, CWS_CONST_BSON_KEY("uom"),  type->uom, -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

//Special case date time dTim required
#define BSON_READ_TIMESTAMP_COMMENT_STRING_BUILDER_21(type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns1__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if ((type->__item!=NULL)&&(!bson_append_utf8(&child, KEY_USCORE_VALUE, type->__item, -1))) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (!bson_append_time_t(&child, CWS_CONST_BSON_KEY("dTim"), type->dTim)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->dTim") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(date_time) \
\
  CWS_RETURN(soap_internal) \
}

//aqui
//Special case for eventType
#define BSON_READ_EVENT_TYPE_BUILDER_21(type, enumTypeSuffixFunction) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns1__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if ((type->__item!=NULL)&&(!bson_append_utf8(&child, KEY_USCORE_VALUE, type->__item, -1))) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (!bson_append_utf8(&child, CWS_CONST_BSON_KEY("Class"), (char *)soap_ns1__##enumTypeSuffixFunction##2s(soap_internal, type->Class), -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->Class") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(event_type) \
\
  CWS_RETURN(soap_internal) \
}

#define BSON_READ_UNITY_BUILDER_21(type, enumTypeSuffixFunction) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns1__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if ((type->__item!=NULL)&&(!bson_append_utf8(&child, KEY_USCORE_VALUE, type->__item, -1))) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (!bson_put_two_attributes_if_exist(&child, \
    CWS_CONST_BSON_KEY("authority"), type->authority, -1, \
    CWS_CONST_BSON_KEY("kind"), (char *)soap_ns2__##enumTypeSuffixFunction##2s(soap_internal, type->kind), -1) \
  ) CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attributes " #type "->authority and " #type "->kind") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(kind) \
\
  CWS_RETURN(soap_internal) \
}

#define BSON_READ_STRING_MEASURE_NULLABLE_BUILDER_21(type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT( #type "21(READ). BEGIN: Could not append document in BSON" ) \
    return SOAP_FAULT; \
  } \
\
  if ((type->__item!=NULL)&&(!bson_append_utf8(&child, KEY_USCORE_VALUE, type->__item, -1))) { \
    CWS_SET_SOAP_FAULT("xsd:string(READ). Could not read and append xsd:string utf-8 to BSON") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if ((type->uom!=NULL)&&(!bson_append_utf8(&child, CWS_CONST_BSON_KEY("uom"), type->uom, -1))) \
    CWS_SET_SOAP_FAULT("uom(READ). Could not read and append uom utf-8 to BSON") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(measure) \
\
  CWS_RETURN(soap_internal) \
}

/// FUNCTIONS MACROS
//NOTE: struct ns_object * SHOLD NOT BE NULL 
#define BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns, type) \
static \
int bson_read_##type##21_util_helper( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length,\
  struct ns##__##type *type \
) \
{ \
  bson_t \
    *thisBson, \
    child; \
\
  if (key) { \
    if (!bson_append_document_begin(bson, key, key_length, thisBson=&child)) { \
      CWS_SET_SOAP_FAULT(#type "(READ). BEGIN: Could not append document in BSON") \
      return SOAP_FAULT; \
    } \
  } else \
    thisBson=bson;

#define BSON_READ_ARRAY_OF_OBJECT21_END(ns, type) \
bson_read_##type##21_util_helper_resume: \
  if ((key!=NULL)&&(!bson_append_document_end(bson, thisBson))) \
    CWS_SET_SOAP_FAULT(#type"(READ). END: Could not end document") \
\
  CWS_RETURN(soap_internal) \
} \
\
BSON_READ_ARRAY_OF_OBJECT_BUILDER_21(ns, type)

#define BSON_READ_ARRAY_OF_OBJECT21_END_(ns, type) \
  if ((key!=NULL)&&(!bson_append_document_end(bson, thisBson))) \
    CWS_SET_SOAP_FAULT(#type"(READ). END: Could not end document") \
\
  CWS_RETURN(soap_internal) \
} \
\
BSON_READ_ARRAY_OF_OBJECT_BUILDER_21(ns, type)

//struct ns##__##type can be NULL
#define BSON_READ_OBJECT21_BEGIN(ns, type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns##__##type *type \
) \
{ \
  bson_t \
    child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type  "(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  }

#define BSON_READ_OBJECT21_END(type) \
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "(READ). END: Could not end document") \
\
  CWS_RETURN(soap_internal); \
}

#define BSON_READ_OBJECT21_END_(type) \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "(READ). END: Could not end document") \
\
  CWS_RETURN(soap_internal); \
}

//COST builder. Currency required
#define BSON_READ_COST_BUILDER_21_C(type) \
static \
int bson_read_##type##21_util( \
  struct soap *soap_internal, \
  bson_t *bson, \
  const char *key, int key_length, \
  struct ns2__##type *type \
) \
{ \
  bson_t child; \
\
  if (!type) \
    return SOAP_OK; \
\
  if (!bson_append_document_begin(bson, key, key_length, &child)) { \
    CWS_SET_SOAP_FAULT(#type"21(READ). BEGIN: Could not append document in BSON") \
    return SOAP_FAULT; \
  } \
\
  if (!bson_append_double(&child, KEY_USCORE_VALUE, type->__item)) { \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set " #type "->_value") \
    goto bson_read_##type##21_util_resume; \
  } \
\
  if (bson_put_single_attribute_required( \
    &child, \
    CWS_CONST_BSON_KEY("currency"), \
    type->currency, -1)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). Could not set attribute " #type "->uom") \
\
bson_read_##type##21_util_resume: \
  if (!bson_append_document_end(bson, &child)) \
    CWS_SET_SOAP_FAULT(#type "21(READ). END: Could not end document in " #type) \
\
  CAPTURE_STAT_ON_SUCCESS(cost) \
\
  CWS_RETURN(soap_internal) \
}

////

//struct ns2__StringMeasure
BSON_READ_STRING_MEASURE_NULLABLE_BUILDER_21(StringMeasure)

//struct ns2__PlaneAngleMeasure
BSON_READ_MEASURE_BUILDER_21_B(PlaneAngleMeasure, PlaneAngleUom)

//struct ns2__LengthMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(LengthMeasureExt)

//struct ns2__GenericMeasure
BSON_READ_MEASURE_BUILDER_21_C(GenericMeasure)

//struct ns2__AnglePerLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(AnglePerLengthMeasure, AnglePerLengthUom)

//struct ns2__TimeMeasure
BSON_READ_MEASURE_BUILDER_21_D(TimeMeasure, TimeUom)

//struct ns2__ForceMeasure
BSON_READ_MEASURE_BUILDER_21_D(ForceMeasure, ForceUom)

//struct ns2__MomentOfForceMeasure
BSON_READ_MEASURE_BUILDER_21_D(MomentOfForceMeasure, MomentOfForceUom)

//struct ns2__MassPerVolumeMeasure
BSON_READ_MEASURE_BUILDER_21_C(MassPerVolumeMeasure)

//struct ns2__VolumePerTimeMeasure
BSON_READ_MEASURE_BUILDER_21_C(VolumePerTimeMeasure)

//struct ns2__LengthPerTimeMeasure
BSON_READ_MEASURE_BUILDER_21_D(LengthPerTimeMeasure, LengthPerTimeUom)

//struct ns2__PowerMeasure
BSON_READ_MEASURE_BUILDER_21_D(PowerMeasure, PowerUom)

//struct ns2__PressureMeasure
BSON_READ_MEASURE_BUILDER_21_C(PressureMeasure)

//struct ns2__LengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(LengthMeasure, LengthUom)

//struct ns2__AngularVelocityMeasure
BSON_READ_MEASURE_BUILDER_21_D(AngularVelocityMeasure, AngularVelocityUom)

//struct ns2__ThermodynamicTemperatureMeasure
BSON_READ_MEASURE_BUILDER_21_D(ThermodynamicTemperatureMeasure, ThermodynamicTemperatureUom)

//struct ns2__MassPerLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(MassPerLengthMeasure, MassPerLengthUom)

//struct ns2__DynamicViscosityMeasure
BSON_READ_MEASURE_BUILDER_21_D(DynamicViscosityMeasure, DynamicViscosityUom)

//struct ns2__VolumeMeasure
BSON_READ_MEASURE_BUILDER_21_C(VolumeMeasure)

//struct ns2__VolumePerVolumeMeasure
BSON_READ_MEASURE_BUILDER_21_C(VolumePerVolumeMeasure)

//struct ns2__VolumePerMassMeasure
BSON_READ_MEASURE_BUILDER_21_D(VolumePerMassMeasure, VolumePerMassUom)

//struct ns2__DimensionlessMeasure
BSON_READ_MEASURE_BUILDER_21_D(DimensionlessMeasure, DimensionlessUom)

//struct ns2__MassMeasure
BSON_READ_MEASURE_BUILDER_21_D(MassMeasure, MassUom)

//struct ns2__AreaMeasure
BSON_READ_MEASURE_BUILDER_21_D(AreaMeasure, AreaUom)

//struct ns2__ReciprocalLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(ReciprocalLengthMeasure, ReciprocalLengthUom)

//struct ns2__PressureMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(PressureMeasureExt)

//struct ns2__CationExchangeCapacityMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(CationExchangeCapacityMeasureExt)

//struct ns2__ElectricPotentialDifferenceMeasure
BSON_READ_MEASURE_BUILDER_21_D(ElectricPotentialDifferenceMeasure, ElectricPotentialDifferenceUom)

//struct ns2__MassPerMassMeasure
BSON_READ_MEASURE_BUILDER_21_D(MassPerMassMeasure, MassPerMassUom)

//struct ns2__VolumeMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(VolumeMeasureExt)

//struct ns2__ThermodynamicTemperatureMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(ThermodynamicTemperatureMeasureExt)

//struct ns2__PlaneAngleMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(PlaneAngleMeasureExt)

//struct ns2__LengthPerLengthMeasureExt
BSON_READ_MEASURE_BUILDER_21_E(LengthPerLengthMeasureExt)

//struct ns2__IlluminanceMeasure
BSON_READ_MEASURE_BUILDER_21_D(IlluminanceMeasure, IlluminanceUom)

//struct ns2__AreaPerAreaMeasure
BSON_READ_MEASURE_BUILDER_21_D(AreaPerAreaMeasure, AreaPerAreaUom)

//struct ns2__ElectricCurrentMeasure
BSON_READ_MEASURE_BUILDER_21_D(ElectricCurrentMeasure, ElectricCurrentUom)

//struct ns2__VolumePerVolumeMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(VolumePerVolumeMeasureExt)

//struct ns2__ForceMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(ForceMeasureExt)

//struct ns2__ForcePerVolumeMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(ForcePerVolumeMeasureExt)

//struct ns2__LinearAccelerationMeasure
BSON_READ_MEASURE_BUILDER_21_D(LinearAccelerationMeasure, LinearAccelerationUom)

//struct ns2__MagneticFluxDensityMeasure
BSON_READ_MEASURE_BUILDER_21_D(MagneticFluxDensityMeasure, MagneticFluxDensityUom)

//struct ns2__LengthPerLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(LengthPerLengthMeasure, LengthPerLengthUom)

//struct ns2__PowerPerPowerMeasure
BSON_READ_MEASURE_BUILDER_21_D(PowerPerPowerMeasure, PowerPerPowerUom)

//struct ns2__ForcePerVolumeMeasure
BSON_READ_MEASURE_BUILDER_21_D(ForcePerVolumeMeasure, ForcePerVolumeUom)

//struct ns2__IsothermalCompressibilityMeasure
BSON_READ_MEASURE_BUILDER_21_D(IsothermalCompressibilityMeasure, IsothermalCompressibilityUom)

//struct ns2__SpecificHeatCapacityMeasure
BSON_READ_MEASURE_BUILDER_21_D(SpecificHeatCapacityMeasure, SpecificHeatCapacityUom)

//struct ns2__ThermalConductivityMeasure
BSON_READ_MEASURE_BUILDER_21_D(ThermalConductivityMeasure, ThermalConductivityUom)

//struct ns2__VolumetricThermalExpansionMeasure
BSON_READ_MEASURE_BUILDER_21_D(VolumetricThermalExpansionMeasure, VolumetricThermalExpansionUom)

//struct ns2__PermeabilityRockMeasure
BSON_READ_MEASURE_BUILDER_21_D(PermeabilityRockMeasure, PermeabilityRockUom)

//struct ns2__MassPerTimeMeasure
BSON_READ_MEASURE_BUILDER_21_D(MassPerTimeMeasure, MassPerTimeUom)

//struct ns2__VolumePerLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(VolumePerLengthMeasure, VolumePerLengthUom)

//struct ns2__AreaPerMassMeasure
BSON_READ_MEASURE_BUILDER_21_D(AreaPerMassMeasure, AreaPerMassUom)

//struct ns2__PermeabilityLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(PermeabilityLengthMeasure, PermeabilityLengthUom)

//struct ns2__LengthPerTimeMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(LengthPerTimeMeasureExt)

//SPECIAL CASE
//struct ns2__GenericMeasure
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, GenericMeasure)
  READ_A_DOUBLE_ITEM_21_OR_ELSE_GOTO_RESUME(GenericMeasure)
  READ_A_PUT_SINGLE_ATTR_21_VOID(GenericMeasure, uom)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, GenericMeasure)

//struct ns2__MassMeasureExt
BSON_READ_MEASURE_BUILDER_21_C(MassMeasureExt)

//struct ns2__ForcePerLengthMeasure
BSON_READ_MEASURE_BUILDER_21_D(ForcePerLengthMeasure, ForcePerLengthUom)

//struct ns2__DigitalStorageMeasure
BSON_READ_MEASURE_BUILDER_21_D(DigitalStorageMeasure, DigitalStorageUom)

////// COST

BSON_READ_COST_BUILDER_21_C(Cost)

//TIMESTAMP COMMENT STRING
//struct ns1__TimestampedCommentString
BSON_READ_TIMESTAMP_COMMENT_STRING_BUILDER_21(TimestampedCommentString)

// EVENT TYPE
BSON_READ_EVENT_TYPE_BUILDER_21(EventType, EventClassType)

//char **values
static
int bson_read_arrayOfString21_util(
  struct soap *soap_internal,
  bson_t *bson,
  const char *array_key, int array_key_length,
  int numberOfObjectsInArray,
  char **value,
  int addEmptyObjectAsEmptyJson
)
{
  // SPECIAL CASE
  int i;
  size_t index_str_key_len;
  IdxType index_str_buf;
  const char *index_str_key;
  idx2str_fn indexer;
  bson_t
    childArray,
    thisArray;

  if (!numberOfObjectsInArray)
    return SOAP_OK;

  if (!(indexer=init_indexer(numberOfObjectsInArray, index_str_buf))) {
    CWS_SET_SOAP_FAULT("stringArray(READ). Invalid index array")
    return SOAP_FAULT;
  }

  if (!bson_append_array_begin(bson, array_key, array_key_length, &childArray)) {
    CWS_SET_SOAP_FAULT("stringArray(READ). BEGIN ARRAY. Error")
    return SOAP_FAULT;
  }

  i=0;
  bson_init(&thisArray);

  do {
    if (*value) {
      if (!bson_append_utf8(&thisArray, KEY_USCORE_VALUE, *value, -1)) {
        CWS_SET_SOAP_FAULT("stringArray(READ). Could not read and append string utf-8 to BSON array")
        break;
      }

bson_read_arrayOfString21_util_IDX:
      index_str_key_len=indexer((uint32_t)i, &index_str_key, index_str_buf);
      if (!bson_append_document(&childArray, index_str_key, (int)index_str_key_len, &thisArray)) {
        CWS_SET_SOAP_FAULT("stringArray(READ). APPEND Error")
        break;
      }

    } else if (addEmptyObjectAsEmptyJson)
      goto bson_read_arrayOfString21_util_IDX;
    
    if (numberOfObjectsInArray>(++i)) {
      if (((*(++value))!=NULL)||(addEmptyObjectAsEmptyJson!=0))
        bson_reinit(&thisArray);
    } else
      break;

  } while (1);

  if (!bson_append_array_end(bson, &childArray))
    CWS_SET_SOAP_FAULT("stringArray(READ). END Error")

  bson_destroy(&thisArray);

  CAPTURE_STAT_ON_SUCCESS_A(numberOfObjectsInArray, string)

  CWS_RETURN(soap_internal);
}

#define READ_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  if (bson_read_arrayOfEnum21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), \
    objectParent->__size##objectName, (int *)objectParent->objectName, -1, (enum_caller_fn)soap_##ns##__##enumFunctionName##2s)) goto onErrorGoto##_resume;

#define READ_ARRAY_OF_OBJECT_ENUM_21_VOID(ns, bsonType, objectParent, objectName, enumFunctionName, onErrorGoto) \
  bson_read_arrayOfEnum21_util(soap_internal, bsonType, CWS_CONST_BSON_KEY(#objectName), \
    __size##objectName, (int *)objectParent->objectName, -1, (enum_caller_fn)soap_##ns##__##enumFunctionName##2s);

//For array function
#define READ_A_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  READ_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, thisBson, objectParent, objectName, enumFunctionName, bson_read_##objectParent##21_util_helper)

//aqui

enum enum_any_e {
  ENUM_NONE
};

_Static_assert(sizeof(int)==sizeof(enum enum_any_e), "Refactor code. INT and ENUM should NOT be different in size");
typedef char *(*enum_caller_fn)(struct soap *, int);

static
int bson_read_arrayOfEnum21_util(
  struct soap *soap_internal,
  bson_t *bson,
  const char *array_key, int array_key_length,
  int numberOfObjectsInArray,
  int *value,
  int addEmptyObjectAsEmptyJson,
  enum_caller_fn enum_caller
)
{
  // SPECIAL CASE
  int i;
  size_t index_str_key_len;
  IdxType index_str_buf;
  const char *index_str_key;
  idx2str_fn indexer;
  bson_t
    childArray,
    thisArray;

  if (!numberOfObjectsInArray)
    return SOAP_OK;

  if (!(indexer=init_indexer(numberOfObjectsInArray, index_str_buf))) {
    CWS_SET_SOAP_FAULT("emumArray(READ). Invalid index array")
    return SOAP_FAULT;
  }

  if (!bson_append_array_begin(bson, array_key, array_key_length, &childArray)) {
    CWS_SET_SOAP_FAULT("emumArray(READ). BEGIN ARRAY. Error")
    return SOAP_FAULT;
  }

  i=0;
  bson_init(&thisArray);

  do {
    if (value) {
      if (!bson_append_utf8(&thisArray, KEY_USCORE_VALUE, enum_caller(soap_internal, *value), -1)) {
        CWS_SET_SOAP_FAULT("emumArray(READ). Could not read and append emum to BSON array")
        break;
      }

bson_read_arrayOfEnum21_util_IDX:
      index_str_key_len=indexer((uint32_t)i, &index_str_key, index_str_buf);
      if (!bson_append_document(&childArray, index_str_key, (int)index_str_key_len, &thisArray)) {
        CWS_SET_SOAP_FAULT("emumArray(READ). APPEND Error")
        break;
      }

    } else if (addEmptyObjectAsEmptyJson)
      goto bson_read_arrayOfEnum21_util_IDX;
    
    if (numberOfObjectsInArray>(++i)) {
      if (((++value)!=NULL)||(addEmptyObjectAsEmptyJson!=0))
        bson_reinit(&thisArray);
    } else
      break;

  } while (1);

  if (!bson_append_array_end(bson, &childArray))
    CWS_SET_SOAP_FAULT("emumArray(READ). END Error")

  bson_destroy(&thisArray);

  CAPTURE_STAT_ON_SUCCESS_A(numberOfObjectsInArray, enum)

  CWS_RETURN(soap_internal);
}

static
int bson_read_CustomData21_util_helper(
  struct soap *soap_internal,
  bson_t *bson,
  const char *key, int key_length,
  struct ns2__CustomData *CustomData, //NOT NULL
  int addEmptyObjectAsEmptyJson
)
{
  int i;
  idx2str_fn indexer;
  size_t index_str_key_len;
  char **thisAny;
  IdxType index_str_buf;
  const char *index_str_key;
  bson_t
    childArray,
    thisArray;

  if (CustomData->__size<1) {
    if (addEmptyObjectAsEmptyJson)
      return bson_read_parse_empty_array(soap_internal, bson, key, key_length);

    return SOAP_OK;
  }

  if (!(indexer=init_indexer(CustomData->__size, index_str_buf))) {
    CWS_SET_SOAP_FAULT("CustomData(READ). Invalid index array")
    return SOAP_FAULT;
  }

  if (!bson_append_array_begin(bson, key, key_length, &childArray)) {
    CWS_SET_SOAP_FAULT("CustomData(READ). Could not append array in BSON")
    return SOAP_FAULT;
  }

  i=0;
  thisAny=CustomData->__any;
  bson_init(&thisArray);

  do {
    if (thisAny) {
      if (!bson_append_utf8(&thisArray, KEY_USCORE_VALUE, (const char *)(*thisAny)?((*thisAny)):"NULL", -1)) {
        CWS_SET_SOAP_FAULT("CustomData. Could not set _value to CustomData")
        break;
      }

bson_read_CustomData21_util_IDX:
      index_str_key_len=indexer((uint32_t)i, &index_str_key, index_str_buf);
      if (!bson_append_document(&childArray, index_str_key, (int)index_str_key_len, &thisArray)) {
        CWS_SET_SOAP_FAULT("CustomData(APPEND DOCUMENT). Error")
        break;
      }

    } else if (addEmptyObjectAsEmptyJson) {
      if (!bson_read_parse_empty_object(soap_internal, &thisArray, KEY_USCORE_VALUE))
        break;

      goto bson_read_CustomData21_util_IDX;
    }
    
    if (CustomData->__size>(++i)) {
      if (((++thisAny)!=NULL)||(addEmptyObjectAsEmptyJson!=0))
        bson_reinit(&thisArray);
    } else
      break;

  } while (1);

  if (!bson_append_array_end(bson, &childArray))
    CWS_SET_SOAP_FAULT("CustomData. Could not end array in CustomData")

  bson_destroy(&thisArray);

  CWS_RETURN(soap_internal);
}

inline static
int
bson_read_CustomData21_util(
  struct soap *soap_internal,
  bson_t *bson,
  const char *key, int key_length,
  struct ns2__CustomData *CustomData,
  int addEmptyObjectAsEmptyJson
)
{
  if (CustomData)
    return bson_read_CustomData21_util_helper(soap_internal, bson, key, key_length, CustomData, addEmptyObjectAsEmptyJson);

  return SOAP_OK;
}

//struct ns2__ObjectAlias
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, ObjectAlias)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ObjectAlias, Identifier)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ObjectAlias, IdentifierKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ObjectAlias, Description)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ObjectAlias, EffectiveDateTime)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ObjectAlias, TerminationDateTime)
  //if (bson_put_single_attribute_if_exists(thisBson, CWS_CONST_BSON_KEY("authority"), ObjectAlias->authority, -1))
  //  CWS_SET_SOAP_FAULT("ObjectAlias(READ). Could not set uid to _attribute in authority")
  READ_A_PUT_SINGLE_ATTR_21_VOID(ObjectAlias, authority)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, ObjectAlias)

//struct ns2__Citation
BSON_READ_OBJECT21_BEGIN(ns2, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, Title)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, Originator)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME(Citation, Creation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, Format)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, Editor)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Citation, LastUpdate)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, Description)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Citation, EditorHistory)
  READ_O_UTF8_OBJECT_21_VOID(Citation, DescriptiveKeywords)
BSON_READ_OBJECT21_END(Citation)

//struct ns2__OSDULineageAssertion
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, OSDULineageAssertion)

  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDULineageAssertion, ID)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_VOID(ns2, OSDULineageAssertion, LineageRelationshipKind, OSDULineageRelationshipKind)

BSON_READ_ARRAY_OF_OBJECT21_END(ns2, OSDULineageAssertion)

//struct ns2__OSDUSpatialLocationIntegration
BSON_READ_OBJECT21_BEGIN(ns2, OSDUSpatialLocationIntegration)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, SpatialLocationCoordinatesDate)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, QuantitativeAccuracyBand)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, QualitativeSpatialAccuracyType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, CoordinateQualityCheckPerformedBy)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, CoordinateQualityCheckDateTime)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUSpatialLocationIntegration, CoordinateQualityCheckRemark)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_VOID(OSDUSpatialLocationIntegration, AppliedOperation)
BSON_READ_OBJECT21_END(OSDUSpatialLocationIntegration)

//struct ns2__OSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns2, OSDUIntegration)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, LineageAssertions, OSDULineageAssertion)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, OwnerGroup)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, ViewerGroup)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, LegalTags)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, OSDUGeoJSON)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(OSDUIntegration, WGS84Latitude, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(OSDUIntegration, WGS84Longitude, PlaneAngleMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, WGS84LocationMetadata, OSDUSpatialLocationIntegration)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Field)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Country)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, State)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, County)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, City)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Region)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, District)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Block)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Prospect)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUIntegration, Play)
  READ_O_UTF8_OBJECT_21_VOID(OSDUIntegration, Basin)
BSON_READ_OBJECT21_END(OSDUIntegration)

//struct ns2__ExtensionNameValue
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, ExtensionNameValue)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Name)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Value, StringMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, ExtensionNameValue, MeasureClass, MeasureClass)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, DTim)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Index)
  READ_A_UTF8_OBJECT_21_VOID(ExtensionNameValue, Description)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, ExtensionNameValue)

//struct ns1__PassDetail
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PassDetail)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(PassDetail, Pass)
  READ_A_UTF8_OBJECT_21_VOID(PassDetail, Description)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PassDetail)

//struct ns2__AbstractInterval
BSON_READ_OBJECT21_BEGIN(ns2, AbstractInterval)
//TODO. Understand why struct ns2__AbstractInterval transient pointer. And why is it used for?
  READ_O_UTF8_OBJECT_21_VOID(AbstractInterval, Comment)
BSON_READ_OBJECT21_END_(AbstractInterval)

//struct ns2__DataObjectReference
BSON_READ_OBJECT21_BEGIN(ns2, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, Uuid)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, ObjectVersion)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, QualifiedType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, Title)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, EnergisticsUri)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, LocatorUrl)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DataObjectReference, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(DataObjectReference)

//struct ns2__DataObjectReference
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, Uuid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, ObjectVersion)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, QualifiedType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, Title)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, EnergisticsUri)
  READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectReference, LocatorUrl)
  READ_A_ARRAY_OF_OBJECT_21_VOID(DataObjectReference, ExtensionNameValue, ExtensionNameValue)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, DataObjectReference)

//struct ns2__DateTimeInterval
BSON_READ_OBJECT21_BEGIN(ns2, DateTimeInterval)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DateTimeInterval, Comment)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME(DateTimeInterval, StartTime)
  READ_O_TIME_21_VOID(DateTimeInterval, EndTime)
BSON_READ_OBJECT21_END(DateTimeInterval)

//struct ns1__LogOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, LogOSDUIntegration)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LogOSDUIntegration, LogVersion)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(LogOSDUIntegration, ZeroTime)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LogOSDUIntegration, FrameIdentifier)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(LogOSDUIntegration, IsRegular)
  READ_O_OBJECT_21_VOID(LogOSDUIntegration, IntermediaryServiceCompany, DataObjectReference)
BSON_READ_OBJECT21_END(LogOSDUIntegration)

//struct ns1__ChannelSetOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, ChannelSetOSDUIntegration)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSetOSDUIntegration, ChannelSetVersion)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSetOSDUIntegration, FrameIdentifier)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSetOSDUIntegration, IntermediaryServiceCompany, DataObjectReference)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(ChannelSetOSDUIntegration, IsRegular)
  READ_O_TIME_NULLABLE_21_VOID(ChannelSetOSDUIntegration, ZeroTime)
BSON_READ_OBJECT21_END(ChannelSetOSDUIntegration)

//struct ns1__ChannelIndex
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ChannelIndex)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, ChannelIndex, IndexKind, DataIndexKind)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelIndex, IndexPropertyKind, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelIndex, Uom)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, ChannelIndex, Direction, IndexDirection)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelIndex, Mnemonic)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelIndex, Datum, DataObjectReference)
  READ_A_OBJECT_21_VOID(ChannelIndex, IndexInterval, AbstractInterval)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ChannelIndex)

//struct ns1__ChannelOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, ChannelOSDUIntegration)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, ChannelQuality)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, IntermediaryServiceCompany, DataObjectReference)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, IsRegular)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, ZeroTime)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, ChannelBusinessValue)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, ChannelMainFamily)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelOSDUIntegration, ChannelFamily)
BSON_READ_OBJECT21_END(ChannelOSDUIntegration)

//struct ns1__ChannelData
BSON_READ_OBJECT21_BEGIN(ns1, ChannelData)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelData, Data)
  READ_O_UTF8_OBJECT_21_VOID(ChannelData, FileUri)
BSON_READ_OBJECT21_END(ChannelData)

//struct ns1__LogChannelAxis
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, LogChannelAxis)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisStart)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisSpacing)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisCount)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisPropertyKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LogChannelAxis, AxisUom)
  READ_A_PUT_SINGLE_ATTR_21_VOID(LogChannelAxis, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, LogChannelAxis)

//struct ns1__PointMetadata
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PointMetadata)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, Name)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, PointMetadata, DataKind, ChannelDataKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, Description)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, Uom)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, MetadataPropertyKind, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, AxisDefinition, LogChannelAxis)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PointMetadata, Datum, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PointMetadata)

//struct ns1__PPFGChannelOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, PPFGChannelOSDUIntegration)
  READ_O_TIME_NULLABLE_21_VOID(PPFGChannelOSDUIntegration, RecordDate)
BSON_READ_OBJECT21_END_(PPFGChannelOSDUIntegration)

//struct ns1__PPFGChannel
BSON_READ_OBJECT21_BEGIN(ns1, PPFGChannel)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, PPFGChannel, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannel, ActiveStatus, ActiveStatusKind)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Mnemonic)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, GlobalMnemonic)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannel, DataKind, ChannelDataKind)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Uom)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Source)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannel, ChannelState, ChannelState)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, ChannelPropertyKind, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, RunNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PassNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PassDescription)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PassDetail, PassDetail)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PrimaryIndexInterval, AbstractInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, LoggingCompany, DataObjectReference)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PPFGChannel, LoggingCompanyCode)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, ChannelKind, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, LoggingToolKind, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, LoggingToolClass)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannel, Derivation, ChannelDerivation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannel, LoggingMethod, LoggingMethod)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, NominalHoleSize, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, SensorOffset, LengthMeasureExt)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, MudClass)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, MudSubClass)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannel, HoleLoggingStatus, HoleLoggingStatus)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(PPFGChannel, IsContinuous)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, NominalSamplingInterval, GenericMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, ChannelOSDUIntegration, ChannelOSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Datum, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, SeismicReferenceElevation, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Data, ChannelData)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, DataSource, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, Index, ChannelIndex)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, DerivedFrom, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, DataSource, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, AxisDefinition, LogChannelAxis)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PointMetadata, PointMetadata)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGDataProcessingApplied)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGDerivation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGFamily)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGFamilyMnemonic)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGMainFamily)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGModeledLithology)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGTransformModelType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGUncertaintyClass)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannel, PPFGChannelOSDUIntegration, PPFGChannelOSDUIntegration)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(PPFGChannel)
BSON_READ_OBJECT21_END(PPFGChannel)

//struct ns1__Channel
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Channel)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, Aliases, ObjectAlias)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, Existence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, ObjectVersionReason)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, OSDUIntegration, OSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Channel, CustomData, CustomData)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Channel, ActiveStatus, ActiveStatusKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Mnemonic)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, GlobalMnemonic)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Channel, DataKind, ChannelDataKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Uom)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Source)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Channel, ChannelState, ChannelState)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, ChannelPropertyKind, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, RunNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, PassNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, PassDescription)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, PassDetail, PassDetail)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, PrimaryIndexInterval, AbstractInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, LoggingCompany, DataObjectReference)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Channel, LoggingCompanyCode)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, ChannelKind, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, LoggingToolKind, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, LoggingToolClass)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Channel, Derivation, ChannelDerivation)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Channel, LoggingMethod, LoggingMethod)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, NominalHoleSize, LengthMeasureExt)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, SensorOffset, LengthMeasureExt)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, MudClass)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, MudSubClass)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Channel, HoleLoggingStatus, HoleLoggingStatus)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Channel, IsContinuous)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, NominalSamplingInterval, GenericMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, ChannelOSDUIntegration, ChannelOSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Datum, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, SeismicReferenceElevation, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Data, ChannelData)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, DataSource, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Index, ChannelIndex)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, DerivedFrom, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, Wellbore, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, AxisDefinition, LogChannelAxis)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Channel, PointMetadata, PointMetadata)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, Channel, PPFGChannel, PPFGChannel)
  READ_A_PUT_DEFAULT_ATTRIBUTES_21_VOID(Channel)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Channel)

//struct ns1__PPFGChannelSetOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, PPFGChannelSetOSDUIntegration)
  READ_O_TIME_NULLABLE_21_VOID(PPFGChannelSetOSDUIntegration, RecordDate)
BSON_READ_OBJECT21_END_(PPFGChannelSetOSDUIntegration)

//struct ns1__PPFGChannelSet
BSON_READ_OBJECT21_BEGIN(ns1, PPFGChannelSet)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, PPFGChannelSet, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGChannelSet, ActiveStatus, ActiveStatusKind)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannelSet, ChannelState, ChannelState)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, RunNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PassNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PassDescription)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PassDetail, PassDetail)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PrimaryIndexInterval, AbstractInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, LoggingCompany, DataObjectReference)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, LoggingCompanyCode)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, LoggingToolKind, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, LoggingToolClass)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, LoggingToolClassLongName)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannelSet, Derivation, ChannelDerivation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannelSet, LoggingMethod, LoggingMethod)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, NominalHoleSize, LengthMeasureExt)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, MudClass)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, MudSubClass)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGChannelSet, HoleLoggingStatus, HoleLoggingStatus)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, NominalSamplingInterval, GenericMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, ChannelSetOSDUIntegration, ChannelSetOSDUIntegration)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, Index, ChannelIndex)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, Channel, Channel)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, Data, ChannelData)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, Wellbore, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, DataSource, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGComment)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGDerivation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGGaugeType)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGOffsetWellbore, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGTectonicSetting)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGChannelSet, PPFGChannelSetOSDUIntegration, PPFGChannelSetOSDUIntegration)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(PPFGChannelSet)
BSON_READ_OBJECT21_END(PPFGChannelSet)

//struct ns1__ChannelSet
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ChannelSet)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, Aliases, ObjectAlias)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, Existence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, ObjectVersionReason)
  READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, BusinessActivityHistory)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, OSDUIntegration, OSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, ChannelSet, CustomData, CustomData)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, ChannelSet, ActiveStatus, ActiveStatusKind)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ChannelSet, ChannelState, ChannelState)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, RunNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, PassNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, PassDescription)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, PassDetail, PassDetail)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, PrimaryIndexInterval, AbstractInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, LoggingCompany, DataObjectReference)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(ChannelSet, LoggingCompanyCode)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, LoggingToolKind, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, LoggingToolClass)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, LoggingToolClassLongName)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ChannelSet, Derivation, ChannelDerivation)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ChannelSet, LoggingMethod, LoggingMethod)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, NominalHoleSize, LengthMeasureExt)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, MudClass)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, MudSubClass)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ChannelSet, HoleLoggingStatus, HoleLoggingStatus)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, NominalSamplingInterval, GenericMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, ChannelSetOSDUIntegration, ChannelSetOSDUIntegration)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, Index, ChannelIndex)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, Channel, Channel)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, Data, ChannelData)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, Wellbore, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ChannelSet, DataSource, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, ChannelSet, PPFGChannelSet, PPFGChannelSet)
  READ_A_PUT_DEFAULT_ATTRIBUTES_21_VOID(ChannelSet)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ChannelSet)

//struct ns1__PPFGLogOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, PPFGLogOSDUIntegration)
  READ_O_TIME_NULLABLE_21_VOID(PPFGLogOSDUIntegration, RecordDate)
BSON_READ_OBJECT21_END_(PPFGLogOSDUIntegration)

//struct ns1__PPFGLog
BSON_READ_OBJECT21_BEGIN(ns1, PPFGLog)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, PPFGLog, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, PPFGLog, ActiveStatus, ActiveStatusKind)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGLog, ChannelState, ChannelState)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, RunNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PassNumber)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PassDescription)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PassDetail, PassDetail)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PrimaryIndexInterval, AbstractInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingCompany, DataObjectReference)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingCompanyCode)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingToolKind, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingToolClass)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingToolClassLongName)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LoggingServicePeriod, DateTimeInterval)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGLog, Derivation, ChannelDerivation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGLog, LoggingMethod, LoggingMethod)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, NominalHoleSize, LengthMeasureExt)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, MudClass)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, MudSubClass)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PPFGLog, HoleLoggingStatus, HoleLoggingStatus)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, NominalSamplingInterval, GenericMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, LogOSDUIntegration, LogOSDUIntegration)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, ChannelSet, ChannelSet)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, Wellbore, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGComment)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGDerivation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGGaugeType)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGOffsetWellbore, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGTectonicSetting)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PPFGLog, PPFGLogOSDUIntegration, PPFGLogOSDUIntegration)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(PPFGLog)
BSON_READ_OBJECT21_END(PPFGLog)

//struct ns2__MeasuredDepth
BSON_READ_OBJECT21_BEGIN(ns2, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MeasuredDepth, MeasuredDepth, LengthMeasureExt)
  READ_O_OBJECT_21_VOID(MeasuredDepth, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(MeasuredDepth)

//struct ns1__DrillingParams
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillingParams)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, ETimOpBit, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, MdHoleStart, MeasuredDepth)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, MdHoleStop, MeasuredDepth)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, HkldRot, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, OverPull, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, SlackOff, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, HkldUp, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, HkldDn, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TqOnBotAv, MomentOfForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TqOnBotMx, MomentOfForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TqOnBotMn, MomentOfForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TqOffBotAv, MomentOfForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TqDhAv, MomentOfForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WtAboveJar, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WtBelowJar, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WtMud, MassPerVolumeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, FlowratePumpAv, VolumePerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, FlowratePumpMx, VolumePerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, FlowratePumpMn, VolumePerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, VelNozzleAv, LengthPerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, PowBit, PowerMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, PresDropBit, PressureMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimHold, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimSteering, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimDrillRot, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimDrillSlid, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimCirc, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, CTimReam, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, DistDrillRot, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, DistDrillSlid, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, DistReam, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, DistHold, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, DistSteering, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RpmAv, AngularVelocityMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RpmMx, AngularVelocityMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RpmMn, AngularVelocityMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RpmAvDh, AngularVelocityMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RopAv, LengthPerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RopMx, LengthPerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, RopMn, LengthPerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WobAv, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WobMx, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WobMn, ForceMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, WobAvDh, ForceMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, ReasonTrip)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, ObjectiveBha)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, AziTop, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, AziBottom, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, InclStart, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, InclMx, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, InclMn, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(DrillingParams, InclStop, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, TempMudDhMx, ThermodynamicTemperatureMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, PresPumpAv, PressureMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, FlowrateBit, VolumePerTimeMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillingParams, MudClass, MudClass)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillingParams, MudSubClass, MudSubClass)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParams, Tubular, DataObjectReference)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillingParams, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillingParams)

//struct ns2__DatumVerticalDepth
BSON_READ_OBJECT21_BEGIN(ns2, DatumVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DatumVerticalDepth, VerticalDepth, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DatumVerticalDepth, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_VOID(DatumVerticalDepth, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(DatumVerticalDepth)

//struct ns2__ReferencePointVerticalDepth
BSON_READ_OBJECT21_BEGIN(ns2, ReferencePointVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferencePointVerticalDepth, VerticalDepth, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferencePointVerticalDepth, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_VOID(ReferencePointVerticalDepth, ReferencePoint, DataObjectReference)
BSON_READ_OBJECT21_END(ReferencePointVerticalDepth)

//struct ns2__AbstractVerticalDepth
BSON_READ_OBJECT21_BEGIN(ns2, AbstractVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractVerticalDepth, VerticalDepth, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractVerticalDepth, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractVerticalDepth, DatumVerticalDepth, DatumVerticalDepth)
  READ_O_OBJECT_21_VOID_B(ns2, AbstractVerticalDepth, ReferencePointVerticalDepth, ReferencePointVerticalDepth)
BSON_READ_OBJECT21_END(AbstractVerticalDepth)

//struct ns2__ExtensionNameValue
BSON_READ_OBJECT21_BEGIN(ns2, ExtensionNameValue)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Name)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Value, StringMeasure)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, ExtensionNameValue, MeasureClass, MeasureClass)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, DTim)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(ExtensionNameValue, Index)
  READ_O_UTF8_OBJECT_21_VOID(ExtensionNameValue, Description)
BSON_READ_OBJECT21_END(ExtensionNameValue)

//struct ns2__MdInterval
BSON_READ_OBJECT21_BEGIN(ns2, MdInterval)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MdInterval, Comment)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(MdInterval, MdMin)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(MdInterval, MdMax)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MdInterval, Uom)
  READ_O_OBJECT_21_VOID(MdInterval, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(MdInterval)

//struct ns2__MdInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, MdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MdInterval, Comment)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(MdInterval, MdMin)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(MdInterval, MdMax)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MdInterval, Uom)
  READ_A_OBJECT_21_VOID(MdInterval, Datum, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, MdInterval)

//struct ns2__DatumTvdInterval
BSON_READ_OBJECT21_BEGIN(ns2, DatumTvdInterval)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DatumTvdInterval, Comment)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(DatumTvdInterval, TvdMin)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(DatumTvdInterval, TvdMax)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DatumTvdInterval, Uom)
  READ_O_OBJECT_21_VOID(DatumTvdInterval, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_VOID(DatumTvdInterval, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(DatumTvdInterval)

//struct ns2__ReferencePointTvdInterval
BSON_READ_OBJECT21_BEGIN(ns2, ReferencePointTvdInterval)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferencePointTvdInterval, Comment)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(ReferencePointTvdInterval, TvdMin)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(ReferencePointTvdInterval, TvdMax)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferencePointTvdInterval, Uom)
  READ_O_OBJECT_21_VOID(ReferencePointTvdInterval, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_VOID(ReferencePointTvdInterval, ReferencePoint, DataObjectReference)
BSON_READ_OBJECT21_END(ReferencePointTvdInterval)

//struct ns2__AbstractTvdInterval
BSON_READ_OBJECT21_BEGIN(ns2, AbstractTvdInterval)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Comment)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, TvdMin)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, TvdMax)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Uom)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Trajectory, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractTvdInterval, DatumTvdInterval, DatumTvdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractTvdInterval, ReferencePointTvdInterval, ReferencePointTvdInterval)
BSON_READ_OBJECT21_END(AbstractTvdInterval)

//struct ns2__AbstractTvdInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, AbstractTvdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Comment)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, TvdMin)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, TvdMax)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Uom)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractTvdInterval, Trajectory, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractTvdInterval, DatumTvdInterval, DatumTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractTvdInterval, ReferencePointTvdInterval, ReferencePointTvdInterval)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, AbstractTvdInterval)

//struct ns1__WellboreGeometrySection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, WellboreGeometrySection)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeometrySection, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeometrySection, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeometrySection, ExtensionNameValue, ExtensionNameValue)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, uid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, objectVersion)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeometrySection, MdInterval, MdInterval)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, WellboreGeometrySection, TypeHoleCasing, HoleCasingType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, SectionTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, IdSection, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, OdSection, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, WtPerLen, MassPerLengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, Grade)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, CurveConductor)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, DiaDrift, LengthMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellboreGeometrySection, FactFric)
  READ_A_ARRAY_OF_OBJECT_21_VOID(WellboreGeometrySection, BhaRun, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, WellboreGeometrySection)

//struct ns1__WellboreGeometryReport
BSON_READ_OBJECT21_BEGIN(ns1, WellboreGeometryReport)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometryReport, WellboreGeometrySection, WellboreGeometrySection)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeometryReport, DepthWaterMean, LengthMeasureExt)
  READ_O_OBJECT_21_VOID(WellboreGeometryReport, GapAir, LengthMeasureExt)
BSON_READ_OBJECT21_END(WellboreGeometryReport)

//struct ns2__ComponentReference
BSON_READ_OBJECT21_BEGIN(ns2, ComponentReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, QualifiedType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, Uid)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, Name)
  READ_O_LONG64_NULLABLE_21_VOID(ComponentReference, Index)
BSON_READ_OBJECT21_END(ComponentReference)

//struct ns2__ComponentReference
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, ComponentReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, QualifiedType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, Uid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ComponentReference, Name)
  READ_A_LONG64_NULLABLE_21_VOID(ComponentReference, Index)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, ComponentReference)

//struct ns1__CementPumpScheduleStep
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CementPumpScheduleStep)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, Fluid, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, RatioFluidExcess, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, ETimPump, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, RatePump, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, VolPump, VolumeMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, StrokePump)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, PresBack, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, ETimShutdown, TimeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementPumpScheduleStep, Comments)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CementPumpScheduleStep, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CementPumpScheduleStep)

//struct ns1__FluidLocation
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, FluidLocation)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidLocation, Fluid, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidLocation, MDFluidBase, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidLocation, MDFluidTop, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidLocation, Volume, VolumeMeasure)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, FluidLocation, LocationType, WellboreFluidLocation)
  READ_A_PUT_SINGLE_ATTR_21_VOID(FluidLocation, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, FluidLocation)

//struct ns1__CementStageReport
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CementStageReport)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, AnnularFlowAfter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, ReciprocationSlackoff, ForceMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, BotPlug)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, BotPlugNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, DiaTailPipe, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, DisplacementFluid, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, ETimPresHeld, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateMudCirc, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, Gel10Min, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, Gel10Sec, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, MdCircOut, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, MdCoilTbg, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, MdString, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, MdTool, MeasuredDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, MixMethod)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(CementStageReport, NumStage)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, ReciprocationOverpull, ForceMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, PillBelowPlug)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, PlugCatcher)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresBackPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresBump, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresCoilTbgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresCoilTbgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresCsgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresCsgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresDisplace, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresHeld, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresMudCirc, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresTbgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresTbgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PvMud, DynamicViscosityMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, SqueezeObjective)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, StageMdInterval, MdInterval)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, TailPipePerf)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, TailPipeUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, TempBHCT, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, TempBHST, ThermodynamicTemperatureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, TopPlug)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, TypeOriginalMud)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, TypeStage)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolCircPrior, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolCsgIn, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolCsgOut, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolDisplaceFluid, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolExcess, VolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolExcessMethod)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolMudLost, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VolReturns, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, WtMud, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, YpMud, PressureMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, Step, CementPumpScheduleStep)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, OriginalFluidLocation, FluidLocation)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, EndingFluidLocation, FluidLocation)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, DTimMixStart)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, DTimPumpStart)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, DTimPumpEnd)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, DTimDisplaceStart)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresBreakDown, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateBreakDown, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateDisplaceAv, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateDisplaceMx, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresSqueezeAv, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresSqueezeEnd, PressureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresSqueezeHeld)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, ETimMudCirculation, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresSqueeze, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateSqueezeAv, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateSqueezeMx, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowrateEnd, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowratePumpStart, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, FlowratePumpEnd, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, VisFunnelMud, TimeMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, PlugBumped)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, SqueezeObtained)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageReport, PresPriorBump, PressureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageReport, FloatHeld)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CementStageReport, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CementStageReport)

//struct ns1__CementJobReport
BSON_READ_OBJECT21_BEGIN(ns1, CementJobReport)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, CementEngr)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, ETimWaitingOnCement, TimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, PlugInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, MdHole, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, Contractor, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, RpmPipe, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, TqInitPipeRot, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, TqPipeAv, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, TqPipeMx, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, OverPull, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, SlackOff, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, RpmPipeRecip, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, LenPipeRecipStroke, LengthMeasure)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, Reciprocating)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimJobEnd)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimJobStart)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimPlugSet)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, CementDrillOut)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimCementDrillOut)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimSqueeze)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimPipeRotStart)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimPipeRotEnd)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimRecipStart)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobReport, DTimRecipEnd)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobReport, DensMeasBy)
  READ_O_ARRAY_OF_OBJECT_21_VOID(CementJobReport, CementReportStage, CementStageReport)
BSON_READ_OBJECT21_END(CementJobReport)

//struct ns1__CementStageDesign
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CementStageDesign)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, AnnularFlowAfter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, ReciprocationSlackoff, ForceMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, BotPlug)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, BotPlugNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, DiaTailPipe, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, DisplacementFluid, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, ETimPresHeld, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, FlowrateMudCirc, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, Gel10Min, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, Gel10Sec, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, MdCircOut, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, MdCoilTbg, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, MdString, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, MdTool, MeasuredDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, MixMethod)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(CementStageDesign, NumStage)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PillBelowPlug)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PlugCatcher)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresBackPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresBump, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresCoilTbgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresCoilTbgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresCsgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresCsgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresDisplace, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresHeld, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresMudCirc, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresTbgEnd, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PresTbgStart, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, PvMud, DynamicViscosityMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, SqueezeObjective)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, StageMdInterval, MdInterval)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TailPipePerf)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TailPipeUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TempBHCT, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TempBHST, ThermodynamicTemperatureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TopPlug)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TypeOriginalMud)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, TypeStage)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolCircPrior, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolCsgIn, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolCsgOut, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolDisplaceFluid, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolExcess, VolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolExcessMethod)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolMudLost, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, VolReturns, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, WtMud, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, YpMud, PressureMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, Step, CementPumpScheduleStep)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementStageDesign, OriginalFluidLocation, FluidLocation)
  READ_A_ARRAY_OF_OBJECT_21_VOID(CementStageDesign, EndingFluidLocation, FluidLocation)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CementStageDesign)

//struct ns1__CementJobDesign
BSON_READ_OBJECT21_BEGIN(ns1, CementJobDesign)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, CementEngr)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, ETimWaitingOnCement, TimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, PlugInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, MdHole, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, Contractor, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, RpmPipe, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, TqInitPipeRot, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, TqPipeAv, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, TqPipeMx, MomentOfForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, OverPull, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, SlackOff, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, RpmPipeRecip, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJobDesign, LenPipeRecipStroke, LengthMeasure)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJobDesign, Reciprocating)
  READ_O_ARRAY_OF_OBJECT_21_VOID(CementJobDesign, CementDesignStage, CementStageDesign)
BSON_READ_OBJECT21_END(CementJobDesign)

//struct ns1__CementAdditive
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CementAdditive)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, NameAdd)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, TypeAdd)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, FormAdd)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, DensAdd, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, Additive, MassMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementAdditive, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CementAdditive, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CementAdditive)

//struct ns1__RheometerViscosity
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, RheometerViscosity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(RheometerViscosity, Speed, AngularVelocityMeasure)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(RheometerViscosity, Viscosity)
  READ_A_PUT_SINGLE_ATTR_21_VOID(RheometerViscosity, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, RheometerViscosity)

//struct ns1__Rheometer
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Rheometer)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Rheometer, TempRheom, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Rheometer, PresRheom, PressureMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Rheometer, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Rheometer, Viscosity, RheometerViscosity)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Rheometer, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Rheometer)

//struct ns1__CementingFluid
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CementingFluid)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ETimTransitions, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ETimZeroGel, TimeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TypeFluid)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementingFluid, FluidIndex)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DescFluid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Purpose)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ClassSlurryDryBlend)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, SlurryPlacementInterval, MdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, SourceWater)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolWater, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolCement, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, RatioMixWater, VolumePerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolFluid, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ExcessPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolYield, VolumePerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Density, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, SolidVolumeFraction, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolPumped, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolOther, VolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, FluidRheologicalModel)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Viscosity, DynamicViscosityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Yp, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, N, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, K, DimensionlessMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(CementingFluid, Gel10SecReading, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Gel10SecStrength, PressureMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(CementingFluid, Gel10MinReading, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, Gel10MinStrength, PressureMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TypeBaseFluid)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DensBaseFluid, MassPerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DryBlendName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DryBlendDescription)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, MassDryBlend, MassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DensDryBlend, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, MassSackDryBlend, MassMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementingFluid, FoamUsed)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TypeGasFoam)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolGasFoam, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, RatioConstGasMethodAv, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DensConstGasMethod, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, RatioConstGasMethodStart, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, RatioConstGasMethodEnd, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DensConstGasFoam, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ETimThickening, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TempThickening, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, PresTestThickening, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ConsTestThickening, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, PcFreeWater, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TempFreeWater, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolTestFluidLoss, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TempFluidLoss, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, PresTestFluidLoss, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TimeFluidLoss, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolAPIFluidLoss, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ETimComprStren1, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, ETimComprStren2, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, PresComprStren1, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, PresComprStren2, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TempComprStren1, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, TempComprStren2, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, DensAtPres, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolReserved, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, VolTotSlurry, VolumeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementingFluid, CementAdditive, CementAdditive)
  READ_A_ARRAY_OF_OBJECT_21_VOID(CementingFluid, Rheometer, Rheometer)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CementingFluid, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CementingFluid)

//struct ns1__EventRefInfo
BSON_READ_OBJECT21_BEGIN(ns1, EventRefInfo)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EventRefInfo, Event, DataObjectReference)
  READ_O_TIME_NULLABLE_21_VOID(EventRefInfo, EventDate)
BSON_READ_OBJECT21_END(EventRefInfo)

//struct ns1__EventInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, EventInfo)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(EventInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EventInfo, BeginEvent, EventRefInfo)
  READ_A_OBJECT_21_VOID(EventInfo, EndEvent, EventRefInfo)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, EventInfo)

//struct ns1__EventInfo
BSON_READ_OBJECT21_BEGIN(ns1, EventInfo)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(EventInfo, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EventInfo, BeginEvent, EventRefInfo)
  READ_O_OBJECT_21_VOID(EventInfo, EndEvent, EventRefInfo)
BSON_READ_OBJECT21_END(EventInfo)

//struct ns1__ObjectSequence
BSON_READ_OBJECT21_BEGIN(ns1, ObjectSequence)
  READ_O_PUT_SINGLE_ATTR_21_VOID(ObjectSequence, description)
BSON_READ_OBJECT21_END_(ObjectSequence)

//struct ns1__CasingConnectionType
BSON_READ_OBJECT21_BEGIN(ns1, CasingConnectionType)
  READ_O_OBJECT_ENUM_21_VOID(ns1, CasingConnectionType, CasingConnectionType, CasingConnectionTypes)
BSON_READ_OBJECT21_END_(CasingConnectionType)

//struct ns1__OtherConnectionType
BSON_READ_OBJECT21_BEGIN(ns1, OtherConnectionType)
  READ_O_OBJECT_ENUM_21_VOID(ns1, OtherConnectionType, OtherConnectionType, OtherConnectionTypes)
BSON_READ_OBJECT21_END_(OtherConnectionType)

//struct ns1__RodConnectionType
BSON_READ_OBJECT21_BEGIN(ns1, RodConnectionType)
  READ_O_OBJECT_ENUM_21_VOID(ns1, RodConnectionType, RodConnectionType, RodConnectionTypes)
BSON_READ_OBJECT21_END_(RodConnectionType)

//struct ns1__TubingConnectionType
BSON_READ_OBJECT21_BEGIN(ns1, TubingConnectionType)
  READ_O_OBJECT_ENUM_21_VOID(ns1, TubingConnectionType, TubingConnectionType, TubingConnectionTypes)
BSON_READ_OBJECT21_END_(TubingConnectionType)

//struct ns1__AbstractConnectionType
BSON_READ_OBJECT21_BEGIN(ns1, AbstractConnectionType)
//TODO understand what is Transient pointer to a derived type value ...
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractConnectionType, CasingConnectionType, CasingConnectionType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractConnectionType, OtherConnectionType, OtherConnectionType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractConnectionType, RodConnectionType, RodConnectionType)
  READ_O_OBJECT_21_VOID_B(ns1, AbstractConnectionType, TubingConnectionType, TubingConnectionType)
BSON_READ_OBJECT21_END(AbstractConnectionType)

//struct ns1__ReferenceContainer
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ReferenceContainer)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferenceContainer, String, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferenceContainer, Equipment, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferenceContainer, AccesoryEquipment, ComponentReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferenceContainer, Comment)
  READ_A_PUT_SINGLE_ATTR_21_VOID(ReferenceContainer, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ReferenceContainer)

//struct ns1__EquipmentConnection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, EquipmentConnection)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Id, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Od, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Len, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TypeThread)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, SizeThread, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TensYield, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TqYield, MomentOfForceMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, EquipmentConnection, Position, ConnectionPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, CriticalCrossSection, AreaMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, PresLeak, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TqMakeup, MomentOfForceMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Equipment, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, RadialOffset, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, EquipmentConnection, ConnectionForm, ConnectionFormType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ConnectionUpset)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ConnectionType, AbstractConnectionType)
  READ_A_PUT_SINGLE_ATTR_21_VOID(EquipmentConnection, uid)//NOTICE: Order of uid is at the end differs from this struct *
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, EquipmentConnection)

//struct ns1__EquipmentConnection
BSON_READ_OBJECT21_BEGIN(ns1, EquipmentConnection)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Id, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Od, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Len, LengthMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TypeThread)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, SizeThread, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TensYield, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TqYield, MomentOfForceMeasure)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, EquipmentConnection, Position, ConnectionPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, CriticalCrossSection, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, PresLeak, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, TqMakeup, MomentOfForceMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, Equipment, ComponentReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, RadialOffset, LengthMeasure)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, EquipmentConnection, ConnectionForm, ConnectionFormType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ConnectionUpset)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EquipmentConnection, ConnectionType, AbstractConnectionType)
  READ_O_PUT_SINGLE_ATTR_21_VOID(EquipmentConnection, uid)
BSON_READ_OBJECT21_END(EquipmentConnection)

//NOTICE. Must be declare because recursive struct ns1__Assembly
static
int bson_read_arrayOfStringEquipment21_util(
  struct soap *,
  bson_t *,
  const char *, int,
  const char *, int,
  int,
  struct ns1__StringEquipment *,
  int
);

//struct ns1__Assembly
BSON_READ_OBJECT21_BEGIN(ns1, Assembly)
  READ_O_ARRAY_OF_OBJECT_21_VOID(Assembly, Part, StringEquipment)
BSON_READ_OBJECT21_END_(Assembly)

//struct ns1__StringEquipment
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StringEquipment)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, Equipment, ComponentReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, EquipmentType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, Name)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, EquipmentEventHistory, EventInfo)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, Status)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, RunNo)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PreviousRunDays, TimeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, ObjectCondition)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, SurfaceCondition)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StringEquipment, Count)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, Length, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, TvdInterval, AbstractTvdInterval)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StringEquipment, OutsideString)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, TensileMax, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PresRating, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PresCollapse, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PresBurst, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, HeatRating, ThermodynamicTemperatureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StringEquipment, IsLinetoSurface)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StringEquipment, IsCentralized)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StringEquipment, HasScratchers)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PerforationSet, ComponentReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, PermanentRemarks)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, UsageComment)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, OrderOfObject, ObjectSequence)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, InsideComponent, ReferenceContainer)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, OutsideComponent, ReferenceContainer)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, ConnectionNext, EquipmentConnection)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StringEquipment, Assembly, Assembly) //TODO Recursive. Add limit root to avoid client abuse
  READ_A_PUT_SINGLE_ATTR_21_VOID(StringEquipment, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StringEquipment)

//struct ns1__StringEquipmentSet
BSON_READ_OBJECT21_BEGIN(ns1, StringEquipmentSet)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StringEquipmentSet, StringEquipment, StringEquipment)
BSON_READ_OBJECT21_END_(StringEquipmentSet)

//struct ns1__StringAccessory
BSON_READ_OBJECT21_BEGIN(ns1, StringAccessory)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StringAccessory, Accessory, StringEquipment)
BSON_READ_OBJECT21_END_(StringAccessory)

//struct ns1__DownholeString
BSON_READ_OBJECT21_BEGIN(ns1, DownholeString)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DownholeString, StringType, DownholeStringType)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DownholeString, SubStringType, SubStringType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, Name)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DownholeString, StringInstallDate)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, StringMdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, AxisOffset, LengthMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ParentString, ComponentReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, StringEquipmentSet, StringEquipmentSet)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, Accessories, StringAccessory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ReferenceWellbore, DataObjectReference)
  READ_O_PUT_SINGLE_ATTR_21_VOID(DownholeString, uid)
BSON_READ_OBJECT21_END(DownholeString)

//struct ns1__Borehole
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Borehole)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, Name)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Borehole, TypeBorehole, BoreholeType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, TvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, BoreholeDiameter, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, DescriptionPermanent)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Borehole, EquipmentEventHistory, EventInfo)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Borehole, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Borehole)

//struct ns1__GeologyFeature
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, GeologyFeature)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GeologyFeature, Name)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, GeologyFeature, GeologyType, GeologyType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GeologyFeature, FeatureMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GeologyFeature, FeatureTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GeologyFeature, GeologicUnitInterpretation, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GeologyFeature, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(GeologyFeature, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, GeologyFeature)

//struct ns1__BoreholeString
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, BoreholeString)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeString, Name)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeString, Borehole, Borehole)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeString, GeologyFeature, GeologyFeature)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeString, Accessories, StringAccessory)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeString, ReferenceWellbore, DataObjectReference)
  READ_A_PUT_SINGLE_ATTR_21_VOID(BoreholeString, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, BoreholeString)

//struct ns1__BoreholeStringSet
BSON_READ_OBJECT21_BEGIN(ns1, BoreholeStringSet)
  READ_O_ARRAY_OF_OBJECT_21_VOID(BoreholeStringSet, BoreholeString, BoreholeString)
BSON_READ_OBJECT21_END_(BoreholeStringSet)

//struct ns1__DownholeString
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DownholeString)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DownholeString, StringType, DownholeStringType)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DownholeString, SubStringType, SubStringType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, Name)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DownholeString, StringInstallDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, StringMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, AxisOffset, LengthMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ParentString, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, StringEquipmentSet, StringEquipmentSet)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, Accessories, StringAccessory)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeString, ReferenceWellbore, DataObjectReference)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DownholeString, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DownholeString)

//struct ns1__DownholeStringSet
BSON_READ_OBJECT21_BEGIN(ns1, DownholeStringSet)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DownholeStringSet, DownholeString, DownholeString)
BSON_READ_OBJECT21_END_(DownholeStringSet)

//struct ns1__ExtPropNameValue
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ExtPropNameValue)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtPropNameValue, Name)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ExtPropNameValue, Name)
  READ_A_PUT_SINGLE_ATTR_21_VOID(ExtPropNameValue, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ExtPropNameValue)

//struct ns1__PerfSlot
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PerfSlot)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfSlot, SlotHeight, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfSlot, SlotWidth, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfSlot, SlotCenterDistance, LengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerfSlot, SlotCount)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfSlot, Remarks)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfSlot, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PerfSlot, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PerfSlot)

//struct ns1__PerfHole
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PerfHole)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, TvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, HoleDiameter, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(PerfHole, HoleAngle, PlaneAngleMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, HolePattern)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, Remarks)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerfHole, HoleDensity, ReciprocalLengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerfHole, HoleCount)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PerfHole, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PerfHole)

//struct ns1__Equipment
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Equipment)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, EquipmentName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, EquipmentType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Manufacturer, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Model)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, CatalogId)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, CatalogName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, BrandName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, ModelType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Series)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Equipment, IsSerialized)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, SerialNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, PartNo)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, SurfaceCondition)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Material)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Equipment, Grade, GradeType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, UnitWeight, MassPerLengthMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Equipment, CoatingLinerApplied)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Equipment, OutsideCoating, Coating)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Equipment, InsideCoating, Coating)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, UnitLength, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MajorOd, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MinorOd, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Od, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MaxOd, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MinOd, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MajorId, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MinorId, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Id, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MaxId, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, MinId, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Drift, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, NominalSize, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, NameService)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Description)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, DescriptionPermanent)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Remark)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, Property, ExtPropNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, SlotAsManufactured, PerfSlot)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Equipment, HoleAsManufactured, PerfHole)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Equipment, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Equipment)

//struct ns1__EquipmentSet
BSON_READ_OBJECT21_BEGIN(ns1, EquipmentSet)
  READ_O_ARRAY_OF_OBJECT_21_VOID(EquipmentSet, Equipment, Equipment)
BSON_READ_OBJECT21_END_(EquipmentSet)

//struct ns1__PerforationSet
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PerforationSet)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, BoreholeString, ComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, DownholeString, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, TvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, HoleDiameter, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(PerforationSet, HoleAngle, PlaneAngleMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, HolePattern)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, HoleDensity, ReciprocalLengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationSet, HoleCount)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationSet, FrictionFactor)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, FrictionPres, PressureMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationSet, DischargeCoefficient)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PerforationSet, PerforationTool, PerforationToolType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, PerforationPenetration, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, CrushZoneDiameter, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, CrushDamageRatio)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationSet, PerforationDate)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, PermanentRemarks)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSet, EventHistory, EventInfo)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PerforationSet, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PerforationSet)

//struct ns1__PerforationSets
BSON_READ_OBJECT21_BEGIN(ns1, PerforationSets)
  READ_O_ARRAY_OF_OBJECT_21_VOID(PerforationSets, PerforationSet, PerforationSet)
BSON_READ_OBJECT21_END_(PerforationSets)

//struct ns1__BitRecord
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, BitRecord)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, NumBit)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaBit, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaPassThru, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaPilot, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Manufacturer, DataObjectReference)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, TypeBit, BitType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CodeMfg)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CodeIADC)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitInner, IadcIntegerCode)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitOuter, IadcIntegerCode)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitDull, BitDullCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitLocation)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitBearing, IadcBearingWearCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitGauge)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitOther)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitReason, BitReasonPulled)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalInner, IadcIntegerCode)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalOuter, IadcIntegerCode)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalDull, BitDullCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalLocation)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalBearing, IadcBearingWearCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalGauge)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalOther)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalReason, BitReasonPulled)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Drive)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, BitClass)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Cost, Cost)
  READ_A_PUT_SINGLE_ATTR_21_VOID(BitRecord, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, BitRecord)

//struct ns1__BitRecord
BSON_READ_OBJECT21_BEGIN(ns1, BitRecord)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, NumBit)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaBit, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaPassThru, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, DiaPilot, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Manufacturer, DataObjectReference)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, TypeBit, BitType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CodeMfg)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CodeIADC)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitInner, IadcIntegerCode)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitOuter, IadcIntegerCode)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitDull, BitDullCode)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitLocation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitBearing, IadcBearingWearCode)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitGauge)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondInitOther)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondInitReason, BitReasonPulled)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalInner, IadcIntegerCode)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalOuter, IadcIntegerCode)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalDull, BitDullCode)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalLocation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalBearing, IadcBearingWearCode)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalGauge)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, CondFinalOther)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BitRecord, CondFinalReason, BitReasonPulled)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Drive)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, BitClass)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BitRecord, Cost, Cost)
  READ_O_PUT_SINGLE_ATTR_21_VOID(BitRecord, uid)
BSON_READ_OBJECT21_END(BitRecord)

//struct ns1__DrillReportWellboreInfo
BSON_READ_OBJECT21_BEGIN(ns1, DrillReportWellboreInfo)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportWellboreInfo, DTimSpud)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportWellboreInfo, DTimPreSpud)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellboreInfo, DateDrillComplete)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellboreInfo, Operator, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellboreInfo, DrillContractor, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DrillReportWellboreInfo, Rig, DataObjectReference)
BSON_READ_OBJECT21_END(DrillReportWellboreInfo)

//struct ns2__DatumElevation
BSON_READ_OBJECT21_BEGIN(ns2, DatumElevation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DatumElevation, Elevation, LengthMeasureExt)
  READ_O_OBJECT_21_VOID(DatumElevation, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(DatumElevation)

//struct ns2__ReferencePointElevation
BSON_READ_OBJECT21_BEGIN(ns2, ReferencePointElevation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ReferencePointElevation, Elevation, LengthMeasureExt)
  READ_O_OBJECT_21_VOID(ReferencePointElevation, ReferencePoint, DataObjectReference)
BSON_READ_OBJECT21_END(ReferencePointElevation)

//struct ns2__AbstractElevation
BSON_READ_OBJECT21_BEGIN(ns2, AbstractElevation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractElevation, Elevation, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractElevation, DatumElevation, DatumElevation)
  READ_O_OBJECT_21_VOID_B(ns2, AbstractElevation, ReferencePointElevation, ReferencePointElevation)
BSON_READ_OBJECT21_END(AbstractElevation)

//struct ns1__DrillReportStatusInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportStatusInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Tvd, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdPlugTop, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DiaHole, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdDiaHoleStart, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DiaPilot, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdDiaPilotPlan, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, TvdDiaPilotPlan, AbstractVerticalDepth)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportStatusInfo, TypeWellbore, WellboreType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdKickoff, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, TvdKickoff, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, StrengthForm, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdStrengthForm, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, TvdStrengthForm, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DiaCsgLast, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdCsgLast, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, TvdCsgLast, AbstractVerticalDepth)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportStatusInfo, PresTestType, PresTestType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, MdPlanned, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistDrill, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Sum24Hr)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Forecast24Hr)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, RopCurrent, LengthPerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, RigUtilization, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimStart, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimSpud, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimLoc, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimDrill, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, RopAv, LengthPerTimeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Supervisor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Engineer)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Geologist)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimDrillRot, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimDrillSlid, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimCirc, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimReam, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimHold, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ETimSteering, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistDrillRot, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistDrillSlid, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistReam, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistHold, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, DistSteering, LengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, NumPob)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, NumContract)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, NumOperator)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, NumService)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, NumAFE)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ConditionHole)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, TvdLot, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, PresLotEmw, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, PresKickTol, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, VolKickTol, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Maasp, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, Tubular, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ParentWellbore, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, ElevKelly, AbstractElevation)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, CostDay, Cost)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStatusInfo, CostDayMud, Cost)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportStatusInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportStatusInfo)

//struct ns1__Fluid
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Fluid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Type)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, LocationSample)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Fluid, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Tvd, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Ecd, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, KickToleranceVolume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, KickToleranceIntensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, TempFlowLine, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PresBopRating, PressureMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Fluid, MudClass, MudClass)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Density, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, VisFunnel, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, TempVis, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Pv, DynamicViscosityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Yp, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Gel3Sec, PressureMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Gel6Sec, PressureMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Gel10Sec, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Gel10Min, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Gel30Min, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, FilterCakeLtlp, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, FiltrateLtlp, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, TempHthp, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PresHthp, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, FiltrateHthp, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, FilterCakeHthp, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, WaterPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, OilPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SandPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsLowGravPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsLowGrav, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsCalcPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, BaritePc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Lcm, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Mbt, CationExchangeCapacityMeasureExt)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Fluid, Ph)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, TempPh, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Pm, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PmFiltrate, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Mf, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, AlkalinityP1, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, AlkalinityP2, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Chloride, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Calcium, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Magnesium, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Potassium, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, BrinePc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, BrineDensity, MassPerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, BrineClass)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Lime, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, ElectStab, ElectricPotentialDifferenceMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, CalciumChloridePc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, CalciumChloride, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Company, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Engineer)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Asg, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsHiGravPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolidsHiGrav, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Polymer, VolumePerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PolyType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SolCorPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, OilCtg, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, OilCtgDry, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, HardnessCa, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Sulfide, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, AverageCuttingSize, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Carbonate, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Iron, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, MetalRecovered, MassMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Turbidity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, OilGrease, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Salt, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SaltPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Tct, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, WaterPhaseSalinity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, WholeMudCalcium, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, WholeMudChloride, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, ZincOxide, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SodiumChloride, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, SodiumChloridePc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PptSpurtLoss, VolumeMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PptPressure, PressureMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, PptTemperature, ThermodynamicTemperatureMeasureExt)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Fluid, Rheometer, Rheometer)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Fluid, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Fluid)

//struct ns1__DrillReportPorePressure
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportPorePressure)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportPorePressure, ReadingKind, ReadingKind)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPorePressure, EquivalentMudWeight, MassPerVolumeMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportPorePressure, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPorePressure, Tvd, AbstractVerticalDepth)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPorePressure, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportPorePressure, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportPorePressure)

//struct ns1__TrajectoryOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, TrajectoryOSDUIntegration)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryOSDUIntegration, ActiveIndicator)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryOSDUIntegration, AppliedOperation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryOSDUIntegration, IntermediaryServiceCompany, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryOSDUIntegration, SurveyToolType)
  READ_O_UTF8_OBJECT_21_VOID(TrajectoryOSDUIntegration, TrajectoryVersion)
BSON_READ_OBJECT21_END(TrajectoryOSDUIntegration)

//struct ns1__DrillReportSurveyStationReport
BSON_READ_OBJECT21_BEGIN(ns1, DrillReportSurveyStationReport)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, AcquisitionRemark)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, MagDeclUsed, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, MdMaxExtrapolated, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, MdMaxMeasured, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, MdTieOn, MeasuredDepth)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, NominalCalcAlgorithm)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, NominalTypeSurveyTool)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, NominalTypeTrajStation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, TrajectoryOSDUIntegration, TrajectoryOSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, GridConUsed, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, GridScaleFactorUsed, LengthPerLengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, AziVertSect, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, DispNsVertSectOrig, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportSurveyStationReport, DispEwVertSectOrig, LengthMeasureExt)
  READ_O_OBJECT_ENUM_NULLABLE_21_VOID(ns2, DrillReportSurveyStationReport, AziRef, NorthReferenceKind)
BSON_READ_OBJECT21_END(DrillReportSurveyStationReport)

//struct ns2__NameStruct
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, NameStruct)
  READ_A_UTF8_OBJECT_ITEM_21_OR_ELSE_GOTO_RESUME(NameStruct)
  READ_A_PUT_SINGLE_ATTR_21_VOID(NameStruct, authority)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, NameStruct)

//struct ns1__DrillActivity
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillActivity)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillActivity, DTimStart)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, ProprietaryCode, NameStruct)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillActivity, DTimEnd)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Duration, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Tvd, AbstractVerticalDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Phase)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillActivity, ActivityCode, DrillActivityCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, DetailActivity)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillActivity, TypeActivityClass, DrillActivityClassType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, ActivityMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, ActivityTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, BitMdInterval, MdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, State)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillActivity, StateDetailActivity, StateDetailActivity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Operator, DataObjectReference)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillActivity, Optimum)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillActivity, Productive)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillActivity, ItemState)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, BhaRun, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillActivity, Tubular, DataObjectReference)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillActivity, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillActivity)

//struct ns1__BottomHoleCirculatingTemperature
BSON_READ_OBJECT21_BEGIN(ns1, BottomHoleCirculatingTemperature)
  READ_O_OBJECT_21_VOID(BottomHoleCirculatingTemperature, BottomHoleTemperature, ThermodynamicTemperatureMeasure)
BSON_READ_OBJECT21_END_(BottomHoleCirculatingTemperature)

//struct ns1__BottomHoleStaticTemperature
BSON_READ_OBJECT21_BEGIN(ns1, BottomHoleStaticTemperature)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BottomHoleStaticTemperature, BottomHoleTemperature, ThermodynamicTemperatureMeasure)
  READ_O_OBJECT_21_VOID(BottomHoleStaticTemperature, ETimStatic, TimeMeasure)
BSON_READ_OBJECT21_END(BottomHoleStaticTemperature)

//struct ns1__AbstractBottomHoleTemperature
BSON_READ_OBJECT21_BEGIN(ns1, AbstractBottomHoleTemperature)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AbstractBottomHoleTemperature, BottomHoleTemperature, ThermodynamicTemperatureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractBottomHoleTemperature, BottomHoleCirculatingTemperature, BottomHoleCirculatingTemperature)
  READ_O_OBJECT_21_VOID_B(ns1, AbstractBottomHoleTemperature, BottomHoleStaticTemperature, BottomHoleStaticTemperature)
BSON_READ_OBJECT21_END(AbstractBottomHoleTemperature)

//struct ns1__DrillReportLogInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportLogInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, DTim)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, RunNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, ServiceCompany, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, LoggedMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, LoggedTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, Tool, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, MdTempTool, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, TvdTempTool, AbstractVerticalDepth)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLogInfo, BottomHoleTemperature, AbstractBottomHoleTemperature)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportLogInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportLogInfo)

//struct ns1__DrillReportCoreInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportCoreInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, DTim)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, CoreNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, CoredMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, CoredTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, LenRecovered, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, RecoverPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, LenBarrel, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportCoreInfo, InnerBarrelType, InnerBarrelType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, CoreDescription)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportCoreInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportCoreInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportCoreInfo)

//struct ns1__DrillReportWellTestInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportWellTestInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, DTim)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportWellTestInfo, TestType, WellTestType)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, TestNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, ChokeOrificeSize, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, DensityOil, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, DensityWater, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, DensityGas, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, FlowRateOil, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, FlowRateWater, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, FlowRateGas, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, PresShutIn, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, PresFlowing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, PresBottom, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, GasOilRatio, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, WaterOilRatio, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, Chloride, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, CarbonDioxide, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, HydrogenSulfide, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, VolOilTotal, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, VolGasTotal, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, VolWaterTotal, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, VolOilStored, VolumeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportWellTestInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportWellTestInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportWellTestInfo)

//struct ns1__DrillReportFormTestInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportFormTestInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, Tvd, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, PresPore, PressureMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, GoodSeal)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, MdSample, MeasuredDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, DominateComponent)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, DensityHC, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, VolumeSample, VolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, Description)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportFormTestInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportFormTestInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportFormTestInfo)

//struct ns1__DrillReportLithShowInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportLithShowInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, ShowMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, ShowTvdInterval, AbstractTvdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, Show)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, Lithology)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportLithShowInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportLithShowInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportLithShowInfo)

//struct ns1__DrillReportEquipFailureInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportEquipFailureInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, Tvd, AbstractVerticalDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, EquipClass)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, ETimMissProduction, TimeMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, DTimRepair)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, Description)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportEquipFailureInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportEquipFailureInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportEquipFailureInfo)

//struct ns1__DrillReportControlIncidentInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportControlIncidentInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, MdInflow, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, TvdInflow, AbstractVerticalDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, Phase)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportControlIncidentInfo, ActivityCode, DrillActivityCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, DetailActivity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, ETimLost, TimeMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, DTimRegained)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, DiaBit, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, MdBit, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, WtMud, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, PorePressure, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, DiaCsgLast, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, MdCsgLast, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, VolMudGained, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, PresShutInCasing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, PresShutInDrill, PressureMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportControlIncidentInfo, IncidentType, WellControlIncidentType)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportControlIncidentInfo, KillingType, WellKillingProcedureType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, Formation)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, TempBottom, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, PresMaxChoke, PressureMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, Description)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportControlIncidentInfo, ProprietaryCode, NameStruct)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportControlIncidentInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportControlIncidentInfo)

//struct ns1__DrillReportStratInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportStratInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportStratInfo, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStratInfo, MdTop, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStratInfo, TvdTop, AbstractVerticalDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStratInfo, Description)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportStratInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportStratInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportStratInfo)

//struct ns1__DrillReportPerfInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportPerfInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportPerfInfo, DTimOpen)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportPerfInfo, DTimClose)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPerfInfo, PerforationMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPerfInfo, PerforationTvdInterval, AbstractTvdInterval)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportPerfInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportPerfInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportPerfInfo)

//struct ns1__DrillReportGasReadingInfo
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillReportGasReadingInfo)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, DTim)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReportGasReadingInfo, ReadingType, GasPeakType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, GasReadingMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, GasReadingTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, GasHigh, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, GasLow, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Meth, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Eth, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Prop, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Ibut, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Nbut, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, Ipent, VolumePerVolumeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReportGasReadingInfo, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DrillReportGasReadingInfo, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillReportGasReadingInfo)

//struct ns2__GrowingObjectIndex
BSON_READ_OBJECT21_BEGIN(ns2, GrowingObjectIndex)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, GrowingObjectIndex, IndexKind, DataIndexKind)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GrowingObjectIndex, IndexPropertyKind, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GrowingObjectIndex, Uom)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, GrowingObjectIndex, Direction, IndexDirection)
  READ_O_OBJECT_21_VOID(GrowingObjectIndex, Datum, DataObjectReference)
BSON_READ_OBJECT21_END(GrowingObjectIndex)

//struct ns1__CuttingsIntervalShow
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CuttingsIntervalShow)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, Citation, Citation)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, ShowRating, ShowRating)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, StainColor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, StainDistr)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, StainPc, AreaPerAreaMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, CutSpeed, ShowSpeed)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CutColor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CutStrength)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, CutForm, ShowLevel)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CutLevel)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, CutFlorForm, ShowLevel)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CutFlorColor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CutFlorStrength)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, CutFlorSpeed, ShowSpeed)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, CutFlorLevel, ShowFluorescence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, NatFlorColor)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, NatFlorPc, AreaPerAreaMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CuttingsIntervalShow, NatFlorLevel, ShowFluorescence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, NatFlorDesc)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, ResidueColor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, ImpregnatedLitho)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, Odor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalShow, CuttingFluid)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CuttingsIntervalShow, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CuttingsIntervalShow)

//struct ns1__LithologyQualifier
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, LithologyQualifier)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LithologyQualifier, Kind)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(LithologyQualifier, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(LithologyQualifier, Abundance, VolumePerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LithologyQualifier, Description)
  READ_A_PUT_SINGLE_ATTR_21_VOID(LithologyQualifier, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, LithologyQualifier)

//struct ns1__CuttingsIntervalLithology
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CuttingsIntervalLithology)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Kind)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, LithPc, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, CodeLith)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Color)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Texture)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Hardness)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Compaction)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, SizeGrain)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Roundness)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Sphericity)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Sorting)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, CuttingsIntervalLithology, MatrixCement, MatrixCementKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, PorosityVisible)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, PorosityFabric)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Permeability)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Shows, CuttingsIntervalShow)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsIntervalLithology, Qualifier, LithologyQualifier)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CuttingsIntervalLithology, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CuttingsIntervalLithology)

//struct ns1__CuttingsGeologyInterval
BSON_READ_OBJECT21_BEGIN(ns1, CuttingsGeologyInterval)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, Creation)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, LastUpdate)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DensBulk, MassPerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DensShale, MassPerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Calcite, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CalcStab, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Cec, DimensionlessMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Dolomite, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, SizeMin, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, SizeMax, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Qft, IlluminanceMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CleaningMethod)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DryingMethod)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, BottomsUpTime, TimeMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CuttingsIntervalLithology, CuttingsIntervalLithology)
  READ_O_PUT_TWO_ATTR_21_VOID(CuttingsGeologyInterval, uid, objectVersion)
BSON_READ_OBJECT21_END(CuttingsGeologyInterval)

//struct ns1__CuttingsGeologyInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CuttingsGeologyInterval)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeologyInterval, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DensBulk, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DensShale, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Calcite, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CalcStab, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Cec, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Dolomite, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, SizeMin, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, SizeMax, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, Qft, IlluminanceMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CleaningMethod)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, DryingMethod)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, BottomsUpTime, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeologyInterval, CuttingsIntervalLithology, CuttingsIntervalLithology)
  READ_A_PUT_TWO_ATTR_21_VOID(CuttingsGeologyInterval, uid, objectVersion)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CuttingsGeologyInterval)

//struct ns1__GeochronologicalUnit
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, GeochronologicalUnit)
  READ_A_UTF8_OBJECT_ITEM_21_OR_ELSE_GOTO_RESUME(GeochronologicalUnit)
  READ_A_PUT_TWO_ATTR_21_VOID_B(GeochronologicalUnit, authority, kind, GeochronologicalRank)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, GeochronologicalUnit)

//struct ns1__LithostratigraphicUnit
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, LithostratigraphicUnit)
  READ_A_UTF8_OBJECT_ITEM_21_OR_ELSE_GOTO_RESUME(LithostratigraphicUnit)
  READ_A_PUT_TWO_ATTR_21_VOID_B(LithostratigraphicUnit, authority, kind, LithostratigraphicRank)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, LithostratigraphicUnit)

//struct ns1__InterpretedIntervalLithology
BSON_READ_OBJECT21_BEGIN(ns1, InterpretedIntervalLithology)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Kind)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, CodeLith)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Color)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Texture)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Hardness)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Compaction)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, SizeGrain)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Roundness)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Sorting)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Sphericity)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, InterpretedIntervalLithology, MatrixCement, MatrixCementKind)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, PorosityVisible)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, PorosityFabric)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Permeability)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedIntervalLithology, Qualifier, LithologyQualifier)
  READ_O_PUT_SINGLE_ATTR_21_VOID(InterpretedIntervalLithology, uid)
BSON_READ_OBJECT21_END(InterpretedIntervalLithology)

//struct ns1__InterpretedGeologyInterval
BSON_READ_OBJECT21_BEGIN(ns1, InterpretedGeologyInterval)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, Creation)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, LastUpdate)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, MdInterval, MdInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, GeochronologicalUnit, GeochronologicalUnit)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, LithostratigraphicUnit, LithostratigraphicUnit)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, InterpretedLithology, InterpretedIntervalLithology)
  READ_O_PUT_TWO_ATTR_21_VOID(InterpretedGeologyInterval, uid, objectVersion)
BSON_READ_OBJECT21_END(InterpretedGeologyInterval)

//struct ns1__InterpretedGeologyInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, InterpretedGeologyInterval)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeologyInterval, MdInterval, MdInterval)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, GeochronologicalUnit, GeochronologicalUnit)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, LithostratigraphicUnit, LithostratigraphicUnit)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeologyInterval, InterpretedLithology, InterpretedIntervalLithology)
  READ_A_PUT_TWO_ATTR_21_VOID(InterpretedGeologyInterval, uid, objectVersion)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, InterpretedGeologyInterval)

//struct ns1__ShowEvaluationInterval
BSON_READ_OBJECT21_BEGIN(ns1, ShowEvaluationInterval)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, Creation)
  READ_O_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, LastUpdate)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, MdInterval, MdInterval)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, ShowEvaluationInterval, ShowFluid, ShowFluid)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ShowEvaluationInterval, ShowRating, ShowRating)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ShowEvaluationInterval, BottomsUpTime, TimeMeasure)
  READ_O_PUT_TWO_ATTR_21_VOID(ShowEvaluationInterval, uid, objectVersion)
BSON_READ_OBJECT21_END(ShowEvaluationInterval)

//struct ns1__ShowEvaluationInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ShowEvaluationInterval)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluationInterval, MdInterval, MdInterval)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, ShowEvaluationInterval, ShowFluid, ShowFluid)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ShowEvaluationInterval, ShowRating, ShowRating)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShowEvaluationInterval, BottomsUpTime, TimeMeasure)
  READ_A_PUT_TWO_ATTR_21_VOID(ShowEvaluationInterval, uid, objectVersion)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ShowEvaluationInterval)

//struct ns1__Chromatograph
BSON_READ_OBJECT21_BEGIN(ns1, Chromatograph)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, ChromatographMdInterval, MdInterval)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Chromatograph, DateTimeGasSampleProcessed)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, ChromatographType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, ETimChromCycle, TimeMeasure)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Chromatograph, ChromReportTime)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, MudWeightIn, MassPerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, MudWeightOut, MassPerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, MethAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, MethMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, MethMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EthAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EthMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EthMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, PropAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, PropMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, PropMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IbutAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IbutMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IbutMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NbutAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NbutMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NbutMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IpentAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IpentMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IpentMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NpentAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NpentMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NpentMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EpentAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EpentMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, EpentMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IhexAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IhexMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, IhexMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NhexAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NhexMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, NhexMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, Co2Av, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, Co2Mn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, Co2Mx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, H2sAv, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, H2sMn, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, H2sMx, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Chromatograph, Acetylene, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_VOID(Chromatograph, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(Chromatograph)

//struct ns1__RopStatistics
BSON_READ_OBJECT21_BEGIN(ns1, RopStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RopStatistics, Average, LengthPerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RopStatistics, Minimum, LengthPerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RopStatistics, Maximum, LengthPerTimeMeasure)
  READ_O_OBJECT_21_VOID(RopStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(RopStatistics)

//struct ns1__WobStatistics
BSON_READ_OBJECT21_BEGIN(ns1, WobStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(WobStatistics, Average, ForceMeasure)
  READ_O_OBJECT_21_VOID(WobStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(WobStatistics)

//struct ns1__TorqueStatistics
BSON_READ_OBJECT21_BEGIN(ns1, TorqueStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TorqueStatistics, Average, MomentOfForceMeasure)
  READ_O_OBJECT_21_VOID(TorqueStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(TorqueStatistics)

//TorqueCurrentStatistics
BSON_READ_OBJECT21_BEGIN(ns1, TorqueCurrentStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TorqueCurrentStatistics, Average, ElectricCurrentMeasure)
  READ_O_OBJECT_21_VOID(TorqueCurrentStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(TorqueCurrentStatistics)

//struct ns1__RpmStatistics
BSON_READ_OBJECT21_BEGIN(ns1, RpmStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RpmStatistics, Average, AngularVelocityMeasure)
  READ_O_OBJECT_21_VOID(RpmStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(RpmStatistics)

//struct ns1__MudDensityStatistics
BSON_READ_OBJECT21_BEGIN(ns1, MudDensityStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudDensityStatistics, Average, MassPerVolumeMeasure)
  READ_O_OBJECT_21_VOID(MudDensityStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(MudDensityStatistics)

//struct ns1__EcdStatistics
BSON_READ_OBJECT21_BEGIN(ns1, EcdStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(EcdStatistics, Average, MassPerVolumeMeasure)
  READ_O_OBJECT_21_VOID(EcdStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(EcdStatistics)

//struct ns1__DxcStatistics
BSON_READ_OBJECT21_BEGIN(ns1, DxcStatistics)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DxcStatistics, Average, DimensionlessMeasure)
  READ_O_OBJECT_21_VOID(DxcStatistics, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(DxcStatistics)

//struct ns1__DrillingParameters
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DrillingParameters)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, Rop, RopStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageWeightOnBit, WobStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageTorque, TorqueStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageTorqueCurrent, TorqueCurrentStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageTurnRate, RpmStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageMudDensity, MudDensityStatistics)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillingParameters, AverageEcdAtTd, EcdStatistics)
  READ_A_OBJECT_21_VOID(DrillingParameters, AverageDrillingCoefficient, DxcStatistics)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DrillingParameters)

//struct ns1__GasInMud
BSON_READ_OBJECT21_BEGIN(ns1, GasInMud)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GasInMud, Average, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GasInMud, Maximum, VolumePerVolumeMeasure)
  READ_O_OBJECT_21_VOID(GasInMud, Channel, DataObjectReference)
BSON_READ_OBJECT21_END(GasInMud)

//struct ns1__GasPeak
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, GasPeak)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, GasPeak, PeakType, GasPeakType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GasPeak, MdPeak, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GasPeak, AverageGas, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GasPeak, PeakGas, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GasPeak, BackgroundGas, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_VOID(GasPeak, Channel, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, GasPeak)

//struct ns1__MudGas
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, MudGas)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudGas, GasInMud, GasInMud)
  READ_A_ARRAY_OF_OBJECT_21_VOID(MudGas, GasPeak, GasPeak)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, MudGas)

//struct ns1__MudlogReportInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, MudlogReportInterval)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, MudlogReportInterval, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, MudlogReportInterval, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudlogReportInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudlogReportInterval, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, CuttingsGeologyInterval, CuttingsGeologyInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, InterpretedGeologyInterval, InterpretedGeologyInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, ShowEvaluationInterval, ShowEvaluationInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, BottomsUpTime, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, Chromatograph, Chromatograph)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, DrillingParameters, DrillingParameters)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MudlogReportInterval, MudGas, MudGas)
  READ_A_PUT_TWO_ATTR_21_VOID(MudlogReportInterval, uid, objectVersion)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, MudlogReportInterval)

//struct ns1__MudLogConcentrationParameter
BSON_READ_OBJECT21_BEGIN(ns1, MudLogConcentrationParameter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogConcentrationParameter, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogConcentrationParameter, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogConcentrationParameter, Comments)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogConcentrationParameter, Value, VolumePerVolumeMeasureExt)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, MudLogConcentrationParameter, ConcentrationParameterKind, ConcentrationParameterKind)
  READ_O_PUT_SINGLE_ATTR_21_VOID(MudLogConcentrationParameter, uid)
BSON_READ_OBJECT21_END(MudLogConcentrationParameter)

//struct ns1__MudLogForceParameter
BSON_READ_OBJECT21_BEGIN(ns1, MudLogForceParameter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogForceParameter, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogForceParameter, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogForceParameter, Comments)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogForceParameter, Value, ForceMeasureExt)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, MudLogForceParameter, ForceParameterKind, ForceParameterKind)
  READ_O_PUT_SINGLE_ATTR_21_VOID(MudLogForceParameter, uid)
BSON_READ_OBJECT21_END(MudLogForceParameter)

//struct ns1__MudLogPressureGradientParameter
BSON_READ_OBJECT21_BEGIN(ns1, MudLogPressureGradientParameter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureGradientParameter, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureGradientParameter, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureGradientParameter, Comments)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureGradientParameter, Value, ForcePerVolumeMeasureExt)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, MudLogPressureGradientParameter, PressureGradientParameterKind, PressureGradientParameterKind)
  READ_O_PUT_SINGLE_ATTR_21_VOID(MudLogPressureGradientParameter, uid)
BSON_READ_OBJECT21_END(MudLogPressureGradientParameter)

//struct ns1__MudLogPressureParameter
BSON_READ_OBJECT21_BEGIN(ns1, MudLogPressureParameter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureParameter, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureParameter, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureParameter, Comments)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogPressureParameter, Value, PressureMeasureExt)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, MudLogPressureParameter, PressureParameterKind, PressureParameterKind)
  READ_O_PUT_SINGLE_ATTR_21_VOID(MudLogPressureParameter, uid)
BSON_READ_OBJECT21_END(MudLogPressureParameter)

//struct ns1__MudLogStringParameter
BSON_READ_OBJECT21_BEGIN(ns1, MudLogStringParameter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogStringParameter, MdInterval, MdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogStringParameter, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogStringParameter, Comments)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogStringParameter, Value)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, MudLogStringParameter, MudLogStringParameterKind, MudLogStringParameterKind)
  READ_O_PUT_SINGLE_ATTR_21_VOID(MudLogStringParameter, uid)
BSON_READ_OBJECT21_END(MudLogStringParameter)

//struct ns1__MudLogParameter
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, MudLogParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogParameter, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogParameter, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogParameter, Comments)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, MudLogParameter, MudLogConcentrationParameter, MudLogConcentrationParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, MudLogParameter, MudLogForceParameter, MudLogForceParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, MudLogParameter, MudLogPressureGradientParameter, MudLogPressureGradientParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, MudLogParameter, MudLogPressureParameter, MudLogPressureParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, MudLogParameter, MudLogStringParameter, MudLogStringParameter)
  READ_A_PUT_SINGLE_ATTR_21_VOID(MudLogParameter, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, MudLogParameter)

//struct ns1__NameTag
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, NameTag)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(NameTag, Name)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, NameTag, NumberingScheme, NameTagNumberingScheme)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, NameTag, Technology, NameTagTechnology)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, NameTag, Location, NameTagLocation)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(NameTag, InstallationDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(NameTag, InstallationCompany, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(NameTag, MountingCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(NameTag, Comment)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(NameTag, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(NameTag, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, NameTag)

//struct ns1__DayCost
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DayCost)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumAFE)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostGroup)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostClass)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostSubCode)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostItemDescription)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, ItemKind)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(DayCost, ItemSize)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(DayCost, QtyItem)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumInvoice)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumPO)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumTicket)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DayCost, IsCarryOver)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DayCost, IsRental)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NameTag, NameTag)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumSerial)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, Vendor, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, NumVendor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, Pool)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(DayCost, Estimated)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostAmount, Cost)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DayCost, CostPerItem, Cost)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DayCost, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DayCost)

//struct ns1__TrajectoryStationOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, TrajectoryStationOSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStationOSDUIntegration, Easting, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStationOSDUIntegration, Northing, LengthMeasureExt)
  READ_O_OBJECT_21_VOID(TrajectoryStationOSDUIntegration, RadiusOfUncertainty, LengthMeasure)
BSON_READ_OBJECT21_END(TrajectoryStationOSDUIntegration)

//struct ns1__StnTrajRawData
BSON_READ_OBJECT21_BEGIN(ns1, StnTrajRawData)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajRawData, GravAxialRaw, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajRawData, GravTran1Raw, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajRawData, GravTran2Raw, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajRawData, MagAxialRaw, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajRawData, MagTran1Raw, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_VOID(StnTrajRawData, MagTran2Raw, MagneticFluxDensityMeasure)
BSON_READ_OBJECT21_END(StnTrajRawData)

//struct ns1__StnTrajCorUsed
BSON_READ_OBJECT21_BEGIN(ns1, StnTrajCorUsed)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, GravAxialAccelCor, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, GravTran1AccelCor, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, GravTran2AccelCor, LinearAccelerationMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagAxialDrlstrCor, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagTran1DrlstrCor, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagTran2DrlstrCor, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagTran1MSACor, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagTran2MSACor, MagneticFluxDensityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, MagAxialMSACor, MagneticFluxDensityMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StnTrajCorUsed, SagIncCor, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StnTrajCorUsed, SagAziCor, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StnTrajCorUsed, StnMagDeclUsed, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StnTrajCorUsed, StnGridConUsed, PlaneAngleMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajCorUsed, StnGridScaleFactorUsed, LengthPerLengthMeasure)
  READ_O_OBJECT_21_VOID(StnTrajCorUsed, DirSensorOffset, LengthMeasure)
BSON_READ_OBJECT21_END(StnTrajCorUsed)

//struct ns1__StnTrajValid
BSON_READ_OBJECT21_BEGIN(ns1, StnTrajValid)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajValid, MagTotalFieldCalc, MagneticFluxDensityMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StnTrajValid, MagDipAngleCalc, PlaneAngleMeasure)
  READ_O_OBJECT_21_VOID(StnTrajValid, GravTotalFieldCalc, LinearAccelerationMeasure)
BSON_READ_OBJECT21_END(StnTrajValid)

//struct ns1__StnTrajMatrixCov
BSON_READ_OBJECT21_BEGIN(ns1, StnTrajMatrixCov)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceNN, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceNE, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceNVert, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceEE, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceEVert, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, VarianceVertVert, AreaMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, BiasN, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, BiasE, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StnTrajMatrixCov, BiasVert, LengthMeasure)
  READ_O_DOUBLE_21_VOID(StnTrajMatrixCov, Sigma)
BSON_READ_OBJECT21_END(StnTrajMatrixCov)

//struct ns2__LocalEngineering2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, LocalEngineering2dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(LocalEngineering2dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(LocalEngineering2dPosition, Coordinate2)
  READ_O_OBJECT_21_VOID(LocalEngineering2dPosition, LocalEngineering2dCrs, DataObjectReference)
BSON_READ_OBJECT21_END(LocalEngineering2dPosition)

//struct ns2__Projected2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Projected2dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Projected2dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Projected2dPosition, Coordinate2)
  READ_O_OBJECT_21_VOID(Projected2dPosition, ProjectedCrs, DataObjectReference)
BSON_READ_OBJECT21_END(Projected2dPosition)

//struct ns2__PublicLandSurveySystemLocation
BSON_READ_OBJECT21_BEGIN(ns2, PublicLandSurveySystemLocation)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, PublicLandSurveySystemLocation, PrincipalMeridian, PrincipalMeridian)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystemLocation, Range)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, PublicLandSurveySystemLocation, RangeDir, EastOrWest)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystemLocation, Township)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, PublicLandSurveySystemLocation, TownshipDir, NorthOrSouth)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystemLocation, Section)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystemLocation, QuarterSection)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystemLocation, QuarterTownship)
  READ_O_OBJECT_ENUM_21_VOID(ns2, PublicLandSurveySystemLocation, AxisOrder, AxisOrder2d)
BSON_READ_OBJECT21_END(PublicLandSurveySystemLocation)

//struct ns2__PublicLandSurveySystem2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, PublicLandSurveySystem2dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystem2dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(PublicLandSurveySystem2dPosition, Coordinate2)
  READ_O_OBJECT_21_VOID(PublicLandSurveySystem2dPosition, PublicLandSurveySystemLocation, PublicLandSurveySystemLocation)
BSON_READ_OBJECT21_END(PublicLandSurveySystem2dPosition)

//struct ns2__AbstractCartesian2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, AbstractCartesian2dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractCartesian2dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AbstractCartesian2dPosition, Coordinate2)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractCartesian2dPosition, LocalEngineering2dPosition, LocalEngineering2dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractCartesian2dPosition, Projected2dPosition, Projected2dPosition)
  READ_O_OBJECT_21_VOID_B(ns2, AbstractCartesian2dPosition, PublicLandSurveySystem2dPosition, PublicLandSurveySystem2dPosition)
BSON_READ_OBJECT21_END(AbstractCartesian2dPosition)

//struct ns2__Geographic2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Geographic2dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic2dPosition, Latitude, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic2dPosition, Longitude, PlaneAngleMeasureExt)
  READ_O_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Geographic2dPosition, Epoch)
  READ_O_OBJECT_21_VOID(Geographic2dPosition, GeographicCrs, DataObjectReference)
BSON_READ_OBJECT21_END(Geographic2dPosition)

//struct ns2__Abstract2dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Abstract2dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract2dPosition, AbstractCartesian2dPosition, AbstractCartesian2dPosition)
  READ_O_OBJECT_21_VOID_B(ns2, Abstract2dPosition, Geographic2dPosition, Geographic2dPosition)
BSON_READ_OBJECT21_END(Abstract2dPosition)

//struct ns2__Geocentric3dCrs
BSON_READ_OBJECT21_BEGIN(ns2, Geocentric3dCrs)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_D(Geocentric3dCrs, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Geocentric3dCrs, ExtensionNameValue, ExtensionNameValue)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(Geocentric3dCrs)
BSON_READ_OBJECT21_END(Geocentric3dCrs)

//struct ns2__Geocentric3dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Geocentric3dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geocentric3dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geocentric3dPosition, Coordinate2)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geocentric3dPosition, Coordinate3)
  READ_O_OBJECT_21_VOID(Geocentric3dPosition, Geocentric3dCrs, Geocentric3dCrs)
BSON_READ_OBJECT21_END(Geocentric3dPosition)

//struct ns2__Geographic3dCrs
BSON_READ_OBJECT21_BEGIN(ns2, Geographic3dCrs)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_D(Geographic3dCrs, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Geographic3dCrs, ExtensionNameValue, ExtensionNameValue)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(Geographic3dCrs)
BSON_READ_OBJECT21_END(Geographic3dCrs)

//struct ns2__Geographic3dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Geographic3dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geographic3dPosition, Latitude)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geographic3dPosition, Longitude)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Geographic3dPosition, EllipsoidalHeight)
  READ_O_OBJECT_21_VOID(Geographic3dPosition, Geographic3dCrs, Geographic3dCrs)
BSON_READ_OBJECT21_END(Geographic3dPosition)

//struct ns2__LocalEngineering3dPosition
BSON_READ_OBJECT21_BEGIN(ns2, LocalEngineering3dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(LocalEngineering3dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(LocalEngineering3dPosition, Coordinate2)
  READ_O_DOUBLE_21_VOID(LocalEngineering3dPosition, VerticalCoordinate)
BSON_READ_OBJECT21_END(LocalEngineering3dPosition)

//struct ns2__Projected3dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Projected3dPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Projected3dPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Projected3dPosition, Coordinate2)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(Projected3dPosition, EllipsoidalHeight)
  READ_O_OBJECT_21_VOID(Projected3dPosition, ProjectedCrs, DataObjectReference)
BSON_READ_OBJECT21_END(Projected3dPosition)

//struct ns2__Abstract3dPosition
BSON_READ_OBJECT21_BEGIN(ns2, Abstract3dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, Geocentric3dPosition, Geocentric3dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, Geographic3dPosition, Geographic3dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, LocalEngineering3dPosition, LocalEngineering3dPosition)
  READ_O_OBJECT_21_VOID_B(ns2, Abstract3dPosition, Projected3dPosition, Projected3dPosition)
BSON_READ_OBJECT21_END(Abstract3dPosition)

//struct ns2__Abstract3dPosition
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, Abstract3dPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, Geocentric3dPosition, Geocentric3dPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, Geographic3dPosition, Geographic3dPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Abstract3dPosition, LocalEngineering3dPosition, LocalEngineering3dPosition)
  READ_A_OBJECT_21_VOID_B(ns2, Abstract3dPosition, Projected3dPosition, Projected3dPosition)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, Abstract3dPosition)

//struct ns2__EngineeringCompoundPosition
BSON_READ_OBJECT21_BEGIN(ns2, EngineeringCompoundPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(EngineeringCompoundPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(EngineeringCompoundPosition, Coordinate2)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(EngineeringCompoundPosition, VerticalCoordinate)
  READ_O_OBJECT_21_VOID(EngineeringCompoundPosition, LocalEngineeringCompoundCrs, DataObjectReference)
BSON_READ_OBJECT21_END(EngineeringCompoundPosition)

//struct ns2__GeographicCompoundCrs
BSON_READ_OBJECT21_BEGIN(ns2, GeographicCompoundCrs)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_D(GeographicCompoundCrs, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, VerticalCrs, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GeographicCompoundCrs, Geographic2dCrs, DataObjectReference)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(GeographicCompoundCrs)
BSON_READ_OBJECT21_END(GeographicCompoundCrs)

//struct ns2__GeographicCompoundPosition
BSON_READ_OBJECT21_BEGIN(ns2, GeographicCompoundPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(GeographicCompoundPosition, Latitude)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(GeographicCompoundPosition, Longitude)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(GeographicCompoundPosition, VerticalCoordinate)
  READ_O_OBJECT_21_VOID(GeographicCompoundPosition, GeographicCompoundCrs, GeographicCompoundCrs)
BSON_READ_OBJECT21_END(GeographicCompoundPosition)

//struct ns2__ProjectedCompoundCrs
BSON_READ_OBJECT21_BEGIN(ns2, ProjectedCompoundCrs)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_D(ProjectedCompoundCrs, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, VerticalCrs, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundCrs, ProjectedCrs, DataObjectReference)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(ProjectedCompoundCrs)
BSON_READ_OBJECT21_END(ProjectedCompoundCrs)

//struct ns2__ProjectedCompoundPosition
BSON_READ_OBJECT21_BEGIN(ns2, ProjectedCompoundPosition)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundPosition, Coordinate1)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundPosition, Coordinate2)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(ProjectedCompoundPosition, VerticalCoordinate)
  READ_O_OBJECT_21_VOID(ProjectedCompoundPosition, ProjectedCompoundCrs, ProjectedCompoundCrs)
BSON_READ_OBJECT21_END(ProjectedCompoundPosition)

//struct ns2__AbstractCompoundPosition
BSON_READ_OBJECT21_BEGIN(ns2, AbstractCompoundPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractCompoundPosition, EngineeringCompoundPosition, EngineeringCompoundPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractCompoundPosition, GeographicCompoundPosition, GeographicCompoundPosition)
  READ_O_OBJECT_21_VOID_B(ns2, AbstractCompoundPosition, ProjectedCompoundPosition, ProjectedCompoundPosition)
BSON_READ_OBJECT21_END(AbstractCompoundPosition)

//struct ns2__AbstractPosition
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, AbstractPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractPosition, Abstract2dPosition, Abstract2dPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractPosition, Abstract3dPosition, Abstract3dPosition)
  READ_A_OBJECT_21_VOID_B(ns2, AbstractPosition, AbstractCompoundPosition, AbstractCompoundPosition)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, AbstractPosition)

//struct ns2__AbstractPosition
BSON_READ_OBJECT21_BEGIN(ns2, AbstractPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractPosition, Abstract2dPosition, Abstract2dPosition)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, AbstractPosition, Abstract3dPosition, Abstract3dPosition)
  READ_O_OBJECT_21_VOID_B(ns2, AbstractPosition, AbstractCompoundPosition, AbstractCompoundPosition)
BSON_READ_OBJECT21_END(AbstractPosition)

//struct ns1__SourceTrajectoryStation
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, SourceTrajectoryStation)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SourceTrajectoryStation, StationReference, ComponentReference)
  READ_A_OBJECT_21_VOID(SourceTrajectoryStation, Trajectory, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, SourceTrajectoryStation)

//struct ns1__TrajectoryStation
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, TrajectoryStation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, TrajectoryStation, Creation)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME_B(ns2, TrajectoryStation, LastUpdate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, TrajectoryStation, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, TrajectoryStation, Md, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Closure, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, ClosureDirection, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, DispLatitude, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, DispLongitude, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, OriginalLatitude, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, OriginalLongitude, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, TrajectoryStationOSDUIntegration, TrajectoryStationOSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, TrueClosure, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, TrueClosureDirection, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, WGS84Latitude, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, WGS84Longitude, PlaneAngleMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, ManuallyEntered)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Target, DataObjectReference)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, DTimStn)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, TrajectoryStation, TypeTrajStation, TrajStationType)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, TrajectoryStation, TypeSurveyTool, TypeSurveyTool)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, TrajectoryStation, CalcAlgorithm, TrajStnCalcAlgorithm)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Tvd, AbstractVerticalDepth)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, Incl, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, Azi, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, Mtf, PlaneAngleMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, Gtf, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, DispNs, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, DispEw, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, VertSect, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Dls, AnglePerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, RateTurn, AnglePerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, RateBuild, AnglePerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MdDelta, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, TvdDelta, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, GravTotalUncert, LinearAccelerationMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, DipAngleUncert, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagTotalUncert, MagneticFluxDensityMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, GravAccelCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagXAxialCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, SagCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagDrlstrCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, InfieldRefCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, InterpolatedInfieldRefCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, InHoleRefCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, AxialMagInterferenceCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, CosagCorUsed)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MSACorUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, GravTotalFieldReference, LinearAccelerationMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagTotalFieldReference, MagneticFluxDensityMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TrajectoryStation, MagDipAngleReference, PlaneAngleMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagModelUsed)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MagModelValid)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, GeoModelUsed)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, TrajectoryStation, StatusTrajStation, TrajStationStatus)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, RawData, StnTrajRawData)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, CorUsed, StnTrajCorUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Valid, StnTrajValid)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, MatrixCov, StnTrajMatrixCov)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, Location, AbstractPosition)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, SourceStation, SourceTrajectoryStation)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryStation, ToolErrorModel, DataObjectReference)
  READ_A_PUT_TWO_ATTR_21_VOID(TrajectoryStation, uid, objectVersion)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, TrajectoryStation)

//struct ns1__TrajectoryReport
BSON_READ_OBJECT21_BEGIN(ns1, TrajectoryReport)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, AcquisitionRemark)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, MagDeclUsed, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, MdMaxExtrapolated, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, MdMaxMeasured, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, MdTieOn, MeasuredDepth)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, NominalCalcAlgorithm)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, NominalTypeSurveyTool)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, NominalTypeTrajStation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, TrajectoryOSDUIntegration, TrajectoryOSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, GridConUsed, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, GridScaleFactorUsed, LengthPerLengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, AziVertSect, PlaneAngleMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, DispNsVertSectOrig, LengthMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TrajectoryReport, DispEwVertSectOrig, LengthMeasureExt)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, TrajectoryReport, AziRef, NorthReferenceKind)
  READ_O_ARRAY_OF_OBJECT_21_VOID(TrajectoryReport, TrajectoryStation, TrajectoryStation)
BSON_READ_OBJECT21_END(TrajectoryReport)

//struct ns1__AnchorState
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, AnchorState)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AnchorState, AnchorName)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(AnchorState, AnchorAngle, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(AnchorState, AnchorTension, ForceMeasure)
  READ_A_UTF8_OBJECT_21_VOID(AnchorState, Description)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, AnchorState)

//struct ns1__RigResponse
BSON_READ_OBJECT21_BEGIN(ns1, RigResponse)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, RigHeading, PlaneAngleMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, RigHeave, LengthMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, RigPitchAngle, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, RigRollAngle, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, RiserAngle, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, RiserDirection, PlaneAngleMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, RiserTension, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, VariableDeckLoad, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, TotalDeckLoad, ForceMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, GuideBaseAngle, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, BallJointAngle, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(RigResponse, BallJointDirection, PlaneAngleMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, OffsetRig, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, LoadLeg1, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, LoadLeg2, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, LoadLeg3, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, LoadLeg4, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, PenetrationLeg1, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, PenetrationLeg2, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, PenetrationLeg3, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, PenetrationLeg4, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, DispRig, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RigResponse, MeanDraft, LengthMeasure)
  READ_O_ARRAY_OF_OBJECT_21_VOID(RigResponse, AnchorState, AnchorState)
BSON_READ_OBJECT21_END(RigResponse)

//struct ns1__ShakerScreen
BSON_READ_OBJECT21_BEGIN(ns1, ShakerScreen)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ShakerScreen, DTimStart)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ShakerScreen, DTimEnd)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(ShakerScreen, NumDeck)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerScreen, MeshX, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerScreen, MeshY, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerScreen, Manufacturer, DataObjectReference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerScreen, Model)
  READ_O_OBJECT_21_VOID(ShakerScreen, CutPoint, LengthMeasure)
BSON_READ_OBJECT21_END(ShakerScreen)

//struct ns1__ShakerOp
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ShakerOp)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, Shaker, ComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, MdHole, MeasuredDepth)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ShakerOp, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, HoursRun, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, PcScreenCovered, AreaPerAreaMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ShakerOp, ShakerScreen, ShakerScreen)
  READ_A_PUT_SINGLE_ATTR_21_VOID(ShakerOp, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ShakerOp)

//struct ns1__Incident
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Incident)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME(Incident, DTim)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, Reporter)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Incident, NumMinorInjury)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Incident, NumMajorInjury)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Incident, NumFatality)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Incident, IsNearMiss)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, DescLocation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, DescAccident)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, RemedialActionDesc)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, CauseDesc)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, ETimLostGross, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, CostLossGross, Cost)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, ResponsibleCompany, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Incident, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Incident, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Incident)

//struct ns1__Hse
BSON_READ_OBJECT21_BEGIN(ns1, Hse)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, DaysIncFree, TimeMeasure)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastCsgPresTest)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresLastCsg, PressureMeasure)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastBopPresTest)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, NextBopPresTest)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresStdPipe, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresKellyHose, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresDiverter, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresAnnular, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresRams, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresChokeLine, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, PresChokeMan, PressureMeasure)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastFireBoatDrill)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastAbandonDrill)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastRigInspection)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastSafetyMeeting)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastSafetyInspection)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastTripDrill)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastDiverterDrill)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, LastBopDrill)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, RegAgencyInsp)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, NonComplianceIssued)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Hse, NumStopCards)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, FluidDischarged, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, VolCtgDischarged, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, VolOilCtgDischarge, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, WasteDischarged, VolumeMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Hse, Comments)
  READ_O_ARRAY_OF_OBJECT_21_VOID(Hse, Incident, Incident)
BSON_READ_OBJECT21_END(Hse)

//struct ns1__SupportCraft
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, SupportCraft)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SupportCraft, Name)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, SupportCraft, TypeSupportCraft, SupportCraftType)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(SupportCraft, DTimArrived)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(SupportCraft, DTimDeparted)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SupportCraft, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SupportCraft, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(SupportCraft, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, SupportCraft)

//struct ns1__Weather
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Weather)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME(Weather, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, Agency, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, BarometricPressure, PressureMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Weather, BeaufortScaleNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, TempSurfaceMn, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, TempSurfaceMx, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, TempWindChill, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, Tempsea, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, Visibility, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Weather, AziWave, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, HtWave, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, SignificantWave, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, MaxWave, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, PeriodWave, TimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Weather, AziWind, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, VelWind, LengthPerTimeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, TypePrecip)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, AmtPrecip, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, CoverCloud)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, CeilingCloud, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, CurrentSea, LengthPerTimeMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Weather, AziCurrentSea, PlaneAngleMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Weather, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Weather, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Weather)

//struct ns1__MudLosses
BSON_READ_OBJECT21_BEGIN(ns1, MudLosses)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostShakerSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostMudCleanerSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostPitsSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostTrippingSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostOtherSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolTotMudLostSurf, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostCircHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostCsgHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostCmtHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostBhdCsgHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostAbandonHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLosses, VolLostOtherHole, VolumeMeasure)
  READ_O_OBJECT_21_VOID(MudLosses, VolTotMudLostHole, VolumeMeasure)
BSON_READ_OBJECT21_END(MudLosses)

//struct ns1__MudVolume
BSON_READ_OBJECT21_BEGIN(ns1, MudVolume)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolTotMudStart, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudDumped, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudReceived, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudReturned, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudBuilt, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudString, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudCasing, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudHole, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolMudRiser, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MudVolume, VolTotMudEnd, VolumeMeasure)
  READ_O_OBJECT_21_VOID(MudVolume, MudLosses, MudLosses)
BSON_READ_OBJECT21_END(MudVolume)

//struct ns1__ItemVolPerUnit
BSON_READ_OBJECT21_BEGIN(ns1, ItemVolPerUnit)
  READ_O_OBJECT_21_VOID(ItemVolPerUnit, ItemVolPerUnit, VolumeMeasure)
BSON_READ_OBJECT21_END_(ItemVolPerUnit)

//struct ns1__ItemWtPerUnit
BSON_READ_OBJECT21_BEGIN(ns1, ItemWtPerUnit)
  READ_O_OBJECT_21_VOID(ItemWtPerUnit, ItemWtPerUnit, MassMeasure)
BSON_READ_OBJECT21_END_(ItemWtPerUnit)

//struct ns1__AbstractItemWtOrVolPerUnit
BSON_READ_OBJECT21_BEGIN(ns1, AbstractItemWtOrVolPerUnit)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractItemWtOrVolPerUnit, ItemVolPerUnit, ItemVolPerUnit)
  READ_O_OBJECT_21_VOID_B(ns1, AbstractItemWtOrVolPerUnit, ItemWtPerUnit, ItemWtPerUnit)
BSON_READ_OBJECT21_END(AbstractItemWtOrVolPerUnit)

//struct ns1__Inventory
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Inventory)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Inventory, Name)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Inventory, ItemWtOrVolPerUnit, AbstractItemWtOrVolPerUnit)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Inventory, PricePerUnit, Cost)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyStart)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyAdjustment)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyReceived)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyReturned)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Inventory, CostItem, Cost)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Inventory, QtyOnLocation)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Inventory, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Inventory, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Inventory)

//struct ns1__Personnel
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Personnel)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Personnel, Company, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Personnel, TypeService)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Personnel, NumPeople)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Personnel, TotalTime, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Personnel, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Personnel, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Personnel)

//struct ns1__PumpOp
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PumpOp)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PumpOp, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, Pump, ComponentReference)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PumpOp, TypeOperation, PumpOpType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, IdLiner, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, LenStroke, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, RateStroke, AngularVelocityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, Pressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, PcEfficiency, PowerPerPowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, PumpOutput, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, MdBit, MeasuredDepth)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PumpOp, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PumpOp, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PumpOp)

//struct ns1__PitVolume
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PitVolume)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, Pit, ComponentReference)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PitVolume, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, VolPit, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, DensFluid, MassPerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, DescFluid)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, VisFunnel, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PitVolume, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PitVolume, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PitVolume)

//struct ns1__StimPerforationCluster
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimPerforationCluster)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, Aliases, ObjectAlias)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, Existence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, ObjectVersionReason)
  READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, BusinessActivityHistory)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, OSDUIntegration, OSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, StimPerforationCluster, CustomData, CustomData)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimPerforationCluster, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, MdPerforatedInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, TvdPerforatedInterval, AbstractTvdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, Type)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, PerforationCount)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, Size, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, DensityPerforation, ReciprocalLengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(StimPerforationCluster, PhasingPerforation, PlaneAngleMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, FrictionFactor)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, FrictionPres, PressureMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPerforationCluster, DischargeCoefficient)
  READ_A_PUT_DEFAULT_ATTRIBUTES_21_VOID(StimPerforationCluster)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimPerforationCluster)

//struct ns1__StimPerforationClusterSet
BSON_READ_OBJECT21_BEGIN(ns1, StimPerforationClusterSet)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StimPerforationClusterSet, StimPerforationCluster, StimPerforationCluster)
BSON_READ_OBJECT21_END_(StimPerforationClusterSet)

//struct ns1__StimFetTest
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimFetTest)
  READ_A_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, StimFetTest, AnalysisMethod, StimFetTestAnalysisMethod)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimFetTest, DTimStart)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimFetTest, DTimEnd)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, EndPdlDuration, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FluidEfficiency, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureCloseDuration, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureClosePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureExtensionPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureGradient, ForcePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureLength, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, FractureWidth, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, NetPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, PdlCoef, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, PorePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, PseudoRadialPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, ResidualPermeability, PermeabilityRockMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFetTest, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimFetTest, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimFetTest)

//struct ns1__StimPressureFlowRate
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimPressureFlowRate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPressureFlowRate, Pressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPressureFlowRate, BottomholeRate, VolumePerTimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPressureFlowRate, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimPressureFlowRate, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimPressureFlowRate)

//struct ns1__StimStepTest
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimStepTest)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepTest, FractureExtensionPres, PressureMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepTest, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepTest, PresMeasurement, StimPressureFlowRate)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimStepTest, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimStepTest)

//struct ns1__StimPumpFlowBackTestStep
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimPumpFlowBackTestStep)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, FlowbackVolume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, FlowbackVolumeRate, VolumePerTimeMeasure)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, Number)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, BottomholeRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, Pres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, PipeFriction, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, EntryFriction, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, PerfFriction, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, NearWellboreFriction, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, SurfaceRate, VolumePerTimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTestStep, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimPumpFlowBackTestStep, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimPumpFlowBackTestStep)

//struct ns1__StimPumpFlowBackTest
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimPumpFlowBackTest)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, DTimEnd)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, FlowBackVolume, VolumeMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, DTimStart)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, FractureCloseDuration, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, PresCasing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, PresTubing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, FractureClosePres, PressureMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimPumpFlowBackTest, Step, StimPumpFlowBackTestStep)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimPumpFlowBackTest, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimPumpFlowBackTest)

//struct ns1__StimStepDownTest
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimStepDownTest)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, InitialShutinPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, BottomholeFluidDensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, DiameterEntryHole, LengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, PerforationCount)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, DischargeCoefficient, DimensionlessMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, EffectivePerfs)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, Step, StimPumpFlowBackTestStep)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimStepDownTest, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimStepDownTest, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimStepDownTest)

//struct ns1__StimJobDiagnosticSession
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimJobDiagnosticSession)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, Name)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, Number)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, Description)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, ChokeSize, LengthMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, DTimPumpOn)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, DTimPumpOff)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, PumpDuration, TimeMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, DTimWellShutin)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, DTimFractureClose)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, AvgBottomholeTreatmentPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, AvgBottomholeTreatmentRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, BaseFluidVol, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, BottomholeHydrostaticPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, BubblePointPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidDensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FractureClosePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FrictionPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, InitialShutinPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, PorePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, WellboreVolume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, MdSurface, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, MdBottomhole, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, MdMidPerforation, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, TvdMidPerforation, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, SurfaceTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, BottomholeTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, SurfaceFluidTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidCompressibility, IsothermalCompressibilityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, ReservoirTotalCompressibility, IsothermalCompressibilityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidNprimeFactor, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidKprimeFactor, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidSpecificHeat, SpecificHeatCapacityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidThermalConductivity, ThermalConductivityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidThermalExpansionCoefficient, VolumetricThermalExpansionMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidEfficiency, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FoamQuality, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, PercentPad, VolumePerVolumeMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, StageNumber)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, TemperatureCorrectionApplied)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, FluidEfficiencyTest, StimFetTest)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, StepRateTest, StimStepTest)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, StepDownTest, StimStepDownTest)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiagnosticSession, PumpFlowBackTest, StimPumpFlowBackTest)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimJobDiagnosticSession, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimJobDiagnosticSession)

//struct ns1__StimShutInPressure
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimShutInPressure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimShutInPressure, Pressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimShutInPressure, TimeAfterShutin, TimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimShutInPressure, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimShutInPressure, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimShutInPressure)

//struct ns1__StimEvent
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimEvent)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(StimEvent, Number)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimEvent, DTim)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimEvent, Comment)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimEvent, NumStep)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimEvent, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimEvent, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimEvent)

//struct ns1__StimMaterialQuantity
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, Density, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, Mass, MassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, MassFlowRate, MassPerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, StdVolume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, Volume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, VolumeConcentration, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, VolumetricFlowRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, Material, ComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimMaterialQuantity, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimMaterialQuantity, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimMaterialQuantity)

//struct ns2__UnitlessMeasure
BSON_READ_OBJECT21_BEGIN(ns2, UnitlessMeasure)
  READ_O_DOUBLE_ITEM_21_VOID(UnitlessMeasure)
BSON_READ_OBJECT21_END_(UnitlessMeasure)

//struct ns1__StimFluid
BSON_READ_OBJECT21_BEGIN(ns1, StimFluid)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Name)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimFluid, Kind, StimFluidKind)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimFluid, Subtype, StimFluidSubtype)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Purpose)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Description)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Supplier)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimFluid, IsKillFluid)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Volume, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Density, MassPerVolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, FluidTemp, ThermodynamicTemperatureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, GelStrength10Min, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, GelStrength10Sec, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, SpecificGravity, DimensionlessMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, Viscosity, DynamicViscosityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFluid, pH, UnitlessMeasure)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StimFluid, AdditiveConcentration, StimMaterialQuantity)
BSON_READ_OBJECT21_END(StimFluid)

//struct ns1__StimJobStep
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimJobStep)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StepName)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(StimJobStep, StepNumber)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, Kind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, Description)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStep, DTimStart)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStep, DTimEnd)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgBaseFluidQuality, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgCO2BaseFluidQuality, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgHydraulicPower, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgInternalPhaseFraction, VolumePerVolumeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgMaterialUsedRate, StimMaterialQuantity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgMaterialUseRateBottomhole, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgN2BaseFluidQuality, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgPresBottomhole, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgPropConc, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgSlurryPropConc, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgSlurryRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, AvgVolumeRateWellhead, VolumePerTimeMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStep, BallsRecovered)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStep, BallsUsed)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, BaseFluidBypassVol, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, BaseFluidVol, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndDirtyMaterialRate, VolumePerTimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndMaterialUsedRate, StimMaterialQuantity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndMaterialUsedRateBottomhole, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndPresBottomhole, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndRateSurfaceCO2, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, EndStdRateSurfaceN2, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolBase, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolCirculated, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolPumped, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolReturned, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolSlurry, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolSqueezed, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FluidVolWashed, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FractureGradientFinal, ForcePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FractureGradientInitial, ForcePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, FrictionFactor, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxHydraulicPower, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxSlurryPropConc, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxVolumeRateWellhead, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, PipeFrictionPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, PumpTime, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartDirtyMaterialRate, VolumePerTimeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartMaterialUsedRate, StimMaterialQuantity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartMaterialUsedRateBottomHole, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartPresBottomhole, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, StartProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, WellheadVol, VolumeMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaterialUsed, StimMaterialQuantity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, MaxMaterialUsedRate, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStep, Fluid, StimFluid)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimJobStep, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimJobStep)

//struct ns1__StimTubular
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimTubular)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, Type)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, Id, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, Od, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, Weight, MassPerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, TubularMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, TubularTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, VolumeFactor, VolumePerLengthMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimTubular, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimTubular, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimTubular)

//struct ns1__StimFlowPath
BSON_READ_OBJECT21_BEGIN(ns1, StimFlowPath)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, AvgPmaxPacPres, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, FrictionFactorOpenHole, DimensionlessMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, AvgPmaxWeaklinkPres, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, BreakDownPres, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, BridgePlugMD, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, FractureGradient, ForcePerVolumeMeasure)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimFlowPath, Kind, StimFlowPathType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, MaxPmaxPacPres, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, MaxPmaxWeaklinkPres, PressureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, PackerMD, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, FrictionFactorPipe, DimensionlessMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimFlowPath, TubingBottomMD, MeasuredDepth)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StimFlowPath, Tubular, StimTubular)
BSON_READ_OBJECT21_END(StimFlowPath)

//struct ns1__StimReservoirInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimReservoirInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithFormationPermeability, PermeabilityRockMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithYoungsModulus, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithPorePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithNetPayThickness, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithName)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, GrossPayMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, GrossPayThickness, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayThickness, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayPorePres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayFluidCompressibility, IsothermalCompressibilityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayFluidViscosity, DynamicViscosityMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayName)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayFormationPermeability, PermeabilityRockMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, LithPoissonsRatio, DimensionlessMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NetPayFormationPorosity, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, FormationPermeability, PermeabilityRockMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, FormationPorosity, VolumePerVolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, NameFormation)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimReservoirInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimReservoirInterval, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimReservoirInterval)

//struct ns1__StimJobDiversion
BSON_READ_OBJECT21_BEGIN(ns1, StimJobDiversion)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiversion, Contractor, DataObjectReference)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimJobDiversion, Method, StimJobDiversionMethod)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobDiversion, ToolDescription)
  READ_O_OBJECT_21_VOID(StimJobDiversion, ElementSpacing, LengthMeasure)
BSON_READ_OBJECT21_END(StimJobDiversion)

//struct ns1__StimJobStage
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimJobStage)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, Aliases, ObjectAlias)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, Citation, Citation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, Existence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, ObjectVersionReason)
  READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, BusinessActivityHistory)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, OSDUIntegration, OSDUIntegration)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, StimJobStage, CustomData, CustomData)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJobStage, ExtensionNameValue, ExtensionNameValue)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStage, Number)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FractureHeight, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, PercentPad, VolumePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, StagePerforationClusters, StimPerforationClusterSet)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgBaseFluidReturnVolumeRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgBHStaticTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgBHTreatingTemperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgBottomholePumpedVolumeRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgConductivity, LengthPerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgFractureWidth, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgHydraulicPower, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgPresAnnulus, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgPresCasing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgPresTubing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, AvgSlurryReturnVolumeRate, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, BreakDownPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ClosureDuration, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ClosurePres, PressureMeasure)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStage, DTimEnd)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStage, DTimStart)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FormationBreakLengthPerDay, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FormationName)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FormationProppantMass, MassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FractureGradientFinal, ForcePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FractureGradientInitial, ForcePerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FractureLength, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FrictionPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, HhpOrderedCO2, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, HhpOrderedFluid, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, HhpUsedCO2, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, HhpUsedFluid, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, InitialShutinPres, PressureMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxFluidVolumeRateAnnulus, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxFluidVolumeRateCasing, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxFluidVolumeRateTubing, VolumePerTimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxHydraulicPower, PowerMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxPresAnnulus, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxPresCasing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxPresSurface, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxPresTubing, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxProppantConcBottomhole, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxProppantConcSurface, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MdFormationBottom, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MdFormationTop, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MdOpenHoleBottom, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MdOpenHoleTop, MeasuredDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, NetPres, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, OpenHoleDiameter, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, OpenHoleName)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, PercentProppantPumped, VolumePerVolumeMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStage, PerfBallCount)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, PerfBallSize, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, PerfProppantConc, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ProppantHeight, LengthMeasure)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJobStage, ScreenedOut)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ScreenOutPres, PressureMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TechnologyType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TotalProppantInFormation, MassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TotalPumpTime, TimeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TotalVolume, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TvdFormationBottom, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TvdFormationTop, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TvdOpenHoleBottom, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, TvdOpenHoleTop, AbstractVerticalDepth)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, VolumeBody, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, VolumeFlush, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, VolumePad, VolumeMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, WaterSource)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, WellboreProppantMass, MassMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, PdatSession, StimJobDiagnosticSession)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ShutInPres, StimShutInPressure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, JobEvent, StimEvent)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, JobStep, StimJobStep)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaxMaterialUsageRate, StimMaterialQuantity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, MaterialUsed, StimMaterialQuantity)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, FlowPath, StimFlowPath)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, ReservoirInterval, StimReservoirInterval)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, StimStageLog, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobStage, Diversion, StimJobDiversion)
  READ_A_PUT_MULTIPLE_ATTRIBUTES_21_VOID(StimJobStage, SET_MULTIPLE_ATTRIBUTES(
    CWS_CONST_BSON_KEY("uuid"), StimJobStage->uuid,
    CWS_CONST_BSON_KEY("schemaVersion"), StimJobStage->schemaVersion,
    CWS_CONST_BSON_KEY("objectVersion"), StimJobStage->objectVersion,
    CWS_CONST_BSON_KEY("uid"), StimJobStage->uid
  ))
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimJobStage)

//struct ns1__StimAdditive
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimAdditive)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimAdditive, Kind, StimMaterialKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimAdditive, Name)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimAdditive, Supplier)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimAdditive, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimAdditive, AdditiveKind, StimAdditiveKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimAdditive, Type)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimAdditive, SupplierCode)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimAdditive, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimAdditive)

//struct ns1__ISO13503_USCORE2CrushTestData
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ISO13503_USCORE2CrushTestData)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ISO13503_USCORE2CrushTestData, Fines, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ISO13503_USCORE2CrushTestData, Stress, PressureMeasure)
  READ_A_PUT_SINGLE_ATTR_21_VOID(ISO13503_USCORE2CrushTestData, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ISO13503_USCORE2CrushTestData)

//struct ns1__ISO13503_USCORE2SieveAnalysisData
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ISO13503_USCORE2SieveAnalysisData)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ISO13503_USCORE2SieveAnalysisData, PercentRetained, MassPerMassMeasure)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(ISO13503_USCORE2SieveAnalysisData, SieveNumber)
  READ_A_PUT_SINGLE_ATTR_21_VOID(ISO13503_USCORE2SieveAnalysisData, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ISO13503_USCORE2SieveAnalysisData)

//struct ns1__StimISO13503_USCORE2Properties
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimISO13503_USCORE2Properties)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, AbsoluteDensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, ClustersPercent, DimensionlessMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, KValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, MeanParticleDiameter, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, MedianParticleDiameter, LengthMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, SpecificGravity)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, Roundness)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, AcidSolubility, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, ApparentDensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, BulkDensity, MassPerVolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, LossOnIgnition, DimensionlessMeasure)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, Sphericity)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, Turbidity)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, CrushTestData, ISO13503_USCORE2CrushTestData)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE2Properties, SieveAnalysisData, ISO13503_USCORE2SieveAnalysisData)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimISO13503_USCORE2Properties, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimISO13503_USCORE2Properties)

//struct ns1__StimISO13503_USCORE5Point
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimISO13503_USCORE5Point)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE5Point, Conductivity, PermeabilityLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE5Point, Temperature, ThermodynamicTemperatureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE5Point, Permeability, PermeabilityRockMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimISO13503_USCORE5Point, Stress, PressureMeasure)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimISO13503_USCORE5Point, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimISO13503_USCORE5Point)

//struct ns1__StimProppantAgent
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimProppantAgent)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimProppantAgent, Kind, StimMaterialKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, Name)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, Supplier)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, ExtensionNameValue, ExtensionNameValue)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, FrictionCoefficientLaminar)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, FrictionCoefficientTurbulent)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, MassAbsorptionCoefficient, AreaPerMassMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, MeshSizeHigh)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, MeshSizeLow)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, UnconfinedCompressiveStrength, PressureMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimProppantAgent, ProppantAgentKind, ProppantAgentKind)
  READ_A_ARRAY_OF_OBJECT_ALIAS_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, ISO13503_USCORE2Properties, ISO13503_2Properties, StimISO13503_USCORE2Properties)
  READ_A_ARRAY_OF_OBJECT_ALIAS_21_OR_ELSE_GOTO_RESUME(StimProppantAgent, ISO13503_USCORE5Point, ISO13503_5Point, StimISO13503_USCORE5Point)
  READ_A_PUT_SINGLE_ATTR_21_VOID(StimProppantAgent, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StimProppantAgent)

//struct ns1__StimJobMaterialCatalog
BSON_READ_OBJECT21_BEGIN(ns1, StimJobMaterialCatalog)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJobMaterialCatalog, Additives, StimAdditive)
  READ_O_ARRAY_OF_OBJECT_21_VOID(StimJobMaterialCatalog, ProppantAgents, StimProppantAgent)
BSON_READ_OBJECT21_END(StimJobMaterialCatalog)

//struct ns1__StimJobLogCatalog
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StimJobLogCatalog)
  READ_A_ARRAY_OF_OBJECT_21_VOID(StimJobLogCatalog, JobLog, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END_(ns1, StimJobLogCatalog)

//struct ns1__SurveySection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, SurveySection)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(SurveySection, Sequence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, Name)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, MdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, SurveyCompany, DataObjectReference)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, NameTool)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, TypeTool)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, ModelError, DataObjectReference)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(SurveySection, Overwrite)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, FrequencyMx, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, SurveySection, ItemState, ExistenceKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveySection, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(SurveySection, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, SurveySection)

//struct ns1__TargetSection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, TargetSection)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(TargetSection, SectNumber)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, TargetSection, TypeTargetSectionScope, TargetSectionScope)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TargetSection, LenRadius, LengthMeasure)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(TargetSection, AngleArc, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TargetSection, ThickAbove, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TargetSection, ThickBelow, LengthMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TargetSection, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TargetSection, Location, Abstract3dPosition)
  READ_A_PUT_SINGLE_ATTR_21_VOID(TargetSection, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, TargetSection)

//struct ns1__CustomOperatingRange
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CustomOperatingRange)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, Comment)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, EndInclusive)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, StartInclusive)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, Start)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, End)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CustomOperatingRange, Title)
  READ_A_UTF8_OBJECT_21_VOID(CustomOperatingRange, Uom)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CustomOperatingRange)

//struct ns1__AzimuthRange
BSON_READ_OBJECT21_BEGIN(ns1, AzimuthRange)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Comment)
  READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(AzimuthRange, EndInclusive)
  READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(AzimuthRange, StartInclusive)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Start)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(AzimuthRange, End)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Uom)
  READ_O_BOOLEAN_21_VOID(AzimuthRange, IsMagneticNorth)
BSON_READ_OBJECT21_END(AzimuthRange)

//struct ns1__AzimuthRange
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, AzimuthRange)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Comment)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(AzimuthRange, EndInclusive)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(AzimuthRange, StartInclusive)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Start)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(AzimuthRange, End)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(AzimuthRange, Uom)
  READ_A_BOOLEAN_21_VOID(AzimuthRange, IsMagneticNorth)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, AzimuthRange)

//struct ns1__PlaneAngleOperatingRange
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PlaneAngleOperatingRange)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Comment)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, EndInclusive)
  READ_A_BOOLEAN_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, StartInclusive)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Start)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, End)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Uom)
  READ_A_OBJECT_21_VOID_B(ns1, PlaneAngleOperatingRange, AzimuthRange, AzimuthRange)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PlaneAngleOperatingRange)

//struct ns1__PlaneAngleOperatingRange
BSON_READ_OBJECT21_BEGIN(ns1, PlaneAngleOperatingRange)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Comment)
  READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, EndInclusive)
  READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, StartInclusive)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Start)
  READ_O_DOUBLE_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, End)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PlaneAngleOperatingRange, Uom)
  READ_O_OBJECT_21_VOID_B(ns1, PlaneAngleOperatingRange, AzimuthRange, AzimuthRange)
BSON_READ_OBJECT21_END(PlaneAngleOperatingRange)

//struct ns1__OperatingConstraints
BSON_READ_OBJECT21_BEGIN(ns1, OperatingConstraints)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, CustomLimits, GenericMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, HorizontalEastWestMaxValue, PlaneAngleMeasureExt)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, MdRange, MdInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, TvdRange, AbstractTvdInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, PressureLimit, PressureMeasureExt)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, ThermodynamicTemperatureLimit, ThermodynamicTemperatureMeasureExt)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, CustomRange, CustomOperatingRange)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, LatitudeRange, PlaneAngleOperatingRange)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OperatingConstraints, InclinationRange, PlaneAngleOperatingRange)
  READ_O_ARRAY_OF_OBJECT_21_VOID(OperatingConstraints, AzimuthRange, AzimuthRange)
BSON_READ_OBJECT21_END(OperatingConstraints)

//struct ns1__Authorization
BSON_READ_OBJECT21_BEGIN(ns1, Authorization)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Authorization, ApprovalAuthority)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Authorization, ApprovedBy)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Authorization, ApprovedOn)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Authorization, CheckedBy)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Authorization, CheckedOn)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Authorization, RevisionComment)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Authorization, RevisionDate)
  READ_O_OBJECT_ENUM_NULLABLE_21_VOID(ns1, Authorization, Status, AuthorizationStatus)
BSON_READ_OBJECT21_END(Authorization)

//struct ns1__ErrorTermValue
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ErrorTermValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ErrorTermValue, Magnitude, GenericMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ErrorTermValue, MeanValue, GenericMeasure)
  READ_A_OBJECT_21_VOID(ErrorTermValue, ErrorTerm, DataObjectReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ErrorTermValue)

//struct ns1__ContinuousGyro
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, ContinuousGyro)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, ContinuousGyro, AxisCombination, GyroAxisCombination)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ContinuousGyro, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ContinuousGyro, GyroReinitializationDistance, LengthMeasureExt)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(ContinuousGyro, NoiseReductionFactor)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ContinuousGyro, Range, PlaneAngleOperatingRange)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(ContinuousGyro, Speed, LengthPerTimeMeasureExt)
  READ_A_OBJECT_21_VOID(ContinuousGyro, Initialization, PlaneAngleMeasureExt)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, ContinuousGyro)

//struct ns1__XyAccelerometer
BSON_READ_OBJECT21_BEGIN(ns1, XyAccelerometer)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(XyAccelerometer, CantAngle, PlaneAngleMeasureExt)
  READ_O_BOOLEAN_21_VOID(XyAccelerometer, Switching)
BSON_READ_OBJECT21_END(XyAccelerometer)

//struct ns1__StationaryGyro
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, StationaryGyro)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, StationaryGyro, AxisCombination, GyroAxisCombination)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StationaryGyro, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_VOID(StationaryGyro, Range, PlaneAngleOperatingRange)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, StationaryGyro)

//struct ns1__GyroToolConfiguration
BSON_READ_OBJECT21_BEGIN(ns1, GyroToolConfiguration)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, GyroToolConfiguration, AccelerometerAxisCombination, AccelerometerAxisCombination)
  READ_O_BOOLEAN_21_OR_ELSE_GOTO_RESUME(GyroToolConfiguration, ExternalReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GyroToolConfiguration, ContinuousGyro, ContinuousGyro)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(GyroToolConfiguration, XyAccelerometer, XyAccelerometer)
  READ_O_ARRAY_OF_OBJECT_21_VOID(GyroToolConfiguration, StationaryGyro, StationaryGyro)
BSON_READ_OBJECT21_END(GyroToolConfiguration)

//struct ns1__Scr
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Scr)
  READ_A_TIME_21_OR_ELSE_GOTO_RESUME(Scr, DTim)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Scr, Pump, ComponentReference)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Scr, TypeScr, ScrType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Scr, RateStroke, AngularVelocityMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Scr, PresRecorded, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Scr, MdBit, MeasuredDepth)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Scr, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Scr, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Scr)

//struct ns1__OSDUTubularAssemblyStatus
BSON_READ_OBJECT21_BEGIN(ns1, OSDUTubularAssemblyStatus)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(OSDUTubularAssemblyStatus, Date)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OSDUTubularAssemblyStatus, Description)
  READ_O_UTF8_OBJECT_21_VOID(OSDUTubularAssemblyStatus, StatusType)
BSON_READ_OBJECT21_END(OSDUTubularAssemblyStatus)

//struct ns1__TubularOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, TubularOSDUIntegration)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, ActiveIndicator)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, ActivityType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, ActivityTypeReasonDescription)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, ArtificialLiftType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, AssemblyBaseMd, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, AssemblyBaseReportedTvd, AbstractVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, AssemblyTopMd, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, AssemblyTopReportedTvd, AbstractVerticalDepth)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, LinerType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, OSDUTubularAssemblyStatus, OSDUTubularAssemblyStatus)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, Parent, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, PilotHoleSize, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, SeaFloorPenetrationLength, LengthMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, StringClass)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularOSDUIntegration, SuspensionPointMd, MeasuredDepth)
  READ_O_LONG64_NULLABLE_21_VOID(TubularOSDUIntegration, TubularAssemblyNumber)
BSON_READ_OBJECT21_END(TubularOSDUIntegration)

//struct ns1__TubularUmbilicalCut
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, TubularUmbilicalCut)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(TubularUmbilicalCut, CutDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularUmbilicalCut, CutMd, MeasuredDepth)
  READ_A_BOOLEAN_NULLABLE_21_VOID(TubularUmbilicalCut, IsAccidental)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, TubularUmbilicalCut)

//struct ns1__TubularUmbilicalOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, TubularUmbilicalOSDUIntegration)
  READ_O_UTF8_OBJECT_21_VOID(TubularUmbilicalOSDUIntegration, WellheadOutletKey)
BSON_READ_OBJECT21_END_(TubularUmbilicalOSDUIntegration)

//struct ns1__TubularUmbilical
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, TubularUmbilical)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularUmbilical, ConnectedTubularComponent, ComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularUmbilical, Cut, TubularUmbilicalCut)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularUmbilical, ServiceType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularUmbilical, TubularUmbilicalOSDUIntegration, TubularUmbilicalOSDUIntegration)
  READ_A_UTF8_OBJECT_21_VOID(TubularUmbilical, UmbilicalType)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, TubularUmbilical)

//struct ns1__TubularComponentOSDUIntegration
BSON_READ_OBJECT21_BEGIN(ns1, TubularComponentOSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, PackerSetDepthTvd, AbstractVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, PilotHoleSize, LengthMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, SectionType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, ShoeDepthTvd, AbstractVerticalDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentBaseMd, MeasuredDepth)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentBaseReportedTvd, AbstractVerticalDepth)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentBottomConnectionType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentBoxPinConfig)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentMaterialType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentTopConnectionType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponentOSDUIntegration, TubularComponentTopMd, MeasuredDepth)
  READ_O_OBJECT_21_VOID(TubularComponentOSDUIntegration, TubularComponentTopReportedTvd, AbstractVerticalDepth)
BSON_READ_OBJECT21_END(TubularComponentOSDUIntegration)

//struct ns1__Connection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Connection)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, Id, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, Od, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, Len, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, TypeThread)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, SizeThread, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, TensYield, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, TqYield, MomentOfForceMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Connection, Position, ConnectionPosition)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, CriticalCrossSection, AreaMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, PresLeak, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, TqMakeup, MomentOfForceMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Connection, ExtensionNameValue, ExtensionNameValue)

  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, Connection, EquipmentConnection, EquipmentConnection)

  READ_A_PUT_SINGLE_ATTR_21_VOID(Connection, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Connection)

//struct ns1__Jar
BSON_READ_OBJECT21_BEGIN(ns1, Jar)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForUpSet, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForDownSet, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForUpTrip, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForDownTrip, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForPmpOpen, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Jar, ForSealFric, ForceMeasure)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Jar, TypeJar, JarType)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Jar, JarAction, JarAction)
  READ_O_ARRAY_OF_OBJECT_21_VOID(Jar, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(Jar)

//struct ns1__Sensor
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Sensor)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Sensor, TypeMeasurement, MeasurementType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Sensor, OffsetBot, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Sensor, Comments)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Sensor, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Sensor, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Sensor)

//struct ns1__MwdTool
BSON_READ_OBJECT21_BEGIN(ns1, MwdTool)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MwdTool, FlowrateMn, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MwdTool, FlowrateMx, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MwdTool, TempMx, ThermodynamicTemperatureMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(MwdTool, IdEquv, LengthMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MwdTool, ExtensionNameValue, ExtensionNameValue)
  READ_O_ARRAY_OF_OBJECT_21_VOID(MwdTool, Sensor, Sensor)
BSON_READ_OBJECT21_END(MwdTool)

//struct ns1__Motor
BSON_READ_OBJECT21_BEGIN(ns1, Motor)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, OffsetTool, LengthMeasure)
  READ_O_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, PresLossFact)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, FlowrateMn, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, FlowrateMx, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, DiaRotorNozzle, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, ClearanceBearBox, LengthMeasure)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, LobesRotor)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, LobesStator)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Motor, TypeBearing, BearingType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, TempOpMx, ThermodynamicTemperatureMeasure)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, RotorCatcher)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, DumpValve)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, DiaNozzle, LengthMeasure)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Motor, Rotatable)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Motor, BendSettingsMn, PlaneAngleMeasure)
  READ_O_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Motor, BendSettingsMx, PlaneAngleMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Motor, Sensor, Sensor)
  READ_O_ARRAY_OF_OBJECT_21_VOID(Motor, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(Motor)

//struct ns1__Stabilizer
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Stabilizer)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, LenBlade, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, LenBladeGauge, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, OdBladeMx, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, OdBladeMn, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, DistBladeBot, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Stabilizer, ShapeBlade, BladeShapeType)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(Stabilizer, FactFric)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Stabilizer, TypeBlade, BladeType)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Stabilizer, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Stabilizer, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Stabilizer)

//struct ns1__Bend
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Bend)
  READ_A_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Bend, Angle, PlaneAngleMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Bend, DistBendBot, LengthMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Bend, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Bend, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Bend)

//struct ns1__HoleOpener
BSON_READ_OBJECT21_BEGIN(ns1, HoleOpener)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, HoleOpener, TypeHoleOpener, HoleOpenerType)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(HoleOpener, NumCutter)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(HoleOpener, Manufacturer, DataObjectReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(HoleOpener, DiaHoleOpener, LengthMeasure)
  READ_O_ARRAY_OF_OBJECT_21_VOID(HoleOpener, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(HoleOpener)

//struct ns1__BendAngle
BSON_READ_OBJECT21_BEGIN(ns1, BendAngle)
  READ_O_MEASURE_OBJECT_21_VOID_B(BendAngle, BendAngle, PlaneAngleMeasure)
BSON_READ_OBJECT21_END_(BendAngle)

//struct ns1__BendOffset
BSON_READ_OBJECT21_BEGIN(ns1, BendOffset)
  READ_O_OBJECT_21_VOID(BendOffset, BendOffset, LengthMeasure)
BSON_READ_OBJECT21_END_(BendOffset)

//struct ns1__AbstractRotarySteerableTool
BSON_READ_OBJECT21_BEGIN(ns1, AbstractRotarySteerableTool)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractRotarySteerableTool, BendAngle, BendAngle)
  READ_O_OBJECT_21_VOID_B(ns1, AbstractRotarySteerableTool, BendOffset, BendOffset)
BSON_READ_OBJECT21_END(AbstractRotarySteerableTool)

//struct ns1__RotarySteerableTool
BSON_READ_OBJECT21_BEGIN(ns1, RotarySteerableTool)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, RotarySteerableTool, DeflectionMethod, DeflectionMethod)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, HoleSizeMn, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, HoleSizeMx, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, WobMx, ForceMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, OperatingSpeed, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, SpeedMx, AngularVelocityMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, FlowRateMn, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, FlowRateMx, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, DownLinkFlowRateMn, VolumePerTimeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, DownLinkFlowRateMx, VolumePerTimeMeasure)
  READ_O_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, PressLossFact)
  READ_O_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, PadCount)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, PadLen, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, PadWidth, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, PadOffset, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, OpenPadOd, LengthMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, ClosePadOd, LengthMeasure)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(RotarySteerableTool, Tool, AbstractRotarySteerableTool)
  READ_O_ARRAY_OF_OBJECT_21_VOID(RotarySteerableTool, Sensor, Sensor)
BSON_READ_OBJECT21_END(RotarySteerableTool)

//struct ns1__Nozzle
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Nozzle)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Nozzle, Index)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Nozzle, DiaNozzle, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Nozzle, TypeNozzle, NozzleType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Nozzle, Len, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Nozzle, Orientation)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Nozzle, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Nozzle, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Nozzle)

//struct ns1__TubularComponent
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, TubularComponent)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Manufacturer, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, NominalDiameter, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, NominalWeight, MassMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Supplier, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TensStrength, PressureMeasureExt)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TubularComponentOSDUIntegration, TubularComponentOSDUIntegration)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TypeTubularComponent)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(TubularComponent, Sequence)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Description)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Id, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Od, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, OdMx, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Len, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, LenJointAv, LengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(TubularComponent, NumJointStand)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, WtPerLen, MassPerLengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(TubularComponent, Count)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Grade)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, OdDrift, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TensYield, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TqYield, MomentOfForceMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, StressFatigue, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, LenFishneck, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, IdFishneck, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, OdFishneck, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Disp, VolumeMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, PresBurst, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, PresCollapse, PressureMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, ClassService)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, WearWall, LengthPerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, ThickWall, LengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, TubularComponent, ConfigCon, BoxPinConfig)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, BendStiffness, ForcePerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, AxialStiffness, ForcePerLengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, TorsionalStiffness, ForcePerLengthMeasure)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, TubularComponent, TypeMaterial, MaterialType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, DoglegMx, AnglePerLengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Model)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, NameTag, NameTag)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, AreaNozzleFlow, AreaMeasure)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Connection, Connection)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Jar, Jar)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, MwdTool, MwdTool)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Motor, Motor)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Stabilizer, Stabilizer)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Bend, Bend)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, HoleOpener, HoleOpener)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, RotarySteerableTool, RotarySteerableTool)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, BitRecord, BitRecord)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(TubularComponent, Nozzle, Nozzle)
  READ_A_PUT_SINGLE_ATTR_21_VOID(TubularComponent, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, TubularComponent)

//struct ns1__LicensePeriod
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, LicensePeriod)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(LicensePeriod, NumLicense)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(LicensePeriod, TerminationDateTime)
  READ_A_TIME_NULLABLE_21_VOID(LicensePeriod, EffectiveDateTime)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, LicensePeriod)

//struct ns2__FacilityLifecyclePeriod
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, FacilityLifecyclePeriod)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(FacilityLifecyclePeriod, State)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(FacilityLifecyclePeriod, StartDateTime)
  READ_A_TIME_NULLABLE_21_VOID(FacilityLifecyclePeriod, EndDateTime)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, FacilityLifecyclePeriod)

//struct ns2__WellStatusPeriod
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, WellStatusPeriod)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, WellStatusPeriod, Status, WellStatus)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellStatusPeriod, StartDateTime)
  READ_A_TIME_NULLABLE_21_VOID(WellStatusPeriod, EndDateTime)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, WellStatusPeriod)

//struct ns1__WellPurposePeriod
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, WellPurposePeriod)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, WellPurposePeriod, Purpose, WellPurpose)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellPurposePeriod, StartDateTime)
  READ_A_TIME_NULLABLE_21_VOID(WellPurposePeriod, EndDateTime)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, WellPurposePeriod)

//struct ns2__FacilityOperator
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, FacilityOperator)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(FacilityOperator, BusinessAssociate, DataObjectReference)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(FacilityOperator, EffectiveDateTime)
  READ_A_TIME_NULLABLE_21_VOID(FacilityOperator, TerminationDateTime)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, FacilityOperator)

//struct ns1__BottomHoleLocation
BSON_READ_OBJECT21_BEGIN(ns1, BottomHoleLocation)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(BottomHoleLocation, Location, AbstractPosition)
  READ_O_OBJECT_21_VOID(BottomHoleLocation, OSDULocationMetadata, OSDUSpatialLocationIntegration)
BSON_READ_OBJECT21_END(BottomHoleLocation)

//struct ns1__CompletionStatusHistory
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, CompletionStatusHistory)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CompletionStatusHistory, Status, CompletionStatus)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CompletionStatusHistory, StartDate)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(CompletionStatusHistory, EndDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(CompletionStatusHistory, PerforationMdInterval, MdInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CompletionStatusHistory, Comment)
  READ_A_PUT_SINGLE_ATTR_21_VOID(CompletionStatusHistory, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, CompletionStatusHistory)

//struct ns2__DataObjectComponentReference
BSON_READ_OBJECT21_BEGIN(ns2, DataObjectComponentReference)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectComponentReference, DataObject, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DataObjectComponentReference, Component, ComponentReference)
BSON_READ_OBJECT21_END(DataObjectComponentReference)

//struct ns2__DataObjectComponentReference
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns2, DataObjectComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DataObjectComponentReference, DataObject, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_VOID(DataObjectComponentReference, Component, ComponentReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns2, DataObjectComponentReference)

//struct ns1__IntervalStatusHistory
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, IntervalStatusHistory)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, IntervalStatusHistory, PhysicalStatus, PhysicalStatus)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(IntervalStatusHistory, StartDate)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(IntervalStatusHistory, EndDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(IntervalStatusHistory, StatusMdInterval, MdInterval)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(IntervalStatusHistory, AllocationFactor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(IntervalStatusHistory, Comment)
  READ_A_PUT_SINGLE_ATTR_21_VOID(IntervalStatusHistory, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, IntervalStatusHistory)

//struct ns1__GravelPackInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, GravelPackInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, DownholeString, DataObjectComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, GravelPackMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, GravelPackTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, EventHistory, EventInfo)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, GeologyFeature, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, GeologicUnitInterpretation, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GravelPackInterval, StatusHistory, IntervalStatusHistory)
  READ_A_PUT_SINGLE_ATTR_21_VOID(GravelPackInterval, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, GravelPackInterval)

//struct ns1__OpenHoleInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, OpenHoleInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, BoreholeString, DataObjectComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, OpenHoleMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, OpenHoleTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, EventHistory, EventInfo)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, GeologyFeature, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, GeologicUnitInterpretation, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpenHoleInterval, StatusHistory, IntervalStatusHistory)
  READ_A_PUT_SINGLE_ATTR_21_VOID(OpenHoleInterval, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, OpenHoleInterval)

//struct ns1__PerforationStatusHistory
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PerforationStatusHistory)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, PerforationStatusHistory, PerforationStatus, PerforationStatus)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, StartDate)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, EndDate)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, PerforationMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, PerforationTvdInterval, AbstractTvdInterval)
  READ_A_DOUBLE_NULLABLE_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, AllocationFactor)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationStatusHistory, Comment)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PerforationStatusHistory, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PerforationStatusHistory)

//struct ns1__PerforationSetInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, PerforationSetInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, PerforationSet, DataObjectComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, PerforationSetMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, PerforationSetTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, EventHistory, EventInfo)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, GeologyFeature, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, GeologicUnitInterpretation, DataObjectReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforationSetInterval, PerforationStatusHistory, PerforationStatusHistory)
  READ_A_PUT_SINGLE_ATTR_21_VOID(PerforationSetInterval, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, PerforationSetInterval)

//struct ns1__SlotsInterval
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, SlotsInterval)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, GeologicUnitInterpretation, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, StringEquipment, DataObjectComponentReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, SlottedMdInterval, MdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, SlottedTvdInterval, AbstractTvdInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, EventHistory, EventInfo)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, GeologyFeature, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SlotsInterval, StatusHistory, IntervalStatusHistory)
  READ_A_PUT_SINGLE_ATTR_21_VOID(SlotsInterval, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, SlotsInterval)

//struct ns1__ContactIntervalSet
BSON_READ_OBJECT21_BEGIN(ns1, ContactIntervalSet)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ContactIntervalSet, GravelPackInterval, GravelPackInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ContactIntervalSet, OpenHoleInterval, OpenHoleInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ContactIntervalSet, PerforationSetInterval, PerforationSetInterval)
  READ_O_ARRAY_OF_OBJECT_21_VOID(ContactIntervalSet, SlotsInterval, SlotsInterval)
BSON_READ_OBJECT21_END(ContactIntervalSet)

//struct ns1__CuttingsGeology
BSON_READ_OBJECT21_BEGIN(ns1, CuttingsGeology)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, CuttingsGeology, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, ActiveStatus, ActiveStatusKind)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, Index, GrowingObjectIndex)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CuttingsGeology, MdInterval, MdInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeology, CuttingsGeologyInterval, CuttingsGeologyInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CuttingsGeology, Wellbore, DataObjectReference)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(CuttingsGeology)
BSON_READ_OBJECT21_END(CuttingsGeology)

//struct ns1__InterpretedGeology
BSON_READ_OBJECT21_BEGIN(ns1, InterpretedGeology)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, InterpretedGeology, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, ActiveStatus, ActiveStatusKind)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, Index, GrowingObjectIndex)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, InterpretedGeology, MdInterval, MdInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeology, InterpretedGeologyInterval, InterpretedGeologyInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(InterpretedGeology, Wellbore, DataObjectReference)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(InterpretedGeology)
BSON_READ_OBJECT21_END(InterpretedGeology)

//struct ns1__ShowEvaluation
BSON_READ_OBJECT21_BEGIN(ns1, ShowEvaluation)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, Aliases, ObjectAlias)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, Citation, Citation)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, Existence)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, ObjectVersionReason)
  READ_O_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, BusinessActivityHistory)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, OSDUIntegration, OSDUIntegration)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, ShowEvaluation, CustomData, CustomData)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, ActiveStatus, ActiveStatusKind)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, Index, GrowingObjectIndex)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ShowEvaluation, MdInterval, MdInterval)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ShowEvaluation, ShowEvaluationInterval, ShowEvaluationInterval)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(ShowEvaluation, Wellbore, DataObjectReference)
  READ_O_PUT_DEFAULT_ATTRIBUTES_21_VOID(ShowEvaluation)
BSON_READ_OBJECT21_END(ShowEvaluation)

//struct ns1__BoreholeStringReference
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, BoreholeStringReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(BoreholeStringReference, BoreholeString, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_VOID(BoreholeStringReference, StringEquipment, ComponentReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, BoreholeStringReference)

//struct ns1__DownholeStringReference
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DownholeStringReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeStringReference, DownholeString, DataObjectComponentReference)
  READ_A_ARRAY_OF_OBJECT_21_VOID(DownholeStringReference, StringEquipment, ComponentReference)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DownholeStringReference)

//struct ns1__DownholeComponentReference
BSON_READ_OBJECT21_BEGIN(ns1, DownholeComponentReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponentReference, StringEquipment, DataObjectComponentReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponentReference, PerforationSet, DataObjectComponentReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponentReference, BoreholeStringReference, BoreholeStringReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DownholeComponentReference, DownholeStringReference, DownholeStringReference)
BSON_READ_OBJECT21_END(DownholeComponentReference)

//struct ns1__AcidizeFracExtension
BSON_READ_OBJECT21_BEGIN(ns1, AcidizeFracExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(AcidizeFracExtension, StimJobID, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(AcidizeFracExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(AcidizeFracExtension)

//struct ns1__BHPExtension
BSON_READ_OBJECT21_BEGIN(ns1, BHPExtension)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BHPExtension, BHPRefID)
  READ_O_ARRAY_OF_OBJECT_21_VOID(BHPExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(BHPExtension)

//struct ns1__CementExtension
BSON_READ_OBJECT21_BEGIN(ns1, CementExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(CementExtension, CementJob, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(CementExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(CementExtension)

//struct ns1__DirectionalSurveyExtension
BSON_READ_OBJECT21_BEGIN(ns1, DirectionalSurveyExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DirectionalSurveyExtension, Trajectory, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DirectionalSurveyExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(DirectionalSurveyExtension)

//struct ns1__DownholeExtension
BSON_READ_OBJECT21_BEGIN(ns1, DownholeExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeExtension, DownholeComponent, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(DownholeExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(DownholeExtension)

//struct ns1__FluidReportExtension
BSON_READ_OBJECT21_BEGIN(ns1, FluidReportExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidReportExtension, FluidsReport, DataObjectReference)
  READ_O_ARRAY_OF_OBJECT_21_VOID(FluidReportExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(FluidReportExtension)

//struct ns1__JobExtension
BSON_READ_OBJECT21_BEGIN(ns1, JobExtension)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(JobExtension, JobReason)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(JobExtension, JobStatus)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(JobExtension, PrimaryMotivationForJob)
  READ_O_ARRAY_OF_OBJECT_21_VOID(JobExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(JobExtension)

//struct ns1__LostCirculationExtension
BSON_READ_OBJECT21_BEGIN(ns1, LostCirculationExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(LostCirculationExtension, VolumeLost, VolumeMeasure)
  READ_O_ARRAY_OF_OBJECT_21_VOID(LostCirculationExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(LostCirculationExtension)

//struct ns1__Perforating
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, Perforating)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Perforating, StageNumber)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, BottomPackerSet, MeasuredDepth)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, PerforationFluidType)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, HydrostaticPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, SurfacePressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ReservoirPressure, PressureMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, FluidDensity, MassPerMassMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, FluidLevel, MeasuredDepth)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Perforating, ConveyanceMethod, PerfConveyanceMethod)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Perforating, ShotsPlanned)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ShotsDensity, ReciprocalLengthMeasure)
  READ_A_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Perforating, ShotsMisfired)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, Orientation)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, OrientationMethod)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, PerforationCompany, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, CarrierManufacturer, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, CarrierSize, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, CarrierDescription)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ChargeManufacturer, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ChargeSize, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ChargeWeight, MassMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ChargeType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, RefLog)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, GunCentralized)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, GunSize, LengthMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, GunDesciption)
  READ_A_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Perforating, GunLeftInHole)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Perforating, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(Perforating, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, Perforating)

//struct ns1__PerforatingExtension
BSON_READ_OBJECT21_BEGIN(ns1, PerforatingExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforatingExtension, PerforationSet, DataObjectComponentReference)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(PerforatingExtension, ExtensionNameValue, ExtensionNameValue)
  READ_O_ARRAY_OF_OBJECT_21_VOID(PerforatingExtension, Perforating, Perforating)
BSON_READ_OBJECT21_END(PerforatingExtension)

//struct ns1__PressureTestExtension
BSON_READ_OBJECT21_BEGIN(ns1, PressureTestExtension)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, DiaOrificeSize, LengthMeasure)
  READ_O_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, DTimeNextTestDate)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, FlowrateRateBled, VolumePerTimeMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, IdentifierJob)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, IsSuccess)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, MaxPressureDuration, PressureMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, CirculatingPosition)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, FluidBledType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, OrientationMethod)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, TestFluidType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, TestSubType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, TestType)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, AnnulusPressure, PressureMeasure)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, WellPressureUsed)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, Str10Reference)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, UidAssembly)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, VolumeBled, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, VolumeLost, VolumeMeasure)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(PressureTestExtension, VolumePumped, VolumeMeasure)
  READ_O_ARRAY_OF_OBJECT_21_VOID(PressureTestExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(PressureTestExtension)

//struct ns1__WaitingOnExtension
BSON_READ_OBJECT21_BEGIN(ns1, WaitingOnExtension)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WaitingOnExtension, SubCategory)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WaitingOnExtension, ChargeTypeCode)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WaitingOnExtension, BusinessOrgWaitingOn)
  READ_O_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WaitingOnExtension, IsNoChargeToProducer)
  READ_O_ARRAY_OF_OBJECT_21_VOID(WaitingOnExtension, ExtensionNameValue, ExtensionNameValue)
BSON_READ_OBJECT21_END(WaitingOnExtension)

//struct ns1__AbstractEventExtension
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, AbstractEventExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, AcidizeFracExtension, AcidizeFracExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, BHPExtension, BHPExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, CementExtension, CementExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, DirectionalSurveyExtension, DirectionalSurveyExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, DownholeExtension, DownholeExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, FluidReportExtension, FluidReportExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, JobExtension, JobExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, LostCirculationExtension, LostCirculationExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, PerforatingExtension, PerforatingExtension)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, AbstractEventExtension, PressureTestExtension, PressureTestExtension)
  READ_A_OBJECT_21_VOID_B(ns1, AbstractEventExtension, WaitingOnExtension, WaitingOnExtension)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, AbstractEventExtension)

//struct ns1__MemberObject
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, MemberObject)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, MemberObject, IndexKind, DataIndexKind)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, IndexInterval, AbstractInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, MnemonicList)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, ReferenceDepth, MeasuredDepth)
  READ_A_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(MemberObject, ReferenceDateTime)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, ObjectReference, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, Sequence1, ObjectSequence)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, Sequence2, ObjectSequence)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(MemberObject, Sequence3, ObjectSequence)
  READ_A_PUT_SINGLE_ATTR_21_VOID(MemberObject, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, MemberObject)

//struct ns1__Participant
BSON_READ_OBJECT21_BEGIN(ns1, Participant)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Participant, ExtensionNameValue, ExtensionNameValue)
  READ_O_ARRAY_OF_OBJECT_21_VOID(Participant, Participant, MemberObject)
BSON_READ_OBJECT21_END(Participant)

//struct ns1__DepthRegPoint
BSON_READ_OBJECT21_BEGIN(ns1, DepthRegPoint)
  READ_O_LONG64_21_OR_ELSE_GOTO_RESUME(DepthRegPoint, X)
  READ_O_LONG64_21_VOID(DepthRegPoint, Y)
BSON_READ_OBJECT21_END(DepthRegPoint)

//struct ns1__DepthRegRectangle
BSON_READ_OBJECT21_BEGIN(ns1, DepthRegRectangle)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ul, DepthRegPoint)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ur, DepthRegPoint)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ll, DepthRegPoint)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Lr, DepthRegPoint)
  READ_O_PUT_SINGLE_ATTR_21_VOID(DepthRegRectangle, uid)
BSON_READ_OBJECT21_END(DepthRegRectangle)

//struct ns1__DepthRegRectangle
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ul, DepthRegPoint)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ur, DepthRegPoint)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Ll, DepthRegPoint)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegRectangle, Lr, DepthRegPoint)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegRectangle, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegRectangle)

//struct ns1__DepthRegLogRect
BSON_READ_OBJECT21_BEGIN(ns1, DepthRegLogRect)
  READ_O_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegLogRect, Type, LogRectangleType)
  READ_O_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, Name)
  READ_O_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, ExtensionNameValue, ExtensionNameValue)
  READ_O_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, Position, DepthRegRectangle)
  READ_O_PUT_SINGLE_ATTR_21_VOID(DepthRegLogRect, uid)
BSON_READ_OBJECT21_END(DepthRegLogRect)

//struct ns1__DepthRegLogRect
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegLogRect)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegLogRect, Type, LogRectangleType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, Name)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, ExtensionNameValue, ExtensionNameValue)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogRect, Position, DepthRegRectangle)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegLogRect, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegLogRect)

//struct ns1__DepthRegParameter
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegParameter)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, Mnemonic)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, Dictionary)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, IndexInterval, AbstractInterval)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, Value, GenericMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, Description)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegParameter, ExtensionNameValue, ExtensionNameValue)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegParameter, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegParameter)

//struct ns1__DepthRegCalibrationPoint
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegCalibrationPoint)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Index, GenericMeasure)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Track)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegCalibrationPoint, Role, CalibrationPointRole)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, CurveName)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Fraction, DimensionlessMeasure)
  READ_A_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Comment)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Parameter, DepthRegParameter)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegCalibrationPoint, Point, DepthRegPoint)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegCalibrationPoint, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegCalibrationPoint)

//struct ns1__DepthRegTrackCurve
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegTrackCurve)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, CurveInfo)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegTrackCurve, LineStyle, LineStyle)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, LineWeight)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, LineColor)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegTrackCurve, CurveScaleType, ScaleType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, CurveUnit)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, CurveLeftScaleValue)
  READ_A_DOUBLE_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, CurveRightScaleValue)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegTrackCurve, CurveBackupScaleType, BackupScaleType)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, CurveScaleRect, DepthRegRectangle)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrackCurve, Description)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegTrackCurve, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegTrackCurve)

//struct ns1__DepthRegTrack
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegTrack)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, Name)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegTrack, Type, LogTrackType)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, LeftEdge)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, RightEdge)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, TrackCurveScaleRect, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegTrack, AssociatedCurve, DepthRegTrackCurve)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegTrack, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegTrack)

//struct ns1__DepthRegLogSection
BSON_READ_ARRAY_OF_OBJECT21_BEGIN(ns1, DepthRegLogSection)
  READ_A_LONG64_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, LogSectionSequenceNumber)
  READ_A_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegLogSection, LogSectionType, LogSectionType)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, LogSectionName)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, LogMatrix)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, ScaleNumerator, LengthMeasure)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, ScaleDenominator, GenericMeasure)
  READ_A_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns2, DepthRegLogSection, IndexKind, DataIndexKind)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, IndexUom)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, IndexDatum, DataObjectReference)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, IndexInterval, AbstractInterval)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, VerticalLabel)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, VerticalRatio)
  READ_A_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, Comment)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, ExtensionNameValue, ExtensionNameValue)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, UpperCurveScaleRect, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, CalibrationPoint, DepthRegCalibrationPoint)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, WhiteSpace, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, LowerCurveScaleRect, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, LogSectionRect, DepthRegRectangle)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, Parameter, DepthRegParameter)
  READ_A_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, Track, DepthRegTrack)
  READ_A_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegLogSection, ChannelSet, DataObjectReference)
  READ_A_PUT_SINGLE_ATTR_21_VOID(DepthRegLogSection, uid)
BSON_READ_ARRAY_OF_OBJECT21_END(ns1, DepthRegLogSection)

//TODO
#define WITSML_OBJECT_BEGIN(ns, object) \
int bson_read_##object##2_1(struct soap *soap_internal) \
/* object NOT NULL */ \
{ \
  struct ns##__##object *object=(GET_INTERNAL_SOAP_WITSML_OBJECT)->object; \
  CWS_CONSTRUCT_BSON(object)

#define WITSML_OBJECT_END(object) \
  return SOAP_OK; \
\
bson_read_##object##2_1_resume: \
  CWS_BSON_FREE \
\
  return SOAP_FAULT; \
}

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, arrayTypeName) \
  READ_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, arrayTypeName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, typeName) \
  READ_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, typeName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  READ_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName) \
  READ_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns, objectParent, objectName, typeName) \
  READ_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, typeName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns, objectParent, objectName, enumFunctionName) \
  READ_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, enumFunctionName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  READ_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, enumFunctionName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, measureType) \
  READ_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, measureType, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, arrayTypeName) \
  READ_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, arrayTypeName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_OBJECT_21_VOID(objectParent, objectName, arrayTypeName) \
  READ_ARRAY_OF_OBJECT_21_VOID(GET_INTERNAL_SOAP_BSON, objectParent, objectName, arrayTypeName)

//used for ROOT Witsml object
#define READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName, typeName) \
  READ_OBJECT_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, typeName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_21_VOID(objectParent, objectName, typeName) \
  READ_OBJECT_21_VOID(GET_INTERNAL_SOAP_BSON, objectParent, objectName, typeName)

//used for ROOT Witsml object
#define READ_W_OBJECT_21_VOID_B(ns, objectParent, objectName, typeName) \
  READ_OBJECT_21_VOID_B(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, typeName)

//used for ROOT Witsml object
#define READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_TIME_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_TIME_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(objectParent) \
  READ_PUT_DEFAULT_ATTRIBUTES_21_VOID(GET_INTERNAL_SOAP_BSON, objectParent)

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  READ_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, enumFunctionName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, objectParent, objectName, enumFunctionName) \
  READ_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns, GET_INTERNAL_SOAP_BSON, objectParent, objectName, enumFunctionName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_LONG64_21_OR_ELSE_GOTO_RESUME(objectParent, objectName) \
  READ_LONG64_21_OR_ELSE_GOTO_RESUME(GET_INTERNAL_SOAP_BSON, objectParent, objectName, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(objectParent, objectName, measureType) \
  READ_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(GET_INTERNAL_SOAP_BSON, objectParent, objectName, measureType, bson_read_##objectParent##2_1)

//used for ROOT Witsml object
#define READ_W_PUT_MULTIPLE_ATTRIBUTES_21_VOID(objectParent, ...) \
  READ_PUT_MULTIPLE_ATTRIBUTES_21_VOID(GET_INTERNAL_SOAP_BSON, objectParent, __VA_ARGS__)

//struct ns1__BhaRun
WITSML_OBJECT_BEGIN(ns1, BhaRun)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, BhaRun, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, BhaRun, ExtensionNameValue, ExtensionNameValue)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, DTimStart)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, DTimStop)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, DTimStartDrilling)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, DTimStopDrilling)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, PlanDogleg, AnglePerLengthMeasure)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, ActDogleg, AnglePerLengthMeasure)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, ActDoglegMx, AnglePerLengthMeasure)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, BhaRun, BhaRunStatus, BhaStatus)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, NumBitRun)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(BhaRun, NumStringRun)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, ReasonTrip)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, ObjectiveBha)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, DrillingParams, DrillingParams)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(BhaRun, Tubular, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(BhaRun)
WITSML_OBJECT_END(BhaRun)

//struct ns1__CementJob
WITSML_OBJECT_BEGIN(ns1, CementJob)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, CementJob, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, CementJob, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, CementJob, JobType, CementJobType)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, JobConfig)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, NameCementedString)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, NameWorkString)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJob, OffshoreJob)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, MdWater, LengthMeasure)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJob, ReturnsToSeabed)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, MdPrevShoe, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, MdHole, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, TvdPrevShoe, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, MdStringSet, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, TvdStringSet, AbstractVerticalDepth)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, TypePlug)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, NameCementString)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, TypeSqueeze)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, MdSqueeze, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, ToolCompany, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, TypeTool)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(CementJob, CoilTubing)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, HoleConfig, WellboreGeometryReport)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, JobReport, CementJobReport)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, Design, CementJobDesign)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(CementJob, CementingFluid, CementingFluid)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(CementJob)
WITSML_OBJECT_END(CementJob)

//struct ns1__DownholeComponent
WITSML_OBJECT_BEGIN(ns1, DownholeComponent)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, DownholeComponent, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DownholeComponent, ExtensionNameValue, ExtensionNameValue)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DownholeComponent, StartDate)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DownholeComponent, EndDate)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, WellHead, DownholeString)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, BoreholeStringSet, BoreholeStringSet)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, DownholeStringSet, DownholeStringSet)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, EquipmentSet, EquipmentSet)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, PerforationSets, PerforationSets)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DownholeComponent, Well, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(DownholeComponent)
WITSML_OBJECT_END(DownholeComponent)

//struct ns1__DrillReport
WITSML_OBJECT_BEGIN(ns1, DrillReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, DrillReport, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DrillReport, ExtensionNameValue, ExtensionNameValue)
  READ_W_TIME_21_OR_ELSE_GOTO_RESUME(DrillReport, DTimStart)
  READ_W_TIME_21_OR_ELSE_GOTO_RESUME(DrillReport, DTimEnd)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DrillReport, VersionKind, OpsReportVersion)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(DrillReport, CreateDate)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, Datum, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, BitRecord, BitRecord)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, WellboreInfo, DrillReportWellboreInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, StatusInfo, DrillReportStatusInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, Fluid, Fluid)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, PorePressure, DrillReportPorePressure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, ExtendedReport, TimestampedCommentString)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, SurveyStations, DrillReportSurveyStationReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, DrillActivity, DrillActivity)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, LogInfo, DrillReportLogInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, CoreInfo, DrillReportCoreInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, WellTestInfo, DrillReportWellTestInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, FormTestInfo, DrillReportFormTestInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, LithShowInfo, DrillReportLithShowInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, EquipFailureInfo, DrillReportEquipFailureInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, ControlIncidentInfo, DrillReportControlIncidentInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, StratInfo, DrillReportStratInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, PerfInfo, DrillReportPerfInfo)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, GasReadingInfo, DrillReportGasReadingInfo)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DrillReport, Wellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(DrillReport)
WITSML_OBJECT_END(DrillReport)

//struct ns1__FluidsReport
WITSML_OBJECT_BEGIN(ns1, FluidsReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, FluidsReport, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, FluidsReport, ExtensionNameValue, ExtensionNameValue)
  READ_W_TIME_21_OR_ELSE_GOTO_RESUME(FluidsReport, DTim)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidsReport, Md, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidsReport, Tvd, AbstractVerticalDepth)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(FluidsReport, NumReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidsReport, Fluid, Fluid)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(FluidsReport, Wellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(FluidsReport)
WITSML_OBJECT_END(FluidsReport)

//struct ns1__MudLogReport
WITSML_OBJECT_BEGIN(ns1, MudLogReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, MudLogReport, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, Index, GrowingObjectIndex)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, MudLogReport, MdInterval, MdInterval)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, MudLogCompany, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, MudLogEngineers)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, MudLogGeologists)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, WellboreGeology, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, RelatedLog, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, MudlogReportInterval, MudlogReportInterval)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(MudLogReport, Parameter, MudLogParameter)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(MudLogReport)
WITSML_OBJECT_END(MudLogReport)

//struct ns1__OpsReport
WITSML_OBJECT_BEGIN(ns1, OpsReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, OpsReport, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, OpsReport, ExtensionNameValue, ExtensionNameValue)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ConditionHole)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, CostDay, Cost)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, CostDayMud, Cost)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DiaCsgLast, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DiaHole, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistDrill, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistDrillRot, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistDrillSlid, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistHold, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistReam, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DistSteering, LengthMeasure)
  READ_W_TIME_21_OR_ELSE_GOTO_RESUME(OpsReport, DTim)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Engineer)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimCirc, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimDrill, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimDrillRot, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimDrillSlid, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimHold, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimLoc, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimReam, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimSpud, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimStart, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ETimSteering, TimeMeasure)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Forecast24Hr)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Geologist)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Lithology)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Maasp, PressureMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, MdCsgLast, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, MdPlanned, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, MdReport, MeasuredDepth)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, NameFormation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, NumAFE)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(OpsReport, NumContract)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(OpsReport, NumOperator)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(OpsReport, NumPob)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(OpsReport, NumService)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, PresKickTol, PressureMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, PresLotEmw, MassPerVolumeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, RigUtilization, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, RopAv, LengthPerTimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, RopCurrent, LengthPerTimeMeasure)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, StatusCurrent)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Sum24Hr)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Supervisor)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Tubular, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, TvdCsgLast, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, TvdLot, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, TvdReport, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, VolKickTol, VolumeMeasure)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Activity, DrillActivity)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DrillingParams, DrillingParams)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, WbGeometry, WellboreGeometryReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, DayCost, DayCost)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, TrajectoryStations, TrajectoryReport)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Fluid, Fluid)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Scr, Scr)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, RigResponse, RigResponse)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, ShakerOp, ShakerOp)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Hse, Hse)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, SupportCraft, SupportCraft)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Weather, Weather)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, MudVolume, MudVolume)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, BulkInventory, Inventory)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, MudInventory, Inventory)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, Personnel, Personnel)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, PumpOp, PumpOp)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(OpsReport, PitVolume, PitVolume)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(OpsReport)
WITSML_OBJECT_END(OpsReport)

//struct ns1__Rig
WITSML_OBJECT_BEGIN(ns1, Rig)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Rig, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Rig, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, Owner, DataObjectReference)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Rig, TypeRig, RigType)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, Manufacturer, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, YearEntService)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, ClassRig)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, Approvals)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, Registration)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, TelNumber)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, FaxNumber)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, EmailAddress)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, NameContact)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, RatingDrillDepth, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, RatingWaterDepth, LengthMeasure)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Rig, IsOffshore)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Rig, TypeDerrick, DerrickType)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, RatingDerrick, ForceMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, HtDerrick, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Rig, CapWindDerrick, LengthPerTimeMeasure)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Rig, NumCranes)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Rig)
WITSML_OBJECT_END(Rig)

//struct ns1__Risk
WITSML_OBJECT_BEGIN(ns1, Risk)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Risk, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Risk, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Risk, Type, RiskType)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Risk, Category, RiskCategory)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Risk, SubCategory, RiskSubCategory)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, ExtendCategory)
  READ_W_ARRAY_OF_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Risk, AffectedPersonnel, RiskAffectedPersonnel)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Risk, DTimStart)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Risk, DTimEnd)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, MdHoleStart, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, MdHoleEnd, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, TvdHoleStart, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, TvdHoleEnd, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, MdBitStart, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, MdBitEnd, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, DiaHole, LengthMeasure)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Risk, SeverityLevel)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Risk, ProbabilityLevel)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Summary)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Details)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Identification)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Contingency)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Mitigation)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, ObjectReference, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Risk, Wellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Risk)
WITSML_OBJECT_END(Risk)

//struct ns1__StimJob
WITSML_OBJECT_BEGIN(ns1, StimJob)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, StimJob, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, StimJob, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, AvgJobPres, PressureMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, BottomholeStaticTemperature, ThermodynamicTemperatureMeasure)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, CustomerName)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJob, DTimArrival)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJob, DTimEnd)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJob, DTimStart)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, FlowBackPres, PressureMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, FlowBackRate, VolumePerTimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, FlowBackVolume, VolumeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, FluidEfficiency, VolumePerVolumeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, HhpOrdered, PowerMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, HhpUsed, PowerMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, JobPerforationClusters, StimPerforationClusterSet)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, Kind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, MaxFluidRate, VolumePerTimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, MaxJobPres, PressureMeasure)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, StimJob, PIDXCommodityCode, PIDXCommodityCode)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, ServiceCompany, DataObjectReference)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(StimJob, StageCount)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, Supervisor)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, TotalJobVolume, VolumeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, TotalProppantInFormation, MassMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, TotalProppantUsed, MassMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, TotalPumpTime, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, TreatingBottomholeTemperature, ThermodynamicTemperatureMeasure)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, JobStage, StimJobStage)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, MaterialUsed, StimMaterialQuantity)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, MaterialCatalog, StimJobMaterialCatalog)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(StimJob, LogCatalog, StimJobLogCatalog)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(StimJob)
WITSML_OBJECT_END(StimJob)

//struct ns1__SurveyProgram
WITSML_OBJECT_BEGIN(ns1, SurveyProgram)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, SurveyProgram, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, SurveyProgram, ExtensionNameValue, ExtensionNameValue)
  READ_W_LONG64_21_OR_ELSE_GOTO_RESUME(SurveyProgram, SurveyVer)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveyProgram, Engineer)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveyProgram, Final)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveyProgram, SurveySection, SurveySection)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(SurveyProgram, Wellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(SurveyProgram)
WITSML_OBJECT_END(SurveyProgram)

//struct ns1__Target
WITSML_OBJECT_BEGIN(ns1, Target)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Target, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Target, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispNsCenter, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispEwCenter, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, Tvd, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispNsOffset, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispEwOffset, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, ThickAbove, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, ThickBelow, LengthMeasure)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Target, Dip, PlaneAngleMeasure)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Target, Strike, PlaneAngleMeasure)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME_B(Target, Rotation, PlaneAngleMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, LenMajorAxis, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, WidMinorAxis, LengthMeasure)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, TargetScope)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispNsSectOrig, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, DispEwSectOrig, LengthMeasure)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, Target, AziRef, NorthReferenceKind)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, CatTarg)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, Location, Abstract3dPosition)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, TargetSection, TargetSection)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Target, ParentTarget, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Target)
WITSML_OBJECT_END(Target)

//struct ns1__ToolErrorModel
WITSML_OBJECT_BEGIN(ns1, ToolErrorModel)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, ToolErrorModel, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, ToolErrorModel, ExtensionNameValue, ExtensionNameValue)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, Application)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, Source)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, ToolErrorModel, ToolKind, ToolKind)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, ToolSubKind)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, OperatingCondition)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, SurveyRunDateEnd)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, CorrectionConsidered)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, SurveyRunDateStart)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, ToolErrorModel, MisalignmentMode, MisalignmentMode)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, OperatingConstraints, OperatingConstraints)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, Authorization, Authorization)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, ErrorTermValue, ErrorTermValue)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, GyroToolConfiguration, GyroToolConfiguration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(ToolErrorModel, Replaces, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(ToolErrorModel)
WITSML_OBJECT_END(ToolErrorModel)

//struct ns1__Trajectory
WITSML_OBJECT_BEGIN(ns1, Trajectory)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Trajectory, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, Index, GrowingObjectIndex)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Trajectory, MdInterval, MdInterval)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, UniqueIdentifier)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, ServiceCompany, DataObjectReference)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Trajectory, DTimTrajEnd)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Trajectory, DTimTrajStart)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Trajectory, Definitive)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Trajectory, Memory)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Trajectory, FinalTraj)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, DefaultMdDatum, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, DefaultTvdDatum, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, ParentTrajectory, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, TrajectoryStation, TrajectoryStation)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, Wellbore, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, SourceTrajectory, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, SurveyProgram, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, AcquisitionRemark)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, MagDeclUsed, PlaneAngleMeasureExt)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, MdMaxExtrapolated, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, MdMaxMeasured, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, MdTieOn, MeasuredDepth)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, NominalCalcAlgorithm)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, NominalTypeSurveyTool)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, NominalTypeTrajStation)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, TrajectoryOSDUIntegration, TrajectoryOSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, GridConUsed, PlaneAngleMeasureExt)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, GridScaleFactorUsed, LengthPerLengthMeasureExt)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, AziVertSect, PlaneAngleMeasureExt)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, DispNsVertSectOrig, LengthMeasureExt)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Trajectory, DispEwVertSectOrig, LengthMeasureExt)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, Trajectory, AziRef, NorthReferenceKind)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Trajectory)
WITSML_OBJECT_END(Trajectory)

//struct ns1__Tubular
WITSML_OBJECT_BEGIN(ns1, Tubular)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Tubular, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Tubular, ExtensionNameValue, ExtensionNameValue)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Tubular, MixedString)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, NominalDiameter, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, TubularOSDUIntegration, TubularOSDUIntegration)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, TubularUmbilical, TubularUmbilical)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME(ns1, Tubular, TypeTubularAssy, TubularAssembly)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Tubular, ValveFloat)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Tubular, SourceNuclear)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, DiaHoleAssy, LengthMeasure)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, TubularComponent, TubularComponent)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Tubular, Wellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Tubular)
WITSML_OBJECT_END(Tubular)

//struct ns1__Well
WITSML_OBJECT_BEGIN(ns1, Well)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Well, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Well, ActiveStatus, ActiveStatusKind)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, UniqueIdentifier)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, NameLegal)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, NumGovt)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, NumAPI)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, OperatingEnvironment)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, TimeZone)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Basin)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Play)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Prospect)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Field)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Country)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, State)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, County)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Region)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, District)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, NumLicense)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Well, DTimLicense)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, LicenseHistory, LicensePeriod)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Block)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, InterestType)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, PcInterest, DimensionlessMeasure)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, SlotName)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, LifeCycleState)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, LifeCycleHistory, FacilityLifecyclePeriod)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, Operator, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, OperatorDiv)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, OriginalOperator, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, OperatorHistory, FacilityOperator)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, Well, StatusWell, WellStatus)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, StatusHistory, WellStatusPeriod)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Well, PurposeWell, WellPurpose)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, PurposeHistory, WellPurposePeriod)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Well, FluidWell, WellFluid)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Well, DirectionWell, WellDirection)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Well, DTimSpud)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Well, DTimPa)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, WaterDepth, LengthMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, InformationalGeographicLocationWGS84, Geographic2dPosition)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, InformationalProjectedLocation, Projected2dPosition)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, DataSourceOrganization, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, WellheadElevation, AbstractElevation)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, GroundElevation, AbstractElevation)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Well, WellSurfaceLocation, AbstractPosition)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Well)
WITSML_OBJECT_END(Well)

//struct ns1__Wellbore
WITSML_OBJECT_BEGIN(ns1, Wellbore)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Wellbore, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Wellbore, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, GeographicBottomHoleLocation, BottomHoleLocation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, Number)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, ProjectedBottomHoleLocation, BottomHoleLocation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, SuffixAPI)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, NumLicense)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, LicenseHistory, LicensePeriod)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, NumGovt)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, UniqueIdentifier)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, SlotName)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, Operator, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, OriginalOperator, DataObjectReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, OperatorHistory, FacilityOperator)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, DataSourceOrganization, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, LifecycleState)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, LifecycleHistory, FacilityLifecyclePeriod)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns2, Wellbore, StatusWellbore, WellStatus)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, StatusHistory, WellStatusPeriod)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Wellbore, PurposeWellbore, WellPurpose)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, PurposeHistory, WellPurposePeriod)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Wellbore, TypeWellbore, WellboreType)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Wellbore, Shape, WellboreShape)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Wellbore, FluidWellbore, WellFluid)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(Wellbore, DTimKickoff)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(Wellbore, AchievedTD)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, Md, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, Tvd, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, MdBit, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TvdBit, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, MdKickoff, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TvdKickoff, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, MdPlanned, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TvdPlanned, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, MdSubSeaPlanned, MeasuredDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TvdSubSeaPlanned, AbstractVerticalDepth)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, DayTarget, TimeMeasure)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TargetFormation)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, TargetGeologicUnitInterpretation, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, DefaultMdDatum, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, DefaultTvdDatum, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, Well, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Wellbore, ParentWellbore, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Wellbore)
WITSML_OBJECT_END(Wellbore)

//struct ns1__WellboreCompletion
WITSML_OBJECT_BEGIN(ns1, WellboreCompletion)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, WellboreCompletion, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreCompletion, ExtensionNameValue, ExtensionNameValue)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, WellboreCompletionNo)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, WellboreCompletionAlias)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, EventHistory, EventInfo)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, WellboreCompletionDate)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, SuffixAPI)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, CompletionMdInterval, MdInterval)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, CompletionTvdInterval, AbstractTvdInterval)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, WellboreCompletion, CurrentStatus, CompletionStatus)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, StatusDate)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, StatusHistory, CompletionStatusHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, ContactIntervalSet, ContactIntervalSet)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, ReferenceWellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreCompletion, WellCompletion, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(WellboreCompletion)
WITSML_OBJECT_END(WellboreCompletion)

//struct ns1__WellboreGeology
WITSML_OBJECT_BEGIN(ns1, WellboreGeology)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, WellboreGeology, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, WellboreGeology, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeology, MdInterval, MdInterval)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeology, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeology, CuttingsGeology, CuttingsGeology)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeology, InterpretedGeology, InterpretedGeology)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellboreGeology, ShowEvaluation, ShowEvaluation)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(WellboreGeology)
WITSML_OBJECT_END(WellboreGeology)

//struct ns1__WellCMLedger
WITSML_OBJECT_BEGIN(ns1, WellCMLedger)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, WellCMLedger, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCMLedger, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, ParentEvent, DataObjectReference)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, DTimStart)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, DTimEnd)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Duration, TimeMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, MdInterval, MdInterval)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, EventOrder)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Rig, DataObjectReference)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, WellCMLedger, ActivityCode, DrillActivityCode)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Type, EventType)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, IsPlan)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, WorkOrderID)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, BusinessAssociate, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, ResponsiblePerson)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Contact)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Nonproductive)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Trouble)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, PreventiveMaintenance)
  READ_W_BOOLEAN_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Unplanned)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Phase)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Comment)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Description)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, DownholeComponentReference, DownholeComponentReference)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, EventExtension, AbstractEventExtension)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Cost, DayCost)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Participant, Participant)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCMLedger, EventType, EventType)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(WellCMLedger)
WITSML_OBJECT_END(WellCMLedger)

//struct ns1__WellCompletion
WITSML_OBJECT_BEGIN(ns1, WellCompletion)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, WellCompletion, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, WellCompletion, ExtensionNameValue, ExtensionNameValue)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, FieldID)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, FieldCode)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, FieldType)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCompletion, EffectiveDate)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCompletion, ExpiredDate)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, E_USCOREP_USCORERightsID)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, WellCompletion, CurrentStatus, CompletionStatus)
  READ_W_TIME_NULLABLE_21_OR_ELSE_GOTO_RESUME(WellCompletion, StatusDate)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, StatusHistory, CompletionStatusHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(WellCompletion, Well, DataObjectReference)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(WellCompletion)
WITSML_OBJECT_END(WellCompletion)

//struct ns1__DepthRegImage
WITSML_OBJECT_BEGIN(ns1, DepthRegImage)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, DepthRegImage, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, DepthRegImage, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegImage, FileNameType, FileNameType)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegImage, Mimetype, MimeType)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, FileName)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, FileSize, DigitalStorageMeasure)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, DepthRegImage, Checksum, MessageDigestType)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DepthRegImage, ImagePixelWidth)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(DepthRegImage, ImagePixelHeight)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, Version)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, ImageBoundary, DepthRegRectangle)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, HeaderSection, DepthRegLogRect)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, LogSection, DepthRegLogSection)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, AlternateSection, DepthRegLogRect)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(DepthRegImage, Wellbore, DataObjectReference)
  READ_W_PUT_MULTIPLE_ATTRIBUTES_21_VOID(DepthRegImage, SET_MULTIPLE_ATTRIBUTES(
    CWS_CONST_BSON_KEY("uuid"), DepthRegImage->uuid,
    CWS_CONST_BSON_KEY("schemaVersion"), DepthRegImage->schemaVersion,
    CWS_CONST_BSON_KEY("objectVersion"), DepthRegImage->objectVersion,
    CWS_CONST_BSON_KEY("uid"), DepthRegImage->uid
  ))
WITSML_OBJECT_END(DepthRegImage)

//struct ns1__Log
WITSML_OBJECT_BEGIN(ns1, Log)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, Aliases, ObjectAlias)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, Citation, Citation)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, Existence)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, ObjectVersionReason)
  READ_W_ARRAY_OF_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, BusinessActivityHistory)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, OSDUIntegration, OSDUIntegration)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_C(ns2, Log, CustomData, CustomData)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, ExtensionNameValue, ExtensionNameValue)
  READ_W_OBJECT_ENUM_21_OR_ELSE_GOTO_RESUME_B(ns2, Log, ActiveStatus, ActiveStatusKind)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Log, ChannelState, ChannelState)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, RunNumber)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, PassNumber)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, PassDescription)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, PassDetail, PassDetail)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, PrimaryIndexInterval, AbstractInterval)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LoggingCompany, DataObjectReference)
  READ_W_LONG64_NULLABLE_21_OR_ELSE_GOTO_RESUME(Log, LoggingCompanyCode)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LoggingToolKind, DataObjectReference)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LoggingToolClass)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LoggingToolClassLongName)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LoggingServicePeriod, DateTimeInterval)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Log, Derivation, ChannelDerivation)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Log, LoggingMethod, LoggingMethod)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, NominalHoleSize, LengthMeasureExt)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, MudClass)
  READ_W_UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, MudSubClass)
  READ_W_OBJECT_ENUM_NULLABLE_21_OR_ELSE_GOTO_RESUME(ns1, Log, HoleLoggingStatus, HoleLoggingStatus)
  READ_W_MEASURE_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, NominalSamplingInterval, GenericMeasure)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, LogOSDUIntegration, LogOSDUIntegration)
  READ_W_ARRAY_OF_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, ChannelSet, ChannelSet)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME(Log, Wellbore, DataObjectReference)
  READ_W_OBJECT_21_OR_ELSE_GOTO_RESUME_B(ns1, Log, PPFGLog, PPFGLog)
  READ_W_PUT_DEFAULT_ATTRIBUTES_21_VOID(Log)
WITSML_OBJECT_END(Log)

#undef _CWS_NULLABLE_XSD_SIGNED_SHORT
#undef _CWS_NULLABLE_XSD_STR
#undef KEY_USCORE_VALUE
#undef KEY_USCORE_ATTRIBUTES
#undef SET_MULTIPLE_ATTRIBUTES

