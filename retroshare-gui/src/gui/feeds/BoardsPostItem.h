/*******************************************************************************
 * retroshare-gui/src/gui/feeds/BoardsPostItem.h                               *
 *                                                                             *
 * Copyright (c) 2012, Robert Fernie   <retroshare.project@gmail.com>          *
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

#ifndef _BOARDS_POST_ITEM_H
#define _BOARDS_POST_ITEM_H

#include <QMetaType>

#include <retroshare/rsposted.h>
#include "gui/feeds/GxsFeedItem.h"

namespace Ui {
class BoardsPostItem;
}

class FeedHolder;
class SubFileItem;

class BoardsPostItem : public GxsFeedItem
{
	Q_OBJECT

public:
	// This one is used in NewFeed for incoming channel posts. Only the group and msg ids are known at this point.
	// It can be used for all apparences of channel posts. But in rder to merge comments from the previous versions of the post, the list of
	// previous posts should be supplied. It's optional. If not supplied only the comments of the new version will be displayed.

    BoardsPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());
    virtual ~BoardsPostItem();

    uint64_t uniqueIdentifier() const override { return hash_64bits("BoardsPostItem " + messageId().toStdString()) ; }

protected:

	bool isUnread() const ;
	void setReadStatus(bool isNew, bool isUnread);

    static uint64_t computeIdentifier(const RsGxsMessageId& msgid) { return hash64("BoardsPostItem " + msgid.toStdString()) ; }

	/* FeedItem */
    virtual void doExpand(bool open) override ;
    virtual void expandFill(bool first) override ;

	// This does nothing except triggering the loading of the post data and comments. This function is mainly used to detect
	// when the post is actually made visible.

	virtual void paintEvent(QPaintEvent *) override;

	/* GxsGroupFeedItem */
    virtual QString groupName() override;
	virtual void loadGroup() override;
    virtual RetroShareLink::enumType getLinkType()  override{ return RetroShareLink::TYPE_CHANNEL; }

	/* GxsFeedItem */
    virtual QString messageName() override;
    virtual void loadMessage() override;
    virtual void loadComment() override {}

private slots:
	/* default stuff */
    void toggle() override;
	void readAndClearItem();
	void readToggled(bool checked);
    void viewPicture();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

private:
	void setup();
	void fill();
    void fillExpandFrame();

private:
	bool mCloseOnRead;

    LoadingStatus mLoadingStatus;

    bool mLoadingMessage;
    bool mLoadingGroup;

	RsGroupMetaData mGroupMeta;
    RsPostedPost mPost;

	/** Qt Designer generated object */
    Ui::BoardsPostItem *ui;
};

#endif
