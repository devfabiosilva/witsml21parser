#ter 20 set 2022 20:51:51 -03
ENDIAN?=LITTLE
STAT?=WITH_STATISTICS
AR=ar rcs
LD=ld -r -b binary
CC=gcc
STRIP=strip
CURDIR=$(PWD)
INCLUDEDIR=$(CURDIR)/include
#FLAG=-DWITH_ZLIB -DWITH_OPENSSL -DWITH_NOSERVEREQUEST -Wno-stringop-truncation
FLAG=-lpthread -Wno-stringop-truncation -D$(STAT) -DCWS_$(ENDIAN)_ENDIAN
DEBUG_FLAG=-g -fsanitize=address,leak -DSOAP_DEBUG $(FLAG)
TARG=cws
TARG_DBG=$(TARG)_debug
LIBANAME=cws
LIBDIR=$(CURDIR)/lib

MONGO_C_GIT=https://github.com/mongodb/mongo-c-driver.git
MONGO_C_BRANCH=1.24.3
MONGO_C_DIR=$(CURDIR)/third-party/mongo-c-driver

FLAG_JNI=-Wno-stringop-truncation -DJNI_RUSAGE_CHILDREN -DCWS_$(ENDIAN)_ENDIAN -D$(STAT)

LIBANAME_JNI=$(LIBANAME)_jni
LIBJNI=libjwitsmlparser21.so
JAVAINCLUDE=/usr/lib/jvm/java-11-openjdk-amd64/include
JAVAINCLUDE_LINUX=/usr/lib/jvm/java-11-openjdk-amd64/include/linux
EXECSTACK=execstack -c

all: main

cws_version.o:
	@echo "Generating version ..."
	@$(CC) -O2 $(CURDIR)/misc/versionBuilder.c $(CURDIR)/src/cws_utils.c $(CURDIR)/src/cws_bson_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/misc/versionBuilder -L$(LIBDIR) -lbson-static-1.0 -Wall $(FLAG) -DVERGEN
	cd $(CURDIR)/misc/;./versionBuilder
	$(LD) -o $(CURDIR)/src/version_bson.o version.bson

cws_utils.o:
	@echo "Build utilities for CWS for parsing ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_utils.o -Wall $(FLAG)

request_context.o:
	@echo "Build request context for parsing ..."
	@$(CC) -O2 -c $(CURDIR)/src/request_context.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/request_context.o -Wall $(FLAG)

wistml2bson_deserializer.o:
	@echo "Build WITSML to BSON deserializer ..."
	@$(CC) -O2 -c $(CURDIR)/src/wistml2bson_deserializer.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/wistml2bson_deserializer.o -Wall $(FLAG)

cws_soap.o:
	@echo "Build CWS INTERNAL/EXTERNAL SOAP constructors/destructors ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_soap.c -I$(INCLUDEDIR) -I../include -o $(CURDIR)/src/cws_soap.o -Wall $(FLAG)

cws_memory.o:
	@echo "Build memory management for CWS ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_memory.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_memory.o -Wall $(FLAG)

cws_bson_utils.o:
	@echo "Build BSON utilities for CWS ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_bson_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_bson_utils.o -Wall $(FLAG)

lib$(LIBANAME).a: cws_version.o cws_utils.o request_context.o cws_memory.o cws_bson_utils.o cws_soap.o wistml2bson_deserializer.o
	@echo "Build lib$(LIBANAME).a ..."
	$(AR) $(LIBDIR)/lib$(LIBANAME).a $(CURDIR)/src/*.o -v

soapC.o:
	@echo "Building soapC.o ... It can take a little time ..."
	@$(CC) -O2 -c -o soapC.o soapC.c -I../include -I../share/gsoap -L../lib  -Wall $(FLAG)
	@echo "Finishing soapC.o"

soapC_shared.o:
	@echo "Building soapC_shared.o ... It can take a little time ..."
	@$(CC) -O2 -fPIC -c -o soapC_shared.o soapC.c -I../include -I../share/gsoap -L../lib  -Wall $(FLAG)
	@echo "Finishing soapC_shared.o"

wistml2bson_deserializer_debug.o:
	@echo "Build WITSML to BSON deserializer (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/wistml2bson_deserializer.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/wistml2bson_deserializer_debug.o -Wall $(DEBUG_FLAG)

cws_utils_debug.o:
	@echo "Build utilities for CWS for parsing (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_utils_debug.o -Wall $(DEBUG_FLAG)

request_context_debug.o:
	@echo "Build request context for parsing (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/request_context.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/request_context_debug.o -Wall $(DEBUG_FLAG)

cws_soap_debug.o:
	@echo "Build CWS INTERNAL/EXTERNAL SOAP constructors/destructors (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_soap.c -I$(INCLUDEDIR) -I../include -o $(CURDIR)/src/cws_soap_debug.o -Wall $(DEBUG_FLAG)

cws_memory_debug.o:
	@echo "Build memory management for CWS (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_memory.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_memory_debug.o -Wall $(DEBUG_FLAG)

cws_bson_utils_debug.o:
	@echo "Build BSON utilities for CWS (DEBUG) ..."
	@$(CC) -O2 -c $(CURDIR)/src/cws_bson_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_bson_utils_debug.o -Wall $(DEBUG_FLAG)

lib$(LIBANAME)_debug.a: cws_version.o cws_utils_debug.o request_context_debug.o cws_memory_debug.o cws_bson_utils_debug.o cws_soap_debug.o wistml2bson_deserializer_debug.o
	@echo "Build lib$(LIBANAME)_debug.a ..."
	$(AR) $(LIBDIR)/lib$(LIBANAME)_debug.a $(CURDIR)/src/*.o -v

soapC_debug_sanitize.o:
	@echo "Building soapC_debug_sanitize.o ... It can take a little time ..."
	@$(CC) -O2 -c -o soapC_debug_sanitize.o soapC.c -I../include -I../share/gsoap -L../lib  -Wall $(DEBUG_FLAG)
	@echo "Finishing soapC_debug_sanitize.o"
###
soapC_shared_debug_sanitize.o:
	@echo "Building soapC_shared_debug_sanitize.o ... It can take a little time ..."
	@$(CC) -O2 -fPIC -c -o soapC_shared_debug_sanitize.o soapC.c -I../include -I../share/gsoap -L../lib  -Wall $(DEBUG_FLAG)
	@echo "Finishing soapC_shared_debug_sanitize.o"

main: soapC.o lib$(LIBANAME).a
	@echo "Compiling ..."
	@$(CC) -O2 -o $(TARG) main.c stdsoap2.c soapC.o soapServer.c -I$(INCLUDEDIR) -I../include -I../share/gsoap -L../lib -L$(LIBDIR) -lcws  -lpthread -lbson-static-1.0 -Wall $(FLAG)
	@echo "Finished"

dbg: soapC_debug_sanitize.o lib$(LIBANAME)_debug.a
	@echo "Compiling in debug mode ..."
	@$(CC) -O2 -o $(TARG_DBG) main.c stdsoap2.c soapC_debug_sanitize.o soapServer.c -I$(INCLUDEDIR) -I../include -I../share/gsoap -L$(LIBDIR) -L../lib -lcws_debug  -lpthread -lbson-static-1.0 -Wall $(DEBUG_FLAG)
	@echo "Finished in debug mode."

##JNI ONLY SOAP INTERNAL
cws_utils_jni.o:
	@echo "Build utilities for CWS for parsing (JNI) ..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/cws_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_utils_jni.o -Wall $(FLAG_JNI)

request_context_jni.o:
	@echo "Build request context for parsing (JNI)..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/request_context.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/request_context_jni.o -Wall $(FLAG_JNI)

wistml2bson_deserializer_jni.o:
	@echo "Build WITSML to BSON deserializer (JNI)  ..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/wistml2bson_deserializer.c -I$(INCLUDEDIR) -I$(CURDIR) -I$(CURDIR)/../include -o $(CURDIR)/src/wistml2bson_deserializer_jni.o -Wall $(FLAG_JNI)

cws_soap_jni.o:
	@echo "Build CWS INTERNAL/EXTERNAL SOAP constructors/destructors (JNI)..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/cws_soap.c -I$(INCLUDEDIR) -I../include -o $(CURDIR)/src/cws_soap_jni.o -Wall $(FLAG_JNI)

cws_memory_jni.o:
	@echo "Build memory management for CWS (JNI)..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/cws_memory.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_memory_jni.o -Wall $(FLAG_JNI)

cws_bson_utils_jni.o:
	@echo "Build BSON utilities for CWS (JNI)..."
	@$(CC) -O2 -fPIC -c $(CURDIR)/src/cws_bson_utils.c -I$(INCLUDEDIR) -o $(CURDIR)/src/cws_bson_utils_jni.o.o -Wall $(FLAG_JNI)

lib$(LIBANAME_JNI).a: cws_version.o cws_utils_jni.o request_context_jni.o wistml2bson_deserializer_jni.o cws_soap_jni.o cws_memory_jni.o cws_bson_utils_jni.o
	@echo "Build lib$(LIBANAME_JNI).a for Java/Kotlin ..."
	$(AR) $(LIBDIR)/lib$(LIBANAME_JNI).a $(CURDIR)/src/*.o -v

.PHONY:
clean:
ifneq ("$(wildcard $(CURDIR)/misc/versionBuilder)","")
	@echo "Removing versionBuilder ..."
	rm -v $(CURDIR)/misc/versionBuilder
else
	@echo "Nothing to do with versionBuilder"
endif
ifneq ("$(wildcard $(CURDIR)/src/*.o)","")
	@echo "Removing objects ..."
	rm -v $(CURDIR)/src/*.o
else
	@echo "Nothing to do with objects"
endif

ifneq ("$(wildcard $(CURDIR)/$(TARG_DBG))","")
	@echo "Removing main $(TARG_DBG)..."
	rm -v $(CURDIR)/$(TARG_DBG)
else
	@echo "Nothing to do $(TARG_DBG)"
endif

ifneq ("$(wildcard $(CURDIR)/$(TARG))","")
	@echo "Removing main $(TARG)..."
	rm -v $(CURDIR)/$(TARG)
else
	@echo "Nothing to do $(TARG)"
endif

ifneq ("$(wildcard $(LIBDIR)/lib$(LIBANAME).a)","")
	@echo "Removing lib$(LIBANAME).a..."
	rm -v $(LIBDIR)/lib$(LIBANAME).a
else
	@echo "Nothing to do $(LIBDIR)/lib$(LIBANAME).a"
endif

ifneq ("$(wildcard $(LIBDIR)/lib$(LIBANAME)_debug.a)","")
	@echo "Removing lib$(LIBANAME)_debug.a..."
	rm -v $(LIBDIR)/lib$(LIBANAME)_debug.a
else
	@echo "Nothing to do $(LIBDIR)/lib$(LIBANAME)_debug.a"
endif

ifneq ("$(wildcard $(LIBDIR)/lib$(LIBANAME_JNI).a)","")
	@echo "Removing lib$(LIBANAME_JNI).a..."
	rm -v $(LIBDIR)/lib$(LIBANAME_JNI).a
else
	@echo "Nothing to do $(LIBDIR)/lib$(LIBANAME_JNI).a"
endif

ifneq ("$(wildcard $(CURDIR)/*.log)","")
	@echo "Removing debug logs ..."
	rm -iv $(CURDIR)/*.log
else
	@echo "Nothing to do with logs"
endif

ifneq ("$(wildcard $(CURDIR)/*.bson)","")
	@echo "Removing BSON files ..."
	rm -v $(CURDIR)/*.bson
else
	@echo "Nothing to do with BSON files"
endif

ifneq ("$(wildcard $(CURDIR)/$(LIBJNI))","")
	@echo "Removing shared object $(LIBJNI) ..."
	rm -v $(CURDIR)/$(LIBJNI)
else
	@echo "Nothing to do with $(LIBJNI)"
endif

ifneq ("$(wildcard $(CURDIR)/examples/*.json)","")
	@echo "Removing json files example ..."
	rm -v $(CURDIR)/examples/*.json
else
	@echo "Nothing to do with examples/*.json"
endif

ifneq ("$(wildcard $(CURDIR)/examples/*.bson)","")
	@echo "Removing BSON files example ..."
	rm -v $(CURDIR)/examples/*.bson
else
	@echo "Nothing to do with examples/*.bson"
endif

.PHONY:
pre_remove:
ifneq ("$(wildcard $(CURDIR)/soapC.o)","")
	@echo "Removing soapC.o ..."
	rm -v $(CURDIR)/soapC.o
else
	@echo "Nothing to do with soapC.o"
endif

ifneq ("$(wildcard $(CURDIR)/soapC_debug_sanitize.o)","")
	@echo "Removing object soapC_debug_sanitize.o ..."
	rm -v $(CURDIR)/soapC_debug_sanitize.o
else
	@echo "Nothing to do with soapC_debug_sanitize.o"
endif

ifneq ("$(wildcard $(CURDIR)/soapC_shared.o)","")
	@echo "Removing object soapC_shared.o ..."
	rm -v $(CURDIR)/soapC_shared.o
else
	@echo "Nothing to do with soapC_shared.o"
endif

ifneq ("$(wildcard $(CURDIR)/soapC_shared_debug_sanitize.o)","")
	@echo "Removing object soapC_shared_debug_sanitize.o ..."
	rm -v $(CURDIR)/soapC_shared_debug_sanitize.o
else
	@echo "Nothing to do with soapC_shared_debug_sanitize.o"
endif

.PHONY:
pre: soapC.o soapC_debug_sanitize.o

.PHONY:
pre_shared: soapC_shared.o soapC_shared_debug_sanitize.o

#JNI ONLY SOAP INTERNAL
.PHONY:
jni: lib$(LIBANAME_JNI).a
	@echo "Compiling $(LIBJNI)..."
	@$(CC) -O2 -shared -fPIC -o $(LIBJNI) src/jni/parser.c stdsoap2.c soapC_shared.o soapServer.c -I$(INCLUDEDIR) -I$(JAVAINCLUDE) -I$(JAVAINCLUDE_LINUX) -I../include -I../share/gsoap -L../lib -L$(LIBDIR) -lcws_jni -lpthread -lbson-shared-1.0 -Wall $(FLAG_JNI)
	@echo "Striping $(LIBJNI) ..."
	@$(STRIP) $(LIBJNI)
	@echo "Disabling execstack ..."
	@$(EXECSTACK) $(LIBJNI)
	@echo "Unchecking executable library ..."
	@chmod -x $(LIBJNI)
	@echo "Finished"

.PHONY:
install_bson:
	@echo "Check if mongo-c-driver directory exists ..."
ifneq ("$(wildcard $(MONGO_C_DIR))","")
	@echo "Already cloned. Skip"
else
	@echo "Cloning branch $(MONGO_C_BRANCH) from $(MONGO_C_GIT)"
	pwd; cd $(CURDIR)/third-party; pwd; git clone -b $(MONGO_C_BRANCH) $(MONGO_C_GIT); cd mongo-c-driver;mkdir compiled && cd compiled; cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_MONGOC=OFF -DCMAKE_INSTALL_PREFIX=$(MONGO_C_DIR)/compiled/out; make -j12;make install; pwd; cp out/lib/libbson-static-1.0.a $(LIBDIR) -v;cp -frv out/include/libbson-1.0/bson $(INCLUDEDIR);cd src/libbson/CMakeFiles/bson_shared.dir; pwd; ar rcs $(LIBDIR)/libbson-shared-1.0.a src/bson/*.o src/jsonsl/*.o __/common/*.o
endif
