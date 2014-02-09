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

#include <QMenu>
#include <QMessageBox>

#include <iostream>
#include <algorithm>

#include "GxsChannelDialog.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxschannels/GxsChannelGroupDialog.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define WARNING_LIMIT 3600*24*2

/* Images for TreeWidget */
#define IMAGE_CHANNELBLUE     ":/images/channelsblue.png"
#define IMAGE_CHANNELGREEN    ":/images/channelsgreen.png"
#define IMAGE_CHANNELRED      ":/images/channelsred.png"
#define IMAGE_CHANNELYELLOW   ":/images/channelsyellow.png"

/****
 * #define DEBUG_CHANNEL
 ***/

#define USE_THREAD

#define TOKEN_TYPE_GROUP_CHANGE     1 // THIS MUST MIRROR GxsGroupDialog parameters.

#define TOKEN_TYPE_MESSAGE_CHANGE   4
#define TOKEN_TYPE_LISTING          5
#define TOKEN_TYPE_GROUP_DATA       6
#define TOKEN_TYPE_POSTS            7

/** Constructor */
GxsChannelDialog::GxsChannelDialog(QWidget *parent)
	: RsGxsUpdateBroadcastPage(rsGxsChannels, parent), GxsServiceDialog(dynamic_cast<GxsCommentContainer *>(parent))
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui.progressBar, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui.progressLabel, UISTATE_LOADING_VISIBLE);

	mStateHelper->addLoadPlaceholder(TOKEN_TYPE_GROUP_DATA, ui.nameLabel);

	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui.postButton);
	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui.logoLabel);

	mChannelQueue = new TokenQueue(rsGxsChannels->getTokenService(), this);

	connect(ui.postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
//	connect(NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	connect(ui.treeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(channelListCustomPopupMenu(QPoint)));
	connect(ui.treeWidget, SIGNAL(treeCurrentItemChanged(QString)), this, SLOT(selectChannel(QString)));
	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	mChannelId.clear();

	/* Set initial size the splitter */
	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui.splitter->setSizes(sizes);

	/* Initialize group tree */
	QToolButton *newChannelButton = new QToolButton(this);
	newChannelButton->setIcon(QIcon(":/images/add_channel24.png"));
	newChannelButton->setToolTip(tr("Create Channel"));
	connect(newChannelButton, SIGNAL(clicked()), this, SLOT(createChannel()));
	ui.treeWidget->addToolButton(newChannelButton);

	ownChannels = ui.treeWidget->addCategoryItem(tr("My Channels"), QIcon(IMAGE_CHANNELBLUE), true);
	subcribedChannels = ui.treeWidget->addCategoryItem(tr("Subscribed Channels"), QIcon(IMAGE_CHANNELRED), true);
	popularChannels = ui.treeWidget->addCategoryItem(tr("Popular Channels"), QIcon(IMAGE_CHANNELGREEN), false);
	otherChannels = ui.treeWidget->addCategoryItem(tr("Other Channels"), QIcon(IMAGE_CHANNELYELLOW), false);

	ui.progressLabel->hide();
	ui.progressBar->hide();

	ui.nameLabel->setMinimumWidth(20);

	/* load settings */
	processSettings(true);

	/* Initialize empty GUI */
	requestGroupData(mChannelId);
}

GxsChannelDialog::~GxsChannelDialog()
{
	// save settings
	processSettings(false);
}

void GxsChannelDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
							 "<li>Threaded load of messages"
							 "<li>Share key"
							 "<li>Restore channel keys"
							 "<li>Copy/navigate channel link"
							 "<li>Display count of unread messages"
							 "<li>Show/Edit channel details"
							 "<li>Set all as read"
							 "<li>Set read/unread status"
							 "</ul>");
}

void GxsChannelDialog::updateDisplay(bool complete)
{
	std::list<RsGxsGroupId> &grpIds = getGrpIds();
	if (complete || !grpIds.empty()) {
		/* Update channel list */
		insertChannels();
	}
	if (!mChannelId.empty() && std::find(grpIds.begin(), grpIds.end(), mChannelId) != grpIds.end()) {
		requestGroupData(mChannelId);
	}

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgs = getMsgIds();
	if (!msgs.empty())
	{
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit = msgs.find(mChannelId);
		if(mit != msgs.end())
		{
			requestPosts(mChannelId);
		}
	}
}

// Callback from Widget->FeedHolder->ServiceDialog->CommentContainer->CommentDialog,
void GxsChannelDialog::openComments(uint32_t /*type*/, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title)
{
	commentLoad(groupId, msgId, title);
}

//UserNotify *GxsChannelDialog::getUserNotify(QObject *parent)
//{
//	return new ChannelUserNotify(parent);
//}

void GxsChannelDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("GxsChannelDialog"));

	if (load) {
		// load settings

		// state of splitter
		ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui.splitter->saveState());
	}

	ui.treeWidget->processSettings(Settings, load);

	Settings->endGroup();
}

void GxsChannelDialog::channelListCustomPopupMenu( QPoint /*point*/ )
{
	if (mChannelId.empty()) 
	{
		return;
	}

	uint32_t subscribeFlags = ui.treeWidget->subscribeFlags(QString::fromStdString(mChannelId));

	QMenu contextMnu(this);

	bool isAdmin = IS_GROUP_ADMIN(subscribeFlags);
	bool isPublisher = IS_GROUP_PUBLISHER(subscribeFlags);
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);
	bool autoDownload = rsGxsChannels->getChannelAutoDownload(mChannelId);

	if (isPublisher)
	{
		QAction *postchannelAct = new QAction(QIcon(":/images/mail_reply.png"), tr( "Post to Channel" ), &contextMnu);
		connect( postchannelAct , SIGNAL( triggered() ), this, SLOT( createMsg() ) );
	  	contextMnu.addAction( postchannelAct );
		contextMnu.addSeparator();
	}

	if (isSubscribed)
	{
		QAction *setallasreadchannelAct = new QAction(QIcon(":/images/message-mail-read.png"), tr( "Set all as read" ), &contextMnu);
		connect( setallasreadchannelAct , SIGNAL( triggered() ), this, SLOT( setAllAsReadClicked() ) );
		contextMnu.addAction( setallasreadchannelAct );

		contextMnu.addSeparator();

		QAction *autoAct = new QAction(QIcon(":/images/redled.png"), tr( "Disable Auto-Download" ), &contextMnu);
		QAction *noautoAct = new QAction(QIcon(":/images/start.png"),tr( "Enable Auto-Download" ), &contextMnu);
		connect( autoAct , SIGNAL( triggered() ), this, SLOT( toggleAutoDownload() ) );
		connect( noautoAct , SIGNAL( triggered() ), this, SLOT( toggleAutoDownload() ) );

		contextMnu.addAction( autoAct );
		contextMnu.addAction( noautoAct );

		autoAct->setEnabled(autoDownload);
		noautoAct->setEnabled(!autoDownload);

		QAction *unsubscribechannelAct = new QAction(QIcon(":/images/cancel.png"), tr( "Unsubscribe to Channel" ), &contextMnu);
		connect( unsubscribechannelAct , SIGNAL( triggered() ), this, SLOT( unsubscribeChannel() ) );
		contextMnu.addAction( unsubscribechannelAct );
	}
	else
	{
		QAction *subscribechannelAct = new QAction(QIcon(":/images/edit_add24.png"), tr( "Subscribe to Channel" ), &contextMnu);
		connect( subscribechannelAct , SIGNAL( triggered() ), this, SLOT( subscribeChannel() ) );
		contextMnu.addAction( subscribechannelAct );
	}

	QAction *channeldetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Channel Details" ), &contextMnu);
	connect( channeldetailsAct , SIGNAL( triggered() ), this, SLOT( showChannelDetails() ) );
	contextMnu.addAction( channeldetailsAct );

	if (isAdmin)
	{
		QAction *editChannelDetailAct = new QAction(QIcon(":/images/edit_16.png"), tr("Edit Channel Details"), &contextMnu);
		connect( editChannelDetailAct, SIGNAL( triggered() ), this, SLOT( editChannelDetail() ) );
		contextMnu.addAction( editChannelDetailAct);
	}

	if (isPublisher)
	{
		QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Channel" ), &contextMnu);
		connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreChannelKeys() ) );
		contextMnu.addAction( restoreKeysAct );
	}
	else
	{
		QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Channel"), &contextMnu);
		connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
		contextMnu.addAction( shareKeyAct );
	}

	contextMnu.addSeparator();
	QAction *action = contextMnu.addAction(QIcon(":/images/copyrslink.png"), tr("Copy RetroShare Link"), this, SLOT(copyChannelLink()));
	action->setEnabled(!mChannelId.empty());

	contextMnu.exec(QCursor::pos());

#if 0
	ChannelInfo ci;
	if (!rsChannels->getChannelInfo(mChannelId, ci)) {
		return;
	}

	QMenu contextMnu(this);

	QAction *postchannelAct = new QAction(QIcon(":/images/mail_reply.png"), tr( "Post to Channel" ), &contextMnu);
	connect( postchannelAct , SIGNAL( triggered() ), this, SLOT( createMsg() ) );

	QAction *subscribechannelAct = new QAction(QIcon(":/images/edit_add24.png"), tr( "Subscribe to Channel" ), &contextMnu);
	connect( subscribechannelAct , SIGNAL( triggered() ), this, SLOT( subscribeChannel() ) );

	QAction *unsubscribechannelAct = new QAction(QIcon(":/images/cancel.png"), tr( "Unsubscribe to Channel" ), &contextMnu);
	connect( unsubscribechannelAct , SIGNAL( triggered() ), this, SLOT( unsubscribeChannel() ) );

	QAction *setallasreadchannelAct = new QAction(QIcon(":/images/message-mail-read.png"), tr( "Set all as read" ), &contextMnu);
	connect( setallasreadchannelAct , SIGNAL( triggered() ), this, SLOT( setAllAsReadClicked() ) );

	bool autoDl = false;
	rsChannels->channelGetAutoDl(mChannelId, autoDl);

	QAction *autochannelAct = autoDl? (new QAction(QIcon(":/images/redled.png"), tr( "Disable Auto-Download" ), &contextMnu))
			 									: (new QAction(QIcon(":/images/start.png"),tr( "Enable Auto-Download" ), &contextMnu)) ;

	connect( autochannelAct , SIGNAL( triggered() ), this, SLOT( toggleAutoDownload() ) );

	QAction *channeldetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Channel Details" ), &contextMnu);
	connect( channeldetailsAct , SIGNAL( triggered() ), this, SLOT( showChannelDetails() ) );

	QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Channel" ), &contextMnu);
	connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreChannelKeys() ) );

	QAction *editChannelDetailAct = new QAction(QIcon(":/images/edit_16.png"), tr("Edit Channel Details"), &contextMnu);
	connect( editChannelDetailAct, SIGNAL( triggered() ), this, SLOT( editChannelDetail() ) );

	QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Channel"), &contextMnu);
	connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );

	if((ci.channelFlags & RS_DISTRIB_ADMIN) && (ci.channelFlags & RS_DISTRIB_SUBSCRIBED))
		contextMnu.addAction( editChannelDetailAct);
	else
		contextMnu.addAction( channeldetailsAct );

	if((ci.channelFlags & RS_DISTRIB_PUBLISH) && (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)) 
	{
		contextMnu.addAction( postchannelAct );
		contextMnu.addAction( shareKeyAct );
	}

	if(ci.channelFlags & RS_DISTRIB_SUBSCRIBED)
	{
		contextMnu.addAction( unsubscribechannelAct );
		contextMnu.addAction( restoreKeysAct );
		contextMnu.addSeparator();
		contextMnu.addAction( autochannelAct );
		contextMnu.addAction( setallasreadchannelAct );
	}
	else
		contextMnu.addAction( subscribechannelAct );

	contextMnu.addSeparator();
	QAction *action = contextMnu.addAction(QIcon(":/images/copyrslink.png"), tr("Copy RetroShare Link"), this, SLOT(copyChannelLink()));
	action->setEnabled(!mChannelId.empty());

	contextMnu.exec(QCursor::pos());

#endif
}

void GxsChannelDialog::createChannel()
{
	GxsChannelGroupDialog cc(mChannelQueue, this);
	cc.exec();
}

/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

QScrollArea *GxsChannelDialog::getScrollArea()
{
	return ui.scrollArea;
}

void GxsChannelDialog::deleteFeedItem(QWidget * /*item*/, uint32_t /*type*/)
{
}

void GxsChannelDialog::openChat(std::string /*peerId*/)
{
}

void GxsChannelDialog::editChannelDetail()
{
       if (mChannelId.empty()) {
               return;
       }

        GxsChannelGroupDialog cf(mChannelQueue, rsGxsChannels->getTokenService(), GxsGroupDialog::MODE_EDIT, mChannelId, this);
        cf.exec ();
}

void GxsChannelDialog::shareKey()
{
#if 0
	ShareKey shareUi(this, mChannelId, CHANNEL_KEY_SHARE);
	shareUi.exec();
#endif
}

void GxsChannelDialog::copyChannelLink()
{
#if 0
	if (mChannelId.empty()) {
		return;
	}

	ChannelInfo ci;
	if (rsChannels->getChannelInfo(mChannelId, ci)) {
		RetroShareLink link;
		if (link.createChannel(ci.channelId, "")) {
			QList<RetroShareLink> urls;
			urls.push_back(link);
			RSLinkClipboard::copyLinks(urls);
		}
	}
#endif
}

void GxsChannelDialog::createMsg()
{
	if (mChannelId.empty()) {
		return;
	}

	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(mChannelId);
	msgDialog->show();

	/* window will destroy itself! */
}

void GxsChannelDialog::restoreChannelKeys()
{
#if 0
	if(rsChannels->channelRestoreKeys(mChannelId))
		QMessageBox::information(NULL,tr("Publish rights restored."),tr("Publish rights have been restored for this channel.")) ;
	else
		QMessageBox::warning(NULL,tr("Publish not restored."),tr("Publish rights can't be restored for this channel.<br/>You're not the creator of this channel.")) ;
#endif
}

void GxsChannelDialog::selectChannel(const QString &id)
{
	mChannelId = id.toStdString();

	bool autoDl = rsGxsChannels->getChannelAutoDownload(mChannelId);
	setAutoDownloadButton(autoDl);

	requestGroupData(mChannelId);
	requestPosts(mChannelId);
}

static void channelInfoToGroupItemInfo(const RsGroupMetaData &channelInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(channelInfo.mGroupId);
	groupItemInfo.name = QString::fromUtf8(channelInfo.mGroupName.c_str());
	groupItemInfo.popularity = channelInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(channelInfo.mLastPost);
	groupItemInfo.subscribeFlags = channelInfo.mSubscribeFlags;

	QPixmap chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	groupItemInfo.icon = QIcon(chanImage);
}

void GxsChannelDialog::insertChannelData(const std::list<RsGroupMetaData> &channelList)
{
	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for(it = channelList.begin(); it != channelList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		channelInfoToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_SUBSCRIBED(flags)) 
		{
			if (IS_GROUP_ADMIN(flags) || IS_GROUP_PUBLISHER(flags))
			{
				adminList.push_back(groupItemInfo);
			}
			else
			{
				/* subscribed forum */
				subList.push_back(groupItemInfo);
			}
		} 
		else 
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->mPop, groupItemInfo));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10) 
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) 
	{
		if (i < popCount) 
		{
			popList.push_back(rit->second);
			i++;
		} 
		else 
		{
			otherList.push_back(rit->second);
		}
	}

	/* now we have our lists ---> update entries */

	ui.treeWidget->fillGroupItems(ownChannels, adminList);
	ui.treeWidget->fillGroupItems(subcribedChannels, subList);
	ui.treeWidget->fillGroupItems(popularChannels, popList);
	ui.treeWidget->fillGroupItems(otherChannels, otherList);

	updateMessageSummaryList("");
}

//void GxsChannelDialog::channelMsgReadSatusChanged(const QString& channelId, const QString& /*msgId*/, int /*status*/)
//{
//	updateMessageSummaryList(channelId.toStdString());
//}

void GxsChannelDialog::updateMessageSummaryList(const std::string &channelId)
{
#if 0
	QTreeWidgetItem *items[2] = { ownChannels, subcribedChannels };

	for (int item = 0; item < 2; item++) {
		int child;
		int childCount = items[item]->childCount();
		for (child = 0; child < childCount; child++) {
			QTreeWidgetItem *childItem = items[item]->child(child);
			std::string childId = ui.treeWidget->itemId(childItem).toStdString();
			if (childId.empty()) {
				continue;
			}

			if (channelId.empty() || childId == channelId) {
				/* Calculate unread messages */
				unsigned int newMessageCount = 0;
				unsigned int unreadMessageCount = 0;
				rsChannels->getMessageCount(childId, newMessageCount, unreadMessageCount);

				ui.treeWidget->setUnreadCount(childItem, unreadMessageCount);

				if (channelId.empty() == false) {
					/* Calculate only this channel */
					break;
				}
			}
		}
	}
#endif
}

#if 0
static bool sortChannelMsgSummary(const ChannelMsgSummary &msg1, const ChannelMsgSummary &msg2)
{
	return (msg1.ts > msg2.ts);
}
#endif


void GxsChannelDialog::insertChannelDetails(const RsGxsChannelGroup &group)
{
	/* IMAGE */
	QPixmap chanImage;
	if (group.mImage.mData != NULL) {
		chanImage.loadFromData(group.mImage.mData, group.mImage.mSize, "PNG");
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}
	ui.logoLabel->setPixmap(chanImage);

	/* set Channel name */
	ui.nameLabel->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));

	if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
	{
		mStateHelper->setWidgetEnabled(ui.postButton, true);
	}
	else
	{
		mStateHelper->setWidgetEnabled(ui.postButton, false);
	}

	if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	{
		ui.actionEnable_Auto_Download->setEnabled(true);
	}
	else
	{
		ui.actionEnable_Auto_Download->setEnabled(false);
	}
}

void GxsChannelDialog::insertChannelPosts(const std::vector<RsGxsChannelPost> &posts)
{
	std::vector<RsGxsChannelPost>::const_iterator it;

	// Do these need sorting? probably.
	// can we add that into the request?
	//msgs.sort(sortChannelMsgSummary);

	uint32_t subscribeFlags = 0xffffffff;

	for (it = posts.begin(); it != posts.end(); it++)
	{
		GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, *it, subscribeFlags, true);
		mChannelPostItems.push_back(item);
		ui.verticalLayout->addWidget(item);
	}
}

#if 0
void GxsChannelDialog::updateChannelMsgs()
{
	if (fillThread) {
#ifdef DEBUG_CHANNEL
		std::cerr << "GxsChannelDialog::updateChannelMsgs() stop current fill thread" << std::endl;
#endif
		// stop current fill thread
		GxsChannelFillThread *thread = fillThread;
		fillThread = NULL;
		thread->stop();
		delete(thread);

		progressLabel->hide();
		progressBar->hide();
	}

	if (!rsChannels) {
		return;
	}

	/* replace all the messages with new ones */
	QList<ChanMsgItem *>::iterator mit;
	for (mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++) {
		delete (*mit);
	}
	mChanMsgItems.clear();

	ChannelInfo ci;
	if (!rsChannels->getChannelInfo(mChannelId, ci)) {
		postButton->setEnabled(false);
		nameLabel->setText(tr("No Channel Selected"));
		logoLabel->setPixmap(QPixmap(":/images/channels.png"));
		logoLabel->setEnabled(false);
		return;
	}

	QPixmap chanImage;
	if (ci.pngImageLen != 0) {
		chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}
	logoLabel->setPixmap(chanImage);
	logoLabel->setEnabled(true);

	/* set Channel name */
	nameLabel->setText(QString::fromStdWString(ci.channelName));

	if (ci.channelFlags & RS_DISTRIB_PUBLISH) {
		postButton->setEnabled(true);
	} else {
		postButton->setEnabled(false);
	}

	if (!(ci.channelFlags & RS_DISTRIB_ADMIN) &&
		 (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)) {
		actionEnable_Auto_Download->setEnabled(true);
	} else {
		actionEnable_Auto_Download->setEnabled(false);
	}

#ifdef USE_THREAD
	progressLabel->show();
	progressBar->reset();
	progressBar->show();

	// create fill thread
	fillThread = new GxsChannelFillThread(this, mChannelId);

	// connect thread
	connect(fillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
	connect(fillThread, SIGNAL(addMsg(QString,QString,int,int)), this, SLOT(fillThreadAddMsg(QString,QString,int,int)), Qt::BlockingQueuedConnection);

#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::updateChannelMsgs() Start fill thread" << std::endl;
#endif

	// start thread
	fillThread->start();
#else
	std::list<ChannelMsgSummary> msgs;
	std::list<ChannelMsgSummary>::iterator it;
	rsChannels->getChannelMsgList(mChannelId, msgs);

	msgs.sort(sortChannelMsgSummary);

	for (it = msgs.begin(); it != msgs.end(); it++) {
		ChanMsgItem *cmi = new ChanMsgItem(this, 0, mChannelId, it->msgId, true);
		mChanMsgItems.push_back(cmi);
		verticalLayout_2->addWidget(cmi);
	}
#endif
}

void GxsChannelDialog::fillThreadFinished()
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::fillThreadFinished()" << std::endl;
#endif

	// thread has finished
	GxsChannelFillThread *thread = dynamic_cast<GxsChannelFillThread*>(sender());
	if (thread) {
		if (thread == fillThread) {
			// current thread has finished, hide progressbar and release thread
			progressBar->hide();
			progressLabel->hide();
			fillThread = NULL;
		}

#ifdef DEBUG_CHANNEL
		if (thread->wasStopped()) {
			// thread was stopped
			std::cerr << "GxsChannelDialog::fillThreadFinished() Thread was stopped" << std::endl;
		}
#endif

#ifdef DEBUG_CHANNEL
		std::cerr << "GxsChannelDialog::fillThreadFinished() Delete thread" << std::endl;
#endif

		thread->deleteLater();
		thread = NULL;
	}

#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::fillThreadFinished done()" << std::endl;
#endif
}

void GxsChannelDialog::fillThreadAddMsg(const QString &channelId, const QString &channelMsgId, int current, int count)
{
	if (sender() == fillThread) {
		// show fill progress
		if (count) {
			progressBar->setValue(current * progressBar->maximum() / count);
		}

		lockLayout(NULL, true);

		ChanMsgItem *cmi = new ChanMsgItem(this, 0, channelId.toStdString(), channelMsgId.toStdString(), true);
		mChanMsgItems.push_back(cmi);
		verticalLayout->addWidget(cmi);
		cmi->show();

		lockLayout(cmi, false);
	}
}

#endif

void GxsChannelDialog::unsubscribeChannel()
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::unsubscribeChannel()";
	std::cerr << std::endl;
#endif

	if (mChannelId.empty())
		return;

	uint32_t token = 0;
	rsGxsChannels->subscribeToGroup(token, mChannelId, false);
	mChannelQueue->queueRequest(token, 0 , RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_GROUP_CHANGE);
}

void GxsChannelDialog::subscribeChannel()
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::subscribeChannel()";
	std::cerr << std::endl;
#endif

	if (mChannelId.empty())
		return;

	uint32_t token = 0;
	rsGxsChannels->subscribeToGroup(token, mChannelId, true);
	mChannelQueue->queueRequest(token, 0 , RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_GROUP_CHANGE);
}

void GxsChannelDialog::showChannelDetails()
{
       if (mChannelId.empty()) {
               return;
       }

        GxsChannelGroupDialog cf(mChannelQueue, rsGxsChannels->getTokenService(), GxsGroupDialog::MODE_SHOW, mChannelId, this);
        cf.exec ();
}


void GxsChannelDialog::setAllAsReadClicked()
{
#if 0
	if (mChannelId.empty()) {
		return;
	}

	if (!rsChannels) {
		return;
	}

	ChannelInfo ci;
	if (rsChannels->getChannelInfo(mChannelId, ci) == false) {
		return;
	}

	if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED) {
		std::list<ChannelMsgSummary> msgs;
		std::list<ChannelMsgSummary>::iterator it;

		rsChannels->getChannelMsgList(mChannelId, msgs);

		for(it = msgs.begin(); it != msgs.end(); it++) {
			rsChannels->setMessageStatus(mChannelId, it->msgId, CHANNEL_MSG_STATUS_READ, CHANNEL_MSG_STATUS_READ | CHANNEL_MSG_STATUS_UNREAD_BY_USER);
		}
	}
#endif
}

void GxsChannelDialog::toggleAutoDownload()
{
	if (mChannelId.empty())
		return;

	bool autoDl = rsGxsChannels->getChannelAutoDownload(mChannelId);
	if (rsGxsChannels->setChannelAutoDownload(mChannelId, !autoDl))
	{
		setAutoDownloadButton(!autoDl);
	}
	else
	{
		std::cerr << "GxsChannelDialog::toggleAutoDownload() Auto Download failed to set";
		std::cerr << std::endl;
	}
}

bool GxsChannelDialog::navigate(const std::string& channelId, const std::string& msgId)
{
#if 0
	if (channelId.empty()) {
		return false;
	}

	if (treeWidget->activateId(QString::fromStdString(channelId), msgId.empty()) == NULL) {
		return false;
	}

	/* Messages are filled in selectChannel */
	if (mChannelId != channelId) {
		return false;
	}

	if (msgId.empty()) {
		return true;
	}

	/* Search exisiting item */
	QList<ChanMsgItem*>::iterator mit;
	for (mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++) {
		ChanMsgItem *item = *mit;
		if (item->msgId() == msgId) {
			// the next two lines are necessary to calculate the layout of the widgets in the scroll area (maybe there is a better solution)
			item->show();
			QCoreApplication::processEvents();

			scrollArea->ensureWidgetVisible(item, 0, 0);
			return true;
		}
	}
#endif

	return false;
}

void GxsChannelDialog::setAutoDownloadButton(bool autoDl)
{
	if (autoDl) {
		ui.actionEnable_Auto_Download->setText(tr("Disable Auto-Download"));
	}else{
		ui.actionEnable_Auto_Download->setText(tr("Enable Auto-Download"));
	}
}

/**********************************************************************************************
 * New Stuff here.
 *************/

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsChannelDialog::insertChannels()
{
	requestGroupSummary();
}

void GxsChannelDialog::requestGroupSummary()
{
	mStateHelper->setLoading(TOKEN_TYPE_LISTING, true);

#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::requestGroupSummary()";
	std::cerr << std::endl;
#endif

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_LISTING);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mChannelQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_LISTING);
}

void GxsChannelDialog::loadGroupSummary(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::loadGroupSummary()";
	std::cerr << std::endl;
#endif

	std::list<RsGroupMetaData> groupInfo;
	rsGxsChannels->getGroupSummary(token, groupInfo);

	if (groupInfo.size() > 0)
	{
		mStateHelper->setActive(TOKEN_TYPE_LISTING, true);

		insertChannelData(groupInfo);
	}
	else
	{
		std::cerr << "GxsChannelDialog::loadGroupSummary() ERROR No Groups...";
		std::cerr << std::endl;

		mStateHelper->setActive(TOKEN_TYPE_LISTING, false);
		mStateHelper->clear(TOKEN_TYPE_LISTING);
	}

	mStateHelper->setLoading(TOKEN_TYPE_LISTING, false);
}

void GxsChannelDialog::requestGroupData(const RsGxsGroupId &grpId)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::requestGroupData()";
	std::cerr << std::endl;
#endif

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_GROUP_DATA);

	if (grpId.empty()) {
		mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->clear(TOKEN_TYPE_GROUP_DATA);

		ui.nameLabel->setText(tr("No Channel Selected"));
		ui.logoLabel->setPixmap(QPixmap(":/images/channels.png"));

		return;
	}

	mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, true);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(grpId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	mChannelQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, TOKEN_TYPE_GROUP_DATA);
}

void GxsChannelDialog::loadGroupData(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::loadGroupData()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelGroup> groups;
	rsGxsChannels->getGroupData(token, groups);

	mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, false);

	if (groups.size() == 1)
	{
		mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, true);

		insertChannelDetails(groups[0]);
	}
	else
	{
		std::cerr << "GxsChannelDialog::loadGroupData() ERROR Not just one Group";
		std::cerr << std::endl;

		mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->clear(TOKEN_TYPE_GROUP_DATA);
	}
}

void GxsChannelDialog::requestPosts(const RsGxsGroupId &grpId)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::requestPosts()";
	std::cerr << std::endl;
#endif

	/* replace all the messages with new ones */
	QList<GxsChannelPostItem *>::iterator mit;
	for (mit = mChannelPostItems.begin(); mit != mChannelPostItems.end(); mit++) {
		delete (*mit);
	}
	mChannelPostItems.clear();

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_POSTS);

	if (grpId.empty()) {
		mStateHelper->setActive(TOKEN_TYPE_POSTS, false);
		mStateHelper->setLoading(TOKEN_TYPE_POSTS, false);
		mStateHelper->clear(TOKEN_TYPE_POSTS);
		return;
	}

	mStateHelper->setLoading(TOKEN_TYPE_POSTS, true);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(grpId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mChannelQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, TOKEN_TYPE_POSTS);
}

void GxsChannelDialog::loadPosts(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::loadPosts()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getPostData(token, posts);

	mStateHelper->setActive(TOKEN_TYPE_POSTS, true);

	insertChannelPosts(posts);

	mStateHelper->setLoading(TOKEN_TYPE_POSTS, false);
}

void GxsChannelDialog::acknowledgeGroupUpdate(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::acknowledgeGroupUpdate()";
	std::cerr << std::endl;
#endif

	RsGxsGroupId grpId;
	rsGxsChannels->acknowledgeGrp(token, grpId);
	insertChannels();
}

void GxsChannelDialog::acknowledgeMessageUpdate(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::acknowledgeMessageUpdate() TODO";
	std::cerr << std::endl;
#endif

	std::pair<RsGxsGroupId, RsGxsMessageId> msgId;
	rsGxsChannels->acknowledgeMsg(token, msgId);
	if (msgId.first == mChannelId)
	{
		requestPosts(mChannelId);
	}
}

void GxsChannelDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mChannelQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_GROUP_CHANGE:
				acknowledgeGroupUpdate(req.mToken);
				break;
		case TOKEN_TYPE_MESSAGE_CHANGE:
				acknowledgeMessageUpdate(req.mToken);
				break;
		case TOKEN_TYPE_LISTING:
				loadGroupSummary(req.mToken);
				break;
		case TOKEN_TYPE_GROUP_DATA:
				loadGroupData(req.mToken);
				break;
		case TOKEN_TYPE_POSTS:
				loadPosts(req.mToken);
				break;
		default:
				std::cerr << "GxsChannelDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
