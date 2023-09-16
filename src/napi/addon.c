// initialize NodeJS Witsml 2.1 API
//a = require('./jswitsml21bson')
//napi_add_finalizer
/*
a = require('./jswitsml21bson')

b=a.create()
c=a.create()

{
  "targets": [
    {
      "target_name": "jswitsml21bson",
      "sources": [ "./src/napi/addon.c", "./soapServer.c", "./stdsoap2.c" ],
      "include_dirs":[ "./include", "./" ],
      "libraries": [
              "-Wl,-rpath,./lib/libbson-shared-1.0.a", "-Wl,-rpath,./lib/libcws_js.a"
          ],
      "extra_objects": ["./soapC_shared.o"],
      "defines": ["CWS_LITTLE_ENDIAN", "WITH_STATISTICS"]
    }
  ]
}

*/
#define NAPI_VERSION 8
#include "../../ns1.nsmap" // XML namespace mapping table (only needed once at the global level)
#include "../../soapH.h"    // server stubs, serializers, etc.
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <cws_utils.h>
#include <node_api.h>

//https://nodejs.org/api/n-api.html
//https://github.com/nodejs/nan/tree/HEAD/examples/

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

/// UTILITIES
typedef napi_value (*cws_node_fn)(napi_env, napi_callback_info);

struct cws_js_err_t {
  char buf[256];
  char err[16];
};

#define SET_JS_FN_CALL(fn) {#fn, c_##fn}
typedef struct cws_js_fn_call_t {
   const char *function_name;
   cws_node_fn fn;
} CWS_JS_FUNCTIONS_OBJ;

//https://nodejs.org/api/n-api.html#napi_finalize
//typedef void (*napi_finalize)(napi_env env, void* finalize_data, void* finalize_hint);
static void cws_js_finalize(napi_env env, void *finalize_data, void* finalize_hint)
{
  printf("\nFinalize %p %s", finalize_data, (char *)finalize_hint);
  if (finalize_data) {
    printf("Freeing finalize_data ...\n");
    free(finalize_data);
  }
}

static void cws_js_throw(napi_env env, struct cws_js_err_t *caster, const char *c_function_name, const char *errMessage, int err)
{
  snprintf(caster->buf, sizeof(caster->buf), "%s:%d %s", c_function_name, err, errMessage);
  snprintf(caster->err, sizeof(caster->err), "%d", err);
  napi_throw_error(env, caster->err, caster->buf);
}

static int cws_add_function_util(napi_value *fn_out, napi_env env, napi_value exports, CWS_JS_FUNCTIONS_OBJ *function, void *data)
{
  napi_value *fnTmp, fn;

  fnTmp=(fn_out)?fn_out:&fn;

  while (function->function_name) {
    if (napi_create_function(env, NULL, 0, function->fn, data, fnTmp)!=napi_ok)
      return 300;

    if (napi_set_named_property(env, exports, function->function_name, *fnTmp)!=napi_ok)
      return 301;

    function++;
  }

  return 0;
}

///

napi_value c__witsml21bsonInit_(napi_env env, napi_callback_info info)
{
  return NULL;
}

napi_value c_getBsonVersion(napi_env env, napi_callback_info info)
{
  napi_value argv, res;
  size_t argc=1;
  void *thisData, *buf;
  struct cws_js_err_t cws_js_err;
  struct cws_version_t version;

  if (napi_get_cb_info(env, info, &argc, &argv, NULL, &thisData)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_getBsonVersion", "Can't parse arguments", -19);
    return NULL;
  }

  if (argc) {
    cws_js_throw(env, &cws_js_err, "c_getBsonVersion", "Too many arguments", -20);
    return NULL;
  }

  cws_version(&version);

/*
#define TEST "Testing module"
  if (napi_create_string_utf8(env, (const char *)TEST, sizeof(TEST)-1, &res)!=napi_ok) {
    napi_throw_error(env, "-21", "c_version: Fail on parse UTF-8 string");
    return NULL;
  }
*/

  if (napi_create_arraybuffer(env, version.versionSize, &buf, &res)==napi_ok) {
    memcpy(buf, (void *)version.version, version.versionSize);
    printf("\nData pointer thisData %p\n", thisData);
    return res;
  }

  cws_js_throw(env, &cws_js_err, "c_getBsonVersion", "Fail to create ArrayBuffer", -21);

  return NULL;
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_CREATE_FUNCTIONS[] = {
   SET_JS_FN_CALL(getBsonVersion),
   {NULL}
};

napi_value c_create(napi_env env, napi_callback_info info)
{
  int err;
  napi_value thisArg, constructor, argv=NULL, res;
  //size_t argc = 1;
  size_t argc=0;
  struct cws_js_err_t cws_js_err;

  if (napi_get_cb_info(env, info, &argc, &argv, &thisArg, NULL)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Can't parse arguments. Wrong argument at create", 29);
    return NULL;
  }

  if (argc) {
    cws_js_throw(env, &cws_js_err, "c_create", "Too many arguments", 30);
    return NULL;
  }
/*
  if (napi_get_global(env, &thisArg)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Could not get global env", 31);
    return NULL;  
  }
*/
/*
  if (napi_get_named_property(env, thisArg, "_witsml21bsonInit_", &constructor)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Could not add '_witsml21bsonInit_' method to constructor", 32);
    return NULL;  
  }
*/

  if (napi_create_function(env, NULL, 0, c__witsml21bsonInit_, NULL, &constructor)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "napi_create_function error", 100);
    return NULL;
  }

/*
  if ((err=cws_add_function_util(&constructor, env, thisArg, CWS_JS_CREATE_FUNCTIONS))) {
    cws_js_throw(env, &cws_js_err, "c_create", "Could add functions to constructor", err);
    return NULL;
  }
*/
  if (napi_new_instance(env, constructor, 0, &argv, &res)==napi_ok) {

    void *v=malloc(32);
    if ((err=cws_add_function_util(NULL, env, res, CWS_JS_CREATE_FUNCTIONS, v))) {
      if (v)
        free(v);
      cws_js_throw(env, &cws_js_err, "c_createA", "Could add functions to constructor", err);
      return NULL;
    }

    printf("\nCreating ... %p", v);

/*
    if ((err=cws_add_function_util(NULL, env, res, CWS_JS_CREATE_FUNCTIONS))) {
      cws_js_throw(env, &cws_js_err, "c_create", "Could add functions to constructor", err);
      return NULL;
    }
*/

    if (napi_add_finalizer(env, res, (void *)v, cws_js_finalize, (void *)"abc", NULL)!=napi_ok) {
      cws_js_throw(env, &cws_js_err, "c_create", "Could not add finalizer callback", 33);
      return NULL;
    }

    return res;
  }

  cws_js_throw(env, &cws_js_err, "c_create", "Could not create new instance", 34);
  return NULL;
}

/*
CWS_JS_FUNCTIONS_OBJ CWS_JS_INIT_FUNCTIONS[] = {
   {"getBsonVersion", c_bson_version},
   {"create", c_create_constructor},
   {NULL}
};
*/
CWS_JS_FUNCTIONS_OBJ CWS_JS_INIT_FUNCTIONS[] = {
   SET_JS_FN_CALL(create),
   {NULL}
};

napi_value Init(napi_env env, napi_value exports)
{

  int err;

  struct cws_js_err_t cws_js_err;

  if ((err=cws_add_function_util(NULL, env, exports, CWS_JS_INIT_FUNCTIONS, NULL))) {
    cws_js_throw(env, &cws_js_err, "cws_add_function_util", "Could not initialize C functions", err);
    return NULL;
  }

  if (napi_set_instance_data(env, (void *)NULL, cws_js_finalize, (void *)"test")!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "napi_set_instance_data", "Could not set instance data to object", err);
    return NULL;
  }

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

