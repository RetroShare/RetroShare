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

#ifndef _GXS_GENERIC_FEED_ITEM_H
#define _GXS_GENERIC_FEED_ITEM_H

#include <retroshare/rsgxsifacehelper.h>
#include "util/TokenQueue.h"
#include <stdint.h>

class FeedHolder;

class GxsFeedItem : public QWidget, public TokenResponse
{
	Q_OBJECT

public:
	/** Note parent can = NULL */
	GxsFeedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, RsGxsIfaceHelper *iface, bool loadData);
virtual ~GxsFeedItem();

protected:

	// generic Fns - to be overloaded.
virtual void updateItemStatic();
virtual void updateItem();
virtual void loadMessage(const uint32_t &token) = 0;
	

	void requestMessage();

virtual void loadGroupMeta(const uint32_t &token);
virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	// general fns that can be implemented here.
protected slots:

	void comments(const QString &title);
	void subscribe();
	void unsubscribe();
	void removeItem();

protected:

	FeedHolder *mParent;
	uint32_t    mFeedId;
	bool	    mIsHome;

	RsGxsGroupId mGroupId;
	RsGxsMessageId mMessageId;

	RsGroupMetaData mGroupMeta;

private:

	void requestGroupMeta();

	RsGxsIfaceHelper *mGxsIface;
	TokenQueue *mLoadQueue;
};

#endif

