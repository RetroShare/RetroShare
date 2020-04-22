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

class BasePostedItem : public GxsFeedItem
{
	Q_OBJECT

public:
	BasePostedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	BasePostedItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);
	virtual ~BasePostedItem()=default;

	bool setPost(const RsPostedPost& post, bool doFill = true);

    const RsPostedPost& getPost() const { return mPost ; }
    RsPostedPost& getPost() { return mPost ; }

	uint64_t uniqueIdentifier() const override { return hash_64bits("PostedItem " + messageId().toStdString()); }

private slots:
	void loadComments();
	void readToggled(bool checked);
	void readAndClearItem();
	void copyMessageLink();
	void viewPicture();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

protected:
	/* FeedItem */
    virtual void paintEvent(QPaintEvent *) override;

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup() override;
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_UNKNOWN; }

	/* GxsFeedItem */
	virtual QString messageName();

	virtual void loadMessage();
	virtual void loadComment();

	bool mInFill;
	RsGroupMetaData mGroupMeta;
	RsPostedPost mPost;

	virtual void setup()=0;
	virtual void fill()=0;
	virtual void doExpand(bool open)=0;
	virtual void setComment(const RsGxsComment&)=0;
	virtual void setReadStatus(bool isNew, bool isUnread)=0;
	virtual void setCommentsSize(int comNb)=0;
    virtual void makeUpVote()=0;
    virtual void makeDownVote()=0;
	virtual void toggleNotes()=0;

private:
    bool mLoaded;
};

class PostedItem: public BasePostedItem
{
public:
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	PostedItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);

protected:
	void setup() override;
	void fill() override;
	void doExpand(bool open) override;
	void setComment(const RsGxsComment&) override;
	void setReadStatus(bool isNew, bool isUnread) override;
    void toggle() override ;
	void setCommentsSize(int comNb) override;
    void makeUpVote() override;
    void makeDownVote() override;
	void toggleNotes() override;

private:
	/** Qt Designer generated object */
	Ui::PostedItem *ui;
};

Q_DECLARE_METATYPE(RsPostedPost)

#endif
