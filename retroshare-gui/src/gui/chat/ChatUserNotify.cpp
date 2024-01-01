/*******************************************************************************
 * gui/chat/ChatUserNotify.cpp                                                 *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/FilesDefs.h"
#include "ChatUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"
#include "gui/chat/ChatDialog.h"
#include "gui/settings/rsharesettings.h"

#include <algorithm>
#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>

static std::map<ChatId, int> waitingChats;
static ChatUserNotify* instance = 0;

/*static*/ void ChatUserNotify::getPeersWithWaitingChat(std::vector<RsPeerId> &peers)
{
    for(std::map<ChatId, int>::iterator mit = waitingChats.begin(); mit != waitingChats.end(); ++mit)
    {
        if(mit->first.isPeerId() && std::find(peers.begin(), peers.end(), mit->first.toPeerId()) == peers.end())
            peers.push_back(mit->first.toPeerId());
    }
}

/*static*/ void ChatUserNotify::clearWaitingChat(ChatId id)
{
    std::map<ChatId, int>::iterator mit = waitingChats.find(id);
    if(mit != waitingChats.end())
    {
        waitingChats.erase(mit);
        if(instance)
            instance->updateIcon();
    }
}

ChatUserNotify::ChatUserNotify(QObject *parent) :
    UserNotify(parent)
{
    connect(NotifyQt::getInstance(), SIGNAL(chatMessageReceived(ChatMessage)), this, SLOT(chatMessageReceived(ChatMessage)));
    instance = this;
}

ChatUserNotify::~ChatUserNotify()
{
    instance = 0;
}

bool ChatUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Private Chat");
	if (group) *group = "PrivateChat";

	return true;
}

QIcon ChatUserNotify::getIcon()
{
    return FilesDefs::getIconFromQtResourcePath(":/images/orange-bubble-64.png");
}

QIcon ChatUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/network-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/network.png");
}

unsigned int ChatUserNotify::getNewCount()
{
    int sum = 0;
    for(std::map<ChatId, int>::iterator mit = waitingChats.begin(); mit != waitingChats.end(); ++mit)
    {
        sum += mit->second;
    }
    return sum;
}

QString ChatUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 mentions") : tr("You have %1 mention");
}

QString ChatUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 mentions") : tr("%1 mention");
}

void ChatUserNotify::iconClicked()
{
	ChatDialog *chatDialog = NULL;
    // ChatWidget removes the waiting chat from the list with clearWaitingChat()
    chatDialog = ChatDialog::getChat(waitingChats.begin()->first, RS_CHAT_OPEN | RS_CHAT_FOCUS);

	if (chatDialog == NULL) {
		MainWindow::showWindow(MainWindow::Friends);
	}
    updateIcon();
}

void ChatUserNotify::chatMessageReceived(ChatMessage msg)
{
    if(!msg.chat_id.isBroadcast()
            &&( ChatDialog::getExistingChat(msg.chat_id)
                || (Settings->getChatFlags() & RS_CHAT_OPEN)
                || msg.chat_id.isDistantChatId()))
    {
        ChatDialog::chatMessageReceived(msg);
    }
    else
    {
        // this implicitly counts broadcast messages, because broadcast messages are not handled by chat dialog
        bool found = false;
        for(std::map<ChatId, int>::iterator mit = waitingChats.begin(); mit != waitingChats.end(); ++mit)
        {
            if(msg.chat_id.isSameEndpoint(mit->first))
            {
                mit->second++;
                found = true;
            }
        }
        if(!found)
            waitingChats[msg.chat_id] = 1;
        updateIcon();
    }
}
