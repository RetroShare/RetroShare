//#include <QTimer>
#include <QMap>

#include "RsGxsUpdateBroadcast.h"
#include "RsProtectedTimer.h"

#include <retroshare/rsgxsifacehelper.h>

QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*> updateBroadcastMap;

RsGxsUpdateBroadcast::RsGxsUpdateBroadcast(RsGxsIfaceHelper *ifaceImpl) :
	QObject(NULL), mIfaceImpl(ifaceImpl)
{
	mTimer = new RsProtectedTimer(this);
	mTimer->setInterval(1000);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(poll()));
}

void RsGxsUpdateBroadcast::cleanup()
{
	QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*>::iterator it;
	for (it = updateBroadcastMap.begin(); it != updateBroadcastMap.end(); ++it) {
		delete(it.value());
	}

	updateBroadcastMap.clear();
}

RsGxsUpdateBroadcast *RsGxsUpdateBroadcast::get(RsGxsIfaceHelper *ifaceImpl)
{
	QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*>::iterator it = updateBroadcastMap.find(ifaceImpl);
	if (it != updateBroadcastMap.end()) {
		return it.value();
	}

	RsGxsUpdateBroadcast *updateBroadcast = new RsGxsUpdateBroadcast(ifaceImpl);
	updateBroadcastMap.insert(ifaceImpl, updateBroadcast);
	updateBroadcast->poll();

	return updateBroadcast;
}

void RsGxsUpdateBroadcast::poll()
{
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;
	std::list<RsGxsGroupId> grps;

	if (mIfaceImpl->updated(true, true))
	{
		mIfaceImpl->msgsChanged(msgs);
		if (!msgs.empty())
		{
			emit msgsChanged(msgs);
		}

		mIfaceImpl->groupsChanged(grps);
		if (!grps.empty())
		{
			emit grpsChanged(grps);
		}

		emit changed();
	}

	mTimer->start();
}
