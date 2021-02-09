/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardsCommentsItem.cpp                        *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team     <retroshare.project@gmail.com>    *
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
#include "BoardsCommentsItem.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/DateTime.h"
#include "util/misc.h"
#include "util/stringutil.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "gui/MainWindow.h"
#include "gui/Identity/IdDialog.h"
#include "gui/Posted/PostedDialog.h"

#include "ui_BoardsCommentsItem.h"

#include <retroshare/rsposted.h>
#include <retroshare/rsidentity.h>

#include <chrono>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

/** Constructor */

//========================================================================================
//                                     BaseBoardsCommentsItem                                   //
//========================================================================================

BaseBoardsCommentsItem::BaseBoardsCommentsItem( FeedHolder *feedHolder, uint32_t feedId
                              , const RsGroupMetaData &group_meta, const RsGxsMessageId& post_id
                              , bool isHome, bool autoUpdate)
    : GxsFeedItem(feedHolder, feedId, group_meta.mGroupId, post_id, isHome, rsPosted, autoUpdate)
    , mInFill(false), mGroupMeta(group_meta)
    , mLoaded(false), mIsLoadingGroup(false), mIsLoadingMessage(false), mIsLoadingComment(false)
{
	mPost.mMeta.mMsgId = post_id;
	mPost.mMeta.mGroupId = mGroupMeta.mGroupId;
}

BaseBoardsCommentsItem::BaseBoardsCommentsItem( FeedHolder *feedHolder, uint32_t feedId
                              , const RsGxsGroupId &groupId, const RsGxsMessageId& post_id
                              , bool isHome, bool autoUpdate)
    : GxsFeedItem(feedHolder, feedId, groupId, post_id, isHome, rsPosted, autoUpdate)
    , mInFill(false)
    , mLoaded(false), mIsLoadingGroup(false), mIsLoadingMessage(false), mIsLoadingComment(false)
{
	mPost.mMeta.mMsgId = post_id;
}

BaseBoardsCommentsItem::~BaseBoardsCommentsItem()
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

void BaseBoardsCommentsItem::paintEvent(QPaintEvent *e)
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

bool BaseBoardsCommentsItem::setPost(const RsPostedPost &post, bool doFill)
{
	if (groupId() != post.mMeta.mGroupId || messageId() != post.mMeta.mMsgId) {
		std::cerr << "BaseBoardsCommentsItem::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mPost = post;

	if (doFill)
		fill();

	return true;
}

void BaseBoardsCommentsItem::loadGroup()
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

void BaseBoardsCommentsItem::loadMessage()
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
			RsErr() << "BaseBoardsCommentsItem::loadMessage() ERROR getting data" << std::endl;
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


void BaseBoardsCommentsItem::loadComment()
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
			RsErr() << "BaseBoardsCommentsItem::loadGroup() ERROR getting data" << std::endl;
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

QString BaseBoardsCommentsItem::groupName()
{
	return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

QString BaseBoardsCommentsItem::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void BaseBoardsCommentsItem::loadComments()
{
	std::cerr << "BaseBoardsCommentsItem::loadComments()";
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
void BaseBoardsCommentsItem::readToggled(bool checked)
{
	if (mInFill) {
		return;
	}

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsPosted->setMessageReadStatus(token, msgPair, !checked);

	setReadStatus(false, checked);
}

void BaseBoardsCommentsItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "BaseBoardsCommentsItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	readToggled(false);
	removeItem();
}
void BaseBoardsCommentsItem::copyMessageLink()
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

void BaseBoardsCommentsItem::showAuthorInPeople()
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
//                                        BoardsCommentsItem                                    //
//========================================================================================

BoardsCommentsItem::BoardsCommentsItem(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData &group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    BaseBoardsCommentsItem(feedHolder, feedId, group_meta, post_id, isHome, autoUpdate)
{
	setup();
}

BoardsCommentsItem::BoardsCommentsItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate) :
    BaseBoardsCommentsItem(feedHolder, feedId, groupId, post_id, isHome, autoUpdate)
{
	setup();
	loadGroup();
}

void BoardsCommentsItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::BoardsCommentsItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mInFill = false;

	/* clear ui */
	ui->datetimeLabel->clear();
	ui->replyFrame->hide();

	ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
	ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
	ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
	ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

	/* general ones */
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT( toggle()));
	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));

	// hide voting buttons, backend is not implemented yet
	ui->voteUpButton->hide();
	ui->voteDownButton->hide();
	ui->scoreLabel->hide();

	// hide expand button, replies is not implemented yet
	ui->expandButton->hide();

	ui->clearButton->hide();
	ui->readAndClearButton->hide();
}

void BoardsCommentsItem::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void BoardsCommentsItem::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}

void BoardsCommentsItem::setComment(const RsGxsComment& cmt)
{
	ui->commLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(cmt.mComment.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS));

	ui->nameLabel->setId(cmt.mMeta.mAuthorId);
	ui->datetimeLabel->setText(DateTime::formatLongDateTime(cmt.mMeta.mPublishTs));

	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(cmt.mMeta.mAuthorId,idDetails);
	QPixmap pixmap;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
		pixmap = GxsIdDetails::makeDefaultIcon(cmt.mMeta.mAuthorId,GxsIdDetails::SMALL);
		ui->avatarLabel->setPixmap(pixmap);

}
void BoardsCommentsItem::setCommentsSize(int comNb)
{
	QString sComButText = tr("Comment");
	if (comNb == 1)
		sComButText = sComButText.append("(1)");
	else if(comNb > 1)
		sComButText = tr("Comments ").append("(%1)").arg(comNb);

	//ui->commentButton->setText(sComButText);
}

void BoardsCommentsItem::fill()
{

	ui->logoLabel->hide();
	//ui->logoLabel->setPixmap( FilesDefs::getPixmapFromQtResourcePath(":/icons/png/comment.png"));

	//RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_POSTED, mGroupMeta.mGroupId, groupName());
	//ui->titleLabel->setText(link.toHtml());

	RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, messageName());
	ui->subjectLabel->setText(msgLink.toHtml());

	mInFill = true;

	//QString score = QString::number(mPost.mTopScore);
	//ui->scoreLabel->setText(score);

	// differences between Feed or Top of Comment.
	if (mFeedHolder)
	{
		// feed.
		//frame_comment->show();
		//ui->commentButton->show();

		/*if (mPost.mComments)
		{
			QString commentText = QString::number(mPost.mComments);
			commentText += " ";
			commentText += tr("Comments");
			ui->commentButton->setText(commentText);
		}
		else
		{
			ui->commentButton->setText(tr("Comment"));
		}*/

		//setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
	}
	else
	{
		// no feed.
		//frame_comment->hide();

		ui->readButton->hide();
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
	
	// hide read button not yet functional
	ui->readButton->hide();

	// disable voting buttons - if they have already voted.
	if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		ui->voteUpButton->setEnabled(false);
		ui->voteDownButton->setEnabled(false);
	}

	mInFill = false;

	emit sizeChanged(this);
}

void BoardsCommentsItem::setReadStatus(bool isNew, bool isUnread)
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

	ui->mainFrame->setProperty("new", isNew);
	ui->mainFrame->style()->unpolish(ui->mainFrame);
	ui->mainFrame->style()->polish(  ui->mainFrame);
}

void BoardsCommentsItem::toggle()
{
	expand(ui->replyFrame->isHidden());
}

void BoardsCommentsItem::doExpand(bool open)
{
	if (open)
	{
		ui->replyFrame->show();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		ui->replyFrame->hide();
		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

}
