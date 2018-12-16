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

	mTokenTypeMessage = nextTokenType();
	mTokenTypeComment = nextTokenType();
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

void GxsFeedItem::fillDisplay(RsGxsUpdateBroadcastBase *updateBroadcastBase, bool complete)
{
	GxsGroupFeedItem::fillDisplay(updateBroadcastBase, complete);

	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > msgs;
	updateBroadcastBase->getAllMsgIds(msgs);

	if (!msgs.empty())
	{
		std::map<RsGxsGroupId, std::set<RsGxsMessageId> >::const_iterator mit = msgs.find(groupId());

		if (mit != msgs.end() && mit->second.find(messageId()) != mit->second.end())
				requestMessage();
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
	std::set<RsGxsMessageId> &vect_msgIds = msgIds[groupId()];
	vect_msgIds.insert(mMessageId);

	uint32_t token;
	mLoadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeMessage);
}

void GxsFeedItem::requestComment()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::requestComment()";
	std::cerr << std::endl;
#endif

	if (!initLoadQueue()) {
		return;
	}

	if (mLoadQueue->activeRequestExist(mTokenTypeComment)) {
		/* Request already running */
		return;
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;

	std::vector<RsGxsGrpMsgIdPair> msgIds;

	for(int i=0;i<mMessageVersions.size();++i)
		msgIds.push_back(std::make_pair(groupId(),mMessageVersions[i])) ;

	msgIds.push_back(std::make_pair(groupId(),messageId()));

	uint32_t token;
	mLoadQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeComment);
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
		if (req.mUserType == mTokenTypeComment) {
			loadComment(req.mToken);
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
