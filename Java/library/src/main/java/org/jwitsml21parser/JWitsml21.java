package org.jwitsml21parser;

import org.jwitsml21parser.exception.JWitsmlException;

import java.io.FileInputStream;

public class JWitsml21 {
    private final int SOAP_XML_IGNORENS = 0x00004000;
    private final int SOAP_XML_STRICT = 0x00001000;
    private int options = SOAP_XML_IGNORENS | SOAP_XML_STRICT;
    static {
        String os = System.getProperty("os.name");
        if (os.equals("Linux")) {
            try {
                System.loadLibrary("jwitsmlparser21");
            } catch (Throwable e) {
                System.out.println("JNI example load library 'jwitsmlparser21.so' error @ version 2.1.");
                System.out.println(e.getMessage());
                System.exit(1);
            }
        } else {
            System.out.println("OS not supported: " + os + ". Only Linux OS is supported");
            System.exit(1);
        }
    }

    public native JWitsmlParser parser(String xml, int options) throws JWitsmlException, Exception;
    public static native byte[] bsonToString(byte[] bson) throws Exception;
    public static native byte[] cwsVersion() throws Exception;

    public JWitsmlParser parser(String xml) throws JWitsmlException, Exception {
        return parser(xml, this.options);
    }

    public JWitsml21 setDefault() {
        this.options = SOAP_XML_IGNORENS | SOAP_XML_STRICT;
        return this;
    }

    public JWitsml21 setXMLIgnoreNS() {
        this.options |= SOAP_XML_IGNORENS;
        return this;
    }

    public JWitsml21 clearXMLIgnoreNS() {
        this.options &= (~SOAP_XML_IGNORENS);
        return this;
    }

    public JWitsml21 setXMLStrict() {
        this.options |= SOAP_XML_STRICT;
        return this;
    }

    public JWitsml21 clearXMLStrict() {
        this.options &= (~SOAP_XML_STRICT);
        return this;
    }

    public JWitsml21 clearOptions() {
        this.options = 0;
        return this;
    }

    public JWitsmlParser readFile(String fileName, int options) throws Exception, JWitsmlException {
        if (fileName == null)
            throw new Exception("filename is null");

        FileInputStream fstream = null;
        String str;
        try {
            fstream = new FileInputStream(fileName);
            str = new String(fstream.readAllBytes());
        } finally {
            if (fstream != null)
                fstream.close();
        }

        return this.parser(str, options);
    }
    public JWitsmlParser readFile(String fileName) throws Exception, JWitsmlException {
        return this.readFile(fileName, this.options);
    }
}
