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

#include "MsgItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"

#include "rsiface/rsmsgs.h"
#include "rsiface/rspeers.h"

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
MsgItem::MsgItem(FeedHolder *parent, uint32_t feedId, std::string msgId, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId), mMsgId(msgId), mIsHome(isHome)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect( playButton, SIGNAL( clicked( void ) ), this, SLOT( playMedia ( void ) ) );
  connect( deleteButton, SIGNAL( clicked( void ) ), this, SLOT( deleteMsg ( void ) ) );
  connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyMsg ( void ) ) );

  small();
  updateItemStatic();
  updateItem();
}


void MsgItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	MessageInfo mi;

	if (!rsMsgs) 
		return;

	if (!rsMsgs->getMessage(mMsgId, mi))
		return;

	/* get peer Id */
	mPeerId = mi.srcId;

	QString title;
        QString timestamp;
	QString srcName = QString::fromStdString(rsPeers->getPeerName(mi.srcId));

	{
		QDateTime qtime;
		qtime.setTime_t(mi.ts);
                timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
	}

	if (!mIsHome)
	{
		title = "Message From: ";
	}
	else
	{
		/* subject */
		uint32_t box = mi.msgflags & RS_MSG_BOXMASK;
		switch(box)
		{
			case RS_MSG_SENTBOX:
				title = "Sent Msg: ";
				replyButton->setEnabled(false);
				break;
			case RS_MSG_DRAFTBOX:
				title = "Draft Msg: ";
				replyButton->setEnabled(false);
				break;
			case RS_MSG_OUTBOX:
				title = "Pending Msg: ";
				//deleteButton->setEnabled(false);
				replyButton->setEnabled(false);
				break;
			default:
			case RS_MSG_INBOX:
				title = "";
				break;
		}
	}
	title += srcName + " @ " + timestamp;


	titleLabel->setText(title);
	subjectLabel->setText(QString::fromStdWString(mi.title));
		
	msgLabel->setText(QString::fromStdWString(mi.msg));

	std::list<FileInfo>::iterator it;
	for(it = mi.files.begin(); it != mi.files.end(); it++)
	{
		/* add file */
		SubFileItem *fi = new SubFileItem(it->hash, it->fname, it->path, it->size, SFI_STATE_REMOTE, mi.srcId);
		mFileItems.push_back(fi);

		QLayout *layout = expandFrame->layout();
		layout->addWidget(fi);
	}

	playButton->setEnabled(false);
	
	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);

		/* hide buttons */
		clearButton->hide();
	}
	else
	{
		//deleteButton->setEnabled(false);
	}
}


void MsgItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::updateItem()";
	std::cerr << std::endl;
#endif
	int msec_rate = 10000;

	/* Very slow Tick to check when all files are downloaded */
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); it++)
	{
		if (!(*it)->done())
		{
			/* loop again */
	  		QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
			return;
		}
	}
	if (mFileItems.size() > 0)
	{
		playButton->setEnabled(true);
	}
}


void MsgItem::small()
{
	expandFrame->hide();
}

void MsgItem::toggle()
{
	if (expandFrame->isHidden())
	{
		expandFrame->show();
	}
	else
	{
		expandFrame->hide();
	}
}


void MsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}


void MsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIOSN ***********************/


void MsgItem::deleteMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::deleteMsg()";
	std::cerr << std::endl;
#endif
	if (rsMsgs)
	{
		rsMsgs->MessageDelete(mMsgId);

		hide(); /* will be cleaned up next refresh */
	}
}

void MsgItem::replyMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::replyMsg()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		mParent->openMsg(FEEDHOLDER_MSG_MESSAGE, mPeerId, mMsgId);
        }
}



void MsgItem::playMedia()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::playMedia()";
	std::cerr << std::endl;
#endif
}



