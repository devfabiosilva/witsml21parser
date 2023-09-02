#ifndef WITSML2BSON_DESERIALIZER_H
 #define WITSML2BSON_DESERIALIZER_H

typedef int (*readWitsmlObjectFn)(struct soap *soap_internal);
readWitsmlObjectFn readWitsmlObjectBsonParser(struct soap *, struct ns21__witsmlObject *);

int bson_read_BhaRun2_1(struct soap *);
int bson_read_CementJob2_1(struct soap *);
int bson_read_DepthRegImage2_1(struct soap *);
int bson_read_DownholeComponent2_1(struct soap *);
int bson_read_DrillReport2_1(struct soap *);
int bson_read_FluidsReport2_1(struct soap *);
int bson_read_Log2_1(struct soap *);
int bson_read_MudLogReport2_1(struct soap *);
int bson_read_OpsReport2_1(struct soap *);
int bson_read_Rig2_1(struct soap *);
int bson_read_Risk2_1(struct soap *);
int bson_read_StimJob2_1(struct soap *);
int bson_read_SurveyProgram2_1(struct soap *);
int bson_read_Target2_1(struct soap *);
int bson_read_ToolErrorModel2_1(struct soap *);
int bson_read_Trajectory2_1(struct soap *);
int bson_read_Tubular2_1(struct soap *);
int bson_read_Well2_1(struct soap *);
int bson_read_Wellbore2_1(struct soap *);
int bson_read_WellboreCompletion2_1(struct soap *);
int bson_read_WellboreGeology2_1(struct soap *);
int bson_read_WellCMLedger2_1(struct soap *);
int bson_read_WellCompletion2_1(struct soap *);

#endif

