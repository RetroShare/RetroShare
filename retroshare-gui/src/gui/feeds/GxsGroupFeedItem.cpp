/*******************************************************************************
 * retroshare-gui/src/retroshare-gui/src/gui/feeds/GxsGroupFeedItem.cpp        *
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

#include "GxsGroupFeedItem.h"

#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include "util/qtthreadsutils.h"

#include <iostream>
#include <algorithm>

/**
 * #define DEBUG_ITEM	1
 **/

const uint GxsGroupFeedItem::GROUP_ITEM_LOADING_TIMEOUT_ms = 2000;

GxsGroupFeedItem::GxsGroupFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, RsGxsIfaceHelper *iface, bool /*autoUpdate*/) :
    FeedItem(feedHolder,feedId,NULL)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::GxsGroupFeedItem()";
	std::cerr << std::endl;
#endif

	/* this are just generally useful for all children */
	mIsHome = isHome;
    mLastDelay = 300;	// re-update after 300ms on fail. See deferred_update()

	/* load data if we can */
	mGroupId = groupId;
	mGxsIface = iface;
}

GxsGroupFeedItem::~GxsGroupFeedItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsGroupFeedItem::~GxsGroupFeedItem()";
	std::cerr << std::endl;
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

void GxsGroupFeedItem::fillDisplaySlot(bool /*complete*/)
{
		requestGroup();
}

/***********************************************************/

void GxsGroupFeedItem::requestGroup()
{
    loadGroup();
}

void GxsGroupFeedItem::deferred_update()
{
    mLastDelay = (int)(float(mLastDelay)*1.2);
    mLastDelay += 100.0*RsRandom::random_f32();

    if(mLastDelay < 10000.0)
    {
        std::cerr << "Launching deferred update at " << mLastDelay << " ms." << std::endl;
        RsQThreadUtils::postToObject( [this]() { QTimer::singleShot(mLastDelay,this,SLOT(update())); }, this );
    }
}





