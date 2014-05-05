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

#include "GxsChannelPostsWidget.h"
#include "ui_GxsChannelPostsWidget.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"

#include <algorithm>

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define WARNING_LIMIT 3600*24*2

/****
 * #define DEBUG_CHANNEL
 ***/

#define USE_THREAD

#define TOKEN_TYPE_MESSAGE_CHANGE   4
#define TOKEN_TYPE_GROUP_DATA       6
#define TOKEN_TYPE_POSTS            7
#define TOKEN_TYPE_RELATEDPOSTS     8

/** Constructor */
GxsChannelPostsWidget::GxsChannelPostsWidget(const RsGxsGroupId &channelId, QWidget *parent)
	: GxsMessageFrameWidget(rsGxsChannels, parent),
	ui(new Ui::GxsChannelPostsWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	// No progress yet
//	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui->progressBar, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui->progressLabel, UISTATE_LOADING_VISIBLE);

	mStateHelper->addLoadPlaceholder(TOKEN_TYPE_GROUP_DATA, ui->nameLabel);

	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui->postButton);
	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui->logoLabel);

	mChannelQueue = new TokenQueue(rsGxsChannels->getTokenService(), this);

	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
//	connect(NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	ui->progressLabel->hide();
	ui->progressBar->hide();

	ui->nameLabel->setMinimumWidth(20);

	/* load settings */
	processSettings(true);

	/* Initialize GUI */
	setGroupId(channelId);
}

GxsChannelPostsWidget::~GxsChannelPostsWidget()
{
	// save settings
	processSettings(false);
}

void GxsChannelPostsWidget::updateDisplay(bool complete)
{
	if (complete) {
		/* Fill complete */
		requestGroupData(mChannelId);
		requestPosts(mChannelId);
		return;
	}

	std::list<RsGxsGroupId> &grpIds = getGrpIds();
	if (!mChannelId.isNull() && std::find(grpIds.begin(), grpIds.end(), mChannelId) != grpIds.end()) {
		requestGroupData(mChannelId);
		/* Do we need to fill all posts? */
		requestPosts(mChannelId);
	} else {
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgs = getMsgIds();
		if (!msgs.empty())
		{
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit = msgs.find(mChannelId);
			if(mit != msgs.end())
			{
				requestRelatedPosts(mChannelId, mit->second);
			}
		}
	}
}

//UserNotify *GxsChannelPostsWidget::getUserNotify(QObject *parent)
//{
//	return new ChannelUserNotify(parent);
//}

void GxsChannelPostsWidget::processSettings(bool load)
{
//	Settings->beginGroup(QString("GxsChannelDialog"));
//
//	if (load) {
//		// load settings
//	} else {
//		// save settings
//	}
//
//	Settings->endGroup();
}

void GxsChannelPostsWidget::setGroupId(const RsGxsGroupId &groupId)
{
	if (mChannelId == groupId) {
		if (!groupId.isNull()) {
			return;
		}
	}

	mChannelId = groupId;
	ui->nameLabel->setText(mChannelId.isNull () ? "" : tr("Loading"));

	emit groupChanged(this);

	fillComplete();
}

QString GxsChannelPostsWidget::groupName(bool withUnreadCount)
{
	QString name = mChannelId.isNull () ? tr("No name") : ui->nameLabel->text();

//	if (withUnreadCount && mUnreadCount) {
//		name += QString(" (%1)").arg(mUnreadCount);
//	}

	return name;
}

QIcon GxsChannelPostsWidget::groupIcon()
{
	if (mStateHelper->isLoading(TOKEN_TYPE_POSTS)) {
		return QIcon(":/images/kalarm.png");
	}

//	if (mNewCount) {
//		return QIcon(":/images/message-state-new.png");
//	}

	return QIcon();
}

/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

QScrollArea *GxsChannelPostsWidget::getScrollArea()
{
	return ui->scrollArea;
}

void GxsChannelPostsWidget::deleteFeedItem(QWidget * /*item*/, uint32_t /*type*/)
{
}

void GxsChannelPostsWidget::openChat(const RsPeerId & /*peerId*/)
{
}

// Callback from Widget->FeedHolder->ServiceDialog->CommentContainer->CommentDialog,
void GxsChannelPostsWidget::openComments(uint32_t /*type*/, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title)
{
	emit commentLoad(groupId, msgId, title);
}

void GxsChannelPostsWidget::createMsg()
{
	if (mChannelId.isNull()) {
		return;
	}

	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(mChannelId);
	msgDialog->show();

	/* window will destroy itself! */
}

//void GxsChannelPostsWidget::channelMsgReadSatusChanged(const QString& channelId, const QString& /*msgId*/, int /*status*/)
//{
//	updateMessageSummaryList(channelId.toStdString());
//}

void GxsChannelPostsWidget::insertChannelDetails(const RsGxsChannelGroup &group)
{
	mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, true);

	/* IMAGE */
	QPixmap chanImage;
	if (group.mImage.mData != NULL) {
		chanImage.loadFromData(group.mImage.mData, group.mImage.mSize, "PNG");
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}
	ui->logoLabel->setPixmap(chanImage);

	/* set Channel name */
	ui->nameLabel->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));

	if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
	{
		mStateHelper->setWidgetEnabled(ui->postButton, true);
	}
	else
	{
		mStateHelper->setWidgetEnabled(ui->postButton, false);
	}

	setAutoDownloadButton(group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);

	emit groupChanged(this);
}

static bool sortChannelMsgSummaryAsc(const RsGxsChannelPost &msg1, const RsGxsChannelPost &msg2)
{
	return (msg1.mMeta.mPublishTs > msg2.mMeta.mPublishTs);
}

static bool sortChannelMsgSummaryDesc(const RsGxsChannelPost &msg1, const RsGxsChannelPost &msg2)
{
	return (msg1.mMeta.mPublishTs < msg2.mMeta.mPublishTs);
}

void GxsChannelPostsWidget::insertChannelPosts(std::vector<RsGxsChannelPost> &posts, bool related)
{
	std::vector<RsGxsChannelPost>::const_iterator it;

	// Do these need sorting? probably.
	// can we add that into the request?
	if (related) {
		/* Sort descending to add posts at top */
		std::sort(posts.begin(), posts.end(), sortChannelMsgSummaryDesc);
	} else {
		std::sort(posts.begin(), posts.end(), sortChannelMsgSummaryAsc);
	}

	uint32_t subscribeFlags = 0xffffffff;

	for (it = posts.begin(); it != posts.end(); it++)
	{
		GxsChannelPostItem *item = NULL;
		if (related) {
			foreach (GxsChannelPostItem *loopItem, mChannelPostItems) {
				if (loopItem->messageId() == it->mMeta.mMsgId) {
					item = loopItem;
					break;
				}
			}
		}
		if (item) {
			//TODO: Update item
		} else {
			item = new GxsChannelPostItem(this, 0, *it, subscribeFlags, true);
			mChannelPostItems.push_back(item);
			if (related) {
				ui->verticalLayout->insertWidget(0, item);
			} else {
				ui->verticalLayout->addWidget(item);
			}
		}
	}
}

#if 0
void GxsChannelPostsWidget::updateChannelMsgs()
{
	if (fillThread) {
#ifdef DEBUG_CHANNEL
		std::cerr << "GxsChannelPostsWidget::updateChannelMsgs() stop current fill thread" << std::endl;
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
	std::cerr << "GxsChannelPostsWidget::updateChannelMsgs() Start fill thread" << std::endl;
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

void GxsChannelPostsWidget::fillThreadFinished()
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::fillThreadFinished()" << std::endl;
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
			std::cerr << "GxsChannelPostsWidget::fillThreadFinished() Thread was stopped" << std::endl;
		}
#endif

#ifdef DEBUG_CHANNEL
		std::cerr << "GxsChannelPostsWidget::fillThreadFinished() Delete thread" << std::endl;
#endif

		thread->deleteLater();
		thread = NULL;
	}

#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::fillThreadFinished done()" << std::endl;
#endif
}

void GxsChannelPostsWidget::fillThreadAddMsg(const QString &channelId, const QString &channelMsgId, int current, int count)
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

void GxsChannelPostsWidget::setAllMessagesRead(bool read)
{
#if 0
	if (mChannelId.isNull()) {
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

void GxsChannelPostsWidget::setAutoDownloadButton(bool autoDl)
{
	if (autoDl) {
		ui->actionEnable_Auto_Download->setText(tr("Disable Auto-Download"));
	}else{
		ui->actionEnable_Auto_Download->setText(tr("Enable Auto-Download"));
	}
}

void GxsChannelPostsWidget::clearPosts()
{
	/* replace all the messages with new ones */
	QList<GxsChannelPostItem *>::iterator mit;
	for (mit = mChannelPostItems.begin(); mit != mChannelPostItems.end(); mit++) {
		delete (*mit);
	}
	mChannelPostItems.clear();
}

/**********************************************************************************************
 * New Stuff here.
 *************/

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsChannelPostsWidget::requestGroupData(const RsGxsGroupId &grpId)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::requestGroupData()";
	std::cerr << std::endl;
#endif

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_GROUP_DATA);

    if (grpId.isNull()) {
		mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->clear(TOKEN_TYPE_GROUP_DATA);

		ui->nameLabel->setText(tr("No Channel Selected"));
		ui->logoLabel->setPixmap(QPixmap(":/images/channels.png"));

		emit groupChanged(this);

		return;
	}

	mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, true);
	emit groupChanged(this);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(grpId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	mChannelQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, TOKEN_TYPE_GROUP_DATA);
}

void GxsChannelPostsWidget::loadGroupData(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::loadGroupData()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelGroup> groups;
	rsGxsChannels->getGroupData(token, groups);

	mStateHelper->setLoading(TOKEN_TYPE_GROUP_DATA, false);

	if (groups.size() == 1)
	{
		insertChannelDetails(groups[0]);
	}
	else
	{
		std::cerr << "GxsChannelPostsWidget::loadGroupData() ERROR Not just one Group";
		std::cerr << std::endl;

		mStateHelper->setActive(TOKEN_TYPE_GROUP_DATA, false);
		mStateHelper->clear(TOKEN_TYPE_GROUP_DATA);

		emit groupChanged(this);
	}
}

void GxsChannelPostsWidget::requestPosts(const RsGxsGroupId &grpId)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::requestPosts()";
	std::cerr << std::endl;
#endif

	/* Request all posts */
	clearPosts();

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_POSTS);

    if (grpId.isNull()) {
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

void GxsChannelPostsWidget::loadPosts(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::loadPosts()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getPostData(token, posts);

	mStateHelper->setActive(TOKEN_TYPE_POSTS, true);

	insertChannelPosts(posts, false);

	mStateHelper->setLoading(TOKEN_TYPE_POSTS, false);
}

void GxsChannelPostsWidget::requestRelatedPosts(const RsGxsGroupId &grpId, const std::vector<RsGxsMessageId> &msgIds)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::requestRelatedPosts()";
	std::cerr << std::endl;
#endif

	mChannelQueue->cancelActiveRequestTokens(TOKEN_TYPE_RELATEDPOSTS);

	if (grpId.isNull()) {
		mStateHelper->setActive(TOKEN_TYPE_POSTS, false);
		mStateHelper->setLoading(TOKEN_TYPE_POSTS, false);
		mStateHelper->clear(TOKEN_TYPE_POSTS);
		return;
	}

	mStateHelper->setLoading(TOKEN_TYPE_POSTS, true);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(grpId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_VERSIONS;

	uint32_t token;
	std::vector<RsGxsGrpMsgIdPair> relatedMsgIds;
	for (std::vector<RsGxsMessageId>::const_iterator msgIt = msgIds.begin(); msgIt != msgIds.end(); ++msgIt) {
		relatedMsgIds.push_back(RsGxsGrpMsgIdPair(grpId, *msgIt));
	}
	mChannelQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, relatedMsgIds, TOKEN_TYPE_RELATEDPOSTS);
}

void GxsChannelPostsWidget::loadRelatedPosts(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::loadRelatedPosts()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getRelatedPosts(token, posts);

	mStateHelper->setActive(TOKEN_TYPE_POSTS, true);

	insertChannelPosts(posts, true);

	mStateHelper->setLoading(TOKEN_TYPE_POSTS, false);
}

void GxsChannelPostsWidget::acknowledgeMessageUpdate(const uint32_t &token)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::acknowledgeMessageUpdate() TODO";
	std::cerr << std::endl;
#endif

	std::pair<RsGxsGroupId, RsGxsMessageId> msgId;
	rsGxsChannels->acknowledgeMsg(token, msgId);
	if (msgId.first == mChannelId)
	{
		requestPosts(mChannelId);
	}
}

void GxsChannelPostsWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_CHANNEL
	std::cerr << "GxsChannelPostsWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mChannelQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_MESSAGE_CHANGE:
				acknowledgeMessageUpdate(req.mToken);
				break;
		case TOKEN_TYPE_GROUP_DATA:
				loadGroupData(req.mToken);
				break;
		case TOKEN_TYPE_POSTS:
				loadPosts(req.mToken);
				break;
		case TOKEN_TYPE_RELATEDPOSTS:
				loadRelatedPosts(req.mToken);
				break;
		default:
				std::cerr << "GxsChannelPostsWidget::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
