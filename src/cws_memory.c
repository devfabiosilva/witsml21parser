#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "stdsoap2.h"
// C Witsml store memory alignment 1^4 (16 bytes)
//#include <stdio.h>

#define CWS_ALIGNMENT (size_t)(1<<4)
#define CWS_ALIGNMENT_MASK (size_t)(CWS_ALIGNMENT-1)

#define CWS_ALIGN \
size_t size_tmp=size; \
\
if (size_tmp&(CWS_ALIGNMENT_MASK)) {\
  size_tmp&=(~CWS_ALIGNMENT_MASK);\
  size_tmp+=CWS_ALIGNMENT;\
}

void *cws_malloc(size_t size)
{
  uint8_t *result;

  CWS_ALIGN

  if (size_tmp>SOAP_MAXALLOCSIZE)
    return NULL;

  if ((result=(uint8_t *)malloc(size_tmp)))
    if ((size_tmp-=size))
       memset((void *)&result[size], 0, size_tmp);

  return (void *)result;
}

void *cws_realloc(void *ptr, size_t size)
{
  uint8_t *result;

  CWS_ALIGN

  if (size_tmp>SOAP_MAXALLOCSIZE)
    return NULL;

  if ((result=(uint8_t *)realloc(ptr, size_tmp)))
    if ((size_tmp-=size))
       memset((void *)&result[size], 0, size_tmp);

  return (void *)result;
}

inline
size_t cws_is_aligned(size_t size)
{
  CWS_ALIGN
  return size_tmp-size;
}

#undef CWS_ALIGN

