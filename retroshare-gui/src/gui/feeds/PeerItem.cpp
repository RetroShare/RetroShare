/*******************************************************************************
 * gui/feeds/PeerItem.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
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

#include <QDateTime>
#include <QTimer>

#include "PeerItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/AvatarDefs.h"
#include "util/DateTime.h"

#include "gui/notifyqt.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsrtt.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PeerItem::PeerItem(FeedHolder *parent, uint32_t feedId, const RsPeerId &peerId, uint32_t type, bool isHome) :
    FeedItem(parent,feedId,NULL),
    mPeerId(peerId), mType(type), mIsHome(isHome)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);
  
    sendmsgButton->setEnabled(false);

    /* general ones */
    connect( expandButton, SIGNAL( clicked() ), this, SLOT( toggle() ) );
    connect( clearButton, SIGNAL( clicked() ), this, SLOT( removeItem() ) );

    /* specific ones */
    connect( chatButton, SIGNAL( clicked() ), this, SLOT( openChat() ) );
    connect( sendmsgButton, SIGNAL( clicked() ), this, SLOT( sendMsg() ) );

    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateItem()));

    avatar->setId(ChatId(mPeerId));// TODO: remove unnecesary converstation

    expandFrame->hide();

    updateItemStatic();
    updateItem();
}

uint64_t PeerItem::uniqueIdentifier() const
{
    return hash_64bits("PeerItem " + mPeerId.toStdString() + " " + QString::number(mType).toStdString()) ;
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
			title = tr("Connection refused by peer");
			break;
		case PEER_TYPE_NEW_FOF:
			title = tr("Friend of Friend");
			break;
		case PEER_TYPE_OFFSET:
			title = tr("Friend Time Offset");
			break;
		default:
			title = tr("Peer");
			break;
	}

	titleLabel->setText(title);

	RsPeerDetails details;
    if (rsPeers->getPeerDetails(RsPeerId(mPeerId), details))
	{
		/* set peer name */
		peerNameLabel->setText(QString::fromUtf8(details.name.c_str()));
		lastLabel-> setText(DateTime::formatLongDateTime(details.lastConnect));

		/* expanded Info */
		nameLabel->setText(QString::fromUtf8(details.name.c_str()));
		idLabel->setText(QString::fromStdString(details.id.toStdString()));
		locLabel->setText(QString::fromUtf8(details.location.c_str()));

		if (rsRtt)
		{
			double offset = rsRtt->getMeanOffset(RsPeerId(mPeerId));
			offsetLabel->setText(QString::number(offset,'f',2).append(" s"));
		}
	}
	else
	{
		peerNameLabel->setText(tr("Unknown peer"));
		statusLabel->setText(tr("Unknown"));
		titleLabel->setText(tr("Unknown peer"));
		trustLabel->setText(tr("Unknown"));
		nameLabel->setText(tr("Unknown"));
		idLabel->setText(tr("Unknown"));
		locLabel->setText(tr("Unknown"));
		ipLabel->setText(tr("Unknown"));
		connLabel->setText(tr("Unknown"));
		lastLabel->setText(tr("Unknown"));

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
			sendmsgButton->setEnabled(false);

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

        QString ip_string;

        if(details.localPort != 0)
            ip_string += QString("%1:%2").arg(QString::fromStdString(details.localAddr)).arg(details.localPort);

        if(details.extPort != 0)
        {
            if(!ip_string.isNull())
                ip_string += "/" ;

            ip_string += ip_string += QString("%1:%2").arg(QString::fromStdString(details.extAddr)).arg(details.extPort);
        }
		ipLabel->setText(ip_string);

		connLabel->setText(StatusDefs::connectStateString(details));

		/* do buttons */
		chatButton->setEnabled(details.state & RS_PEER_STATE_CONNECTED);
		if (details.state & RS_PEER_STATE_CONNECTED)
		{
			sendmsgButton->setEnabled(true);
		}
		else
		{
			sendmsgButton->setEnabled(false);
		}

		if (rsRtt)
		{
			double offset = rsRtt->getMeanOffset(RsPeerId(mPeerId));
			offsetLabel->setText(QString::number(offset,'f',2).append(" s"));
		}

	}

	/* slow Tick  */
	int msec_rate = 10129;
	
	QTimer::singleShot( msec_rate, this, SLOT(updateItem() ));
	return;
}

void PeerItem::toggle()
{
	expand(expandFrame->isHidden());
}

void PeerItem::doExpand(bool open)
{
	if (mFeedHolder) {
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		expandFrame->show();
        expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		expandFrame->hide();
        expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder) {
		mFeedHolder->lockLayout(this, false);
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

    nMsgDialog->addRecipient(MessageComposer::TO, mPeerId);
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
	if (mFeedHolder)
	{
		mFeedHolder->openChat(mPeerId);
	}
}

