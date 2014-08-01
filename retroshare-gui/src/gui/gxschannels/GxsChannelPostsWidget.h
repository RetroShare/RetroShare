/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _GXS_CHANNELPOSTSWIDGET_H
#define _GXS_CHANNELPOSTSWIDGET_H

#include <map>

#include "gui/gxs/GxsMessageFramePostWidget.h"

#include "gui/feeds/FeedHolder.h"

namespace Ui {
class GxsChannelPostsWidget;
}

class GxsChannelPostItem;
class QTreeWidgetItem;
class FeedItem;

class GxsChannelPostsWidget : public GxsMessageFramePostWidget, public FeedHolder
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsChannelPostsWidget(const RsGxsGroupId &channelId, QWidget *parent = 0);
	/** Default Destructor */
	~GxsChannelPostsWidget();

	/* GxsMessageFrameWidget */
	virtual QIcon groupIcon();
	virtual void setAllMessagesRead(bool read);

	/* FeedHolder */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(QWidget *item, uint32_t type);
	virtual void openChat(const RsPeerId& peerId);
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title);

protected:
	/* GxsMessageFramePostWidget */
	virtual void groupNameChanged(const QString &name);
	virtual bool insertGroupData(const uint32_t &token, RsGroupMetaData &metaData);
	virtual void insertPosts(const uint32_t &token, GxsMessageFramePostThread *thread);
	virtual void insertRelatedPosts(const uint32_t &token);
	virtual void clearPosts();
	virtual bool useThread() { return true; }
	virtual void fillThreadCreatePost(const QVariant &post, bool related, int current, int count);

private slots:
	void createMsg();
	void toggleAutoDownload();
	void subscribeGroup(bool subscribe);
	void filterChanged(int filter);

private:
	void processSettings(bool load);

	void setAutoDownload(bool autoDl);
	static bool filterItem(FeedItem *feedItem, const QString &text, int filter);

	void insertChannelDetails(const RsGxsChannelGroup &group);
	void insertChannelPosts(std::vector<RsGxsChannelPost> &posts, GxsMessageFramePostThread *thread, bool related);

	void createPostItem(const RsGxsChannelPost &post, bool related);

private:
	QAction *mAutoDownloadAction;

	bool mInProcessSettings;

	/* UI - from Designer */
	Ui::GxsChannelPostsWidget *ui;
};

#endif
