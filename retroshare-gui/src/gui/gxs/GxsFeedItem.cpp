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

#include <iostream>

/**
 * #define DEBUG_ITEM	1
 **/

#define DEBUG_ITEM	1

#define GXSFEEDITEM_GROUPMETA	5
#define GXSFEEDITEM_MESSAGE	6


void GxsFeedItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::removeItem()";
	std::cerr << std::endl;
#endif

	if (mParent)
	{
		mParent->lockLayout(this, true);
	}

	hide();

	if (mParent)
	{
		mParent->lockLayout(this, false);
		mParent->deleteFeedItem(this, mFeedId);
	}
}

void GxsFeedItem::comments(const QString &title)
{
#ifdef DEBUG_ITEM
        std::cerr << "GxsFeedItem::comments()";
        std::cerr << std::endl;
#endif

        if (mParent)
        {
                mParent->openComments(mFeedId, mGroupId, mMessageId, title);
        }
}

void GxsFeedItem::unsubscribe()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::unsubscribe()";
	std::cerr << std::endl;
#endif

	if (mGxsIface)
	{
    		uint32_t token;
    		mGxsIface->subscribeToGroup(token, mGroupId, false);
	}
}


void GxsFeedItem::subscribe()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::subscribe()";
	std::cerr << std::endl;
#endif

	if (mGxsIface)
	{
    		uint32_t token;
    		mGxsIface->subscribeToGroup(token, mGroupId, true);
	}
}


void GxsFeedItem::updateItemStatic()
{
	std::cerr << "GxsFeedItem::updateItemStatic()";
	std::cerr << std::endl;
        requestMessage();
}


void GxsFeedItem::updateItem()
{
	std::cerr << "GxsFeedItem::updateItem() EMPTY";
	std::cerr << std::endl;
}


/***********************************************************/

GxsFeedItem::GxsFeedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, RsGxsIfaceHelper *iface, bool loadData)
	:QWidget(NULL)
{
        std::cerr << "GxsFeedItem::GxsFeedItem()";
        std::cerr << std::endl;

	/* this are just generally useful for all children */
	mParent = parent;
	mFeedId = feedId;
	mIsHome = isHome;

	/* load data if we can */
	mGroupId = groupId;
	mMessageId = messageId;
	mGxsIface = iface;

	if (loadData && iface)
	{
		mLoadQueue = new TokenQueue(iface->getTokenService(), this);
		requestGroupMeta();
	}
	else
	{
		mLoadQueue = NULL;
	}
}


GxsFeedItem::~GxsFeedItem()
{
        std::cerr << "GxsFeedItem::~GxsFeedItem()";
        std::cerr << std::endl;

	if (mLoadQueue)
	{
		delete mLoadQueue;
	}
}


void GxsFeedItem::requestGroupMeta()
{
        std::cerr << "GxsFeedItem::requestGroup()";
        std::cerr << std::endl;

	if (!mLoadQueue)
	{
		return;
	}

        std::list<std::string> ids;
        ids.push_back(mGroupId);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
        uint32_t token;
        mLoadQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, ids, GXSFEEDITEM_GROUPMETA);

	updateItemStatic();
}


void GxsFeedItem::requestMessage()
{
        std::cerr << "GxsFeedItem::requestMessage()";
        std::cerr << std::endl;

	if (!mLoadQueue)
	{
		return;
	}

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

        GxsMsgReq msgIds;
        std::vector<RsGxsMessageId> &vect_msgIds = msgIds[mGroupId];
        vect_msgIds.push_back(mMessageId);

        uint32_t token;
        mLoadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, GXSFEEDITEM_MESSAGE);
}


void GxsFeedItem::loadGroupMeta(const uint32_t &token)
{
        std::cerr << "GxsFeedItem::loadGroupMeta()";
        std::cerr << std::endl;

        std::list<RsGroupMetaData> groupMeta;

        if (!mGxsIface->getGroupSummary(token, groupMeta))
        {
                std::cerr << "GxsFeedItem::loadGroupMeta() Error getting GroupMeta";
                std::cerr << std::endl;
                return;
        }

        if (groupMeta.size() == 1)
        {
                mGroupMeta = *groupMeta.begin();
        }
        else
        {
                std::cerr << "GxsFeedItem::loadGroupMeta() ERROR Should be ONE GroupMeta";
                std::cerr << std::endl;
        }
}


void GxsFeedItem::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
        std::cerr << "GxsFeedItem::loadRequest()";
        std::cerr << std::endl;

        if (queue == mLoadQueue)
        {
                switch(req.mUserType)
                {
                        case GXSFEEDITEM_GROUPMETA:
                                loadGroupMeta(req.mToken);
                                break;

                        case GXSFEEDITEM_MESSAGE:
                                loadMessage(req.mToken);
                                break;

                        default:
                                std::cerr << "GxsFeedItem::loadRequest() ERROR: INVALID TYPE";
                                std::cerr << std::endl;
                        break;
                }
        }
}


