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
#include <retroshare/rsidentity.h>

#include <retroshare/rsmsgs.h>

ChatLobbyToaster::ChatLobbyToaster(const ChatLobbyId &lobby_id, const RsGxsId &sender_id, const QString &message):
    QWidget(NULL), mLobbyId(lobby_id)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	connect(ui.toasterButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(ui.closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
	//ui.textLabel->setText(RsHtml().formatText(NULL, message, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_CLEANSTYLE));
    // Disabled, because emoticon replacement kills performance
	ui.textLabel->setText(RsHtml().formatText(NULL, message, RSHTML_FORMATTEXT_CLEANSTYLE));
	ui.avatarWidget->setFrameType(AvatarWidget::NORMAL_FRAME);
	ui.avatarWidget->setDefaultAvatar(":images/chat_64.png");

	/* Get sender info */
	RsIdentityDetails idd;
	if(!rsIdentity->getIdDetails(sender_id, idd))
		return;

    ui.avatarWidget->setGxsId(sender_id);

	QString lobbyName = RsHtml::plainText(idd.mNickname);

    ChatLobbyInfo clinfo ;
    if(rsMsgs->getChatLobbyInfo(mLobbyId,clinfo))
            lobbyName += "@" + RsHtml::plainText(clinfo.lobby_name);

	ui.toasterLabel->setText(lobbyName);
}

void ChatLobbyToaster::chatButtonSlot()
{
    ChatDialog::chatFriend(ChatId(mLobbyId));
	hide();
}
