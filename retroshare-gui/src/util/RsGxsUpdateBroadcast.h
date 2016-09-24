#ifndef RSGXSUPDATEBROADCAST_H
#define RSGXSUPDATEBROADCAST_H

#include <QObject>

#include <retroshare/rsgxsifacetypes.h>

class RsGxsIfaceHelper;
class RsGxsChanges;

class RsGxsUpdateBroadcast : public QObject
{
	Q_OBJECT

public:
	static void cleanup();

	static RsGxsUpdateBroadcast *get(RsGxsIfaceHelper* ifaceImpl);

signals:
	void changed();
	void msgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds, const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIdsMeta);
	void grpsChanged(const std::list<RsGxsGroupId>& grpIds, const std::list<RsGxsGroupId>& grpIdsMeta);

private slots:
    void onChangesReceived(const RsGxsChanges& changes);

private:
	explicit RsGxsUpdateBroadcast(RsGxsIfaceHelper* ifaceImpl);

private:
	RsGxsIfaceHelper* mIfaceImpl;
};

#endif // RSGXSUPDATEBROADCAST_H
