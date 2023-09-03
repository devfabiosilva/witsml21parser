# witsml21parser

A fast, robust and portable Witsml 2.1 to BSON parser

# Before you install

Before you install you need to check these tools:

```sh
A Linux based OS
gcc
cmake
make
git
execstack
```

# Downloading

```sh
git clone https://github.com/devfabiosilva/witsml21parser.git
```

# Compiling

**_witsml21parser_** needs _libbson_ library to be compiled. For first time compiling you need to download and compile dependencies first.

There are two statics dependencies:

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

- Go to Java sources [folder](https://github.com/devfabiosilva/witsml21parser/tree/master/Java/src)
- Execute shell script:

```sh
source env.sh
```
- Open with your favorite IDE and run Java code [here](https://github.com/devfabiosilva/witsml21parser/tree/master/Java/src)
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

