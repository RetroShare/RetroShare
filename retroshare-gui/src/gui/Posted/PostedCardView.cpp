/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCardView.cpp                            *
 *                                                                             *
 * Copyright (C) 2019  Retroshare Team       <retroshare.project@gmail.com>    *
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
#include "PostedCardView.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"

#include "ui_PostedCardView.h"

#include <retroshare/rsposted.h>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

/** Constructor */

PostedCardView::PostedCardView(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData &group_meta, const RsGxsMessageId &post_id, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, group_meta.mGroupId, post_id, isHome, rsPosted, autoUpdate),
  	mGroupMeta(group_meta)
{
    mPost.mMeta.mMsgId = post_id;
    mPost.mMeta.mGroupId = mGroupMeta.mGroupId;

    mLoaded = false;
	setup();
}

PostedCardView::PostedCardView(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, post_id, isHome, rsPosted, autoUpdate)
{
    mPost.mMeta.mMsgId = post_id;
    mLoaded = false;

	setup();

    loadGroup();
}

PostedCardView::~PostedCardView()
{
	delete(ui);
}
void PostedCardView::paintEvent(QPaintEvent *e)
{
	/* This method employs a trick to trigger a deferred loading. The post and group is requested only
	 * when actually displayed on the screen. */

	if(!mLoaded)
	{
		mLoaded = true ;

		fill();
		requestMessage();
		requestComment();
	}

	GxsFeedItem::paintEvent(e) ;
}

void PostedCardView::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::PostedCardView;
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
	
	QAction *CopyLinkAction = new QAction(QIcon(""),tr("Copy RetroShare Link"), this);
	connect(CopyLinkAction, SIGNAL(triggered()), this, SLOT(copyMessageLink()));
	
	
	int S = QFontMetricsF(font()).height() ;
	
	ui->voteUpButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->voteDownButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->commentButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->readButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->shareButton->setIconSize(QSize(S*1.5,S*1.5));
	
	QMenu *menu = new QMenu();
	menu->addAction(CopyLinkAction);
	ui->shareButton->setMenu(menu);

	ui->clearButton->hide();
	ui->readAndClearButton->hide();
}

bool PostedCardView::setPost(const RsPostedPost &post, bool doFill)
{
	if (groupId() != post.mMeta.mGroupId || messageId() != post.mMeta.mMsgId) {
		std::cerr << "PostedCardView::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mPost = post;

	if (doFill) {
		fill();
	}

	return true;
}

void PostedCardView::loadGroup()
{
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
			return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsPostedGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
			return;
		}
		RsPostedGroup group(groups[0]);

		RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            mGroupMeta = group.mMeta;

		}, this );
	});
}

void PostedCardView::loadMessage()
{
	RsThread::async([this]()
	{
		// 1 - get group data

		std::vector<RsPostedPost> posts;
		std::vector<RsGxsComment> comments;

		if(! rsPosted->getBoardContent( groupId(), std::set<RsGxsMessageId>( { messageId() } ),posts,comments))
		{
			RsErr() << "PostedItem::loadMessage() ERROR getting data" << std::endl;
			return;
		}

		if (posts.size() == 1)
		{
			std::cerr << (void*)this << ": Obtained post, with msgId = " << posts[0].mMeta.mMsgId << std::endl;
            const RsPostedPost& post(posts[0]);

			RsQThreadUtils::postToObject( [post,this]() { setPost(post,true);  }, this );
		}
		else if(comments.size() == 1)
		{
			const RsGxsComment& cmt = comments[0];
			std::cerr << (void*)this << ": Obtained comment, setting messageId to threadID = " << cmt.mMeta.mThreadId << std::endl;

			RsQThreadUtils::postToObject( [cmt,this]()
			{
				//Change this item to be uploaded with thread element.
				setMessageId(cmt.mMeta.mThreadId);
				requestMessage();

			}, this );

		}
		else
		{
			std::cerr << "GxsChannelPostItem::loadMessage() Wrong number of Items. Remove It.";
			std::cerr << std::endl;

			RsQThreadUtils::postToObject( [this]() {  removeItem(); }, this );
		}
	});

}

void PostedCardView::loadComment()
{
	RsThread::async([this]()
	{
		// 1 - get group data

        std::set<RsGxsMessageId> msgIds;

        for(auto MsgId: messageVersions())
            msgIds.insert(MsgId);

		std::vector<RsPostedPost> posts;
		std::vector<RsGxsComment> comments;

		if(! rsPosted->getBoardContent( groupId(),msgIds,posts,comments))
		{
			RsErr() << "PostedCardView::loadGroup() ERROR getting data" << std::endl;
			return;
		}

        int comNb = comments.size();

		RsQThreadUtils::postToObject( [comNb,this]()
		{
			QString sComButText = tr("Comment");
			if (comNb == 1)
				sComButText = sComButText.append("(1)");
			else if(comNb > 1)
				sComButText = tr("Comments ").append("(%1)").arg(comNb);

			ui->commentButton->setText(sComButText);

		}, this );
	});
}

void PostedCardView::fill()
{
//	if (isLoading()) {
//		/* Wait for all requests */
//		return;
//	}

	QPixmap sqpixmap2 = QPixmap(":/images/thumb-default.png");

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
		
		QPixmap scaledpixmap;
		if(pixmap.width() > 800){
			QPixmap scaledpixmap = pixmap.scaledToWidth(800, Qt::SmoothTransformation);
			ui->pictureLabel->setPixmap(scaledpixmap);
		}else{
			ui->pictureLabel->setPixmap(pixmap);
		}
	}
	else if (mPost.mImage.mData == NULL)
	{
		ui->picture_frame->hide();
	}
	else
	{
		ui->picture_frame->show();
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
		ui->notes->hide();
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

//const RsPostedPost &PostedCardView::getPost() const
//{
//	return mPost;
//}

//RsPostedPost &PostedCardView::post()
//{
//	return mPost;
//}

QString PostedCardView::groupName()
{
	return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

QString PostedCardView::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void PostedCardView::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void PostedCardView::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}

void PostedCardView::loadComments()
{
	std::cerr << "PostedCardView::loadComments()";
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

void PostedCardView::setReadStatus(bool isNew, bool isUnread)
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

void PostedCardView::readToggled(bool checked)
{
	if (mInFill) {
		return;
	}

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsPosted->setMessageReadStatus(token, msgPair, !checked);

	setReadStatus(false, checked);
}

void PostedCardView::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "PostedCardView::readAndClearItem()";
	std::cerr << std::endl;
#endif

	readToggled(false);
	removeItem();
}


void PostedCardView::doExpand(bool open)
{
	/*if (open)
	{
		
	}
	else
	{		

	}

	emit sizeChanged(this);*/

}

void PostedCardView::copyMessageLink()
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
