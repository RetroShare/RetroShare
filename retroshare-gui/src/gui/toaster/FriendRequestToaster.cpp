/*******************************************************************************
 * gui/toaster/FriendRequestToaster.cpp                                        *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "FriendRequestToaster.h"
#include "gui/FriendsDialog.h"
#include "gui/connect/ConnectFriendWizard.h"

#include <retroshare/rspeers.h>

FriendRequestToaster::FriendRequestToaster(const RsPgpId &gpgId, const QString &sslName, const RsPeerId &peerId)
	: QWidget(NULL), mGpgId(gpgId), mSslId(peerId), mSslName(sslName)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	bool knownPeer = false;
	RsPeerDetails details;
	if (rsPeers->getGPGDetails(mGpgId, details)) {
		knownPeer = true;
	}

	if (knownPeer) {
		connect(ui.toasterButton, SIGNAL(clicked()), SLOT(friendrequestButtonSlot()));
	} else {
		ui.toasterButton->setEnabled(false);
	}
	connect(ui.closeButton, SIGNAL(clicked()), SLOT(hide()));
	
	QString peerName = QString::fromUtf8(details.name.c_str());

	/* set informations */
	ui.avatarWidget->setFrameType(AvatarWidget::NO_FRAME);
	if (knownPeer) {
		ui.textLabel->setText( peerName + " " + tr("wants to be friend with you on RetroShare"));
		ui.avatarWidget->setDefaultAvatar(":/images/avatar_request.png");
	} else {
		ui.textLabel->setText( sslName + " " + tr("Unknown (Incoming) Connect Attempt"));
		ui.avatarWidget->setDefaultAvatar(":/images/avatar_request_unknown.png");
	}
}

void FriendRequestToaster::friendrequestButtonSlot()
{
	ConnectFriendWizard *connectFriendWizard = new ConnectFriendWizard;
	connectFriendWizard->setAttribute(Qt::WA_DeleteOnClose, true);
	connectFriendWizard->setGpgId(mGpgId, mSslId, true);
	connectFriendWizard->show();

	hide();
}
