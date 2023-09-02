#ifndef REQUESTS_H
 #define REQUESTS_H

#define MESSAGE_REQUEST_BEGIN \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<SOAP-ENV:Envelope\
    xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\"\
    xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\"\
    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\
    xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\
    xmlns:ns1=\"http://www.energistics.org/energyml/data/witsmlv2\"\
    xmlns:ns2=\"http://www.energistics.org/energyml/data/commonv2\"\
    xmlns:ns21=\"http://cwitsml.org/ns21.xsd\">\
 <SOAP-ENV:Body>\
  <ns21:readWitsmlObject>\
   <WitsmlObject xsi:type=\"ns21:witsmlObject\">"

#define MESSAGE_REQUEST_END \
"   </WitsmlObject>\
  </ns21:readWitsmlObject>\
 </SOAP-ENV:Body>\
</SOAP-ENV:Envelope>"

#define MESSAGE_REQUEST_BEGIN_SIZE sizeof(MESSAGE_REQUEST_BEGIN)
#define MESSAGE_REQUEST_END_SIZE sizeof(MESSAGE_REQUEST_END)

#define MESSAGE_REQUEST_BEGIN_LEN MESSAGE_REQUEST_BEGIN_SIZE-1
#define MESSAGE_REQUEST_END_LEN MESSAGE_REQUEST_END_SIZE-1

#endif
