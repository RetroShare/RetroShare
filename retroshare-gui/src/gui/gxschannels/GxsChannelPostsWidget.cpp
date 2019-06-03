/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsWidget.cpp                *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QDateTime>
#include <QSignalMapper>

#include "retroshare/rsgxscircles.h"

#include "GxsChannelPostsWidget.h"
#include "ui_GxsChannelPostsWidget.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include <algorithm>

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

	ui->postButton->setText(tr("Add new post"));
	
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
void GxsChannelPostsWidget::openComments(uint32_t /*type*/, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId>& msg_versions,const RsGxsMessageId &msgId, const QString &title)
{
	emit loadComment(groupId, msg_versions,msgId, title);
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
		GxsIdDetails::loadPixmapFromData(group.mImage.mData, group.mImage.mSize, chanImage);
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
	mStateHelper->setWidgetEnabled(ui->subscribeToolButton, true);


    bool autoDownload ;
            rsGxsChannels->getChannelAutoDownload(group.mMeta.mGroupId,autoDownload);
	setAutoDownload(autoDownload);
	
	RetroShareLink link;

	if (IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags)) {
		ui->feedToolButton->setEnabled(true);

		ui->fileToolButton->setEnabled(true);
		ui->infoWidget->hide();
		setViewMode(viewMode());
		
		ui->subscribeToolButton->setText(tr("Subscribed") + " " + QString::number(group.mMeta.mPop) );


		ui->infoPosts->clear();
		ui->infoDescription->clear();
	} else {
        ui->infoPosts->setText(QString::number(group.mMeta.mVisibleMsgCount));
		if(group.mMeta.mLastPost==0)
            ui->infoLastPost->setText(tr("Never"));
        else
            ui->infoLastPost->setText(DateTime::formatLongDateTime(group.mMeta.mLastPost));
			QString formatDescription = QString::fromUtf8(group.mDescription.c_str());

			unsigned int formatFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

			// embed smileys ?
			if (Settings->valueFromGroup(QString("ChannelPostsWidget"), QString::fromUtf8("Emoteicons_ChannelDecription"), true).toBool()) {
				formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
			}

			formatDescription = RsHtml().formatText(NULL, formatDescription, formatFlag);

			ui->infoDescription->setText(formatDescription);
        
        	ui->infoAdministrator->setId(group.mMeta.mAuthorId) ;
			
			link = RetroShareLink::createMessage(group.mMeta.mAuthorId, "");
			ui->infoAdministrator->setText(link.toHtml());
        
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
		
		ui->subscribeToolButton->setText(tr("Subscribe ") + " " + QString::number(group.mMeta.mPop) );

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

void GxsChannelPostsWidget::createPostItem(const RsGxsChannelPost& post, bool related)
{
	GxsChannelPostItem *item = NULL;

    if(!post.mMeta.mOrigMsgId.isNull())
    {
		FeedItem *feedItem = ui->feedWidget->findGxsFeedItem(post.mMeta.mGroupId, post.mMeta.mOrigMsgId);
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);

        if(item)
		{
			ui->feedWidget->removeFeedItem(item) ;

			GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, post, true, false,post.mOlderVersions);
			ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));

			return ;
		}
    }

	if (related)
    {
		FeedItem *feedItem = ui->feedWidget->findGxsFeedItem(post.mMeta.mGroupId, post.mMeta.mMsgId);
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	}
	if (item) {
		item->setPost(post);
		ui->feedWidget->setSort(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));
	} else {
		GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, post, true, false,post.mOlderVersions);
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

	int count = posts.size();
	int pos = 0;

	if (!thread) {
		ui->feedWidget->setSortingEnabled(false);
	}

    // collect new versions of posts if any

#ifdef DEBUG_CHANNEL
    std::cerr << "Inserting channel posts" << std::endl;
#endif

    std::vector<uint32_t> new_versions ;
    for (uint32_t i=0;i<posts.size();++i)
    {
		if(posts[i].mMeta.mOrigMsgId == posts[i].mMeta.mMsgId)
			posts[i].mMeta.mOrigMsgId.clear();

#ifdef DEBUG_CHANNEL
        std::cerr << "  " << i << ": msg_id=" << posts[i].mMeta.mMsgId << ": orig msg id = " << posts[i].mMeta.mOrigMsgId << std::endl;
#endif

        if(!posts[i].mMeta.mOrigMsgId.isNull())
            new_versions.push_back(i) ;
    }

#ifdef DEBUG_CHANNEL
    std::cerr << "New versions: " << new_versions.size() << std::endl;
#endif

    if(!new_versions.empty())
    {
#ifdef DEBUG_CHANNEL
        std::cerr << "  New versions present. Replacing them..." << std::endl;
        std::cerr << "  Creating search map."  << std::endl;
#endif

        // make a quick search map
        std::map<RsGxsMessageId,uint32_t> search_map ;
		for (uint32_t i=0;i<posts.size();++i)
            search_map[posts[i].mMeta.mMsgId] = i ;

        for(uint32_t i=0;i<new_versions.size();++i)
        {
#ifdef DEBUG_CHANNEL
            std::cerr << "  Taking care of new version  at index " << new_versions[i] << std::endl;
#endif

            uint32_t current_index = new_versions[i] ;
            uint32_t source_index  = new_versions[i] ;
#ifdef DEBUG_CHANNEL
            RsGxsMessageId source_msg_id = posts[source_index].mMeta.mMsgId ;
#endif

            // What we do is everytime we find a replacement post, we climb up the replacement graph until we find the original post
            // (or the most recent version of it). When we reach this post, we replace it with the data of the source post.
            // In the mean time, all other posts have their MsgId cleared, so that the posts are removed from the list.

            //std::vector<uint32_t> versions ;
            std::map<RsGxsMessageId,uint32_t>::const_iterator vit ;

            while(search_map.end() != (vit=search_map.find(posts[current_index].mMeta.mOrigMsgId)))
            {
#ifdef DEBUG_CHANNEL
                std::cerr << "    post at index " << current_index << " replaces a post at position " << vit->second ;
#endif

				// Now replace the post only if the new versionis more recent. It may happen indeed that the same post has been corrected multiple
				// times. In this case, we only need to replace the post with the newest version

				//uint32_t prev_index = current_index ;
				current_index = vit->second ;

				if(posts[current_index].mMeta.mMsgId.isNull())	// This handles the branching situation where this post has been already erased. No need to go down further.
                {
#ifdef DEBUG_CHANNEL
                    std::cerr << "  already erased. Stopping." << std::endl;
#endif
                    break ;
                }

				if(posts[current_index].mMeta.mPublishTs < posts[source_index].mMeta.mPublishTs)
				{
#ifdef DEBUG_CHANNEL
                    std::cerr << " and is more recent => following" << std::endl;
#endif
                    for(std::set<RsGxsMessageId>::const_iterator itt(posts[current_index].mOlderVersions.begin());itt!=posts[current_index].mOlderVersions.end();++itt)
						posts[source_index].mOlderVersions.insert(*itt);

					posts[source_index].mOlderVersions.insert(posts[current_index].mMeta.mMsgId);
					posts[current_index].mMeta.mMsgId.clear();	    // clear the msg Id so the post will be ignored
				}
#ifdef DEBUG_CHANNEL
                else
                    std::cerr << " but is older -> Stopping" << std::endl;
#endif
            }
        }
    }

#ifdef DEBUG_CHANNEL
    std::cerr << "Now adding posts..." << std::endl;
#endif

    for (std::vector<RsGxsChannelPost>::const_reverse_iterator it = posts.rbegin(); it != posts.rend(); ++it)
    {
#ifdef DEBUG_CHANNEL
		std::cerr << "  adding post: " << (*it).mMeta.mMsgId ;
#endif

        if(!(*it).mMeta.mMsgId.isNull())
		{
#ifdef DEBUG_CHANNEL
            std::cerr << " added" << std::endl;
#endif

			if (thread && thread->stopped())
				break;

			if (thread)
				thread->emitAddPost(qVariantFromValue(*it), related, ++pos, count);
			else
				createPostItem(*it, related);
		}
#ifdef DEBUG_CHANNEL
        else
            std::cerr << " skipped" << std::endl;
#endif
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

void GxsChannelPostsWidget::blank()
{
	mStateHelper->setWidgetEnabled(ui->postButton, false);
	mStateHelper->setWidgetEnabled(ui->subscribeToolButton, false);
	
	clearPosts();

    groupNameChanged(QString());

	ui->infoWidget->hide();
	ui->feedWidget->show();
	ui->fileWidget->hide();
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
	RsGxsGroupId grpId(groupId());
	if (grpId.isNull()) return;

	RsThread::async([=]()
	{
		rsGxsChannels->subscribeToChannel(grpId, subscribe);
	} );
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

	bool autoDownload;
	if(!rsGxsChannels->getChannelAutoDownload(grpId, autoDownload))
	{
		std::cerr << __PRETTY_FUNCTION__ << " failed to get autodownload value "
		          << "for channel: " << grpId.toStdString() << std::endl;
		return;
	}

	RsThread::async([this, grpId, autoDownload]()
	{
		if(!rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to set autodownload "
			          << "for channel: " << grpId.toStdString() << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [=]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

			std::cerr << __PRETTY_FUNCTION__ << " Has been executed on GUI "
			          << "thread but was scheduled by async thread" << std::endl;
		}, this );
	});
}

bool GxsChannelPostsWidget::insertGroupData(const uint32_t &token, RsGroupMetaData &metaData)
{
	std::vector<RsGxsChannelGroup> groups;
	rsGxsChannels->getGroupData(token, groups);

	if(groups.size() == 1)
	{
		insertChannelDetails(groups[0]);
		metaData = groups[0].mMeta;
		return true;
	}
    else
    {
        RsGxsChannelGroup distant_group;
        if(rsGxsChannels->retrieveDistantGroup(groupId(),distant_group))
        {
			insertChannelDetails(distant_group);
			metaData = distant_group.mMeta;
            return true ;
        }
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
