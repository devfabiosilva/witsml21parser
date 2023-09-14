{
  "targets": [
    {
      "target_name": "jswitsml21bson",
      "sources": [ "./src/napi/addon.c", "./soapServer.c", "./stdsoap2.c" ],
      "include_dirs":[ "./include", "./" ],
      "libraries": [
              "-Wl,-rpath,./lib/libbson-shared-1.0.a", "-Wl,-rpath,./lib/libcws_js.a"
          ],
      "extra_objects": ["./soapC_shared.o"],
      "defines": ["CWS_LITTLE_ENDIAN", "WITH_STATISTICS"]
    }
  ]
}

