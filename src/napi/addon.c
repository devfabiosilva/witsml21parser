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

#define JS_CWS_THROW_GOTO(errFunc, errDesc, errCode, onErrorGoto) \
  cws_js_throw(env, &cws_js_err, errFunc, errDesc, errCode); \
  goto onErrorGoto;

#define JS_CWS_THROW_COND_GOTO(condT, errFunc, errDesc, errCode, onErrorGoto) \
  if (condT) { \
    JS_CWS_THROW_GOTO(errFunc, errDesc, errCode, onErrorGoto) \
  }

#define JS_CWS_THROW_COND(condT, errFunc, errDesc, errCode) \
  if (condT) { \
    JS_CWS_THROW(errFunc, errDesc, errCode) \
  }

#define JS_GET_INSTANCE_NAME ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->instance_name

#define ERR js_cws_instance->err

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

struct cws_js_int32_t {
   const char *name;
   int32_t value;
} CWS_JS_INT32[] = {
  {"TYPE_None", TYPE_None},
  {"TYPE_BhaRun", TYPE_BhaRun},
  {"TYPE_CementJob", TYPE_CementJob},
  {"TYPE_DepthRegImage", TYPE_DepthRegImage},
  {"TYPE_DownholeComponent", TYPE_DownholeComponent},
  {"TYPE_DrillReport", TYPE_DrillReport},
  {"TYPE_FluidsReport", TYPE_FluidsReport},
  {"TYPE_Log", TYPE_Log},
  {"TYPE_MudLogReport", TYPE_MudLogReport},
  {"TYPE_OpsReport", TYPE_OpsReport},
  {"TYPE_Rig", TYPE_Rig},
  {"TYPE_Risk", TYPE_Risk},
  {"TYPE_StimJob", TYPE_StimJob},
  {"TYPE_SurveyProgram", TYPE_SurveyProgram},
  {"TYPE_Target", TYPE_Target},
  {"TYPE_ToolErrorModel", TYPE_ToolErrorModel},
  {"TYPE_Trajectory", TYPE_Trajectory},
  {"TYPE_Tubular", TYPE_Tubular},
  {"TYPE_Well", TYPE_Well},
  {"TYPE_Wellbore", TYPE_Wellbore},
  {"TYPE_WellboreCompletion", TYPE_WellboreCompletion},
  {"TYPE_WellboreGeology", TYPE_WellboreGeology},
  {"TYPE_WellCMLedger", TYPE_WellCMLedger},
  {"TYPE_WellCompletion", TYPE_WellCompletion},
  {NULL}
};

#define TEXT_BUF_ALLOC_MAX_SZ 2048
static char *textBufAlloc(size_t len)
{
  len++;

  if ((len>1)&&(len<TEXT_BUF_ALLOC_MAX_SZ))
    return cws_malloc(len);

  return NULL;
}
#undef TEXT_BUF_ALLOC_MAX_SZ

static char *js_cws_get_value_string_utf8(size_t *len, napi_env env, napi_value value)
{
  char *res;

  if (napi_get_value_string_utf8(env, value, NULL, 0, len)!=napi_ok)
    return NULL;

  if (!(res=textBufAlloc(*len)))
    return NULL;

  if (napi_get_value_string_utf8(env, value, res, (*len)+1, len)==napi_ok) {
    JS_WITSML21_DEBUG("\njs_cws_get_value_string_utf8: has NULL terminator: %d\n", res[*len]==0)
    JS_WITSML21_DEBUG("\njs_cws_get_value_string_utf8: value: %s\n", res)
    return res;
  }

  free((void *)res);
  return NULL;
}

enum c_parse_util_e {
  IS_BSON=1,
  IS_JSON
};

static int c_parse_util(
  struct soap *soap_internal, 
  void **v_ser,
  const char *c_xml,
  size_t c_xml_size,
  enum c_parse_util_e c_parse_util 
)
{

  DECLARE_CONFIG(soap_internal)
  *v_ser=NULL;

  cws_internal_soap_recycle(soap_internal);
  cws_recycle_config(config);

  if (!cws_parse_XML_soap_envelope(soap_internal, (char *)c_xml, (size_t)c_xml_size))
    return -500;

  if (cws_soap_serve(soap_internal))
    return -501;

  if (c_parse_util==IS_BSON) {
    if ((*v_ser=(void *)bsonSerialize(soap_internal)))
      return 0;

  } else {
    if ((*v_ser=(void *)getJson(soap_internal)))
      return 0;
  }

  return -503;
}

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

static int cws_add_int32_util(napi_value *int32_out, napi_env env, napi_value exports, struct cws_js_int32_t *value)
{
  napi_value *int32Tmp, int32;

  int32Tmp=(int32_out)?int32_out:&int32;

  while (value->name) {
    if (napi_create_int32(env, value->value, int32Tmp)!=napi_ok)
      return 600;

    if (napi_set_named_property(env, exports, value->name, *int32Tmp)!=napi_ok)
      return 601;

    value++;
  }

  return 0;
}

inline int js_cws_new_array_buffer(napi_value *dest, napi_env env, void *src, size_t src_sz)
{
  void *buf;

  if (napi_create_arraybuffer(env, src_sz, &buf, dest)==napi_ok) {
    memcpy(buf, src, src_sz);
    return 0;
  }

  return -1;
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
  struct cws_js_err_t cws_js_err;
  struct cws_version_t version;

  JS_CWS_THROW_COND(napi_get_cb_info(env, info, &argc, &argv, NULL, NULL)!=napi_ok, "napi_get_cb_info", "Can't parse arguments", -19)

  JS_CWS_THROW_COND(argc, "c_getBsonVersion", "Too many arguments", -20)

  cws_version(&version);

  if (js_cws_new_array_buffer(&res, env, (void *)version.version, version.versionSize)==0)
    return res;

  JS_CWS_THROW("c_getBsonVersion", "Fail to create ArrayBuffer", -21);
}

napi_value c_parseFromFile(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  char *filename, *text;
  size_t argc=1, filenameLen, textLen;
  struct cws_js_err_t cws_js_err;
  struct c_bson_serialized_t *bson_ser;
  struct js_cws_config_t *js_cws_instance;

  JS_CWS_THROW_COND(
    (ERR=(napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok)),
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at create",
    120
  )

  JS_CWS_THROW_COND(
    (ERR=(js_cws_instance==NULL)),
    "c_parseFromFile",
    "Fatal: js_cws_instance. Was expected NOT NULL",
    121
  )

  JS_CWS_THROW_COND(
    (ERR=(argc==0)),
    "c_parseFromFile",
    "Missing argument. Was expected: file path/filename",
    122
  )

  JS_CWS_THROW_COND(
    (ERR=(argc>1)),
    "c_parseFromFile",
    "Too many arguments",
    123
  )

  JS_CWS_THROW_COND(
    (ERR=((filename=js_cws_get_value_string_utf8(&filenameLen, env, argv))==NULL)),
    "c_parseFromFile",
    "Could not parse filename. Wrong format or invalid utf-8 or no space in C string buffer",
    124
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=readText((const char **)&text, &textLen, filename)),
    "readText", "Could not read xml/text",
    ERR, c_parseFromFile_exit1
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=c_parse_util(js_cws_instance->soap_internal, (void **)&bson_ser, text, textLen, IS_BSON)),
    "c_parse_util", "BSON parse error @ c_parseFromFile",
    ERR, c_parseFromFile_exit2
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=js_cws_new_array_buffer(&res, env, (void *)bson_ser->bson, bson_ser->bson_size)),
    "js_cws_new_array_buffer", "Could not copy BSON bytes to JavaScript array buffer @ c_parseFromFile",
    ERR, c_parseFromFile_exit2
  )

  free((void *)text);
  free((void *)filename);

  return res;

c_parseFromFile_exit2:
  free((void *)text);

c_parseFromFile_exit1:
  free((void *)filename);

  return NULL;
}

napi_value c_getInstanceName(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getInstanceName",
    130
  )

  JS_CWS_THROW_COND(argc, "c_getInstanceName", "Too many arguments @ getInstanceName", 131)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getInstanceName",
    "Fatal: js_cws_instance @ c_getInstanceName. Was expected NOT NULL",
    132
  )

  JS_CWS_THROW_COND(
    (napi_create_string_utf8(env, JS_GET_INSTANCE_NAME, NAPI_AUTO_LENGTH, &res)!=napi_ok),
    "napi_create_string_utf8",
    "napi_create_string_utf8 @ c_getInstanceName. Error on parsing instance name",
    133
  )

  return res;
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_CREATE_FUNCTIONS[] = {
   SET_JS_FN_CALL(parseFromFile),
   SET_JS_FN_CALL(getInstanceName),
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
   SET_JS_FN_CALL(getBsonVersion),
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

  JS_CWS_THROW_COND(
    (err=cws_add_int32_util(NULL, env, exports, CWS_JS_INT32)),
    "cws_add_int32_util",
    "Could not initialize C WITSML 2.1 const object",
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

