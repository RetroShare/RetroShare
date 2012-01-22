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
#include "gui/settings/rsharesettings.h"
#include "gui/chat/ChatDialog.h"
#include "util/WidgetBackgroundImage.h"
#include "gui/common/AvatarDefs.h"

OnlineToaster::OnlineToaster(const std::string &peerId, const QString &name) : QWidget(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	this->peerId = peerId;

	/* connect buttons */
	connect(ui.messageButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(ui.closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
	ui.messageLabel->setText(name);

	QPixmap avatar;
	AvatarDefs::getAvatarFromSslId(peerId, avatar, ":/images/user/personal64.png");
	ui.pixmaplabel->setPixmap(avatar);

	WidgetBackgroundImage::setBackgroundImage(ui.windowFrame, ":images/toaster/backgroundtoaster.png", WidgetBackgroundImage::AdjustNone);
}

void OnlineToaster::chatButtonSlot()
{
	ChatDialog::chatFriend(peerId);
	hide();
}
