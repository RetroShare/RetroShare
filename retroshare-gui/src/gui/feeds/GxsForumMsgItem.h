/*
 * Retroshare Gxs Feed Item
 *
 * Copyright 2014 RetroShare Team
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

#ifndef _GXSFORUMMSGITEM_H
#define _GXSFORUMMSGITEM_H

#include <retroshare/rsgxsforums.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class GxsForumMsgItem;
}

class FeedHolder;

class GxsForumMsgItem : public GxsFeedItem
{
	Q_OBJECT

public:
	GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate);
	GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumGroup &group, const RsGxsForumMsg &post, bool isHome, bool autoUpdate);
	GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumMsg &post, bool isHome, bool autoUpdate);
	virtual ~GxsForumMsgItem();

	bool setGroup(const RsGxsForumGroup &group, bool doFill = true);
	bool setMessage(const RsGxsForumMsg &msg, bool doFill = true);

protected:
	/* FeedItem */
	virtual void doExpand(bool open);
	virtual void expandFill(bool first);

	/* load message data */
	void requestParentMessage(const RsGxsMessageId &msgId);
	virtual void loadParentMessage(const uint32_t &token);

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_FORUM; }
	virtual bool isLoading();

	/* GxsFeedItem */
	virtual void loadMessage(const uint32_t &token);
	virtual QString messageName();

private slots:
	/* default stuff */
	void toggle();
	void readAndClearItem();

	void unsubscribeForum();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);	

private:
	void setup();
	void fill();
	void fillExpandFrame();
	void setReadStatus(bool isNew, bool isUnread);
	void setAsRead();
	bool isTop();

private:
	bool mInFill;
	bool mCloseOnRead;

	RsGxsForumGroup mGroup;
	RsGxsForumMsg mMessage;
	RsGxsForumMsg mParentMessage;
	uint32_t mTokenTypeParentMessage;

	/** Qt Designer generated object */
	Ui::GxsForumMsgItem *ui;
};

#endif
