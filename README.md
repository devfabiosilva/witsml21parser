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


