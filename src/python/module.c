#define PY_SSIZE_T_CLEAN
#include "../../ns1.nsmap" // XML namespace mapping table (only needed once at the global level)
#include "../../soapH.h"    // server stubs, serializers, etc.
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <cws_utils.h>
#include <Python.h>
//https://docs.python.org/3/c-api/intro.html#useful-macros
#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

#define ERR ((Py_WITSML21_PARSER *)self)->err
#define CHECK_HAS_ERROR(msgOnError, retn) \
  if ((ERR!=0)||(GET_INTERNAL_SOAP_ERROR!=0)||(GET_OBJECT_TYPE==TYPE_None)) {\
    PyErr_SetString(PyExc_Exception, msgOnError);\
    return retn;\
  }

typedef struct {
  PyObject_HEAD
  int err;
  struct soap *soap_internal;
} Py_WITSML21_PARSER;

struct const_t {
   const char *name;
   int value;
} CONST[] = {
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

#ifdef PY_WITSML21DEBUG
 #define Py_WITSML21_DEBUG(std, ...) \
    fprintf(std, __VA_ARGS__);
#else
 #define Py_WITSML21_DEBUG(std, ...)
#endif

#define Py_WITSML21_ERROR(err_msg, errNumber) \
 {\
   PyErr_SetString(PyExc_Exception, err_msg);\
    return errNumber;\
 }

static PyObject *c_obj_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *self;

  if (!(self=type->tp_alloc(type, 0)))
    Py_WITSML21_ERROR("Could not alloc Py_WITSML21_PARSER object", NULL)

  ((Py_WITSML21_PARSER *)self)->err=0;
  ((Py_WITSML21_PARSER *)self)->soap_internal=NULL;

  Py_WITSML21_DEBUG(stdout, "New object self created at %p\n", self)

  return (PyObject *)self;
}

static int c_obj_init(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  CWS_CONFIG *config=cws_config_new("PyWITSML 2.1 BSON parser");

  if (!config)
    Py_WITSML21_ERROR("Could not initialize CWS config", -500)

  Py_WITSML21_DEBUG(stdout, "Initializing config %p\n", config)

  if (cws_internal_soap_new(&((Py_WITSML21_PARSER *)self)->soap_internal, config, NULL)) {
    cws_config_free(&config);
    PyErr_SetString(PyExc_Exception, "Could not initialize cws soap internal");
    return -501;
  }

  Py_WITSML21_DEBUG(stdout, "Initializing soap internal %p\n", ((Py_WITSML21_PARSER *)self)->soap_internal)

  return 0;
}

static void c_obj_dealloc(void *self)
{
  CWS_CONFIG *config;

  if (((Py_WITSML21_PARSER *)self)->soap_internal) {
    config=(CWS_CONFIG *)((Py_WITSML21_PARSER *)self)->soap_internal->user;

    Py_WITSML21_DEBUG(stdout, "Destroying soap internal %p\n", ((Py_WITSML21_PARSER *)self)->soap_internal)
    cws_internal_soap_free(&((Py_WITSML21_PARSER *)self)->soap_internal);
    Py_WITSML21_DEBUG(stdout, "Destroyed soap internal %p\n", ((Py_WITSML21_PARSER *)self)->soap_internal)

    Py_WITSML21_DEBUG(stdout, "Destroying config %p\n", config)
    cws_config_free(&config);
    Py_WITSML21_DEBUG(stdout, "Destroyed config %p\n", config)
  }
}

enum c_parse_util_e {
  IS_BSON=1,
  IS_JSON
};

static int c_parse_util(
  struct soap *soap_internal, 
  void **v_ser,
  const char *c_xml,
  Py_ssize_t c_xml_size,
  const char **errMsg,
  enum c_parse_util_e c_parse_util 
)
{

  DECLARE_CONFIG(soap_internal)
  *v_ser=NULL;

  Py_WITSML21_DEBUG(stdout, "\nSoap internal %p\n", soap_internal)

  if (c_xml_size<1) {
    *errMsg="Wrong xml size or empty xml string";
    return -100;
  }

  cws_internal_soap_recycle(soap_internal);
  cws_recycle_config(config);

  if (!cws_parse_XML_soap_envelope(soap_internal, (char *)c_xml, (size_t)c_xml_size)) {
    *errMsg="Could not parse xml string. See xml errors for details";
    return -101;
  }

  if (cws_soap_serve(soap_internal)) {
    *errMsg="Could not deserialize xml string. See xml errors for details";
    return -102;
  }

  if (c_parse_util==IS_BSON) {
    if ((*v_ser=(void *)bsonSerialize(soap_internal))) {
      *errMsg="";
      return 0;
    }

    *errMsg="Could not serialize BSON";
  } else {
    if ((*v_ser=(void *)getJson(soap_internal))) {
      *errMsg="";
      return 0;
    }
    *errMsg="Could not serialize JSON";
  }

  return -103;
}

static PyObject *c_parse(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  struct c_bson_serialized_t *bson_ser;
  char
   *kwlist[] = {"witsml21_xml", NULL};

  const char
    *c_xml=NULL,
    *errMsg;
  Py_ssize_t c_xml_size=0;

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "s#", kwlist,
    &c_xml, &c_xml_size
  )) Py_WITSML21_ERROR("Error on parse WITSML 2.1 xml", NULL)

  if ((ERR=c_parse_util(soap_internal, (void **)&bson_ser, c_xml, c_xml_size, &errMsg, IS_BSON)))
    Py_WITSML21_ERROR(errMsg, NULL)

  return Py_BuildValue("y#", (const char *)bson_ser->bson, (Py_ssize_t)bson_ser->bson_size);
}

static PyObject *c_parse_from_file(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct c_bson_serialized_t *bson_ser;
  char msgBuf[512];
  char
   *kwlist[] = {"witsml21_xml_file", NULL};

  const char
    *c_xml_file=NULL,
    *text,
    *errMsg;

  size_t text_len;

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "s", kwlist,
    &c_xml_file
  )) Py_WITSML21_ERROR("Error on parse WITSML 2.1 xml", NULL)

  if ((ERR=readText(&text, &text_len, c_xml_file))) {
    snprintf(msgBuf, sizeof(msgBuf), "Error %d. Could not open filename '%s'", ERR, c_xml_file);
    PyErr_SetString(PyExc_Exception, msgBuf);
    return NULL;
  }

  if ((ERR=c_parse_util(((Py_WITSML21_PARSER *)self)->soap_internal, (void **)&bson_ser, text, text_len, &errMsg, IS_BSON))) {
    free((void *)text);
    snprintf(msgBuf, sizeof(msgBuf), "Error %d. %s", ERR, errMsg);
    PyErr_SetString(PyExc_Exception, msgBuf);
    return NULL;
  }

  free((void *)text);
  return Py_BuildValue("y#", (const char *)bson_ser->bson, (Py_ssize_t)bson_ser->bson_size);
}

static PyObject *c_parse_to_json(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct c_json_str_t *json_ser;
  char msgBuf[512];
  char
   *kwlist[] = {"witsml21_xml", NULL};

  const char
    *c_xml_file=NULL,
    *errMsg;

  Py_ssize_t c_xml_len=0;

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "s#", kwlist,
    &c_xml_file, &c_xml_len
  )) Py_WITSML21_ERROR("c_parse_to_json: Error on parse WITSML 2.1 xml", NULL)

  if ((ERR=c_parse_util(((Py_WITSML21_PARSER *)self)->soap_internal, (void **)&json_ser, c_xml_file, c_xml_len, &errMsg, IS_JSON))) {
    snprintf(msgBuf, sizeof(msgBuf), "c_parse_to_json: Error %d. %s", ERR, errMsg);
    PyErr_SetString(PyExc_Exception, msgBuf);
    return NULL;
  }

  return Py_BuildValue("s#", (const char *)json_ser->json, (Py_ssize_t)json_ser->json_len);
}

static PyObject *c_parse_to_json_from_file(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct c_json_str_t *json_ser;
  char msgBuf[512];
  char
   *kwlist[] = {"witsml21_xml_file", NULL};

  const char
    *c_xml_file=NULL,
    *text,
    *errMsg;

  size_t text_len;

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "s", kwlist,
    &c_xml_file
  )) Py_WITSML21_ERROR("c_parse_to_json_from_file: Error on parse WITSML 2.1 xml", NULL)

  if ((ERR=readText(&text, &text_len, c_xml_file))) {
    snprintf(msgBuf, sizeof(msgBuf), "c_parse_to_json_from_file: Error %d. Could not open filename '%s'", ERR, c_xml_file);
    PyErr_SetString(PyExc_Exception, msgBuf);
    return NULL;
  }

  if ((ERR=c_parse_util(((Py_WITSML21_PARSER *)self)->soap_internal, (void **)&json_ser, text, text_len, &errMsg, IS_JSON))) {
    free((void *)text);
    snprintf(msgBuf, sizeof(msgBuf), "c_parse_to_json_from_file: Error %d. %s", ERR, errMsg);
    PyErr_SetString(PyExc_Exception, msgBuf);
    return NULL;
  }

  free((void *)text);
  return Py_BuildValue("s#", (const char *)json_ser->json, (Py_ssize_t)json_ser->json_len);
}

static PyObject *c_get_object_name(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  CHECK_HAS_ERROR("Could not get object name. Object not parsed or internal error", NULL)
  return Py_BuildValue("z", GET_OBJECT_NAME);
}

static PyObject *c_get_object_type(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  CHECK_HAS_ERROR("Could not get object type. Object not parsed or internal error", NULL)
  return PyLong_FromLong((long int)GET_OBJECT_TYPE);
}

static PyObject *c_get_instance_name(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  return Py_BuildValue("s", (const char *)GET_INSTANCE_NAME);
}

static PyObject *c_get_error(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  return PyLong_FromLong((long int)(GET_INTERNAL_SOAP_ERROR)?GET_INTERNAL_SOAP_ERROR:ERR);
}

static PyObject *c_get_faultstring(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  DECLARE_CONFIG(soap_internal)
  return Py_BuildValue("z#", (const char *)config->cws_soap_fault.faultstring, (Py_ssize_t)config->cws_soap_fault.faultstring_len);
}

static PyObject *c_get_XMLfaultdetail(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  DECLARE_CONFIG(soap_internal)
  return Py_BuildValue("z#", (const char *)config->cws_soap_fault.XMLfaultdetail, (Py_ssize_t)config->cws_soap_fault.XMLfaultdetail_len);
}

static PyObject *c_get_bson_version(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct cws_version_t version;
  cws_version(&version);
  return Py_BuildValue("y#", (const char *)version.version, (Py_ssize_t)version.versionSize);
}

static PyObject *c_get_bson_bytes(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  struct c_bson_serialized_t *bson_ser;

  CHECK_HAS_ERROR("c_get_bson_bytes: Could not get BSON serialized for this object. Object not found or error on parsing", NULL)

  if ((bson_ser=bsonSerialize(soap_internal)))
    return Py_BuildValue("y#", (const char *)bson_ser->bson, (Py_ssize_t)bson_ser->bson_size);

  PyErr_SetString(PyExc_Exception, "c_get_bson_bytes: Could not parse bson serialized. Maybe libbson out of memory");
  return NULL;
}

static PyObject *c_get_json(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  struct c_json_str_t *c_json;

  CHECK_HAS_ERROR("c_get_json: Could not get JSON string for this object. Object not found or error on parsing", NULL)

  if ((c_json=getJson(soap_internal)))
    return Py_BuildValue("s#", (const char *)c_json->json, (Py_ssize_t)c_json->json_len);

  PyErr_SetString(PyExc_Exception, "c_get_json: Could not parse JSON string. Maybe libbson out of memory");
  return NULL;
}

static PyObject *c_save_to_file(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  char
   *kwlist[] = {"bson_filename", NULL};

  const char
    *c_xml_file=NULL;

  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;

  CHECK_HAS_ERROR("c_save_to_file: Could not save BSON serialized to file for this object. Object not found or error on parsing", NULL)

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "|z", kwlist,
    &c_xml_file
  )) Py_WITSML21_ERROR("c_save_to_file: Error on parse output BSON filename", NULL)

  if (!writeToFile(soap_internal, c_xml_file))
    return Py_None;

  PyErr_SetString(PyExc_Exception, "c_save_to_file: Could not save BSON to file.");
  return NULL;
}

static PyObject *c_save_json_to_file(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  char
   *kwlist[] = {"json_filename", NULL};

  const char
    *c_xml_file=NULL;

  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;

  CHECK_HAS_ERROR("c_save_json_to_file: Could not save JSON string to file for this object. Object not found or error on parsing", NULL)

  if (!PyArg_ParseTupleAndKeywords(
    args, kwds, "|z", kwlist,
    &c_xml_file
  )) Py_WITSML21_ERROR("c_save_json_to_file: Error on parse output JSON string filename", NULL)

  if (!writeToFileJSON(soap_internal, c_xml_file))
    return Py_None;

  PyErr_SetString(PyExc_Exception, "c_save_json_to_file: Could not save JSON string to file.");
  return NULL;
}

#ifdef WITH_STATISTICS
#define Py_SET_STAT(k) #k, (long int)stat->k
static PyObject *c_get_statistics(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  struct statistics_t *stat;

  CHECK_HAS_ERROR("Statistics for this object not found or error on parsing", NULL)

  stat=getStatistics(soap_internal);

  return Py_BuildValue(
    "{sIsIsIsIsIsIsIsIsIsIsIsIsIsI}",
    Py_SET_STAT(costs),
    Py_SET_STAT(strings),
    Py_SET_STAT(shorts),
    Py_SET_STAT(ints),
    Py_SET_STAT(long64s),
    Py_SET_STAT(enums),
    Py_SET_STAT(arrays),
    Py_SET_STAT(booleans),
    Py_SET_STAT(doubles),
    Py_SET_STAT(date_times),
    Py_SET_STAT(measures),
    Py_SET_STAT(event_types),
    Py_SET_STAT(total),
    Py_SET_STAT(used_memory)
  );

}
#undef Py_SET_STAT
#endif

#undef CHECK_HAS_ERROR
#undef ERR

static PyMethodDef py_witsml21_methods[] = {
  {"parse", (PyCFunction)c_parse, METH_VARARGS|METH_KEYWORDS, "Parses a WITSML 2.1 XML to BSON"},
  {"parseToJSON", (PyCFunction)c_parse_to_json, METH_VARARGS|METH_KEYWORDS, "Parses a WITSML 2.1 XML to JSON string"},
  {"parseFromFile", (PyCFunction)c_parse_from_file, METH_VARARGS|METH_KEYWORDS, "Parses a WITSML 2.1 XML to BSON from file"},
  {"parseToJSONFromFile", (PyCFunction)c_parse_to_json_from_file, METH_VARARGS|METH_KEYWORDS, "Parses a WITSML 2.1 XML to JSON string from file"},
  {"getBsonVersion", (PyCFunction)c_get_bson_version, METH_NOARGS, "Get WITSML 2.1 API C version"},
  {"getObjectName", (PyCFunction)c_get_object_name, METH_NOARGS, "Get WITSML 2.1 object name"},
  {"getObjectType", (PyCFunction)c_get_object_type, METH_NOARGS, "Get WITSML 2.1 object type"},
  {"getInstanceName", (PyCFunction)c_get_instance_name, METH_NOARGS, "Get WITSML 2.1 instance name and its C config pointer"},
  {"getError", (PyCFunction)c_get_error, METH_NOARGS, "Get WITSML 2.1 error"},
  {"getFaultString", (PyCFunction)c_get_faultstring, METH_NOARGS, "Get WITSML 2.1 XML error 'faultstring' description"},
  {"getXMLfaultdetail", (PyCFunction)c_get_XMLfaultdetail, METH_NOARGS, "Get WITSML 2.1 XML error 'XMLfaultdetail' description"},
  {"getBsonBytes", (PyCFunction)c_get_bson_bytes, METH_NOARGS, "Get WITSML 2.1 objects in BSON serialized"},
  {"getJson", (PyCFunction)c_get_json, METH_NOARGS, "Get WITSML 2.1 objects in JSON string"},
  {"saveToFile", (PyCFunction)c_save_to_file, METH_VARARGS|METH_KEYWORDS, "Saves WITSML 2.1 objects to BSON file"},
  {"saveToFileJSON", (PyCFunction)c_save_json_to_file, METH_VARARGS|METH_KEYWORDS, "Saves WITSML 2.1 objects to JSON string file"},
#ifdef WITH_STATISTICS
  {"getStatistics", (PyCFunction)c_get_statistics, METH_NOARGS, "Get WITSML 2.1 xml document statistics"},
#endif
  {NULL}
};

static PyTypeObject Py_WITSML21_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name="WITSML 2.1 to BSON parser",
  .tp_doc="Creates a WITSML 2.1 to BSON parser instance",
  .tp_basicsize=sizeof(Py_WITSML21_PARSER),
  .tp_itemsize=0,
  .tp_flags=Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
  .tp_new=c_obj_new,
  .tp_init=(initproc)c_obj_init,
  .tp_dealloc=(destructor)c_obj_dealloc,
  .tp_methods=py_witsml21_methods
};

static PyModuleDef Py_WITSML21_module = {
  PyModuleDef_HEAD_INIT,
  .m_name="WITSML 2.1 to BSON parser module",
  .m_doc="WITSML 2.1 to BSON parser module for Python 3 using C library",
  .m_size=-1,
};

PyMODINIT_FUNC PyInit_witsml21bson(void)
{
  PyObject *m;
  struct const_t *c;

  Py_WITSML21_DEBUG(stdout, "Check is WITSML 2.1 parser is ready\n")
  if (PyType_Ready(&Py_WITSML21_Type)<0)
    Py_WITSML21_ERROR("\n\"Can't initialize module WITSML 2.1 parser\"\n", NULL)

  Py_WITSML21_DEBUG(stdout, "Creating module ...\n")
  if (!(m=PyModule_Create(&Py_WITSML21_module)))
    Py_WITSML21_ERROR("\nCannot create module \"Py_WITSML21_module\"\n", NULL)

  Py_WITSML21_DEBUG(stdout, "Module %p created\n", m);
  if (PyModule_AddObject(m, "create", (PyObject *) &Py_WITSML21_Type)<0)
    Py_WITSML21_ERROR("\nCannot create module \"WITSML 2.1 to BSON parser\" from \"Py_WITSML21_Type\"\n", NULL)

  c=CONST;
  while (c->name) {
    if (PyModule_AddIntConstant(m, c->name, (long int)c->value))
      Py_WITSML21_ERROR("Could not add const value", NULL)

    c++;
  }

  return m;
}

/////////////////////////////////////////////////////// C Witsml Parser ///////////////////////////////////////////////////////

C_WITSML21_PARSER_BUILD

