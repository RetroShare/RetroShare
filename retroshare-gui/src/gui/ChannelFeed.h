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

#ifndef _CHANNEL_FEED_DIALOG_H
#define _CHANNEL_FEED_DIALOG_H

#include "mainpage.h"
#include "ui_ChannelFeed.h"

#include <QStandardItemModel>

#include "gui/feeds/FeedHolder.h"

#define OWN	0
#define SUBSCRIBED	1
#define POPULAR	2
#define OTHER	3

class ChanMsgItem;

class ChannelFeed : public MainPage, public FeedHolder, private Ui::ChannelFeed
{
  Q_OBJECT

public:
  /** Default Constructor */
  ChannelFeed(QWidget *parent = 0);
  /** Default Destructor */


virtual void deleteFeedItem(QWidget *item, uint32_t type);
virtual void openChat(std::string peerId);
virtual void openMsg(uint32_t type, std::string grpId, std::string inReplyTo);

public slots:

	void selectChannel( std::string );
	void selectChannel(const QModelIndex &);
	void toggleSelection(const QModelIndex &);

private slots:

  void channelListCustomPopupMenu( QPoint point );

	void checkUpdate();

	void createChannel();

	void channelSelection();

	void subscribeChannel();
	void unsubscribeChannel();
	
	void createMsg();

  void showChannelDetails();

private:

	void updateChannelList();
	void updateChannelListOwn(std::list<std::string> &ids);
	void updateChannelListSub(std::list<std::string> &ids);
	void updateChannelListPop(std::list<std::string> &ids);
	void updateChannelListOther(std::list<std::string> &ids);

	void updateChannelMsgs();

	QStandardItemModel *model;

	std::string mChannelId; /* current Channel */

	/* Layout Pointers */
	QBoxLayout *mMsgLayout;

	std::list<ChanMsgItem *> mChanMsgItems;

	QFont mChannelFont;
	QFont itemFont;
	
	QAction* postchannelAct;
	QAction* subscribechannelAct;
	QAction* unsubscribechannelAct;
	QAction* channeldetailsAct;

};



#endif

