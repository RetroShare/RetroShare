/*******************************************************************************
 * gui/feeds/ChannelsCommentsItem.h                                                 *
 *                                                                             *
 * Copyright (c) 2020, Retroshare Team   <retroshare.project@gmail.com>        *
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

#ifndef _CHANNELS_COMMENTS_ITEM_H
#define _CHANNELS_COMMENTS_ITEM_H

#include <QMetaType>

#include <retroshare/rsgxschannels.h>
#include "gui/gxs/GxsFeedItem.h"

namespace Ui {
class ChannelsCommentsItem;
}

class FeedHolder;
class SubFileItem;

class ChannelsCommentsItem : public GxsFeedItem
{
	Q_OBJECT

public:
	// This one is used in NewFeed for incoming channel posts. Only the group and msg ids are known at this point.
	// It can be used for all apparences of channel posts. But in rder to merge comments from the previous versions of the post, the list of
	// previous posts should be supplied. It's optional. If not supplied only the comments of the new version will be displayed.

    ChannelsCommentsItem(FeedHolder *feedHolder,
                         uint32_t feedId,
                         const RsGxsGroupId& groupId,
                         const RsGxsMessageId& commentId,
                         const RsGxsMessageId& threadId,
                         bool isHome,
                         bool autoUpdate);

	// This one is used in channel thread widget. We don't want the group data to reload at every post, so we load it in the hosting
    // GxsChannelsPostsWidget and pass it to created items.
    // ChannelsCommentsItem(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData& group, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId>& older_versions = std::set<RsGxsMessageId>());

	virtual ~ChannelsCommentsItem();

    uint64_t uniqueIdentifier() const override { return hash_64bits("ChannelsCommentsItem " + messageId().toStdString()) ; }

	bool setGroup(const RsGxsChannelGroup& group, bool doFill = true);
	bool setPost(const RsGxsChannelPost& post, bool doFill = true);
    bool setMissingPost();

	QString getTitleLabel();
	QString getMsgLabel();

	bool isLoaded() const {return mLoaded;};
	bool isUnread() const ;
	void setReadStatus(bool isNew, bool isUnread);

	const std::set<RsGxsMessageId>& olderVersions() const { return mPost.mOlderVersions; }

	static uint64_t computeIdentifier(const RsGxsMessageId& msgid) { return hash64("ChannelsCommentsItem " + msgid.toStdString()) ; }
protected:
	//void init(const RsGxsMessageId& messageId,const std::set<RsGxsMessageId>& older_versions);

	/* FeedItem */
	virtual void doExpand(bool open);
	virtual void expandFill(bool first);

	// This does nothing except triggering the loading of the post data and comments. This function is mainly used to detect
	// when the post is actually made visible.

	virtual void paintEvent(QPaintEvent *) override;

	/* GxsGroupFeedItem */
	virtual QString groupName();
	virtual void loadGroup() override;
	virtual RetroShareLink::enumType getLinkType() { return RetroShareLink::TYPE_CHANNEL; }

	/* GxsFeedItem */
	virtual QString messageName();
    virtual void loadMessage() override {}
    virtual void loadComment() override {}

private slots:
	/* default stuff */
	void toggle() override;
	void readAndClearItem();

	void loadComments();
	void readToggled(bool checked);
	void unsubscribeChannel();

	void makeUpVote();
	void makeDownVote();

signals:
	void vote(const RsGxsGrpMsgIdPair& msgId, bool up);	

private:
    void load();
    void setup();
    void fill(bool missing_post=false);

private:
	bool mInFill;
	bool mCloseOnRead;
	bool mLoaded;

    bool mLoading;

	RsGroupMetaData mGroupMeta;
	RsGxsChannelPost mPost;
    RsGxsMessageId mThreadId;

	/** Qt Designer generated object */
	Ui::ChannelsCommentsItem *ui;
};

//Q_DECLARE_METATYPE(RsGxsChannelPost)

#endif
