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
#include <QListWidget>
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
#include "gui/ChatLobbyWidget.h"

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const std::string& dialog_id, const ChatLobbyId& lid, const QString &name, QWidget *parent, Qt::WFlags flags)
	: PopupChatDialog(dialog_id, name, parent, flags), lobby_id(lid)
{
	// remove the avatar widget. Replace it with a friends list.
	ui.avatarWidget->hide();
	ui.ownAvatarWidget->hide();
	PopupChatDialog::updateStatus(QString::fromStdString(getPeerId()),RS_STATUS_ONLINE);

    // hide history buttons
    ui.actionClearOfflineMessages->setVisible(false);
    ui.actionDelete_Chat_History->setVisible(false);
    ui.actionMessageHistory->setVisible(false);

    ui.avatarFrameButton->setToolTip(tr("Hide participants"));

	ui.avatarframe->layout()->addWidget(new QLabel(tr("Participants:")));
	friendsListWidget = new QListWidget;
	ui.avatarframe->layout()->addWidget(friendsListWidget);
	ui.avatarframe->layout()->addItem(new QSpacerItem(12, 335, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby

	// check that the lobby still exists.
	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(getPeerId(),lid)) {
		rsMsgs->unsubscribeChatLobby(lobby_id);
	}
}

bool ChatLobbyDialog::addToParent()
{
	ChatTabWidget *tabWidget = ChatLobbyWidget::getTabWidget();
	if (tabWidget) {
		tabWidget->addDialog(this);
		return true;
	}

	return false;
}

void ChatLobbyDialog::setNickName(const QString& nick)
{
	rsMsgs->setNickNameForChatLobby(lobby_id, nick.toUtf8().constData());
}

void ChatLobbyDialog::addIncomingChatMsg(const ChatInfo& info)
{
	QDateTime sendTime = QDateTime::fromTime_t(info.sendTime);
	QDateTime recvTime = QDateTime::fromTime_t(info.recvTime);
	QString message = QString::fromStdWString(info.msg);
	QString name = QString::fromUtf8(info.peer_nickname.c_str());

	addChatMsg(true, name, sendTime, recvTime, message, TYPE_NORMAL);

	// also update peer list.

	static time_t last = 0 ;
	time_t now = time(NULL) ;

	if (now > last) {
		last = now;
		updateFriendsList();
	}
}

void ChatLobbyDialog::updateFriendsList()
{
	friendsListWidget->clear();

	std::list<ChatLobbyInfo> linfos;
	rsMsgs->getChatLobbyList(linfos);

	std::list<ChatLobbyInfo>::const_iterator it(linfos.begin());
	for (; it!=linfos.end() && (*it).lobby_id != lobby_id; ++it);

	if (it != linfos.end()) {
		for (std::set<std::string>::const_iterator it2((*it).nick_names.begin()); it2 != (*it).nick_names.end(); ++it2) {
			friendsListWidget->addItem(QString::fromUtf8((*it2).c_str()));
		}
	}
}

void ChatLobbyDialog::displayLobbyEvent(int event_type, const QString& nickname, const QString& str)
{
	switch(event_type)
	{
		case RS_CHAT_LOBBY_EVENT_PEER_LEFT:
			addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 has left the lobby.").arg(str), TYPE_NORMAL);
			break;
		case RS_CHAT_LOBBY_EVENT_PEER_JOINED:
			addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 joined the lobby.").arg(str), TYPE_NORMAL);
			break;
		case RS_CHAT_LOBBY_EVENT_PEER_STATUS:
			updateStatusString(nickname,str);
			break;
		default:
			std::cerr << "ChatLobbyDialog::displayLobbyEvent() Unhandled lobby event type " << event_type << std::endl;
	}
}

QString ChatLobbyDialog::makeStatusString(const QString& peer_id, const QString& status_string) const
{
	 return QString::fromUtf8(peer_id.toStdString().c_str()) + " " + tr(status_string.toAscii());
}

bool ChatLobbyDialog::canClose()
{
	// check that the lobby still exists.
	ChatLobbyId lid;
	if (!rsMsgs->isLobbyId(getPeerId(),lid)) {
		return true;
	}

	if (QMessageBox::Yes == QMessageBox::question(NULL, tr("Unsubscribe to lobby?"), tr("Do you want to unsubscribe to this chat lobby?"), QMessageBox::Yes | QMessageBox::No)) {
		return true;
	}

	return false;
}
