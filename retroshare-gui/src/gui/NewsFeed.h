/*******************************************************************************
 * gui/NewsFeed.h                                                              *
 *                                                                             *
 * Copyright (c) 2008 Robert Fernie    <retroshare.project@gmail.com>          *
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

#ifndef _NEWS_FEED_DIALOG_H
#define _NEWS_FEED_DIALOG_H

#include "mainpage.h"

#include "gui/feeds/FeedHolder.h"
#include "util/TokenQueue.h"
#include <retroshare-gui/RsAutoUpdatePage.h>

#define IMAGE_NEWSFEED ":/icons/plugins_128.png"

const uint32_t NEWSFEED_PEERLIST =       0x0001;

const uint32_t NEWSFEED_FORUMNEWLIST =   0x0002;
const uint32_t NEWSFEED_FORUMMSGLIST =   0x0003;
const uint32_t NEWSFEED_CHANNELNEWLIST = 0x0004;
//const uint32_t NEWSFEED_CHANNELMSGLIST = 0x0005;
#if 0
const uint32_t NEWSFEED_BLOGNEWLIST =    0x0006;
const uint32_t NEWSFEED_BLOGMSGLIST =    0x0007;
#endif

const uint32_t NEWSFEED_MESSAGELIST =      0x0008;
const uint32_t NEWSFEED_CHATMSGLIST =      0x0009;
const uint32_t NEWSFEED_SECLIST =          0x000a;
const uint32_t NEWSFEED_POSTEDNEWLIST =    0x000b;
const uint32_t NEWSFEED_POSTEDMSGLIST =    0x000c;
const uint32_t NEWSFEED_CIRCLELIST    =    0x000d;
const uint32_t NEWSFEED_CHANNELPUBKEYLIST= 0x000e;

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
//	virtual void openChat(const RsPeerId& peerId);
//	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &versions, const RsGxsMessageId &msgId, const QString &title);

	static void testFeeds(uint notifyFlags);
	static void testFeed(FeedNotify *feedNotify);

	virtual void updateDisplay();

	void handleEvent(std::shared_ptr<const RsEvent> event);	// get events from libretroshare

signals:
	void newsFeedChanged(int count);

protected:
	void processSettings(bool load);

	/* TokenResponse */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void feedoptions();
	void sortChanged(int index);

	void sendNewsFeedChanged();

private:
	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

	void handleSecurityEvent(std::shared_ptr<const RsEvent> event);
	void handleConnectionEvent(std::shared_ptr<const RsEvent> event);
	void handleCircleEvent(std::shared_ptr<const RsEvent> event);
	void handleForumEvent(std::shared_ptr<const RsEvent> event);
	void handlePostedEvent(std::shared_ptr<const RsEvent> event);
	void handleChannelEvent(std::shared_ptr<const RsEvent> event);

	void addFeedItem(FeedItem *item);
	void addFeedItemIfUnique(FeedItem *item, bool replace);
	void remUniqueFeedItem(FeedItem *item);
#if 0
	void addFeedItemBlogNew(const RsFeedItem &fi);
	void addFeedItemBlogMsg(const RsFeedItem &fi);
#endif

//	void addFeedItemChatNew(const RsFeedItem &fi, bool addWithoutCheck);
//	void addFeedItemMessage(const RsFeedItem &fi);
//	void addFeedItemFilesNew(const RsFeedItem &fi);

private:
	/* UI - from Designer */
	Ui::NewsFeed *ui;

    RsEventsHandlerId_t mEventHandlerId;
};

#endif
