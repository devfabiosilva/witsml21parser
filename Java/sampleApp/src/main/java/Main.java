
import org.jwitsml21parser.JWitsml21;
import org.jwitsml21parser.JWitsmlParser;
import org.jwitsml21parser.exception.JWitsmlException;

public class Main {
    public static void main(String[] args) {
        System.out.println("Welcome to WITSML 2.1 parser");
        if (args.length > 0) {
            boolean fail = false;
            JWitsml21 jWitsml21 = new JWitsml21();
            try {
                for (String arg : args) {
                    System.out.println("\n========================\nOpening " + arg);
                    JWitsmlParser parser = jWitsml21.readFile(arg);
                    System.out.println("Instance name: " + parser.getWitsmlParserInstanceName());
                    String filePrefix = parser.getWitsmlObjectName();
                    System.out.println("Saving to file " + filePrefix + ".json");
                    parser.saveToFileJson(filePrefix);
                    System.out.println("Saving to file " + filePrefix + ".bson");
                    parser.saveToFile(filePrefix);
                    System.out.println("\nStatistics for \"" + arg + "\":\n\t" + parser.getStatistic());
                }
            } catch (JWitsmlException e) {
                fail = true;
                System.out.println("\nERROR: " + e.getCwitsmlError());
                System.out.println("\tInstance name: " + e.getWitsmlParserInstanceName());
                System.out.println("\tException: " + e.getMessage());
                System.out.println("\tDetail: " + e.getFaultstring());
                System.out.println("\tXML Detail: " + e.getXMLfaultdetail());
            } catch (Exception e) {
                fail = true;
                System.out.println("\nException: " + e.getMessage());
            }

            if (fail)
                System.exit(1);
        } else
            System.out.println("Select valid XML files");
    }
}
