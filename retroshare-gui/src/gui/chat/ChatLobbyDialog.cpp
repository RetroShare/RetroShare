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
#include <retroshare/rsstatus.h>

#include <time.h>
#include <algorithm>

#include "ChatLobbyDialog.h"

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const std::string& dialog_id,const ChatLobbyId& lid, const QString &name, QWidget *parent, Qt::WFlags flags)
	: PopupChatDialog(dialog_id,name,parent,flags),lobby_id(lid)
{
	// remove the avatar widget. Replace it with a friends list.
	
	ui.avatarWidget->hide() ;
	PopupChatDialog::updateStatus(QString::fromStdString(getPeerId()),RS_STATUS_ONLINE) ;

	QObject::connect(this,SIGNAL(close()),this,SLOT(closeAndAsk())) ;
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby
	
	if(QMessageBox::Yes == QMessageBox::question(NULL,tr("Unsubscribe to lobby?"),tr("Do you want to unsubscribe to this chat lobby?"),QMessageBox::Yes | QMessageBox::No))
		rsMsgs->unsubscribeChatLobby(lobby_id) ;
}

void ChatLobbyDialog::setNickName(const QString& nick)
{
	rsMsgs->setNickNameForChatLobby(lobby_id,nick.toStdString()) ;
}

void ChatLobbyDialog::updateStatus(const QString &peer_id, int status)
{
	// For now. We need something more efficient to tell when the lobby is disconnected.
	//
}

