/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#include "ChatLobbyUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"

ChatLobbyUserNotify::ChatLobbyUserNotify(QObject *parent) :
    UserNotify(parent)
{
}

bool ChatLobbyUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Chat Lobbies");
	if (group) *group = "ChatLobby";

	return true;
}

QIcon ChatLobbyUserNotify::getIcon()
{
	return QIcon(":/images/chat_32.png");
}

QIcon ChatLobbyUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/chat_red24.png") : QIcon(":/images/chat_32.png");
}

unsigned int ChatLobbyUserNotify::getNewCount()
{
	return mUnreadCount;
}

void ChatLobbyUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::ChatLobby);
}

void ChatLobbyUserNotify::unreadCountChanged(unsigned int unreadCount)
{
	mUnreadCount = unreadCount;

	updateIcon();
}
