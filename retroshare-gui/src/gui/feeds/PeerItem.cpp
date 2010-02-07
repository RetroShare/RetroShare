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

#include "PeerItem.h"
#include "FeedHolder.h"

#include "rsiface/rspeers.h"

#include <iostream>
#include <sstream>
#include <time.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PeerItem::PeerItem(FeedHolder *parent, uint32_t feedId, std::string peerId, uint32_t type, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mPeerId(peerId), mType(type), mIsHome(isHome)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect( chatButton, SIGNAL( clicked( void ) ), this, SLOT( openChat ( void ) ) );
  connect( msgButton, SIGNAL( clicked( void ) ), this, SLOT( sendMsg ( void ) ) );
  connect( addButton, SIGNAL( clicked( void ) ), this, SLOT( addFriend ( void ) ) );
  connect( removeButton, SIGNAL( clicked( void ) ), this, SLOT( removeFriend ( void ) ) );

  small();
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

	RsPeerDetails details;
	if (rsPeers->getPeerDetails(mPeerId, details))
	{
		QString title;

		switch(mType)
		{
			case PEER_TYPE_STD:
				title = "Friend: ";
				break;
			case PEER_TYPE_CONNECT:
				title = "Friend Connected: ";
				break;
			case PEER_TYPE_HELLO:
				title = "Connect Attempt: ";
				break;
			case PEER_TYPE_NEW_FOF:
				title = "Friend of Friend: ";
				break;
			default:
				title = "Peer: ";
				break;
		}

		title += QString::fromStdString(details.name);
		titleLabel->setText(title);

		/* expanded Info */
		nameLabel->setText(QString::fromStdString(details.name));
		idLabel->setText(QString::fromStdString(details.id));
		orgLabel->setText(QString::fromStdString(details.org));
		locLabel->setText(QString::fromStdString(details.location));
		countryLabel->setText("");
	}
	else
	{
		statusLabel->setText("Unknown Peer");
		titleLabel->setText("Unknown Peer");
		trustLabel->setText("Unknown Peer");
		nameLabel->setText("Unknown Peer");
		idLabel->setText("Unknown Peer");
		orgLabel->setText("Unknown Peer");
		locLabel->setText("Unknown Peer");
		countryLabel->setText("Unknown Peer");
		ipLabel->setText("Unknown Peer");
		connLabel->setText("Unknown Peer");
		lastLabel->setText("Unknown Peer");

		chatButton->setEnabled(false);
		addButton->setEnabled(false);
		removeButton->setEnabled(false);
		msgButton->setEnabled(false);
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);

		/* disable buttons */
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
	RsPeerDetails details;
	if (!rsPeers->getPeerDetails(mPeerId, details))
	{
		return;
	}

	/* top Level info */
	QString status = QString::fromStdString(RsPeerStateString(details.state));

#if 0
	/* Append additional status info from status service */
	StatusInfo statusInfo;
	if ((rsStatus) && (rsStatus->getStatus(*it, statusInfo)))
	{
		status.append(QString::fromStdString("/" + RsStatusString(statusInfo.status)));
	}
#endif
	statusLabel->setText(status);
	trustLabel->setText(QString::fromStdString(
		RsPeerTrustString(details.trustLvl)));

	{
		std::ostringstream out;
		out << details.localAddr << ":";
		out << details.localPort << "/";
		out << details.extAddr << ":";
		out << details.extPort;
		ipLabel->setText(QString::fromStdString(out.str()));
	}

	connLabel->setText(QString::fromStdString(details.autoconnect));
	QDateTime date = QDateTime::fromTime_t(details.lastConnect);
  QString stime = date.toString(Qt::LocalDate);
  lastLabel-> setText(stime);

	/* do buttons */
	chatButton->setEnabled(details.state & RS_PEER_STATE_CONNECTED);
	if (details.state & RS_PEER_STATE_FRIEND)
	{
		addButton->setEnabled(false);
		removeButton->setEnabled(true);
		msgButton->setEnabled(true);
	}
	else
	{
		addButton->setEnabled(true);
		removeButton->setEnabled(false);
		msgButton->setEnabled(false);
	}
		
	/* slow Tick  */
	int msec_rate = 10129;

	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	return;
}


void PeerItem::small()
{
	expandFrame->hide();
}

void PeerItem::toggle()
{
	if (expandFrame->isHidden())
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	    expandButton->setToolTip("Hide");
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	    expandButton->setToolTip("Expand");
	}
}


void PeerItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}


void PeerItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "PeerItem::gotoHome()";
	std::cerr << std::endl;
#endif
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

	if (mParent)
	{
		mParent->openMsg(FEEDHOLDER_MSG_MESSAGE, mPeerId, "");
	}
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

