/*
 * Retroshare Posted Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QDateTime>

#include "rshare.h"
#include "PostedItem.h"
#include "gui/feeds/FeedHolder.h"
#include "ui_PostedItem.h"

#include <retroshare/rsposted.h>

#include <iostream>

#define COLOR_NORMAL QColor(248, 248, 248)
#define COLOR_NEW    QColor(220, 236, 253)

/** Constructor */

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsPosted, autoUpdate)
{
	setup();

	requestGroup();
	requestMessage();
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedGroup &group, const RsPostedPost &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsPosted, autoUpdate)
{
	setup();

	setGroup(group, false);
	setPost(post);
}

PostedItem::PostedItem(FeedHolder *feedHolder, uint32_t feedId, const RsPostedPost &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsPosted, autoUpdate)
{
	setup();

	requestGroup();
	setPost(post);
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

	/* general ones */
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));

	connect(ui->commentButton, SIGNAL( clicked()), this, SLOT(loadComments()));
	connect(ui->voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));

	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

	ui->clearButton->hide();
	ui->readAndClearButton->hide();

	ui->frame_notes->hide();
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
	if (!rsPosted->getPostData(token, posts))
	{
		std::cerr << "GxsChannelPostItem::loadMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (posts.size() != 1)
	{
		std::cerr << "GxsChannelPostItem::loadMessage() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setPost(posts[0]);
}

void PostedItem::fill()
{
	if (isLoading()) {
		/* Wait for all requests */
		return;
	}

	mInFill = true;

	QDateTime qtime;
	qtime.setTime_t(mPost.mMeta.mPublishTs);
	QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
	ui->dateLabel->setText(timestamp);
	ui->fromLabel->setId(mPost.mMeta.mAuthorId);

	// Use QUrl to check/parse our URL
	// The only combination that seems to work: load as EncodedUrl, extract toEncoded().
	QUrl url;
	QByteArray urlarray(mPost.mLink.c_str());
	url.setEncodedUrl(urlarray.trimmed());
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
		sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#2255AA;\"> %2 </span></a>").arg(siteurl).arg(siteurl);
	}

	ui->titleLabel->setText(urlstr);
	ui->siteLabel->setText(sitestr);

	//QString score = "Hot" + QString::number(post.mHotScore);
	//score += " Top" + QString::number(post.mTopScore); 
	//score += " New" + QString::number(post.mNewScore);

	QString score = QString::number(mPost.mTopScore);

	ui->scoreLabel->setText(score);

	// FIX THIS UP LATER.
	ui->notes->setText(QString::fromUtf8(mPost.mNotes.c_str()));
	// differences between Feed or Top of Comment.
	if (mFeedHolder)
	{
		// feed.
		ui->frame_notes->hide();
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
		if(ui->notes->text().isEmpty())
		{
			ui->frame_notes->hide();
		}
		else
		{
			ui->frame_notes->show();
		}
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

	uint32_t up, down, nComments;

#if 0
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
		mFeedHolder->openComments(0, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, title);
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

	/* unpolish widget to clear the stylesheet's palette cache */
	ui->frame->style()->unpolish(ui->frame);

	QPalette palette = ui->frame->palette();
	palette.setColor(ui->frame->backgroundRole(), isNew ? COLOR_NEW : COLOR_NORMAL); // QScrollArea
	palette.setColor(QPalette::Base, isNew ? COLOR_NEW : COLOR_NORMAL); // QTreeWidget
	ui->frame->setPalette(palette);

	ui->frame->setProperty("new", isNew);
	Rshare::refreshStyleSheet(ui->frame, false);
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
