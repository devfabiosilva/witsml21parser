#define _GNU_SOURCE
#include <stdio.h>
#include <cws_errors.h>
#include <request_context.h>
#include "../stdsoap2.h"

void cws_set_soap_fault_util(struct soap *, int, char *, size_t, int);

#define CWS_INTERNAL_SOAP_FREE(ptr) \
    (ptr)->is=NULL; \
    soap_destroy(ptr); \
    soap_end(ptr); \
    soap_free(ptr); \
    ptr=NULL;

int cws_internal_soap_new(struct soap **soap_internal, CWS_CONFIG *config,  int *init_options)
{

  if (!(*soap_internal = soap_new1((init_options)?(*init_options):config->internalInitFlag)))
    return CWS_ERR_INIT_INTERNAL_GSOAP;

  config->soap_internal = (*soap_internal);

  soap_set_mode(*soap_internal, SOAP_XML_GRAPH);
  soap_clr_mode(*soap_internal, SOAP_XML_TREE);

  (*soap_internal)->user=(void *)config;

  (*soap_internal)->sendfd=-1;
  (*soap_internal)->recvfd=-1;

  config->internal_os=NULL;

  (*soap_internal)->os=&config->internal_os;

  return 0;
}

int cws_soap_serve(struct soap *soap_internal)
{
  int errCode, errType;
  const char *str;

  soap_internal->is=(const char *)((CWS_CONFIG *)soap_internal->user)->xmlSoap;

  if ((errCode=soap_serve(soap_internal))==SOAP_OK)
    return 0;

  if ((str=soap_fault_subcode(soap_internal)))
    (strcmp("SOAP-ENV:Client", str))?(errType=-1):(errType=0);
  else
    errType=0;


  #define UNKNOWN_SOAP_INTERNAL_FAULTSTR "Unknown faultstring in SOAP internal. Err. CWS_ERR_INTERNAL_GSOAP_FAULTSTRING"
  #define UNKNOWN_SOAP_INTERNAL_FAULTSTR_LEN sizeof(UNKNOWN_SOAP_INTERNAL_FAULTSTR)-1
  if ((str=soap_fault_string(soap_internal)))
    cws_set_soap_fault_util(soap_internal, errCode=CWS_ERR_INTERNAL_GSOAP_SERVE, (char *)str, strlen(str), errType);
  else
    cws_set_soap_fault_util(soap_internal, errCode=CWS_ERR_INTERNAL_GSOAP_FAULTSTRING, UNKNOWN_SOAP_INTERNAL_FAULTSTR, UNKNOWN_SOAP_INTERNAL_FAULTSTR_LEN, errType);
  #undef UNKNOWN_SOAP_INTERNAL_FAULTSTR_LEN
  #undef UNKNOWN_SOAP_INTERNAL_FAULTSTR

  return errCode;
}

void cws_internal_soap_free(struct soap **soap)
{
  CWS_CONFIG *config;

  if (*soap) {

    config=(CWS_CONFIG *)((*soap)->user);

    if (config->cws_soap_fault.faultstring) {
      free((void *)config->cws_soap_fault.faultstring);
      config->cws_soap_fault.faultstring=NULL;
    }

    if (config->cws_soap_fault.XMLfaultdetail) {
      free((void *)config->cws_soap_fault.XMLfaultdetail);
      config->cws_soap_fault.XMLfaultdetail=NULL;
    }

    CWS_INTERNAL_SOAP_FREE(*soap)
  }
}

#undef CWS_INTERNAL_SOAP_FREE

//SOAP INTERNAL
// -1 -> Server 0 -> Client ???
#define CWITSML_FAULT_DETAIL "<CWitsmlStoreError type=\"SOAP_INTERNAL\" subCode=\"%s\" errorCode=\"%d\">%.*s</CWitsmlStoreError>"
#define CWITSML_SET_FAULT_STRING(soap_ptr, subCode) \
if ((config->cws_soap_fault.XMLfaultdetail_len=asprintf( \
     &config->cws_soap_fault.XMLfaultdetail, CWITSML_FAULT_DETAIL, subCode, error_code, (int)faultstring_len, faultstring))<0) { \
  config->cws_soap_fault.XMLfaultdetail=NULL; \
  config->cws_soap_fault.XMLfaultdetail_len=0; \
} \
\
if ((config->cws_soap_fault.faultstring=cws_malloc((size_t)(faultstring_len+1)))) { \
  config->cws_soap_fault.faultstring_len=faultstring_len; \
  strcpy(config->cws_soap_fault.faultstring, faultstring); \
  if (type) \
    soap_receiver_fault(soap_ptr, (const char *)config->cws_soap_fault.faultstring, (const char *)config->cws_soap_fault.XMLfaultdetail); \
  else \
    soap_sender_fault(soap_ptr, (const char *)config->cws_soap_fault.faultstring, (const char *)config->cws_soap_fault.XMLfaultdetail); \
}

void cws_set_soap_fault_util(struct soap *soap, int error_code, char *faultstring, size_t faultstring_len, int type)
{

  CWS_CONFIG *config=(CWS_CONFIG *)soap->user;

  if ((config->internal_soap_error!=0)||(error_code==0))
    return; // Doesn't overlap last error;

  config->internal_soap_error=error_code;

  CWITSML_SET_FAULT_STRING(config->soap_internal, (type)?"SOAP-ENV:Server":"SOAP-ENV:Client")
}

#undef CWITSML_SET_FAULT_STRING
#undef CWITSML_FAULT_DETAIL

