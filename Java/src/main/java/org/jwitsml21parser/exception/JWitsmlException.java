package org.jwitsml21parser.exception;

public class JWitsmlException extends Throwable {
    private final int cwitsmlError;
    private final String faultstring;
    private final String XMLfaultdetail;
    private final String witsmlParserInstanceName;

    public JWitsmlException(
            String message, int cwitsmlError, String faultstring,
            String XMLfaultdetail, String witsmlParserInstanceName
    ) {
        super(message);
        this.cwitsmlError = cwitsmlError;
        this.faultstring = faultstring;
        this.XMLfaultdetail = XMLfaultdetail;
        this.witsmlParserInstanceName = witsmlParserInstanceName;
    }

    public int getCwitsmlError() { return this.cwitsmlError; }
    public String getFaultstring() { return this.faultstring; }
    public String getXMLfaultdetail() { return this.XMLfaultdetail; }
    public String getWitsmlParserInstanceName() { return this.witsmlParserInstanceName; }
}
