#ifndef RSNXSITEMS_TEST_H
#define RSNXSITEMS_TEST_H

#include "serialiser/rsnxsitems.h"


RsSerialType* init_item(RsNxsGrp&);
RsSerialType* init_item(RsNxsMsg&);
RsSerialType* init_item(RsSyncGrp&);
RsSerialType* init_item(RsSyncGrpMsg&);
RsSerialType* init_item(RsSyncGrpList&);
RsSerialType* init_item(RsSyncGrpMsgList&);
RsSerialType* init_item(RsNxsTransac& );

bool operator==(const RsNxsGrp&, const RsNxsGrp&);
bool operator==(const RsNxsMsg&, const RsNxsMsg&);
bool operator==(const RsSyncGrp&, const RsSyncGrp&);
bool operator==(const RsSyncGrpMsg&, const RsSyncGrpMsg&);
bool operator==(const RsSyncGrpList&, const RsSyncGrpList&);
bool operator==(const RsSyncGrpMsgList&, const RsSyncGrpMsgList&);
bool operator==(const RsNxsTransac&, const RsNxsTransac& );


#endif // RSNXSITEMS_TEST_H
