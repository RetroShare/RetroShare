#ifndef DATA_SUPPORT_H
#define DATA_SUPPORT_H

#include "serialiser/rsnxsitems.h"
#include "gxs/rsgxsdata.h"

bool operator==(const RsNxsGrp&, const RsNxsGrp&);
bool operator==(const RsNxsMsg&, const RsNxsMsg&);

void init_item(RsNxsGrp& nxg);
void init_item(RsNxsMsg& nxm);
void init_item(RsGxsGrpMetaData* metaGrp);
void init_item(RsGxsMsgMetaData* metaMsg);

#endif // DATA_SUPPORT_H
