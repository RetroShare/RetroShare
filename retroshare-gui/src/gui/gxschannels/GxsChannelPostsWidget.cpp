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
#include <QSignalMapper>

#include "retroshare/rsgxscircles.h"

#include "GxsChannelPostsWidget.h"
#include "ui_GxsChannelPostsWidget.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"
#include "gui/notifyqt.h"
#include <algorithm>
#include "util/DateTime.h"

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define ROLE_PUBLISH FEED_TREEWIDGET_SORTROLE

/****
 * #define DEBUG_CHANNEL
 ***/

/* View mode */
#define VIEW_MODE_FEEDS  1
#define VIEW_MODE_FILES  2

/** Constructor */
GxsChannelPostsWidget::GxsChannelPostsWidget(const RsGxsGroupId &channelId, QWidget *parent) :
	GxsMessageFramePostWidget(rsGxsChannels, parent),
	ui(new Ui::GxsChannelPostsWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);


	/* Setup UI helper */

	mStateHelper->addWidget(mTokenTypeAllPosts, ui->progressBar, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(mTokenTypeAllPosts, ui->loadingLabel, UISTATE_LOADING_VISIBLE);
	mStateHelper->addWidget(mTokenTypeAllPosts, ui->filterLineEdit);

	mStateHelper->addWidget(mTokenTypePosts, ui->loadingLabel, UISTATE_LOADING_VISIBLE);

	mStateHelper->addLoadPlaceholder(mTokenTypeGroupData, ui->nameLabel);

	mStateHelper->addWidget(mTokenTypeGroupData, ui->postButton);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->logoLabel);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->subscribeToolButton);

	/* Connect signals */
	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), FILTER_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Message"), FILTER_MSG, tr("Search Message"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Filename"), FILTER_FILE_NAME, tr("Search Filename"));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), ui->feedWidget, SLOT(setFilterText(QString)));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), ui->fileWidget, SLOT(setFilterText(QString)));
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterChanged(int)));

	/* Initialize view button */
	//setViewMode(VIEW_MODE_FEEDS); see processSettings
	ui->infoWidget->hide();

	QSignalMapper *signalMapper = new QSignalMapper(this);
	connect(ui->feedToolButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	connect(ui->fileToolButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	signalMapper->setMapping(ui->feedToolButton, VIEW_MODE_FEEDS);
	signalMapper->setMapping(ui->fileToolButton, VIEW_MODE_FILES);
	connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setViewMode(int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	ui->loadingLabel->hide();
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
	settingsChanged();
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
	Settings->beginGroup(QString("ChannelPostsWidget"));

	if (load) {
		// load settings

		/* Filter */
		ui->filterLineEdit->setCurrentFilter(Settings->value("filter", FILTER_TITLE).toInt());

		/* View mode */
		setViewMode(Settings->value("viewMode", VIEW_MODE_FEEDS).toInt());
	} else {
		// save settings

		/* Filter */
		Settings->setValue("filter", ui->filterLineEdit->currentFilter());

		/* View mode */
		Settings->setValue("viewMode", viewMode());
	}

	Settings->endGroup();
}

void GxsChannelPostsWidget::settingsChanged()
{
	mUseThread = Settings->getChannelLoadThread();

	mStateHelper->setWidgetVisible(ui->progressBar, mUseThread);
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
	if (mStateHelper->isLoading(mTokenTypeGroupData) || mStateHelper->isLoading(mTokenTypeAllPosts)) {
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

	ui->subscribersLabel->setText(QString::number(group.mMeta.mPop)) ;

	if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
	{
		mStateHelper->setWidgetEnabled(ui->postButton, true);
	}
	else
	{
		mStateHelper->setWidgetEnabled(ui->postButton, false);
	}

	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));

    bool autoDownload ;
            rsGxsChannels->getChannelAutoDownload(group.mMeta.mGroupId,autoDownload);
	setAutoDownload(autoDownload);

	if (IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags)) {
		ui->feedToolButton->setEnabled(true);

		ui->fileToolButton->setEnabled(true);
		ui->infoWidget->hide();
		setViewMode(viewMode());

		ui->infoPosts->clear();
		ui->infoDescription->clear();
	} else {
		ui->infoPosts->setText(QString::number(group.mMeta.mVisibleMsgCount));
		
		ui->infoLastPost->setText(DateTime::formatLongDateTime(group.mMeta.mLastPost));
		
		
		ui->infoDescription->setText(QString::fromUtf8(group.mDescription.c_str()));
        
        	ui->infoAdministrator->setId(group.mMeta.mAuthorId) ;
        
        	QString distrib_string ( "[unknown]" );
            
        	switch(group.mMeta.mCircleType)
		{
		case GXS_CIRCLE_TYPE_PUBLIC: distrib_string = tr("Public") ;
			break ;
		case GXS_CIRCLE_TYPE_EXTERNAL: 
		{
			RsGxsCircleDetails det ;

			// !! What we need here is some sort of CircleLabel, which loads the circle and updates the label when done.

			if(rsGxsCircles->getCircleDetails(group.mMeta.mCircleId,det)) 
				distrib_string = tr("Restricted to members of circle \"")+QString::fromUtf8(det.mCircleName.c_str()) +"\"";
			else
				distrib_string = tr("Restricted to members of circle ")+QString::fromStdString(group.mMeta.mCircleId.toStdString()) ;
		}
			break ;
		case GXS_CIRCLE_TYPE_YOUR_EYES_ONLY: distrib_string = tr("Your eyes only");
			break ;
		case GXS_CIRCLE_TYPE_LOCAL: distrib_string = tr("You and your friend nodes");
			break ;
		default:
			std::cerr << "(EE) badly initialised group distribution ID = " << group.mMeta.mCircleType << std::endl;
		}
 
		ui->infoDistribution->setText(distrib_string);

		ui->infoWidget->show();
		ui->feedWidget->hide();
		ui->fileWidget->hide();

		ui->feedToolButton->setEnabled(false);
		ui->fileToolButton->setEnabled(false);
	}
}

int GxsChannelPostsWidget::viewMode()
{
	if (ui->feedToolButton->isChecked()) {
		return VIEW_MODE_FEEDS;
	} else if (ui->fileToolButton->isChecked()) {
		return VIEW_MODE_FILES;
	}

	/* Default */
	return VIEW_MODE_FEEDS;
}

void GxsChannelPostsWidget::setViewMode(int viewMode)
{
	switch (viewMode) {
	case VIEW_MODE_FEEDS:
		ui->feedWidget->show();
		ui->fileWidget->hide();

		ui->feedToolButton->setChecked(true);
		ui->fileToolButton->setChecked(false);

		break;
	case VIEW_MODE_FILES:
		ui->feedWidget->hide();
		ui->fileWidget->show();

		ui->feedToolButton->setChecked(false);
		ui->fileToolButton->setChecked(true);

		break;
	default:
		setViewMode(VIEW_MODE_FEEDS);
		return;
	}
}

void GxsChannelPostsWidget::filterChanged(int filter)
{
	ui->feedWidget->setFilterType(filter);
	ui->fileWidget->setFilterType(filter);
}

/*static*/ bool GxsChannelPostsWidget::filterItem(FeedItem *feedItem, const QString &text, int filter)
{
	GxsChannelPostItem *item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!item) {
		return true;
	}

	bool bVisible = text.isEmpty();

	if (!bVisible)
	{
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
	}

	return bVisible;
}

void GxsChannelPostsWidget::createPostItem(const RsGxsChannelPost &post, bool related)
{
	GxsChannelPostItem *item = NULL;
	if (related) {
		FeedItem *feedItem = ui->feedWidget->findGxsFeedItem(post.mMeta.mGroupId, post.mMeta.mMsgId);
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	}
	if (item) {
		item->setPost(post);
		ui->feedWidget->setSort(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));
	} else {
		/* Group is not always available because of the TokenQueue */
		RsGxsChannelGroup dummyGroup;
		dummyGroup.mMeta.mGroupId = groupId();
		dummyGroup.mMeta.mSubscribeFlags = 0xffffffff;
		GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, dummyGroup, post, true, false);
		ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));
	}

	ui->fileWidget->addFiles(post, related);
}

void GxsChannelPostsWidget::fillThreadCreatePost(const QVariant &post, bool related, int current, int count)
{
	/* show fill progress */
	if (count) {
		ui->progressBar->setValue(current * ui->progressBar->maximum() / count);
	}

	if (!post.canConvert<RsGxsChannelPost>()) {
		return;
	}

	createPostItem(post.value<RsGxsChannelPost>(), related);
}

void GxsChannelPostsWidget::insertChannelPosts(std::vector<RsGxsChannelPost> &posts, GxsMessageFramePostThread *thread, bool related)
{
	if (related && thread) {
		std::cerr << "GxsChannelPostsWidget::insertChannelPosts fill only related posts as thread is not possible" << std::endl;
		return;
	}

    std::vector<RsGxsChannelPost>::const_reverse_iterator it;

	int count = posts.size();
	int pos = 0;

	if (!thread) {
		ui->feedWidget->setSortingEnabled(false);
	}

    for (it = posts.rbegin(); it != posts.rend(); ++it)
	{
		if (thread && thread->stopped()) {
			break;
		}

		if (thread) {
			thread->emitAddPost(qVariantFromValue(*it), related, ++pos, count);
		} else {
			createPostItem(*it, related);
		}
	}

	if (!thread) {
		ui->feedWidget->setSortingEnabled(true);
	}
}

void GxsChannelPostsWidget::clearPosts()
{
	ui->feedWidget->clear();
	ui->fileWidget->clear();
}

bool GxsChannelPostsWidget::navigatePostItem(const RsGxsMessageId &msgId)
{
	FeedItem *feedItem = ui->feedWidget->findGxsFeedItem(groupId(), msgId);
	if (!feedItem) {
		return false;
	}

	return ui->feedWidget->scrollTo(feedItem, true);
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

    bool autoDownload ;
        if(!rsGxsChannels->getChannelAutoDownload(grpId,autoDownload) || !rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
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

void GxsChannelPostsWidget::insertAllPosts(const uint32_t &token, GxsMessageFramePostThread *thread)
{
	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getPostData(token, posts);

	insertChannelPosts(posts, thread, false);
}

void GxsChannelPostsWidget::insertPosts(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;
	rsGxsChannels->getPostData(token, posts);

	insertChannelPosts(posts, NULL, true);
}

class GxsChannelPostsReadData
{
public:
	GxsChannelPostsReadData(bool read)
	{
		mRead = read;
		mLastToken = 0;
	}

public:
	bool mRead;
	uint32_t mLastToken;
};

static void setAllMessagesReadCallback(FeedItem *feedItem, void *data)
{
	GxsChannelPostItem *channelPostItem = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!channelPostItem) {
		return;
	}

	GxsChannelPostsReadData *readData = (GxsChannelPostsReadData*) data;
	bool is_not_new = !channelPostItem->isUnread() ;

	if(is_not_new == readData->mRead)
		return ;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(channelPostItem->groupId(), channelPostItem->messageId());
	rsGxsChannels->setMessageReadStatus(readData->mLastToken, msgPair, readData->mRead);
}

void GxsChannelPostsWidget::setAllMessagesReadDo(bool read, uint32_t &token)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(subscribeFlags())) {
		return;
	}

	GxsChannelPostsReadData data(read);
	ui->feedWidget->withAll(setAllMessagesReadCallback, &data);

	token = data.mLastToken;
}
