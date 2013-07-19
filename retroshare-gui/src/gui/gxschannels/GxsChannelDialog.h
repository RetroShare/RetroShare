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

#ifndef _GXS_CHANNEL_DIALOG_H
#define _GXS_CHANNEL_DIALOG_H

#include <map>

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

#include "ui_GxsChannelDialog.h"

#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/GxsCommentContainer.h"

#include "util/TokenQueue.h"

//class ChanMsgItem;
class GxsChannelPostItem;
class QTreeWidgetItem;
class UIStateHelper;

class GxsChannelDialog : public RsGxsUpdateBroadcastPage, public TokenResponse, public GxsServiceDialog, public FeedHolder
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsChannelDialog(QWidget *parent = 0);
	/** Default Destructor */
	~GxsChannelDialog();

//	virtual UserNotify *getUserNotify(QObject *parent);

	/* FeedHolder */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(QWidget *item, uint32_t type);
	virtual void openChat(std::string peerId);
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title);

	bool navigate(const std::string& channelId, const std::string& msgId);

	/* NEW GXS FNS */
	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

protected:
	virtual void updateDisplay(bool complete);

private slots:
	void todo();
	void channelListCustomPopupMenu(QPoint point);
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

//	void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);

	//void generateMassData();

	//void fillThreadFinished();
	//void fillThreadAddMsg(const QString &channelId, const QString &channelMsgId, int current, int count);

private:
	//void updateChannelList();
	//void updateChannelMsgs();
	void updateMessageSummaryList(const std::string &channelId);

	void processSettings(bool load);

	void setAutoDownloadButton(bool autoDl);

	/* NEW GXS FNS */
	void insertChannels();

	void requestGroupSummary();
	void loadGroupSummary(const uint32_t &token);

	void requestGroupData(const RsGxsGroupId &grpId);
	void loadGroupData(const uint32_t &token);

	void requestPosts(const RsGxsGroupId &grpId);
	void loadPosts(const uint32_t &token);

	void insertChannelData(const std::list<RsGroupMetaData> &channelList);

	void insertChannelDetails(const RsGxsChannelGroup &group);
	void insertChannelPosts(const std::vector<RsGxsChannelPost> &posts);

	void acknowledgeGroupUpdate(const uint32_t &token);
	void acknowledgeMessageUpdate(const uint32_t &token);

	std::string mChannelId; /* current Channel */
	TokenQueue *mChannelQueue;

	/* Layout Pointers */
	QBoxLayout *mMsgLayout;

	//QList<ChanMsgItem *> mChanMsgItems;
	QList<GxsChannelPostItem *> mChannelPostItems;

	std::map<std::string, uint32_t> mChanSearchScore; //chanId, score

	QTreeWidgetItem *ownChannels;
	QTreeWidgetItem *subcribedChannels;
	QTreeWidgetItem *popularChannels;
	QTreeWidgetItem *otherChannels;

	UIStateHelper *mStateHelper;

	/* UI - from Designer */
	Ui::GxsChannelDialog ui;
};

#endif
