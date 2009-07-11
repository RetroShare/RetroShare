#include "rscontrol.h"

RsControl::RsControl(RsIface &i, NotifyBase &callback) :
        cb(callback),
        rsIface(i)
{
}

RsControl::~RsControl()
{
}

NotifyBase& RsControl::getNotify()
{
    return cb;
}

RsIface& RsControl::getIface()
{
    return rsIface;
}
