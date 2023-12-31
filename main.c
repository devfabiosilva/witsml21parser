//qua 05 abr 2023 16:05:25
//__        _____ _____ ____  __  __ _       ____    _   ____ ____   ___  _   _ 
//\ \      / /_ _|_   _/ ___||  \/  | |     |___ \  / | | __ ) ___| / _ \| \ | |
// \ \ /\ / / | |  | | \___ \| |\/| | |       __) | | | |  _ \___ \| | | |  \| |
//  \ V  V /  | |  | |  ___) | |  | | |___   / __/ _| | | |_) |__) | |_| | |\  |
//   \_/\_/  |___| |_| |____/|_|  |_|_____| |_____(_)_| |____/____/ \___/|_| \_|

#define _GNU_SOURCE

#include "ns1.nsmap"
#include "soapH.h"
#include <request_context.h>
#include <cws_soap.h>
#include <stdio.h>
#include <wistml2bson_deserializer.h>
#include <sys/resource.h>
#include <cws_utils.h>

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

#ifdef WITH_STATISTICS
void printStatistics(struct soap *soap_internal) {
  struct statistics_t *statistics=getStatistics(soap_internal);

  printf("\nTOTAL: %d\n\tCosts: %d\n\tStrings: %d\n\tShorts: %d\n\tInts: %d\n\tLong64s: %d\n\tEnums: %d\n\tArrays: %d\n\tBooleans: %d\n\t\
Doubles: %d\n\tDate Time: %d\n\tMeasures: %d\n\tEvent types: %d\n\tMem: %lu",
    statistics->total, statistics->costs, statistics->strings, statistics->shorts, statistics->ints, statistics->long64s, statistics->enums,
    statistics->arrays, statistics->booleans, statistics->doubles, statistics->date_times, statistics->measures,
    statistics->event_types, (unsigned long int)statistics->used_memory
  );
}
#endif

int ns21__readWitsmlObject(struct soap *soap_internal, struct ns21__witsmlObject *WitsmlObject, char **result)
{
  int err;
  readWitsmlObjectFn parser=readWitsmlObjectBsonParser(soap_internal, WitsmlObject);
  struct c_json_str_t *c_json_str=getJson(soap_internal);

  printf("\nGetting BSON serialized before parser %p", bsonSerialize(soap_internal));

  if (parser) {
    if ((err=parser(soap_internal))) {
      printf("\nError on parsing Object type: %d Object name: %s... Error: %d", GET_OBJECT_TYPE, GET_OBJECT_NAME, err);
      return SOAP_FAULT;
    }
    printf("\nParsing Object type: %d Object name: %s...", GET_OBJECT_TYPE, GET_OBJECT_NAME);
  } else
    return SOAP_FAULT;

  printf("\nGetting JSON string after parser %p", c_json_str=getJson(soap_internal));

  if (c_json_str) {
    printf("\nGetting value %.*s", (int)c_json_str->json_len, c_json_str->json);
  } else {
    printf("\nUnexpected c_json_str=NULL");
    return SOAP_FAULT;
  }

#ifdef WITH_STATISTICS
  printStatistics(soap_internal);
#endif

  printf("\nGetting BSON serialized after parser %p", bsonSerialize(soap_internal));

  return SOAP_OK;
}

int main(int argc, char **argv)
{
  int err;
  const char *file, *text;
  size_t text_len;
  struct soap *soap;
  CWS_CONFIG *config;
  struct rusage r_usage;

  if (argc != 2) {
    printf("\nSelect one file");
    return -5;
  }

  file=(const char *)argv[1];
  printf("\nReading file %s", file);

  if ((err=readText(&text, &text_len, file))) {
    printf("\nCould not open file error: %d \"%s\"\n", err, file);
    return err;
  }

  config=cws_config_new(NULL);

  if (!config) {
    err=-3;
    printf("\nFatal. Could not initialize config");
    goto _exit_error1;
  }

  if ((err=cws_internal_soap_new(&soap, config, NULL))) {
    printf("\nError cws_external_soap_new %d\n", err);
    goto _exit_error2;
  }

  if (!cws_parse_XML_soap_envelope(soap, (char *)text, text_len)) {
    err=-2;
    goto _exit_error4;
  }

  if ((err=cws_soap_serve(soap))!=SOAP_OK) {
_exit_error4:
    soap_print_fault(soap, stderr);
    goto _exit_error3;
  }

  printf("\nWriting to file ");
  if ((err=writeToFile(soap, NULL)))
    printf("\nError on writing file %d\n", err);

  err=getrusage(RUSAGE_SELF, &r_usage);
  if(err == 0)
    printf("\nMemory usage: %ld kilobytes\n",r_usage.ru_maxrss);
  else
    printf("\nError in getrusage. errno = %d\n", errno);

_exit_error3:
  cws_internal_soap_free(&soap);

_exit_error2:
  cws_config_free(&config);

_exit_error1:
  readTextFree(&text);

  return err;
}

