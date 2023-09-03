#include <cws_bson_utils.h>
#include <cws_utils.h>

int main(int argc, char **argv)
{
  #define JSONFILE "../version.json"
  #define BSONFILE "../version.bson"
  int err;
  char msg[1024];
  const char *text;
  size_t textSize;
  uint8_t *bson;
  size_t bsonSize;
  FILE *f;

  printf("\nOpening "JSONFILE"\n");

  if ((err=readText(&text, &textSize, JSONFILE))) {
    printf("\nError. Could not open "JSONFILE" %d\n", err);
    return err;
  }

  printf("\nOK\nSerializing json to bson ...\n");

  if ((err=json_to_bson_serialized(&bson, &bsonSize, text, (ssize_t)textSize, msg, sizeof(msg)))) {
    printf("\nError %d with reason: %s\n", err, msg);
    goto exit_1;
  }

  printf("\nOK\nCreating "BSONFILE"...\n");

  if ((f=fopen(BSONFILE, "w"))) {
    if (fwrite((void *)bson, sizeof(uint8_t), bsonSize, f)==bsonSize)
      printf("\nOK\n");
    else {
      err=-99;
      printf("\nFail. Could not create "BSONFILE"\n");
    }

    fclose(f);
  } else {
    err=-100;
    printf("\nError. Could not create "BSONFILE"\n");
  }

  bson_free((void *)bson);

exit_1:
  free((void *)text);

  return err;
}

