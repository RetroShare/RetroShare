/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _FORUM_MSG_ITEM_DIALOG_H
#define _FORUM_MSG_ITEM_DIALOG_H

#include "ui_ForumMsgItem.h"

class FeedHolder;

#include <stdint.h>
class ForumMsgItem : public QWidget, private Ui::ForumMsgItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	ForumMsgItem(FeedHolder *parent, uint32_t feedId, const std::string &forumId, const std::string &postId, bool isHome);

	void updateItemStatic();

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void readAndClearItem();
	void unsubscribeForum();
	void subscribeForum();
	void replyToPost();
	void sendMsg();

	void forumMsgReadSatusChanged(const QString &forumId, const QString &msgId, int status);

	void updateItem();

private:
	void setAsRead();

	FeedHolder *mParent;
	uint32_t mFeedId;
	bool canReply;

	std::string mForumId;
	std::string mPostId;
	bool mIsHome;
	bool mIsTop;
};

#endif

