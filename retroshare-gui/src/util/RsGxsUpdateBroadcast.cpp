#include <QTimer>

#include "RsGxsUpdateBroadcast.h"


RsGxsUpdateBroadcast::RsGxsUpdateBroadcast(RsGxsIfaceImpl *ifaceImpl, float dt, QObject *parent) :
    QObject(parent), mIfaceImpl(ifaceImpl), mDt(dt)
{
}

void RsGxsUpdateBroadcast::startMonitor()
{
    slowPoll();
}


void RsGxsUpdateBroadcast::fastPoll()
{

}

void RsGxsUpdateBroadcast::slowPoll()
{
    std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;
    std::list<RsGxsGroupId> grps;

    if(mIfaceImpl->updated())
    {
        mIfaceImpl->msgsChanged(msgs);
        if(!msgs.empty())
        {
            emit msgsChanged(msgs);
        }

        mIfaceImpl->groupsChanged(grps);

        if(!grps.empty())
        {
            emit grpsChanged(grps);
        }

        QTimer::singleShot((int) (mDt * 1000.0), this, SLOT(slowPoll()));
    }
}
