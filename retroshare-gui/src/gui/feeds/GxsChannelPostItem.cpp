/*
 * Retroshare Gxs Feed Item
 *
 * Copyright 2012-2013 by Robert Fernie.
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
#include <QTimer>
#include <QFileInfo>

#include "rshare.h"
#include "GxsChannelPostItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"
#include "gui/notifyqt.h"
#include "util/misc.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

#define DEBUG_ITEM 	1

#define COLOR_NORMAL QColor(248, 248, 248)
#define COLOR_NEW    QColor(220, 236, 253)

#define SELF_LOAD      1
#define DATA_PROVIDED  2

GxsChannelPostItem::GxsChannelPostItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome) :
	GxsFeedItem(parent, feedId, groupId, messageId, isHome, rsGxsChannels, true)
{
	mMode = SELF_LOAD;

	setup();
}

/** Constructor */
GxsChannelPostItem::GxsChannelPostItem(FeedHolder *parent, uint32_t feedId, const RsGxsChannelPost &post, uint32_t subFlags, bool isHome) :
	GxsFeedItem(parent, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsGxsChannels, false)
{
	std::cerr << "GxsChannelPostItem::GxsChannelPostItem() Direct Load";
	std::cerr << std::endl;

	mMode = DATA_PROVIDED;
	mGroupMeta.mSubscribeFlags = subFlags;
	mInUpdateItemStatic = false;

	setup();

	// is it because we are in the constructor?
	loadPost(post);
}

GxsChannelPostItem::~GxsChannelPostItem()
{
	delete(ui);
}

void GxsChannelPostItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsChannelPostItem;
	ui->setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mInUpdateItemStatic = false;

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked(void)), this, SLOT(toggle(void)));
	connect(ui->clearButton, SIGNAL(clicked(void)), this, SLOT(removeItem(void)));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->unsubscribeButton, SIGNAL(clicked(void)), this, SLOT(unsubscribeChannel(void)));

	connect(ui->downloadButton, SIGNAL(clicked(void)), this, SLOT(download(void)));
	// HACK FOR NOW.
	connect(ui->commentButton, SIGNAL(clicked(void)), this, SLOT(loadComments(void)));

	connect(ui->playButton, SIGNAL(clicked(void)), this, SLOT( play(void)));
	connect(ui->copyLinkButton, SIGNAL(clicked(void)), this, SLOT(copyLink(void)));

	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));
	//connect(NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)), Qt::QueuedConnection);

	//connect(ui-> voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	//connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT(makeDownVote()));

	ui->downloadButton->hide();
	ui->playButton->hide();
	ui->warn_image_label->hide();
	ui->warning_label->hide();

	ui->titleLabel->setMinimumWidth(100);
	ui->subjectLabel->setMinimumWidth(100);
	ui->warning_label->setMinimumWidth(100);

	ui->frame->setProperty("state", "");
	QPalette palette = ui->frame->palette();
	palette.setColor(ui->frame->backgroundRole(), COLOR_NORMAL);
	ui->frame->setPalette(palette);

	ui->expandFrame->hide();
}

void GxsChannelPostItem::loadComments()
{
	QString title = QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
	comments(title);
}

void GxsChannelPostItem::loadMessage(const uint32_t &token)
{
	std::cerr << "GxsChannelPostItem::loadMessage()";
	std::cerr << std::endl;

	std::vector<RsGxsChannelPost> posts;
	if (!rsGxsChannels->getPostData(token, posts))
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

	loadPost(posts[0]);

	updateItem();
}

void GxsChannelPostItem::loadPost(const RsGxsChannelPost &post)
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "GxsChannelPostItem::loadPost()";
	std::cerr << std::endl;
#endif

	mInUpdateItemStatic = true;

	mPost = post;

	QString title;

	if (!mIsHome)
	{
		title = tr("Channel Feed") + ": ";
		RetroShareLink link;
		link.createChannel(post.mMeta.mGroupId.toStdString(), "");
		title += link.toHtml();
		ui->titleLabel->setText(title);
		RetroShareLink msgLink;
		msgLink.createChannel(post.mMeta.mGroupId.toStdString(), post.mMeta.mMsgId.toStdString());
		ui->subjectLabel->setText(msgLink.toHtml());

		if (IS_GROUP_SUBSCRIBED(mSubscribeFlags) || IS_GROUP_ADMIN(mSubscribeFlags))
		{
			ui->unsubscribeButton->setEnabled(true);
		}
		else 
		{
			ui->unsubscribeButton->setEnabled(false);
		}
		ui->readButton->hide();
		//newLabel->hide();
		ui->copyLinkButton->hide();
	}
	else
	{
		/* subject */
		ui->titleLabel->setText(QString::fromUtf8(post.mMeta.mMsgName.c_str()));
		ui->subjectLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(post.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
		
		//QString score = QString::number(post.mTopScore);
		// scoreLabel->setText(score); 

		/* disable buttons: deletion facility not enabled with cache services yet */
		ui->clearButton->setEnabled(false);
		ui->unsubscribeButton->setEnabled(false);
		ui->clearButton->hide();
		ui->readAndClearButton->hide();
		ui->unsubscribeButton->hide();
		ui->copyLinkButton->show();

		if (IS_GROUP_SUBSCRIBED(mGroupMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags))
		{
			ui->readButton->setVisible(true);
#if 0
			uint32_t status = 0;
			rsChannels->getMessageStatus(mChanId, mMsgId, status);
#endif

			if (IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
			{
				ui->readButton->setChecked(true);
				ui->readButton->setIcon(QIcon(":/images/message-state-unread.png"));
			} 
			else 
			{
				ui->readButton->setChecked(false);
				ui->readButton->setIcon(QIcon(":/images/message-state-read.png"));
			}

			bool newState;
			QColor color;
			if (!IS_MSG_UNREAD(post.mMeta.mMsgStatus))
			{
				//newLabel->setVisible(false);
				newState = false;
				color = COLOR_NORMAL;
			} 
			else 
			{
				//newLabel->setVisible(true);
				newState = true;
				color = COLOR_NEW;
			}

			/* unpolish widget to clear the stylesheet's palette cache */
			ui->frame->style()->unpolish(ui->frame);

			QPalette palette = ui->frame->palette();
			palette.setColor(ui->frame->backgroundRole(), color);
			ui->frame->setPalette(palette);

			ui->frame->setProperty("new", newState);
			Rshare::refreshStyleSheet(ui->frame, false);
		} 
		else 
		{
			ui->readButton->setVisible(false);
			//ui->newLabel->setVisible(false);
		}
	}
	
	// differences between Feed or Top of Comment.
	if (mParent)
	{
		ui->commentButton->show();

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

	ui->msgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(post.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
	ui->msgFrame->setVisible(!post.mMsg.empty());

	ui->datetimelabel->setText(DateTime::formatLongDateTime(post.mMeta.mPublishTs));

	ui->filelabel->setText(QString("(%1 %2) %3").arg(post.mCount).arg(tr("Files")).arg(misc::friendlyUnit(post.mSize)));

	if (mFileItems.empty() == false) {
		std::list<SubFileItem *>::iterator it;
		for(it = mFileItems.begin(); it != mFileItems.end(); it++)
		{
			delete(*it);
		}
		mFileItems.clear();
	}

	std::list<RsGxsFile>::const_iterator it;
	for(it = post.mFiles.begin(); it != post.mFiles.end(); it++)
	{
		/* add file */
		std::string path;
		SubFileItem *fi = new SubFileItem(it->mHash, it->mName, path, it->mSize, SFI_STATE_REMOTE | SFI_TYPE_CHANNEL, RsPeerId());
		mFileItems.push_back(fi);
		
		/* check if the file is a media file */
		if (!misc::isPreviewable(QFileInfo(QString::fromUtf8(it->mName.c_str())).suffix())) 
		  fi->mediatype();

		QLayout *layout = ui->expandFrame->layout();
		layout->addWidget(fi);
	}

	if(post.mThumbnail.mData != NULL)
	{
		QPixmap thumbnail;
		thumbnail.loadFromData(post.mThumbnail.mData, post.mThumbnail.mSize, "PNG");
		// Wiping data - as its been passed to thumbnail.
		ui->logoLabel->setPixmap(thumbnail);
	}

	mInUpdateItemStatic = false;
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
	for(it = mFileItems.begin(); it != mFileItems.end(); it++)
	{
		SubFileItem *item = *it;

		if (item->isDownloadable(startable)) {
			downloadCount++;
			if (startable) {
				downloadStartable++;
			}
		}
		if (item->isPlayable(startable)) {
			playCount++;
			if (startable) {
				playStartable++;
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

void GxsChannelPostItem::toggle()
{
	if (mParent)
	{
		mParent->lockLayout(this, true);
	}

	if (ui->expandFrame->isHidden())
	{
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		readToggled(false);
	}
	else
	{
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	if (mParent)
	{
		mParent->lockLayout(this, false);
	}
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
	updateItemStatic();
}

void GxsChannelPostItem::download()
{
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); it++)
	{
		(*it)->download();
	}

	updateItem();
}

void GxsChannelPostItem::play()
{
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); it++)
	{
		bool startable;
		if ((*it)->isPlayable(startable) && startable) {
			(*it)->play();
		}
	}
}

void GxsChannelPostItem::readToggled(bool checked)
{
#if 0
	if (mInUpdateItemStatic) {
		return;
	}

	/* set always as read ... */
	uint32_t statusNew = CHANNEL_MSG_STATUS_READ;
	if (checked) {
		/* ... and as unread by user */
		statusNew |= CHANNEL_MSG_STATUS_UNREAD_BY_USER;
	} else {
		/* ... and as read by user */
		statusNew &= ~CHANNEL_MSG_STATUS_UNREAD_BY_USER;
	}

	if (!mIsHome) {
		disconnect( NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)));
	}
	rsChannels->setMessageStatus(mChanId, mMsgId, statusNew, CHANNEL_MSG_STATUS_READ | CHANNEL_MSG_STATUS_UNREAD_BY_USER);
	if (!mIsHome) {
		connect( NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)), Qt::QueuedConnection);
	}
#endif
}

void GxsChannelPostItem::channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status)
{
#if 0
	if (channelId.toStdString() == mChanId && msgId.toStdString() == mMsgId) {
		if (!mIsHome) {
			if (status & CHANNEL_MSG_STATUS_READ) {
				close();
				return;
			}
		}
		updateItemStatic();
	}
#endif
}

void GxsChannelPostItem::copyLink()
{
#if 0
	if (mChanId.empty() || mMsgId.empty()) {
		return;
	}

	ChannelMsgInfo cmi;
	if (rsChannels->getChannelMessage(mChanId, mMsgId, cmi)) {
		RetroShareLink link;
		if (link.createChannel(cmi.channelId, cmi.msgId)) {
			QList<RetroShareLink> urls;
			urls.push_back(link);
			RSLinkClipboard::copyLinks(urls);
		}
	}
#endif
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
