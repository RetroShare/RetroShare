#ifndef RSNXSITEMS_TEST_H
#define RSNXSITEMS_TEST_H

#include "serialiser/rsnxsitems.h"


RsSerialType* init_item(RsGrpResp&);
RsSerialType* init_item(RsGrpMsgResp&);
RsSerialType* init_item(RsSyncGrp&);
RsSerialType* init_item(RsSyncGrpMsg&);
RsSerialType* init_item(RsSyncGrpList&);
RsSerialType* init_item(RsSyncGrpMsgList&);

bool operator==(const RsGrpResp&, const RsGrpResp&);
bool operator==(const RsGrpMsgResp&, const RsGrpMsgResp&);
bool operator==(const RsSyncGrp&, const RsSyncGrp&);
bool operator==(const RsSyncGrpMsg&, const RsSyncGrpMsg&);
bool operator==(const RsSyncGrpList&, const RsSyncGrpList&);
bool operator==(const RsSyncGrpMsgList&, const RsSyncGrpMsgList&);


#endif // RSNXSITEMS_TEST_H
