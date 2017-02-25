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

#ifndef _NEWS_FEED_DIALOG_H
#define _NEWS_FEED_DIALOG_H

#include "mainpage.h"

#include "gui/feeds/FeedHolder.h"
#include "util/TokenQueue.h"
#include <retroshare-gui/RsAutoUpdatePage.h>

#define IMAGE_NEWSFEED ":/icons/plugins_128.png"

namespace Ui {
class NewsFeed;
}

class RsFeedItem;
class FeedNotify;
class FeedItem;

class NewsFeed : public RsAutoUpdatePage, public FeedHolder, public TokenResponse
{
	Q_OBJECT

public:
	/** Default Constructor */
	NewsFeed(QWidget *parent = 0);
	/** Default Destructor */
	virtual ~NewsFeed();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_NEWSFEED) ; } //MainPage
	virtual QString pageName() const { return tr("Log") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	virtual UserNotify *getUserNotify(QObject *parent);

	/* FeedHolder Functions (for FeedItem functionality) */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(QWidget *item, uint32_t type);
	virtual void openChat(const RsPeerId& peerId);
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title);

	static void testFeeds(uint notifyFlags);
	static void testFeed(FeedNotify *feedNotify);

	virtual void updateDisplay();

signals:
	void newsFeedChanged(int count);

protected:
	void processSettings(bool load);

	/* TokenResponse */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
//	void toggleChanMsgItems(bool on);
	void feedoptions();
	void sortChanged(int index);

	void sendNewsFeedChanged();

private:
	void addFeedItem(FeedItem *item);
	void addFeedItemIfUnique(FeedItem *item, int itemType, const RsPeerId &sslId, const std::string& ipAddr, const std::string& ipAddrReported, bool replace);

	void addFeedItemPeerConnect(const RsFeedItem &fi);
	void addFeedItemPeerDisconnect(const RsFeedItem &fi);
	void addFeedItemPeerNew(const RsFeedItem &fi);
	void addFeedItemPeerHello(const RsFeedItem &fi);

	void addFeedItemSecurityConnectAttempt(const RsFeedItem &fi);
	void addFeedItemSecurityAuthDenied(const RsFeedItem &fi);
	void addFeedItemSecurityUnknownIn(const RsFeedItem &fi);
	void addFeedItemSecurityUnknownOut(const RsFeedItem &fi);
	void addFeedItemSecurityIpBlacklisted(const RsFeedItem &fi, bool isTest);
	void addFeedItemSecurityWrongExternalIpReported(const RsFeedItem &fi, bool isTest);

	void addFeedItemChannelNew(const RsFeedItem &fi);
//	void addFeedItemChannelUpdate(const RsFeedItem &fi);
	void addFeedItemChannelMsg(const RsFeedItem &fi);

	void addFeedItemForumNew(const RsFeedItem &fi);
//	void addFeedItemForumUpdate(const RsFeedItem &fi);
	void addFeedItemForumMsg(const RsFeedItem &fi);

	void addFeedItemPostedNew(const RsFeedItem &fi);
//	void addFeedItemPostedUpdate(const RsFeedItem &fi);
	void addFeedItemPostedMsg(const RsFeedItem &fi);

#if 0
	void addFeedItemBlogNew(const RsFeedItem &fi);
	void addFeedItemBlogMsg(const RsFeedItem &fi);
#endif

	void addFeedItemChatNew(const RsFeedItem &fi, bool addWithoutCheck);
	void addFeedItemMessage(const RsFeedItem &fi);
	void addFeedItemFilesNew(const RsFeedItem &fi);

	virtual void loadChannelGroup(const uint32_t &token);
	virtual void loadChannelPost(const uint32_t &token);
	virtual void loadChannelPublishKey(const uint32_t &token);

	virtual void loadForumGroup(const uint32_t &token);
	virtual void loadForumMessage(const uint32_t &token);
	virtual void loadForumPublishKey(const uint32_t &token);

	virtual void loadPostedGroup(const uint32_t &token);
	virtual void loadPostedMessage(const uint32_t &token);

private:
	TokenQueue *mTokenQueueChannel;
	TokenQueue *mTokenQueueForum;
	TokenQueue *mTokenQueuePosted;

	/* UI - from Designer */
	Ui::NewsFeed *ui;
};

#endif
