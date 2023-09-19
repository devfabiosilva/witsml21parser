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
    fprintf(stdout, "\nJSWITSML21_PARSER DEBUG: ");\
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

#define JS_GET_INTERNAL_SOAP_ERROR ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->internal_soap_error
#define JS_GET_OBJECT_TYPE ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->object_type
#define JS_GET_OBJECT_NAME ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->object_name
#define JS_GET_OBJECT_TYPE ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->object_type
#define JS_GET_CWS_CONFIG ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))
#define JS_GET_ERROR ((CWS_CONFIG *)(js_cws_instance->soap_internal->user))->internal_soap_error

#define ERR js_cws_instance->err

#define CHECK_HAS_ERROR(errFunc, errDesc, errCde) \
  if ((ERR!=0)||(JS_GET_INTERNAL_SOAP_ERROR!=0)||(JS_GET_OBJECT_TYPE==TYPE_None)||(JS_GET_OBJECT_NAME==NULL)) {\
    JS_CWS_THROW(errFunc, errDesc, errCde) \
  }

#define JS_CWS_RETURN_NULL \
  JS_CWS_THROW_COND( \
    napi_get_null(env, &res)!=napi_ok, \
    "napi_get_null", \
    "Create JavaScript object 'null' error", \
    -15\
  )\
\
  return res;

#define JS_CWS_RETURN_UNDEFINED \
  JS_CWS_THROW_COND( \
    napi_get_undefined(env, &res)!=napi_ok, \
    "napi_get_undefined", \
    "Create JavaScript object 'undefined' error", \
    -16\
  )\
\
  return res;

/// UTILITIES
typedef napi_value (*cws_node_fn)(napi_env, napi_callback_info);

struct js_cws_config_t {
  struct soap *soap_internal;
  int err;
  char *str; //Alloc'd draft text for manipulating string and data. Must be free
  size_t str_len; // Copied string length
  size_t str_size; // Alloc'd string size
};

struct cws_js_err_t {
  char buf[512];
  char err[16];
};

#define SET_JS_CWS_UINT32_STAT(s) {#s, offsetof(struct statistics_t, s)}
struct js_cws_uint32_stat_t {
  const char *name;
  size_t offset;
} JS_CWS_UINT32_STAT[] = {
  SET_JS_CWS_UINT32_STAT(costs),
  SET_JS_CWS_UINT32_STAT(strings),
  SET_JS_CWS_UINT32_STAT(shorts),
  SET_JS_CWS_UINT32_STAT(ints),
  SET_JS_CWS_UINT32_STAT(long64s),
  SET_JS_CWS_UINT32_STAT(enums),
  SET_JS_CWS_UINT32_STAT(arrays),
  SET_JS_CWS_UINT32_STAT(booleans),
  SET_JS_CWS_UINT32_STAT(doubles),
  SET_JS_CWS_UINT32_STAT(date_times),
  SET_JS_CWS_UINT32_STAT(measures),
  SET_JS_CWS_UINT32_STAT(event_types),
  SET_JS_CWS_UINT32_STAT(total),
  {NULL}
};

#define SET_JS_CWS_UINT64_STAT(s) SET_JS_CWS_UINT32_STAT(s)

struct js_cws_uint64_stat_t {
  const char *name;
  size_t offset;
} JS_CWS_UINT64_STAT[] = {
  SET_JS_CWS_UINT64_STAT(used_memory),
  {NULL}
};

#undef SET_JS_CWS_UINT64_STAT
#undef SET_JS_CWS_UINT32_STAT


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

static int cws_set_uint32_stat_list(napi_env env, napi_value exports, struct statistics_t *stat, struct js_cws_uint32_stat_t *list)
{
  napi_value int32;

  while (list->name) {
    if (napi_create_uint32(env, (uint32_t)*((uint32_t *)(((size_t)stat)+list->offset)), &int32)!=napi_ok)
      return 650;

    if (napi_set_named_property(env, exports, list->name, int32)!=napi_ok)
      return 651;

    list++;
  }

  return 0;
}

static int cws_set_uint64_stat_list(napi_env env, napi_value exports, struct statistics_t *stat, struct js_cws_uint64_stat_t *list)
{
  napi_value uint64;

  while (list->name) {
    if (napi_create_bigint_uint64(env, (uint64_t)*((uint64_t *)(((size_t)stat)+list->offset)), &uint64)!=napi_ok)
      return 650;

    if (napi_set_named_property(env, exports, list->name, uint64)!=napi_ok)
      return 651;

    list++;
  }

  return 0;
}
/* DEPRECATED
#define TEXT_BUF_ALLOC_MAX_SZ 2048
static char *textBufAlloc(size_t len)
{
  len++;

  if ((len>1)&&(len<TEXT_BUF_ALLOC_MAX_SZ))
    return cws_malloc(len);

  return NULL;
}
#undef TEXT_BUF_ALLOC_MAX_SZ
*/
static char *textBufAlloc(size_t *sz, struct js_cws_config_t *js_cws_config, size_t len)
{
  char *str_tmp;
  size_t str_sz_tmp;

  if (len>0) {
    JS_WITSML21_DEBUG("textBufAlloc: Begin creation/realloc text buf of length %lu", len)
    if (js_cws_config->str) {
      if (js_cws_config->str_size>len) {
        *sz=js_cws_config->str_size;
        js_cws_config->str[len]=0;
        JS_WITSML21_DEBUG("textBufAlloc: Recycled buffer %p of size %lu", js_cws_config->str, js_cws_config->str_size)
        return js_cws_config->str;
      }

      JS_WITSML21_DEBUG("textBufAlloc: Try to realloc pointer %p of size %lu...", js_cws_config->str, js_cws_config->str_size)
      if ((str_tmp=(char *)cws_realloc((void *)js_cws_config->str, (str_sz_tmp=(len+1))))) {
        js_cws_config->str_len=len;
        *sz=js_cws_config->str_size=str_sz_tmp;
        JS_WITSML21_DEBUG("textBufAlloc: realloc'd pointer has changed? = %s", (str_tmp==js_cws_config->str)?"FALSE":"TRUE")
        (js_cws_config->str=str_tmp)[len]=0;
        JS_WITSML21_DEBUG("textBufAlloc: realloc'd pointer %p of new size %lu...", js_cws_config->str, js_cws_config->str_size)
        return js_cws_config->str;
      }
      
      *sz=js_cws_config->str_size; // Size <= len. Keep last size (realloc fail) and old pointer
      js_cws_config->str_len=0;
      js_cws_config->str[0]=0; // On realloc fail text buffer has empty string
      JS_WITSML21_DEBUG("textBufAlloc: Fail on realloc. Keep old pointer %p of size %lu", js_cws_config->str, js_cws_config->str_size)
    } else {
      JS_WITSML21_DEBUG("textBufAlloc: try to create new text buffer of length %lu", len)
      if ((js_cws_config->str=(char *)cws_malloc((js_cws_config->str_size=(len+1))))) {
        js_cws_config->str_len=len;
        *sz=js_cws_config->str_size;
        JS_WITSML21_DEBUG("textBufAlloc: try to create text buffer of length %lu. Success at %p with size %lu", len, js_cws_config->str, js_cws_config->str_size)
        return js_cws_config->str;
      }
      js_cws_config->str_len=0;
      *sz=js_cws_config->str_size=0;
      JS_WITSML21_DEBUG("textBufAlloc: Unable to alloc new text buffer.")
    }
  }

  return NULL;
}

static char *js_cws_get_value_string_utf8(size_t *len, napi_env env, napi_value value, struct js_cws_config_t *js_cws_config)
{
  char *res;
  size_t sz;

  if (napi_get_value_string_utf8(env, value, NULL, 0, len)!=napi_ok)
    return NULL;

  if (!(res=textBufAlloc(&sz, js_cws_config, *len)))
    return NULL;

  if (napi_get_value_string_utf8(env, value, res, sz, len)==napi_ok) {
    JS_WITSML21_DEBUG("js_cws_get_value_string_utf8: has NULL terminator: %d\n", res[*len]==0)
    JS_WITSML21_DEBUG("js_cws_get_value_string_utf8: value: %s\n", res)
    return res;
  }

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

  JS_WITSML21_DEBUG("js_cws_config_free: Begin free if is NOT NULL %p", *js_cws_config)

  if (*js_cws_config) {
    config=(CWS_CONFIG *)(*js_cws_config)->soap_internal->user;

    JS_WITSML21_DEBUG("js_cws_config_free: Destroying soap_internal %p", (*js_cws_config)->soap_internal)
    cws_internal_soap_free(&(*js_cws_config)->soap_internal);

    JS_WITSML21_DEBUG("js_cws_config_free: Destroying config %p", config)
    cws_config_free(&config);

    if ((*js_cws_config)->str) {
      JS_WITSML21_DEBUG(
        "js_cws_config_free: Freeing draft string buffer %p of length %lu of size %lu",
        (*js_cws_config)->str, (*js_cws_config)->str_len, (*js_cws_config)->str_size
      )
      free((*js_cws_config)->str);
      (*js_cws_config)->str=NULL;
    }

    JS_WITSML21_DEBUG("js_cws_config_free: Destroying js_cws_config object %p", js_cws_config)
    free((void *)*js_cws_config);
    *js_cws_config=NULL;

    JS_WITSML21_DEBUG("js_cws_config_free: Destroyed")
  }

}

static struct js_cws_config_t *js_cws_config_init()
{
  CWS_CONFIG *config;
  struct js_cws_config_t *js_cws_config;

  JS_WITSML21_DEBUG("js_cws_config_init: Initializing ...")
  if (!(js_cws_config=(struct js_cws_config_t *)malloc(sizeof(struct js_cws_config_t))))
    return NULL;

  if (!(config=cws_config_new("JSWITSML 2.1 BSON parser")))
    goto js_cws_config_init_exit1;

  if (cws_internal_soap_new(&js_cws_config->soap_internal, config, NULL))
    goto js_cws_config_init_exit2;

  js_cws_config->err=0;
  js_cws_config->str=NULL;
  js_cws_config->str_len=0;
  js_cws_config->str_size=0;

  JS_WITSML21_DEBUG("js_cws_config_init: SUCCESS ... %p", js_cws_config)

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
static void cws_js_finalize(napi_env env, void *finalize_data, void *finalize_hint)
{
  void *data=finalize_data;
  JS_WITSML21_DEBUG("\ncws_js_finalize: Entering and perform cleanup %p", data)

  js_cws_config_free((struct js_cws_config_t **)&data);

}

static void cws_js_throw(napi_env env, struct cws_js_err_t *caster, const char *c_function_name, const char *errMessage, int err)
{
  snprintf(caster->buf, sizeof(caster->buf), "%s:Err.:%d %s", c_function_name, err, errMessage);
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

static int cws_add_int32_util(napi_env env, napi_value exports, struct cws_js_int32_t *value)
{
  napi_value int32;

  while (value->name) {
    if (napi_create_int32(env, value->value, &int32)!=napi_ok)
      return 600;

    if (napi_set_named_property(env, exports, value->name, int32)!=napi_ok)
      return 601;

    value++;
  }

  return 0;
}

static int cws_add_int32_object_list_util(napi_value *int32_out, const char *objName, napi_env env, napi_value exports, struct cws_js_int32_t *value)
{
  napi_value obj, *objTmp;

  objTmp=(int32_out)?int32_out:&obj;

  if (napi_create_object(env, objTmp)!=napi_ok)
    return 602;

  if (napi_set_named_property(env, exports, objName, *objTmp)!=napi_ok)
    return 603;

  return cws_add_int32_util(env, obj, value);
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
    "Can't parse arguments. Wrong argument at c_parseFromFile",
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
    (ERR=((filename=js_cws_get_value_string_utf8(&filenameLen, env, argv, js_cws_instance))==NULL)),
    "c_parseFromFile",
    "Could not parse filename. Wrong format or empty string or invalid utf-8 or no space in C string buffer",
    124
  )

  JS_CWS_THROW_COND(
    (ERR=readText((const char **)&text, &textLen, filename)),
    "readText", "Could not read xml/text",
    ERR
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=c_parse_util(js_cws_instance->soap_internal, (void **)&bson_ser, text, textLen, IS_BSON)),
    "c_parse_util", "BSON parse error @ c_parseFromFile",
    ERR, c_parseFromFile_exit1
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=js_cws_new_array_buffer(&res, env, (void *)bson_ser->bson, bson_ser->bson_size)),
    "js_cws_new_array_buffer", "Could not copy BSON bytes to JavaScript array buffer @ c_parseFromFile",
    ERR, c_parseFromFile_exit1
  )

  free((void *)text);

  return res;

c_parseFromFile_exit1:
  free((void *)text);

  return NULL;
}

napi_value c_parseFromFileJSON(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  char *filename, *text;
  size_t argc=1, filenameLen, textLen;
  struct cws_js_err_t cws_js_err;
  struct c_json_str_t *json_ser;
  struct js_cws_config_t *js_cws_instance;

  JS_CWS_THROW_COND(
    (ERR=(napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok)),
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_parseFromFileJSON",
    140
  )

  JS_CWS_THROW_COND(
    (ERR=(js_cws_instance==NULL)),
    "c_parseFromFileJSON",
    "Fatal: js_cws_instance. Was expected NOT NULL",
    141
  )

  JS_CWS_THROW_COND(
    (ERR=(argc==0)),
    "c_parseFromFileJSON",
    "Missing argument. Was expected: file path/filename",
    142
  )

  JS_CWS_THROW_COND(
    (ERR=(argc>1)),
    "c_parseFromFileJSON",
    "Too many arguments",
    143
  )

  JS_CWS_THROW_COND(
    (ERR=((filename=js_cws_get_value_string_utf8(&filenameLen, env, argv, js_cws_instance))==NULL)),
    "c_parseFromFileJSON",
    "Could not parse filename. Wrong format or empty string or invalid utf-8 or no space in C string buffer",
    144
  )

  JS_CWS_THROW_COND(
    (ERR=readText((const char **)&text, &textLen, filename)),
    "readText", "Could not read xml/text @ c_parseFromFileJSON",
    ERR
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=c_parse_util(js_cws_instance->soap_internal, (void **)&json_ser, text, textLen, IS_JSON)),
    "c_parse_util", "BSON parse error @ c_parseFromFileJSON",
    ERR, c_parseFromFileJSON_exit1
  )

  JS_CWS_THROW_COND_GOTO(
    (ERR=(napi_create_string_utf8(env, json_ser->json, json_ser->json_len, &res)!=napi_ok)),
    "napi_create_string_utf8",
    "napi_create_string_utf8 @ c_parseFromFileJSON. Error on parsing JSON string",
    145, c_parseFromFileJSON_exit1
  )

  free((void *)text);

  return res;

c_parseFromFileJSON_exit1:
  free((void *)text);

  return NULL;
}

napi_value c_saveToFile(napi_env env, napi_callback_info info)
{
  int err;
  napi_value argv=NULL, res;
  size_t argc=1, filenameLen;
  char *filename;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_saveToFile",
    150
  )

  JS_CWS_THROW_COND(argc==0, "c_saveToFile", "Missing arguments. Expected: Output filename", 151)

  JS_CWS_THROW_COND(argc>1, "c_saveToFile", "Too many arguments. Expected only one argument: output filename", 152)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_saveToFile",
    "Fatal: js_cws_instance @ c_saveToFile. Was expected NOT NULL",
    153
  )

  CHECK_HAS_ERROR("c_saveToFile", "Could not save BSON serialized to file for this object. Object not found or error on parsing", 154)

  JS_CWS_THROW_COND(
    (filename=js_cws_get_value_string_utf8(&filenameLen, env, argv, js_cws_instance))==NULL,
    "c_saveToFile",
    "Could not parse filename. Wrong format or empty string or invalid utf-8 or no space in C string buffer",
    155
  )

  JS_CWS_THROW_COND(
    (err=writeToFile(js_cws_instance->soap_internal, filename)),
    "writeToFile", "Could not save BSON file @ c_saveToFile",
    err
  )

  JS_CWS_RETURN_UNDEFINED
}

napi_value c_saveToFileJSON(napi_env env, napi_callback_info info)
{
  int err;
  napi_value argv=NULL, res;
  size_t argc=1, filenameLen;
  char *filename;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_saveToFileJSON",
    160
  )

  JS_CWS_THROW_COND(argc==0, "c_saveToFileJSON", "Missing arguments. Expected: Output filename", 161)

  JS_CWS_THROW_COND(argc>1, "c_saveToFileJSON", "Too many arguments. Expected only one argument: output filename", 162)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_saveToFileJSON",
    "Fatal: js_cws_instance @ c_saveToFileJSON. Was expected NOT NULL",
    163
  )

  CHECK_HAS_ERROR("c_saveToFileJSON", "Could not save JSON serialized to file for this object. Object not found or error on parsing", 154)

  JS_CWS_THROW_COND(
    (filename=js_cws_get_value_string_utf8(&filenameLen, env, argv, js_cws_instance))==NULL,
    "c_saveToFileJSON",
    "Could not parse filename. Wrong format or empty string or invalid utf-8 or no space in C string buffer",
    165
  )

  JS_CWS_THROW_COND(
    (err=writeToFileJSON(js_cws_instance->soap_internal, filename)),
    "writeToFileJSON", "Could not save JSON file @ writeToFileJSON",
    err
  )

  JS_CWS_RETURN_UNDEFINED
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

napi_value c_getObjectName(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getObjectName",
    40
  )

  JS_CWS_THROW_COND(argc, "c_getObjectName", "Too many arguments @ c_getObjectName", 41)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getObjectName",
    "Fatal: js_cws_instance @ c_getObjectName. Was expected NOT NULL",
    42
  )

  CHECK_HAS_ERROR("c_getObjectName", "Could not get object name. Object not parsed or internal error", 43)

  JS_CWS_THROW_COND(
    (napi_create_string_utf8(env, JS_GET_OBJECT_NAME, NAPI_AUTO_LENGTH, &res)!=napi_ok),
    "napi_create_string_utf8",
    "napi_create_string_utf8 @ c_getObjectName. Error on get object name",
    44
  )

  return res;
}

napi_value c_getObjectType(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getObjectType",
    50
  )

  JS_CWS_THROW_COND(argc, "c_getObjectType", "Too many arguments @ c_getObjectType", 51)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getObjectType",
    "Fatal: js_cws_instance @ c_getObjectType. Was expected NOT NULL",
    52
  )

  CHECK_HAS_ERROR("c_getObjectType", "Could not get object name. Object not parsed or internal error", 53)

  JS_CWS_THROW_COND(
    (napi_create_int32(env, JS_GET_OBJECT_TYPE, &res)!=napi_ok),
    "napi_create_int32",
    "napi_create_int32 @ c_getObjectType. Error on get object type",
    54
  )

  return res;
}

napi_value c_getError(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getError",
    55
  )

  JS_CWS_THROW_COND(argc, "c_getError", "Too many arguments @ c_getError", 56)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getError",
    "Fatal: js_cws_instance @ c_getError. Was expected NOT NULL",
    57
  )

  JS_CWS_THROW_COND(
    (napi_create_int32(env, (JS_GET_ERROR)?JS_GET_ERROR:ERR, &res)!=napi_ok),
    "napi_create_int32",
    "napi_create_int32 @ c_getError. Unable to get Witsml 2.1 parser error",
    58
  )

  return res;
}

napi_value c_getBsonBytes(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;
  struct c_bson_serialized_t *bson_ser;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getBsonBytes",
    70
  )

  JS_CWS_THROW_COND(argc, "c_getBsonBytes", "Too many arguments @ c_getBsonBytes", 71)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getBsonBytes",
    "Fatal: js_cws_instance @ c_getBsonBytes. Was expected NOT NULL",
    72
  )

  CHECK_HAS_ERROR("c_getBsonBytes", "Could not get BSON serialized. Object not parsed or internal error", 73)

  JS_CWS_THROW_COND(
    (bson_ser=bsonSerialize(js_cws_instance->soap_internal))==NULL,
    "bsonSerialize",
    "bsonSerialize @ c_getBsonBytes. Unable to load parsed BSON binary data",
    74
  )

  JS_CWS_THROW_COND(
    js_cws_new_array_buffer(&res, env, (void *)bson_ser->bson, bson_ser->bson_size),
    "napi_create_int32",
    "napi_create_int32 @ c_getBsonBytes. Unable to copy BSON to JavaScript ArrayBuffer",
    75
  )

  return res;
}

napi_value c_getJson(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;
  struct c_json_str_t *c_json;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getJson",
    80
  )

  JS_CWS_THROW_COND(argc, "c_getJson", "Too many arguments @ c_getJson", 81)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getJson",
    "Fatal: js_cws_instance @ c_getJson. Was expected NOT NULL",
    82
  )

  CHECK_HAS_ERROR("c_getJson", "Could not get JSON string. Object not parsed or internal error", 83)

  JS_CWS_THROW_COND(
    (c_json=getJson(js_cws_instance->soap_internal))==NULL,
    "getJson",
    "getJson @ c_getJson. Unable to load parsed JSON string",
    84
  )

  JS_CWS_THROW_COND(
    (napi_create_string_utf8(env, c_json->json, c_json->json_len, &res)!=napi_ok),
    "napi_create_int32",
    "napi_create_int32 @ c_getJson. Unable to copy JSON string",
    85
  )

  return res;
}

napi_value c_getStatistics(napi_env env, napi_callback_info info)
{
  napi_value argv=NULL, res;
  int err;
  size_t argc=0;
  struct js_cws_config_t *js_cws_instance;
  struct cws_js_err_t cws_js_err;
  struct statistics_t *stat;

  JS_CWS_THROW_COND(
    napi_get_cb_info(env, info, &argc, &argv, NULL, (void **)&js_cws_instance)!=napi_ok,
    "napi_get_cb_info",
    "Can't parse arguments. Wrong argument at c_getStatistics",
    90
  )

  JS_CWS_THROW_COND(argc, "c_getStatistics", "Too many arguments @ c_getStatistics", 91)

  JS_CWS_THROW_COND(
    (js_cws_instance==NULL),
    "c_getStatistics",
    "Fatal: js_cws_instance @ c_getStatistics. Was expected NOT NULL",
    92
  )

  CHECK_HAS_ERROR("c_getStatistics", "Could not get statistics. Object not parsed or internal error", 93)

  JS_CWS_THROW_COND(
    (napi_create_object(env, &res)!=napi_ok),
    "napi_create_object",
    "napi_create_object @ c_getStatistics. Unable to create object",
    94
  )

  JS_CWS_THROW_COND(
    (err=cws_set_uint32_stat_list(env, res, (stat=getStatistics(js_cws_instance->soap_internal)), JS_CWS_UINT32_STAT)),
    "cws_set_uint32_stat_list",
    "cws_set_uint32_stat_list @ c_getStatistics. Unable to parse statistic to JavaScript object",
    err
  )

_Static_assert(sizeof(uint64_t)>=sizeof(size_t), "Archtecture error. Refactor it");

  JS_CWS_THROW_COND(
    (err=cws_set_uint64_stat_list(env, res, stat, JS_CWS_UINT64_STAT)),
    "cws_set_uint64_stat_list",
    "cws_set_uint64_stat_list @ c_getStatistics. Unable to parse statistic to JavaScript object",
    err
  )

  return res;
}

CWS_JS_FUNCTIONS_OBJ CWS_JS_CREATE_FUNCTIONS[] = {
  SET_JS_FN_CALL(getInstanceName),
  SET_JS_FN_CALL(getObjectName),
  SET_JS_FN_CALL(getObjectType),
  SET_JS_FN_CALL(getBsonBytes),
  SET_JS_FN_CALL(getJson),
  SET_JS_FN_CALL(getStatistics),
  SET_JS_FN_CALL(getError),
  SET_JS_FN_CALL(parseFromFile),
  SET_JS_FN_CALL(parseFromFileJSON),
  SET_JS_FN_CALL(saveToFile),
  SET_JS_FN_CALL(saveToFileJSON),
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
    (err=cws_add_int32_object_list_util(NULL, "WITSML21_TYPES_ENUM", env, exports, CWS_JS_INT32)),
    "cws_add_int32_object_list_util",
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

