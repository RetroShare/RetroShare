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

#ifndef _PEER_ITEM_DIALOG_H
#define _PEER_ITEM_DIALOG_H

#include "ui_PeerItem.h"
#include <stdint.h>

const uint32_t PEER_TYPE_STD     = 0x0001;
const uint32_t PEER_TYPE_CONNECT = 0x0002;
const uint32_t PEER_TYPE_HELLO   = 0x0003; /* failed Connect Attempt */
const uint32_t PEER_TYPE_NEW_FOF = 0x0004; /* new Friend of Friend */

class FeedHolder;

class PeerItem : public QWidget, private Ui::PeerItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	PeerItem(FeedHolder *parent, uint32_t feedId, const std::string &peerId, uint32_t type, bool isHome);

	void updateItemStatic();

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void addFriend();
	void removeFriend();
	void sendMsg();
	void openChat();

	void updateItem();

	void togglequickmessage();
	void sendMessage();
	
	void on_quickmsgText_textChanged();

private:
	FeedHolder *mParent;
	uint32_t mFeedId;

	std::string mPeerId;
	uint32_t mType;
	bool mIsHome;
};

#endif

