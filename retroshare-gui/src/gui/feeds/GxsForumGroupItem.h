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
#include <retroshare/rsevents.h>
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
	GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const std::list<RsGxsId>& added_moderators,const std::list<RsGxsId>& removed_moderators,bool isHome, bool autoUpdate);
	GxsForumGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumGroup &group, bool isHome, bool autoUpdate);
    virtual ~GxsForumGroupItem() override;

	bool setGroup(const RsGxsForumGroup &group);

    uint64_t uniqueIdentifier() const override { return hash_64bits("GxsForumGroupItem " + groupId().toStdString()) ; }
protected:
	/* FeedItem */
    virtual void doExpand(bool open) override;

	/* GxsGroupFeedItem */
    virtual QString groupName() override;
	virtual void loadGroup() override;
    virtual RetroShareLink::enumType getLinkType() override { return RetroShareLink::TYPE_FORUM; }

private slots:
	void subscribeForum();
    void toggle() override;

private:
	void fill();
	void setup();
    void addEventHandler();

private:
	RsGxsForumGroup mGroup;

	/** Qt Designer generated object */
	Ui::GxsForumGroupItem *ui;

    std::list<RsGxsId> mAddedModerators;
    std::list<RsGxsId> mRemovedModerators;

    RsEventsHandlerId_t mEventHandlerId;
};

#endif
