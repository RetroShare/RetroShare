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

#include "rshare.h"
#include "PostedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "util/misc.h"

#include "ui_PostedItem.h"

#include <retroshare/rsposted.h>

#include <iostream>

/** Constructor */

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsPosted, autoUpdate)
{
	setup();

	requestGroup();
	requestMessage();
	requestComment();
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedGroup &group, const RsPostedPost &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsPosted, autoUpdate)
{
	setup();
	
	mMessageId = post.mMeta.mMsgId;


	setGroup(group, false);
	setPost(post);
	requestComment();
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedPost &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsPosted, autoUpdate)
{
	setup();

	requestGroup();
	setPost(post);
	requestComment();
}

PostedItem::~PostedItem()
{
	delete(ui);
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
	
	QAction *CopyLinkAction = new QAction(QIcon(""),tr("Copy RetroShare Link"), this);
	connect(CopyLinkAction, SIGNAL(triggered()), this, SLOT(copyMessageLink()));
	
	
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
	ui->shareButton->setMenu(menu);

	ui->clearButton->hide();
	ui->readAndClearButton->hide();
}

bool PostedItem::setGroup(const RsPostedGroup &group, bool doFill)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "PostedItem::setGroup() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;

	if (doFill) {
		fill();
	}

	return true;
}

bool PostedItem::setPost(const RsPostedPost &post, bool doFill)
{
	if (groupId() != post.mMeta.mGroupId || messageId() != post.mMeta.mMsgId) {
		std::cerr << "PostedItem::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mPost = post;

	if (doFill) {
		fill();
	}

	return true;
}

void PostedItem::loadGroup(const uint32_t &token)
{
	std::vector<RsPostedGroup> groups;
	if (!rsPosted->getGroupData(token, groups))
	{
		std::cerr << "PostedItem::loadGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "PostedItem::loadGroup() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setGroup(groups[0]);
}

void PostedItem::loadMessage(const uint32_t &token)
{
	std::vector<RsPostedPost> posts;
	std::vector<RsGxsComment> cmts;
	if (!rsPosted->getPostData(token, posts, cmts))
	{
		std::cerr << "GxsChannelPostItem::loadMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (posts.size() == 1)
	{
		setPost(posts[0]);
	}
	else if (cmts.size() == 1)
	{
		RsGxsComment cmt = cmts[0];

		ui->newCommentLabel->show();
		ui->commLabel->show();
		ui->commLabel->setText(QString::fromUtf8(cmt.mComment.c_str()));

		//Change this item to be uploaded with thread element.
		setMessageId(cmt.mMeta.mThreadId);
		requestMessage();
	}
	else
	{
		std::cerr << "GxsChannelPostItem::loadMessage() Wrong number of Items. Remove It.";
		std::cerr << std::endl;
		removeItem();
		return;
	}
}

void PostedItem::loadComment(const uint32_t &token)
{
	std::vector<RsGxsComment> cmts;
	if (!rsPosted->getRelatedComments(token, cmts))
	{
		std::cerr << "GxsChannelPostItem::loadComment() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	size_t comNb = cmts.size();
	QString sComButText = tr("Comment");
	if (comNb == 1) {
		sComButText = sComButText.append("(1)");
	} else if (comNb > 1) {
		sComButText = tr("Comments").append(" (%1)").arg(comNb);
	}
	ui->commentButton->setText(sComButText);
}

void PostedItem::fill()
{
	if (isLoading()) {
		/* Wait for all requests */
		return;
	}

	QPixmap sqpixmap2 = QPixmap(":/images/thumb-default.png");

	mInFill = true;
	int desired_height = 1.5*(ui->voteDownButton->height() + ui->voteUpButton->height() + ui->scoreLabel->height());
	int desired_width =  sqpixmap2.width()*desired_height/(float)sqpixmap2.height();

	if(mPost.mImage.mData != NULL)
	{
		QPixmap pixmap;
		pixmap.loadFromData(mPost.mImage.mData, mPost.mImage.mSize, "PNG");
		// Wiping data - as its been passed to thumbnail.
		
		QPixmap sqpixmap = pixmap.scaled(desired_width,desired_height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		ui->thumbnailLabel->setPixmap(sqpixmap);
		ui->pictureLabel->setPixmap(pixmap);
	}
	else
	{
		//ui->thumbnailLabel->setFixedSize(desired_width,desired_height);
		ui->expandButton->setDisabled(true);
	}

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

		QString siteurl = url.scheme() + "://" + url.host();
		sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#0079d3;\"> %2 </span></a>").arg(siteurl).arg(siteurl);
		
		ui->titleLabel->setText(urlstr);
	}else
	{
		ui->titleLabel->setText(messageName());

	}

	ui->siteLabel->setText(sitestr);

	//QString score = "Hot" + QString::number(post.mHotScore);
	//score += " Top" + QString::number(post.mTopScore); 
	//score += " New" + QString::number(post.mNewScore);

	QString score = QString::number(mPost.mTopScore);

	ui->scoreLabel->setText(score);

	// FIX THIS UP LATER.
	ui->notes->setText(QString::fromUtf8(mPost.mNotes.c_str()));
	if(ui->notes->text().isEmpty())
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
	}
	else
	{
		ui->clearButton->show();
		ui->readAndClearButton->show();
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

const RsPostedPost &PostedItem::getPost() const
{
	return mPost;
}

RsPostedPost &PostedItem::post()
{
	return mPost;
}

QString PostedItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

QString PostedItem::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
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

void PostedItem::loadComments()
{
	std::cerr << "PostedItem::loadComments()";
	std::cerr << std::endl;

	if (mFeedHolder)
	{
		QString title = QString::fromUtf8(mPost.mMeta.mMsgName.c_str());

#warning (csoler) Posted item versions not handled yet. When it is the case, start here.

        QVector<RsGxsMessageId> post_versions ;
        post_versions.push_back(mPost.mMeta.mMsgId) ;

		mFeedHolder->openComments(0, mPost.mMeta.mGroupId, post_versions,mPost.mMeta.mMsgId, title);
	}
}

void PostedItem::setReadStatus(bool isNew, bool isUnread)
{
	if (isUnread)
	{
		ui->readButton->setChecked(true);
		ui->readButton->setIcon(QIcon(":/images/message-state-unread.png"));
	}
	else
	{
		ui->readButton->setChecked(false);
		ui->readButton->setIcon(QIcon(":/images/message-state-read.png"));
	}

	ui->newLabel->setVisible(isNew);

	ui->mainFrame->setProperty("new", isNew);
	ui->mainFrame->style()->unpolish(ui->mainFrame);
	ui->mainFrame->style()->polish(  ui->mainFrame);
}

void PostedItem::readToggled(bool checked)
{
	if (mInFill) {
		return;
	}

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsPosted->setMessageReadStatus(token, msgPair, !checked);

	setReadStatus(false, checked);
}

void PostedItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "PostedItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	readToggled(false);
	removeItem();
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
		ui->expandButton->setIcon(QIcon(QString(":/images/decrease.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->frame_picture->hide();
		ui->expandButton->setIcon(QIcon(QString(":/images/expand.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

}

void PostedItem::copyMessageLink()
{
	if (groupId().isNull() || mMessageId.isNull()) {
		return;
	}

	RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, groupId(), mMessageId, messageName());

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
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

}
