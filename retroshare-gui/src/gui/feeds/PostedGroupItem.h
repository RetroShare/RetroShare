/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#ifndef _POSTEDGROUPITEM_H
#define _POSTEDGROUPITEM_H

#include <retroshare/rsposted.h>
#include "gui/gxs/GxsGroupFeedItem.h"

namespace Ui {
class PostedGroupItem;
}
 
class FeedHolder;

class PostedGroupItem : public GxsGroupFeedItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	PostedGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate);
	PostedGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedGroup &group, bool isHome, bool autoUpdate);
	~PostedGroupItem();

	bool setGroup(const RsPostedGroup &group);

protected:
	/* FeedItem */
	virtual void doExpand(bool open);

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_UNKNOWN; }

private slots:
	/* default stuff */
	void toggle();

	void subscribePosted();

private:
	void fill();
	void setup();

private:
	RsPostedGroup mGroup;

	/** Qt Designer generated object */
	Ui::PostedGroupItem *ui;
};

#endif
