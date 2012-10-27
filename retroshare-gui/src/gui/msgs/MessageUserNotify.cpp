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
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"

#include <retroshare/rsmsgs.h>

MessageUserNotify::MessageUserNotify(QObject *parent) :
	UserNotify(parent)
{
	connect(NotifyQt::getInstance(), SIGNAL(messagesChanged()), this, SLOT(updateIcon()));
}

bool MessageUserNotify::hasSetting(QString &name)
{
	name = tr("Message");

	return true;
}

bool MessageUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_MESSAGES);
}

bool MessageUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_MESSAGES_COMBINED);
}

bool MessageUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_MESSAGES);
}

void MessageUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_MESSAGES;
	} else {
		notifyFlags &= ~TRAYNOTIFY_MESSAGES;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_MESSAGES_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_MESSAGES_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_MESSAGES;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_MESSAGES;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon MessageUserNotify::getIcon()
{
	return QIcon(":/images/newmsg.png");
}

QIcon MessageUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/message_new.png") : QIcon(":/images/evolution.png");
}

unsigned int MessageUserNotify::getNewCount()
{
	unsigned int newInboxCount = 0;
	rsMsgs->getMessageCount(NULL, &newInboxCount, NULL, NULL, NULL, NULL);

	return newInboxCount;
}

void MessageUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Messages);
}
