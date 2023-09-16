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

#ifdef JS_SOAP_DEBUG
 #define JS_WITSML21_DEBUG(...) \
    fprintf(stdout, __VA_ARGS__);
#else
 #define JS_WITSML21_DEBUG(std, ...)
#endif

#define JS_CWS_THROW(errFunc, errDesc, errCode) \
  cws_js_throw(env, &cws_js_err, errFunc, errDesc, errCode); \
  return NULL;

#define JS_CWS_THROW_COND(condT, errFunc, errDesc, errCode) \
  if (condT) { \
    JS_CWS_THROW(errFunc, errDesc, errCode) \
  }

/// UTILITIES
typedef napi_value (*cws_node_fn)(napi_env, napi_callback_info);

struct js_cws_config_t {
  struct soap *soap_internal;
  int err;
};

struct cws_js_err_t {
  char buf[256];
  char err[16];
};

#define SET_JS_FN_CALL(fn) {#fn, c_##fn}
typedef struct cws_js_fn_call_t {
   const char *function_name;
   cws_node_fn fn;
} CWS_JS_FUNCTIONS_OBJ;

static void js_cws_config_free(struct js_cws_config_t **js_cws_config)
{
  CWS_CONFIG *config;

  JS_WITSML21_DEBUG("\njs_cws_config_free: Begin free if is NOT NULL %p", *js_cws_config)

  if (*js_cws_config) {
    config=(CWS_CONFIG *)(*js_cws_config)->soap_internal->user;

    JS_WITSML21_DEBUG("\njs_cws_config_free: Destroying soap_internal %p", (*js_cws_config)->soap_internal)
    cws_internal_soap_free(&(*js_cws_config)->soap_internal);

    JS_WITSML21_DEBUG("\njs_cws_config_free: Destroying config %p", config)
    cws_config_free(&config);

    JS_WITSML21_DEBUG("\njs_cws_config_free: Destroying js_cws_config object %p", js_cws_config)
    free((void *)*js_cws_config);
    *js_cws_config=NULL;

    JS_WITSML21_DEBUG("\njs_cws_config_free: Destroyed")
  }

}

static struct js_cws_config_t *js_cws_config_init()
{
  CWS_CONFIG *config;
  struct js_cws_config_t *js_cws_config;

  JS_WITSML21_DEBUG("\njs_cws_config_init: Initializing ...")
  if (!(js_cws_config=(struct js_cws_config_t *)malloc(sizeof(struct js_cws_config_t))))
    return NULL;

  if (!(config=cws_config_new("JSWITSML 2.1 BSON parser")))
    goto js_cws_config_init_exit1;

  if (cws_internal_soap_new(&js_cws_config->soap_internal, config, NULL))
    goto js_cws_config_init_exit2;

  js_cws_config->err=0;

  JS_WITSML21_DEBUG("\njs_cws_config_init: SUCCESS ... %p", js_cws_config)

  return js_cws_config;

js_cws_config_init_exit2:
  cws_config_free(&config);

js_cws_config_init_exit1:
  free((void *)js_cws_config);

  JS_WITSML21_DEBUG("\njs_cws_config_init: FAIL ...")

  return NULL;
}

//https://nodejs.org/api/n-api.html#napi_finalize
//typedef void (*napi_finalize)(napi_env env, void* finalize_data, void* finalize_hint);
static void cws_js_finalize(napi_env env, void *finalize_data, void* finalize_hint)
{
  void *data=finalize_data;
  JS_WITSML21_DEBUG("\ncws_js_finalize: Entering and perform cleanup %p", data)

  js_cws_config_free((struct js_cws_config_t **)&data);

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
  void *buf;
  struct cws_js_err_t cws_js_err;
  struct cws_version_t version;

  if (napi_get_cb_info(env, info, &argc, &argv, NULL, NULL)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "c_getBsonVersion", "Can't parse arguments", -19);
    return NULL;
  }

  if (argc) {
    cws_js_throw(env, &cws_js_err, "c_getBsonVersion", "Too many arguments", -20);
    return NULL;
  }

  cws_version(&version);

  if (napi_create_arraybuffer(env, version.versionSize, &buf, &res)==napi_ok) {
    memcpy(buf, (void *)version.version, version.versionSize);
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
  napi_value constructor, argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, NULL)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at create",
    98
  )

  JS_CWS_THROW_COND(argc, "c_create", "Too many arguments", 99)

  JS_CWS_THROW_COND(
    napi_create_function(env, NULL, 0, c__witsml21bsonInit_, NULL, &constructor)!=napi_ok,
    "napi_create_function",
    "Error on generating c__witsml21bsonInit_",
    100
  )

  if (napi_new_instance(env, constructor, 0, &argv, &res)==napi_ok) {

    JS_CWS_THROW_COND((js_cws_instance=js_cws_config_init())==NULL, "js_cws_config_init", "Could not alloc JSWITSML 2.1 C instance", 101)

    if ((err=cws_add_function_util(NULL, env, res, CWS_JS_CREATE_FUNCTIONS, js_cws_instance))) {
      js_cws_config_free(&js_cws_instance);

      JS_CWS_THROW("cws_add_function_util", "Could add functions @ c_create constructor", 102)
    }

    if (napi_add_finalizer(env, res, (void *)js_cws_instance, cws_js_finalize, NULL, NULL)!=napi_ok) {
      js_cws_config_free(&js_cws_instance);

      JS_CWS_THROW("napi_add_finalizer", "Could not add finalizer callback", 103)
    }

    return res;
  }

  JS_CWS_THROW("c_create", "Could not create new instance", 104)
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_INIT_FUNCTIONS[] = {
   SET_JS_FN_CALL(create),
   {NULL}
};

napi_value Init(napi_env env, napi_value exports)
{
  int err;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    (err=cws_add_function_util(NULL, env, exports, CWS_JS_INIT_FUNCTIONS, NULL)),
    "cws_add_function_util",
    "Could not initialize C functions",
    err
  )

/*
  TODO remove it. Maybe not necessary
  if (napi_set_instance_data(env, (void *)NULL, cws_js_finalize, (void *)NULL)!=napi_ok) {
    cws_js_throw(env, &cws_js_err, "napi_set_instance_data", "Could not set instance data to object", err);
    return NULL;
  }
*/
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

