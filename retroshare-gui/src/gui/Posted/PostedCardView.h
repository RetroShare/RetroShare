/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCardView.h                              *
 *                                                                             *
 * Copyright (C) 2019 by Retroshare Team       <retroshare.project@gmail.com>  *
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

#ifndef _POSTED_CARDVIEW_H
#define _POSTED_CARDVIEW_H

#include <QMetaType>

#include <retroshare/rsposted.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class PostedCardView;
}

class FeedHolder;
class RsPostedPost;

class PostedCardView : public GxsFeedItem
{
	Q_OBJECT

public:
	PostedCardView(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate);
	PostedCardView(FeedHolder *parent, uint32_t feedId, const RsPostedGroup &group, const RsPostedPost &post, bool isHome, bool autoUpdate);
	PostedCardView(FeedHolder *parent, uint32_t feedId, const RsPostedPost &post, bool isHome, bool autoUpdate);
	virtual ~PostedCardView();

	bool setGroup(const RsPostedGroup& group, bool doFill = true);
	bool setPost(const RsPostedPost& post, bool doFill = true);

	const RsPostedPost &getPost() const;
	RsPostedPost &post();

protected:
	/* FeedItem */
	virtual void doExpand(bool open);

private slots:
	void loadComments();
	void makeUpVote();
	void makeDownVote();
	void readToggled(bool checked);
	void readAndClearItem();
	void copyMessageLink();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

protected:
	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_UNKNOWN; }

	/* GxsFeedItem */
	virtual QString messageName();
	virtual void loadMessage(const uint32_t &token);
	virtual void loadComment(const uint32_t &token);

private:
	void setup();
	void fill();
	void setReadStatus(bool isNew, bool isUnread);

private:
	bool mInFill;

	RsPostedGroup mGroup;
	RsPostedPost mPost;
	RsGxsMessageId mMessageId;

	/** Qt Designer generated object */
	Ui::PostedCardView *ui;
};

//Q_DECLARE_METATYPE(RsPostedPost)

#endif
