/*
 * Retroshare Gxs Feed Item
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QTimer>

#include "gui/gxs/GxsGroupFeedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include <iostream>
//#include <algorithm>

/**
 * #define DEBUG_ITEM	1
 **/

GxsGroupFeedItem::GxsGroupFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, RsGxsIfaceHelper *iface, bool autoUpdate) :
    FeedItem(NULL)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::GxsGroupFeedItem()";
	std::cerr << std::endl;
#endif

	/* this are just generally useful for all children */
	mFeedHolder = feedHolder;
	mFeedId = feedId;
	mIsHome = isHome;

	/* load data if we can */
	mGroupId = groupId;
	mGxsIface = iface;

	mNextTokenType = 0;
	mTokenTypeGroup = nextTokenType();

	mLoadQueue = NULL;

	if (mGxsIface && autoUpdate) {
		/* Connect to update broadcast */
		mUpdateBroadcastBase = new RsGxsUpdateBroadcastBase(mGxsIface);
		connect(mUpdateBroadcastBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplaySlot(bool)));
	} else {
		mUpdateBroadcastBase = NULL;
	}
}

GxsGroupFeedItem::~GxsGroupFeedItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::~GxsGroupFeedItem()";
	std::cerr << std::endl;
#endif

	if (mLoadQueue) {
		delete mLoadQueue;
	}

	if (mUpdateBroadcastBase)
	{
		delete(mUpdateBroadcastBase);
	}
}

bool GxsGroupFeedItem::initLoadQueue()
{
	if (mLoadQueue) {
		return true;
	}

	if (!mGxsIface) {
		return false;
	}

	mLoadQueue = new TokenQueue(mGxsIface->getTokenService(), this);
	return (mLoadQueue != NULL);
}

void GxsGroupFeedItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::removeItem()";
	std::cerr << std::endl;
#endif

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
	}

	hide();

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
		mFeedHolder->deleteFeedItem(this, mFeedId);
	}
}

void GxsGroupFeedItem::unsubscribe()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::unsubscribe()";
	std::cerr << std::endl;
#endif

	if (mGxsIface)
	{
		uint32_t token;
		mGxsIface->subscribeToGroup(token, mGroupId, false);
	}
}

void GxsGroupFeedItem::subscribe()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::subscribe()";
	std::cerr << std::endl;
#endif

	if (mGxsIface)
	{
		uint32_t token;
		mGxsIface->subscribeToGroup(token, mGroupId, true);
	}
}

void GxsGroupFeedItem::copyGroupLink()
{
	if (mGroupId.isNull()) {
		return;
	}

	if (getLinkType() == RetroShareLink::TYPE_UNKNOWN) {
		return;
	}

	RetroShareLink link;
	if (link.createGxsGroupLink(getLinkType(), mGroupId, groupName())) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void GxsGroupFeedItem::fillDisplaySlot(bool complete)
{
	fillDisplay(mUpdateBroadcastBase, complete);
}

void GxsGroupFeedItem::fillDisplay(RsGxsUpdateBroadcastBase *updateBroadcastBase, bool /*complete*/)
{
	std::list<RsGxsGroupId> grpIds;
	updateBroadcastBase->getAllGrpIds(grpIds);

	if (std::find(grpIds.begin(), grpIds.end(), groupId()) != grpIds.end()) {
		requestGroup();
	}
}

/***********************************************************/

void GxsGroupFeedItem::requestGroup()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::requestGroup()";
	std::cerr << std::endl;
#endif

	if (!initLoadQueue()) {
		return;
	}

	if (mLoadQueue->activeRequestExist(mTokenTypeGroup)) {
		/* Request already running */
		return;
	}

	std::list<RsGxsGroupId> ids;
	ids.push_back(mGroupId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
	mLoadQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, ids, mTokenTypeGroup);
}

void GxsGroupFeedItem::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::loadRequest()";
	std::cerr << std::endl;
#endif

	if (queue == mLoadQueue) {
		if (req.mUserType == mTokenTypeGroup) {
			loadGroup(req.mToken);
		} else {
			std::cerr << "GxsGroupFeedItem::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		}
	}
}

bool GxsGroupFeedItem::isLoading()
{
	if (mLoadQueue && mLoadQueue->activeRequestExist(mTokenTypeGroup)) {
		return true;
	}

	return false;
}
