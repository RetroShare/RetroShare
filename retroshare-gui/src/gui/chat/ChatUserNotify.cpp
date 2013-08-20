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

#include "ChatUserNotify.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"
#include "gui/chat/ChatDialog.h"

#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>

ChatUserNotify::ChatUserNotify(QObject *parent) :
	UserNotify(parent)
{
	connect(NotifyQt::getInstance(), SIGNAL(privateChatChanged(int, int)), this, SLOT(privateChatChanged(int, int)));
}

bool ChatUserNotify::hasSetting(QString &name)
{
	name = tr("Private Chat");

	return true;
}

bool ChatUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_PRIVATECHAT);
}

bool ChatUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_PRIVATECHAT_COMBINED);
}

bool ChatUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_PRIVATECHAT);
}

void ChatUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_PRIVATECHAT;
	} else {
		notifyFlags &= ~TRAYNOTIFY_PRIVATECHAT;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_PRIVATECHAT_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_PRIVATECHAT_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_PRIVATECHAT;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_PRIVATECHAT;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon ChatUserNotify::getIcon()
{
	return QIcon(":/images/chat.png");
}

QIcon ChatUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/user/friends24_notify.png") : QIcon(":/images/groupchat.png");
}

unsigned int ChatUserNotify::getNewCount()
{
	return rsMsgs->getPrivateChatQueueCount(true);
}

void ChatUserNotify::iconClicked()
{
	ChatDialog *chatDialog = NULL;
	std::list<std::string> ids;
	if (rsMsgs->getPrivateChatQueueIds(true, ids) && ids.size()) {
		chatDialog = ChatDialog::getChat(ids.front(), RS_CHAT_OPEN | RS_CHAT_FOCUS);
	}

	if (chatDialog == NULL) {
		MainWindow::showWindow(MainWindow::Friends);
	}
}

void ChatUserNotify::privateChatChanged(int list, int type)
{
	/* first process the chat messages */
	ChatDialog::chatChanged(list, type);

	if (list == NOTIFY_LIST_PRIVATE_INCOMING_CHAT) {
		updateIcon();
	}
}

