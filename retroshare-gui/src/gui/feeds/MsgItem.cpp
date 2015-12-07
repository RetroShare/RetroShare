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
#include <QTimer>

#include "MsgItem.h"
#include "FeedHolder.h"
#include "SubFileItem.h"
#include "gui/msgs/MessageComposer.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/common/AvatarDefs.h"
#include "gui/notifyqt.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "gui/msgs/MessageInterface.h"

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
MsgItem::MsgItem(FeedHolder *parent, uint32_t feedId, const std::string &msgId, bool isHome) :
   FeedItem(NULL), mParent(parent), mFeedId(feedId), mMsgId(msgId), mIsHome(isHome)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  mCloseOnRead = true;

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect(NotifyQt::getInstance(), SIGNAL(messagesChanged()), this, SLOT(checkMessageReadStatus()));
  connect( playButton, SIGNAL( clicked( void ) ), this, SLOT( playMedia ( void ) ) );
  connect( deleteButton, SIGNAL( clicked( void ) ), this, SLOT( deleteMsg ( void ) ) );
  connect( replyButton, SIGNAL( clicked( void ) ), this, SLOT( replyMsg ( void ) ) );

  expandFrame->hide();

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

	if (!rsMail) 
		return;

	if (!rsMail->getMessage(mMsgId, mi))
		return;

    /* get peer Id */

    if(mi.msgflags & RS_MSG_SIGNED)
        avatar->setGxsId(mi.rsgxsid_srcId);
    else
        avatar->setId(ChatId(mi.rspeerid_srcId));

	QString title;
    QString srcName;

    if ((mi.msgflags & RS_MSG_SYSTEM) && mi.rspeerid_srcId == rsPeers->getOwnId())
		srcName = "RetroShare";
    else
    {
        if(mi.msgflags & RS_MSG_SIGNED)
        {
            RsIdentityDetails details ;
            rsIdentity->getIdDetails(mi.rsgxsid_srcId, details) ;

            srcName = QString::fromUtf8(details.mNickname.c_str());
        }
        else
            srcName = QString::fromUtf8(rsPeers->getPeerName(mi.rspeerid_srcId).c_str());
    }

	timestampLabel->setText(DateTime::formatLongDateTime(mi.ts));

	if (!mIsHome)
	{
		title = tr("Message From") + ": ";
	}
	else
	{
		/* subject */
        uint32_t box = mi.msgflags & RS_MSG_BOXMASK;
		switch(box)
		{
			case RS_MSG_SENTBOX:
				title = tr("Sent Msg") + ": ";
				replyButton->setEnabled(false);
				break;
			case RS_MSG_DRAFTBOX:
				title = tr("Draft Msg") + ": ";
				replyButton->setEnabled(false);
				break;
			case RS_MSG_OUTBOX:
				title = tr("Pending Msg") + ": ";
				//deleteButton->setEnabled(false);
				replyButton->setEnabled(false);
				break;
			default:
			case RS_MSG_INBOX:
				title = "";
				break;
		}
	}
	title += srcName;

	titleLabel->setText(title);
	subjectLabel->setText(QString::fromUtf8(mi.title.c_str()));
	mMsg = QString::fromUtf8(mi.msg.c_str());

	if (wasExpanded() || expandFrame->isVisible()) {
		fillExpandFrame();
	}

	std::list<FileInfo>::iterator it;
	for(it = mi.files.begin(); it != mi.files.end(); ++it)
	{
		/* add file */
        SubFileItem *fi = new SubFileItem(it->hash, it->fname, it->path, it->size, SFI_STATE_REMOTE, mi.rspeerid_srcId);
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
}

void MsgItem::fillExpandFrame()
{
	msgLabel->setText(RsHtml().formatText(NULL, mMsg, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
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
	for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
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

void MsgItem::toggle()
{
	expand(expandFrame->isHidden());
}

void MsgItem::doExpand(bool open)
{
	if (mParent) {
		mParent->lockLayout(this, true);
	}

	if (open)
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		expandButton->setToolTip(tr("Hide"));

		mCloseOnRead = false;
		rsMail->MessageRead(mMsgId, false);
		mCloseOnRead = true;
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mParent) {
		mParent->lockLayout(this, false);
	}
}

void MsgItem::expandFill(bool first)
{
	FeedItem::expandFill(first);

	if (first) {
		fillExpandFrame();
	}
}

void MsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::removeItem()";
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


void MsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIONS ***********************/


void MsgItem::deleteMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::deleteMsg()";
	std::cerr << std::endl;
#endif
	if (rsMail)
	{
		rsMail->MessageDelete(mMsgId);

		removeItem();
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
		//mParent->openMsg(FEEDHOLDER_MSG_MESSAGE, mPeerId, mMsgId);
		
    MessageComposer *nMsgDialog = MessageComposer::replyMsg(mMsgId, false);
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
  }
}

void MsgItem::playMedia()
{
#ifdef DEBUG_ITEM
	std::cerr << "MsgItem::playMedia()";
	std::cerr << std::endl;
#endif
}

void MsgItem::checkMessageReadStatus()
{
	if (!mCloseOnRead) {
		return;
	}

	MessageInfo msgInfo;
	if (!rsMail->getMessage(mMsgId, msgInfo)) {
		std::cerr << "MsgItem::checkMessageReadStatus() Couldn't find Msg" << std::endl;
		return;
	}

	if (msgInfo.msgflags & RS_MSG_NEW) {
		/* Message status is still "new" */
		return;
	}

	removeItem();
}
