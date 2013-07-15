/*
 * Retroshare Posted Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef MRK_POSTED_POSTED_ITEM_H
#define MRK_POSTED_POSTED_ITEM_H

#include "ui_PostedItem.h"

#include <retroshare/rsposted.h>
#include "gui/gxs/GxsFeedItem.h"

class RsPostedPost;

class PostedItem : public GxsFeedItem, private Ui::PostedItem
{
	Q_OBJECT

public:
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome);
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsPostedPost &post, bool isHome);

	const RsPostedPost &getPost() const;
	RsPostedPost &post();
	void setContent(const RsPostedPost& post);

private slots:
	void loadComments();
	void makeUpVote();
	void makeDownVote();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

protected:
	virtual void loadMessage(const uint32_t &token);

private:
	void setup();

	uint32_t mType;
	bool mSelected;
	RsPostedPost mPost;
};

#endif
