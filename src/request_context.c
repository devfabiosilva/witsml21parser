#define _GNU_SOURCE
#include "soapH.h"
#include <request_context.h>
#include <cws_soap.h>
#include <cws_errors.h>
#include <requests.h>
#include <sys/resource.h>

#ifdef WITH_STATISTICS
static
long int getResourceSize() {
  struct rusage r_usage;

#ifdef JNI_RUSAGE_CHILDREN
  if (getrusage(RUSAGE_THREAD, &r_usage)==0)
#else
  if (getrusage(RUSAGE_SELF, &r_usage)==0)
#endif
    return (long int)r_usage.ru_maxrss;

  return -1;
}
#endif
/*
static
char *request_parser_util(CWS_CONFIG *config)
{
  char *p;

  #define PARSE_TOTAL_LEN (MESSAGE_REQUEST_BEGIN_LEN + MESSAGE_REQUEST_END_LEN)

  if ((config->xmlSoapLen=(PARSE_TOTAL_LEN + config->xmlLen))<SOAP_MAXALLOCSIZE) {
    if ((config->xmlSoap=soap_malloc(config->soap_internal, config->xmlSoapLen+1))) {

      p=&((char *)memcpy(config->xmlSoap, MESSAGE_REQUEST_BEGIN, MESSAGE_REQUEST_BEGIN_LEN))[MESSAGE_REQUEST_BEGIN_LEN];
      p=&((char *)memcpy(p, config->xmlIn, config->xmlLen))[config->xmlLen];
      p=&((char *)memcpy(p, MESSAGE_REQUEST_END, MESSAGE_REQUEST_END_LEN))[MESSAGE_REQUEST_END_LEN];
      *p=0;

      return config->xmlSoap;
    }
  }

  #undef PARSE_TOTAL_LEN

  config->xmlSoapLen=0;
  return NULL;
}
*/
static
char *request_parser_util(CWS_CONFIG *config)
{
  char
    *p,
    *xmlSoapTmp;
  size_t soapLenTmp;

  #define PARSE_TOTAL_LEN (MESSAGE_REQUEST_BEGIN_LEN + MESSAGE_REQUEST_END_LEN)

  soapLenTmp=PARSE_TOTAL_LEN + config->xmlLen;

  if (config->xmlSoap) {

    if (config->xmlSoapSize > soapLenTmp)
      goto request_parser_util_copy;

    if ((xmlSoapTmp=cws_realloc((void *)config->xmlSoap, ++soapLenTmp))) {
      config->xmlSoap=xmlSoapTmp;
      config->xmlSoapSize=soapLenTmp;

      goto request_parser_util_copy;
    }

    config->xmlSoapSize=0;
    config->xmlSoap[0]=0;

  } else if ((config->xmlSoap=(char *)cws_malloc(config->xmlSoapSize=(soapLenTmp+1)))) {

request_parser_util_copy:
    p=&((char *)memcpy(config->xmlSoap, MESSAGE_REQUEST_BEGIN, MESSAGE_REQUEST_BEGIN_LEN))[MESSAGE_REQUEST_BEGIN_LEN];
    p=&((char *)memcpy(p, config->xmlIn, config->xmlLen))[config->xmlLen];
    p=&((char *)memcpy(p, MESSAGE_REQUEST_END, MESSAGE_REQUEST_END_LEN))[MESSAGE_REQUEST_END_LEN];
    *p=0;

    return config->xmlSoap;
  } else
    config->xmlSoapSize=0;

  #undef PARSE_TOTAL_LEN

  config->xmlSoapLen=0;
  return NULL;
}

char *cws_parse_XML_soap_envelope(struct soap *soap_internal, char *xmlIn, size_t xmlInLen/*, unsigned int action*/)
{
  CWS_CONFIG *config=(CWS_CONFIG *)soap_internal->user;

  if (((config->xmlIn=xmlIn)==NULL)||((config->xmlLen=xmlInLen)==0)) {
    cws_set_soap_fault_util(
      soap_internal, SERVER_EMPTY_XML_OR_NULL_ERROR,
      SERVER_EMPTY_XML_OR_NULL_ERROR_MESSSAGE, sizeof(SERVER_EMPTY_XML_OR_NULL_ERROR_MESSSAGE)-1,
      -1
    );
    return NULL;
  }

  if (!(config->xmlSoap=request_parser_util(config)))
    cws_set_soap_fault_util(
      soap_internal, SERVER_FAULT_OUT_OF_MEMORY_ERROR,
      SERVER_FAULT_OUT_OF_MEMORY_ERROR_MESSAGE, sizeof(SERVER_FAULT_OUT_OF_MEMORY_ERROR_MESSAGE)-1,
      -1
    );
  return config->xmlSoap;
}

#define CWS_CONFIG_SZ sizeof(CWS_CONFIG)

CWS_CONFIG *cws_config_new(const char *config_name)
{
#ifdef WITH_STATISTICS
  long int r=getResourceSize();
#endif
  CWS_CONFIG *ret=(CWS_CONFIG *)malloc(CWS_CONFIG_SZ);

  if (ret) {
    memset((void *)ret, 0, CWS_CONFIG_SZ);

#ifndef SOAP_DEBUG
    ret->internalInitFlag=SOAP_C_UTFSTRING|SOAP_XML_IGNORENS|SOAP_XML_STRICT|SOAP_ENC_PLAIN;
#else
    ret->internalInitFlag=SOAP_C_UTFSTRING|SOAP_XML_INDENT|SOAP_XML_IGNORENS|SOAP_XML_STRICT;//|SOAP_ENC_PLAIN;
#endif

#ifdef WITH_STATISTICS
    ret->initial_resource_size=r;
#endif
    snprintf(ret->instance_name, sizeof(ret->instance_name), "%s - (%p)", (config_name)?config_name:"CWS_CONFIG_INIT", (void *)ret);
  }

  return ret;
}

#undef CWS_CONFIG_SZ

#define _FREE_CONFIG_PARTIAL \
cws_bson_free(&(*cws_config)->object); \
\
if ((*cws_config)->c_bson_serialized.bson) { \
  bson_free((void *)((*cws_config)->c_bson_serialized.bson)); \
  (*cws_config)->c_bson_serialized.bson=NULL; \
  (*cws_config)->c_bson_serialized.bson_size=0; \
} \
\
if ((*cws_config)->c_json_str.json) { \
  bson_free((*cws_config)->c_json_str.json); \
  (*cws_config)->c_json_str.json=NULL; \
  (*cws_config)->c_json_str.json_len=0; \
}

#define _FREE_CONFIG_PARTIAL_DEBUG \
printf("\nBegin freeing BSON object %p ...", (*cws_config)->object); \
cws_bson_free(&(*cws_config)->object); \
printf("\nFinishing freeing BSON object"); \
\
if ((*cws_config)->c_bson_serialized.bson) { \
  printf("\nBegin freeing BSON (serialized) object %p ...", (*cws_config)->c_bson_serialized.bson); \
  bson_free((void *)((*cws_config)->c_bson_serialized.bson)); \
  (*cws_config)->c_bson_serialized.bson=NULL; \
  (*cws_config)->c_bson_serialized.bson_size=0; \
  printf("\nFinishing freeing BSON (serialized) object"); \
} \
\
if ((*cws_config)->c_json_str.json) { \
  printf("\nBegin freeing JSON string object %p ...", (*cws_config)->c_json_str.json); \
  bson_free((*cws_config)->c_json_str.json); \
  (*cws_config)->c_json_str.json=NULL; \
  (*cws_config)->c_json_str.json_len=0; \
  printf("\nFinishing freeing JSON string object"); \
}

void cws_recycle_config(CWS_CONFIG *cws_config)
{
  cws_bson_free(&cws_config->object);

  if (cws_config->c_bson_serialized.bson) {
    bson_free((void *)cws_config->c_bson_serialized.bson);
    cws_config->c_bson_serialized.bson=NULL;
    cws_config->c_bson_serialized.bson_size=0;
  }

  if (cws_config->c_json_str.json) {
    bson_free(cws_config->c_json_str.json);
    cws_config->c_json_str.json=NULL;
    cws_config->c_json_str.json_len=0;
  }

  cws_config->xmlIn="";
  cws_config->xmlLen=0;
  cws_config->object_type=TYPE_None;
  cws_config->object_name=NULL;

}

void cws_config_free(CWS_CONFIG **cws_config)
{
  if (*cws_config) {
#ifndef SOAP_DEBUG

    if ((*cws_config)->xmlSoap) {
      free((void *)(*cws_config)->xmlSoap);
      (*cws_config)->xmlSoap=NULL;
      (*cws_config)->xmlSoapSize=0;
      (*cws_config)->xmlSoapLen=0;
    }

    _FREE_CONFIG_PARTIAL

    free((void *)*cws_config);

#else

    printf("\nBegin freeing xmlSoap object %p ...", (*cws_config)->xmlSoap);
    if ((*cws_config)->xmlSoap) {
      free((void *)(*cws_config)->xmlSoap);
      (*cws_config)->xmlSoap=NULL;
      (*cws_config)->xmlSoapSize=0;
      (*cws_config)->xmlSoapLen=0;
    }

    _FREE_CONFIG_PARTIAL_DEBUG

    printf(
      "\nBegin freeing config instance name %s ...\n",
      (*cws_config)->instance_name
    );
    free((void *)*cws_config);
    printf("\nFinished freeing cws_config\n");
#endif
    *cws_config=NULL;
  }
}

#undef _FREE_CONFIG_PARTIAL_DEBUG
#undef _FREE_CONFIG_PARTIAL

const char *
cws_get_witsml_version_str_from_id(enum witsml_version_e id)
{
  switch (id) {
    case VERSION_2_1: return "2.1";
    case VERSION_2_0: return "2.0";
    case VERSION_1_4_1_1: return "1.4.1.1";
    case VERSION_1_4_1: return "1.4.1";
    case VERSION_1_3_1_1: return "1.3.1.1";
    default: return "UNKNOWN_WITSML_VERSION";
  }
}

struct c_json_str_t *getJson(struct soap *soap_internal)
{
  CWS_CONFIG *config=(CWS_CONFIG *)soap_internal->user;
  struct c_json_str_t *c_json_str=&(config)->c_json_str;

  if (c_json_str->json)
    return c_json_str;

  if ((config->object!=NULL)&&((c_json_str->json=bson_as_relaxed_extended_json((const bson_t *)config->object, &c_json_str->json_len))!=NULL))
    return c_json_str;

  return NULL;
}

#ifdef WITH_STATISTICS
struct statistics_t *getStatistics(struct soap *soap_internal)
{
  CWS_CONFIG *config=(CWS_CONFIG *)soap_internal->user;
  struct statistics_t *statistics=&(config)->statistics;
  long int l;

  if (statistics->total < 1)
    statistics->total=
      statistics->costs +
      statistics->strings +
      statistics->shorts +
      statistics->ints +
      statistics->long64s +
      statistics->enums +
      statistics->arrays +
      statistics->booleans +
      statistics->doubles +
      statistics->date_times +
      statistics->event_types +
      statistics->measures;

  if (config->initial_resource_size > 0)
    if ((l=getResourceSize()) > config->initial_resource_size)
        statistics->used_memory=(size_t)(l - config->initial_resource_size);

  return statistics;
}
#endif

struct c_bson_serialized_t *bsonSerialize(struct soap *soap_internal)
{
  CWS_CONFIG *config=((CWS_CONFIG *)soap_internal->user);
  struct c_bson_serialized_t *ptr=&config->c_bson_serialized;

  if (ptr->bson)
    return ptr;

  if (config->object)
    if (!bson_serialize(&(ptr->bson), &(ptr->bson_size), (const char *)GET_OBJECT_NAME, -1, config->object))
      return ptr;

  return NULL;
}

int writeToFile(struct soap *soap_internal, const char *filename)
{
  int err;
  FILE *f;
  char fname[64];

  struct c_bson_serialized_t *ptr;

  if (!(ptr=bsonSerialize(soap_internal)))
    return -1;

  if (!filename)
    snprintf((char *)(filename=(const char *)fname), sizeof(fname), "%s.bson", GET_OBJECT_NAME);

  if (!(f=fopen(filename, "w")))
    return -2;

  err=(fwrite((void *)ptr->bson, sizeof(uint8_t), ptr->bson_size, f)==ptr->bson_size)?0:-3;

  fclose(f);

  return err;
}

