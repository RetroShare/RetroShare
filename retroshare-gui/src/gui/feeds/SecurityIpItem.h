/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015, RetroShare Team
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

#ifndef _SECURITYIPITEM_H
#define _SECURITYIPITEM_H

#include "retroshare/rstypes.h"

#include "FeedItem.h"
#include <stdint.h>

namespace Ui {
class SecurityIpItem;
} 

class FeedHolder;

class SecurityIpItem : public FeedItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	SecurityIpItem(FeedHolder *parent, const RsPeerId &sslId, const std::string& ipAddr, uint32_t result, uint32_t type, bool isTest);
	SecurityIpItem(FeedHolder *parent, const RsPeerId &sslId, const std::string& ipAddr, const std::string& ipAddrReported, uint32_t type, bool isTest);

	void updateItemStatic();

	bool isSame(const std::string& ipAddr, const std::string& ipAddrReported, uint32_t type);

	/* FeedItem */
	virtual void expand(bool open);

private:
	void setup();

private slots:
	/* default stuff */
	void removeItem();
	void toggle();
	void peerDetails();
	void updateItem();
	void banIpListChanged(const QString &ipAddress);

private:
	FeedHolder *mParent;
	uint32_t mFeedId;

	uint32_t mType;
	RsPeerId mSslId;
	std::string mIpAddr;
	std::string mIpAddrReported;
	uint32_t mResult;
	bool mIsTest;

	/** Qt Designer generated object */
	Ui::SecurityIpItem *ui;
};

#endif
