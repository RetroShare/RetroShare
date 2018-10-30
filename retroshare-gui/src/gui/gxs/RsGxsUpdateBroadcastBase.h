#pragma once

#include <QObject>
#include <retroshare/rsgxsifacetypes.h>

class QShowEvent;
struct RsGxsIfaceHelper;
class RsGxsUpdateBroadcast;

typedef uint32_t TurtleRequestId ;

class RsGxsUpdateBroadcastBase : public QObject
{
	friend class RsGxsUpdateBroadcastPage;
	friend class RsGxsUpdateBroadcastWidget;
	friend class GxsIdChooser;

	Q_OBJECT

public:
	RsGxsUpdateBroadcastBase(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = nullptr);
	virtual ~RsGxsUpdateBroadcastBase();

	const std::set<RsGxsGroupId> &getGrpIds() { return mGrpIds; }
	const std::set<RsGxsGroupId> &getGrpIdsMeta() { return mGrpIdsMeta; }
	void getAllGrpIds(std::set<RsGxsGroupId> &grpIds);
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIds() { return mMsgIds; }
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIdsMeta() { return mMsgIdsMeta; }
	void getAllMsgIds(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds);
    const std::set<TurtleRequestId>& getSearchResults() { return mTurtleResults ; }

protected:
	void fillComplete();
	void setUpdateWhenInvisible(bool update) { mUpdateWhenInvisible = update; }

	void showEvent(QShowEvent *e);

signals:
	void fillDisplay(bool complete);

private slots:
	void updateBroadcastChanged();
	void updateBroadcastGrpsChanged(const std::list<RsGxsGroupId>& grpIds, const std::list<RsGxsGroupId> &grpIdsMeta);
	void updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIds, const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIdsMeta);
	void updateBroadcastDistantSearchResultsChanged(const std::list<TurtleRequestId>& ids);
	void securedUpdateDisplay();

private:
	RsGxsUpdateBroadcast *mUpdateBroadcast;
	bool mFillComplete;
	bool mUpdateWhenInvisible; // Update also when not visible
	std::set<RsGxsGroupId> mGrpIds;
	std::set<RsGxsGroupId> mGrpIdsMeta;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgIds;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgIdsMeta;
    std::set<TurtleRequestId> mTurtleResults;
};
