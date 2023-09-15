// initialize NodeJS Witsml 2.1 API
//a = require('./jswitsml21bson')
//napi_add_finalizer
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
  char buf[1024];
  char err[16];
};

typedef struct cws_js_fn_call_t {
   const char *function_name;
   cws_node_fn fn;
} CWS_JS_FUNCTIONS_OBJ;

//https://nodejs.org/api/n-api.html#napi_finalize
//typedef void (*napi_finalize)(napi_env env, void* finalize_data, void* finalize_hint);
static void cws_js_finalize(napi_env env, void *finalize_data, void* finalize_hint)
{
  printf("\nFinalize %s %s\n", (char *)finalize_data, (char *)finalize_hint);
}

static void cws_js_throw(napi_env env, struct cws_js_err_t *caster, const char *c_function_name, const char *errMessage, int err)
{
  snprintf(caster->buf, sizeof(caster->buf), "%s:%d %s", c_function_name, err, errMessage);
  snprintf(caster->err, sizeof(caster->err), "%d", err);
  napi_throw_error(env, caster->err, caster->buf);
}

static int cws_add_function_util(napi_env env, napi_value exports, CWS_JS_FUNCTIONS_OBJ *function)
{
  napi_value fn;

  while (function->function_name) {
    if (napi_create_function(env, NULL, 0, function->fn, NULL, &fn)!=napi_ok)
      return 300;

    if (napi_set_named_property(env, exports, function->function_name, fn)!=napi_ok)
      return 301;

    function++;
  }

  return 0;
}

///

napi_value c_bson_version(napi_env env, napi_callback_info info)
{
  napi_value argv, res;
  size_t argc=1;

  if (napi_get_cb_info(env, info, &argc, &argv, NULL, NULL)!=napi_ok) {
    napi_throw_error(env, "-19", "c_version: Can't parse arguments");
    return NULL;
  }

  if (argc) {
    napi_throw_error(env, "-20", "c_version: Too many arguments");
    return NULL;
  }
#define TEST "Testing module"
  if (napi_create_string_utf8(env, (const char *)TEST, sizeof(TEST)-1, &res)!=napi_ok) {
    napi_throw_error(env, "-21", "c_version: Fail on parse UTF-8 string");
    return NULL;
  }

  return res;
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_CREATE_FUNCTIONS[] = {
   {"getBsonVersion", c_bson_version},
   {NULL}
};

napi_value c_create(napi_env env, napi_callback_info info)
{
  int err;
  napi_value global, constructor, argv=NULL, res;
  //size_t argc = 1;
  size_t argc=0;
  struct cws_js_err_t cws_js_err;

  if (napi_get_cb_info(env, info, &argc, &argv, NULL, NULL)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Can't parse arguments. Wrong argument at creare", 29);
    return NULL;
  }

  if (argc) {
    cws_js_throw(env, &cws_js_err, "c_create", "Too many arguments", 30);
    return NULL;
  }

  if (napi_get_global(env, &global)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Could not get global env", 31);
    return NULL;  
  }

  if (napi_get_named_property(env, global, "NewObject", &constructor)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_create", "Could not add 'MyObj' method to constructor", 32);
    return NULL;  
  }

  if (napi_new_instance(env, constructor, 0, &argv, &res)==napi_ok) {

    if ((err=cws_add_function_util(env, res, CWS_JS_CREATE_FUNCTIONS))) {
      cws_js_throw(env, &cws_js_err, "c_create", "Could add functions to constructor", err);
      return NULL;
    }

    if (napi_add_finalizer(env, res, (void *)"abc", cws_js_finalize, NULL, NULL)!=napi_ok) {
      cws_js_throw(env, &cws_js_err, "c_create", "Could not add finalizer callback", 33);
      return NULL;
    }
    
    return res;
  }

  cws_js_throw(env, &cws_js_err, "c_create", "Could not create new instance", 34);
  return NULL;
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_INIT_FUNCTIONS[] = {
   {"Create", c_create},
   {NULL}
};

napi_value Init(napi_env env, napi_value exports)
{

  int err;
  napi_value object;
  struct cws_js_err_t cws_js_err;

  if ((err=cws_add_function_util(env, exports, CWS_JS_INIT_FUNCTIONS))) {
    cws_js_throw(env, &cws_js_err, "cws_add_function_util", "Could not initialize C functions", err);
    return NULL;
  }

  if (napi_create_object(env, &object)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "INIT", "Could not create object", err);
    return NULL;
  }

  if (napi_set_named_property(env, exports, "NewObject", object)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "INIT_NAME", "Could not set NAME to object", err);
    return NULL;
  }

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

