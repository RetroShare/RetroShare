#ifndef RSNXSITEMS_TEST_H
#define RSNXSITEMS_TEST_H

#include "serialiser/rsnxsitems.h"


RsSerialType* init_item(RsNxsGrp&);
RsSerialType* init_item(RsNxsMsg&);
RsSerialType* init_item(RsNxsSyncGrp&);
RsSerialType* init_item(RsNxsSyncMsg&);
RsSerialType* init_item(RsNxsSyncGrpItem&);
RsSerialType* init_item(RsNxsSyncMsgItem&);
RsSerialType* init_item(RsNxsTransac& );

bool operator==(const RsNxsGrp&, const RsNxsGrp&);
bool operator==(const RsNxsMsg&, const RsNxsMsg&);
bool operator==(const RsNxsSyncGrp&, const RsNxsSyncGrp&);
bool operator==(const RsNxsSyncMsg&, const RsNxsSyncMsg&);
bool operator==(const RsNxsSyncGrpItem&, const RsNxsSyncGrpItem&);
bool operator==(const RsNxsSyncMsgItem&, const RsNxsSyncMsgItem&);
bool operator==(const RsNxsTransac&, const RsNxsTransac& );


#endif // RSNXSITEMS_TEST_H
