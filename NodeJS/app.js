// Begin add sample application Node WITSML 2.1 parser
//import { BSON, EJSON, ObjectId } from 'bson';
const { BSON, EJSON } = require('bson');
a = require('./jswitsml21bson')

b = a.create()

bytes = b.parseFromFile('../examples/xmls/OpsReport.xml')

const doc = BSON.deserialize(bytes);
//console.log(doc["website"])
//console.log(doc["dependencies"]["gSoap"])
console.log(doc)
console.log(EJSON.stringify(doc, { relaxed: false }));

b=null

