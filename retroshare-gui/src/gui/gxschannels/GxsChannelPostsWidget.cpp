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

#include <QDateTime>

#include "GxsChannelPostsWidget.h"
#include "ui_GxsChannelPostsWidget.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"

#include <algorithm>

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define ROLE_PUBLISH FEED_TREEWIDGET_SORTROLE

/****
 * #define DEBUG_CHANNEL
 ***/

/* Filters */
#define FILTER_TITLE     1
#define FILTER_MSG       2
#define FILTER_FILE_NAME 3
#define FILTER_COUNT     3

/** Constructor */
GxsChannelPostsWidget::GxsChannelPostsWidget(const RsGxsGroupId &channelId, QWidget *parent) :
	GxsMessageFramePostWidget(rsGxsChannels, parent),
	ui(new Ui::GxsChannelPostsWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	mInProcessSettings = false;

	/* Setup UI helper */

	// No progress yet
	mStateHelper->addWidget(mTokenTypePosts, ui->loadingLabel, UISTATE_LOADING_VISIBLE);
//	mStateHelper->addWidget(mTokenTypePosts, ui->progressBar, UISTATE_LOADING_VISIBLE);
//	mStateHelper->addWidget(mTokenTypePosts, ui->progressLabel, UISTATE_LOADING_VISIBLE);

	mStateHelper->addLoadPlaceholder(mTokenTypeGroupData, ui->nameLabel);

	mStateHelper->addWidget(mTokenTypeGroupData, ui->postButton);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->logoLabel);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->subscribeToolButton);

	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), FILTER_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Message"), FILTER_MSG, tr("Search Message"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Filename"), FILTER_FILE_NAME, tr("Search Filename"));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), ui->feedWidget, SLOT(setFilterText(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterChanged(int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	ui->loadingLabel->hide();
	ui->progressLabel->hide();
	ui->progressBar->hide();

	ui->nameLabel->setMinimumWidth(20);

	/* Initialize feed widget */
	ui->feedWidget->setSortRole(ROLE_PUBLISH, Qt::DescendingOrder);
	ui->feedWidget->setFilterCallback(filterItem);

	/* load settings */
	processSettings(true);

	/* Initialize subscribe button */
	QIcon icon;
	icon.addPixmap(QPixmap(":/images/redled.png"), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap(":/images/start.png"), QIcon::Normal, QIcon::Off);
	mAutoDownloadAction = new QAction(icon, "", this);
	mAutoDownloadAction->setCheckable(true);
	connect(mAutoDownloadAction, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));

	ui->subscribeToolButton->addSubscribedAction(mAutoDownloadAction);

	/* Initialize GUI */
	setAutoDownload(false);
	setGroupId(channelId);
}

GxsChannelPostsWidget::~GxsChannelPostsWidget()
{
	// save settings
	processSettings(false);

	delete(mAutoDownloadAction);

	delete ui;
}

void GxsChannelPostsWidget::processSettings(bool load)
{
	mInProcessSettings = true;
	Settings->beginGroup(QString("ChannelPostsWidget"));

	if (load) {
		// load settings
		ui->filterLineEdit->setCurrentFilter(Settings->value("filter", FILTER_TITLE).toInt());
	} else {
		// save settings
	}

	Settings->endGroup();
	mInProcessSettings = false;
}

void GxsChannelPostsWidget::groupNameChanged(const QString &name)
{
	if (groupId().isNull()) {
		ui->nameLabel->setText(tr("No Channel Selected"));
		ui->logoLabel->setPixmap(QPixmap(":/images/channels.png"));
	} else {
		ui->nameLabel->setText(name);
	}
}

QIcon GxsChannelPostsWidget::groupIcon()
{
	if (mStateHelper->isLoading(mTokenTypeGroupData) || mStateHelper->isLoading(mTokenTypePosts)) {
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
	return NULL;
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
	emit loadComment(groupId, msgId, title);
}

void GxsChannelPostsWidget::createMsg()
{
	if (groupId().isNull()) {
		return;
	}

	if (!IS_GROUP_SUBSCRIBED(subscribeFlags())) {
		return;
	}

	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(groupId());
	msgDialog->show();

	/* window will destroy itself! */
}

void GxsChannelPostsWidget::insertChannelDetails(const RsGxsChannelGroup &group)
{
	/* IMAGE */
	QPixmap chanImage;
	if (group.mImage.mData != NULL) {
		chanImage.loadFromData(group.mImage.mData, group.mImage.mSize, "PNG");
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}
	ui->logoLabel->setPixmap(chanImage);

	if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
	{
		mStateHelper->setWidgetEnabled(ui->postButton, true);
	}
	else
	{
		mStateHelper->setWidgetEnabled(ui->postButton, false);
	}

	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));

	bool autoDownload = rsGxsChannels->getChannelAutoDownload(group.mMeta.mGroupId);
	setAutoDownload(autoDownload);
}

void GxsChannelPostsWidget::filterChanged(int filter)
{
	ui->feedWidget->setFilterType(filter);

	if (mInProcessSettings) {
		return;
	}

	// save index
	Settings->setValueToGroup("ChannelPostsWidget", "filter", filter);
}

/*static*/ bool GxsChannelPostsWidget::filterItem(FeedItem *feedItem, const QString &text, int filter)
{
	GxsChannelPostItem *item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!item) {
		return true;
	}

	bool bVisible = text.isEmpty();

	switch(filter)
	{
	case FILTER_TITLE:
		bVisible = item->getTitleLabel().contains(text,Qt::CaseInsensitive);
		break;
	case FILTER_MSG:
		bVisible = item->getMsgLabel().contains(text,Qt::CaseInsensitive);
		break;
	case FILTER_FILE_NAME:
	{
		std::list<SubFileItem *> fileItems = item->getFileItems();
		std::list<SubFileItem *>::iterator lit;
		for(lit = fileItems.begin(); lit != fileItems.end(); ++lit)
		{
			SubFileItem *fi = *lit;
			QString fileName = QString::fromUtf8(fi->FileName().c_str());
			bVisible = (bVisible || fileName.contains(text,Qt::CaseInsensitive));
		}
		break;
	}
	default:
		bVisible = true;
		break;
	}

	return bVisible;
}

void GxsChannelPostsWidget::insertChannelPosts(std::vector<RsGxsChannelPost> &posts, bool related)
{
	std::vector<RsGxsChannelPost>::const_iterator it;

	uint32_t subscribeFlags = 0xffffffff;

	ui->feedWidget->setSortingEnabled(false);

	for (it = posts.begin(); it != posts.end(); it++)
	{
		const RsGxsChannelPost &msg = *it;

		GxsChannelPostItem *item = NULL;
		if (related) {
			FeedItem *feedItem = ui->feedWidget->findGxsFeedItem(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
			item = dynamic_cast<GxsChannelPostItem*>(feedItem);
		}
		if (item) {
			item->setContent(*it);
			//TODO: Sort timestamp
		} else {
			item = new GxsChannelPostItem(this, 0, *it, subscribeFlags, true, false);
			ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(msg.mMeta.mPublishTs));
		}
	}

	ui->feedWidget->setSortingEnabled(true);
}

void GxsChannelPostsWidget::clearPosts()
{
	ui->feedWidget->clear();
}

void GxsChannelPostsWidget::subscribeGroup(bool subscribe)
{
	if (groupId().isNull()) {
		return;
	}

	uint32_t token;
	rsGxsChannels->subscribeToGroup(token, groupId(), subscribe);
//	mChannelQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void GxsChannelPostsWidget::setAutoDownload(bool autoDl)
{
	mAutoDownloadAction->setChecked(autoDl);
	mAutoDownloadAction->setText(autoDl ? tr("Disable Auto-Download") : tr("Enable Auto-Download"));
}

void GxsChannelPostsWidget::toggleAutoDownload()
{
	RsGxsGroupId grpId = groupId();
	if (grpId.isNull()) {
		return;
	}

	bool autoDownload = rsGxsChannels->getChannelAutoDownload(grpId);
	if (!rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
	{
		std::cerr << "GxsChannelDialog::toggleAutoDownload() Auto Download failed to set";
		std::cerr << std::endl;
	}
}

bool GxsChannelPostsWidget::insertGroupData(const uint32_t &token, RsGroupMetaData &metaData)
{
	std::vector<RsGxsChannelGroup> groups;
	rsGxsChannels->getGroupData(token, groups);

	if (groups.size() == 1)
	{
		insertChannelDetails(groups[0]);
		metaData = groups[0].mMeta;
		return true;
	}

	return false;
}

void GxsChannelPostsWidget::insertPosts(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getPostData(token, posts);

	insertChannelPosts(posts, false);
}

void GxsChannelPostsWidget::insertRelatedPosts(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getRelatedPosts(token, posts);

	insertChannelPosts(posts, true);
}

static void setAllMessagesReadCallback(FeedItem *feedItem, const QVariant &data)
{
	GxsChannelPostItem *channelPostItem = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!channelPostItem) {
		return;
	}

	RsGxsGrpMsgIdPair msgPair = std::make_pair(channelPostItem->groupId(), channelPostItem->messageId());

	uint32_t token;
	rsGxsChannels->setMessageReadStatus(token, msgPair, data.toBool());
}

void GxsChannelPostsWidget::setAllMessagesRead(bool read)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(subscribeFlags())) {
		return;
	}

	ui->feedWidget->withAll(setAllMessagesReadCallback, read);
}
