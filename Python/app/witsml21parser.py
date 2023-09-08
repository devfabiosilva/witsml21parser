import witsml21bson
import bson
#https://pymongo.readthedocs.io/en/stable/api/bson/json_util.html
#https://docs.python.org/3/c-api/
from bson.json_util import dumps

def main():
#TODO Implementing C Python modules
  f = open("../../examples/xmls/BhaRun.xml")
  parser=witsml21bson.create()
  print(parser.getInstanceName())
  r=parser.parse(f.read())
  print(parser.getObjectName())
  print(r)
  f.close()
  f = open("../../examples/xmls/OpsReport.xml")
  r=parser.parse(f.read())
  print(parser.getObjectName())
  print(r)
  d=bson.decode(r)
  print(bson.decode(r))
  print(type(d))
  s=dumps(d)
  print(s)
  print(type(s))
  print(parser.getObjectType())
  f.close()

if __name__ == "__main__":
  main()

