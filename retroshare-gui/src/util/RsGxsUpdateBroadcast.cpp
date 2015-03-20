#include <QMap>

#include "RsGxsUpdateBroadcast.h"
#include "gui/notifyqt.h"

#include <retroshare/rsgxsifacehelper.h>

// previously gxs allowed only one event consumer to poll for changes
// this required a single broadcast instance per service
// now the update notify works through rsnotify and notifyqt
// so the single instance per service is not really needed anymore

QMap<RsGxsIfaceHelper*, RsGxsUpdateBroadcast*> updateBroadcastMap;

RsGxsUpdateBroadcast::RsGxsUpdateBroadcast(RsGxsIfaceHelper *ifaceImpl) :
	QObject(NULL), mIfaceImpl(ifaceImpl)
{
    connect(NotifyQt::getInstance(), SIGNAL(gxsChange(RsGxsChanges)), this, SLOT(onChangesReceived(RsGxsChanges)));
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

	return updateBroadcast;
}

void RsGxsUpdateBroadcast::onChangesReceived(const RsGxsChanges& changes)
{
    if(changes.mService != mIfaceImpl->getTokenService())
        return;

    if (!changes.mMsgs.empty() || !changes.mMsgsMeta.empty())
    {
        emit msgsChanged(changes.mMsgs, changes.mMsgsMeta);
    }

    if (!changes.mGrps.empty() || !changes.mGrpsMeta.empty())
    {
        emit grpsChanged(changes.mGrps, changes.mGrpsMeta);
    }

    emit changed();
}
