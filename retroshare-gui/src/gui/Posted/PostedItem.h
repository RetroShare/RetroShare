/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedItem.h                                  *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef MRK_POSTED_POSTED_ITEM_H
#define MRK_POSTED_POSTED_ITEM_H

#include <QMetaType>

#include <retroshare/rsposted.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class PostedItem;
}

class FeedHolder;
class RsPostedPost;

class PostedItem : public GxsFeedItem
{
	Q_OBJECT

public:
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);

	virtual ~PostedItem();

	bool setPost(const RsPostedPost& post, bool doFill = true);

    const RsPostedPost& getPost() const { return mPost ; }
    RsPostedPost& getPost() { return mPost ; }

	uint64_t uniqueIdentifier() const override { return hash_64bits("PostedItem " + messageId().toStdString()); }
protected:
	/* FeedItem */
	virtual void doExpand(bool open);
    virtual void paintEvent(QPaintEvent *) override;

private slots:
	void loadComments();
	void makeUpVote();
	void makeDownVote();
	void readToggled(bool checked);
	void readAndClearItem();
	void toggle() override;
	void copyMessageLink();
	void toggleNotes();
	void viewPicture();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

protected:
	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup() override;
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_UNKNOWN; }

	/* GxsFeedItem */
	virtual QString messageName();

	virtual void loadMessage();
	virtual void loadComment();

private:
	void setup();
	void fill();
	void setReadStatus(bool isNew, bool isUnread);

private:
	bool mInFill;
    bool mLoaded;

	RsGroupMetaData mGroupMeta;
	RsPostedPost mPost;

	/** Qt Designer generated object */
	Ui::PostedItem *ui;
};

Q_DECLARE_METATYPE(RsPostedPost)

#endif
