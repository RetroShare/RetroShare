#include <QTimer>

#include "RsGxsUpdateBroadcastBase.h"
#include "RsAutoUpdatePage.h"
#include "util/RsGxsUpdateBroadcast.h"

#include <algorithm>

RsGxsUpdateBroadcastBase::RsGxsUpdateBroadcastBase(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
	: QObject(parent)
{
	mUpdateWhenInvisible = false;
	mFillComplete = true;

	mUpdateBroadcast = RsGxsUpdateBroadcast::get(ifaceImpl);
	connect(mUpdateBroadcast, SIGNAL(changed()), this, SLOT(updateBroadcastChanged()));
	connect(mUpdateBroadcast, SIGNAL(grpsChanged(std::list<RsGxsGroupId>, std::list<RsGxsGroupId>)), this, SLOT(updateBroadcastGrpsChanged(std::list<RsGxsGroupId>,std::list<RsGxsGroupId>)));
	connect(mUpdateBroadcast, SIGNAL(msgsChanged(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >, std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >)), this, SLOT(updateBroadcastMsgsChanged(std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >,std::map<RsGxsGroupId,std::vector<RsGxsMessageId> >)));
}

RsGxsUpdateBroadcastBase::~RsGxsUpdateBroadcastBase()
{
}

void RsGxsUpdateBroadcastBase::fillComplete()
{
	mFillComplete = true;

	updateBroadcastChanged();
}

void RsGxsUpdateBroadcastBase::securedUpdateDisplay()
{
	if (RsAutoUpdatePage::eventsLocked()) {
		/* Wait until events are not locked */
		QTimer::singleShot(500, this, SLOT(securedUpdateDisplay()));
		return;
	}

	emit fillDisplay(mFillComplete);
	mFillComplete = false;

	/* Clear updated ids */
	mGrpIds.clear();
	mGrpIdsMeta.clear(),
	mMsgIds.clear();
	mMsgIdsMeta.clear();
}

void RsGxsUpdateBroadcastBase::showEvent(QShowEvent */*event*/)
{
	if (mFillComplete) {
		/* Initial fill */
		securedUpdateDisplay();
	}

	if (!mUpdateWhenInvisible) {
		if (!mGrpIds.empty() || !mGrpIdsMeta.empty() || !mMsgIds.empty() || !mMsgIdsMeta.empty()) {
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

void RsGxsUpdateBroadcastBase::updateBroadcastGrpsChanged(const std::list<RsGxsGroupId> &grpIds, const std::list<RsGxsGroupId> &grpIdsMeta)
{
	std::list<RsGxsGroupId>::const_iterator it;
	for (it = grpIds.begin(); it != grpIds.end(); ++it) {
		if (std::find(mGrpIds.begin(), mGrpIds.end(), *it) == mGrpIds.end()) {
			mGrpIds.push_back(*it);
		}
	}
	for (it = grpIdsMeta.begin(); it != grpIdsMeta.end(); ++it) {
		if (std::find(mGrpIdsMeta.begin(), mGrpIdsMeta.end(), *it) == mGrpIdsMeta.end()) {
			mGrpIdsMeta.push_back(*it);
		}
	}
}

void RsGxsUpdateBroadcastBase::updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds, const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIdsMeta)
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
	for (mapIt = msgIdsMeta.begin(); mapIt != msgIdsMeta.end(); ++mapIt) {
		const RsGxsGroupId &grpId = mapIt->first;
		const std::vector<RsGxsMessageId> &srcMsgIds = mapIt->second;
		std::vector<RsGxsMessageId> &destMsgIds = mMsgIdsMeta[grpId];

		std::vector<RsGxsMessageId>::const_iterator msgIt;
		for (msgIt = srcMsgIds.begin(); msgIt != srcMsgIds.end(); ++msgIt) {
			if (std::find(destMsgIds.begin(), destMsgIds.end(), *msgIt) == destMsgIds.end()) {
				destMsgIds.push_back(*msgIt);
			}
		}
	}
}

void RsGxsUpdateBroadcastBase::getAllGrpIds(std::list<RsGxsGroupId> &grpIds)
{
	std::list<RsGxsGroupId>::const_iterator it;
	for (it = mGrpIds.begin(); it != mGrpIds.end(); ++it) {
		if (std::find(grpIds.begin(), grpIds.end(), *it) == grpIds.end()) {
			grpIds.push_back(*it);
		}
	}
	for (it = mGrpIdsMeta.begin(); it != mGrpIdsMeta.end(); ++it) {
		if (std::find(grpIds.begin(), grpIds.end(), *it) == grpIds.end()) {
			grpIds.push_back(*it);
		}
	}
}

void RsGxsUpdateBroadcastBase::getAllMsgIds(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds)
{
	/* Copy first map */
	msgIds = mMsgIds;

	/* Append second map */
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mapIt;
	for (mapIt = mMsgIdsMeta.begin(); mapIt != mMsgIdsMeta.end(); ++mapIt) {
		const RsGxsGroupId &grpId = mapIt->first;
		const std::vector<RsGxsMessageId> &srcMsgIds = mapIt->second;
		std::vector<RsGxsMessageId> &destMsgIds = msgIds[grpId];

		std::vector<RsGxsMessageId>::const_iterator msgIt;
		for (msgIt = srcMsgIds.begin(); msgIt != srcMsgIds.end(); ++msgIt) {
			if (std::find(destMsgIds.begin(), destMsgIds.end(), *msgIt) == destMsgIds.end()) {
				destMsgIds.push_back(*msgIt);
			}
		}
	}
}
