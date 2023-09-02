#ifndef CWS_UTILS_H
 #define CWS_UTILS_H

typedef char IdxType[16];
typedef size_t (*idx2str_fn)(uint32_t, const char **, IdxType);

size_t cws_bson32_to_string(uint32_t, const char **, IdxType);
idx2str_fn init_indexer(int, IdxType);

#endif

