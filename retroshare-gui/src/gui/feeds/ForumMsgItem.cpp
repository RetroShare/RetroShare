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

#include "ForumMsgItem.h"
#include "FeedHolder.h"

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
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeForum ( void ) ) );
  connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyToPost ( void ) ) );
  
  connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));

  small();
  updateItemStatic();
  updateItem();
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

	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		QString title = "Forum Post: ";
		title += QString::fromStdWString(fi.forumName);

		titleLabel->setText(title);
		if (!(fi.forumFlags & RS_DISTRIB_SUBSCRIBED))
		{
			unsubscribeButton->setEnabled(false);
			replyButton->setEnabled(true);
		}
		else
		{
			unsubscribeButton->setEnabled(true);
			replyButton->setEnabled(true);
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

			prevSubLabel->setText(QString::fromStdWString(msg.title));
			prevMsgLabel->setText(RsHtml::formatText(QString::fromStdWString(msg.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
			
            QDateTime qtime;
            qtime.setTime_t(msg.ts);
            QString timestamp = qtime.toString("dd.MM.yyyy hh:mm:ss");
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

			nextSubLabel->setText(QString::fromStdWString(msg.title));
			nextMsgLabel->setText(RsHtml::formatText(QString::fromStdWString(msg.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
			
			QDateTime qtime;
            qtime.setTime_t(msg.ts);
            QString timestamp = qtime.toString("dd.MM.yyyy hh:mm:ss");
            timestamplabel->setText(timestamp);
			
			prevSHLabel->setText(tr("In Reply to") + ": ");

			ForumMsgInfo msgParent;
			if (rsForums->getForumMessage(mForumId, msg.parentId, msgParent))
			{
				mGpgIdPrev = msgParent.srcId;

				prevSubLabel->setText(QString::fromStdWString(msgParent.title));
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
		srcLabel->setText(QString::fromStdString(msg.srcId));

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


void ForumMsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
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
#ifdef DEBUG_ITEM
	std::cerr << "ForumMsgItem::replyToPost()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		//mParent->openMsg(FEEDHOLDER_MSG_FORUM, mForumId, mPostId);
		CreateForumMsg *cfm = new CreateForumMsg(mForumId, mPostId);
		cfm->show();
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
