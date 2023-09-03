package org.jwitsml21parser;

import org.junit.Test;
import org.jwitsml21parser.exception.JWitsmlException;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.*;
import static org.jwitsml21parser.JWitsml21.bsonToString;

public class JWitsml21Test {

    JWitsml21 publicParser = new JWitsml21();
    private final String relativePath = "../../examples/";
    private final String relativeInputPath = relativePath + "xmls/";

    public String readFile(String fileName) throws IOException {
        FileInputStream fstream = new FileInputStream(fileName);
        String str = new String(fstream.readAllBytes());
        fstream.close();
        return str;
    }
    @Test
    public void test() throws Exception {
        String f = readFile(relativeInputPath + "OpsReport.xml");
        JWitsml21 jWitsml21 = new JWitsml21();

        try {

            JWitsmlParser parser = jWitsml21.parser(f, -1);

            parser.saveToFile(relativePath + "bsonFile" + parser.getWitsmlObjectName());
            parser.saveToFileJson(relativePath + "jsonFile" + parser.getWitsmlObjectName());

            assertNotNull(parser.getBson());
            assertNotNull(parser.getMap());
            assertNotNull(parser.getJson());

            System.out.println(parser.getStatistic());
        } catch (JWitsmlException e) {
            assertNotNull(e.getMessage());
            assertNull(e.getFaultstring());
            assertNull(e.getXMLfaultdetail());
        }
    }

    @Test
    public void test2() throws Exception {
        try {
            JWitsmlParser parser = new JWitsml21().readFile ( relativeInputPath + "OpsReport.xml", -1);

            parser.saveToFile(relativePath + "bsonFileTest2" + parser.getWitsmlObjectName());
            parser.saveToFileJson(relativePath + "jsonFileTest2" + parser.getWitsmlObjectName());

            assertNotNull(parser.getBson());
            assertNotNull(parser.getMap());
            assertNotNull(parser.getJson());

            System.out.println(parser.getStatistic());
        } catch (JWitsmlException e) {
            assertNotNull(e.getMessage());
            assertNull(e.getFaultstring());
            assertNull(e.getXMLfaultdetail());
        }
    }

    @Test
    public void testOptions() throws Exception, JWitsmlException {

        JWitsmlParser parser = publicParser.readFile(relativeInputPath + "StimJob.xml");

        assertNotNull(parser.getBson());
        assertNotNull(parser.getMap());
        assertNotNull(parser.getJson());

        System.out.println(parser.getStatistic());

        try {
            publicParser.readFile(relativeInputPath + "StimJob.xml", 500);
            fail("Could not execute this line");
        } catch (Exception e) {
            assertEquals("Invalid options", e.getMessage());
        }

        try {
            publicParser.clearOptions().readFile(relativeInputPath +"StimJob.xml");
            fail("Could never execute this code");
        } catch (JWitsmlException e) {
            assertNotNull(e.getWitsmlParserInstanceName());
            assertEquals("Validation constraint violation: tag name or namespace mismatch in element 'Aliases'", e.getFaultstring());
        }

        parser = publicParser.setXMLIgnoreNS().readFile(relativeInputPath + "StimJob.xml");
        parser.saveToFileJson(relativePath + parser.getWitsmlObjectName());
        parser.saveToFile(relativePath + parser.getWitsmlObjectName());

        assertNotNull(parser.getWitsmlObjectName());

        try {
            publicParser.clearOptions().setXMLStrict().readFile(relativeInputPath + "WellboreGeology.xml");
        } catch (JWitsmlException e) {
            assertEquals(13, e.getCwitsmlError());
            assertEquals("<CWitsmlStoreError type=\"SOAP_INTERNAL\" subCode=\"SOAP-ENV:Client\" errorCode=\"13\">Validation constraint violation: tag name or namespace mismatch in element 'Aliases'</CWitsmlStoreError>", e.getXMLfaultdetail());
        }

        parser = publicParser.clearXMLStrict().setXMLIgnoreNS().readFile(relativeInputPath + "Trajectory.xml");

        parser.saveToFileJson(relativePath + parser.getWitsmlObjectName());
        parser.saveToFile(relativePath + parser.getWitsmlObjectName());

        parser = publicParser.readFile(relativeInputPath + "Risk.xml");

        parser.saveToFileJson(relativePath + parser.getWitsmlObjectName());
        parser.saveToFile(relativePath + parser.getWitsmlObjectName());

        parser = publicParser.setDefault().readFile(relativeInputPath + "ToolErrorModel.xml");

        parser.saveToFileJson(relativePath + parser.getWitsmlObjectName());
        parser.saveToFile(relativePath + parser.getWitsmlObjectName());

        String f = readFile(relativeInputPath + "Rig.xml");

        parser = publicParser.parser(f);

        parser.saveToFileJson(relativePath + parser.getWitsmlObjectName());
        parser.saveToFile(relativePath + parser.getWitsmlObjectName());

    }

    @Test
    public void TestIntegrity() {
        byte[] helloBson = {
                0x16, 0x00, 0x00, 0x00,        // total document size
                0x02,                          // 0x02 = type String
                'h', 'e', 'l', 'l', 'o', 0x00, // field name
                0x06, 0x00, 0x00, 0x00,
                'w', 'o', 'r', 'l', 'd', 0x00, // field value
                0x00                           // 0x00 = type EOO ('end of object')
        };
        byte[] res = null;
        try {
            res = bsonToString(helloBson);
        } catch (Exception e) {
            fail("Could not execute this line: " + e.getMessage());
        }
        assertEquals(
                "{ \"hello\" : \"world\" }",
                new String(res, StandardCharsets.UTF_8)
        );
        byte[] invalidBsonData = {1, 2, 3, 4};
        try {
            bsonToString(invalidBsonData);
            fail("Could not execute this line");
        } catch (Exception e) {
            assertEquals("Invalid BSON data", e.getMessage());
        }

        try {
            bsonToString(null);
            fail("Could not execute this line");
        } catch (Exception e) {
            assertEquals("Binary BSON cannot be null", e.getMessage());
        }
        byte[] invalidEmptyBsonData = {};
        try {
            bsonToString(invalidEmptyBsonData);
            fail("Could not execute this line");
        } catch (Exception e) {
            assertEquals("Empty Binary BSON", e.getMessage());
        }
    }

    @Test
    public void testVersion() throws JWitsmlException, Exception {
        JWitsmlParser jWitsmlParser = new JWitsmlParser();
        assertNotNull(jWitsmlParser.getBsonVersion());
        assertEquals("0.1.0", jWitsmlParser.getBsonVersion().get("version"));
    }
}