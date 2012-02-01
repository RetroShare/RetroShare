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

#ifndef _CHATMSG_ITEM_DIALOG_H
#define _CHATMSG_ITEM_DIALOG_H

#include "ui_ChatMsgItem.h"
#include <stdint.h>

class FeedHolder;

class ChatMsgItem : public QWidget, private Ui::ChatMsgItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	ChatMsgItem(FeedHolder *parent, uint32_t feedId, const std::string &peerId, const std::string &message);

	void updateItemStatic();

private slots:
	/* default stuff */
	void gotoHome();
	void removeItem();

	void sendMsg();
	void openChat();

	void updateItem();

	void togglequickmessage();
	void sendMessage();

	void on_quickmsgText_textChanged();

private:
	void insertChat(const std::string &message);

	FeedHolder *mParent;
	uint32_t mFeedId;

	std::string mPeerId;
};

#endif
