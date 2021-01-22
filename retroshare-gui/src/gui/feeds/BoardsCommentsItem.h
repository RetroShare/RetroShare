/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardsCommentsItem.h                          *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team     <retroshare.project@gmail.com>    *
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

#ifndef MRK_BOARDSCOMMENTS_ITEM_H
#define MRK_BOARDSCOMMENTS_ITEM_H

#include <QMetaType>

#include <retroshare/rsposted.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class BoardsCommentsItem;
}

class FeedHolder;
struct RsPostedPost;

class BaseBoardsCommentsItem : public GxsFeedItem
{
	Q_OBJECT

public:
	BaseBoardsCommentsItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	BaseBoardsCommentsItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);
	virtual ~BaseBoardsCommentsItem();

	bool setPost(const RsPostedPost& post, bool doFill = true);

    const RsPostedPost& getPost() const { return mPost ; }
    RsPostedPost& getPost() { return mPost ; }

	uint64_t uniqueIdentifier() const override { return hash_64bits("BoardsCommentsItem " + messageId().toStdString()); }

private slots:
	void loadComments();
	void readToggled(bool checked);
	void readAndClearItem();
	void copyMessageLink();
	void showAuthorInPeople();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);

protected:
	/* FeedItem */
	virtual void paintEvent(QPaintEvent *) override;

	/* GxsGroupFeedItem */
	virtual QString groupName() override;
	virtual void loadGroup() override;
	virtual RetroShareLink::enumType getLinkType() override { return RetroShareLink::TYPE_UNKNOWN; }

	/* GxsFeedItem */
	virtual QString messageName() override;

	virtual void loadMessage() override;
	virtual void loadComment() override;

	bool mInFill;
	RsGroupMetaData mGroupMeta;
	RsPostedPost mPost;

	virtual void setup()=0;
	virtual void fill()=0;
	virtual void doExpand(bool open) override =0;
	virtual void setComment(const RsGxsComment&)=0;
	virtual void setReadStatus(bool isNew, bool isUnread)=0;
	virtual void setCommentsSize(int comNb)=0;
	virtual void makeUpVote()=0;
	virtual void makeDownVote()=0;

private:
	bool mLoaded;
	bool mIsLoadingGroup;
	bool mIsLoadingMessage;
	bool mIsLoadingComment;
};

class BoardsCommentsItem: public BaseBoardsCommentsItem
{
	Q_OBJECT

public:
	BoardsCommentsItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	BoardsCommentsItem(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);

protected:
	void setup() override;
	void fill() override;
	void setComment(const RsGxsComment&) override;
	void setReadStatus(bool isNew, bool isUnread) override;
	void setCommentsSize(int comNb) override;

private slots:
	void doExpand(bool open);
	void toggle();
	void makeUpVote();
	void makeDownVote();

private:
	/** Qt Designer generated object */
	Ui::BoardsCommentsItem *ui;
};

//Q_DECLARE_METATYPE(RsPostedPost)

#endif
