#ifndef REQUEST_CONTEXT_H
 #define REQUEST_CONTEXT_H
#include <cws_memory.h>
#include "../stdsoap2.h"

#include <cws_bson_utils.h>

enum witsml_version_e {
  VERSION_UNKNOWN=0, // Always 0
  VERSION_1_3_1_1=1, // Deprecated. Not used anymore. Only for informative base message
  VERSION_1_4_1_1, // Not used anymore. Use 2.1 instead
  VERSION_1_4_1, // Not used anymore. Use 2.1 instead
  VERSION_2_1,
  VERSION_2_0, //Not used anymore. Use 2.1 instead
};

_Static_assert(VERSION_UNKNOWN==0, "VERSION_UNKNOWN must be 0");

struct cws_soap_fault_t {
  char *faultstring; // alloc'd human-readable message with the reason of the error. It should be free after use
  size_t faultstring_len; // length string of faultstring
  char *XMLfaultdetail; //alloc'd XML-formatted string with details or NULL. It should be free after use
  size_t XMLfaultdetail_len; // length string of XMLfaultdetail_len
};

struct c_json_str_t {
  char *json;
  size_t json_len;
};

struct c_bson_serialized_t {
  uint8_t *bson;
  size_t bson_size;
};

#ifdef WITH_STATISTICS
struct statistics_t {
  //Statistics in objects
  int32_t costs;
  int32_t strings;
  int32_t shorts;
  int32_t ints;
  int32_t long64s;
  int32_t enums;
  int32_t arrays;
  int32_t booleans;
  int32_t doubles;
  int32_t date_times;
  int32_t measures;
  int32_t event_types;
  int32_t total;
  size_t used_memory;
};
#endif

enum object_type_e {
  TYPE_None=0,
  TYPE_BhaRun,
  TYPE_CementJob,
  TYPE_DepthRegImage,
  TYPE_DownholeComponent,
  TYPE_DrillReport,
  TYPE_FluidsReport,
  TYPE_Log,
  TYPE_MudLogReport,
  TYPE_OpsReport,
  TYPE_Rig,
  TYPE_Risk,
  TYPE_StimJob,
  TYPE_SurveyProgram,
  TYPE_Target,
  TYPE_ToolErrorModel,
  TYPE_Trajectory,
  TYPE_Tubular,
  TYPE_Well,
  TYPE_Wellbore,
  TYPE_WellboreCompletion,
  TYPE_WellboreGeology,
  TYPE_WellCMLedger,
  TYPE_WellCompletion
};

typedef struct cws_config_t {
  struct soap *soap_internal; // Pointer of soap internal. For service use
  char instance_name[128]; // Name of config instance (Optional)
  int internal_soap_error; // Error number for internal soap.
  int internalInitFlag; // soap internal initialization flag
  char *xmlIn; // Pointer of XML object in (from client)
  size_t xmlLen; // Length of XML in (from client) excluding NULL char
  char *xmlSoap; // Pointer of XML object with soap envelope (for internal service process object). Should be free
  size_t xmlSoapLen; //Length of XML with SOAP envelope in (from client) excluding NULL char
  size_t xmlSoapSize; // Size of alloc'd memory of xmlSoap pointer
  const char *internal_os; // Internal SOAP output stream. Used only for debug
  struct ns21__witsmlObject *WitsmlObject; // Pointer of WITSML object
  enum witsml_version_e witsml_version; // Witsml version parsed from client
  bson_t *object; // Object parsed in BSON. Must be free by bson library
  struct c_bson_serialized_t c_bson_serialized; // Bson serialized (can be flushed to file). Must be free by bson library
  struct c_json_str_t c_json_str; // C JSON parsed to string. *json can be NULL. SHOULD be free.
  enum object_type_e object_type; // Object type (Log, MudLog ...)
  const char *object_name; // Object name string of object
  struct cws_soap_fault_t cws_soap_fault; // CWS SOAP FAULT
#ifdef WITH_STATISTICS
  struct statistics_t statistics; // Statistics about objects for monitoring systems support
  long int initial_resource_size; // Initial resource size for statistic
#endif
} CWS_CONFIG;

char *cws_parse_XML_soap_envelope(struct soap *, char *, size_t/*, unsigned int*/);
CWS_CONFIG *cws_config_new(const char *);
void cws_recycle_config(CWS_CONFIG *);
void cws_config_free(CWS_CONFIG **);

struct c_json_str_t *getJson(struct soap *);
#ifdef WITH_STATISTICS
struct statistics_t *getStatistics(struct soap *);
#endif
struct c_bson_serialized_t *bsonSerialize(struct soap *); 
int writeToFile(struct soap *, const char *);
int writeToFileJSON(struct soap *, const char *);

#define GET_INSTANCE_NAME ((CWS_CONFIG *)(soap_internal->user))->instance_name
#define GET_OBJECT_TYPE ((CWS_CONFIG *)(soap_internal->user))->object_type
#define GET_OBJECT_NAME ((CWS_CONFIG *)(soap_internal->user))->object_name
#define GET_INTERNAL_SOAP_ERROR ((CWS_CONFIG *)(soap_internal->user))->internal_soap_error
#define DECLARE_CONFIG(soap) CWS_CONFIG *config=((CWS_CONFIG *)(soap->user));

#define IS_WITSML_2_1(soap_ptr) (((CWS_CONFIG *)(soap_ptr->user))->witsml_version==VERSION_2_1)

#endif

