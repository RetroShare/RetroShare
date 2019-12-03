/*******************************************************************************
 * gui/feeds/ChatMsgItem.h                                                     *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
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

#ifndef _CHATMSG_ITEM_DIALOG_H
#define _CHATMSG_ITEM_DIALOG_H

#include "ui_ChatMsgItem.h"
#include "FeedItem.h"
#include <stdint.h>

class FeedHolder;

class ChatMsgItem : public FeedItem, private Ui::ChatMsgItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	ChatMsgItem(FeedHolder *parent, uint32_t feedId, const RsPeerId &peerId, const std::string &message);

	void updateItemStatic();

    virtual QString uniqueIdentifier() const override { return "ChatMsgItem " + QString::fromStdString(mPeerId.toStdString()) ;}
protected:
	/* FeedItem */
	virtual void doExpand(bool /*open*/) {}

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

	RsPeerId mPeerId;
};

#endif
