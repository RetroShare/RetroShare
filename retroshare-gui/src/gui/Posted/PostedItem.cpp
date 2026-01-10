/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedItem.cpp                                *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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
#include <QMenu>
#include <QStyle>
#include <QTextDocument>

#include "rshare.h"
#include "PostedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "gui/MainWindow.h"
#include "gui/Identity/IdDialog.h"
#include "PhotoView.h"
#include "gui/Posted/PostedDialog.h"
#include "ui_PostedItem.h"
#include "util/DateTime.h"

#include <retroshare/rsposted.h>

#include <chrono>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

static QString formatDate(uint64_t seconds)
{
    QDateTime dt = DateTime::DateTimeFromTime_t(seconds);
    switch(Settings->getDateFormat()) {
        case RshareSettings::DateFormat_ISO:
            return dt.toString(Qt::ISODate).replace('T', ' ');
        case RshareSettings::DateFormat_Text:
            // Use the official System Long Format (Absolute date, no Today/Yesterday logic)
            return QLocale::system().toString(dt, QLocale::LongFormat);
        case RshareSettings::DateFormat_System:
        default:
            // Standard System Short Format
            return QLocale::system().toString(dt, QLocale::ShortFormat);
    }
}

/** Constructor */

//========================================================================================
//                                     BasePostedItem                                   //
//========================================================================================

BasePostedItem::BasePostedItem( FeedHolder *feedHolder, uint32_t feedId
                              , const RsGroupMetaData &group_meta, const RsGxsMessageId& post_id
                              , bool isHome, bool autoUpdate)
    : GxsFeedItem(feedHolder, feedId, group_meta.mGroupId, post_id, isHome, rsPosted, autoUpdate)
    , mInFill(false), mGroupMeta(group_meta)
    , mLoaded(false), mIsLoadingGroup(false), mIsLoadingMessage(false), mIsLoadingComment(false)
{
	mPost.mMeta.mMsgId = post_id;
	mPost.mMeta.mGroupId = mGroupMeta.mGroupId;
}

BasePostedItem::BasePostedItem( FeedHolder *feedHolder, uint32_t feedId
                              , const RsGxsGroupId &groupId, const RsGxsMessageId& post_id
                              , bool isHome, bool autoUpdate)
    : GxsFeedItem(feedHolder, feedId, groupId, post_id, isHome, rsPosted, autoUpdate)
    , mInFill(false)
    , mLoaded(false), mIsLoadingGroup(false), mIsLoadingMessage(false), mIsLoadingComment(false)
{
	mPost.mMeta.mMsgId = post_id;
}

BasePostedItem::~BasePostedItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
	while( (mIsLoadingGroup || mIsLoadingMessage || mIsLoadingComment)
	       && std::chrono::steady_clock::now() < timeout)
	{
		RsDbg() << __PRETTY_FUNCTION__ << " is Waiting "
		        << (mIsLoadingGroup ? "Group " : "")
		        << (mIsLoadingMessage ? "Message " : "")
		        << (mIsLoadingComment ? "Comment " : "")
		        << "loading finished." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void BasePostedItem::paintEvent(QPaintEvent *e)
{
	/* This method employs a trick to trigger a deferred loading. The post and group is requested only
	 * when actually displayed on the screen. */

	if(!mLoaded)
	{
		mLoaded = true ;

		requestMessage();
		requestComment();
	}

	GxsFeedItem::paintEvent(e) ;
}

bool BasePostedItem::setPost(const RsPostedPost &post, bool doFill)
{
	if (groupId() != post.mMeta.mGroupId || messageId() != post.mMeta.mMsgId) {
		std::cerr << "BasePostedItem::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mPost = post;

	if (doFill)
		fill();

	return true;
}

void BasePostedItem::loadGroup()
{
	mIsLoadingGroup = true;
	RsThread::async([this]()
	{
		// 1 - get group data

#ifdef DEBUG_FORUMS
		std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

		std::vector<RsPostedGroup> groups;
		const std::list<RsGxsGroupId> groupIds = { groupId() };

		if(!rsPosted->getBoardsInfo(groupIds,groups))
		{
			RsErr() << "GxsPostedGroupItem::loadGroup() ERROR getting data" << std::endl;
			mIsLoadingGroup = false;
			return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsPostedGroupItem::loadGroup() Wrong number of Items" << std::endl;
			mIsLoadingGroup = false;
			return;
		}
		RsPostedGroup group(groups[0]);

		RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			mGroupMeta = group.mMeta;
			mIsLoadingGroup = false;

		}, this );
	});
}

void BasePostedItem::loadMessage()
{
	mIsLoadingMessage = true;
	RsThread::async([this]()
	{
		// 1 - get group data

		std::vector<RsPostedPost> posts;
		std::vector<RsGxsComment> comments;
		std::vector<RsGxsVote> votes;

		if(! rsPosted->getBoardContent( groupId(), std::set<RsGxsMessageId>( { messageId() } ),posts,comments,votes))
		{
			RsErr() << "BasePostedItem::loadMessage() ERROR getting data" << std::endl;
			mIsLoadingMessage = false;
			return;
		}

		if (posts.size() == 1)
		{
			std::cerr << (void*)this << ": Obtained post, with msgId = " << posts[0].mMeta.mMsgId << std::endl;
			const RsPostedPost& post(posts[0]);

			RsQThreadUtils::postToObject( [post,this]() { setPost(post,true); mIsLoadingMessage = false;}, this );
		}
		else if(comments.size() == 1)
		{
			const RsGxsComment& cmt = comments[0];
			std::cerr << (void*)this << ": Obtained comment, setting messageId to threadID = " << cmt.mMeta.mThreadId << std::endl;

			RsQThreadUtils::postToObject( [cmt,this]()
			{
				setComment(cmt);

				//Change this item to be uploaded with thread element.
				setMessageId(cmt.mMeta.mThreadId);
				requestMessage();

				mIsLoadingMessage = false;
			}, this );

		}
		else
		{
			std::cerr << "GxsChannelPostItem::loadMessage() Wrong number of Items. Remove It." << std::endl;

			RsQThreadUtils::postToObject( [this]() {  removeItem(); mIsLoadingMessage = false;}, this );
		}
	});
}


void BasePostedItem::loadComment()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::loadComment()";
	std::cerr << std::endl;
#endif
	mIsLoadingComment = true;
	RsThread::async([this]()
	{
		// 1 - get group data

        std::set<RsGxsMessageId> msgIds;

        for(auto MsgId: messageVersions())
            msgIds.insert(MsgId);

		std::vector<RsPostedPost> posts;
		std::vector<RsGxsComment> comments;
		std::vector<RsGxsVote> votes;

		if(! rsPosted->getBoardContent( groupId(),msgIds,posts,comments,votes))
		{
			RsErr() << "BasePostedItem::loadGroup() ERROR getting data" << std::endl;
			mIsLoadingComment = false;
			return;
		}

		int comNb = comments.size();

		RsQThreadUtils::postToObject( [comNb,this]()
		{
			setCommentsSize(comNb);
			mIsLoadingComment = false;
		}, this );
	});
}

QString BasePostedItem::groupName()
{
	return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

QString BasePostedItem::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void BasePostedItem::loadComments()
{
	std::cerr << "BasePostedItem::loadComments()";
	std::cerr << std::endl;

	if (mFeedHolder)
	{
		/* window will destroy itself! */
		PostedDialog *postedDialog = dynamic_cast<PostedDialog*>(MainWindow::getPage(MainWindow::Posted));

		if (!postedDialog)
			return ;

		MainWindow::showWindow(MainWindow::Posted);
		postedDialog->navigate(mPost.mMeta.mGroupId, mPost.mMeta.mMsgId) ;
	}
}
void BasePostedItem::readToggled(bool checked)
{
    if (mInFill) {
        return;
    }
    setReadStatus(false, checked);

    RsThread::async([this,checked]()
    {
        RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());
        rsPosted->setPostReadStatus(msgPair, !checked);
    });
}

void BasePostedItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "BasePostedItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	readToggled(false);
	removeItem();
}
void BasePostedItem::copyMessageLink()
{
	if (groupId().isNull() || messageId().isNull()) {
		return;
	}

	RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, groupId(), messageId(), messageName());

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void BasePostedItem::viewPicture()
{
	if(mPost.mImage.mData == NULL) {
		return;
	}

	// [MODIFICATION] Replaced timeRelativeToNow with our standardized absolute formatDate.
	QString timestamp = formatDate(mPost.mMeta.mPublishTs);
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

void BasePostedItem::showAuthorInPeople()
{
	if(mPost.mMeta.mAuthorId.isNull())
	{
		std::cerr << "(EE) GxsForumThreadWidget::loadMsgData_showAuthorInPeople() ERROR Missing Message Data...";
		std::cerr << std::endl;
	}

	/* window will destroy itself! */
	IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

	if (!idDialog)
		return ;

	MainWindow::showWindow(MainWindow::People);
	idDialog->navigate(RsGxsId(mPost.mMeta.mAuthorId));
}

//========================================================================================
//                                        PostedItem                                    //
//========================================================================================

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData &group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    BasePostedItem(feedHolder, feedId, group_meta, post_id, isHome, autoUpdate)
{
    PostedItem::setup();
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    BasePostedItem(feedHolder, feedId, groupId, post_id, isHome, autoUpdate)
{
    PostedItem::setup();
    PostedItem::loadGroup();
}


void PostedItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::PostedItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mInFill = false;

	/* clear ui */
	ui->titleLabel->setText(tr("Loading"));
	ui->dateLabel->clear();
	ui->fromLabel->clear();
	ui->siteLabel->clear();
	ui->newCommentLabel->hide();
	ui->frame_picture->hide();
	ui->commLabel->hide();
	ui->frame_notes->hide();

	/* general ones */
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));

	connect(ui->commentButton, SIGNAL( clicked()), this, SLOT(loadComments()));
	connect(ui->voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT( toggle()));
	connect(ui->notesButton, SIGNAL(clicked()), this, SLOT( toggleNotes()));

	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));
	connect(ui->thumbnailLabel, SIGNAL(clicked()), this, SLOT(viewPicture()));

	QAction *CopyLinkAction = new QAction(QIcon(""),tr("Copy RetroShare Link"), this);
	connect(CopyLinkAction, SIGNAL(triggered()), this, SLOT(copyMessageLink()));

	QAction *showInPeopleAct = new QAction(QIcon(), tr("Show author in people tab"), this);
	connect(showInPeopleAct, SIGNAL(triggered()), this, SLOT(showAuthorInPeople()));

	int S = QFontMetricsF(font()).height() ;

	ui->voteUpButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->voteDownButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->commentButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->expandButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->notesButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->readButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->shareButton->setIconSize(QSize(S*1.5,S*1.5));

	QMenu *menu = new QMenu();
	menu->addAction(CopyLinkAction);
	menu->addSeparator();
	menu->addAction(showInPeopleAct);
	ui->shareButton->setMenu(menu);

	ui->clearButton->hide();
	ui->readAndClearButton->hide();
	ui->nameLabel->hide();

	//hide read & new not used
	ui->readButton->hide();
	ui->newLabel->hide();
}

void PostedItem::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void PostedItem::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}

void PostedItem::setComment(const RsGxsComment& cmt)
{
	ui->newCommentLabel->show();
	ui->commLabel->show();
	ui->commLabel->setText(QString::fromUtf8(cmt.mComment.c_str()));
}
void PostedItem::setCommentsSize(int comNb)
{
	QString sComButText = tr("Comment");
	if (comNb == 1)
		sComButText = sComButText.append("(1)");
	else if(comNb > 1)
		sComButText = tr("Comments ").append("(%1)").arg(comNb);

	ui->commentButton->setText(sComButText);
}

void PostedItem::fill()
{
	RsReputationLevel overall_reputation = rsReputations->overallReputationLevel(mPost.mMeta.mAuthorId);
	bool redacted = (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

	if(redacted) {
		ui->expandButton->setDisabled(true);
		ui->commentButton->setDisabled(true);
		ui->voteUpButton->setDisabled(true);
		ui->voteDownButton->setDisabled(true);

        ui->thumbnailLabel->setPicture( FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default.png"));
		ui->fromLabel->setId(mPost.mMeta.mAuthorId);
		ui->titleLabel->setText(tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mPost.mMeta.mAuthorId.toStdString()))) ;
		
		// [MODIFICATION] Call the helper for the ban message date label
		ui->dateLabel->setText(formatDate(mPost.mMeta.mPublishTs));
	} else {
		RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_POSTED, mGroupMeta.mGroupId, groupName());
		ui->nameLabel->setText(link.toHtml());

		mInFill = true;
        int desired_height = ui->voteDownButton->height() + ui->voteUpButton->height() + ui->scoreLabel->height();
        ui->thumbnailLabel->setFixedSize(16/9.0 * desired_height, desired_height);

		// [MODIFICATION] Unified date display using the static helper. 
        // No hardcoded "ago" logic here.
		ui->dateLabel->setText(formatDate(mPost.mMeta.mPublishTs));
		ui->dateLabel->setToolTip(QLocale::system().toString(DateTime::DateTimeFromTime_t(mPost.mMeta.mPublishTs), QLocale::ShortFormat));

		ui->fromLabel->setId(mPost.mMeta.mAuthorId);

		QByteArray urlarray(mPost.mLink.c_str());
		QUrl url = QUrl::fromEncoded(urlarray.trimmed());
		QString urlstr = "Invalid Link";
		QString sitestr = "Invalid Link"; // [FIX] Added missing sitestr declaration

		bool urlOkay = url.isValid();
		if (urlOkay && (url.scheme() == "https" || url.scheme() == "http" || url.scheme() == "ftp" || url.scheme() == "retroshare"))
		{
			urlstr =  QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#2255AA;\"> %2 </span></a>")
                             .arg(QString(url.toEncoded())).arg(messageName());
			ui->titleLabel->setText(urlstr);
			sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#0079d3;\"> %1 </span></a>").arg(QString(url.toEncoded()));
		}else
		{
			ui->titleLabel->setText(messageName());
		}

		if (urlarray.isEmpty()) ui->siteLabel->hide();
		ui->siteLabel->setText(sitestr);

		if(mPost.mImage.mData != NULL)
		{
			QPixmap pixmap;
			GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
            ui->thumbnailLabel->setPicture(pixmap);
            if(pixmap.width() > 800)
                ui->pictureLabel->setPixmap(pixmap.scaledToWidth(800, Qt::SmoothTransformation));
            else
                ui->pictureLabel->setPixmap(pixmap);
		}
		else
		{
			ui->thumbnailLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(urlOkay ? LINK_IMAGE : ":/images/thumb-default.png"));
		}
	}

	ui->scoreLabel->setText(QString::number(mPost.mTopScore));
    ui->notes->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS));

	if(mFeedHolder)
	{
		ui->commentButton->show();
		ui->commentButton->setText(mPost.mComments ? QString::number(mPost.mComments) + " " + tr("Comments") : tr("Comment"));
		setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
	}
	else
	{
		ui->commentButton->hide();
		ui->readButton->hide();
	}

	if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		ui->voteUpButton->setEnabled(false);
		ui->voteDownButton->setEnabled(false);
	}

	mInFill = false;
	emit sizeChanged(this);
}

void PostedItem::setReadStatus(bool isNew, bool isUnread)
{
	if (isUnread)
	{
		ui->readButton->setChecked(true);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		ui->readButton->setChecked(false);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	//ui->newLabel->setVisible(isNew);

	ui->feedFrame->setProperty("new", isNew);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);
}


void PostedItem::toggle()
{
	expand(ui->frame_picture->isHidden());
}

void PostedItem::doExpand(bool open)
{
	if (open)
	{
		ui->frame_picture->show();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/decrease.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->frame_picture->hide();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/expand.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

}

void PostedItem::toggleNotes()
{
	if (ui->notesButton->isChecked())
	{
		ui->frame_notes->show();
	}
	else
	{
		ui->frame_notes->hide();
	}

	emit sizeChanged(this);
}
