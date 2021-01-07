/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastBase.cpp                     *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QTimer>

#include "RsGxsUpdateBroadcastBase.h"
#include <retroshare-gui/RsAutoUpdatePage.h>

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
	connect(mUpdateBroadcast, SIGNAL(msgsChanged(std::map<RsGxsGroupId,std::set<RsGxsMessageId> >, std::map<RsGxsGroupId,std::set<RsGxsMessageId> >)), this, SLOT(updateBroadcastMsgsChanged(std::map<RsGxsGroupId,std::set<RsGxsMessageId> >,std::map<RsGxsGroupId,std::set<RsGxsMessageId> >)));
	connect(mUpdateBroadcast, SIGNAL(distantSearchResultsChanged(const std::list<TurtleRequestId>&)), this, SLOT(updateBroadcastDistantSearchResultsChanged(const std::list<TurtleRequestId>&)));
}

RsGxsUpdateBroadcastBase::~RsGxsUpdateBroadcastBase()
{
}

void RsGxsUpdateBroadcastBase::updateBroadcastDistantSearchResultsChanged(const std::list<TurtleRequestId>& ids)
{
    for(auto it(ids.begin());it!=ids.end();++it)
        mTurtleResults.insert(*it);
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

    // This is *bad* because if the connection is done asynchronously the client will call mGrpIds, mGrpIdsMeta, etc without the guarranty that the
    // the structed havnt' been cleared in the mean time.

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

        // (cyril) Re-load the entire group is new messages are here, or if group metadata has changed (e.g. visibility permissions, admin rights, etc).
        // Do not re-load if Msg data has changed, which means basically the READ flag has changed, because this action is done in the UI in the
        // first place so there's no need to re-update the UI once this is done.
        //
        // The question to whether we should re=load when mGrpIds is not empty is still open. It's not harmful anyway.
        // This should probably be decided by the service itself.

		if (!mGrpIds.empty() || !mGrpIdsMeta.empty() /*|| !mMsgIds.empty()*/ || !mMsgIdsMeta.empty() || !mTurtleResults.empty())
            mFillComplete = true ;

		securedUpdateDisplay();
	}
}

void RsGxsUpdateBroadcastBase::updateBroadcastGrpsChanged(const std::list<RsGxsGroupId> &grpIds, const std::list<RsGxsGroupId> &grpIdsMeta)
{
	std::list<RsGxsGroupId>::const_iterator it;
	for (it = grpIds.begin(); it != grpIds.end(); ++it)
        mGrpIds.insert(*it) ;

	for (it = grpIdsMeta.begin(); it != grpIdsMeta.end(); ++it)
        mGrpIdsMeta.insert(*it);
}

template<class U> void merge(std::set<U>& dst,const std::set<U>& src) { for(auto it(src.begin());it!=src.end();++it) dst.insert(*it) ; }

void RsGxsUpdateBroadcastBase::updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds, const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIdsMeta)
{
	for (auto mapIt = msgIds.begin(); mapIt != msgIds.end(); ++mapIt)
        merge(mMsgIds[mapIt->first],mapIt->second) ;

	for (auto mapIt = msgIdsMeta.begin(); mapIt != msgIdsMeta.end(); ++mapIt)
        merge(mMsgIdsMeta[mapIt->first],mapIt->second) ;
}

void RsGxsUpdateBroadcastBase::getAllGrpIds(std::set<RsGxsGroupId> &grpIds)
{
    grpIds = mGrpIds;
    merge(grpIds,mGrpIdsMeta) ;
}

void RsGxsUpdateBroadcastBase::getAllMsgIds(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds)
{
	/* Copy first map */
	msgIds = mMsgIds;

	/* Append second map */
	for (auto mapIt = mMsgIdsMeta.begin(); mapIt != mMsgIdsMeta.end(); ++mapIt)
        merge(msgIds[mapIt->first],mapIt->second);
}
