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

#include "MessageUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"

#include "gui/msgs/MessageInterface.h"

MessageUserNotify::MessageUserNotify(QObject *parent) :
	UserNotify(parent)
{
	connect(NotifyQt::getInstance(), SIGNAL(messagesChanged()), this, SLOT(updateIcon()));
}

bool MessageUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Message");
	if (group) *group = "Message";

	return true;
}

QIcon MessageUserNotify::getIcon()
{
    return QIcon(":/icons/png/messages.png");
}

QIcon MessageUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/icons/png/messages-notify.png") : QIcon(":/icons/png/messages.png");
}

unsigned int MessageUserNotify::getNewCount()
{
	uint32_t newInboxCount = 0;
	uint32_t a, b, c, d, e; // dummies
	rsMail->getMessageCount(a, newInboxCount, b, c, d, e);

	return newInboxCount;
}

void MessageUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Messages);
}
