/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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

#include "ChatLobbyToaster.h"
#include "gui/chat/ChatDialog.h"
#include "util/HandleRichText.h"

#include <retroshare/rsmsgs.h>

ChatLobbyToaster::ChatLobbyToaster(const std::string &peerId, const QString &name, const QString &message) : QWidget(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	this->peerId = peerId;

	connect(ui.toasterButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(ui.closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
	ui.textLabel->setText(RsHtml().formatText(NULL, message, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_CLEANSTYLE));
	ui.avatarWidget->setFrameType(AvatarWidget::NORMAL_FRAME);
	ui.avatarWidget->setDefaultAvatar(":images/user/agt_forum64.png");

	QString lobbyName = RsHtml::plainText(name);

	std::list<ChatLobbyInfo> linfos;
	rsMsgs->getChatLobbyList(linfos);

	ChatLobbyId lobbyId;
	if (rsMsgs->isLobbyId(peerId, lobbyId)) {
		for (std::list<ChatLobbyInfo>::const_iterator it(linfos.begin()); it != linfos.end(); ++it) {
			if ((*it).lobby_id == lobbyId) {
				lobbyName += "@" + RsHtml::plainText(it->lobby_name);
				break;
			}
		}
	}
	ui.toasterLabel->setText(lobbyName);
}

void ChatLobbyToaster::chatButtonSlot()
{
	ChatDialog::chatFriend(peerId);
	hide();
}
