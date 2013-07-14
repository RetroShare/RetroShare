#pragma once

#include <QObject>
#include <retroshare/rsgxsifacetypes.h>

class QShowEvent;
class RsGxsIfaceHelper;
class RsGxsUpdateBroadcast;

class RsGxsUpdateBroadcastBase : public QObject
{
	friend class RsGxsUpdateBroadcastPage;
	friend class RsGxsUpdateBroadcastWidget;

	Q_OBJECT

protected:
	RsGxsUpdateBroadcastBase(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = NULL);
	virtual ~RsGxsUpdateBroadcastBase();

	void setUpdateWhenInvisible(bool update) { mUpdateWhenInvisible = update; }
	std::list<RsGxsGroupId> &getGrpIds() { return mGrpIds; }
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &getMsgIds() { return mMsgIds; }

	void showEvent(QShowEvent *e);

signals:
	void fillDisplay(bool initialFill);

private slots:
	void updateBroadcastChanged();
	void updateBroadcastGrpsChanged(const std::list<RsGxsGroupId>& grpIds);
	void updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds);
	void securedUpdateDisplay();

private:
	RsGxsUpdateBroadcast *mUpdateBroadcast;
	bool mFirstVisible;
	bool mUpdateWhenInvisible; // Update also when not visible
	std::list<RsGxsGroupId> mGrpIds;
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > mMsgIds;
};
