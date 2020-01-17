/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedListWidget.h                            *
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

#ifndef POSTED_LIST_WIDGET_H
#define POSTED_LIST_WIDGET_H

#include <QMap>

#include "gui/gxs/GxsMessageFramePostWidget.h"
#include "gui/feeds/FeedHolder.h"

class RsPostedGroup;
class RsPostedPost;
class PostedItem;

namespace Ui {
class PostedListWidget;
}

class PostedListWidget : public GxsMessageFramePostWidget, public FeedHolder
{
	Q_OBJECT

public:
	PostedListWidget(const RsGxsGroupId &postedId, QWidget *parent = 0);
	~PostedListWidget();

	/* GxsMessageFrameWidget */
	virtual QIcon groupIcon();
	virtual void groupIdChanged();

	/* FeedHolder */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(FeedItem *item, uint32_t type);
	virtual void openChat(const RsPeerId& peerId);
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &versions, const RsGxsMessageId &msgId, const QString &title);

	/* GXS functions */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

protected:
	/* GxsMessageFramePostWidget */
	virtual bool insertGroupData(const uint32_t &token, RsGroupMetaData &metaData);
	virtual void insertAllPosts(const uint32_t &token, GxsMessageFramePostThread *thread);
	virtual void insertPosts(const uint32_t &token);
	virtual void clearPosts();
	virtual void blank();
	virtual bool navigatePostItem(const RsGxsMessageId& msgId);

	/* GxsMessageFrameWidget */
	virtual void setAllMessagesReadDo(bool read, uint32_t &token);

private slots:
	void newPost();

	void submitVote(const RsGxsGrpMsgIdPair& msgId, bool up);

	void getRankings(int);

	void subscribeGroup(bool subscribe);

	void showNext();
	void showPrev();

private:
	void processSettings(bool load);
	void updateShowText();

	/*!
	 * Only removes it from layout
	 */
	void shallowClearPosts();

	void loadPost(const RsPostedPost &post);

	void insertPostedDetails(const RsPostedGroup &group);

	// subscribe/unsubscribe ack.
//	void acknowledgeSubscribeChange(const uint32_t &token);

	// votes
	void acknowledgeVoteMsg(const uint32_t& token);
	void loadVoteData(const uint32_t &token);

	// ranking
	//void loadRankings(const uint32_t& token);
	//void applyRanking(const PostedRanking& ranks);
	void applyRanking();

private:
	int	mSortMethod;
	int	mLastSortMethod;
	int	mPostIndex;
	int	mPostShow;

	uint32_t mTokenTypeVote;

	QMap<RsGxsMessageId, PostedItem*> mPosts;
	QList<PostedItem*> mPostItems;

	/* UI - from Designer */
	Ui::PostedListWidget *ui;
};

#endif
