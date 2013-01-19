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
#include <QMessageBox>

#include "SecurityItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/StatusDefs.h"
#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "gui/common/AvatarDefs.h"
#include "util/DateTime.h"

#include "gui/notifyqt.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
SecurityItem::SecurityItem(FeedHolder *parent, uint32_t feedId, const std::string &gpgId, const std::string &sslId, const std::string &sslCn, const std::string& ip_address,uint32_t type, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mGpgId(gpgId), mSslId(sslId), mSslCn(sslCn), mType(type), mIsHome(isHome), mIP(ip_address)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);
  
    messageframe->setVisible(false);
    sendmsgButton->setEnabled(false);
    quickmsgButton->hide();
    chatButton->hide();
    removeFriendButton->setEnabled(false);
    removeFriendButton->hide();
    peerDetailsButton->setEnabled(false);
    friendRequesttoolButton->hide();
    requestLabel->hide();

    /* general ones */
    connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
    connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

    /* specific ones */
    connect( chatButton, SIGNAL( clicked( void ) ), this, SLOT( openChat ( void ) ) );
    connect( actionNew_Message, SIGNAL( triggered( ) ), this, SLOT( sendMsg ( void ) ) );

    connect( quickmsgButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );
    connect( cancelButton, SIGNAL( clicked( ) ), this, SLOT( togglequickmessage() ) );

    connect( sendmsgButton, SIGNAL( clicked( ) ), this, SLOT( sendMessage() ) );
    connect( removeFriendButton, SIGNAL(clicked()), this, SLOT(removeFriend()));
    connect( peerDetailsButton, SIGNAL(clicked()), this, SLOT(peerDetails()));
    connect( friendRequesttoolButton, SIGNAL(clicked()), this, SLOT(friendRequest()));

    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateItem()));

    QMenu *msgmenu = new QMenu();
    msgmenu->addAction(actionNew_Message);

    quickmsgButton->setMenu(msgmenu);

    avatar->setId(mSslId, false);

    expandFrame->hide();

    updateItemStatic();
    updateItem();
}


bool SecurityItem::isSame(const std::string &sslId, uint32_t type)
{
	if ((mSslId == sslId) && (mType == type))
	{
		return true;
	}
	return false;
}


void SecurityItem::updateItemStatic()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::updateItemStatic()";
	std::cerr << std::endl;
#endif
	QString title;

	switch(mType)
	{
		case SEC_TYPE_CONNECT_ATTEMPT:
			title = tr("Connect Attempt");
			requestLabel->show();
			avatar->setDefaultAvatar(":images/avatar_request.png");
			break;
		case SEC_TYPE_AUTH_DENIED:
			title = tr("Not Yet Friends");
			requestLabel->hide();
			avatar->setDefaultAvatar(":images/avatar_request.png");
			break;
		case SEC_TYPE_UNKNOWN_IN:
			title = tr("Unknown (Incoming) Connect Attempt");
			requestLabel->hide();
			avatar->setDefaultAvatar(":images/avatar_request_unknown.png");
			break;
		case SEC_TYPE_UNKNOWN_OUT:
			title = tr("Unknown (Outgoing) Connect Attempt");
			requestLabel->hide();
			break;
		default:
			title = tr("Unknown Security Issue");
			requestLabel->hide();
			break;
	}

	titleLabel->setText(title);

	QDateTime currentTime = QDateTime::currentDateTime();
	timeLabel->setText(DateTime::formatLongDateTime(currentTime.toTime_t()));

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);

		/* hide buttons */
		clearButton->hide();
	}
}

void SecurityItem::updateItem()
{
	if (!rsPeers)
		return;

	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::updateItem()";
	std::cerr << std::endl;
#endif

	if(!RsAutoUpdatePage::eventsLocked()) {
		RsPeerDetails details;
		/* first try sslid */
		if (!rsPeers->getPeerDetails(mSslId, details))
		{
			/* then gpgid */
			if (!rsPeers->getPeerDetails(mGpgId, details))
			{
				/* it is very likely that we will end up here for some of the
				 * Unknown peer cases.... so allow them here
				 */

				/* set peer name */
				peerNameLabel->setText(QString("%1 (%2)").arg(tr("Unknown Peer"), QString::fromUtf8(mSslCn.c_str())));

				nameLabel->setText(QString::fromUtf8(mSslCn.c_str()) + " (" + QString::fromStdString(mGpgId) + ")");
				idLabel->setText(QString::fromStdString(mSslId));

				statusLabel->setText(tr("Unknown Peer"));
				trustLabel->setText(tr("Unknown Peer"));
				locLabel->setText(tr("Unknown Peer"));
				ipLabel->setText(QString::fromStdString(mIP)) ; //tr("Unknown Peer"));
				connLabel->setText(tr("Unknown Peer"));

				chatButton->hide();
				quickmsgButton->hide();
				requestLabel->hide();


                removeFriendButton->setEnabled(false);
                removeFriendButton->hide();
                peerDetailsButton->setEnabled(false);

				return;
			}
		}

		/* set peer name */
		peerNameLabel->setText(QString::fromUtf8(details.name.c_str()));

		/* expanded Info */
		nameLabel->setText(QString::fromUtf8(details.name.c_str()) + " (" + QString::fromStdString(mGpgId) + ")");
		//idLabel->setText(QString::fromStdString(details.id));
		idLabel->setText(QString::fromStdString(mSslId));
		locLabel->setText(QString::fromUtf8(details.location.c_str()));

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

		ipLabel->setText(QString::fromStdString(mIP)) ; //QString("%1:%2/%3:%4").arg(QString::fromStdString(details.localAddr)).arg(details.localPort).arg(QString::fromStdString(details.extAddr)).arg(details.extPort));

		connLabel->setText(StatusDefs::connectStateString(details));

		/* do buttons */
		peerDetailsButton->setEnabled(true);
		
		if (details.state & RS_PEER_STATE_CONNECTED)
		{
			chatButton->show();
		}
		else
		{
			chatButton->hide();
		}

		if (details.accept_connection)
		{
			friendRequesttoolButton->hide();
			requestLabel->hide();
			removeFriendButton->setEnabled(true);
			removeFriendButton->show();
		}
		else
		{
			friendRequesttoolButton->show();
			requestLabel->show();
			removeFriendButton->setEnabled(false);
			removeFriendButton->hide();
		}

		if (details.state & RS_PEER_STATE_FRIEND)
		{
			quickmsgButton->show();
		}
		else
		{
			quickmsgButton->hide();
		}
	}

	/* slow Tick  */
	int msec_rate = 10129;

	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	return;
}

void SecurityItem::toggle()
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

void SecurityItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::removeItem()";
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

void SecurityItem::removeFriend()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::removeFriend()";
	std::cerr << std::endl;
#endif

	if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this Friend?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
	{
		rsPeers->removeFriend(mGpgId);
	}
}

void SecurityItem::friendRequest()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::friendReguest()";
	std::cerr << std::endl;
#endif

	ConnectFriendWizard *connectFriendWizard = new ConnectFriendWizard;
	connectFriendWizard->setAttribute(Qt::WA_DeleteOnClose, true);
	connectFriendWizard->setGpgId(mGpgId, true);
	connectFriendWizard->show();
}

void SecurityItem::peerDetails()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::peerDetails()";
	std::cerr << std::endl;
#endif

	RsPeerDetails details;
	/* first try sslid */
	if (rsPeers->getPeerDetails(mSslId, details))
	{
		ConfCertDialog::showIt(mSslId, ConfCertDialog::PageDetails);
		return;
	}

	/* then gpgid */
	if (rsPeers->getPeerDetails(mGpgId, details))
	{
		ConfCertDialog::showIt(mGpgId, ConfCertDialog::PageDetails);
	}
}

void SecurityItem::sendMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::sendMsg()";
	std::cerr << std::endl;
#endif

	MessageComposer *nMsgDialog = MessageComposer::newMsg();
	if (nMsgDialog == NULL) {
		return;
	}

	nMsgDialog->addRecipient(MessageComposer::TO, mGpgId, false);
	nMsgDialog->show();
	nMsgDialog->activateWindow();

	/* window will destroy itself! */
}

void SecurityItem::openChat()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::openChat()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		mParent->openChat(mGpgId);
	}
}

void SecurityItem::togglequickmessage()
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

void SecurityItem::sendMessage()
{
	/* construct a message */
	MessageInfo mi;

	mi.title = tr("Quick Message").toStdWString();
	mi.msg =   quickmsgText->toHtml().toStdWString();
	mi.msgto.push_back(mGpgId);

	rsMsgs->MessageSend(mi);

	quickmsgText->clear();
	messageframe->setVisible(false);
}

void SecurityItem::on_quickmsgText_textChanged()
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
