const witml21bson = require('./jswitsml21bson')
const { BSON } = require('bson')

parser = witml21bson.create()

O_LIST = [
 'BhaRun', 'CementJob', 'DepthRegImage', 'DownholeComponent', 'DrillReport', 'FluidsReport', 'Log', 'MudLogReport', 'OpsReport', 'Rig',
 'Risk', 'StimJob', 'SurveyProgram', 'Target', 'ToolErrorModel', 'Trajectory', 'Tubular', 'Well', 'Wellbore', 'WellboreCompletion',
 'WellboreGeology', 'WellCMLedger', 'WellCompletion', "INVALID_Log", "INVALID_OpsReport"
]

console.log("\n" + parser.getInstanceName() + "\n")

const DEFAULT_PATH = "../examples/"
const DEFAULT_PATH_XML = DEFAULT_PATH + "xmls/"

function save_to_file(caller, file_path) {
  var ret = false
  try {
    caller(file_path)
  } catch (e) {
    ret = true
    console.log("Error " + e.toString())
    console.log("Maybe file " + file_path + " exists")
  }

  return ret
}

function print_parser_error(parser) {
  console.log("Error number: " + parser.getError().toString())
  var faultString = parser.getFaultString()
  var XMLfaultdetail = parser.getXMLfaultdetail()
  if (faultString != null)
    console.log(faultString)
  if (XMLfaultdetail != null)
    console.log(XMLfaultdetail)
}

function readWitsml21Objects(parser) {
  for (o of O_LIST) {
    var fp = DEFAULT_PATH_XML + o + ".xml"
    var bsonByte
    console.log("\nOpening " + fp)
    console.log(o)
    try {
      bsonByte = parser.parseFromFile(fp)
    } catch (e) {
      console.log(e.toString())
      print_parser_error(parser)
      continue
    }

    try {
      console.log("WITSML 2.1 XML statistics:")
      console.log(parser.getStatistics())
      var jsonFile = DEFAULT_PATH + o + ".json"
      console.log("Saving JSON to file " + jsonFile)

      if (save_to_file(parser.saveToFileJSON, jsonFile))
        continue

      bsonFile = DEFAULT_PATH + o + ".bson"
      console.log("Saving BSON to file " + bsonFile)

      if (save_to_file(parser.saveToFile, bsonFile))
        continue

      console.log(bsonByte)
      console.log("BSON to NodeJS object")
      console.log(BSON.deserialize(bsonByte))
      console.log("JSON string")
      console.log(parser.getJson())
    } catch (e) {
      console.log("Error " + e.toString())
    }
  }
}

readWitsml21Objects(parser)

