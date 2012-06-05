#ifndef DATA_SUPPORT_H
#define DATA_SUPPORT_H

#include "serialiser/rsnxsitems.h"

bool operator==(const RsNxsGrp&, const RsNxsGrp&);
bool operator==(const RsNxsMsg&, const RsNxsMsg&);

void init_item(RsNxsGrp* nxg);
void init_item(RsNxsMsg* nxm);

#endif // DATA_SUPPORT_H
