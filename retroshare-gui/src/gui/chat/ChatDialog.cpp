/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, RetroShare Team
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

#include "ChatDialog.h"
#include "gui/common/PeerDefs.h"
#include "PopupChatDialog.h"
#include "ChatLobbyDialog.h"
#include "PopupChatWindow.h"
#include "gui/settings/rsharesettings.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/SoundManager.h"

#include <retroshare/rsiface.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

static std::map<std::string, ChatDialog*> chatDialogs;

ChatDialog::ChatDialog(QWidget *parent, Qt::WFlags flags) :
	QWidget(parent, flags)
{
}

ChatDialog::~ChatDialog()
{
	emit dialogClose(this);

	std::map<std::string, ChatDialog *>::iterator it;
	if (chatDialogs.end() != (it = chatDialogs.find(getPeerId()))) {
		chatDialogs.erase(it);
	}
}

void ChatDialog::init(const std::string &peerId, const QString &title)
{
	this->peerId = peerId;

	ChatWidget *cw = getChatWidget();
	if (cw) {
		cw->init(peerId, title);

		connect(cw, SIGNAL(infoChanged(ChatWidget*)), this, SLOT(chatInfoChanged(ChatWidget*)));
		connect(cw, SIGNAL(newMessage(ChatWidget*)), this, SLOT(chatNewMessage(ChatWidget*)));
	}
}

/*static*/ ChatDialog *ChatDialog::getExistingChat(const std::string &peerId)
{
	std::map<std::string, ChatDialog*>::iterator it;
	if (chatDialogs.end() != (it = chatDialogs.find(peerId))) {
		/* exists already */
		return it->second;
	}

	return NULL;
}

/*static*/ ChatDialog *ChatDialog::getChat(const std::string &peerId, uint chatflags)
{
	/* see if it already exists */
	ChatDialog *cd = getExistingChat(peerId);

	if (cd == NULL) {
		ChatLobbyId lobby_id;
		if (rsMsgs->isLobbyId(peerId, lobby_id)) {
			chatflags = RS_CHAT_OPEN | RS_CHAT_FOCUS; // use own flags
		}

		if (chatflags & RS_CHAT_OPEN) {
			if (lobby_id) {
				std::list<ChatLobbyInfo> linfos;
				rsMsgs->getChatLobbyList(linfos);

				for (std::list<ChatLobbyInfo>::const_iterator it(linfos.begin()); it != linfos.end(); ++it) {
					if ((*it).lobby_id == lobby_id) {
						cd = new ChatLobbyDialog(lobby_id);
						chatDialogs[peerId] = cd;
						cd->init(peerId, QString::fromUtf8((*it).lobby_name.c_str()));
					}
				}
			} else {
				RsPeerDetails sslDetails;
				if (rsPeers->getPeerDetails(peerId, sslDetails)) {
					cd = new PopupChatDialog();
					chatDialogs[peerId] = cd;
					cd->init(peerId, PeerDefs::nameWithLocation(sslDetails));
				}
			}
		}
	}

	if (cd == NULL) {
		return NULL;
	}

	cd->insertChatMsgs();
	cd->showDialog(chatflags);

	return cd;
}

/*static*/ void ChatDialog::cleanupChat()
{
	PopupChatWindow::cleanup();

	/* ChatDialog destuctor removes the entry from the map */
	std::list<ChatDialog*> list;

	std::map<std::string, ChatDialog*>::iterator it;
	for (it = chatDialogs.begin(); it != chatDialogs.end(); it++) {
		if (it->second) {
			list.push_back(it->second);
		}
	}

	chatDialogs.clear();

	std::list<ChatDialog*>::iterator it1;
	for (it1 = list.begin(); it1 != list.end(); it1++) {
		delete (*it1);
	}
}

/*static*/ void ChatDialog::chatChanged(int list, int type)
{
	if (list == NOTIFY_LIST_PRIVATE_INCOMING_CHAT && type == NOTIFY_TYPE_ADD) {
		// play sound when recv a message
		soundManager->play(SoundManager::NEW_CHAT_MESSAGE);

		std::list<std::string> ids;
		if (rsMsgs->getPrivateChatQueueIds(true, ids)) {
			uint chatflags = Settings->getChatFlags();

			std::list<std::string>::iterator id;
			for (id = ids.begin(); id != ids.end(); id++) {
				ChatDialog *cd = getChat(*id, chatflags);

				if (cd) {
					cd->insertChatMsgs();
				}
			}
		}
	}

	/* now notify all open priavate chat windows */
	std::map<std::string, ChatDialog *>::iterator it;
	for (it = chatDialogs.begin (); it != chatDialogs.end(); it++) {
		if (it->second) {
			it->second->onChatChanged(list, type);
		}
	}
}

/*static*/ void ChatDialog::closeChat(const std::string& peerId)
{
	ChatDialog *chatDialog = getExistingChat(peerId);

	if (chatDialog) {
		delete(chatDialog);
	}
}

/*static*/ void ChatDialog::chatFriend(const std::string &peerId)
{
	if (peerId.empty()){
		return;
	}

	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(peerId, lid)) {
		getChat(peerId, RS_CHAT_OPEN | RS_CHAT_FOCUS);
		return;
	}

	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(peerId, detail))
		return;

	std::string firstId;

	if (detail.isOnlyGPGdetail) {
		//let's get the ssl child details, and open all the chat boxes
		std::list<std::string> sslIds;
		rsPeers->getAssociatedSSLIds(detail.gpg_id, sslIds);
		for (std::list<std::string>::iterator it = sslIds.begin(); it != sslIds.end(); it++) {
			if (firstId.empty()) {
				firstId = *it;
			}

			RsPeerDetails sslDetails;
			if (rsPeers->getPeerDetails(*it, sslDetails)) {
				if (sslDetails.state & RS_PEER_STATE_CONNECTED) {
					getChat(*it, RS_CHAT_OPEN | RS_CHAT_FOCUS);
					return;
				}
			}
		}
	} else {
		if (detail.state & RS_PEER_STATE_CONNECTED) {
			getChat(peerId, RS_CHAT_OPEN | RS_CHAT_FOCUS);
			return;
		}
		firstId = peerId;
	}

	/* info dialog */
	QMessageBox mb(QMessageBox::Question, tr("Friend not Online"), tr("Your Friend is offline \nDo you want to send them a Message instead"), QMessageBox::Yes | QMessageBox::No);
	mb.setWindowIcon(QIcon(":/images/rstray3.png"));
	if (mb.exec() == QMessageBox::Yes) {
		MessageComposer::msgFriend(peerId, false);
	} else {
		if (firstId.empty() == false) {
			getChat(firstId, RS_CHAT_OPEN | RS_CHAT_FOCUS);
		}
	}
}

void ChatDialog::addToParent(QWidget *newParent)
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		cw->addToParent(newParent);
	}
}

void ChatDialog::removeFromParent(QWidget *oldParent)
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		cw->removeFromParent(oldParent);
	}
}

bool ChatDialog::isTyping()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->isTyping();
	}

	return false;
}

bool ChatDialog::hasNewMessages()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->hasNewMessages();
	}

	return false;
}

int ChatDialog::getPeerStatus()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->getPeerStatus();
	}

	return 0;
}

QString ChatDialog::getTitle()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->getTitle();
	}

	return "";
}

void ChatDialog::focusDialog()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		cw->focusDialog();
	}
}

bool ChatDialog::setStyle()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->setStyle();
	}

	return false;
}

const RSStyle *ChatDialog::getStyle()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->getStyle();
	}

	return NULL;
}

void ChatDialog::chatInfoChanged(ChatWidget*)
{
	emit infoChanged(this);
}

void ChatDialog::chatNewMessage(ChatWidget*)
{
	emit newMessage(this);
}

void ChatDialog::insertChatMsgs()
{
	std::string peerId = getPeerId();

	std::list<ChatInfo> newchat;
	if (!rsMsgs->getPrivateChatQueue(true, peerId, newchat)) {
		return;
	}

	std::list<ChatInfo>::iterator it;
	for(it = newchat.begin(); it != newchat.end(); it++)
	 {
		/* are they public? */
		if ((it->chatflags & RS_CHAT_PRIVATE) == 0) {
			/* this should not happen */
			continue;
		}

		addIncomingChatMsg(*it);
	}

	rsMsgs->clearPrivateChatQueue(true, peerId);
}
