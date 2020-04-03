/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsFeedItem.cpp                                  *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#include "gui/gxs/GxsFeedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include <iostream>
#include <algorithm>

/**
 * #define DEBUG_ITEM	1
 **/

GxsFeedItem::GxsFeedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, RsGxsIfaceHelper *iface, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, iface, autoUpdate)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::GxsFeedItem()";
	std::cerr << std::endl;
#endif

	/* load data if we can */
	mMessageId = messageId;
}

GxsFeedItem::~GxsFeedItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::~GxsFeedItem()";
	std::cerr << std::endl;
#endif
}

void GxsFeedItem::comments(const QString &title)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::comments()";
	std::cerr << std::endl;
#endif

	if (mFeedHolder)
	{
        if(mMessageVersions.empty())
			mFeedHolder->openComments(feedId(), groupId(),QVector<RsGxsMessageId>(1,messageId()), messageId(), title);
		else
			mFeedHolder->openComments(feedId(), groupId(),mMessageVersions, messageId(), title);
	}
}

void GxsFeedItem::copyMessageLink()
{
	if (groupId().isNull() || mMessageId.isNull()) {
		return;
	}

	if (getLinkType() == RetroShareLink::TYPE_UNKNOWN) {
		return;
	}

	RetroShareLink link = RetroShareLink::createGxsMessageLink(getLinkType(), groupId(), mMessageId, messageName());
	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void GxsFeedItem::requestMessage()
{
    loadMessage();

}

void GxsFeedItem::requestComment()
{
    loadComment();
}

