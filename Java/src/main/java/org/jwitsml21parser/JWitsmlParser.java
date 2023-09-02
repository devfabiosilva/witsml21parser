package org.jwitsml21parser;

import org.bson.*;
import org.jwitsml21parser.exception.JWitsmlException;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.TreeMap;

import static org.jwitsml21parser.JWitsml21.bsonToString;

@SuppressWarnings("all")
public class JWitsmlParser {
    private final String witsmlParserInstanceName = null;
    private final String witsmlObjectName = null;
    public byte [] object = null;
    private BSONObject bson = null;
    private String json = null;
    private byte[] jsonByte = null;
    private long memoryUsed = -1;
    private int type = -1;
    private int costs = -1;
    private int strings = -1;
    private int shorts = -1;
    private int ints = -1;
    private int long64s = -1;
    private int enums = -1;
    private int arrays = -1;
    private int booleans = -1;
    private int doubles = -1;
    private int date_times = -1;
    private int measures = -1;
    private int event_types = -1 ;
    private int total = -1;
    private Map<String,Object> map = null;
    private Map<String, Object> statistic = null;

    public Map<String, Object> getStatistic() throws JWitsmlException {
        if (statistic != null)
            return statistic;
        this.statistic = new TreeMap<>();
        if (this.memoryUsed > 0)
            this.statistic.put("memoryUsed", this.memoryUsed);
        else if (this.memoryUsed < 0)
            throw new JWitsmlException(
                    "Statistic feature not installed in JWitsml 2.1 parser. Please recompile with \"make jni WITH_STATISTICS=WITH_STATISTICS\"", 11, "", "",
                    this.witsmlParserInstanceName
            );
        if (this.costs > 0)
            this.statistic.put("costs", this.costs);
        if (this.strings > 0)
            this.statistic.put("strings", this.strings);
        if (this.shorts > 0)
            this.statistic.put("shorts", this.shorts);
        if (this.ints > 0)
            this.statistic.put("ints", this.ints);
        if (this.long64s > 0)
            this.statistic.put("long64s", this.long64s);
        if (this.enums > 0)
            this.statistic.put("enums", this.enums);
        if (this.arrays > 0)
            this.statistic.put("arrays", this.arrays);
        if (this.booleans > 0)
            this.statistic.put("booleans", this.booleans);
        if (this.doubles > 0)
            this.statistic.put("doubles", this.doubles);
        if (this.date_times > 0)
            this.statistic.put("dateTime", this.date_times);
        if (this.measures > 0)
            this.statistic.put("measures", this.measures);
        if (this.event_types > 0)
            this.statistic.put("eventTypes", this.event_types);
        if (this.total > 0)
            this.statistic.put("total", this.total);
        if (statistic.size() > 0)
            return this.statistic;
        return (this.statistic = null);
    }
    public JWitsmlParser() {}
    public void saveToFile(String filename) throws Exception {

        if (this.object != null) {
            File outputFile = new File(filename +".bson");
            try (FileOutputStream outputStream = new FileOutputStream(outputFile)) {
                outputStream.write(this.object);
            }
        } else
            throw new Exception("BSON data object not found");
    }

    public void saveToFileJson(String filename) throws Exception, JWitsmlException {

        File outputFile = new File(filename +".json");
        try (FileOutputStream outputStream = new FileOutputStream(outputFile)) {
            outputStream.write((this.jsonByte != null)?(this.jsonByte):(this.jsonByte = bsonToString(this.object)));
        }
    }

    public String getWitsmlParserInstanceName() {
        return (this.witsmlParserInstanceName != null)?this.witsmlParserInstanceName:"Unable to get instance name";
    }

    public String getWitsmlObjectName() {
        return (this.witsmlObjectName != null)?this.witsmlObjectName:"Unable to get instance name";
    }

    public int getType() {
        return this.type;
    }

    public BSONObject getBson() throws JWitsmlException {
        if (this.bson != null)
            return this.bson;

        if (this.object != null) {
            BSONDecoder decoder = new BasicBSONDecoder();
            return (this.bson = decoder.readObject(this.object));
        }
        throw new JWitsmlException("Could not parse binary data to BSON", 10, "", "", this.witsmlParserInstanceName);
    }

    public Map<String, Object> getMap() throws JWitsmlException {
        if (this.map != null)
            return this.map;

        return (this.map = getBson().toMap());
    }

    public String getJson() throws JWitsmlException, Exception {
        if (this.json != null)
            return json;

        if (this.jsonByte == null)
            this.jsonByte = bsonToString(this.object);

        return (this.json =  new String(jsonByte, StandardCharsets.UTF_8));
    }
}
