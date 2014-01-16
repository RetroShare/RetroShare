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
#include "ui_NewsFeed.h"

#include "gui/feeds/FeedHolder.h"
#include <retroshare-gui/RsAutoUpdatePage.h>
class RsFeedItem;

class ForumNewItem;
class ChanMsgItem;
class ChatMsgItem;
class FeedNotify;

class NewsFeed : public RsAutoUpdatePage, public FeedHolder, private Ui::NewsFeed
{
  Q_OBJECT

public:
  /** Default Constructor */
  NewsFeed(QWidget *parent = 0);
  /** Default Destructor */
  virtual ~NewsFeed();

  virtual UserNotify *getUserNotify(QObject *parent);

    /* FeedHolder Functions (for FeedItem functionality) */
  virtual QScrollArea *getScrollArea();
  virtual void deleteFeedItem(QWidget *item, uint32_t type);
  virtual void openChat(std::string peerId);
  virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title);

  static void testFeeds(uint notifyFlags);
  static void testFeed(FeedNotify *feedNotify);

  virtual void updateDisplay();
signals:
  void newsFeedChanged(int count);

private slots:
 // void toggleChanMsgItems(bool on);
  void feedoptions();
 
  void removeAll();
  void itemDestroyed(QObject*);

private:
  void  addFeedItem(QWidget *item);
  void  addFeedItemIfUnique(QWidget *item, int itemType, const std::string &sslId, bool replace);

  void	addFeedItemPeerConnect(RsFeedItem &fi);
  void	addFeedItemPeerDisconnect(RsFeedItem &fi);
  void	addFeedItemPeerNew(RsFeedItem &fi);
  void	addFeedItemPeerHello(RsFeedItem &fi);

  void  addFeedItemSecurityConnectAttempt(RsFeedItem &fi);
  void  addFeedItemSecurityAuthDenied(RsFeedItem &fi);
  void  addFeedItemSecurityUnknownIn(RsFeedItem &fi);
  void  addFeedItemSecurityUnknownOut(RsFeedItem &fi);

  void	addFeedItemChanNew(RsFeedItem &fi);
  void	addFeedItemChanUpdate(RsFeedItem &fi);
  void	addFeedItemChanMsg(RsFeedItem &fi);
  void	addFeedItemForumNew(RsFeedItem &fi);
  void	addFeedItemForumUpdate(RsFeedItem &fi);
  void	addFeedItemForumMsg(RsFeedItem &fi);
  void  addFeedItemBlogNew(RsFeedItem &fi);
  void	addFeedItemBlogMsg(RsFeedItem &fi);
  void	addFeedItemChatNew(RsFeedItem &fi, bool addWithoutCheck);
  void	addFeedItemMessage(RsFeedItem &fi);
  void	addFeedItemFilesNew(RsFeedItem &fi);

  void sendNewsFeedChanged();

  std::list<QObject*> widgets;

  /* lists of feedItems */
  std::list<ForumNewItem *> 	mForumNewItems;

  std::list<ChanMsgItem *> 	mChanMsgItems;
};

#endif
