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

#include <retroshare/rschannels.h>
#include <QStandardItemModel>
#include <map>

#include "mainpage.h"
#include "RsAutoUpdatePage.h"

#include "ui_ChannelFeed.h"

#include "gui/feeds/FeedHolder.h"



#define OWN	0
#define SUBSCRIBED	1
#define POPULAR	2
#define OTHER	3

class ChanMsgItem;
class QStandardItemModel;
class QStandardItem;

class ChannelFeed : public RsAutoUpdatePage, public FeedHolder, private Ui::ChannelFeed
{
    Q_OBJECT

public:
    /** Default Constructor */
    ChannelFeed(QWidget *parent = 0);
    /** Default Destructor */


    virtual void deleteFeedItem(QWidget *item, uint32_t type);
    virtual void openChat(std::string peerId);

    /* overloaded from RsAuthUpdatePage */ 
    virtual void updateDisplay();

public slots:
    void selectChannel(QModelIndex index);

private slots:
    void channelListCustomPopupMenu( QPoint point );

    void createChannel();

    void channelSelection();

    void subscribeChannel();
    void unsubscribeChannel();
    void setAllAsReadClicked();

    void createMsg();

    void showChannelDetails();
    void restoreChannelKeys();
    void editChannelDetail();
    void shareKey();

    void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);
    void filterRegExpChanged();
    void finishSearching();

private:

    void updateChannelList();
    void fillChannelList(int channelItem, std::list<ChannelInfo> &channelInfos);
    void filterChannelList(std::list<ChannelInfo>&);
    void updateChannelMsgs();
    void updateMessageSummaryList(const std::string &channelId);

    QStandardItemModel *model;

    std::string mChannelId; /* current Channel */

    /* Layout Pointers */
    QBoxLayout *mMsgLayout;

    std::list<ChanMsgItem *> mChanMsgItems;
    std::map<std::string, uint32_t> mChanSearchScore; //chanId, score

    QFont mChannelFont;
    QFont itemFont;
};

class QChannelItem : public QStandardItem
{

public:

	QChannelItem();
	virtual ~QChannelItem();

	/**
	 * reimplementing comparison operator so QChannelItems can be ordered in terms
	 * of occurences of property searchText in its data columns
	 */
	bool operator<(const QStandardItem& other) const;

};


#endif

