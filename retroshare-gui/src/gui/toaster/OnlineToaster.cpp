/*
 * RetroShare
 * Copyright (C) 2006,2007  crypton
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

#include "OnlineToaster.h"
#include "gui/chat/ChatDialog.h"

#include <retroshare/rspeers.h>

OnlineToaster::OnlineToaster(const RsPeerId &peerId) : QWidget(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	this->peerId = peerId;

	/* connect buttons */
	connect(ui.toasterButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(ui.closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
    //ui.textLabel->setText(QString::fromUtf8(rsPeers->getPeerName(peerId).c_str()));
    ui.textLabel->setText(QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGId(peerId)).c_str()));
    ui.avatarWidget->setId(ChatId(peerId));
    ui.avatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);
}

void OnlineToaster::chatButtonSlot()
{
    ChatDialog::chatFriend(ChatId(peerId));
	hide();
}
