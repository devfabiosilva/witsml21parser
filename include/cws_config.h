#ifndef CWS_CONFIG_H
 #define CWS_CONFIG_H

#ifdef CWS_LITTLE_ENDIAN
 #define CHECKED_ENDIANESS
#endif

#ifdef CWS_BIG_ENDIAN
 #define CHECKED_ENDIANESS
#endif

#ifndef CHECKED_ENDIANESS
  #error "Choose CWS_LITTLE_ENDIAN or CWS_BIG_ENDIAN"
#endif

#endif
