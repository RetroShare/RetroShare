/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QDateTime>
#include <QMessageBox>

#include "ForumMsgItem.h"
#include "FeedHolder.h"
#include "gui/RetroShareLink.h"

#include <retroshare/rsforums.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

#include "gui/forums/CreateForumMsg.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/common/AvatarDefs.h"
#include "gui/notifyqt.h"
#include "gui/ForumsDialog.h"
//#include "gui/settings/rsharesettings.h"

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ForumMsgItem::ForumMsgItem(FeedHolder *parent, uint32_t feedId, const std::string &forumId, const std::string &postId, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId), mForumId(forumId), mPostId(postId), mIsHome(isHome), mIsTop(false)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	/* general ones */
	connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
	connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

	/* specific ones */
	connect(readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeForum ( void ) ) );
	connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyToPost ( void ) ) );
	connect( sendButton, SIGNAL( clicked( ) ), this, SLOT( sendMsg() ) );

	connect(NotifyQt::getInstance(), SIGNAL(forumMsgReadSatusChanged(QString,QString,int)), this, SLOT(forumMsgReadSatusChanged(QString,QString,int)), Qt::QueuedConnection);

	subjectLabel->setMinimumWidth(20);

	nextFrame->hide();
	prevFrame->hide();

	updateItemStatic();
	updateItem();
	textEdit->hide();
	sendButton->hide();
	signedcheckBox->hide();
}

void ForumMsgItem::updateItemStatic()
{
	if (!rsForums)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	canReply = false;

	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		RetroShareLink link;
		link.createForum(fi.forumId, "");
		QString title = tr("Forum Post") + ": ";
		title += link.toHtml();

		titleLabel->setText(title);
		if (fi.subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED))
		{
			unsubscribeButton->setEnabled(true);
			replyButton->setEnabled(true);

			if (fi.forumFlags & RS_DISTRIB_AUTHEN_REQ)
			{
				signedcheckBox->setChecked(true);
				signedcheckBox->setEnabled(false);
			}

			canReply = true;
		}
		else
		{
			unsubscribeButton->setEnabled(false);
			replyButton->setEnabled(false);
		}
	}
	else
	{
		titleLabel->setText(tr("Unknown Forum Post"));
	}

	/* get actual Message */
	ForumMsgInfo msg;
	if (rsForums->getForumMessage(mForumId, mPostId, msg))
	{
#ifdef DEBUG_ITEM
		std::cerr << "Ids: MsgId: " << msg.msgId;
		std::cerr << std::endl;
		std::cerr << "Ids: ParentId: " << msg.parentId;
		std::cerr << std::endl;
		std::cerr << "Ids: ThreadId: " << msg.threadId;
		std::cerr << std::endl;
#endif

		/* decide if top or not */
		if ((msg.msgId == msg.threadId) || (msg.threadId == ""))
		{
			mIsTop = true;
		}
		
		RetroShareLink link;
		link.createForum(msg.forumId, msg.msgId);

		if (mIsTop)
		{
			avatar->setId(msg.srcId, true);

			if (rsPeers->getPeerName(msg.srcId) !="")
			{
				RetroShareLink linkMessage;
				linkMessage.createMessage(msg.srcId, "");
				nameLabel->setText(linkMessage.toHtml());
			}
			else
			{
				nameLabel->setText(tr("Anonymous"));
			}

			prevSubLabel->setText(link.toHtml());
			prevMsgLabel->setText(RsHtml().formatText(NULL, ForumsDialog::messageFromInfo(msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

			timestamplabel->setText(DateTime::formatLongDateTime(msg.ts));

			nextFrame->hide();
		}
		else
		{
			nextAvatar->setId(msg.srcId, true);

			if (rsPeers->getPeerName(msg.srcId) !="")
			{
				RetroShareLink linkMessage;
				linkMessage.createMessage(msg.srcId, "");
				nextNameLabel->setText(linkMessage.toHtml());
			}
			else
			{
				nextNameLabel->setText(tr("Anonymous"));
			}

			nextSubLabel->setText(link.toHtml());
			nextMsgLabel->setText(RsHtml().formatText(NULL, ForumsDialog::messageFromInfo(msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
			
			timestamplabel->setText(DateTime::formatLongDateTime(msg.ts));
			
			prevSHLabel->setText(tr("In Reply to") + ": ");

			ForumMsgInfo msgParent;
			if (rsForums->getForumMessage(mForumId, msg.parentId, msgParent))
			{
				avatar->setId(msgParent.srcId, true);

				RetroShareLink linkParent;
				linkParent.createForum(msgParent.forumId, msgParent.msgId);
				prevSubLabel->setText(linkParent.toHtml());
				prevMsgLabel->setText(RsHtml().formatText(NULL, ForumsDialog::messageFromInfo(msgParent), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
				
				if (rsPeers->getPeerName(msgParent.srcId) !="")
				{
					RetroShareLink linkMessage;
					linkMessage.createMessage(msgParent.srcId, "");
					nameLabel->setText(linkMessage.toHtml());
				}
				else
				{
					nameLabel->setText(tr("Anonymous"));
				}
			}
			else
			{
				prevSubLabel->setText("???");
				prevMsgLabel->setText("???");
			}
		}

		/* header stuff */
		subjectLabel->setText(link.toHtml());
		//srcLabel->setText(QString::fromStdString(msg.srcId));
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);

		clearButton->hide();
	}

	unsubscribeButton->hide();
}

void ForumMsgItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::updateItem()";
	std::cerr << std::endl;
#endif
}

void ForumMsgItem::toggle()
{
	mParent->lockLayout(this, true);

	if (prevFrame->isHidden())
	{
		prevFrame->show();
		textEdit->setVisible(canReply);
		sendButton->setVisible(canReply);
		signedcheckBox->setVisible(canReply);
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		expandButton->setToolTip(tr("Hide"));
		if (!mIsTop)
		{
			nextFrame->show();
		}

		setAsRead();
	}
	else
	{
		prevFrame->hide();
		nextFrame->hide();
		textEdit->hide();
		sendButton->hide();
		signedcheckBox->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	mParent->lockLayout(this, false);
}

void ForumMsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::removeItem()";
	std::cerr << std::endl;
#endif

	mParent->lockLayout(this, true);
	hide();
	mParent->lockLayout(this, false);

	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}

/*********** SPECIFIC FUNCTIOSN ***********************/

void ForumMsgItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	setAsRead();
	removeItem();
}

void ForumMsgItem::unsubscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::unsubscribeForum()";
	std::cerr << std::endl;
#endif
	if (rsForums)
	{
		rsForums->forumSubscribe(mForumId, false);
	}
	updateItemStatic();
}

void ForumMsgItem::subscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::subscribeForum()";
	std::cerr << std::endl;
#endif
	if (rsForums)
	{
		rsForums->forumSubscribe(mForumId, true);
	}
	updateItemStatic();
}

void ForumMsgItem::replyToPost()
{
	if (canReply == false) {
		return;
	}

#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::replyToPost()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		CreateForumMsg *cfm = new CreateForumMsg(mForumId, mPostId);
		cfm->show();
	}
}

void ForumMsgItem::sendMsg()
{
	if (canReply == false) {
		return;
	}

	QString desc = textEdit->toHtml();

	if(textEdit->toPlainText().isEmpty())
	{	/* error message */
		QMessageBox::warning(this, "RetroShare",tr("Please give a Text Message"),
							 QMessageBox::Ok, QMessageBox::Ok);

		return; //Don't add  a empty Message!!
	}

	ForumMsgInfo msg;

	/* get message */
	if (rsForums->getForumMessage(mForumId, mPostId, msg)) {
		ForumMsgInfo msgInfo;

		msgInfo.forumId = mForumId;
		msgInfo.threadId = "";
		msgInfo.parentId = mPostId;
		msgInfo.msgId = "";

		/* modify title */
		QString text = QString::fromStdWString(msg.title);
		if (text.startsWith("Re:", Qt::CaseInsensitive)) {
			msgInfo.title = msg.title;
		} else {
			msgInfo.title = L"Re: " + msg.title;
		}

		msgInfo.msg = desc.toStdWString();
		msgInfo.msgflags = 0;

		if (signedcheckBox->isChecked())
		{
			msgInfo.msgflags = RS_DISTRIB_AUTHEN_REQ;
		}

		if ((msgInfo.msg == L"") && (msgInfo.title == L""))
			return; /* do nothing */

		if (rsForums->ForumMessageSend(msgInfo) == true) {
			textEdit->clear();
		}
	}
}

void ForumMsgItem::forumMsgReadSatusChanged(const QString &forumId, const QString &msgId, int status)
{
	if (mForumId == forumId.toStdString() && mPostId == msgId.toStdString()) {
		if (status & FORUM_MSG_STATUS_READ) {
			close();
		}
	}
}

void ForumMsgItem::setAsRead()
{
	if (canReply) {
		uint32_t status;
		rsForums->getMessageStatus(mForumId, mPostId, status);

		/* set always to read ... */
		uint32_t statusNew = status | FORUM_MSG_STATUS_READ;

//			bool setToReadOnActive = Settings->getForumMsgSetToReadOnActivate();
//			if (setToReadOnActive) {
			/* ... and to read by user */
			statusNew &= ~FORUM_MSG_STATUS_UNREAD_BY_USER;
//			} else {
//				/* ... and to unread by user */
//				statusNew |= FORUM_MSG_STATUS_UNREAD_BY_USER;
//			}

		if (status != statusNew) {
			disconnect(NotifyQt::getInstance(), SIGNAL(forumMsgReadSatusChanged(QString,QString,int)), this, SLOT(forumMsgReadSatusChanged(QString,QString,int)));
			rsForums->setMessageStatus(mForumId, mPostId, statusNew, FORUM_MSG_STATUS_READ | FORUM_MSG_STATUS_UNREAD_BY_USER);
			connect(NotifyQt::getInstance(), SIGNAL(forumMsgReadSatusChanged(QString,QString,int)), this, SLOT(forumMsgReadSatusChanged(QString,QString,int)), Qt::QueuedConnection);
		}
	}
}
