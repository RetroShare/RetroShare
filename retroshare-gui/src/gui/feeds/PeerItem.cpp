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

#include "PeerItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/AvatarDefs.h"
#include "rshare.h"

#include "gui/notifyqt.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PeerItem::PeerItem(FeedHolder *parent, uint32_t feedId, const std::string &peerId, uint32_t type, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mPeerId(peerId), mType(type), mIsHome(isHome)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);
  
    messageframe->setVisible(false);
    sendmsgButton->setEnabled(false);

    /* general ones */
    connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
    connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

    /* specific ones */
    connect( chatButton, SIGNAL( clicked( void ) ), this, SLOT( openChat ( void ) ) );
    connect( actionNew_Message, SIGNAL( triggered( ) ), this, SLOT( sendMsg ( void ) ) );

    connect( quickmsgButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );
    connect( cancelButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );

    connect( sendmsgButton, SIGNAL( clicked( ) ), this, SLOT( sendMessage() ) );

    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateItem()));

    QMenu *msgmenu = new QMenu();
    msgmenu->addAction(actionNew_Message);

    quickmsgButton->setMenu(msgmenu);

    avatar->setId(mPeerId, false);

    expandFrame->hide();

    updateItemStatic();
    updateItem();
}


void PeerItem::updateItemStatic()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	QString title;

	switch(mType)
	{
		case PEER_TYPE_STD:
			title = tr("Friend");
			break;
		case PEER_TYPE_CONNECT:
			title = tr("Friend Connected");
			break;
		case PEER_TYPE_HELLO:
			title = tr("Connect Attempt");
			break;
		case PEER_TYPE_NEW_FOF:
			title = tr("Friend of Friend");
			break;
		default:
			title = tr("Peer");
			break;
	}

	titleLabel->setText(title);

	RsPeerDetails details;
	if (rsPeers->getPeerDetails(mPeerId, details))
	{
		/* set peer name */
		peerNameLabel->setText(QString::fromUtf8(details.name.c_str()));
		lastLabel-> setText(Rshare::customLongDate(details.lastConnect));

		/* expanded Info */
		nameLabel->setText(QString::fromUtf8(details.name.c_str()));
		idLabel->setText(QString::fromStdString(details.id));
		locLabel->setText(QString::fromUtf8(details.location.c_str()));
	}
	else
	{
		statusLabel->setText(tr("Unknown Peer"));
		titleLabel->setText(tr("Unknown Peer"));
		trustLabel->setText(tr("Unknown Peer"));
		nameLabel->setText(tr("Unknown Peer"));
		idLabel->setText(tr("Unknown Peer"));
		locLabel->setText(tr("Unknown Peer"));
		ipLabel->setText(tr("Unknown Peer"));
		connLabel->setText(tr("Unknown Peer"));
		lastLabel->setText(tr("Unknown Peer"));

		chatButton->setEnabled(false);
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);

		/* hide buttons */
		clearButton->hide();
	}
}

void PeerItem::updateItem()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::updateItem()";
	std::cerr << std::endl;
#endif
	if(!RsAutoUpdatePage::eventsLocked()) {
		RsPeerDetails details;
		if (!rsPeers->getPeerDetails(mPeerId, details))
		{
			chatButton->setEnabled(false);
			quickmsgButton->setEnabled(false);

			return;
		}

		/* top Level info */
		QString status = StatusDefs::peerStateString(details.state);

#if 0
		/* Append additional status info from status service */
		StatusInfo statusInfo;
		if ((rsStatus) && (rsStatus->getStatus(*it, statusInfo)))
		{
			status.append(QString::fromStdString("/" + RsStatusString(statusInfo.status)));
		}
#endif
		statusLabel->setText(status);
		trustLabel->setText(QString::fromStdString(RsPeerTrustString(details.trustLvl)));

		ipLabel->setText(QString("%1:%2/%3:%4").arg(QString::fromStdString(details.localAddr)).arg(details.localPort).arg(QString::fromStdString(details.extAddr)).arg(details.extPort));

		connLabel->setText(StatusDefs::connectStateString(details));

		/* do buttons */
		chatButton->setEnabled(details.state & RS_PEER_STATE_CONNECTED);
		if (details.state & RS_PEER_STATE_FRIEND)
		{
			quickmsgButton->setEnabled(true);
		}
		else
		{
			quickmsgButton->setEnabled(false);
		}
	}

	/* slow Tick  */
	int msec_rate = 10129;
	
	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	return;
}

void PeerItem::toggle()
{
	mParent->lockLayout(this, true);

	if (expandFrame->isHidden())
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	mParent->lockLayout(this, false);
}


void PeerItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::removeItem()";
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

void PeerItem::addFriend()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::addFriend()";
	std::cerr << std::endl;
#endif
}

void PeerItem::removeFriend()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::removeFriend()";
	std::cerr << std::endl;
#endif
}

void PeerItem::sendMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::sendMsg()";
	std::cerr << std::endl;
#endif

    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->addRecipient(MessageComposer::TO, mPeerId, false);
    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}


void PeerItem::openChat()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::openChat()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		mParent->openChat(mPeerId);
	}
}

void PeerItem::togglequickmessage()
{
	if (messageframe->isHidden())
	{
		messageframe->setVisible(true);
	}
	else
	{
		messageframe->setVisible(false);
	}
}

void PeerItem::sendMessage()
{
    /* construct a message */
    MessageInfo mi;
    
    mi.title = tr("Quick Message").toStdWString();
    mi.msg =   quickmsgText->toHtml().toStdWString();
    mi.msgto.push_back(mPeerId);
    
    rsMsgs->MessageSend(mi);

    quickmsgText->clear();
    messageframe->setVisible(false);
}

void PeerItem::on_quickmsgText_textChanged()
{
    if (quickmsgText->toPlainText().isEmpty())
    {
        sendmsgButton->setEnabled(false);
    }
    else
    {
        sendmsgButton->setEnabled(true);
    }
}
