#ifndef POINTER_ASSERTS_H
 #define POINTER_ASSERTS_H

void test_pointer_assert();

#define DECLARE_SOAP_INTERNAL struct soap *soap_internal;

#define CHECK_SOAP_INTERNAL_OBJ_PTR(obj, cond) soap_internal->obj==cond

#endif

