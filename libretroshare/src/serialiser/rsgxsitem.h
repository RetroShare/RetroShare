#ifndef RSGXSITEM_H
#define RSGXSITEM_H

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"


class RsGxsGrpItem : RsItem
{

    RsGxsItem() : RsItem(0) { return; }
    virtual ~RsGxsItem();

};

class RsGxsMsgItem : RsItem
{

    RsGxsItem() : RsItem(0) { return; }
    virtual ~RsGxsItem();

};

#endif // RSGXSITEM_H
