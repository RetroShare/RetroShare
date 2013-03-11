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
#include <QThread>
#include <map>

#include "mainpage.h"
#include "RsAutoUpdatePage.h"

#include "ui_ChannelFeed.h"

#include "gui/feeds/FeedHolder.h"

class ChanMsgItem;
class QTreeWidgetItem;
class ChannelFillThread;

class ChannelFeed : public RsAutoUpdatePage, public FeedHolder, private Ui::ChannelFeed
{
    Q_OBJECT

public:
    /** Default Constructor */
    ChannelFeed(QWidget *parent = 0);
    /** Default Destructor */
    ~ChannelFeed();

    virtual UserNotify *getUserNotify(QObject *parent);

    /* FeedHolder */
    virtual QScrollArea *getScrollArea();
    virtual void deleteFeedItem(QWidget *item, uint32_t type);
    virtual void openChat(std::string peerId);
    virtual void openComments(uint32_t /*type*/ , const RsGxsGroupId & /*grpId */, const RsGxsMessageId & /*msgId*/);

    bool navigate(const std::string& channelId, const std::string& msgId);

    /* overloaded from RsAuthUpdatePage */ 
    virtual void updateDisplay();

private slots:
    void channelListCustomPopupMenu( QPoint point );
    void selectChannel(const QString &id);

    void createChannel();

    void subscribeChannel();
    void unsubscribeChannel();
    void setAllAsReadClicked();
    void toggleAutoDownload();

    void createMsg();

    void showChannelDetails();
    void restoreChannelKeys();
    void editChannelDetail();
    void shareKey();
    void copyChannelLink();

    void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);

    void generateMassData();

    void fillThreadFinished();
    void fillThreadAddMsg(const QString &channelId, const QString &channelMsgId, int current, int count);

private:
    void updateChannelList();
    void updateChannelMsgs();
    void updateMessageSummaryList(const std::string &channelId);

    void processSettings(bool load);

    void setAutoDownloadButton(bool autoDl);

    std::string mChannelId; /* current Channel */

    /* Layout Pointers */
    QBoxLayout *mMsgLayout;

    QList<ChanMsgItem *> mChanMsgItems;
    std::map<std::string, uint32_t> mChanSearchScore; //chanId, score

	QTreeWidgetItem *ownChannels;
	QTreeWidgetItem *subcribedChannels;
	QTreeWidgetItem *popularChannels;
	QTreeWidgetItem *otherChannels;

	ChannelFillThread *fillThread;
};

class ChannelFillThread : public QThread
{
    Q_OBJECT

public:
    ChannelFillThread(ChannelFeed *parent, const std::string &channelId);
    ~ChannelFillThread();

    void run();
    void stop();
    bool wasStopped() { return stopped; }

signals:
    void addMsg(const QString &channelId, const QString &channelMsgId, int current, int count);

public:
    std::string channelId;

private:
    volatile bool stopped;
};

#endif
