/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsWidget.h                  *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
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
	/* Filters */
	enum Filter {
		FILTER_TITLE =     1,
		FILTER_MSG =       2,
		FILTER_FILE_NAME = 3
	};

public:
	/** Default Constructor */
	GxsChannelPostsWidget(const RsGxsGroupId &channelId, QWidget *parent = 0);
	/** Default Destructor */
	~GxsChannelPostsWidget();

	/* GxsMessageFrameWidget */
	virtual QIcon groupIcon();

	/* FeedHolder */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(FeedItem *feedItem, uint32_t type);
	virtual void openChat(const RsPeerId& peerId);
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &msg_versions, const RsGxsMessageId &msgId, const QString &title);

protected:
	/* GxsMessageFramePostWidget */
	virtual void groupNameChanged(const QString &name);
	virtual bool insertGroupData(const RsGxsGenericGroupData *data) override;
#ifdef TO_REMOVE
	virtual void insertAllPosts(const uint32_t &token, GxsMessageFramePostThread *thread);
	virtual void insertPosts(const uint32_t &token);
#endif
	virtual void clearPosts();
	virtual bool useThread() { return mUseThread; }
	virtual void fillThreadCreatePost(const QVariant &post, bool related, int current, int count);
	virtual bool navigatePostItem(const RsGxsMessageId& msgId);
    virtual void blank() ;

	virtual bool getGroupData(RsGxsGenericGroupData *& data) override;
    virtual void getMsgData(const std::set<RsGxsMessageId>& msgIds,std::vector<RsGxsGenericMsgData*>& posts) override;
    virtual void getAllMsgData(std::vector<RsGxsGenericMsgData*>& posts) override;
	virtual void insertPosts(const std::vector<RsGxsGenericMsgData*>& posts) override;
	virtual void insertAllPosts(const std::vector<RsGxsGenericMsgData*>& posts, GxsMessageFramePostThread *thread) override;

	/* GxsMessageFrameWidget */
	virtual void setAllMessagesReadDo(bool read, uint32_t &token);

private slots:
	void createMsg();
	void toggleAutoDownload();
	void subscribeGroup(bool subscribe);
	void filterChanged(int filter);
	void setViewMode(int viewMode);
	void settingsChanged();

private:
	void processSettings(bool load);

	void setAutoDownload(bool autoDl);
	static bool filterItem(FeedItem *feedItem, const QString &text, int filter);

	int viewMode();

	void insertChannelDetails(const RsGxsChannelGroup &group);
	void insertChannelPosts(std::vector<RsGxsChannelPost> &posts, GxsMessageFramePostThread *thread, bool related);

	void createPostItem(const RsGxsChannelPost &post, bool related);
	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

private:
	QAction *mAutoDownloadAction;

	bool mUseThread;
    RsEventsHandlerId_t mEventHandlerId ;

	/* UI - from Designer */
	Ui::GxsChannelPostsWidget *ui;
};

#endif
