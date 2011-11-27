/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, csoler  
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

#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QFontDialog>
#include <QDir>
#include <QBuffer>
#include <QTextCodec>
#include <QSound>
#include <sys/stat.h>

#include "util/misc.h"
#include "rshare.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <time.h>
#include <algorithm>

#include "ChatLobbyDialog.h"

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const ChatLobbyId& lid, const QString &name, QWidget *parent, Qt::WFlags flags)
	: PopupChatDialog(("Chat lobby 0x"+QString::number(lobby_id,16)).toStdString(),name,parent,flags),lobby_id(lid)
{
	// remove the avatar widget. Replace it with a friends list.
	
	ui.avatarWidget->hide() ;
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby
	
	rsMsgs->unsubscribeChatLobby(lobby_id) ;
}

void ChatLobbyDialog::setNickName(const QString& nick)
{
	rsMsgs->setNickNameForChatLobby(lobby_id,nick.toStdString()) ;
}

bool ChatLobbyDialog::sendPrivateChat(const std::wstring& msg)
{
	return rsMsgs->sendLobbyChat(msg,lobby_id) ;
}

