/*******************************************************************************
 * gui/feeds/BoardsPostItem.cpp                                            *
 *                                                                             *
 * Copyright (c) 2012, Robert Fernie   <retroshare.project@gmail.com>          *
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
#include "gui/Posted/PhotoView.h"
#include "rshare.h"
#include "BoardsPostItem.h"
#include "ui_BoardsPostItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/stringutil.h"

#include <iostream>
#include <cmath>

/****
 * #define DEBUG_ITEM 1
 ****/

BoardsPostItem::BoardsPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate,const std::set<RsGxsMessageId>& older_versions) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsPosted, autoUpdate) // this one should be in GxsFeedItem
{
	QVector<RsGxsMessageId> v;
	//bool self = false;

	for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
		v.push_back(*it) ;

	if(older_versions.find(messageId) == older_versions.end())
		v.push_back(messageId);

    mLoadingStatus = LOADING_STATUS_NO_DATA;
    mLoadingMessage = false;
    mLoadingGroup = false;

    setMessageVersions(v) ;
	setup();
}

void BoardsPostItem::paintEvent(QPaintEvent *e)
{
	/* This method employs a trick to trigger a deferred loading. The post and group is requested only
	 * when actually displayed on the screen. */

    if(mLoadingStatus != LOADING_STATUS_FILLED && !mGroupMeta.mGroupId.isNull() && !mPost.mMeta.mMsgId.isNull() )
        mLoadingStatus = LOADING_STATUS_HAS_DATA;

    if(mGroupMeta.mGroupId.isNull() && !mLoadingGroup)
        requestGroup();

    if(mPost.mMeta.mMsgId.isNull() && !mLoadingMessage)
        requestMessage();

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

BoardsPostItem::~BoardsPostItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

    while( (mLoadingGroup || mLoadingMessage)
           && std::chrono::steady_clock::now() < timeout)
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting for "
                << (mLoadingGroup ? "Group " : "")
                << (mLoadingMessage ? "Message " : "")
                << "loading." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

	delete(ui);
}

bool BoardsPostItem::isUnread() const
{
	return IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) ;
}

void BoardsPostItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */

    ui = new Ui::BoardsPostItem;
	ui->setupUi(this);

    // Manually set icons to allow to use clever resource sharing that is missing in Qt for Icons loaded from Qt resource file.
    // This is particularly important here because a channel may contain many posts, so duplicating the QImages here is deadly for the
    // memory.

    ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default-video.png"));
    //ui->warn_image_label->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/status_unknown.png"));
    ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
    ui->voteUpButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_up.png"));
    ui->voteDownButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_down.png"));
    ui->downloadButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/download.png"));
    ui->playButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/play.png"));
    ui->commentButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/comment.png"));
    //ui->editButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"));
    ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
    ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
    ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
    ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

	setAttribute(Qt::WA_DeleteOnClose, true);

	mCloseOnRead = false;

	/* clear ui */
	ui->titleLabel->setText(tr("Loading..."));
	ui->datetimelabel->clear();
    ui->filelabel->hide();
    ui->newCommentLabel->hide();
    ui->commLabel->hide();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));
    connect(ui->logoLabel, SIGNAL(clicked()), this, SLOT(viewPicture()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribeChannel()));

	connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(download()));
	// HACK FOR NOW.
    ui->commentButton->hide();// hidden until properly enabled.
//	connect(ui->commentButton, SIGNAL(clicked()), this, SLOT(loadComments()));

	connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play(void)));
    //connect(ui->editButton, SIGNAL(clicked()), this, SLOT(edit(void)));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));

	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

    // hide voting buttons, backend is not implemented yet
    ui->voteUpButton->hide();
    ui->voteDownButton->hide();

	ui->scoreLabel->hide();

	// hide unsubscribe button not necessary
	ui->unsubscribeButton->hide();

	ui->downloadButton->hide();
	ui->playButton->hide();
    //ui->warn_image_label->hide();
    //ui->warning_label->hide();

	ui->titleLabel->setMinimumWidth(100);
	//ui->subjectLabel->setMinimumWidth(100);
    //ui->warning_label->setMinimumWidth(100);

	ui->feedFrame->setProperty("new", false);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);

	ui->expandFrame->hide();
}

QString BoardsPostItem::groupName()
{
	return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

void BoardsPostItem::loadGroup()
{
    std::cerr << "BoardsGroupItem::loadGroup()" << std::endl;

    mLoadingGroup = true;

	RsThread::async([this]()
	{
		// 1 - get group data

        std::vector<RsPostedGroup> groups;
		const std::list<RsGxsGroupId> groupIds = { groupId() };

        if(!rsPosted->getBoardsInfo(groupIds,groups))	// would be better to call channel Summaries for a single group
		{
            RsErr() << "BoardsPostItem::loadGroup() ERROR getting data for group " << groupId() << std::endl;
            mLoadingGroup = false;
            return;
		}

		if (groups.size() != 1)
		{
            std::cerr << "BoardsPostItem::loadGroup() Wrong number of Items for group " << groupId() ;
			std::cerr << std::endl;
            mLoadingGroup = false;
            return;
		}
        RsPostedGroup group(groups[0]);

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
void BoardsPostItem::loadMessage()
{
#ifdef DEBUG_ITEM
    std::cerr << "BoardsPostItem::loadMessage()";
	std::cerr << std::endl;
#endif
    mLoadingMessage = true;

	RsThread::async([this]()
	{
		// 1 - get group data

        std::vector<RsPostedPost> posts;
		std::vector<RsGxsComment> comments;
		std::vector<RsGxsVote> votes;

        if(! rsPosted->getBoardContent( groupId(), std::set<RsGxsMessageId>( { messageId() } ),posts,comments,votes))
		{
            RsErr() << "BoardsPostedItem::loadMessage() ERROR getting data" << std::endl;
            mLoadingMessage = false;
            return;
		}

		if (posts.size() == 1)
		{
#ifdef DEBUG_ITEM
			std::cerr << (void*)this << ": Obtained post, with msgId = " << posts[0].mMeta.mMsgId << std::endl;
#endif
            const RsPostedPost& post(posts[0]);

            RsQThreadUtils::postToObject( [post,this]()
            {
                mPost = post;
                mLoadingMessage = false;
                update();	// this triggers a paintEvent if needed.

            }, this );
		}
		else
		{
#ifdef DEBUG_ITEM
            std::cerr << "BoardsPostItem::loadMessage() Wrong number of Items. Remove It.";
			std::cerr << std::endl;
#endif

            RsQThreadUtils::postToObject( [this]()
            {
                removeItem();
                mLoadingMessage = false;
                update();	// this triggers a paintEvent if needed.
            }, this );
		}
	});
}

void BoardsPostItem::fill()
{
#ifdef DEBUG_ITEM
    std::cerr << "BoardsPostItem::fill()";
	std::cerr << std::endl;
#endif

	QString title;
	QString msgText;
	//float f = QFontMetricsF(font()).height()/14.0 ;

    ui->logoLabel->setEnableZoom(false);
    int desired_height = QFontMetricsF(font()).height() * 8;
	ui->logoLabel->setFixedSize(4/3.0*desired_height,desired_height);

    if(mPost.mImage.mData != NULL)
	{
		QPixmap thumbnail;
        GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, thumbnail,GxsIdDetails::ORIGINAL);
		// Wiping data - as its been passed to thumbnail.

		ui->logoLabel->setPicture(thumbnail);
	}
	else
		ui->logoLabel->setPicture( FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default-video.png") );

    //if( !IS_GROUP_PUBLISHER(mGroupMeta.mSubscribeFlags) )
    ui->editButton->hide() ;	// never show this button. Feeds are not the place to edit posts.

    if(mPost.mNotes.empty())
        ui->expandButton->hide() ;	// never show this button.

    if (mCloseOnRead && !IS_MSG_NEW(mPost.mMeta.mMsgStatus)) {
        removeItem();
    }

    title = tr("Board") + ": ";
    RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_POSTED, mPost.mMeta.mGroupId, groupName());
    title += link.toHtml();
    ui->titleLabel->setText(title);

    msgText = tr("Post") + ": ";
    RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, messageName());
    msgText += msgLink.toHtml();
    msgText += "  By: ";

    RsIdentityDetails detail;
    rsIdentity->getIdDetails(mPost.mMeta.mAuthorId,detail);
    msgText += GxsIdDetails::getName(detail);

    ui->subjectLabel->setText(msgText);

    std::cerr << "Copying 1 line from \"" << mPost.mNotes << "\"" << std::endl;
    //ui->newCommentLabel->setText(RsStringUtil::CopyLines(QString::fromUtf8(mPost.mNotes.c_str()), 1)) ;
    //ui->newCommentLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), /* RSHTML_FORMATTEXT_EMBED_SMILEYS |*/ RSHTML_FORMATTEXT_EMBED_LINKS));

    if (IS_GROUP_SUBSCRIBED(mGroupMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags))
    {
        ui->unsubscribeButton->setEnabled(true);
    }
    else
    {
        ui->unsubscribeButton->setEnabled(false);
    }
    ui->readButton->hide();
    ui->newLabel->hide();
    ui->copyLinkButton->hide();

    if (IS_MSG_NEW(mPost.mMeta.mMsgStatus))
        mCloseOnRead = true;
	
	// differences between Feed or Top of Comment.
    if(mFeedHolder)
    {
        if (mIsHome) {
            ui->commentButton->show();
        } else if (ui->commentButton->icon().isNull()){
            //Icon is seted if a comment received.
            ui->commentButton->hide();
        }
    }
    else
    {
        ui->commentButton->hide();
    }

	// disable voting buttons - if they have already voted.
	/*if (post.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		voteUpButton->setEnabled(false);
		voteDownButton->setEnabled(false);
	}*/

	{
		QTextDocument doc;
        doc.setHtml( QString::fromUtf8(mPost.mNotes.c_str()) );

		ui->msgFrame->setVisible(doc.toPlainText().length() > 0);
	}

	ui->datetimelabel->setText(DateTime::formatLongDateTime(mPost.mMeta.mPublishTs));
}

QString BoardsPostItem::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void BoardsPostItem::viewPicture()
{
  if(mPost.mImage.mData == NULL) {
    return;
  }

  QString timestamp = misc::timeRelativeToNow(mPost.mMeta.mPublishTs);
  QPixmap pixmap;
  GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
  RsGxsId authorID = mPost.mMeta.mAuthorId;

  PhotoView *PView = new PhotoView(this);

  PView->setPixmap(pixmap);
  PView->setTitle(messageName());
  PView->setName(authorID);
  PView->setTime(timestamp);
  PView->setGroupId(groupId());
  PView->setMessageId(messageId());

  PView->show();

  /* window will destroy itself! */
}
void BoardsPostItem::setReadStatus(bool isNew, bool isUnread)
{
#ifdef TODO
	if (isNew)
		mPost.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_NEW;
	else
		mPost.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_NEW;

	if (isUnread)
	{
		mPost.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(true);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		mPost.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(false);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	ui->newLabel->setVisible(isNew);

	ui->feedFrame->setProperty("new", isNew);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
    ui->feedFrame->style()->polish(  ui->feedFrame);
#endif
}
void BoardsPostItem::toggle()
{
    expand(ui->expandFrame->isHidden());
}

/*********** SPECIFIC FUNCTIONS ***********************/

void BoardsPostItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
    std::cerr << "BoardsPostItem::readAndClearItem()";
	std::cerr << std::endl;
#endif
	readToggled(false);
	removeItem();
}

void BoardsPostItem::readToggled(bool /*checked*/)
{
	mCloseOnRead = false;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

    rsPosted->setPostReadStatus(msgPair, isUnread());
}

void BoardsPostItem::fillExpandFrame()
{
    ui->msgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), /* RSHTML_FORMATTEXT_EMBED_SMILEYS |*/ RSHTML_FORMATTEXT_EMBED_LINKS));
}

void BoardsPostItem::doExpand(bool open)
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

void BoardsPostItem::expandFill(bool first)
{
    GxsFeedItem::expandFill(first);

    if (first) {
        fillExpandFrame();
    }
}

