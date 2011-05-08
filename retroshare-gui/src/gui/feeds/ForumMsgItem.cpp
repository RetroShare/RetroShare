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

#include "gui/notifyqt.h"

#include "gui/forums/CreateForumMsg.h"
#include "gui/chat/HandleRichText.h"

#include <algorithm>

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
  connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeForum ( void ) ) );
  connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyToPost ( void ) ) );
  connect( sendButton, SIGNAL( clicked( ) ), this, SLOT( sendMsg() ) );
  
  connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));
  
  subjectLabel->setMinimumWidth(20);

  small();
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
		link.createForum(QString::fromStdWString(fi.forumName), QString::fromStdString(fi.forumId), "");
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
		link.createForum(QString::fromStdWString(msg.title), QString::fromStdString(msg.forumId), QString::fromStdString(msg.msgId));

		if (mIsTop)
		{		
			mGpgIdPrev = msg.srcId;

			if (rsPeers->getPeerName(msg.srcId) !="")
			{
				namelabel->setText(QString::fromStdString(rsPeers->getPeerName(msg.srcId)));
			}
			else
			{
				namelabel->setText(tr("Anonymous"));
			}

			prevSubLabel->setText(link.toHtml());
			prevMsgLabel->setText(RsHtml::formatText(QString::fromStdWString(msg.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
			
            QDateTime qtime;
            qtime.setTime_t(msg.ts);
            QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
            timestamplabel->setText(timestamp);            

			nextFrame->hide();
		}
		else
		{
			mGpgIdNext = msg.srcId;

			if (rsPeers->getPeerName(msg.srcId) !="")
			{
				nextnamelabel->setText(QString::fromStdString(rsPeers->getPeerName(msg.srcId)));
			}
			else
			{
				nextnamelabel->setText(tr("Anonymous"));
			}

			nextSubLabel->setText(link.toHtml());
			nextMsgLabel->setText(RsHtml::formatText(QString::fromStdWString(msg.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
			
			QDateTime qtime;
            qtime.setTime_t(msg.ts);
            QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
            timestamplabel->setText(timestamp);
			
			prevSHLabel->setText(tr("In Reply to") + ": ");

			ForumMsgInfo msgParent;
			if (rsForums->getForumMessage(mForumId, msg.parentId, msgParent))
			{
				mGpgIdPrev = msgParent.srcId;

				RetroShareLink linkParent;
				linkParent.createForum(QString::fromStdWString(msgParent.title), QString::fromStdString(msgParent.forumId), QString::fromStdString(msgParent.msgId));
				prevSubLabel->setText(linkParent.toHtml());
				prevMsgLabel->setText(RsHtml::formatText(QString::fromStdWString(msgParent.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
				
				if (rsPeers->getPeerName(msgParent.srcId) !="")
				{
					namelabel->setText(QString::fromStdString(rsPeers->getPeerName(msgParent.srcId)));
				}
				else
				{
					namelabel->setText(tr("Anonymous"));
				}
			}
			else
			{
				prevSubLabel->setText("???");
				prevMsgLabel->setText("???");
			}
		}

		/* header stuff */
		subjectLabel->setText(QString::fromStdWString(msg.title));
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

	showAvatar("", true);
	showAvatar("", false);
}


void ForumMsgItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::updateItem()";
	std::cerr << std::endl;
#endif

}


void ForumMsgItem::small()
{
	nextFrame->hide();
	prevFrame->hide();
}

void ForumMsgItem::toggle()
{
	if (prevFrame->isHidden())
	{
		prevFrame->show();
		textEdit->setVisible(canReply);
		sendButton->setVisible(canReply);
		signedcheckBox->setVisible(canReply);
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	    expandButton->setToolTip("Hide");
		if (!mIsTop)
		{
			nextFrame->show();
		}
	}
	else
	{
		prevFrame->hide();
		nextFrame->hide();
		textEdit->hide();
		sendButton->hide();
		signedcheckBox->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	    expandButton->setToolTip("Expand");
	}
}


void ForumMsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}

/*********** SPECIFIC FUNCTIOSN ***********************/


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

	QString name = prevSubLabel->text();
    QString desc = textEdit->toHtml();
    
	if(textEdit->toPlainText().isEmpty())
    {	/* error message */
		QMessageBox::warning(this, "RetroShare",tr("Please give a Text Message"),
                             QMessageBox::Ok, QMessageBox::Ok);

        return; //Don't add  a empty Message!!
    }

    ForumMsgInfo msgInfo;

    msgInfo.forumId = mForumId;
    msgInfo.threadId = "";
    msgInfo.parentId = mPostId;
    msgInfo.msgId = "";

    msgInfo.title = name.toStdWString();
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

void ForumMsgItem::updateAvatar(const QString &peer_id)
{
	if (mGpgIdPrev.empty() == false) {
		/* Is this one of the ssl ids of the gpg id ? */
		std::list<std::string> sslIds;
		if (rsPeers->getSSLChildListOfGPGId(mGpgIdPrev, sslIds) == false) {
			return;
		}

		if (std::find(sslIds.begin(), sslIds.end(), peer_id.toStdString()) != sslIds.end()) {
			/* One of the ssl ids of the gpg id */
			showAvatar(peer_id.toStdString(), false);
		}
	}

	if (mGpgIdNext.empty() == false) {
		/* Is this one of the ssl ids of the gpg id ? */
		std::list<std::string> sslIds;
		if (rsPeers->getSSLChildListOfGPGId(mGpgIdNext, sslIds) == false) {
			return;
		}

		if (std::find(sslIds.begin(), sslIds.end(), peer_id.toStdString()) != sslIds.end()) {
			/* One of the ssl ids of the gpg id */
			showAvatar(peer_id.toStdString(), true);
		}
	}
}

void ForumMsgItem::showAvatar(const std::string &peer_id, bool next)
{
	std::string gpgId = next ? mGpgIdNext : mGpgIdPrev;
	QLabel *avatar = next ? nextavatarlabel : avatarlabel;

	if (gpgId.empty()) {
		avatar->setPixmap(QPixmap(":/images/user/personal64.png"));
		return;
	}

	unsigned char *data = NULL;
	int size = 0 ;

	if (gpgId == rsPeers->getGPGOwnId()) {
		/* Its me */
		rsMsgs->getOwnAvatarData(data,size);
	} else {
		if (peer_id.empty()) {
			/* Show the first available avatar of one of the ssl ids */
			std::list<std::string> sslIds;
			if (rsPeers->getSSLChildListOfGPGId(gpgId, sslIds) == false) {
				return;
			}

			std::list<std::string>::iterator sslId;
			for (sslId = sslIds.begin(); sslId != sslIds.end(); sslId++) {
				rsMsgs->getAvatarData(*sslId,data,size);
				if (size) {
					break;
				}
			}
		} else {
			rsMsgs->getAvatarData(peer_id,data,size);
		}
	}

	if(size != 0) {
		// set the image
		QPixmap pix ;
		pix.loadFromData(data,size,"PNG") ;
		avatar->setPixmap(pix);
		delete[] data ;
	} else {
		avatar->setPixmap(QPixmap(":/images/user/personal64.png"));
	}
}
