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

