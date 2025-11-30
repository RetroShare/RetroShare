/*******************************************************************************
 * gui/feeds/GxsForumMsgItem.h                                                 *
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

#ifndef _GXSFORUMMSGITEM_H
#define _GXSFORUMMSGITEM_H

#include <retroshare/rsgxsforums.h>
#include "gui/feeds/GxsFeedItem.h"

namespace Ui {
class GxsForumMsgItem;
}

class FeedHolder;

class GxsForumMsgItem : public GxsFeedItem
{
	Q_OBJECT

public:
	GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate);

	virtual ~GxsForumMsgItem();

    uint64_t uniqueIdentifier() const override { return hash_64bits("GxsForumMsgItem " + messageId().toStdString()) ; }
protected:
    virtual void paintEvent(QPaintEvent *e) override;
    /* FeedItem */
	virtual void doExpand(bool open) override;
	virtual void expandFill(bool first) override;

	/* GxsGroupFeedItem */
	virtual QString groupName() override;
	virtual void loadGroup() override;
	virtual RetroShareLink::enumType getLinkType() override { return RetroShareLink::TYPE_FORUM; }

	/* GxsFeedItem */
	virtual QString messageName() override;
	virtual void loadMessage() override;
	virtual void loadComment() override { return; }

private slots:
	/* default stuff */
	void toggle() override;
	void readAndClearItem();

	void unsubscribeForum();

	void on_linkActivated(QString link);

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);	

private:
	void setup();

    void fillGroup();
    void fillMessage();
    void fillParentMessage();
    void fillExpandFrame();
	void setReadStatus(bool isNew, bool isUnread);
	void setAsRead(bool doUpdate);

private:
	bool mCloseOnRead;

    LoadingStatus mLoadingStatus;

    bool mLoadingMessage;
    bool mLoadingGroup;
    bool mLoadingSetAsRead;

    bool mHasParentMessage;	// set to false when we see that the msg does not have a parent.

	RsGxsForumGroup mGroup;
    RsGxsForumMsg   mMessage;
    RsGxsForumMsg   mParentMessage;

	/** Qt Designer generated object */
	Ui::GxsForumMsgItem *ui;
};

#endif
