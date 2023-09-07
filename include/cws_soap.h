#ifndef CWS_SOAP_H
 #define CWS_SOAP_H

#include <request_context.h>
#include "../stdsoap2.h"

int cws_internal_soap_new(struct soap **, CWS_CONFIG *, int *);
int cws_soap_serve(struct soap *);

void cws_internal_soap_recycle(struct soap *);

void cws_internal_soap_free(struct soap **);

void cws_set_soap_fault_util(struct soap *, int, char *, size_t, int);

#define CWS_RETURN(soap) return (((CWS_CONFIG *)(soap->user))->internal_soap_error)?SOAP_FAULT:SOAP_OK; // Remove ???

//#define CWS_RETURN_FAIL return (((CWS_CONFIG *)(soap_internal->user))->internal_soap_error);

#define CWS_FAULTSTR(str) str, (sizeof(str)-1)
#endif

