#ifndef CWS_BSON_UTILS_H
 #define CWS_BSON_UTILS_H
#include <bson/bson.h>

#define CWS_CONST_BSON_KEY(str) (const char *)str, (int)(sizeof(str)-1)

#define CWS_BSON_NEW bson_new()

void cws_bson_free(bson_t **);
char *cws_data_to_json(size_t *, const uint8_t *, size_t);
int bson_serialize(uint8_t **, size_t *, const char *, int, bson_t *);
int json_to_bson_serialized(
  uint8_t **, size_t *,
  const char *, ssize_t,
  char *, size_t
);
#endif

