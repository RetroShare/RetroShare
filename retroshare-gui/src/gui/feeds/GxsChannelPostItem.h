/*******************************************************************************
 * gui/feeds/GxsChannelPostItem.h                                              *
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

#ifndef _GXS_CHANNEL_POST_ITEM_H
#define _GXS_CHANNEL_POST_ITEM_H

#include <QMetaType>

#include <retroshare/rsgxschannels.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class GxsChannelPostItem;
}

class FeedHolder;
class SubFileItem;

class GxsChannelPostItem : public GxsFeedItem
{
	Q_OBJECT

public:
	// This one is used in NewFeed for incoming channel posts. Only the group and msg ids are known at this point.
	// It can be used for all apparences of channel posts. But in rder to merge comments from the previous versions of the post, the list of
	// previous posts should be supplied. It's optional. If not supplied only the comments of the new version will be displayed.

	GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());

	// This method can be called when additional information is known about the post. In this case, the widget will be initialized with some
	// minimap information from the post and completed when the use displays it, which shouldn't cost anything more.

	GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelPost& post, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());

	//GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelGroup &group, const RsGxsChannelPost &post, bool isHome, bool autoUpdate);
	//GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelPost &post, bool isHome, bool autoUpdate);
	virtual ~GxsChannelPostItem();

    uint64_t uniqueIdentifier() const override { hash_64bits("GxsChannelPostItem " + mPost.mMeta.mMsgId.toStdString()) ; }

	bool setGroup(const RsGxsChannelGroup &group, bool doFill = true);
	bool setPost(const RsGxsChannelPost &post, bool doFill = true);

	void setFileCleanUpWarning(uint32_t time_left);

	QString getTitleLabel();
	QString getMsgLabel();
	const std::list<SubFileItem *> &getFileItems() {return mFileItems; }

    bool isUnread() const ;

protected:
	void init(const RsGxsMessageId& messageId,const std::set<RsGxsMessageId>& older_versions);

	/* FeedItem */
	virtual void doExpand(bool open);
	virtual void expandFill(bool first);

	// This does nothing except triggering the loading of the post data and comments. This function is mainly used to detect
	// when the post is actually made visible.

	virtual void paintEvent(QPaintEvent *);

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup(const uint32_t &token);
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_CHANNEL; }

	/* GxsFeedItem */
	virtual QString messageName();
	virtual void loadMessage(const uint32_t &token);
	virtual void loadComment(const uint32_t &token);

private slots:
	/* default stuff */
	void toggle();
	void readAndClearItem();
	void download();
	void play();
	void edit();
	void loadComments();

	void readToggled(bool checked);

	void unsubscribeChannel();
	void updateItem();

	void makeUpVote();
	void makeDownVote();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);	

private:
	void setup();
	void fill();
	void fillExpandFrame();
	void setReadStatus(bool isNew, bool isUnread);

private:
	bool mInFill;
	bool mCloseOnRead;
	bool mLoaded;

	RsGxsChannelGroup mGroup;
	RsGxsChannelPost mPost;

	std::list<SubFileItem*> mFileItems;

	/** Qt Designer generated object */
	Ui::GxsChannelPostItem *ui;
};

Q_DECLARE_METATYPE(RsGxsChannelPost)

#endif
