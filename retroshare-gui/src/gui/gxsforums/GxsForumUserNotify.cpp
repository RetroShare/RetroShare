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

#include "GxsForumUserNotify.h"
#include "gui/settings/rsharesettings.h"
#include "gui/MainWindow.h"

GxsForumUserNotify::GxsForumUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    GxsUserNotify(ifaceImpl, parent)
{
}

bool GxsForumUserNotify::hasSetting(QString &name)
{
	name = tr("Forum Post");

	return true;
}

bool GxsForumUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_FORUMS);
}

bool GxsForumUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_FORUMS_COMBINED);
}

bool GxsForumUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_FORUMS);
}

void GxsForumUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_FORUMS;
	} else {
		notifyFlags &= ~TRAYNOTIFY_FORUMS;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_FORUMS_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_FORUMS_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_FORUMS;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_FORUMS;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon GxsForumUserNotify::getIcon()
{
	return QIcon(":/images/konversation16.png");
}

QIcon GxsForumUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/forums_new.png") : QIcon(":/images/konversation.png");
}

void GxsForumUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Forums);
}
