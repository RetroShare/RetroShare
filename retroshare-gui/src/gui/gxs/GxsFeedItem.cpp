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

	mTokenTypeMessage = nextTokenType();
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
		mFeedHolder->openComments(feedId(), groupId(), messageId(), title);
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

	RetroShareLink link;
	if (link.createGxsMessageLink(getLinkType(), groupId(), mMessageId, messageName())) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void GxsFeedItem::fillDisplay(RsGxsUpdateBroadcastBase *updateBroadcastBase, bool complete)
{
	GxsGroupFeedItem::fillDisplay(updateBroadcastBase, complete);

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;
	updateBroadcastBase->getAllMsgIds(msgs);

	if (!msgs.empty())
	{
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mit = msgs.find(groupId());
		if (mit != msgs.end())
		{
			const std::vector<RsGxsMessageId> &msgIds = mit->second;
			if (std::find(msgIds.begin(), msgIds.end(), messageId()) != msgIds.end()) {
				requestMessage();
			}
		}
	}
}

void GxsFeedItem::requestMessage()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::requestMessage()";
	std::cerr << std::endl;
#endif

	if (!initLoadQueue()) {
		return;
	}

	if (mLoadQueue->activeRequestExist(mTokenTypeMessage)) {
		/* Request already running */
		return;
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect_msgIds = msgIds[groupId()];
	vect_msgIds.push_back(mMessageId);

	uint32_t token;
	mLoadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeMessage);
}

void GxsFeedItem::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::loadRequest()";
	std::cerr << std::endl;
#endif

	if (queue == mLoadQueue) {
		if (req.mUserType == mTokenTypeMessage) {
			loadMessage(req.mToken);
			return;
		}
	}

	GxsGroupFeedItem::loadRequest(queue, req);
}

bool GxsFeedItem::isLoading()
{
	if (GxsGroupFeedItem::isLoading()) {
		return true;
	}

	if (mLoadQueue && mLoadQueue->activeRequestExist(mTokenTypeMessage)) {
		return true;
	}

	return false;
}
