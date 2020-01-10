/*******************************************************************************
 * gui/feeds/SecurityItem.h                                                    *
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

#ifndef _SECURITY_ITEM_DIALOG_H
#define _SECURITY_ITEM_DIALOG_H

#include "ui_SecurityItem.h"
#include "FeedItem.h"
#include <stdint.h>

//const uint32_t SEC_TYPE_CONNECT_ATTEMPT  = 0x0001; /* failed Connect Attempt */
//const uint32_t SEC_TYPE_AUTH_DENIED      = 0x0002; /* failed outgoing attempt */
//const uint32_t SEC_TYPE_UNKNOWN_IN       = 0x0003; /* failed incoming with unknown peer */
//const uint32_t SEC_TYPE_UNKNOWN_OUT      = 0x0004; /* failed outgoing with unknown peer */

class FeedHolder;

class SecurityItem : public FeedItem, private Ui::SecurityItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	SecurityItem(FeedHolder *parent, uint32_t feedId, const RsPgpId &gpgId, const RsPeerId &sslId, const std::string &sslCn, const std::string& ip_addr,uint32_t type, bool isHome);

	void updateItemStatic();

    uint64_t uniqueIdentifier() const override;

protected:
	/* FeedItem */
	virtual void doExpand(bool open);

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

private:
	FeedHolder *mParent;
	uint32_t mFeedId;

	RsPgpId  mGpgId;
	RsPeerId mSslId;
	std::string mSslCn;
	std::string mIP;
	uint32_t mType;
	bool mIsHome;
};

#endif

