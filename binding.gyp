{
  "targets": [
    {
      "target_name": "jswitsml21bson",
      "sources": [ "./src/napi/addon.c", "./soapServer.c", "./stdsoap2.c" ],
      "include_dirs":[ "./include", "./" ],
      "library_dirs": ["./lib"],
      "libraries": [
              "-Wl,--start-group ../lib/libcws_js.a ../lib/libbson-shared-1.0.a ../soapC_shared.o -Wl,--end-group"
          ],
      "defines": ["CWS_LITTLE_ENDIAN", "WITH_STATISTICS"]
    }
  ]
}

