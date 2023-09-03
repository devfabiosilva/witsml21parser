#ifndef CWS_UTILS_H
 #define CWS_UTILS_H

typedef char IdxType[16];
typedef size_t (*idx2str_fn)(uint32_t, const char **, IdxType);

size_t cws_bson32_to_string(uint32_t, const char **, IdxType);
idx2str_fn init_indexer(int, IdxType);

int readText(const char **, size_t *, const char *);
void readTextFree(const char **);

struct cws_version_t {
  uint8_t *version;
  size_t versionSize;
};

#ifndef VERGEN
void cws_version(struct cws_version_t *);
#endif

#endif

