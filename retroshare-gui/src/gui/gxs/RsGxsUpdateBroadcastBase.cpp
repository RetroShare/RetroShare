#include <QTimer>

#include "RsGxsUpdateBroadcastBase.h"
#include "RsAutoUpdatePage.h"
#include "util/RsGxsUpdateBroadcast.h"

#include <algorithm>

RsGxsUpdateBroadcastBase::RsGxsUpdateBroadcastBase(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
	: QObject(parent)
{
	mUpdateWhenInvisible = false;
	mFirstVisible = true;

	mUpdateBroadcast = RsGxsUpdateBroadcast::get(ifaceImpl);
	connect(mUpdateBroadcast, SIGNAL(changed()), this, SLOT(updateBroadcastChanged()));
	connect(mUpdateBroadcast, SIGNAL(grpsChanged(std::list<RsGxsGroupId>)), this, SLOT(updateBroadcastGrpsChanged(std::list<RsGxsGroupId>)));
	connect(mUpdateBroadcast, SIGNAL(msgsChanged(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >)), this, SLOT(updateBroadcastMsgsChanged(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >)));
}

RsGxsUpdateBroadcastBase::~RsGxsUpdateBroadcastBase()
{
}

void RsGxsUpdateBroadcastBase::securedUpdateDisplay()
{
	if (RsAutoUpdatePage::eventsLocked()) {
		/* Wait until events are not locked */
		QTimer::singleShot(500, this, SLOT(securedUpdateDisplay()));
		return;
	}

	emit fillDisplay(mFirstVisible);
	mFirstVisible = false;

	/* Clear updated ids */
	mGrpIds.clear();
	mMsgIds.clear();
}

void RsGxsUpdateBroadcastBase::showEvent(QShowEvent */*event*/)
{
	if (mFirstVisible) {
		/* Initial fill */
		securedUpdateDisplay();
	}

	if (!mUpdateWhenInvisible) {
		if (!mGrpIds.empty() || !mMsgIds.empty()) {
			securedUpdateDisplay();
		}
	}
}

void RsGxsUpdateBroadcastBase::updateBroadcastChanged()
{
	QWidget *widget = qobject_cast<QWidget *>(parent());

	/* Update only update when the widget is visible. */
	if (mUpdateWhenInvisible || !widget || widget->isVisible()) {
		securedUpdateDisplay();
	}
}

void RsGxsUpdateBroadcastBase::updateBroadcastGrpsChanged(const std::list<RsGxsGroupId> &grpIds)
{
	std::list<RsGxsGroupId>::const_iterator it;
	for (it = grpIds.begin(); it != grpIds.end(); ++it) {
		if (std::find(mGrpIds.begin(), mGrpIds.end(), *it) == mGrpIds.end()) {
			mGrpIds.push_back(*it);
		}
	}
}

void RsGxsUpdateBroadcastBase::updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds)
{
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mapIt;
	for (mapIt = msgIds.begin(); mapIt != msgIds.end(); ++mapIt) {
		const RsGxsGroupId &grpId = mapIt->first;
		const std::vector<RsGxsMessageId> &srcMsgIds = mapIt->second;
		std::vector<RsGxsMessageId> &destMsgIds = mMsgIds[grpId];

		std::vector<RsGxsMessageId>::const_iterator msgIt;
		for (msgIt = srcMsgIds.begin(); msgIt != srcMsgIds.end(); ++msgIt) {
			if (std::find(destMsgIds.begin(), destMsgIds.end(), *msgIt) == destMsgIds.end()) {
				destMsgIds.push_back(*msgIt);
			}
		}
	}
}
