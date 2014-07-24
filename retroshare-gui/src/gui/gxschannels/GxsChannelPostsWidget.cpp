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
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"

#include <algorithm>

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

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
	ui->filterLineEdit->setCurrentFilter( Settings->valueFromGroup("ChannelFeed", "filter", FILTER_TITLE).toInt());
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterChanged(int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	ui->loadingLabel->hide();
	ui->progressLabel->hide();
	ui->progressBar->hide();

	ui->nameLabel->setMinimumWidth(20);

	mInProcessSettings = false;

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

void GxsChannelPostsWidget::processSettings(bool /*load*/)
{
	mInProcessSettings = true;
//	Settings->beginGroup(QString("ChannelPostsWidget"));
//
//	if (load) {
//		// load settings
//	} else {
//		// save settings
//	}
//
//	Settings->endGroup();
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
	if (mInProcessSettings) {
		return;
	}
	filterItems(ui->filterLineEdit->text());

	// save index
	Settings->setValueToGroup("ChannelFeed", "filter", filter);
}

void GxsChannelPostsWidget::filterItems(const QString& text)
{
	int filter = ui->filterLineEdit->currentFilter();

	/* Search exisiting item */
	QList<GxsFeedItem*>::iterator lit;
	for (lit = mPostItems.begin(); lit != mPostItems.end(); lit++)
	{
		GxsChannelPostItem *item = dynamic_cast<GxsChannelPostItem*>(*lit);
		if (!item) {
			continue;
		}
		filterItem(item,text,filter);
	}
}

bool GxsChannelPostsWidget::filterItem(GxsChannelPostItem *pItem, const QString &text, const int filter)
{
	bool bVisible = text.isEmpty();

	switch(filter)
	{
	case FILTER_TITLE:
		bVisible=pItem->getTitleLabel().contains(text,Qt::CaseInsensitive);
		break;
	case FILTER_MSG:
		bVisible=pItem->getMsgLabel().contains(text,Qt::CaseInsensitive);
		break;
	case FILTER_FILE_NAME:
	{
		std::list<SubFileItem *> fileItems=pItem->getFileItems();
		std::list<SubFileItem *>::iterator lit;
		for(lit = fileItems.begin(); lit != fileItems.end(); lit++)
		{
			SubFileItem *fi = *lit;
			QString fileName=QString::fromUtf8(fi->FileName().c_str());
			bVisible=(bVisible || fileName.contains(text,Qt::CaseInsensitive));
		}
	}
		break;
	default:
		bVisible=true;
		break;
	}
	pItem->setVisible(bVisible);

	return (bVisible);
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
			foreach (GxsFeedItem *loopItem, mPostItems) {
				if (loopItem->messageId() == it->mMeta.mMsgId) {
					item = dynamic_cast<GxsChannelPostItem*>(loopItem);
					break;
				}
			}
		}
		if (item) {
			item->setPost(*it);
			//TODO: Sort timestamp
		} else {
			item = new GxsChannelPostItem(this, 0, *it, subscribeFlags, true, false);
			if (!ui->filterLineEdit->text().isEmpty())
				filterItem(item, ui->filterLineEdit->text(), ui->filterLineEdit->currentFilter());

			mPostItems.push_back(item);
			if (related) {
				ui->verticalLayout->insertWidget(0, item);
			} else {
				ui->verticalLayout->addWidget(item);
			}
		}
	}
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

void GxsChannelPostsWidget::setMessageRead(GxsFeedItem *item, bool read)
{
	RsGxsGrpMsgIdPair msgPair = std::make_pair(item->groupId(), item->messageId());

	uint32_t token;
	rsGxsChannels->setMessageReadStatus(token, msgPair, read);
}
