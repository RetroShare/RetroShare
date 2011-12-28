/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _CHATLOBBYDIALOG_H
#define _CHATLOBBYDIALOG_H

#include "ui_PopupChatDialog.h"

class QAction;
class QTextEdit;
class QTextCharFormat;
class AttachFileItem;
class ChatInfo;

#include <retroshare/rsmsgs.h>
#include "ChatStyle.h"
#include "gui/style/RSStyle.h"
#include "PopupChatDialog.h"

class ChatLobbyDialog: public PopupChatDialog 
{
	Q_OBJECT 

	protected:
		/** Default constructor */
		ChatLobbyDialog(const std::string& id,const ChatLobbyId& lid, const QString &name, QWidget *parent = 0, Qt::WFlags flags = 0);

		/** Default destructor */
		virtual ~ChatLobbyDialog();

//		virtual void addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, enumChatType chatType);
//		virtual void sendChat();

		friend class PopupChatDialog ;

		// The following methods are differentfrom those of the parent:
		//
		virtual void updateStatus(const QString &peer_id, int status) ;	// needs grouped status. Not yet implemented.
		virtual void addIncomingChatMsg(const ChatInfo& info) ;				// 

	protected slots:
		void setNickName(const QString&) ;

	private:
		ChatLobbyId lobby_id ;
};

#endif
