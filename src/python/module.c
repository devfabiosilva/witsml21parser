#define PY_SSIZE_T_CLEAN
#include "../../ns1.nsmap" // XML namespace mapping table (only needed once at the global level)
#include "../../soapH.h"    // server stubs, serializers, etc.
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <cws_utils.h>
#include <Python.h>

#ifndef WITH_NOIDREF
  #error "Error. Declare first line below in stdsoap2.h #define WITH_NOIDREF"
#endif

typedef struct {
  PyObject_HEAD
  struct soap *soap_internal;
} Py_WITSML21_PARSER;

struct const_t {
   const char *name;
   int value;
} CONST[] = {
   {"ALG_1", 1},
   {"ALG_2", 2},
   {"ALG_3", 3},
   {NULL}
};

//#ifdef SOAP_DEBUG
 #define Py_WITSML21_DEBUG(std, ...) \
    fprintf(std, __VA_ARGS__);
//#else
// #define Py_WITSML21_DEBUG(std, ...)
//#endif

#define Py_WITSML21_ERROR(err_msg, errNumber) \
 {\
   PyErr_SetString(PyExc_Exception, err_msg);\
    return errNumber;\
 }

static PyObject *c_obj_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *self=NULL;

  if (!(self=type->tp_alloc(type, 0)))
    Py_WITSML21_ERROR("Could not alloc Py_WITSML21_PARSER object", NULL)

  ((Py_WITSML21_PARSER *)self)->soap_internal=NULL;

  Py_WITSML21_DEBUG(stdout, "New object self created at %p\n", self)

  return (PyObject *)self;
}

static int c_obj_init(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
  //struct soap *soap_internal=((Py_WITSML21_PARSER *)self)->soap_internal;
  CWS_CONFIG *config=cws_config_new("Python3 Init");

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
  DECLARE_CONFIG(((Py_WITSML21_PARSER *)self)->soap_internal)

  Py_WITSML21_DEBUG(stdout, "Destroying soap internal %p\n", ((Py_WITSML21_PARSER *)self)->soap_internal)
  cws_internal_soap_free(&((Py_WITSML21_PARSER *)self)->soap_internal);
  Py_WITSML21_DEBUG(stdout, "Destroyed soap internal %p\n", ((Py_WITSML21_PARSER *)self)->soap_internal)

  Py_WITSML21_DEBUG(stdout, "Destroying config %p\n", config)
  cws_config_free(&config);
  Py_WITSML21_DEBUG(stdout, "Destroyed config %p\n", config)
}

static PyObject *c_parse(PyTypeObject *self, PyObject *args, PyObject *kwds)
{
   return Py_BuildValue("s", "Test");
}

static PyMethodDef py_witsml21_methods[] = {
  {"parseA", (PyCFunction)c_parse, METH_NOARGS/*METH_VARARGS|METH_KEYWORDS*/, "Parses a WITSML 2.1 XML to BSON"},
  {NULL}
};

static PyTypeObject Py_WITSML21_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name="WITSML 2.1 to BSON parser",
  .tp_doc="This module implements WITSML 2.1 parser",
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

