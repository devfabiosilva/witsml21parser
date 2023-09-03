#include <bson/bson.h>
#include <cws_utils.h>

#define MIN_INDEXER_SIZE (2*sizeof(uint32_t)+1)

size_t cws_bson32_to_string(uint32_t value, const char **strptr, IdxType index)
{
  char *p;
  const char *q, *vec[] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
    "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
    "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
  };

_Static_assert((sizeof(vec)/sizeof(vec[0]))==256, "cws_bson32_to_string: Wrong vector size");

  union val_u {
    uint8_t u8[sizeof(uint32_t)/sizeof(uint8_t)];
    uint32_t u32;
  } val;

  val.u32=value;
  p=index;

#ifdef CWS_LITTLE_ENDIAN
  *(p++)=*(q=vec[(size_t)val.u8[3]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[2]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[1]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[0]]);
  *(p)=*(++q);
#else
  *(p++)=*(q=vec[(size_t)val.u8[0]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[1]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[2]]);
  *(p++)=*(++q);

  *(p++)=*(q=vec[(size_t)val.u8[3]]);
  *(p)=*(++q);
#endif

  *strptr=index;
  return (MIN_INDEXER_SIZE-1);
}
#undef MIN_INDEXER_SIZE

static
size_t bson_uint32_to_string_helper(uint32_t value, const char **strptr, IdxType index)
{
_Static_assert(sizeof(IdxType)==16, "TEST ERROR: Align it correctly");
  return bson_uint32_to_string(value, strptr, index, sizeof(IdxType));  // Ref.: http://mongoc.org/libbson/current/bson_uint32_to_string.html
}

idx2str_fn init_indexer(int key_index, IdxType index)
{
  if (key_index<0)
    return NULL;

  memset((void *)index, 0, sizeof(IdxType));

  if (key_index<1000)
    return bson_uint32_to_string_helper;

  return cws_bson32_to_string;
}

int readText(const char **text, size_t *text_len, const char *filename)
{
  int err;
  FILE *f;
  long l;

  (*text)=NULL;
  (*text_len)=0;

  if (!(f=fopen(filename, "r")))
    return -1;

  if (fseek(f, 0L, SEEK_END)<0) {
    err=-2;
    goto readText_exit1;
  }

  if ((l=ftell(f))<0) {
    err=-3;
    goto readText_exit1;
  }

  if (!((*text)=(char *)malloc((size_t)(l+1)))) {
    err=-4;
    goto readText_exit1;
  }

  (*text_len)=(size_t)l;

  err=0;
  if ((*text_len)==0) {
    ((char *)(*text))[0]=0;
    goto readText_exit1;
  }

  rewind(f);

  if (fread((void *)(*text), sizeof(const char), (*text_len), f)==(*text_len)) {
    ((char *)(*text))[(*text_len)]=0;
    goto readText_exit1;
  }

  free((void *)(*text));

  err=-5;
  (*text)=NULL;
  (*text_len)=0;

readText_exit1:
  fclose(f);

  return err;
}

inline
void readTextFree(const char **text)
{
  if (text) {
    free((void *)(*text));
    (*text)=NULL;
  }
}

#ifndef VERGEN

void cws_version(struct cws_version_t *version)
{
//https://www.devever.net/~hl/incbin
  extern uint8_t _binary_version_bson_start[] asm("_binary_version_bson_start");
  extern uint8_t _binary_version_bson_end[] asm("_binary_version_bson_end");

  version->version=_binary_version_bson_start;
  version->versionSize=_binary_version_bson_end - _binary_version_bson_start;

}
#endif

