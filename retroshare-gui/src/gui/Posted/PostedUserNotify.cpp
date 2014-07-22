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

#include "PostedUserNotify.h"
#include "gui/settings/rsharesettings.h"
#include "gui/MainWindow.h"

PostedUserNotify::PostedUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    GxsUserNotify(ifaceImpl, parent)
{
}

bool PostedUserNotify::hasSetting(QString &name)
{
	name = tr("Posted");

	return true;
}

bool PostedUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_POSTED);
}

bool PostedUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_POSTED_COMBINED);
}

bool PostedUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_POSTED);
}

void PostedUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_POSTED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_POSTED;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_POSTED_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_POSTED_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_POSTED;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_POSTED;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon PostedUserNotify::getIcon()
{
	return QIcon(":/images/wikibook_32.png");
}

QIcon PostedUserNotify::getMainIcon(bool hasNew)
{
	//TODO: add new icon
	return hasNew ? QIcon(":/images/wikibook_32.png") : QIcon(":/images/wikibook_32.png");
}

void PostedUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Posted);
}
