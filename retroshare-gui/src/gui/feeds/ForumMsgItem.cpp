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
#include <QtGui>

#include "ForumMsgItem.h"
#include "FeedHolder.h"

#include "rsiface/rsforums.h"
#include "gui/forums/CreateForumMsg.h"


#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ForumMsgItem::ForumMsgItem(FeedHolder *parent, uint32_t feedId, std::string forumId, std::string postId, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId), 
	mForumId(forumId), mPostId(postId), mIsHome(isHome), mIsTop(false)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeForum ( void ) ) );
  connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyToPost ( void ) ) );

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
		titleLabel->setText("Unknown Forum Post");
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
			prevSHLabel->setText("Subject: ");
			prevSubLabel->setText(QString::fromStdWString(msg.title));
			prevMsgLabel->setText(QString::fromStdWString(msg.msg));

			nextFrame->hide();
		}
		else
		{
			nextSubLabel->setText(QString::fromStdWString(msg.title));
			nextMsgLabel->setText(QString::fromStdWString(msg.msg));
			
			prevSHLabel->setText("In Reply To: ");

			ForumMsgInfo msgParent;
			if (rsForums->getForumMessage(mForumId, msg.parentId, msgParent))
			{
				prevSubLabel->setText(QString::fromStdWString(msgParent.title));
				prevMsgLabel->setText(QString::fromStdWString(msgParent.msg));
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

