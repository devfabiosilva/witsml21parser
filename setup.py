from distutils.core import setup, Extension

def main():

    setup(name="witsml21bson",
        version="0.1.0",
        description="WITSML 2.1 BSON parser module for Python 3 using C library setup",
        author="Fábio Pereira da Silva",
        author_email="fabioegel@gmail.com",
        url="https://github.com/devfabiosilva/witsml21parser",
        maintainer_email="fabioegel@gmail.com",
        ext_modules=[Extension("witsml21bson", ["src/python/module.c", "stdsoap2.c" ,"soapServer.c"],
            library_dirs=['lib'],
            libraries=['cws_py', 'bson-shared-1.0'],
            extra_objects=["soapC_shared.o"],
            include_dirs=['include'],
            define_macros=[('CWS_LITTLE_ENDIAN', None)]
        )])

if __name__ == "__main__":
    main()

