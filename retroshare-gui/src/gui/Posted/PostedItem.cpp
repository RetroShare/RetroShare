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

#include <retroshare/rsposted.h>

#include <chrono>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

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

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsPosted->setMessageReadStatus(token, msgPair, !checked);

	setReadStatus(false, checked);
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
	setup();
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    BasePostedItem(feedHolder, feedId, groupId, post_id, isHome, autoUpdate)
{
	setup();
    loadGroup();
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

        ui->thumbnailLabel->setPixmap( FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default.png"));
		ui->fromLabel->setId(mPost.mMeta.mAuthorId);
		ui->titleLabel->setText(tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mPost.mMeta.mAuthorId.toStdString()))) ;
		QDateTime qtime;
		qtime.setTime_t(mPost.mMeta.mPublishTs);
		QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
		ui->dateLabel->setText(timestamp);
	} else {
		RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_POSTED, mGroupMeta.mGroupId, groupName());
		ui->nameLabel->setText(link.toHtml());

		QPixmap sqpixmap2 = FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default.png");

		mInFill = true;
		int desired_height = 1.5*(ui->voteDownButton->height() + ui->voteUpButton->height() + ui->scoreLabel->height());
		int desired_width =  sqpixmap2.width()*desired_height/(float)sqpixmap2.height();

		QDateTime qtime;
		qtime.setTime_t(mPost.mMeta.mPublishTs);
		QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
		QString timestamp2 = misc::timeRelativeToNow(mPost.mMeta.mPublishTs);
		ui->dateLabel->setText(timestamp2);
		ui->dateLabel->setToolTip(timestamp);

		ui->fromLabel->setId(mPost.mMeta.mAuthorId);

		// Use QUrl to check/parse our URL
		// The only combination that seems to work: load as EncodedUrl, extract toEncoded().
		QByteArray urlarray(mPost.mLink.c_str());
		QUrl url = QUrl::fromEncoded(urlarray.trimmed());
		QString urlstr = "Invalid Link";
		QString sitestr = "Invalid Link";

		bool urlOkay = url.isValid();
		if (urlOkay)
		{
			QString scheme = url.scheme();
			if ((scheme != "https")
				&& (scheme != "http")
				&& (scheme != "ftp")
				&& (scheme != "retroshare"))
			{
				urlOkay = false;
				sitestr = "Invalid Link Scheme";
			}
		}

		if (urlOkay)
		{
			urlstr =  QString("<a href=\"");
			urlstr += QString(url.toEncoded());
			urlstr += QString("\" ><span style=\" text-decoration: underline; color:#2255AA;\"> ");
			urlstr += messageName();
			urlstr += QString(" </span></a>");

			QString siteurl = url.toEncoded();
			sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#0079d3;\"> %2 </span></a>").arg(siteurl).arg(siteurl);

			ui->titleLabel->setText(urlstr);
		}else
		{
			ui->titleLabel->setText(messageName());

		}

		if (urlarray.isEmpty())
		{
			ui->siteLabel->hide();
		}

		ui->siteLabel->setText(sitestr);

		if(mPost.mImage.mData != NULL)
		{
			QPixmap pixmap;
			GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
			// Wiping data - as its been passed to thumbnail.

			QPixmap sqpixmap = pixmap.scaled(desired_width,desired_height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			ui->thumbnailLabel->setPixmap(sqpixmap);
			ui->thumbnailLabel->setToolTip(tr("Click to view Picture"));

			QPixmap scaledpixmap;
			if(pixmap.width() > 800){
				QPixmap scaledpixmap = pixmap.scaledToWidth(800, Qt::SmoothTransformation);
				ui->pictureLabel->setPixmap(scaledpixmap);
			}else{
				ui->pictureLabel->setPixmap(pixmap);
			}
		}
		else if (urlOkay && (mPost.mImage.mData == NULL))
		{
			ui->expandButton->setDisabled(true);
			ui->thumbnailLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(LINK_IMAGE));
		}
		else
		{
			ui->expandButton->setDisabled(true);
			ui->thumbnailLabel->setPixmap(sqpixmap2);
		}
	}


	//QString score = "Hot" + QString::number(post.mHotScore);
	//score += " Top" + QString::number(post.mTopScore);
	//score += " New" + QString::number(post.mNewScore);

	QString score = QString::number(mPost.mTopScore);

	ui->scoreLabel->setText(score);

	// FIX THIS UP LATER.
	ui->notes->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

	QTextDocument doc;
	doc.setHtml(ui->notes->text());

	if(doc.toPlainText().trimmed().isEmpty())
		ui->notesButton->hide();
	// differences between Feed or Top of Comment.
	if (mFeedHolder)
	{
		// feed.
		//frame_comment->show();
		ui->commentButton->show();

		if (mPost.mComments)
		{
			QString commentText = QString::number(mPost.mComments);
			commentText += " ";
			commentText += tr("Comments");
			ui->commentButton->setText(commentText);
		}
		else
		{
			ui->commentButton->setText(tr("Comment"));
		}

		setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
	}
	else
	{
		// no feed.
		//frame_comment->hide();
		ui->commentButton->hide();

		ui->readButton->hide();
		ui->newLabel->hide();
	}

	if (mIsHome)
	{
		ui->clearButton->hide();
		ui->readAndClearButton->hide();
		ui->nameLabel->hide();
	}
	else
	{
		ui->clearButton->show();
		ui->readAndClearButton->show();
		ui->nameLabel->show();
	}

	// disable voting buttons - if they have already voted.
	if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		ui->voteUpButton->setEnabled(false);
		ui->voteDownButton->setEnabled(false);
	}

#if 0
	uint32_t up, down, nComments;

	bool ok = rsPosted->retrieveScores(mPost.mMeta.mServiceString, up, down, nComments);

	if(ok)
	{
		int32_t vote = up - down;
		scoreLabel->setText(QString::number(vote));

		numCommentsLabel->setText("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px;"
								  "margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span"
								  "style=\" font-size:10pt; font-weight:600;\">#</span><span "
								  "style=\" font-size:8pt; font-weight:600;\"> Comments:  "
								  + QString::number(nComments) + "</span></p>");
	}
#endif

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

	ui->newLabel->setVisible(isNew);

	ui->mainFrame->setProperty("new", isNew);
	ui->mainFrame->style()->unpolish(ui->mainFrame);
	ui->mainFrame->style()->polish(  ui->mainFrame);
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
