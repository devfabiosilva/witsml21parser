#define PY_SSIZE_T_CLEAN
#include "../../ns1.nsmap" // XML namespace mapping table (only needed once at the global level)
#include "../../soapH.h"    // server stubs, serializers, etc.
#include <request_context.h>
#include <cws_soap.h>
#include <wistml2bson_deserializer.h>
#include <cws_utils.h>
#include <Python.h>

typedef struct {
  PyObject_HEAD
  struct soap *soap_internal;
} Py_WITSML21_PARSER;


#ifdef SOAP_DEBUG
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

static PyMethodDef py_witsml21_methods[] = {
  {"parse", (PyCFunction)parse, METH_VARARGS|METH_KEYWORDS, "Parses a WITSML 2.1 XML to BSON"},
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

PyMODINIT_FUNC PyInit_witsml2bson(void)
{
   PyObject *m;

   PANEL_DEBUG(stdout, "Check is WITSML 2.1 parser is ready\n")
   if (PyType_Ready(&Py_WITSML21_Type)<0)
      Py_WITSML21_ERROR("\n\"Can't initialize module WITSML 2.1 parser\"\n", NULL)

   PANEL_DEBUG(stdout, "Creating module ...\n")
   if (!(m=PyModule_Create(&Py_WITSML21_module)))
      Py_WITSML21_ERROR("\nCannot create module \"Py_WITSML21_module\"\n", NULL)

   PANEL_DEBUG(stdout, "Module %p created\n", m);
   if (PyModule_AddObject(m, "create", (PyObject *) &Py_WITSML21_Type)<0)
      Py_WITSML21_ERROR("\nCannot create module \"WITSML 2.1 to BSON parser\" from \"Py_WITSML21_Type\"\n", NULL)

   return m;
}

/////////////////////////////////////////////////////// C Witsml Parser /////////////////////////////////////////////////////// 

C_WITSML21_PARSER_BUILD

