/*******************************************************************************
 * gui/chat/ChatDialog.cpp                                                     *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2011 RetroShare Team <retroshare.project@gmail.com>           *
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

#include <QMessageBox>
#include <QCloseEvent>
#include <QMenu>
#include <QWidgetAction>

#include "ChatDialog.h"
#include "gui/common/PeerDefs.h"
#include "PopupChatDialog.h"
#include "PopupDistantChatDialog.h"
#include "ChatLobbyDialog.h"
#include "PopupChatWindow.h"
#include "gui/settings/rsharesettings.h"
#include "gui/SoundManager.h"

#include <retroshare/rsiface.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

static std::map<ChatId, ChatDialog*> chatDialogsList;

ChatDialog::ChatDialog(QWidget *parent, Qt::WindowFlags flags) :
	QWidget(parent, flags)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
}

ChatDialog::~ChatDialog()
{
    std::map<ChatId, ChatDialog *>::iterator it;
    if (chatDialogsList.end() != (it = chatDialogsList.find(mChatId))) {
        chatDialogsList.erase(it);
	}
}

void ChatDialog::closeEvent(QCloseEvent *event)
{
	if (!canClose()) {
		event->ignore();
		return;
	}
	emit dialogClose(this);
}

void ChatDialog::init(const ChatId &id, const QString &title)
{
    mChatId = id;
	ChatWidget *cw = getChatWidget();
	if (cw) {
        cw->init(id, title);

		connect(cw, SIGNAL(infoChanged(ChatWidget*)), this, SLOT(chatInfoChanged(ChatWidget*)));
		connect(cw, SIGNAL(newMessage(ChatWidget*)), this, SLOT(chatNewMessage(ChatWidget*)));
	}
}

/*static*/ ChatDialog* ChatDialog::getExistingChat(ChatId id)
{
    std::map<ChatId, ChatDialog*>::iterator it;
    if (chatDialogsList.end() != (it = chatDialogsList.find(id))) {
        /* exists already */
        return it->second;
    }

    return NULL;
}

/*static*/ ChatDialog* ChatDialog::getChat(ChatId id, uint chatflags)
{
    if(id.isBroadcast() || id.isNotSet())
        return NULL; // broadcast is not handled by a chat dialog

    /* see if it already exists */
    ChatDialog *cd = getExistingChat(id);

    if (cd == NULL) {

        if(id.isDistantChatId())
            chatflags = RS_CHAT_OPEN | RS_CHAT_FOCUS; // force open for distant chat

        if (chatflags & RS_CHAT_OPEN) {
            if (id.isLobbyId()) {
                ChatLobbyDialog* cld = new ChatLobbyDialog(id.toLobbyId());
                cld->init(ChatId(), "");
                cd = cld;
            } 
            else if(id.isDistantChatId())
            {
                PopupDistantChatDialog* pdcd = new PopupDistantChatDialog(id.toDistantChatId());
                
                pdcd->init(id, "");
                cd = pdcd;
            } 
            else 
            {
                RsPeerDetails sslDetails;
                if (rsPeers->getPeerDetails(id.toPeerId(), sslDetails)) {
                    PopupChatDialog* pcd = new PopupChatDialog();
                    pcd->init(id, PeerDefs::nameWithLocation(sslDetails));
                    cd = pcd;
                }
            }
            if(cd)
                chatDialogsList[id] = cd;
        }
    }

    if (cd == NULL) {
        return NULL;
    }

    cd->showDialog(chatflags);

    return cd;
}

/*static*/ void ChatDialog::cleanupChat()
{
	PopupChatWindow::cleanup();

	/* ChatDialog destuctor removes the entry from the map */
	std::list<ChatDialog*> list;

	std::map<ChatId, ChatDialog*>::iterator it;
	for (it = chatDialogsList.begin(); it != chatDialogsList.end(); ++it) {
		if (it->second) {
			list.push_back(it->second);
		}
	}

	chatDialogsList.clear();

	std::list<ChatDialog*>::iterator it1;
	for (it1 = list.begin(); it1 != list.end(); ++it1) {
		delete (*it1);
	}
}

/*static*/ void ChatDialog::closeChat(const ChatId &chat_id)
{
    ChatDialog *chatDialog = getExistingChat(chat_id);

	if (chatDialog) {
        //delete chatDialog; // ChatDialog removes itself from the map
        chatDialog->deleteLater();
	}
}

/*static*/ void ChatDialog::chatMessageReceived(ChatMessage msg)
{
    if(msg.chat_id.isBroadcast())
        return; // broadcast is not handled by a chat dialog

    if(msg.incoming && (msg.chat_id.isPeerId() || msg.chat_id.isDistantChatId()))
        // play sound when recv a message
        SoundManager::play(SOUND_NEW_CHAT_MESSAGE);

    ChatDialog *cd = getChat(msg.chat_id, Settings->getChatFlags());   
    if(cd)
        cd->addChatMsg(msg);
    else
        std::cerr << "ChatDialog::chatMessageReceived(): no ChatDialog for this message. Ignoring Message: " << msg.msg << std::endl;
}

/*static*/ void ChatDialog::chatFriend(const ChatId &peerId, const bool forceFocus)
{
    getChat(peerId, forceFocus ? RS_CHAT_OPEN | RS_CHAT_FOCUS : RS_CHAT_OPEN);

    // below is the old code witch does lots of error checking.
    // because there are many different chat types, there are also different ways to check if the id is valid
    // i think the chatservice should offer a method bool isChatAvailable(ChatId)
    /*
	if (peerId.isNull()){
		return;
	}

	uint32_t distant_peer_status ;

    if(rsMsgs->getDistantChatStatus(RsGxsId(peerId),distant_peer_status))
	{
		getChat(peerId, forceFocus ? RS_CHAT_OPEN | RS_CHAT_FOCUS : RS_CHAT_OPEN ); // use own flags
		return ;
	}

	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(peerId, lid)) {
		getChat(peerId, (forceFocus ? (RS_CHAT_OPEN | RS_CHAT_FOCUS) : RS_CHAT_OPEN));
	}

	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(peerId, detail))
		return;

	if (detail.isOnlyGPGdetail) {
        // Should not happen
		//chatFriend(detail.gpg_id, forceFocus);
		return;
	}

	getChat(peerId, forceFocus ? RS_CHAT_OPEN | RS_CHAT_FOCUS : RS_CHAT_OPEN);
    */
}

/*static*/ void ChatDialog::chatFriend(const RsPgpId &gpgId, const bool forceFocus)
{
	if (gpgId.isNull()){
		return;
	}

	RsPeerDetails detail;
	if (!rsPeers->getGPGDetails(gpgId, detail))
		return;

	if (!detail.isOnlyGPGdetail) {
		return;
	}

	//let's get the ssl child details
	std::list<RsPeerId> sslIds;
	rsPeers->getAssociatedSSLIds(detail.gpg_id, sslIds);

	if (sslIds.size() == 1) {
		// chat with the one ssl id (online or offline)
        chatFriend(ChatId(sslIds.front()), forceFocus);
		return;
	}

	// more than one ssl ids available, check for online
	std::list<RsPeerId> onlineIds;
	for (std::list<RsPeerId>::iterator it = sslIds.begin(); it != sslIds.end(); ++it) {
		if (rsPeers->isOnline(*it)) {
			onlineIds.push_back(*it);
		}
	}

	if (onlineIds.size() == 1) {
		// chat with the online ssl id
        chatFriend(ChatId(onlineIds.front()), forceFocus);
		return;
	}

    // show menu with online locations
    QMenu menu;
    QLabel* label = new QLabel("<strong>Select one of your friends locations to chat with</strong>");
    QWidgetAction *widgetAction = new QWidgetAction(&menu);
    widgetAction->setDefaultWidget(label);
    menu.addAction(widgetAction);
    QObject cleanupchildren;
    for(std::list<RsPeerId>::iterator it = onlineIds.begin(); it != onlineIds.end(); ++it)
    {
        RsPeerDetails detail;
        rsPeers->getPeerDetails(*it, detail);
        menu.addAction(QString::fromUtf8(detail.location.c_str()), new ChatFriendMethod(&cleanupchildren, *it), SLOT(chatFriend()));
    }
    menu.exec(QCursor::pos());
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
QString ChatDialog::getPeerName(const ChatId& id, QString& additional_info) const
{
    if(id.isPeerId())
	{
		additional_info = QString("Peer ID: ")+QString::fromStdString(id.toPeerId().toStdString());
        return QString::fromUtf8(rsPeers->getPeerName(id.toPeerId()).c_str()) ;
	}
    else
        return "ChatDialog::getPeerName(): invalid id type passed (RsPeerId is required). This is a bug.";
}

QString ChatDialog::getOwnName() const
{
    if(mChatId.isPeerId())
        return QString::fromUtf8(rsPeers->getPeerName(rsPeers->getOwnId()).c_str());
    else
        return "ChatDialog::getOwnName(): invalid id type passed (RsPeerId is required). This is a bug.";
}

void ChatDialog::setPeerStatus(RsStatusValue status)
{
	ChatWidget *cw = getChatWidget();
    if (cw)
    {
        // convert to virtual peer id
        // this is only required for private and distant chat,
        // because lobby and broadcast does not have a status
        RsPeerId vpid;
        if(mChatId.isPeerId())
            vpid = mChatId.toPeerId();
        if(mChatId.isDistantChatId())
            vpid = RsPeerId(mChatId.toDistantChatId());
        cw->updateStatus(QString::fromStdString(vpid.toStdString()), status);
    }
}
RsStatusValue ChatDialog::getPeerStatus()
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		return cw->getPeerStatus();
	}

    return RsStatusValue::RS_STATUS_UNKNOWN;
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
