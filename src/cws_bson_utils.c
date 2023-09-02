#include <cws_bson_utils.h>

inline
void cws_bson_free(bson_t **bson)
{
  bson_destroy(*bson); // Does nothing if *bson is NULL. See http://mongoc.org/libbson/current/bson_destroy.html
  *bson=NULL;
}

char *cws_data_to_json(size_t *str_len, const uint8_t *data, size_t data_size)
{
  char *str;
  bson_t *bson=bson_new_from_data(data, data_size);

  if (!bson)
    return NULL;

  str=bson_as_relaxed_extended_json((const bson_t *)bson, str_len);

  bson_destroy(bson);

  return str;
}

//bson_t *
//bson_new_from_json (const uint8_t *data, ssize_t len, bson_error_t *error);

int json_to_bson(bson_t **bson, const char *json, ssize_t json_len, char *errMsg, size_t errMsgSize)
{
  int err;
  bson_error_t bson_error;

#define JSON_NULL_OR_EMPTY_ERR (int)-2
#define BSON_NEW_ERR (int)-1

  if ((json!=NULL)&&(json_len!=0))
    err=((*bson=bson_new_from_json((const uint8_t *)json, json_len, &bson_error)))?0:BSON_NEW_ERR;
  else {
    err=JSON_NULL_OR_EMPTY_ERR;
    *bson=NULL;
  }

  if (errMsg) {
    if (!err)
      errMsg[0]=0;
    else if (err==BSON_NEW_ERR)
      snprintf(errMsg, errMsgSize, "Json to Bson ERROR: %d.%d with message \"%s\"", bson_error.domain, bson_error.code, bson_error.message);
    else
      snprintf(errMsg, errMsgSize, "Json parse error NULL or empty. Parsed json(%p) with length = %ld", json, json_len);
  }

#undef BSON_NEW_ERR
#undef JSON_NULL_OR_EMPTY_ERR
  return err;
}

int bson_serialize(uint8_t **buf, size_t *bufsize, const char *value, int value_len, bson_t *bson)
{
  int err;
  bson_writer_t *writer;
  bson_t *doc;

  (*buf)=NULL;
  (*bufsize)=0;
//http://mongoc.org/libbson/current/bson_writer_t.html
  if (!(writer=bson_writer_new(buf, bufsize, 0, bson_realloc_ctx, NULL))) {
    (*buf)=NULL;
    (*bufsize)=0;
    return -1;
  }

  if (!bson_writer_begin(writer, &doc)) {
    err=-2;
    goto bson_serialize_resume;
  }

  err=(bson_append_document(doc, value, value_len, (const bson_t *)bson))?0:-3;

  bson_writer_end(writer);

  if (err==0)
    (*bufsize)=bson_writer_get_length(writer);
  else {
bson_serialize_resume:
    bson_free((void *)*buf);
    (*buf)=NULL;
    (*bufsize)=0;
  }

  bson_writer_destroy(writer);
  return err;
}

int bson_serialize_copy(uint8_t **buf, size_t *bufsize, bson_t *bson)
{
  int err;
  bson_writer_t *writer;
  bson_t *doc;

  (*buf)=NULL;
  (*bufsize)=0;

  if (!(writer=bson_writer_new(buf, bufsize, 0, bson_realloc_ctx, NULL))) {
    (*buf)=NULL;
    (*bufsize)=0;
    return -21;
  }

  if (!bson_writer_begin(writer, &doc)) {
    err=-22;
    goto bson_serialize_copy_resume;
  }

  err=(bson_concat(doc, (const bson_t *)bson))?0:-23;

  bson_writer_end(writer);

  if (err==0)
    (*bufsize)=bson_writer_get_length(writer);
  else {
bson_serialize_copy_resume:
    bson_free((void *)*buf);
    (*buf)=NULL;
    (*bufsize)=0;
  }

  bson_writer_destroy(writer);
  return err;
}

/*
bson_destroy (bson_t *bson);
bool
bson_init_from_json (bson_t *bson,
                     const char *data,
                     ssize_t len,
                     bson_error_t *error);*/
//int json_to_bson(bson_t **bson, const char *json, ssize_t json_len, char *errMsg, size_t errMsgSize)
//int bson_serialize_copy(uint8_t **buf, size_t *bufsize, bson_t *bson)
int json_to_bson_serialized(
  uint8_t **bson, size_t *bson_size,
  const char *json, ssize_t json_len,
  char *errMsg, size_t errMsgSize
)
{
  int err;
  bson_t *bson_tmp;

  *bson=NULL;
  *bson_size=0;

  if ((err=json_to_bson(&bson_tmp, json, json_len, errMsg, errMsgSize)))
    return err;

  err=bson_serialize_copy(bson, bson_size, bson_tmp);

  if (errMsg) {
    if (!err)
      errMsg[0]=0;
    else
      snprintf(errMsg, errMsgSize, "Json to bson serialize error %d. Could not copy %p", err, bson_tmp);
  }

  bson_destroy(bson_tmp);

  return err;
}

