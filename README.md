# witsml21parser

A fast, robust and portable [Witsml 2.1](https://energistics.org/witsml-data-standards) to [BSON parser](https://bsonspec.org/)

## Features

- Fast
- Robust
- Portable ([NodeJS](https://github.com/devfabiosilva/witsml21parser/tree/master/NodeJS/) | [Java/Kotlin](https://github.com/devfabiosilva/witsml21parser/tree/master/Java) | PHP (in development) | [Python 3](https://github.com/devfabiosilva/witsml21parser/tree/master/Python))
- Low dependency libraries.
- No Garbage Collector on parsing
- Low memory allocation/reallocation
- Optimized for C/C++ applications

# Before you install

Before you install you need to check these tools:

```sh
A Linux based OS
Python >= 3.8 (For Python 3 application)
Python 3 development API (for Python 3 application)
gcc >= 9.4.0
cmake >= 3.16.3
make >= 4.2.1
git >= 2.25.1
execstack >= 1.0 (for Java application)
Java SDK development API (for Java application)
Node JS >= v16.20.2 API (for Node JS)
```

# Downloading

```sh
git clone https://github.com/devfabiosilva/witsml21parser.git
```

# Compiling

**_witsml21parser_** needs _libbson_ library to be compiled. For first time compiling you need to download and compile dependencies first.

There are two static dependencies:

- libbson
- WITSML 2.1 - ETP validation schemas C code

## - Compiling libbson third-party library

```sh
make install_bson
```

For multi-threading fast compiling use _-jN_ option e.g:

```sh
make -j12 install_bson
```
## - WITSML 2.1 - ETP validation schemas C code

```sh
make pre pre_shared
```

For multi-threading fast compiling use _-jN_ option e.g:

```sh
make -j12 pre pre_shared
```

## - Compiling WITSML 2.1 to BSON parser

After compiling _libbson_ third-party library and _WITSML 2.1 - ETP validation schemas C code_ just type:

```sh
make
```

For multi-threading fast compiling use _-jN_ option e.g:

```sh
make -j12
```

### Executing

Just type:

```sh
./cws <FILE_NAME>
```

E.g:

```sh
./cws BhaRun.xml
```

It will convert BhaRun.xml to BhaRun.bson file

## - Compiling WITSML 2.1 to BSON parser for Java/Kotlin

To compile native library for Java/Kotlin just type:

```sh
make jni
```

For multi-threading fast compiling use _-jN_ option e.g:

```sh
make -j12 jni
```

### Executing Java WITSML 2.1 to BSON parser

Before you run your code you MUST set native library environment.

- Go to Java sources [folder](https://github.com/devfabiosilva/witsml21parser/tree/master/Java/library)
- Execute shell script:

```sh
source env.sh
```
- Open with your favorite IDE and run Java code [here](https://github.com/devfabiosilva/witsml21parser/tree/master/Java/library)
- Run Tests and Aplication

# SAMPLE APP jwitsml21cmd-1.0.jar

This app shows how to use WITSML 2.1 BSON parser in Java/Kotlin

## Before use

You need to compile JNI

```sh
cd <WITSML 2.1 BSON PARSER PROJECT FOLDER>/
make jni
```

File _libjwitsmlparser21.so_ must be in root project folder.

Compile Java library with _maven_

```
cd <WITSML 2.1 BSON PARSER PROJECT FOLDER>/Java/library
mvn -U clean install
```

## Running

```sh
cd <WITSML 2.1 BSON PARSER PROJECT FOLDER>/Java/sampleApp
source env.sh
java -jar jwitsml21cmd-1.0.jar <WITSML 2.1 XML FILES>
```

### Examples

```sh
java -jar jwitsml21cmd-1.0.jar ../../examples/xmls/OpsReport.xml

```

```sh
Welcome to WITSML 2.1 parser

========================
Opening ../../examples/xmls/OpsReport.xml
Instance name: jWITSMLParser 2.1 - (0x7f599422ab40)
Saving to file OpsReport.json
Saving to file OpsReport.bson

Statistics for "../../examples/xmls/OpsReport.xml":
        {arrays=60, booleans=22, costs=9, dateTime=73, doubles=28, enums=55, long64s=47, measures=382, memoryUsed=3160, strings=463, total=1139}
```

```sh
java -jar jwitsml21cmd-1.0.jar ../../examples/xmls/OpsReport.xml ../../examples/xmls/Risk.xml
```

```sh
Welcome to WITSML 2.1 parser

========================
Opening ../../examples/xmls/OpsReport.xml
Instance name: jWITSMLParser 2.1 - (0x7fe87424aba0)
Saving to file OpsReport.json
Saving to file OpsReport.bson

Statistics for "../../examples/xmls/OpsReport.xml":
        {arrays=60, booleans=22, costs=9, dateTime=73, doubles=28, enums=55, long64s=47, measures=382, memoryUsed=4532, strings=463, total=1139}

========================
Opening ../../examples/xmls/Risk.xml
Instance name: jWITSMLParser 2.1 - (0x7fe8742a8b30)
Saving to file Risk.json
Saving to file Risk.bson

Statistics for "../../examples/xmls/Risk.xml":
        {arrays=12, dateTime=17, enums=14, long64s=9, measures=18, strings=106, total=176}
```

# Executing NodeJS (>=v16.20.2) WITSML 2.1 to BSON parser

## Before use (first time only)

You need to compile JS WITSML 2.1 to BSON parser

In your root _witsml21parser_ folder

```sh
make nodejs && cd $(pwd)/NodeJS && source env.sh
```

## Running

In your [NodeJS](https://github.com/devfabiosilva/witsml21parser/tree/master/NodeJS/) folder:

```sh
node app.js
```

# Executing Python 3 WITSML 2.1 to BSON parser

## Before use (first time only)

You need to compile PyWITSML 2.1 to BSON parser

In your root _witsml21parser_ folder

```sh
make py && cd $(pwd)/Python && source env.sh
```

## Running

In your [Python](https://github.com/devfabiosilva/witsml21parser/tree/master/Python/) folder:

```sh
python3 witsml21parser.py
```

# BENCHMARKS

Primary benchmarks in Java had been shown that WITSML 1.4.1.1 parsing objects are 56 % faster than JAXB in Java application.

JAXB only parses XML objects and store their values in respective Java objects. In other hand, WITSML BSON parser not only parses XML objects in C structs but it creates BSON objects, adds objects to BSON, serializes it and parses it to Java native byte object.

But wait! How is WITSML BSON parser faster than JAXB doing more stuffs?

Answer is simple.

- Less allocation/reallocation in memory
- Referencing objects instead create and copy
- gSoap optimization on parsing XML to C structs. See some interesting [GENIVIA](https://www.genivia.com/) articles [here](https://www.genivia.com/ugrep.html)
- Recycled alloc'd memory
- Low memory usage
- Only two library dependency ([gSoap](https://www.genivia.com/products.html) and [libbson](https://github.com/mongodb/mongo-c-driver/tree/master/src/libbson))
- No garbage collector on parsing objects

# SIGNING KEY

_Keys for signature_:

**[fabioegel@gmail.com](mailto:fabioegel@gmail.com)**
**fingerprint**: 6E21 A6B6 E1A4 6580 CA00 FA54 1077 26E9 14B9 16A2

# LICENSES

This project is fully open source and MIT license.

## WARNING

WITSML 2.1 BSON parser needs library with different licenses.

See [version.json](https://github.com/devfabiosilva/witsml21parser/blob/master/version.json) for details.

# DONATIONS

Any donation is welcome.

Consider any amount of donation in _BITCOIN_: *1JUzcSh3vsBCRji5n5rJsbHQfW3hYrNAW4*

Thank you :)

