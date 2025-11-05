/*******************************************************************************
 * gui/feeds/ChannelsCommentsItem.cpp                                               *
 *                                                                             *
 * Copyright (c) 2020, Retroshare Team   <retroshare.project@gmail.com>        *
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

#include <QTimer>
#include <QFileInfo>
#include <QStyle>

#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "rshare.h"
#include "ChannelsCommentsItem.h"
#include "ui_ChannelsCommentsItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/stringutil.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/gxschannels/GxsChannelDialog.h"
#include "gui/MainWindow.h"

#include <retroshare/rsidentity.h>

#include <iostream>
#include <cmath>

/****
 * #define DEBUG_ITEM 1
 ****/

ChannelsCommentsItem::ChannelsCommentsItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &commentId, const RsGxsMessageId &threadId, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, commentId, isHome, rsGxsChannels, autoUpdate), // this one should be in GxsFeedItem
    mThreadId(threadId)
{
    mLoadingStatus = LOADING_STATUS_NO_DATA;
    mLoadingComment = false;
    mLoadingGroup = false;
    mLoadingMessage = false;

	setup();
}

void ChannelsCommentsItem::paintEvent(QPaintEvent *e)
{
	/* This method employs a trick to trigger a deferred loading. The post and group is requested only
	 * when actually displayed on the screen. */

    if(mLoadingStatus != LOADING_STATUS_FILLED && !mGroupMeta.mGroupId.isNull() && !mComment.mMeta.mMsgId.isNull())
        mLoadingStatus = LOADING_STATUS_HAS_DATA;

    if(mGroupMeta.mGroupId.isNull() && !mLoadingGroup)
        loadGroupData();

    if(mComment.mMeta.mMsgId.isNull() && !mLoadingComment)
        loadCommentData();

    if(mPost.mMeta.mMsgId.isNull() && !mLoadingMessage)
        loadMessageData();

    switch(mLoadingStatus)
    {
    case LOADING_STATUS_FILLED:
    case LOADING_STATUS_NO_DATA:
    default:
        break;
    case LOADING_STATUS_HAS_DATA:
        fill();
        mLoadingStatus = LOADING_STATUS_FILLED;
        break;
    }

	GxsFeedItem::paintEvent(e) ;
}

ChannelsCommentsItem::~ChannelsCommentsItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

    while( (mLoadingGroup || mLoadingComment)
           && std::chrono::steady_clock::now() < timeout )
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting for data to load "
                << (mLoadingGroup ? "Group " : "")
                << (mLoadingMessage ? "Message " : "")
                << (mLoadingComment ? "Comment " : "")
                << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

	delete(ui);
}

bool ChannelsCommentsItem::isUnread() const
{
    return IS_MSG_UNREAD(mComment.mMeta.mMsgStatus) ;
}

void ChannelsCommentsItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */

	ui = new Ui::ChannelsCommentsItem;
	ui->setupUi(this);

    // Manually set icons to allow to use clever resource sharing that is missing in Qt for Icons loaded from Qt resource file.
    // This is particularly important here because a channel may contain many posts, so duplicating the QImages here is deadly for the
    // memory.

	ui->logoLabel->hide();
    //ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/comment.png"));
    ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
    ui->voteUpButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_up.png"));
    ui->voteDownButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_down.png"));
    ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
    ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
    ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
    ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

	setAttribute(Qt::WA_DeleteOnClose, true);

	mCloseOnRead = false;

	/* clear ui */
	ui->datetimeLabel->clear();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));

	// HACK FOR NOW.
	//connect(ui->commentButton, SIGNAL(clicked()), this, SLOT(loadComments()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));
	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

	// hide voting buttons, backend is not implemented yet
	ui->voteUpButton->hide();
	ui->voteDownButton->hide();
	//connect(ui-> voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	//connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT(makeDownVote()));

	ui->scoreLabel->hide();

	// hide expand button, replies is not implemented yet
	ui->expandButton->hide();

	ui->feedFrame->setProperty("new", false);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);

	ui->expandFrame->hide();
}

QString ChannelsCommentsItem::groupName()
{
	return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

void ChannelsCommentsItem::loadComments()
{
	/* window will destroy itself! */
	GxsChannelDialog *channelDialog = dynamic_cast<GxsChannelDialog*>(MainWindow::getPage(MainWindow::Channels));

	if (!channelDialog)
		return ;

	MainWindow::showWindow(MainWindow::Channels);
    channelDialog->navigate(mComment.mMeta.mGroupId, mComment.mMeta.mMsgId);
}

void ChannelsCommentsItem::loadGroupData()
{
    std::cerr << "GxsChannelGroupItem::loadGroup()" << std::endl;

    mLoadingGroup = true;

    RsThread::async([this]()
    {
        // 1 - get group data

        std::vector<RsGxsChannelGroup> groups;
        const std::list<RsGxsGroupId> groupIds = { groupId() };

        if(!rsGxsChannels->getChannelsInfo(groupIds,groups))	// would be better to call channel Summaries for a single group
        {
            RsErr() << "GxsGxsChannelGroupItem::loadGroup() ERROR getting data for group " << groupId() << std::endl;
            mLoadingGroup = false;
            return;
        }

        if (groups.size() != 1)
        {
            std::cerr << "GxsGxsChannelGroupItem::loadGroup() Wrong number of Items for group " << groupId() ;
            std::cerr << std::endl;
            mLoadingGroup = false;
            return;
        }
        RsGxsChannelGroup group(groups[0]);

        RsQThreadUtils::postToObject( [group,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            mGroupMeta = group.mMeta;
            mLoadingGroup = false;

            update();	// this triggers a paintEvent if needed.

        }, this );
    });
}
void ChannelsCommentsItem::loadMessageData()
{
#ifdef DEBUG_ITEM
    std::cerr << "ChannelsCommentsItem::loadCommentData()";
    std::cerr << std::endl;
#endif
    mLoadingMessage = true;

    RsThread::async([this]()
    {
        // 1 - get message and comment data

        std::vector<RsGxsChannelPost> posts;
        std::vector<RsGxsComment> comments;
        std::vector<RsGxsVote> votes;

        if(! rsGxsChannels->getChannelContent( groupId(), std::set<RsGxsMessageId>( { mThreadId } ),posts,comments,votes))
        {
            RsErr() << "GxsGxsChannelGroupItem::loadMessage() ERROR getting data" << std::endl;
            mLoadingMessage = false;
            return;
        }

        // now that everything is in place, update the UI

        RsQThreadUtils::postToObject( [posts,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            if(posts.size()!=1)	// the original post cannot be found. Removing the comment item.
            {
                mLoadingMessage = false;
                removeItem();
                return;
            }

            mPost = posts[0];
            mLoadingMessage = false;

            update();

        }, this );
    });
}
void ChannelsCommentsItem::loadCommentData()
{
#ifdef DEBUG_ITEM
    std::cerr << "ChannelsCommentsItem::loadCommentData()";
	std::cerr << std::endl;
#endif
    mLoadingComment = true;

    RsThread::async([this]()
	{
        // 1 - get message and comment data

		std::vector<RsGxsChannelPost> posts;
		std::vector<RsGxsComment> comments;
		std::vector<RsGxsVote> votes;

        if(! rsGxsChannels->getChannelContent( groupId(), std::set<RsGxsMessageId>( { messageId(),mThreadId } ),posts,comments,votes))
		{
            RsErr() << "GxsGxsChannelGroupItem::loadComment() ERROR getting data" << std::endl;
            mLoadingComment = false;
            return;
		}
        if(comments.size()!=1)
        {
            mLoadingComment = false;
            return;
        }

        // now that everything is in place, update the UI

        RsQThreadUtils::postToObject( [comments,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            mComment = comments[0];
            mLoadingComment = false;

            update();

        }, this );
    });
}

void ChannelsCommentsItem::fill(bool missing_post)
{
#ifdef DEBUG_ITEM
	std::cerr << "ChannelsCommentsItem::fill()";
	std::cerr << std::endl;
#endif

	if (!mIsHome)
	{
        if (mCloseOnRead && !IS_MSG_NEW(mComment.mMeta.mMsgStatus)) {
			removeItem();
		}

		//RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_CHANNEL, mPost.mMeta.mGroupId, groupName());
		//title += link.toHtml();
		//ui->titleLabel->setText(title);

        RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_CHANNEL, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, messageName());

        if(missing_post)
            ui->subjectLabel->setText("[" + QObject::tr("Missing channel post")+"]");
        else
            ui->subjectLabel->setText(msgLink.toHtml());

		ui->readButton->hide();

        if (IS_MSG_NEW(mComment.mMeta.mMsgStatus)) {
			mCloseOnRead = true;
		}
	}
	else
	{
        if(mPost.mMeta.mMsgId.isNull())
            ui->subjectLabel->setText("[" + QObject::tr("Missing channel post")+"]");
        else
            ui->subjectLabel->setText(RsStringUtil::CopyLines(QString::fromUtf8(mPost.mMsg.c_str()), 2)) ;

		/* disable buttons: deletion facility not enabled with cache services yet */
		ui->clearButton->setEnabled(false);
		ui->clearButton->hide();
		ui->readAndClearButton->hide();
		ui->copyLinkButton->show();
		//ui->titleLabel->hide();

		if (IS_GROUP_SUBSCRIBED(mGroupMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags))
		{
			ui->readButton->setVisible(true);

            setReadStatus(IS_MSG_NEW(mComment.mMeta.mMsgStatus), IS_MSG_UNREAD(mComment.mMeta.mMsgStatus) || IS_MSG_NEW(mComment.mMeta.mMsgStatus));
		} 
		else 
		{
			ui->readButton->setVisible(false);
		}

		mCloseOnRead = false;
	}
    uint32_t autorized_lines = (int)floor( (ui->avatarLabel->height() - ui->button_HL->sizeHint().height())
                       / QFontMetricsF(ui->subjectLabel->font()).height());

    ui->commLabel->setText(RsHtml().formatText(NULL, RsStringUtil::CopyLines(QString::fromUtf8(mComment.mComment.c_str()), autorized_lines), RSHTML_FORMATTEXT_EMBED_LINKS));
    ui->nameLabel->setId(mComment.mMeta.mAuthorId);
    ui->datetimeLabel->setText(DateTime::formatLongDateTime(mComment.mMeta.mPublishTs));

    RsIdentityDetails idDetails ;
    rsIdentity->getIdDetails(mComment.mMeta.mAuthorId,idDetails);
    QPixmap pixmap ;

    if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
        pixmap = GxsIdDetails::makeDefaultIcon(mComment.mMeta.mAuthorId,GxsIdDetails::LARGE);
    ui->avatarLabel->setPixmap(pixmap);

}

QString ChannelsCommentsItem::messageName()
{
    return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void ChannelsCommentsItem::setReadStatus(bool isNew, bool isUnread)
{
	if (isNew)
        mComment.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_NEW;
	else
        mComment.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_NEW;

	if (isUnread)
	{
        mComment.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(true);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
        mComment.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(false);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	ui->feedFrame->setProperty("new", isNew);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);
}

void ChannelsCommentsItem::doExpand(bool open)
{
	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		ui->expandFrame->show();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		readToggled(false);
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
	}
}

void ChannelsCommentsItem::expandFill(bool first)
{
	GxsFeedItem::expandFill(first);
}

void ChannelsCommentsItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

/*********** SPECIFIC FUNCTIONS ***********************/

void ChannelsCommentsItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChannelsCommentsItem::readAndClearItem()";
	std::cerr << std::endl;
#endif
	readToggled(false);
	removeItem();
}

void ChannelsCommentsItem::unsubscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChannelsCommentsItem::unsubscribeChannel()";
	std::cerr << std::endl;
#endif

	unsubscribe();
}

void ChannelsCommentsItem::readToggled(bool /*checked*/)
{
	mCloseOnRead = false;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

    rsGxsChannels->setCommentReadStatus(msgPair, isUnread());
}

void ChannelsCommentsItem::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
    msgId.first = mComment.mMeta.mGroupId;
    msgId.second = mComment.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void ChannelsCommentsItem::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
    msgId.first = mComment.mMeta.mGroupId;
    msgId.second = mComment.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}
