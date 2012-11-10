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

#ifndef _SECURITY_ITEM_DIALOG_H
#define _SECURITY_ITEM_DIALOG_H

#include "ui_SecurityItem.h"
#include <stdint.h>

const uint32_t SEC_TYPE_CONNECT_ATTEMPT  = 0x0001; /* failed Connect Attempt */
const uint32_t SEC_TYPE_AUTH_DENIED      = 0x0002; /* failed outgoing attempt */
const uint32_t SEC_TYPE_UNKNOWN_IN       = 0x0003; /* failed incoming with unknown peer */
const uint32_t SEC_TYPE_UNKNOWN_OUT      = 0x0004; /* failed outgoing with unknown peer */

class FeedHolder;

class SecurityItem : public QWidget, private Ui::SecurityItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	SecurityItem(FeedHolder *parent, uint32_t feedId, const std::string &gpgId, const std::string &sslId, const std::string& ip_addr,uint32_t type, bool isHome);

	void updateItemStatic();

	bool isSame(const std::string &sslId, uint32_t type);

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void friendRequest();
	void removeFriend();
	void peerDetails();
	void sendMsg();
	void openChat();

	void updateItem();

	void togglequickmessage();
	void sendMessage();

	void on_quickmsgText_textChanged();

private:
	FeedHolder *mParent;
	uint32_t mFeedId;

	std::string mSslId;
	std::string mGpgId;
	std::string mIP;
	uint32_t mType;
	bool mIsHome;
};

#endif

