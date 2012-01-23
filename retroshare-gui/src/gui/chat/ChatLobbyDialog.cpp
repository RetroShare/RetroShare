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

#include "ChatLobbyDialog.h"
#include "ChatTabWidget.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/FriendsDialog.h"

#include <retroshare/rsnotify.h>

#include <time.h>

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const ChatLobbyId& lid, QWidget *parent, Qt::WFlags flags)
	: ChatDialog(parent, flags), lobbyId(lid)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	connect(ui.participantsFrameButton, SIGNAL(toggled(bool)), this, SLOT(showParticipantsFrame(bool)));
}

void ChatLobbyDialog::init(const std::string &peerId, const QString &title)
{
	ChatDialog::init(peerId, title);

	std::string nickName;
	rsMsgs->getNickNameForChatLobby(lobbyId, nickName);
	ui.chatWidget->setName(QString::fromUtf8(nickName.c_str()));

	lastUpdateListTime = 0;

	/* Hide or show the participants frames */
	showParticipantsFrame(PeerSettings->getShowParticipantsFrame(peerId));

	// add to window
	ChatTabWidget *tabWidget = FriendsDialog::getTabWidget();
	if (tabWidget) {
		tabWidget->addDialog(this);
	}

	// load settings
	processSettings(true);
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby

	// check that the lobby still exists.
	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(getPeerId(), lid)) {
		rsMsgs->unsubscribeChatLobby(lobbyId);
	}

	// save settings
	processSettings(false);
}

ChatWidget *ChatLobbyDialog::getChatWidget()
{
	return ui.chatWidget;
}

void ChatLobbyDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("ChatLobbyDialog"));

	if (load) {
		// load settings
	} else {
		// save settings
	}

	Settings->endGroup();
}

void ChatLobbyDialog::setNickName(const QString& nick)
{
	rsMsgs->setNickNameForChatLobby(lobbyId, nick.toUtf8().constData());
	ui.chatWidget->setName(nick);
}

void ChatLobbyDialog::addIncomingChatMsg(const ChatInfo& info)
{
	QDateTime sendTime = QDateTime::fromTime_t(info.sendTime);
	QDateTime recvTime = QDateTime::fromTime_t(info.recvTime);
	QString message = QString::fromStdWString(info.msg);
	QString name = QString::fromUtf8(info.peer_nickname.c_str());

	ui.chatWidget->addChatMsg(true, name, sendTime, recvTime, message, ChatWidget::TYPE_NORMAL);

	// also update peer list.

	time_t now = time(NULL);

	if (now > lastUpdateListTime) {
		lastUpdateListTime = now;
		updateParticipantsList();
	}
}

void ChatLobbyDialog::updateParticipantsList()
{
	ui.participantsList->clear();

	std::list<ChatLobbyInfo> linfos;
	rsMsgs->getChatLobbyList(linfos);

	std::list<ChatLobbyInfo>::const_iterator it(linfos.begin());
	for (; it!=linfos.end() && (*it).lobby_id != lobbyId; ++it);

	if (it != linfos.end()) {
		for (std::map<std::string,time_t>::const_iterator it2((*it).nick_names.begin()); it2 != (*it).nick_names.end(); ++it2) {
			ui.participantsList->addItem(QString::fromUtf8((it2->first).c_str()));
		}
	}
}

void ChatLobbyDialog::displayLobbyEvent(int event_type, const QString& nickname, const QString& str)
{
	switch (event_type) {
	case RS_CHAT_LOBBY_EVENT_PEER_LEFT:
		ui.chatWidget->addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 has left the lobby.").arg(str), ChatWidget::TYPE_NORMAL);
		break;
	case RS_CHAT_LOBBY_EVENT_PEER_JOINED:
		ui.chatWidget->addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 joined the lobby.").arg(str), ChatWidget::TYPE_NORMAL);
		break;
	case RS_CHAT_LOBBY_EVENT_PEER_STATUS:
		ui.chatWidget->updateStatusString(nickname + " %1", str);
		break;
	default:
		std::cerr << "ChatLobbyDialog::displayLobbyEvent() Unhandled lobby event type " << event_type << std::endl;
	}
	updateParticipantsList() ;
}

bool ChatLobbyDialog::canClose()
{
	// check that the lobby still exists.
	ChatLobbyId lid;
	if (!rsMsgs->isLobbyId(getPeerId(), lid)) {
		return true;
	}

	if (QMessageBox::Yes == QMessageBox::question(this, tr("Unsubscribe to lobby"), tr("Do you want to unsubscribe to this chat lobby?"), QMessageBox::Yes | QMessageBox::No)) {
		return true;
	}

	return false;
}

void ChatLobbyDialog::showDialog(uint chatflags)
{
	if (chatflags & RS_CHAT_FOCUS) {
		ChatTabWidget *tabWidget = FriendsDialog::getTabWidget();
		if (tabWidget) {
			tabWidget->setCurrentWidget(this);
		}
	}
}

void ChatLobbyDialog::showParticipantsFrame(bool show)
{
	ui.participantsFrame->setVisible(show);
	ui.participantsFrameButton->setChecked(show);

	if (show) {
		ui.participantsFrameButton->setToolTip(tr("Hide Participants"));
		ui.participantsFrameButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));
	} else {
		ui.participantsFrameButton->setToolTip(tr("Show Participants"));
		ui.participantsFrameButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
	}

	PeerSettings->setShowParticipantsFrame(getPeerId(), show);
}
