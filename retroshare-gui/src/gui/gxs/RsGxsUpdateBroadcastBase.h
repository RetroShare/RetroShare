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

public:
	RsGxsUpdateBroadcastBase(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = NULL);
	virtual ~RsGxsUpdateBroadcastBase();

	const std::list<RsGxsGroupId> &getGrpIds() { return mGrpIds; }
	const std::list<RsGxsGroupId> &getGrpIdsMeta() { return mGrpIdsMeta; }
	void getAllGrpIds(std::list<RsGxsGroupId> &grpIds);
	const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &getMsgIds() { return mMsgIds; }
	const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &getMsgIdsMeta() { return mMsgIdsMeta; }
	void getAllMsgIds(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds);

protected:
	void fillComplete();
	void setUpdateWhenInvisible(bool update) { mUpdateWhenInvisible = update; }

	void showEvent(QShowEvent *e);

signals:
	void fillDisplay(bool complete);

private slots:
	void updateBroadcastChanged();
	void updateBroadcastGrpsChanged(const std::list<RsGxsGroupId>& grpIds, const std::list<RsGxsGroupId> &grpIdsMeta);
	void updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds, const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIdsMeta);
	void securedUpdateDisplay();

private:
	RsGxsUpdateBroadcast *mUpdateBroadcast;
	bool mFillComplete;
	bool mUpdateWhenInvisible; // Update also when not visible
	std::list<RsGxsGroupId> mGrpIds;
	std::list<RsGxsGroupId> mGrpIdsMeta;
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > mMsgIds;
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > mMsgIdsMeta;
};
