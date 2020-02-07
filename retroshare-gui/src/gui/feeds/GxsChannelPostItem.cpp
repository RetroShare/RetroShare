/*******************************************************************************
 * gui/feeds/GxsChannelPostItem.cpp                                            *
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
#include "rshare.h"
#include "GxsChannelPostItem.h"
#include "ui_GxsChannelPostItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"
#include "util/misc.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/stringutil.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"

#include <iostream>
#include <cmath>

/****
 * #define DEBUG_ITEM 1
 ****/

GxsChannelPostItem::GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate,const std::set<RsGxsMessageId>& older_versions) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsGxsChannels, autoUpdate)
{
    mPost.mMeta.mMsgId = messageId; // useful for uniqueIdentifer() before the post is loaded
	init(messageId,older_versions) ;
}

GxsChannelPostItem::GxsChannelPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsChannelPost& post, bool isHome, bool autoUpdate,const std::set<RsGxsMessageId>& older_versions) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsGxsChannels, autoUpdate)
{
    mPost.mMeta.mMsgId.clear(); // security
	init(post.mMeta.mMsgId,older_versions) ;
	mPost = post ;
}

void GxsChannelPostItem::init(const RsGxsMessageId& messageId,const std::set<RsGxsMessageId>& older_versions)
{
	QVector<RsGxsMessageId> v;
	//bool self = false;

	for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
		v.push_back(*it) ;

	if(older_versions.find(messageId) == older_versions.end())
		v.push_back(messageId);

	setMessageVersions(v) ;

	setup();

	mLoaded = false ;
}

void GxsChannelPostItem::paintEvent(QPaintEvent *e)
{
	/* This method employs a trick to trigger a deferred loading. The post and group is requested only
	 * when actually displayed on the screen. */

	if(!mLoaded)
	{
		mLoaded = true ;

		requestGroup();
		requestMessage();
		requestComment();
	}

	GxsFeedItem::paintEvent(e) ;
}

GxsChannelPostItem::~GxsChannelPostItem()
{
	delete(ui);
}

bool GxsChannelPostItem::isUnread() const
{
    return IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) ;
}

void GxsChannelPostItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsChannelPostItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mInFill = false;
	mCloseOnRead = false;

	/* clear ui */
	ui->titleLabel->setText(tr("Loading"));
	ui->datetimelabel->clear();
	ui->filelabel->clear();
	ui->newCommentLabel->hide();
	ui->commLabel->hide();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribeChannel()));

	connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(download()));
	// HACK FOR NOW.
    ui->commentButton->hide();// hidden until properly enabled.
	connect(ui->commentButton, SIGNAL(clicked()), this, SLOT(loadComments()));

	connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play(void)));
	connect(ui->editButton, SIGNAL(clicked()), this, SLOT(edit(void)));
	connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));

	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

    // hide voting buttons, backend is not implemented yet
    ui->voteUpButton->hide();
    ui->voteDownButton->hide();
	//connect(ui-> voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	//connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT(makeDownVote()));

	ui->scoreLabel->hide();

	ui->downloadButton->hide();
	ui->playButton->hide();
	ui->warn_image_label->hide();
	ui->warning_label->hide();

	ui->titleLabel->setMinimumWidth(100);
	//ui->subjectLabel->setMinimumWidth(100);
	ui->warning_label->setMinimumWidth(100);

	ui->mainFrame->setProperty("new", false);
	ui->mainFrame->style()->unpolish(ui->mainFrame);
	ui->mainFrame->style()->polish(  ui->mainFrame);

	ui->expandFrame->hide();
}

bool GxsChannelPostItem::setGroup(const RsGxsChannelGroup &group, bool doFill)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "GxsChannelPostItem::setGroup() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;

	// If not publisher, hide the edit button. Without the publish key, there's no way to edit a message.
#ifdef DEBUG_ITEM
	std::cerr << "Group subscribe flags = " << std::hex << mGroup.mMeta.mSubscribeFlags << std::dec << std::endl ;
#endif
	if( !IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags) )
		ui->editButton->hide() ;

	if (doFill) {
		fill();
	}

	return true;
}

bool GxsChannelPostItem::setPost(const RsGxsChannelPost &post, bool doFill)
{
	if (groupId() != post.mMeta.mGroupId || messageId() != post.mMeta.mMsgId) {
		std::cerr << "GxsChannelPostItem::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mPost = post;

	if (doFill) {
		fill();
	}

	updateItem();

	return true;
}

QString GxsChannelPostItem::getTitleLabel()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

QString GxsChannelPostItem::getMsgLabel()
{
	//return RsHtml().formatText(NULL, QString::fromUtf8(mPost.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);
    // Disabled, because emoticon replacement kills performance.
	return QString::fromUtf8(mPost.mMsg.c_str());
}

QString GxsChannelPostItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsChannelPostItem::loadComments()
{
	QString title = QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
	comments(title);
}

void GxsChannelPostItem::loadGroup(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelGroupItem::loadGroup()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelGroup> groups;
	if (!rsGxsChannels->getGroupData(token, groups))
	{
		std::cerr << "GxsChannelGroupItem::loadGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "GxsChannelGroupItem::loadGroup() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setGroup(groups[0]);
}

void GxsChannelPostItem::loadMessage(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::loadMessage()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsChannelPost> posts;
	std::vector<RsGxsComment> cmts;
	if (!rsGxsChannels->getPostData(token, posts, cmts))
	{
		std::cerr << "GxsChannelPostItem::loadMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (posts.size() == 1)
	{
        std::cerr << (void*)this << ": Obtained post, with msgId = " << posts[0].mMeta.mMsgId << std::endl;
		setPost(posts[0]);
	}
	else if (cmts.size() == 1)
	{
		RsGxsComment cmt = cmts[0];

        std::cerr << (void*)this << ": Obtained comment, setting messageId to threadID = " << cmt.mMeta.mThreadId << std::endl;
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

void GxsChannelPostItem::loadComment(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::loadComment()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsComment> cmts;
	if (!rsGxsChannels->getRelatedComments(token, cmts))
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
		sComButText = tr("Comments ").append("(%1)").arg(comNb);
	}
	ui->commentButton->setText(sComButText);
}

void GxsChannelPostItem::fill()
{
	/* fill in */

	if (isLoading()) {
		/* Wait for all requests */
		return;
	}

#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::fill()";
	std::cerr << std::endl;
#endif

	mInFill = true;

	QString title;
	
	float f = QFontMetricsF(font()).height()/14.0 ;

	if(mPost.mThumbnail.mData != NULL)
	{
		QPixmap thumbnail;	
		
        ui->logoLabel->setScaledContents(true);

		GxsIdDetails::loadPixmapFromData(mPost.mThumbnail.mData, mPost.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);
		// Wiping data - as its been passed to thumbnail.
//		if( thumbnail.width() < 90 ){
//			ui->logoLabel->setMaximumSize(82*f,108*f);
//		}
//		else if( thumbnail.width() < 109 ){
//			ui->logoLabel->setMinimumSize(108*f,108*f);
//			ui->logoLabel->setMaximumSize(108*f,108*f);
//		}
//		else{
//			ui->logoLabel->setMinimumSize(156*f,108*f);
//			ui->logoLabel->setMaximumSize(156*f,108*f);
//		}
		ui->logoLabel->setPixmap(thumbnail);
	}

	if (!mIsHome)
	{
		if (mCloseOnRead && !IS_MSG_NEW(mPost.mMeta.mMsgStatus)) {
			removeItem();
		}

		title = tr("Channel Feed") + ": ";
		RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_CHANNEL, mPost.mMeta.mGroupId, groupName());
		title += link.toHtml();
		ui->titleLabel->setText(title);

		RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_CHANNEL, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, messageName());
		//ui->subjectLabel->setText(msgLink.toHtml());

		if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroup.mMeta.mSubscribeFlags))
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

		if (IS_MSG_NEW(mPost.mMeta.mMsgStatus)) {
			mCloseOnRead = true;
		}
	}
	else
	{
		/* subject */
		ui->titleLabel->setText(QString::fromUtf8(mPost.mMeta.mMsgName.c_str()));

		//uint32_t autorized_lines = (int)floor((ui->logoLabel->height() - ui->titleLabel->height() - ui->buttonHLayout->sizeHint().height())/QFontMetricsF(ui->subjectLabel->font()).height());

		// fill first 4 lines of message. (csoler) Disabled the replacement of smileys and links, because the cost is too crazy
		//ui->subjectLabel->setText(RsHtml().formatText(NULL, RsStringUtil::CopyLines(QString::fromUtf8(mPost.mMsg.c_str()), autorized_lines), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

		//ui->subjectLabel->setText(RsStringUtil::CopyLines(QString::fromUtf8(mPost.mMsg.c_str()), 2)) ;

		//QString score = QString::number(post.mTopScore);
		// scoreLabel->setText(score); 

		/* disable buttons: deletion facility not enabled with cache services yet */
		ui->clearButton->setEnabled(false);
		ui->unsubscribeButton->setEnabled(false);
		ui->clearButton->hide();
		ui->readAndClearButton->hide();
		ui->unsubscribeButton->hide();
		ui->copyLinkButton->show();

		if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroup.mMeta.mSubscribeFlags))
		{
			ui->readButton->setVisible(true);

			setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
		} 
		else 
		{
			ui->readButton->setVisible(false);
			ui->newLabel->setVisible(false);
		}

		mCloseOnRead = false;
	}
	
	// differences between Feed or Top of Comment.
	if (mFeedHolder)
	{
		if (mIsHome) {
			ui->commentButton->show();
		} else if (ui->commentButton->icon().isNull()){
			//Icon is seted if a comment received.
			ui->commentButton->hide();
		}

// THIS CODE IS doesn't compile - disabling until fixed.
#if 0
		if (post.mComments)
		{
			QString commentText = QString::number(post.mComments);
			commentText += " ";
			commentText += tr("Comments");
			ui->commentButton->setText(commentText);
		}
		else
		{
			ui->commentButton->setText(tr("Comment"));
		}
#endif

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
		doc.setHtml( QString::fromUtf8(mPost.mMsg.c_str()) );

		ui->msgFrame->setVisible(doc.toPlainText().length() > 0);
	}

	if (wasExpanded() || ui->expandFrame->isVisible()) {
		fillExpandFrame();
	}

	ui->datetimelabel->setText(DateTime::formatLongDateTime(mPost.mMeta.mPublishTs));

	if ( (mPost.mCount != 0) || (mPost.mSize != 0) ) {
		ui->filelabel->setVisible(true);
		ui->filelabel->setText(QString("(%1 %2) %3").arg(mPost.mCount).arg(  (mPost.mCount > 1)?tr("Files"):tr("File")).arg(misc::friendlyUnit(mPost.mSize)));
	} else {
		ui->filelabel->setVisible(false);
	}

	if (mFileItems.empty() == false) {
		std::list<SubFileItem *>::iterator it;
		for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
		{
			delete(*it);
		}
		mFileItems.clear();
	}

	std::list<RsGxsFile>::const_iterator it;
	for(it = mPost.mFiles.begin(); it != mPost.mFiles.end(); ++it)
	{
		/* add file */
		std::string path;
		SubFileItem *fi = new SubFileItem(it->mHash, it->mName, path, it->mSize, SFI_STATE_REMOTE | SFI_TYPE_CHANNEL, RsPeerId());
		mFileItems.push_back(fi);
		
		/* check if the file is a media file */
		if (!misc::isPreviewable(QFileInfo(QString::fromUtf8(it->mName.c_str())).suffix()))
		{ 
        fi->mediatype();
				/* check if the file is not a media file and change text */
        ui->playButton->setText(tr("Open"));
        ui->playButton->setToolTip(tr("Open File"));
    } else {
        ui->playButton->setText(tr("Play"));
        ui->playButton->setToolTip(tr("Play Media"));
    }

		QLayout *layout = ui->expandFrame->layout();
		layout->addWidget(fi);
	}

	mInFill = false;
}

void GxsChannelPostItem::fillExpandFrame()
{
	ui->msgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

}

QString GxsChannelPostItem::messageName()
{
	return QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
}

void GxsChannelPostItem::setReadStatus(bool isNew, bool isUnread)
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

void GxsChannelPostItem::setFileCleanUpWarning(uint32_t time_left)
{
	int hours = (int)time_left/3600;
	int minutes = (time_left - hours*3600)%60;

	ui->warning_label->setText(tr("Warning! You have less than %1 hours and %2 minute before this file is deleted Consider saving it.").arg(
			QString::number(hours)).arg(QString::number(minutes)));

	QFont warnFont = ui->warning_label->font();
	warnFont.setBold(true);
	ui->warning_label->setFont(warnFont);

	ui->warn_image_label->setVisible(true);
	ui->warning_label->setVisible(true);
}

void GxsChannelPostItem::updateItem()
{
	/* fill in */

#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::updateItem()";
	std::cerr << std::endl;
#endif

	int msec_rate = 10000;

	int downloadCount = 0;
	int downloadStartable = 0;
	int playCount = 0;
	int playStartable = 0;
	bool startable;
	bool loopAgain = false;

	/* Very slow Tick to check when all files are downloaded */
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
	{
		SubFileItem *item = *it;

		if (item->isDownloadable(startable)) {
			++downloadCount;
			if (startable) {
				++downloadStartable;
			}
		}
		if (item->isPlayable(startable)) {
			++playCount;
			if (startable) {
				++playStartable;
			}
		}

		if (!item->done())
		{
			/* loop again */
			loopAgain = true;
		}
	}

	if (downloadCount) {
		ui->downloadButton->show();

		if (downloadStartable) {
			ui->downloadButton->setEnabled(true);
		} else {
			ui->downloadButton->setEnabled(false);
		}
	} else {
		ui->downloadButton->hide();
	}
	if (playCount) {
		/* one file is playable */
		ui->playButton->show();

		if (playStartable == 1) {
			ui->playButton->setEnabled(true);
		} else {
			ui->playButton->setEnabled(false);
		}
	} else {
		ui->playButton->hide();
	}

	if (loopAgain) {
		QTimer::singleShot( msec_rate, this, SLOT(updateItem(void)));
	}

	// HACK TO DISPLAY COMMENT BUTTON FOR NOW.
	//downloadButton->show();
	//downloadButton->setEnabled(true);
}

void GxsChannelPostItem::doExpand(bool open)
{
	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		readToggled(false);
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
	}
}

void GxsChannelPostItem::expandFill(bool first)
{
	GxsFeedItem::expandFill(first);

	if (first) {
		fillExpandFrame();
	}
}

void GxsChannelPostItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

/*********** SPECIFIC FUNCTIONS ***********************/

void GxsChannelPostItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	readToggled(false);
	removeItem();
}

void GxsChannelPostItem::unsubscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::unsubscribeChannel()";
	std::cerr << std::endl;
#endif

	unsubscribe();
}

void GxsChannelPostItem::download()
{
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
	{
		(*it)->download();
	}

	updateItem();
}

void GxsChannelPostItem::edit()
{
	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(mGroup.mMeta.mGroupId,mPost.mMeta.mMsgId);
    msgDialog->show();
}

void GxsChannelPostItem::play()
{
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
	{
		bool startable;
		if ((*it)->isPlayable(startable) && startable) {
			(*it)->play();
		}
	}
}

void GxsChannelPostItem::readToggled(bool checked)
{
	if (mInFill) {
		return;
	}

	mCloseOnRead = false;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsGxsChannels->setMessageReadStatus(token, msgPair, !checked);

	setReadStatus(false, checked);
}

void GxsChannelPostItem::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void GxsChannelPostItem::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}
