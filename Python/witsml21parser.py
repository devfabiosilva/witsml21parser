import witsml21bson
import bson

def main():
  parser=witsml21bson.create()
  read_witsml21_objects(parser)

O_LIST = [
 'BhaRun', 'CementJob', 'DepthRegImage', 'DownholeComponent', 'DrillReport', 'FluidsReport', 'Log', 'MudLogReport', 'OpsReport', 'Rig',
 'Risk', 'StimJob', 'SurveyProgram', 'Target', 'ToolErrorModel', 'Trajectory', 'Tubular', 'Well', 'Wellbore', 'WellboreCompletion',
 'WellboreGeology', 'WellCMLedger', 'WellCompletion', "INVALID_Log", "INVALID_OpsReport"
]

DEFAULT_PATH = "../examples/"
DEFAULT_PATH_XML = "../examples/xmls/"

def save_to_file(caller, file_path) -> bool:
  try:
    caller(file_path)
    ret = False
  except Exception as e:
    ret = True
    print("File saving error: " + str(e))
  finally:
    print("Maybe file " + file_path + " exists")

  return ret

def print_parser_error(parser):
  print("Error number: " + str(parser.getError()))
  faultStr = parser.getFaultString()
  if (faultStr != None):
    print("fault string: " + faultStr)
  xmlFaultDetail = parser.getXMLfaultdetail()
  if (xmlFaultDetail != None):
    print("Witsml 2.1 parser detail: " + xmlFaultDetail)

def read_witsml21_objects(parser):
  for l in O_LIST:
    fp = DEFAULT_PATH_XML + l + ".xml"
    print("Opening " + fp)
    try:
      bsonByte = parser.parseFromFile(fp)
    except Exception as e:
      print("Exception " + str(e))
      print_parser_error(parser)
      continue

    try:
      print("WITSML 2.1 XML statistics:")
      print(parser.getStatistics())
      jsonFile = DEFAULT_PATH + l + ".json"
      print("Saving JSON to file " + jsonFile)
      if (save_to_file(parser.saveToFileJSON, jsonFile)):
        continue
      bsonFile = DEFAULT_PATH + l + ".bson"
      print("Saving BSON to file " + bsonFile)
      if (save_to_file(parser.saveToFile, bsonFile)):
        continue
      print("BSON to Python dictionary")
      dictionary = bson.decode(bsonByte)
      print(dictionary)
      print("JSON string")
      print(parser.getJson())
    except Exception as e:
      print("Error:")
      print(str(e))

if __name__ == "__main__":
  main()

