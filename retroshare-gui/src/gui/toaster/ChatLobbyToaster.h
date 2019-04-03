/*******************************************************************************
 * gui/toaster/ChatLobbyToaster.h                                              *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef CHATLOBBYTOASTER_H
#define CHATLOBBYTOASTER_H

#include "ui_ChatLobbyToaster.h"

#include "retroshare/rsmsgs.h"

/**
 * Shows a toaster when a chat is incoming.
 *
 * 
 */
class ChatLobbyToaster : public QWidget
{
	Q_OBJECT

public:
	ChatLobbyToaster(const  ChatLobbyId &lobby_id, const RsGxsId &sender_id, const QString &message);

private slots:
	void chatButtonSlot();

private:
    ChatLobbyId mLobbyId;

	/** Qt Designer generated object */
	Ui::ChatLobbyToaster ui;
};

#endif	//CHATLOBBYTOASTER_H
