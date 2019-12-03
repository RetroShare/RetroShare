/*******************************************************************************
 * gui/feeds/GxsForumGroupItem.h                                               *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _GXSFORUMGROUPITEM_H
#define _GXSFORUMGROUPITEM_H

#include <retroshare/rsgxsforums.h>
#include "gui/gxs/GxsGroupFeedItem.h"

namespace Ui {
class GxsForumGroupItem;
}
 
class FeedHolder;

class GxsForumGroupItem : public GxsGroupFeedItem
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate);
	GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumGroup &group, bool isHome, bool autoUpdate);
	~GxsForumGroupItem();

	bool setGroup(const RsGxsForumGroup &group);

    virtual QString uniqueIdentifier() const override { return "GxsForumGroupItem " + QString::fromStdString(mGroup.mMeta.mGroupId.toStdString()) ; }
protected:
	/* FeedItem */
	virtual void doExpand(bool open);

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_FORUM; }

private slots:
	/* default stuff */
	void toggle();

	void subscribeForum();

private:
	void fill();
	void setup();

private:
	RsGxsForumGroup mGroup;

	/** Qt Designer generated object */
	Ui::GxsForumGroupItem *ui;
};

#endif
