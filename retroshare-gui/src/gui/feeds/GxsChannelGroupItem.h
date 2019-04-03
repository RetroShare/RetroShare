/*******************************************************************************
 * gui/feeds/GxsChannelGroupItem.h                                             *
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

#ifndef _GXSCHANNELGROUPITEM_H
#define _GXSCHANNELGROUPITEM_H

#include <retroshare/rsgxschannels.h>
#include "gui/gxs/GxsGroupFeedItem.h"

namespace Ui {
class GxsChannelGroupItem;
}
 
class FeedHolder;

class GxsChannelGroupItem : public GxsGroupFeedItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsChannelGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate);
	GxsChannelGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelGroup &group, bool isHome, bool autoUpdate);
	~GxsChannelGroupItem();

	bool setGroup(const RsGxsChannelGroup &group);

protected:
	/* FeedItem */
	virtual void doExpand(bool open);

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_CHANNEL; }

private slots:
	/* default stuff */
	void toggle();

	void subscribeChannel();

private:
	void fill();
	void setup();

private:
	RsGxsChannelGroup mGroup;

	/** Qt Designer generated object */
	Ui::GxsChannelGroupItem *ui;
};

#endif
