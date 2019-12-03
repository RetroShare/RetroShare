/*******************************************************************************
 * gui/feeds/SecurityItem.cpp                                                  *
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
#include <QMessageBox>

#include "SecurityItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/StatusDefs.h"
#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/PGPKeyDialog.h"
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
SecurityItem::SecurityItem(FeedHolder *parent, uint32_t feedId, const RsPgpId &gpgId, const RsPeerId &sslId, const std::string &sslCn, const std::string& ip_address,uint32_t type, bool isHome) :
    FeedItem(NULL), mParent(parent), mFeedId(feedId),
    mGpgId(gpgId), mSslId(sslId), mSslCn(sslCn), mIP(ip_address), mType(type), mIsHome(isHome)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	//quickmsgButton->hide();
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

	//connect( quickmsgButton, SIGNAL( clicked( ) ), this, SLOT( sendMsg() ) );

	connect( removeFriendButton, SIGNAL(clicked()), this, SLOT(removeFriend()));
	connect( peerDetailsButton, SIGNAL(clicked()), this, SLOT(peerDetails()));
	connect( friendRequesttoolButton, SIGNAL(clicked()), this, SLOT(friendRequest()));

	connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateItem()));

    avatar->setId(ChatId(mSslId));

	expandFrame->hide();

	updateItemStatic();
	updateItem();
}

QString SecurityItem::uniqueIdentifier() const
{
    return "SecurityItem " + QString::number(mType) + " " + QString::fromStdString(mSslId.toStdString());
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
		case RS_FEED_ITEM_SEC_CONNECT_ATTEMPT:
			title = tr("Connect Attempt");
			requestLabel->show();
			avatar->setDefaultAvatar(":images/avatar_request.png");
			break;
		case RS_FEED_ITEM_SEC_AUTH_DENIED:
			title = tr("Connection refused by remote peer");
			requestLabel->hide();
			avatar->setDefaultAvatar(":images/avatar_refused.png");
			break;
		case RS_FEED_ITEM_SEC_UNKNOWN_IN:
			title = tr("Unknown (Incoming) Connect Attempt");
			requestLabel->hide();
			avatar->setDefaultAvatar(":images/avatar_request_unknown.png");
			break;
		case RS_FEED_ITEM_SEC_UNKNOWN_OUT:
			title = tr("Unknown (Outgoing) Connect Attempt");
			requestLabel->hide();
			break;
		case RS_FEED_ITEM_SEC_WRONG_SIGNATURE:
			title = tr("Certificate has wrong signature!! This peer is not who he claims to be.");
			requestLabel->hide();
			break;
		case RS_FEED_ITEM_SEC_MISSING_CERTIFICATE:
            title = tr("Peer/node not in friendlist (PGP id=")+QString::fromStdString(mGpgId.toStdString())+")";
			avatar->setDefaultAvatar(":images/avatar_request_unknown.png");
			requestLabel->show();
			break;
		case RS_FEED_ITEM_SEC_BAD_CERTIFICATE:
			{
			RsPeerDetails details ;
            if(rsPeers->getGPGDetails(mGpgId, details))
                title = tr("Missing/Damaged SSL certificate for key")+" " + QString::fromStdString(mGpgId.toStdString()) ;
			else
				title = tr("Missing/Damaged certificate. Not a real Retroshare user.");
			requestLabel->hide();
			}
			break;
		case RS_FEED_ITEM_SEC_INTERNAL_ERROR:
			title = tr("Certificate caused an internal error.");
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
            if(!rsPeers->getGPGDetails(mGpgId, details))
			{
				/* it is very likely that we will end up here for some of the
				 * Unknown peer cases.... so allow them here
				 */

				/* set peer name */
				peerNameLabel->setText(QString("%1 (%2)").arg(tr("Unknown Peer"), QString::fromUtf8(mSslCn.c_str())));

                nameLabel->setText(QString::fromUtf8(mSslCn.c_str()) + " (" + QString::fromStdString(mGpgId.toStdString()) + ")");
                idLabel->setText(QString::fromStdString(mSslId.toStdString()));

				statusLabel->setText(tr("Unknown Peer"));
				trustLabel->setText(tr("Unknown Peer"));
				locLabel->setText(tr("Unknown Peer"));
				ipLabel->setText(QString::fromStdString(mIP)) ; //tr("Unknown Peer"));
				connLabel->setText(tr("Unknown Peer"));

				chatButton->hide();
				//quickmsgButton->hide();
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
        nameLabel->setText(QString::fromUtf8(details.name.c_str()) + " (" + QString::fromStdString(mGpgId.toStdString()) + ")");
		//idLabel->setText(QString::fromStdString(details.id));
        idLabel->setText(QString::fromStdString(mSslId.toStdString()));
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

		//quickmsgButton->show();
	}

	/* slow Tick  */
	int msec_rate = 10129;

	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	return;
}

void SecurityItem::toggle()
{
	expand(expandFrame->isHidden());
}

void SecurityItem::doExpand(bool open)
{
	if (mParent) {
		mParent->lockLayout(this, true);
	}

	if (open)
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/icons/png/up-arrow.png")));
		expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/icons/png/down-arrow.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mParent) {
		mParent->lockLayout(this, false);
	}
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
	connectFriendWizard->setGpgId(mGpgId, mSslId, true);
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
    if (rsPeers->getGPGDetails(mGpgId, details))
	{
        PGPKeyDialog::showIt(mGpgId, PGPKeyDialog::PageDetails);
	}
}

void SecurityItem::sendMsg()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::sendMsg()";
	std::cerr << std::endl;
#endif

    std::cerr << __PRETTY_FUNCTION__ << ": suspended for now." << std::endl;
#ifdef SUSPENDED
	MessageComposer *nMsgDialog = MessageComposer::newMsg();
	if (nMsgDialog == NULL) {
		return;
	}

    RsPeerId peerId ;

    if(rsMsgs->getDistantMessagePeerId(mGpgId,peerId))
	{
        nMsgDialog->addRecipient(MessageComposer::TO, peerId, mGpgId);
		nMsgDialog->show();
		nMsgDialog->activateWindow();
	}

    /* window will destroy itself! */
#endif
}

void SecurityItem::openChat()
{
#ifdef DEBUG_ITEM
	std::cerr << "SecurityItem::openChat()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
        mParent->openChat(mSslId);
	}
}
