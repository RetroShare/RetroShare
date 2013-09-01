/*
 * RetroShare
 * Copyright (C) 2012 RetroShare Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "FriendRequestToaster.h"
#include "gui/FriendsDialog.h"
#include "gui/connect/ConnectFriendWizard.h"

#include <retroshare/rspeers.h>

FriendRequestToaster::FriendRequestToaster(const std::string &gpgId, const QString &sslName, const std::string &peerId)
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
	ui.avatarWidget->setFrameType(AvatarWidget::NORMAL_FRAME);
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
