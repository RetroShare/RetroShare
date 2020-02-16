/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupFeedItem.cpp                             *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
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

#include "gui/gxs/GxsGroupFeedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include <iostream>
#include <algorithm>

/**
 * #define DEBUG_ITEM	1
 **/

GxsGroupFeedItem::GxsGroupFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, RsGxsIfaceHelper *iface, bool autoUpdate) :
    FeedItem(feedHolder,feedId,NULL)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::GxsGroupFeedItem()";
	std::cerr << std::endl;
#endif

	/* this are just generally useful for all children */
	mIsHome = isHome;

	/* load data if we can */
	mGroupId = groupId;
	mGxsIface = iface;

#ifdef TO_REMOVE
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
#endif
}

GxsGroupFeedItem::~GxsGroupFeedItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::~GxsGroupFeedItem()";
	std::cerr << std::endl;
#endif

#ifdef TO_REMOVE
	if (mLoadQueue) {
		delete mLoadQueue;
	}

	if (mUpdateBroadcastBase)
	{
		delete(mUpdateBroadcastBase);
	}
#endif
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

	RetroShareLink link = RetroShareLink::createGxsGroupLink(getLinkType(), mGroupId, groupName());
	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void GxsGroupFeedItem::fillDisplaySlot(bool complete)
{
		requestGroup();
}

#ifdef TO_REMOVE
void GxsGroupFeedItem::fillDisplay(RsGxsUpdateBroadcastBase *updateBroadcastBase, bool /*complete*/)
{
	std::set<RsGxsGroupId> grpIds;
	updateBroadcastBase->getAllGrpIds(grpIds);

	if (grpIds.find(groupId()) != grpIds.end())
		requestGroup();
}
#endif

/***********************************************************/

void GxsGroupFeedItem::requestGroup()
{
    loadGroup();
}

