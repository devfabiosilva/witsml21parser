from distutils.core import setup, Extension

def main():

    setup(name="witsml2bson",
        version="0.1.0",
        description="WITSML 2.1 BSON parser module for Python 3 using C library setup",
        author="Fábio Pereira da Silva",
        author_email="fabioegel@gmail.com",
        url="https://github.com/devfabiosilva/witsml21parser",
        maintainer_email="fabioegel@gmail.com",
        ext_modules=[Extension("witsml2bson", ["src/python/module.c"],
            library_dirs=['lib'],
            libraries=['bson-shared-1.0', "cws_python"],
            include_dirs=['include']
        )])

if __name__ == "__main__":
    main()

